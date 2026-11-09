/* sem.c - Analisis semantico (ver sem.h). */
#include <string.h>
#include "sem.h"
#include "intr.h"
#include "mf_api.h"
#include "source.h"
#include "banco.h"
#include "unid.h"
#include "sem_sf.h"
#include "sem_decl.h"

/* Generador de codigo (gen.c). Los spikes de medicion del semantico lo dejan fuera
 * con -DMF_SIN_GEN. */
#ifdef MF_SIN_GEN
#define GEN_ETIQUETA(e)
#define GEN_SENTENCIA()
#define GEN_PRINT_ITEM()
#define GEN_PARAM_ITEM()
#define GEN_READ(i)
#define GEN_GOTO_ETQ(e, p)
#define GEN_FIN_SENTENCIA()
#define GEN_INICIAR()
#define GEN_UNIDAD(c, u)
#define GEN_LLAMADA_INICIO(u)
#define GEN_LLAMADA_ARG(k)
#else
#include "gen.h"
#define GEN_ETIQUETA(e)     gen_etiqueta(e)
#define GEN_SENTENCIA()     gen_sentencia()
#define GEN_PRINT_ITEM()    gen_print_item()
#define GEN_PARAM_ITEM()    gen_param_item()
#define GEN_READ(i)         gen_read(i)
#define GEN_GOTO_ETQ(e, p)  gen_goto_etq(e, p)
#define GEN_FIN_SENTENCIA() gen_fin_sentencia()
#define GEN_INICIAR()       gen_iniciar()
#define GEN_UNIDAD(c, u)    gen_unidad(c, u)
#define GEN_LLAMADA_INICIO(u) gen_llamada_inicio(u)
#define GEN_LLAMADA_ARG(k)  gen_llamada_arg(k)
#endif

Sem sem;

/* Pila de bloques abiertos (IF y DO) de la unidad. */
#define BLQ_IF 1
#define BLQ_DO 2
typedef struct {
    uint8_t  tipo;          /* BLQ_IF o BLQ_DO                              */
    uint8_t  con_else;      /* IF: ya se vio el ELSE                        */
    uint32_t etiq;          /* DO: etiqueta de la sentencia terminal        */
    uint16_t linea, col;    /* donde se abrio (para "sin cerrar")           */
} Bloque;

static uint8_t  nbloques;               /* la pila de bloques vive en la pagina de datos SIM_PAGINA_ETQ desde SEM_BLQ_OFF (ahorra 160 B de bss en el Next) */

static void blq_leer(uint8_t i, Bloque *b)
{
    banco_leer(SIM_PAGINA_ETQ, (uint16_t)(SEM_BLQ_OFF + i * sizeof(Bloque)), b, sizeof(Bloque));
}

static void blq_escribir(uint8_t i, const Bloque *b)
{
    banco_escribir(SIM_PAGINA_ETQ, (uint16_t)(SEM_BLQ_OFF + i * sizeof(Bloque)), b, sizeof(Bloque));
}

/* Tabla de etiquetas (pagina de datos SIM_PAGINA_ETQ). */
#define ETQ_DEFINIDA    0x01
#define ETQ_REFERENCIADA 0x02
typedef struct {
    uint32_t etiq;
    uint16_t linea_ref;     /* primera referencia                            */
    uint16_t col_ref;
    uint8_t  flags;
    uint8_t  reservado;
} Etiqueta;

static uint16_t netiquetas;
static uint16_t etq_base;               /* las etiquetas de la unidad en curso son las de indice > etq_base (los indices son del programa) */

/* Las funciones de sentencia viven en sem_sf.c (otro banco en el Next): una sola puerta de entrada */
#define sf_es_definicion()      sf_entrada(SF_OP_ES_DEF, 0, 0)
#define sf_definir()            ((void)sf_entrada(SF_OP_DEFINIR, 0, 0))
#define sf_referencia(n, d)     sf_entrada(SF_OP_REF, (n), (d))

/* y la unica que usa sem_sf.c para llamar a este fichero */
static uint8_t tipar(uint16_t n);
static void error(ErrorSem cod, uint16_t pos);
static void error_uni(ErrorUni cod, uint16_t pos);

uint16_t sem_cb(uint8_t op, uint16_t a, uint16_t b)
{
    switch (op) {
    case SEMCB_ERROR:
        error((ErrorSem)a, b);
        return 0;
    case SEMCB_ERROR_UNI:
        error_uni((ErrorUni)a, b);
        return 0;
    default:
        return tipar(a);
    }
}

/* sem_mensaje: vive en sem_msg.c (banco 47: el texto de los mensajes no cabia en el 45) */
void sem_iniciar(void)
{
    sim_iniciar();
    uni_iniciar();
    mf_diag_iniciar();
    memset(&sem, 0, sizeof(sem));
    nbloques = 0;
    netiquetas = 0;
    etq_base = 0;
    GEN_INICIAR();
}

/* Anota un error en la posicion bruta pos de la sentencia actual. */
static void error(ErrorSem cod, uint16_t pos)
{
    mf_diag_pos((uint8_t)cod, pos);
}

static void error_en(ErrorSem cod, uint16_t linea, uint16_t col)
{
    mf_diag((uint8_t)cod, linea, col);
}

static void error_uni(ErrorUni cod, uint16_t pos)
{
    mf_diag_pos((uint8_t)(MF_COD_UNI + cod), pos);
}

static uint8_t numerico(uint8_t t)
{
    return t == TD_INTEGER || t == TD_REAL || t == TD_DOUBLE;
}

/* Tipo implicito F77: I-N entero, el resto real. */
static uint8_t tipo_implicito(const char *nombre)
{
    return (nombre[0] >= 'I' && nombre[0] <= 'N') ? TD_INTEGER : TD_REAL;
}

/* Busca el nombre; si no existe lo crea con tipo implicito. 0 = tabla llena. */
static uint16_t obtener(const char *nombre, uint16_t pos)
{
    uint16_t id = sim_buscar(nombre);

    if (id == 0) {
        id = sim_alta(nombre, tipo_implicito(nombre), SIM_VARIABLE);
        if (id == 0) {
            error(SEM_TABLA_LLENA, pos);
        }
    }
    return id;
}

static void marcar(uint16_t id, uint8_t flag)
{
    Simbolo s;

    sim_leer(id, &s);
    if ((s.flags & flag) != flag) {
        s.flags |= flag;
        sim_escribir(id, &s);
    }
}

static uint8_t tipar(uint16_t n);

/* Expresion que debe ser INTEGER; un lugar vacio (n == 0) se admite. */
static void indice(uint16_t n)
{
    uint8_t t;

    if (n == 0) {
        return;
    }
    t = tipar(n);
    if (t != TD_NINGUNO && t != TD_INTEGER) {
        error(SEM_ENTERO_ESPERADO, ast_pos(n));
    }
}

/* Segundo parentesis de A(I)(i:j): debe ser UN rango de expresiones enteras. */
static uint8_t subcadena(uint16_t lista)
{
    uint16_t r;

    if (lista == 0 || ast_b(lista) != 0
        || ast_tipo(ast_a(lista)) != NODO_RANGO) {
        return 0;
    }
    r = ast_a(lista);
    indice(ast_a(r));
    indice(ast_b(r));
    return 1;
}

/* Valor de una constante entera con signo opcional; 0 si no lo es. */
static uint8_t constante(uint16_t n, int16_t *v)
{
    uint16_t nd = n;
    uint32_t x;

    if (ast_tipo(nd) == NODO_NEG && ast_tipo(ast_a(nd)) == NODO_ENTERO) {
        x = ast_valor(ast_a(nd));
        if (x > 32768UL) {
            return 0;
        }
        *v = (int16_t)(-(int32_t)x);
        return 1;
    }
    if (ast_tipo(nd) == NODO_ENTERO) {
        x = ast_valor(nd);
        if (x > 32767UL) {
            return 0;
        }
        *v = (int16_t)x;
        return 1;
    }
    return 0;
}

/* Tipo, clase y n.o de dimensiones de un simbolo en variables PEQUENAS: asi las
 * funciones recursivas (tipar, tipar_ref) no llevan un Simbolo de 16 B en cada
 * marco de pila (medido en el Next: pesaba en el peor caso). 0 = no existe. */
static uint16_t datos_simbolo(const char *nombre, uint8_t *tipo, uint8_t *clase, uint8_t *ndim)
{
    Simbolo s;
    uint16_t id = sim_buscar(nombre);

    if (id != 0) {
        sim_leer(id, &s);
        *tipo = SIM_TIPO(&s);
        *clase = SIM_CLASE(&s);
        *ndim = SIM_NDIM(&s);
    }
    return id;
}

/* Comprueba la llamada n (REF) a una INTRINSECA ya con los argumentos tipados
 * (cada uno deja su tipo en tdato). Hoja: no recursa. */
static uint8_t intr_comprobar(const Intrinseca *in, uint16_t n)
{
    uint16_t nd = n;
    uint16_t c, a;
    uint8_t cuantos = 0;
    uint8_t t0 = TD_NINGUNO;
    uint8_t t, ok;

    for (c = ast_b(nd); c != 0; c = ast_b(c)) {
        cuantos++;
    }
    if (cuantos < in->nmin || cuantos > in->nmax) {
        error(SEM_INTR_ARGS, ast_pos(nd));
        return TD_NINGUNO;
    }
    for (c = ast_b(nd); c != 0; c = ast_b(c)) {
        a = ast_a(c);
        t = ast_tdato(a);
        if (t0 == TD_NINGUNO) {
            t0 = t;                         /* tipo del 1.er argumento */
        }
        switch (in->atipo) {
        case INTR_A_NUM:  ok = numerico(t) && t == t0; break;
        case INTR_A_FLT:  ok = (t == TD_REAL || t == TD_DOUBLE) && t == t0; break;
        case INTR_A_NUMX: ok = numerico(t); break;
        default:          ok = (t == in->atipo); break;
        }
        if (!ok) {
            error(SEM_INTR_TIPO, ast_pos(a));
            return TD_NINGUNO;
        }
    }
    t = in->rtipo == INTR_R_ARG ? t0 : in->rtipo;
    ast_set_tdato(nd, t);
    return t;
}

/* Llamada a una intrinseca: tipa los argumentos (recursion) y comprueba. */
static uint8_t tipar_funcion(uint16_t n)
{
    uint16_t c, a;
    uint8_t mal = 0;

    for (c = ast_b(n); c != 0; c = ast_b(c)) {
        a = ast_a(c);
        if (ast_tipo(a) == NODO_RANGO) {
            error(SEM_RANGO_NO_PERMITIDO, ast_pos(a));
            mal = 1;
        } else if (tipar(a) == TD_NINGUNO) {
            mal = 1;                        /* ya se informo */
        }
    }
    if (mal) {
        return TD_NINGUNO;
    }
    return intr_comprobar(intr_buscar(tabla_texto(ast_a(n))), n);
}

/* Longitud declarada de la variable CHARACTER id (hoja: sin Simbolo en los marcos recursivos). */
static uint16_t longitud_de(uint16_t id)
{
    Simbolo s;

    sim_leer(id, &s);
    return (s.flags & SIMF_LONG_ASUM) ? 0 : s.extra;        /* CHARACTER*(*): no se conoce al compilar */
}

/* Longitud de la subcadena rango (NODO_RANGO) de una cadena de longitud cap: la del rango si sus limites son constantes
 * (S(2:4), S(:3), S(2:)); 0 si no se conoce al compilar. */
static uint16_t largo_rango(uint16_t rango, uint16_t cap)
{
    uint16_t r = rango;
    int16_t lo = 1, hi = (int16_t)cap;

    if (cap == 0 && ast_b(r) == 0) {
        return 0;                                       /* S(i:) de un CHARACTER*(*) */
    }
    if ((ast_a(r) != 0 && !constante(ast_a(r), &lo)) || (ast_b(r) != 0 && !constante(ast_b(r), &hi))) {
        return 0;
    }
    return hi >= lo ? (uint16_t)(hi - lo + 1) : 1;
}

/* Longitud en tiempo de compilacion de un argumento real CHARACTER: constante, variable, elemento de array,
 * funcion CHARACTER*n del usuario, subcadena con limites constantes y concatenaciones. 0 = desconocida. */
static uint16_t largo_arg(uint16_t n)
{
    uint16_t nd = n;
    uint16_t x, y;
    Simbolo s;
    Dim dm;

    switch (ast_tipo(nd)) {
    case NODO_PAREN:
        return largo_arg(ast_a(nd));
    case NODO_CADENA:
        return (uint16_t)strlen(tabla_texto(ast_a(nd)));
    case NODO_NOMBRE:
        return longitud_de(sim_buscar(tabla_texto(ast_a(nd))));
    case NODO_REF:
        sim_leer(sim_buscar(tabla_texto(ast_a(nd))), &s);
        if (SIM_CLASE(&s) != SIM_ARRAY) {
            if (ast_tipo(ast_a(ast_b(nd))) == NODO_RANGO) {
                return largo_rango(ast_a(ast_b(nd)), (s.flags & SIMF_LONG_ASUM) ? 0 : s.extra);     /* S(i:j) */
            }
            return s.extra;
        }
        sim_dim_leer(s.extra, &dm);
        return dm.len;
    case NODO_SUBCAD:                                   /* A(k)(i:j) */
        sim_leer(sim_buscar(tabla_texto(ast_a(ast_a(nd)))), &s);
        sim_dim_leer(s.extra, &dm);
        return largo_rango(ast_a(ast_b(nd)), dm.len);
    case NODO_CONCAT:
        x = largo_arg(ast_a(nd));
        y = largo_arg(ast_b(nd));
        if (x == 0 || y == 0) {
            return 0;
        }
        x = (uint16_t)(x + y);
        return x > SEM_CHAR_MAX ? SEM_CHAR_MAX : x;
    default:
        return 0;
    }
}

/* Hoja: ¿el simbolo id es un formal CHARACTER*(*)? */
static uint8_t es_asumida(uint16_t id)
{
    Simbolo s;

    if (id == 0) {
        return 0;
    }
    sim_leer(id, &s);
    return (uint8_t)((s.flags & SIMF_LONG_ASUM) != 0);
}

/* ¿La expresion CHARACTER n es una subcadena (o contiene una de limites no constantes)? Entonces su longitud real solo se
 * sabe al ejecutar y se anota como 255 ("al menos", sin comprobar). */
static uint8_t es_subcadena(uint16_t n)
{
    uint16_t nd = n;

    switch (ast_tipo(nd)) {
    case NODO_PAREN:
        return es_subcadena(ast_a(nd));
    case NODO_NOMBRE:
        return es_asumida(sim_buscar(tabla_texto(ast_a(nd))));      /* CHARACTER*(*): la longitud es la del argumento real */
    case NODO_SUBCAD:
        return 1;
    case NODO_REF:
        return ast_a(ast_b(nd)) != 0 && ast_tipo(ast_a(ast_b(nd))) == NODO_RANGO;
    case NODO_CONCAT:
        return es_subcadena(ast_a(nd)) || es_subcadena(ast_b(nd));
    default:
        return 0;
    }
}

/* Tipa el argumento real a de una llamada y devuelve en kind lo que unid.h espera (tipo | UA_ARRAY | UA_ELEM) y en len el dato
 * extra de un CHARACTER (su longitud; en un array CHARACTER, la de cada elemento) o de un array entero no CHARACTER (sus
 * elementos, saturado a 255 = "no se sabe": ajustable o supuesto). 0 = error ya anotado. */
static uint8_t tipar_arg(uint16_t a, uint8_t *kind, uint8_t *len)
{
    uint16_t nd = a;
    uint16_t id = 0;
    Simbolo s;
    Dim dm;
    uint8_t t;

    *len = 0;
    if (ast_tipo(nd) == NODO_RANGO) {
        error(SEM_RANGO_NO_PERMITIDO, ast_pos(nd));
        return 0;
    }
    if (ast_tipo(nd) == NODO_NOMBRE) {
        id = sim_buscar(tabla_texto(ast_a(nd)));
        if (id != 0) {
            sim_leer(id, &s);
            if (SIM_CLASE(&s) == SIM_ARRAY) {           /* array entero: se pasa por direccion */
                sim_dim_leer(s.extra, &dm);
                marcar(id, SIMF_USADO);
                ast_set_tdato(a, SIM_TIPO(&s));
                *kind = (uint8_t)(SIM_TIPO(&s) | UA_ARRAY);
                if (SIM_TIPO(&s) == TD_CHARACTER) {
                    *len = (uint8_t)dm.len;
                } else {
                    uint32_t n = 255;

                    if (dm.aj == 0) {
                        n = (uint32_t)(dm.sup[0] - dm.inf[0] + 1);
                        if (SIM_NDIM(&s) == 2) {
                            n *= (uint32_t)(dm.sup[1] - dm.inf[1] + 1);
                        }
                    }
                    *len = n > 255 ? 255 : (uint8_t)n;
                }
                return 1;
            }
        }
    }
    t = tipar(a);
    if (t == TD_NINGUNO) {
        return 0;
    }
    if (t == TD_DOUBLE) {
        error(SEM_GEN_NO_SOPORTADO, ast_pos(nd));
        return 0;
    }
    *kind = t;
    if (ast_tipo(nd) == NODO_REF && (id = sim_buscar(tabla_texto(ast_a(nd)))) != 0) {
        sim_leer(id, &s);
        if (SIM_CLASE(&s) == SIM_ARRAY) {               /* elemento de array: valor o direccion segun el formal */
            *kind = (uint8_t)(t | UA_ELEM);
        }
    }
    if (t == TD_CHARACTER) {
        *len = (uint8_t)largo_arg(a);
        if (*len == 0) {
            if (!es_subcadena(a)) {
                error(SEM_GEN_NO_SOPORTADO, ast_pos(nd));
                return 0;
            }
            *len = 255;                                  /* subcadena de limites variables */
        }
    }
    return 1;
}

/* Anota la referencia a una funcion del usuario (llamador -> funcion) para comprobarla al final del fichero. Hoja. */
static void registrar_funcion(uint16_t nd, uint8_t tipo, uint16_t flen, uint8_t n, const uint8_t *kind, const uint8_t *lens)
{
    const char *nom = tabla_texto(ast_a(nd));
    Unidad x;
    uint16_t linea, col;
    uint8_t u;

    posicion_fuente(ast_pos(nd), &linea, &col);
    u = uni_buscar(nom);
    if (u == 0) {
        u = uni_alta(nom, linea, col);
        if (u == 0) {
            error_uni(UNI_TABLA_LLENA, ast_pos(nd));
            return;
        }
    }
    uni_leer(u, &x);
    x.flags |= UF_LLAMADA | UF_FUNREF;
    uni_escribir(u, &x);
    if (sem.unidad != 0 && !uni_llamada(sem.unidad, u, linea, (uint8_t)(col > 255 ? 255 : col), n, kind, lens, tipo, (uint8_t)flen)) {
        error_uni(UNI_MUCHAS_LLAMADAS, ast_pos(nd));
    }
}

/* F(argumentos) con F una funcion del usuario (decision 0014): tipa los argumentos y la anota. El tipo del resultado
 * es el que el llamador da al nombre (declarado o implicito). */
static uint8_t tipar_usuario(uint16_t n, uint8_t tipo, uint16_t flen)
{
    uint16_t nd = n;
    uint16_t c, a;
    uint8_t kind[UNI_ARGS_MAX];
    uint8_t lens[UNI_ARGS_MAX];
    uint8_t cuantos = 0;
    uint8_t mal = 0;

    if (tipo == TD_DOUBLE) {
        error(SEM_GEN_NO_SOPORTADO, ast_pos(nd));
        return TD_NINGUNO;
    }
    for (c = ast_b(nd); c != 0; c = ast_b(c)) {
        a = ast_a(c);
        if (cuantos >= UNI_ARGS_MAX) {
            error_uni(UNI_MUCHOS_ARGS, ast_pos(a));
            mal = 1;
        } else if (!tipar_arg(a, &kind[cuantos], &lens[cuantos])) {
            mal = 1;
        } else {
            cuantos++;
        }
    }
    if (mal) {
        return TD_NINGUNO;
    }
    registrar_funcion(nd, tipo, flen, cuantos, kind, lens);
    ast_set_tdato(nd, tipo);
    return tipo;
}


/* Variable o elemento: A, A(I), A(I,J), S(2:3), A(I)(2:3). destino: 1 si se asigna. */
static uint8_t tipar_ref(uint16_t n, uint8_t destino)
{
    uint16_t nd = n;
    uint16_t id, c, a;
    uint8_t cuantos = 0;
    uint8_t es_rango = 0;
    uint8_t tipo, clase, ndim;

    if (sem.sf_n != 0) {
        tipo = sf_referencia(n, destino);
        if (tipo != 0xFF) {
            return tipo;
        }
    }
    id = datos_simbolo(tabla_texto(ast_a(nd)), &tipo, &clase, &ndim);
    if (id == 0) {
        if (intr_buscar(tabla_texto(ast_a(nd))) != 0) {
            if (destino) {
                error(SEM_NO_ES_ARRAY, ast_pos(nd));    /* SQRT(X) = 1 */
                return TD_NINGUNO;
            }
            return tipar_funcion(n);                /* funcion intrinseca */
        }
        if (destino) {
            error(SEM_NO_ES_ARRAY, ast_pos(nd));        /* F(X) = ...: funcion de sentencia (etapa S2c) */
            return TD_NINGUNO;
        }
        return tipar_usuario(n, tipo_implicito(tabla_texto(ast_a(nd))), 0);    /* F(X): funcion del usuario */
    }
    if (clase == SIM_FUNCION) {
        if (!destino && intr_buscar(tabla_texto(ast_a(nd))) != 0) {
            return tipar_funcion(n);        /* INTRINSIC SQRT: se llama igual */
        }
        error(SEM_FUNCION, ast_pos(nd));
        return TD_NINGUNO;
    }
    for (c = ast_b(nd); c != 0; c = ast_b(c)) {
        cuantos++;
        if (ast_tipo(ast_a(c)) == NODO_RANGO) {
            es_rango = 1;
        }
    }
    if (clase == SIM_VARIABLE) {
        /* solo vale S(i:j) con S CHARACTER */
        if (es_rango && cuantos == 1 && tipo == TD_CHARACTER) {
            subcadena(ast_b(nd));
            marcar(id, destino ? SIMF_ASIGNADO : SIMF_USADO);
            ast_set_tdato(nd, TD_CHARACTER);
            return TD_CHARACTER;
        }
        if (!destino && !es_rango) {
            return tipar_usuario(n, tipo, longitud_de(id));  /* una variable con tipo declarado, usada como funcion */
        }
        error(es_rango ? SEM_SUBCADENA : SEM_NO_ES_ARRAY, ast_pos(nd));
        return TD_NINGUNO;
    }
    if (cuantos != ndim) {
        error(SEM_NUM_INDICES, ast_pos(nd));
        return TD_NINGUNO;
    }
    for (c = ast_b(nd); c != 0; c = ast_b(c)) {
        a = ast_a(c);
        if (ast_tipo(a) == NODO_RANGO) {
            error(SEM_RANGO_NO_PERMITIDO, ast_pos(a));
        } else {
            indice(a);
        }
    }
    marcar(id, destino ? SIMF_ASIGNADO : SIMF_USADO);
    ast_set_tdato(nd, tipo);
    return tipo;
}

static uint8_t tipar_binario(uint16_t nd, uint16_t n)
{
    uint8_t a = tipar(ast_a(nd));
    uint8_t b = tipar(ast_b(nd));
    uint8_t t = TD_NINGUNO;

    (void)n;
    if (a == TD_NINGUNO || b == TD_NINGUNO) {
        return TD_NINGUNO;                  /* ya se informo */
    }
    switch (ast_tipo(nd)) {
    case NODO_ADD: case NODO_SUB: case NODO_MUL: case NODO_DIV: case NODO_POW:
        if (!numerico(a) || !numerico(b)) {
            error(SEM_NUMERICO_ESPERADO, ast_pos(nd));
        } else {
            t = a > b ? a : b;              /* INTEGER < REAL < DOUBLE */
        }
        break;
    case NODO_CONCAT:
        if (a != TD_CHARACTER || b != TD_CHARACTER) {
            error(SEM_TEXTO_ESPERADO, ast_pos(nd));
        } else {
            t = TD_CHARACTER;
        }
        break;
    case NODO_EQ: case NODO_NE: case NODO_LT: case NODO_LE: case NODO_GT: case NODO_GE:
        if ((numerico(a) && numerico(b)) || (a == TD_CHARACTER && b == TD_CHARACTER)) {
            t = TD_LOGICAL;
        } else {
            error(SEM_INCOMPATIBLES, ast_pos(nd));
        }
        break;
    default:                                /* .AND. .OR. .EQV. .NEQV. */
        if (a != TD_LOGICAL || b != TD_LOGICAL) {
            error(SEM_LOGICO_ESPERADO, ast_pos(nd));
        } else {
            t = TD_LOGICAL;
        }
        break;
    }
    return t;
}

/* Variable sola dentro de una expresion (fuera de tipar: ver datos_simbolo). */
static uint8_t tipar_nombre(uint16_t nd)
{
    Simbolo s;                  /* esta funcion es una hoja: no recursa */
    uint16_t id = obtener(tabla_texto(ast_a(nd)), ast_pos(nd));

    if (id == 0) {
        return TD_NINGUNO;
    }
    sim_leer(id, &s);
    if (s.flags & SIMF_PROC) {
        error(SEM_PROC_ARG, ast_pos(nd));       /* EXTERNAL/INTRINSIC usado como valor o argumento */
        return TD_NINGUNO;
    }
    if (SIM_CLASE(&s) == SIM_ARRAY) {
        error(SEM_ARRAY_SIN_INDICES, ast_pos(nd));
        return TD_NINGUNO;
    }
    marcar(id, SIMF_USADO);
    return SIM_TIPO(&s);
}

/* Tipo de la expresion n (y de todos sus nodos); TD_NINGUNO si hubo error. */
static uint8_t tipar(uint16_t n)
{
    uint16_t nd = n;
    uint8_t t = TD_NINGUNO;

    switch (ast_tipo(nd)) {
    case NODO_ENTERO:
        if (ast_valor(nd) > 32767UL) {
            error(SEM_ENTERO_GRANDE, ast_pos(nd));
        } else {
            t = TD_INTEGER;
        }
        break;
    case NODO_REAL:    t = TD_REAL; break;
    case NODO_DOBLE:   t = TD_DOUBLE; break;
    case NODO_CADENA:  t = TD_CHARACTER; break;
    case NODO_LOGICO:  t = TD_LOGICAL; break;
    case NODO_NOMBRE:
        if (sem.sf_activo) {                                    /* ficticio de la funcion de sentencia? */
            t = sf_entrada(SF_OP_FICT, n, 0);
            if (t != 0xFF) {
                ast_set_tipo(nd, NODO_ARGF);
                ast_set_a(nd, t >> 4);
                t = t & 15;
                break;
            }
        }
        t = tipar_nombre(nd);
        break;
    case NODO_ARGF:
    case NODO_SFUN:
        t = ast_tdato(nd);
        break;
    case NODO_REF:
        t = tipar_ref(n, 0);
        break;
    case NODO_SUBCAD:
        t = tipar_ref(ast_a(nd), 0);
        if (t != TD_NINGUNO && t != TD_CHARACTER) {
            error(SEM_SUBCADENA, ast_pos(nd));
            t = TD_NINGUNO;
        } else if (t != TD_NINGUNO && !subcadena(ast_b(nd))) {
            error(SEM_SUBCADENA, ast_pos(nd));
            t = TD_NINGUNO;
        }
        break;
    case NODO_PAREN:
        t = tipar(ast_a(nd));
        break;
    case NODO_NEG:
    case NODO_POS:
        if (ast_tipo(nd) == NODO_NEG && ast_tipo(ast_a(nd)) == NODO_ENTERO
            && ast_valor(ast_a(nd)) == 32768UL) {
            ast_set_tdato(ast_a(nd), TD_INTEGER);
            t = TD_INTEGER;                 /* -32768 */
            break;
        }
        t = tipar(ast_a(nd));
        if (t != TD_NINGUNO && !numerico(t)) {
            error(SEM_NUMERICO_ESPERADO, ast_pos(nd));
            t = TD_NINGUNO;
        }
        break;
    case NODO_NOT:
        t = tipar(ast_a(nd));
        if (t != TD_NINGUNO && t != TD_LOGICAL) {
            error(SEM_LOGICO_ESPERADO, ast_pos(nd));
            t = TD_NINGUNO;
        }
        break;
    case NODO_RANGO:
        error(SEM_RANGO_NO_PERMITIDO, ast_pos(nd));
        break;
    default:
        t = tipar_binario(nd, n);
        break;
    }
    ast_set_tdato(nd, t);
    return t;
}

/* Expresion que debe tener uno de los tipos de una clase. */
static void exigir_numerica(uint16_t n)
{
    uint8_t t = tipar(n);

    if (t != TD_NINGUNO && !numerico(t)) {
        error(SEM_NUMERICO_ESPERADO, ast_pos(n));
    }
}

static void exigir_logica(uint16_t n)
{
    uint8_t t = tipar(n);

    if (t != TD_NINGUNO && t != TD_LOGICAL) {
        error(SEM_LOGICO_ESPERADO, ast_pos(n));
    }
}

/* ------------------------------------------------------------------
 * Declaraciones
 * ------------------------------------------------------------------ */

/* Longitud CHARACTER de un nodo constante; 0 si no vale. */
/* ------------------------------------------------------------------
 * Sentencias
 * ------------------------------------------------------------------ */

static uint8_t sf_definida;          /* la sentencia en curso es una definicion de funcion de sentencia (no genera codigo) */

static void asignacion(void)
{
    uint16_t dst = stm.a;
    uint8_t td = TD_NINGUNO;
    uint8_t te;
    uint16_t id;
    Simbolo s;

    if (sf_es_definicion()) {
        sf_definir();
        sf_definida = 1;
        return;
    }
    if (ast_tipo(dst) == NODO_NOMBRE) {
        id = obtener(tabla_texto(ast_a(dst)), ast_pos(dst));
        if (id != 0) {
            sim_leer(id, &s);
            if (SIM_CLASE(&s) == SIM_ARRAY) {
                error(SEM_ARRAY_SIN_INDICES, ast_pos(dst));
            } else if (SIM_ES_PARAM(&s)) {
                error(SEM_PARAM_ASIGNA, ast_pos(dst));
            } else {
                marcar(id, SIMF_ASIGNADO);
                td = SIM_TIPO(&s);
            }
        }
    } else if (ast_tipo(dst) == NODO_REF) {
        td = tipar_ref(stm.a, 1);
    } else {                                    /* SUBCAD */
        td = tipar_ref(ast_a(dst), 1);
        if (td != TD_NINGUNO && td != TD_CHARACTER) {
            error(SEM_SUBCADENA, ast_pos(dst));
            td = TD_NINGUNO;
        } else if (td != TD_NINGUNO && !subcadena(ast_b(dst))) {
            error(SEM_SUBCADENA, ast_pos(dst));
            td = TD_NINGUNO;
        }
    }
    te = tipar(stm.b);
    if (td == TD_NINGUNO || te == TD_NINGUNO) {
        return;
    }
    if (!((numerico(td) && numerico(te)) || td == te)) {
        error(SEM_INCOMPATIBLES, ast_pos(dst));
    }
}

/* Elemento de la lista de un READ (stm.e): una variable escalar o un elemento de array, no PARAMETER. 1 = valido. */
static uint8_t elemento_lectura(void)
{
    uint16_t nd = stm.e;
    uint16_t id;
    Simbolo s;
    uint8_t td;

    if (ast_tipo(nd) == NODO_NOMBRE) {
        id = obtener(tabla_texto(ast_a(nd)), ast_pos(nd));
        if (id == 0) {
            return 0;
        }
        sim_leer(id, &s);
        if (SIM_CLASE(&s) == SIM_ARRAY) {
            error(SEM_GEN_NO_SOPORTADO, ast_pos(nd));       /* un array entero: aun no */
        } else if (SIM_ES_PARAM(&s) || (s.flags & SIMF_PROC)) {
            error(SEM_PARAM_ASIGNA, ast_pos(nd));
        } else if (SIM_TIPO(&s) == TD_DOUBLE) {
            error(SEM_GEN_NO_SOPORTADO, ast_pos(nd));
        } else {
            marcar(id, SIMF_ASIGNADO);
            return 1;
        }
        return 0;
    }
    if (ast_tipo(nd) == NODO_REF) {
        id = sim_buscar(tabla_texto(ast_a(nd)));
        if (id != 0) {
            sim_leer(id, &s);
            if (SIM_CLASE(&s) != SIM_ARRAY) {
                error(SEM_NO_ES_ARRAY, ast_pos(nd));        /* F(X) o S(i:j): no son destinos validos */
                return 0;
            }
        }
        td = tipar_ref(stm.e, 1);
        return td != TD_NINGUNO;
    }
    error(SEM_LECTURA_ITEM, ast_pos(nd));
    return 0;
}

static void bucle_do(void)
{
    uint16_t id = obtener(tabla_texto(stm.nombre), ast_pos(stm.a));
    Simbolo s;

    if (id != 0) {
        sim_leer(id, &s);
        if (SIM_ES_PARAM(&s)) {
            error(SEM_PARAM_ASIGNA, ast_pos(stm.a));
        } else if (SIM_CLASE(&s) != SIM_VARIABLE || !numerico(SIM_TIPO(&s))) {
            error(SEM_VARIABLE_DO, ast_pos(stm.a));
        } else {
            marcar(id, SIMF_ASIGNADO | SIMF_USADO);
        }
    }
    exigir_numerica(stm.a);
    exigir_numerica(stm.b);
    if (stm.c != 0) {
        exigir_numerica(stm.c);
    }
}

/* ------------------------------------------------------------------
 * Etiquetas
 * ------------------------------------------------------------------ */

static void etq_leer(uint16_t i, Etiqueta *e)
{
    banco_leer(SIM_PAGINA_ETQ, (uint16_t)((i - 1) * sizeof(Etiqueta)), e, sizeof(Etiqueta));
}

static void etq_escribir(uint16_t i, const Etiqueta *e)
{
    banco_escribir(SIM_PAGINA_ETQ, (uint16_t)((i - 1) * sizeof(Etiqueta)), e, sizeof(Etiqueta));
}

/* Indice (1..) de la etiqueta; la crea si no esta. 0 = tabla llena. */
static uint16_t etq_obtener(uint32_t etiq, uint16_t pos)
{
    Etiqueta e;
    uint16_t i;

    for (i = (uint16_t)(etq_base + 1); i <= netiquetas; i++) {
        etq_leer(i, &e);
        if (e.etiq == etiq) {
            return i;
        }
    }
    if (netiquetas >= SEM_ETIQUETAS_MAX) {
        error(SEM_MUCHAS_ETIQUETAS, pos);
        return 0;
    }
    memset(&e, 0, sizeof(e));
    e.etiq = etiq;
    netiquetas++;
    etq_escribir(netiquetas, &e);
    return netiquetas;
}

/* Una sentencia hace referencia a la etiqueta (pos bruta del uso). */
static void referencia(uint32_t etiq, uint16_t pos)
{
    Etiqueta e;
    uint16_t i = etq_obtener(etiq, pos);

    if (i == 0) {
        return;
    }
    etq_leer(i, &e);
    if (!(e.flags & ETQ_REFERENCIADA)) {
        e.flags |= ETQ_REFERENCIADA;
        posicion_fuente(pos, &e.linea_ref, &e.col_ref);
        etq_escribir(i, &e);
    }
}

/* La sentencia actual lleva esta etiqueta. */
static void definicion(uint32_t etiq)
{
    Etiqueta e;
    uint16_t i = etq_obtener(etiq, 0);

    if (i == 0) {
        return;
    }
    etq_leer(i, &e);
    if (e.flags & ETQ_DEFINIDA) {
        error(SEM_ETIQUETA_DUPLICADA, 0);
        return;
    }
    e.flags |= ETQ_DEFINIDA;
    etq_escribir(i, &e);
}

uint16_t sem_etq_indice(uint32_t etiq)
{
    return etq_obtener(etiq, 0);
}

uint32_t sem_etq_valor(uint16_t indice)
{
    Etiqueta e;

    etq_leer(indice, &e);
    return e.etiq;
}

static uint8_t etq_definida(uint32_t etiq)
{
    Etiqueta e;
    uint16_t i;

    for (i = (uint16_t)(etq_base + 1); i <= netiquetas; i++) {
        etq_leer(i, &e);
        if (e.etiq == etiq) {
            return (e.flags & ETQ_DEFINIDA) != 0;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------
 * Bloques IF y DO
 * ------------------------------------------------------------------ */

static void abrir(uint8_t tipo, uint32_t etiq)
{
    Bloque b;
    uint16_t linea, col;

    if (nbloques >= SEM_BLOQUES_MAX) {
        error(SEM_BLOQUES_PROFUNDOS, 0);
        return;
    }
    posicion_fuente(lex_saltar_blancos(0), &linea, &col);
    b.tipo = tipo;
    b.con_else = 0;
    b.etiq = etiq;
    b.linea = linea;
    b.col = col;
    blq_escribir(nbloques++, &b);
}

/* Descarta los DO que quedan abiertos encima de un IF (cada uno es un error). */
static void descartar_do(void)
{
    Bloque b;

    while (nbloques > 0) {
        blq_leer((uint8_t)(nbloques - 1), &b);
        if (b.tipo != BLQ_DO) {
            break;
        }
        nbloques--;
        error_en(SEM_DO_SIN_CERRAR, b.linea, b.col);
    }
}

/* Sentencias que no pueden ser el final de un DO (X3.9-1978 11.10.3, de memoria,
 * SIN VERIFICAR contra el texto). El IF logico se comprueba aparte. */
static uint8_t terminal_valido(void)
{
    switch (stm.tipo) {
    case SENT_GOTO:
        return stm.modo == 1;                 /* el calculado si vale */
    case SENT_IF_ARITMETICO: case SENT_IF_BLOQUE: case SENT_ELSEIF: case SENT_ELSE:
    case SENT_ENDIF: case SENT_END: case SENT_STOP: case SENT_DO:
        return 0;
    default:
        return 1;
    }
}

/* La sentencia actual lleva etiqueta: cierra los DO que terminan en ella. */
static void cerrar_do_por_etiqueta(void)
{
    uint8_t i;
    uint8_t cerro = 0;
    Bloque b;

    while (nbloques > 0) {
        blq_leer((uint8_t)(nbloques - 1), &b);
        if (b.tipo != BLQ_DO || b.etiq != stm.etiqueta) {
            break;
        }
        nbloques--;
        cerro = 1;
    }
    if (cerro && !terminal_valido()) {
        error(SEM_DO_TERMINAL, 0);
    }
    /* un DO de esa etiqueta mas abajo, tapado por un IF: mal anidado */
    for (i = nbloques; i > 0; i--) {
        blq_leer((uint8_t)(i - 1), &b);
        if (b.tipo == BLQ_DO && b.etiq == stm.etiqueta) {
            error(SEM_ANIDAMIENTO, 0);
            nbloques = (uint8_t)(i - 1);
            break;
        }
    }
}

static void cerrar_unidad(void)
{
    Etiqueta e;
    Unidad x;
    Simbolo s;
    uint16_t i;
    uint8_t k;
    Bloque b;

    while (nbloques > 0) {
        nbloques--;
        blq_leer(nbloques, &b);
        error_en(b.tipo == BLQ_IF ? SEM_IF_SIN_CERRAR : SEM_DO_SIN_CERRAR, b.linea, b.col);
    }
    for (i = (uint16_t)(etq_base + 1); i <= netiquetas; i++) {
        etq_leer(i, &e);
        if ((e.flags & ETQ_REFERENCIADA) && !(e.flags & ETQ_DEFINIDA)) {
            error_en(SEM_ETIQUETA_INDEFINIDA, e.linea_ref, e.col_ref);
        }
    }
    if (sem.unidad != 0) {                       /* lo que las demas unidades necesitan saber de esta */
        uni_leer(sem.unidad, &x);
        x.sim_fin = sim_cuenta();
        if (x.sim_res != 0) {
            sim_leer(x.sim_res, &s);
            x.tipo = SIM_TIPO(&s);
            x.flen = x.tipo == TD_CHARACTER ? (uint8_t)s.extra : 0;
        }
        for (k = 0; k < x.nformales; k++) {
            sim_leer((uint16_t)(x.sim_ini + k), &s);
            x.ftipo[k] = (uint8_t)(SIM_TIPO(&s) | (SIM_CLASE(&s) == SIM_ARRAY ? UA_ARRAY : 0));
        }
        uni_escribir(sem.unidad, &x);
    }
    sem.fin = 1;
    sem.abierta = 0;
}

/* Efecto de la sentencia en la pila de bloques. */
static void bloques(void)
{
    Bloque b;

    switch (stm.tipo) {
    case SENT_IF_BLOQUE:
        abrir(BLQ_IF, 0);
        break;
    case SENT_ELSEIF: case SENT_ELSE: case SENT_ENDIF:
        descartar_do();
        if (nbloques == 0) {
            error(SEM_SIN_IF, 0);
            break;
        }
        blq_leer((uint8_t)(nbloques - 1), &b);
        if (stm.tipo == SENT_ENDIF) {
            nbloques--;
        } else if (b.con_else) {
            error(SEM_ELSE_REPETIDO, 0);
        } else if (stm.tipo == SENT_ELSE) {
            b.con_else = 1;
            blq_escribir((uint8_t)(nbloques - 1), &b);
        }
        break;
    case SENT_DO:
        referencia(stm.l1, 0);
        if (etq_definida(stm.l1)) {
            error(SEM_DO_ETIQUETA_ATRAS, 0);
        }
        abrir(BLQ_DO, stm.l1);
        break;
    case SENT_END:
        cerrar_unidad();
        break;
    default:
        break;
    }
}

void sem_fin_fichero(uint16_t linea)
{
    if (!sem.fin) {
        error_en(SEM_FALTA_END, linea, 0);
    }
    uni_comprobar();
}

/* ------------------------------------------------------------------
 * Unidades de programa y llamadas (decision 0014)
 * ------------------------------------------------------------------ */

/* Primera sentencia de una unidad: la da de alta en el directorio (con el nombre de su cabecera, o sin nombre si es el
 * programa principal sin PROGRAM) y la abre en el generador. */
static void abrir_unidad(void)
{
    Unidad x;
    uint8_t u = 0;
    uint8_t clase = stm.tipo == SENT_SUBROUTINE ? UK_SUBRUTINA : stm.tipo == SENT_FUNCTION ? UK_FUNCION : UK_PROGRAMA;
    const char *nombre = ((stm.tipo == SENT_PROGRAM || stm.tipo == SENT_SUBROUTINE || stm.tipo == SENT_FUNCTION) && stm.err == STM_OK)
                         ? tabla_texto(stm.nombre) : "";
    uint16_t linea, col;

    sim_unidad();
    etq_base = netiquetas;
    nbloques = 0;
    sem.fin = 0;
    sem.nsent = 0;
    sem.ejec = 0;
    sem.sf_n = 0;
    sem.sf_uso = 0;
    posicion_fuente(lex_saltar_blancos(0), &linea, &col);
    if (nombre[0] != '\0') {
        u = uni_buscar(nombre);
    }
    if (u != 0) {
        uni_leer(u, &x);
        if (x.clase != UK_NADA) {
            error_uni(UNI_YA_DEFINIDA, 0);
            u = 0;
        }
    }
    if (u == 0) {
        u = uni_alta(nombre, linea, col);
        if (u == 0) {
            error_uni(UNI_TABLA_LLENA, 0);
            return;
        }
    }
    uni_leer(u, &x);
    x.clase = clase;
    x.linea = linea;
    x.col = col;
    x.sim_ini = (uint16_t)(sim_cuenta() + 1);
    x.nformales = 0;
    x.sim_res = 0;
    x.tipo = 0;
    uni_escribir(u, &x);
    sem.unidad = u;
    sem.clase = clase;
    sem.abierta = 1;
    GEN_UNIDAD(clase, u);
}

/* SUBROUTINE nombre ( formales ): cada formal es un simbolo escalar de la unidad (tipo implicito hasta que se declare). */
static void formales(void)
{
    Unidad x;
    uint8_t n = 0;

    while (stmt_elemento()) {
        uint16_t nd = stm.e;
        const char *nom = tabla_texto(ast_a(nd));
        uint16_t id;

        if (sim_buscar(nom) != 0) {
            error_uni(UNI_ARG_DUPLICADO, ast_pos(nd));
        } else if (n >= UNI_ARGS_MAX) {
            error_uni(UNI_MUCHOS_ARGS, ast_pos(nd));
        } else {
            id = sim_alta(nom, tipo_implicito(nom), SIM_VARIABLE);
            if (id == 0) {
                error(SEM_TABLA_LLENA, ast_pos(nd));
            } else {
                marcar(id, SIMF_FORMAL);
                n++;
            }
        }
    }
    if (sem.unidad != 0) {
        uni_leer(sem.unidad, &x);
        x.nformales = n;
        if (stm.tipo == SENT_FUNCTION && stm.err == STM_OK) {          /* la variable resultado: tiene el nombre de la funcion */
            const char *nom = tabla_texto(stm.nombre);
            uint8_t t = tipo_implicito(nom);
            uint16_t id;

            if (stm.ftipo == SENT_INTEGER) t = TD_INTEGER;
            else if (stm.ftipo == SENT_REAL) t = TD_REAL;
            else if (stm.ftipo == SENT_LOGICAL) t = TD_LOGICAL;
            else if (stm.ftipo == SENT_DOUBLEPRECISION) t = TD_DOUBLE;
            else if (stm.ftipo == SENT_CHARACTER) t = TD_CHARACTER;
            if (t == TD_DOUBLE) {
                error(SEM_GEN_NO_SOPORTADO, stm.resto);
            }
            id = sim_alta(nom, t, SIM_VARIABLE);
            if (id == 0) {
                error_uni(UNI_ARG_DUPLICADO, stm.resto);           /* F(F): el resultado repite un formal */
            } else {
                if (t == TD_CHARACTER) {
                    Simbolo s;

                    if (stm.flen > 255) {
                        error(SEM_LONGITUD, stm.resto);
                    }
                    sim_leer(id, &s);
                    s.extra = stm.flen > 255 || stm.flen == 0 ? 1 : stm.flen;
                    sim_escribir(id, &s);
                }
                marcar(id, (uint8_t)(SIMF_FORMAL | (stm.ftipo != 0 ? SIMF_TIPO_EXPLICITO : 0)));
                x.sim_res = id;
                x.tipo = t;
            }
        }
        uni_escribir(sem.unidad, &x);
    }
}

/* CALL nombre ( argumentos ): tipa cada argumento y lo pasa al generador; anota la llamada para comprobarla al final. */
static void llamada(void)
{
    const char *nom = tabla_texto(stm.nombre);
    uint16_t linea, col;
    uint8_t kind[UNI_ARGS_MAX];
    uint8_t lens[UNI_ARGS_MAX];
    uint8_t n = 0;
    uint8_t u;
    uint8_t mal = 0;                    /* un argumento con error: no se anota la llamada (evita errores en cascada) */

    posicion_fuente(stm.resto, &linea, &col);
    u = uni_buscar(nom);
    if (u == 0) {
        u = uni_alta(nom, linea, col);
        if (u == 0) {
            error_uni(UNI_TABLA_LLENA, stm.resto);
        }
    }
    GEN_LLAMADA_INICIO(u);
    while (stmt_elemento()) {
        if (n >= UNI_ARGS_MAX) {
            error_uni(UNI_MUCHOS_ARGS, ast_pos(stm.e));
            mal = 1;
        } else if (!tipar_arg(stm.e, &kind[n], &lens[n])) {
            mal = 1;
        } else {
            GEN_LLAMADA_ARG(n);
            n++;
        }
    }
    if (stm.err == STM_OK && !mal && sem.unidad != 0 && u != 0
        && !uni_llamada(sem.unidad, u, linea, (uint8_t)(col > 255 ? 255 : col), n, kind, lens, 0, 0)) {
        error_uni(UNI_MUCHAS_LLAMADAS, stm.resto);
    }
}

/* Tipado y referencias a etiquetas de la sentencia (sin bloques). */
static void cuerpo(void)
{
    uint8_t tipo;

    sf_definida = 0;
    switch (stm.tipo) {
    case SENT_INTEGER: case SENT_REAL: case SENT_DOUBLEPRECISION: case SENT_LOGICAL: case SENT_CHARACTER:
    case SENT_PROGRAM: case SENT_SUBROUTINE: case SENT_FUNCTION: case SENT_VACIA: case SENT_END: case SENT_ASIGNACION:
    case SENT_DIMENSION: case SENT_EXTERNAL: case SENT_INTRINSIC: case SENT_PARAMETER: case SENT_DATA:
        break;
    default:
        sem.ejec = 1;                   /* ya no caben funciones de sentencia */
        break;
    }
    switch (stm.tipo) {
    case SENT_INTEGER:         tipo = TD_INTEGER; goto declaracion;
    case SENT_REAL:            tipo = TD_REAL; goto declaracion;
    case SENT_DOUBLEPRECISION: tipo = TD_DOUBLE; goto declaracion;
    case SENT_LOGICAL:         tipo = TD_LOGICAL; goto declaracion;
    case SENT_CHARACTER:       tipo = TD_CHARACTER;
    declaracion:
        while (stmt_elemento()) {
            decl_entrada(DECL_OP_TIPO, tipo, stm.e);
        }
        break;
    case SENT_DIMENSION: tipo = DECL_OP_DIM; goto decl_op;
    case SENT_EXTERNAL:  tipo = DECL_OP_EXT; goto decl_op;
    case SENT_INTRINSIC: tipo = DECL_OP_INT;
    decl_op:
        while (stmt_elemento()) {
            decl_entrada(tipo, 0, stm.e);
        }
        break;
    case SENT_DATA:
        decl_entrada(DECL_OP_DATA_FIN, 1, 0);               /* empieza con la lista vacia */
        while (stmt_dato()) {
            decl_entrada(stm.modo == 1 ? DECL_OP_DATA_DEST : stm.modo == 2 ? DECL_OP_DATA_VAL : DECL_OP_DATA_FIN, 0, stm.e);
        }
        break;
    case SENT_PARAMETER:
        while (stmt_elemento()) {
            uint16_t n0 = mf.n;

            stm.a = ast_a(stm.e);
            stm.b = ast_b(stm.e);
            if (decl_entrada(DECL_OP_PARAM, 0, stm.e)) {
                asignacion();
                if (mf.n == n0) {
                    GEN_PARAM_ITEM();
                    decl_entrada(DECL_OP_PARAM_FIN, 0, stm.e);
                }
            }
        }
        break;
    case SENT_ASIGNACION:
        asignacion();
        if (!sf_definida) {
            sem.ejec = 1;
        }
        break;
    case SENT_READ:
        GEN_READ(0);
        while (stmt_elemento()) {
            if (elemento_lectura()) {
                GEN_READ(1);
            }
        }
        break;
    case SENT_PRINT:
        if (stm.modo == 2) {
            ast_set_tdato(stm.a, TD_CHARACTER);
        }
        while (stmt_elemento()) {
            tipar(stm.e);
            GEN_PRINT_ITEM();
        }
        break;
    case SENT_IF_BLOQUE: case SENT_ELSEIF: case SENT_IF_LOGICO:
        exigir_logica(stm.a);
        break;
    case SENT_IF_ARITMETICO:
        exigir_numerica(stm.a);
        referencia(stm.l1, 0);
        referencia(stm.l2, 0);
        referencia(stm.l3, 0);
        break;
    case SENT_DO:
        bucle_do();
        break;
    case SENT_GOTO:
        if (stm.modo == 1) {
            while (stmt_elemento()) {
                referencia(ast_valor(stm.e),
                           ast_pos(stm.e));
                GEN_GOTO_ETQ(ast_valor(stm.e), ast_pos(stm.e));
            }
            if (stm.err == STM_OK) {
                indice(stm.b);
            }
        } else {
            referencia(stm.l1, 0);
        }
        break;
    case SENT_STOP:
        if (stm.a != 0) {
            tipar(stm.a);
        }
        break;
    case SENT_SUBROUTINE: case SENT_FUNCTION:
        formales();
        break;
    case SENT_CALL:
        llamada();
        break;
    case SENT_RETURN:
        if (sem.clase == UK_PROGRAMA) {
            error_uni(UNI_RETURN_PRINCIPAL, 0);
        }
        break;
    default:
        break;
    }
    if (stm.err == STM_OK && !sf_definida) {
        GEN_SENTENCIA();
    }
}

void sem_sentencia_erronea(void)
{
    if (!stm.interna && !sem.abierta && (stm.tipo == SENT_PROGRAM || stm.tipo == SENT_SUBROUTINE || stm.tipo == SENT_FUNCTION)) {
        abrir_unidad();
        sem.nsent++;
    }
}

void sem_sentencia(void)
{
    uint8_t cabecera;

    if (stm.err != STM_OK) {
        return;
    }
    /* La primera sentencia de cada unidad la abre (la que sigue al END de la anterior o la primera del fichero). */
    if (!stm.interna && stm.tipo != SENT_VACIA && !sem.abierta) {
        abrir_unidad();
    }
    cabecera = stm.tipo == SENT_PROGRAM || stm.tipo == SENT_SUBROUTINE || stm.tipo == SENT_FUNCTION;
    if (cabecera && sem.nsent != 0 && !stm.interna) {
        error_uni(UNI_CABECERA, 0);
        return;
    }
    /* La sentencia interior de un IF logico tiene etiqueta 0 (la lleva el IF) y
     * ninguna de sus clases abre ni cierra bloques: solo pasa por cuerpo(). */
    if (stm.etiqueta != 0) {
        switch (stm.tipo) {
        case SENT_PROGRAM: case SENT_SUBROUTINE: case SENT_FUNCTION: case SENT_INTEGER: case SENT_REAL: case SENT_DOUBLEPRECISION:
        case SENT_LOGICAL: case SENT_CHARACTER: case SENT_DIMENSION: case SENT_EXTERNAL: case SENT_INTRINSIC:
        case SENT_PARAMETER: case SENT_DATA:
            error(SEM_ETIQUETA_EN_DECL, 0);
            break;
        default:
            definicion(stm.etiqueta);
            GEN_ETIQUETA(stm.etiqueta);
            break;
        }
        cerrar_do_por_etiqueta();
    }
    cuerpo();
    if (stm.err == STM_OK) {
        bloques();
        GEN_FIN_SENTENCIA();
    }
    if (!stm.interna && stm.tipo != SENT_VACIA) {
        sem.nsent++;
    }
}
