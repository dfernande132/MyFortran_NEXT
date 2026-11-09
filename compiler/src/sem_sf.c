/* sem_sf.c - Funciones de sentencia F(D1, ..., Dn) = expresion (decision 0014, S2c): definicion y expansion de referencias.
 *
 * Sale de sem.c para dejar sitio al semantico en el banco 45 (en el Next vive en el banco 42). Solo habla con sem.c por dos
 * puertas: sf_entrada (de sem.c hacia aqui) y sem_cb (de aqui hacia sem.c); cada una pasa por UN trampolin. Su estado queda
 * en el residente (bss) como el de los demas modulos. */
#include <string.h>
#include "sem.h"
#include "sem_sf.h"
#include "intr.h"
#include "mf_api.h"
#include "source.h"
#include "banco.h"
#include "unid.h"
#include "gen.h"        /* GEN_PAGINA, GEN_SF_OFF, GEN_SF_TAM */

static void error(ErrorSem cod, uint16_t pos)
{
    sem_cb(SEMCB_ERROR, cod, pos);
}

static void error_uni(ErrorUni cod, uint16_t pos)
{
    sem_cb(SEMCB_ERROR_UNI, cod, pos);
}

static uint8_t tipar(uint16_t n)
{
    return (uint8_t)sem_cb(SEMCB_TIPAR, n, 0);
}

static uint8_t numerico(uint8_t t)
{
    return t == TD_INTEGER || t == TD_REAL || t == TD_DOUBLE;
}

static uint8_t tipo_implicito(const char *nombre)
{
    return (nombre[0] >= 'I' && nombre[0] <= 'N') ? TD_INTEGER : TD_REAL;
}

/* ------------------------------------------------------------------
 * Funciones de sentencia: F(D1, ..., Dn) = expresion (decision 0014, S2c)
 *
 * La definicion guarda el cuerpo YA TIPADO (nodos compactados, ficticios convertidos en NODO_ARGF) en la pagina GEN_PAGINA a
 * partir de GEN_SF_OFF; cada referencia F(a1, ..., an) copia esos nodos en el arbol de SU sentencia (indices reubicados) y se
 * convierte en un NODO_SFUN: el generador evalua los argumentos reales y expande el cuerpo. Los argumentos van por valor y
 * su tipo debe ser el del ficticio. Las definiciones se olvidan al abrir la unidad siguiente.
 * ------------------------------------------------------------------ */

#define SF_ARGS_MAX 8

typedef struct {
    char    nombre[9];
    uint8_t nargs;                  /* ficticios                                           */
    uint8_t tipo;                   /* tipo de la funcion (el del nombre)                  */
    uint8_t nnodos;                 /* nodos del cuerpo guardados a continuacion           */
    uint8_t raiz;                   /* indice (1..) de la raiz del cuerpo                  */
    uint8_t dtipo[SF_ARGS_MAX];     /* tipo de cada ficticio                               */
} SFReg;

static uint8_t  sf_nd;
static uint16_t sf_txt[SF_ARGS_MAX];   /* texto (TablaCadenas) del nombre de cada ficticio */
static uint8_t  sf_tip[SF_ARGS_MAX];
static uint16_t sf_base;            /* donde se escribe el nodo siguiente del cuerpo       */
static uint8_t  sf_cnt;
static uint8_t  sf_lleno;

/* Bit 0: el campo a del nodo es un nodo; bit 1: lo es el campo b. */
static uint8_t sf_hijos(uint8_t t)
{
    if ((t >= NODO_ADD && t <= NODO_NEQV) || t == NODO_SFUN || t == NODO_LISTA || t == NODO_RANGO || t == NODO_SUBCAD) {
        return 3;
    }
    if (t == NODO_REF) {
        return 2;
    }
    if (t == NODO_PAREN || t == NODO_NEG || t == NODO_POS || t == NODO_NOT) {
        return 1;
    }
    return 0;
}

static uint8_t sf_buscar(const char *nombre, SFReg *r, uint16_t *off)
{
    uint16_t o = 0;
    uint8_t i;

    for (i = 0; i < sem.sf_n; i++) {
        banco_leer(GEN_PAGINA, (uint16_t)(GEN_SF_OFF + o), r, sizeof(SFReg));
        if (!strcmp(r->nombre, nombre)) {
            *off = o;
            return 1;
        }
        o = (uint16_t)(o + sizeof(SFReg) + r->nnodos * sizeof(Nodo));
    }
    return 0;
}

/* Copia el subarbol n al almacen en orden posfijo (los hijos antes que el padre): devuelve su indice (1..) o 0 si no cabe. */
static uint16_t sf_copiar(uint16_t n)
{
    Nodo x;
    uint8_t h;

    ast_leer(n, &x);                    /* sccz80: sin inicializadores de estructura */
    h = sf_hijos(x.tipo);
    if ((h & 1) && x.a != 0) {
        x.a = sf_copiar(x.a);
    }
    if ((h & 2) && x.b != 0) {
        x.b = sf_copiar(x.b);
    }
    if (sf_lleno || sf_cnt >= 250
        || (uint16_t)(sem.sf_uso + sizeof(SFReg) + (sf_cnt + 1u) * sizeof(Nodo)) > GEN_SF_TAM) {
        sf_lleno = 1;
        return 0;
    }
    banco_escribir(GEN_PAGINA, (uint16_t)(sf_base + sf_cnt * sizeof(Nodo)), &x, sizeof(Nodo));
    sf_cnt++;
    return sf_cnt;
}

/* Ficticio de la definicion en curso: (indice << 4) | tipo, o 0xFF si el nodo (un NODO_NOMBRE) no es uno. */
static uint8_t sf_ficticio(uint16_t nd)
{
    uint8_t k;

    for (k = 0; k < sf_nd; k++) {
        if (!strcmp(tabla_texto(sf_txt[k]), tabla_texto(ast_a(nd)))) {
            return (uint8_t)((k << 4) | sf_tip[k]);
        }
    }
    return 0xFF;
}

/* ¿La asignacion stm.a = stm.b es F(D1, ..., Dn) = expresion de una funcion de sentencia? Solo antes de las sentencias
 * ejecutables y con F desconocida (o declarada, pero aun sin usar) y todos los argumentos nombres. */
static uint8_t sf_es_definicion(void)
{
    uint16_t dst = stm.a;
    const char *nom = tabla_texto(ast_a(dst));
    uint16_t id, c;
    Simbolo s;

    if (ast_tipo(dst) != NODO_REF || sem.ejec || intr_buscar(nom) != 0) {
        return 0;
    }
    id = sim_buscar(nom);
    if (id != 0) {
        sim_leer(id, &s);
        if (SIM_CLASE(&s) != SIM_VARIABLE || (s.flags & (SIMF_FORMAL | SIMF_USADO | SIMF_ASIGNADO)) != 0) {
            return 0;
        }
    }
    for (c = ast_b(dst); c != 0; c = ast_b(c)) {
        if (ast_tipo(ast_a(c)) != NODO_NOMBRE) {
            return 0;
        }
    }
    return 1;
}

static void sf_definir(void)
{
    uint16_t dst = stm.a;
    const char *nom = tabla_texto(ast_a(dst));
    SFReg r, q;
    uint16_t off, c, id;
    Simbolo s;
    uint8_t k, t, tf, te;

    if (sf_buscar(nom, &q, &off)) {
        error_uni(UNI_FSENT_DUPLICADA, ast_pos(dst));
        return;
    }
    memset(&r, 0, sizeof(r));
    strncpy(r.nombre, nom, 8);
    id = sim_buscar(nom);
    if (id != 0) {
        sim_leer(id, &s);
        tf = SIM_TIPO(&s);
    } else {
        tf = tipo_implicito(nom);
    }
    if (tf == TD_CHARACTER || tf == TD_DOUBLE) {
        error(SEM_GEN_NO_SOPORTADO, ast_pos(dst));          /* funciones de sentencia CHARACTER: etapa S3 */
        return;
    }
    sf_nd = 0;
    for (c = ast_b(dst); c != 0; c = ast_b(c)) {
        uint16_t a = ast_a(c);

        if (sf_nd >= SF_ARGS_MAX) {
            error_uni(UNI_MUCHOS_ARGS, ast_pos(a));
            return;
        }
        for (k = 0; k < sf_nd; k++) {
            if (!strcmp(tabla_texto(sf_txt[k]), tabla_texto(ast_a(a)))) {
                error_uni(UNI_ARG_DUPLICADO, ast_pos(a));
                return;
            }
        }
        id = sim_buscar(tabla_texto(ast_a(a)));
        if (id != 0) {
            sim_leer(id, &s);
            if (SIM_CLASE(&s) != SIM_VARIABLE) {
                error(SEM_GEN_NO_SOPORTADO, ast_pos(a));     /* ficticio con el nombre de un array */
                return;
            }
            t = SIM_TIPO(&s);
        } else {
            t = tipo_implicito(tabla_texto(ast_a(a)));
        }
        if (t == TD_CHARACTER || t == TD_DOUBLE) {
            error(SEM_GEN_NO_SOPORTADO, ast_pos(a));         /* ficticios CHARACTER: etapa S3 */
            return;
        }
        sf_txt[sf_nd] = ast_a(a);
        sf_tip[sf_nd] = t;
        sf_nd++;
    }
    sem.sf_activo = 1;
    te = tipar(stm.b);
    sem.sf_activo = 0;
    if (te == TD_NINGUNO) {
        return;
    }
    if (!((numerico(tf) && numerico(te)) || tf == te)) {
        error(SEM_INCOMPATIBLES, ast_pos(dst));
        return;
    }
    if (sem.sf_uso + sizeof(SFReg) + sizeof(Nodo) > GEN_SF_TAM) {
        error_uni(UNI_FSENT_GRANDE, ast_pos(dst));
        return;
    }
    sf_base = (uint16_t)(GEN_SF_OFF + sem.sf_uso + sizeof(SFReg));
    sf_cnt = 0;
    sf_lleno = 0;
    r.raiz = (uint8_t)sf_copiar(stm.b);
    if (sf_lleno) {
        error_uni(UNI_FSENT_GRANDE, ast_pos(dst));
        return;
    }
    r.nargs = sf_nd;
    r.tipo = tf;
    r.nnodos = sf_cnt;
    for (k = 0; k < sf_nd; k++) {
        r.dtipo[k] = sf_tip[k];
    }
    banco_escribir(GEN_PAGINA, (uint16_t)(GEN_SF_OFF + sem.sf_uso), &r, sizeof(SFReg));
    sem.sf_uso = (uint16_t)(sem.sf_uso + sizeof(SFReg) + sf_cnt * sizeof(Nodo));
    sem.sf_n++;
}

/* n es un NODO_REF F(args): si F es una funcion de sentencia, tipa los argumentos, copia el cuerpo en el arbol y lo marca
 * NODO_SFUN. Devuelve el tipo (TD_NINGUNO si hubo error) o 0xFF si F no es una funcion de sentencia. */
static uint8_t sf_referencia(uint16_t n, uint8_t destino)
{
    uint16_t nd = n;
    SFReg r;
    Nodo x;
    uint16_t off, c, base, a;
    uint8_t k = 0, mal = 0, t, i, h;

    if (!sf_buscar(tabla_texto(ast_a(nd)), &r, &off)) {
        return 0xFF;
    }
    if (destino) {
        error(SEM_NO_ES_ARRAY, ast_pos(nd));            /* F(X) = ... otra vez, o en una zona ejecutable */
        return TD_NINGUNO;
    }
    for (c = ast_b(nd); c != 0; c = ast_b(c)) {
        k++;
    }
    if (k != r.nargs) {
        error_uni(UNI_NUM_ARGS, ast_pos(nd));
        return TD_NINGUNO;
    }
    k = 0;
    for (c = ast_b(nd); c != 0; c = ast_b(c)) {
        a = ast_a(c);
        if (ast_tipo(a) == NODO_RANGO) {
            error(SEM_RANGO_NO_PERMITIDO, ast_pos(a));
            mal = 1;
        } else {
            t = tipar(a);
            if (t == TD_NINGUNO) {
                mal = 1;
            } else if (t != r.dtipo[k]) {
                error_uni(UNI_TIPO_ARG, ast_pos(a));
                mal = 1;
            }
        }
        k++;
    }
    if (mal) {
        return TD_NINGUNO;
    }
    base = (uint16_t)(ast_usados() - 1);
    if (base + r.nnodos > AST_NODOS - 1) {
        error_uni(UNI_FSENT_GRANDE, ast_pos(nd));
        return TD_NINGUNO;
    }
    for (i = 0; i < r.nnodos; i++) {
        banco_leer(GEN_PAGINA, (uint16_t)(GEN_SF_OFF + off + sizeof(SFReg) + i * sizeof(Nodo)), &x, sizeof(Nodo));
        h = sf_hijos(x.tipo);
        if ((h & 1) && x.a != 0) {
            x.a = (uint16_t)(x.a + base);
        }
        if ((h & 2) && x.b != 0) {
            x.b = (uint16_t)(x.b + base);
        }
        x.pos = ast_pos(nd);                            /* los errores del generador apuntan a la referencia */
        ast_escribir((uint16_t)(base + 1 + i), &x);
    }
    ast_recortar((uint16_t)(ast_usados() + r.nnodos));
    ast_set_tipo(nd, NODO_SFUN);
    ast_set_a(nd, (uint16_t)(r.raiz + base));
    ast_set_tdato(nd, r.tipo);
    return r.tipo;
}


uint8_t sf_entrada(uint8_t op, uint16_t n, uint8_t destino)
{
    switch (op) {
    case SF_OP_ES_DEF:
        return sf_es_definicion();
    case SF_OP_DEFINIR:
        sf_definir();
        return 0;
    case SF_OP_REF:
        return sf_referencia(n, destino);
    default:
        return sf_ficticio(n);
    }
}
