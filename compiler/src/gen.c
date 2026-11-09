/* gen.c - Generador de codigo de tres direcciones (ver gen.h). */
#include <string.h>
#include "gen_int.h"

/* La pila de bloques y el buffer del GOTO calculado comparten pagina con la tabla
 * de etiquetas: que no se pisen. */
typedef char gen_comprobar_etiquetas[(SEM_ETIQUETAS_MAX * 12 <= GEN_BLOQUES_OFF) ? 1 : -1];
typedef char gen_comprobar_blq_sem1[(SEM_ETIQUETAS_MAX * 12 <= SEM_BLQ_OFF) ? 1 : -1];        /* (sccz80 no admite && en una constante) */
typedef char gen_comprobar_blq_sem2[(SEM_BLQ_OFF + SEM_BLOQUES_MAX * 12 <= GEN_BLOQUES_OFF) ? 1 : -1];
typedef char gen_comprobar_bloques[(GEN_BLOQUES_OFF + SEM_BLOQUES_MAX * 12 <= GEN_GOTO_OFF) ? 1 : -1];
typedef char gen_comprobar_args1[(GEN_GOTO_OFF + GEN_GOTO_MAX * 2 <= GEN_ARG_OFF) ? 1 : -1];
typedef char gen_comprobar_args2[(GEN_SF_OFF + GEN_SF_TAM <= BANCO_PAGINA_TAM) ? 1 : -1];
typedef char gen_comprobar_goto[(GEN_GOTO_OFF + GEN_GOTO_MAX * 2 <= BANCO_PAGINA_TAM) ? 1 : -1];

#define GB_IF 1
#define GB_DO 2

/* Bloque abierto (IF o DO). IF: a = etiqueta de la rama siguiente (0 = ya hubo
 * ELSE), b = etiqueta del END IF (0 = aun no hace falta). DO: a = cabeza del
 * bucle, b = salida, c = indice de la etiqueta terminal, v = variable. */
typedef struct {
    uint8_t  tipo;
    uint16_t oculta;        /* DO: $oculta = paso (REAL: 2 palabras), despues las iteraciones restantes (indice del PROGRAMA) */
    uint16_t a, b, c, v;
} GBloque;

Gen gen_g;

void gen_activar(uint8_t si)
{
    g.activo = si;
}

uint16_t gen_nombre(void)
{
    return g.nombre;
}

static uint16_t s_nlbl;         /* etiquetas internas dadas en todo el programa (no se repiten entre unidades) */

void gen_iniciar(void)
{
    uint8_t act = g.activo;

    memset(&g, 0, sizeof(g));
    g.activo = act;
    s_nlbl = 0;
    tac_iniciar();
}

void gen_unidad(uint8_t clase, uint8_t uid)
{
    uint8_t act = g.activo;
    uint16_t tb = (uint16_t)(g.tbase + g.tmax);
    uint16_t hb = (uint16_t)(g.hbase + g.hmax);

    memset(&g, 0, sizeof(g));
    g.activo = act;
    g.tbase = tb;
    g.hbase = hb;
    g.ndo = hb;
    g.clase = clase;
    if (seguir()) {
        emitir(TOP_ENTER, 0, tac_entero(uid), 0, 0);
    }
}

/* ------------------------------------------------------------------ */

static uint16_t nueva_etq(void)
{
    return TAC_OPER(TAC_ETQ, TAC_ETQ_INTERNA + ++s_nlbl);
}

/* Pila de bloques (en la pagina de datos). */
static void bpush(const GBloque *b)
{
    banco_escribir(GEN_PAGINA, (uint16_t)(GEN_BLOQUES_OFF + g.nblq * sizeof(GBloque)), b, sizeof(GBloque));
    g.nblq++;
}

static void bpeek(GBloque *b)
{
    banco_leer(GEN_PAGINA, (uint16_t)(GEN_BLOQUES_OFF + (g.nblq - 1) * sizeof(GBloque)), b, sizeof(GBloque));
}

static void bpoke(const GBloque *b)
{
    banco_escribir(GEN_PAGINA, (uint16_t)(GEN_BLOQUES_OFF + (g.nblq - 1) * sizeof(GBloque)), b, sizeof(GBloque));
}

static void bpop(void)
{
    GBloque b;

    bpeek(&b);
    g.nblq--;
    if (b.tipo == GB_DO) {
        g.ndo = b.oculta;
    }
}

/* ------------------------------------------------------------------
 * Expresiones
 * ------------------------------------------------------------------ */

static uint16_t gsfun(uint16_t n);

static uint8_t es_inm(uint16_t op)
{
    return TAC_CLASE(op) == TAC_INM;
}

/* Conversion de modo mixto: INTEGER -> REAL (ITOF) o REAL -> INTEGER (FTOI, trunca). Una constante entera se convierte aqui. */
static uint16_t conv(uint16_t a, uint8_t de, uint8_t hacia)
{
    uint16_t d;

    if (a == 0 || de == hacia) {
        return a;
    }
    if (de == TD_INTEGER && hacia == TD_REAL) {
        if (es_inm(a)) {
            return tac_real(real_de_entero(tac_inm_valor(a)));
        }
        d = nuevo_temp();
        emitir(TOP_ITOF, TD_REAL, d, a, 0);
        return d;
    }
    if (de == TD_REAL && hacia == TD_INTEGER) {
        d = nuevo_temp();
        emitir(TOP_FTOI, TD_INTEGER, d, a, 0);
        return d;
    }
    return a;
}

/* Operacion entera con plegado de constantes. */
static uint16_t aritm(uint8_t op, uint16_t a, uint16_t b)
{
    uint16_t d;

    if (es_inm(a) && es_inm(b)) {
        /* Aritmetica de 16 bits con desbordamiento modular (equivale a calcular en 32 y truncar a int16): asi no
         * arrastra al residente del Next la multiplicacion/division de 32 bits de la libreria. */
        int16_t x = tac_inm_valor(a);
        int16_t y = tac_inm_valor(b);

        switch (op) {
        case TOP_ADD: return tac_entero((int16_t)((uint16_t)x + (uint16_t)y));
        case TOP_SUB: return tac_entero((int16_t)((uint16_t)x - (uint16_t)y));
        case TOP_MUL: return tac_entero((int16_t)((uint16_t)x * (uint16_t)y));
        case TOP_DIV:
            if (y == -1) {
                return tac_entero((int16_t)(0u - (uint16_t)x));
            }
            if (y != 0) {
                return tac_entero((int16_t)(x / y));
            }
            break;
        default:
            break;
        }
    }
    if (op == TOP_ADD && es_inm(b) && tac_inm_valor(b) == 0) {
        return a;
    }
    if (op == TOP_ADD && es_inm(a) && tac_inm_valor(a) == 0) {
        return b;
    }
    if ((op == TOP_MUL || op == TOP_DIV) && es_inm(b) && tac_inm_valor(b) == 1) {
        return a;
    }
    if (op == TOP_SUB && es_inm(b) && tac_inm_valor(b) == 0) {
        return a;
    }
    d = nuevo_temp();
    emitir(op, TD_INTEGER, d, a, b);
    return d;
}

/* Valor de un nodo que es una constante entera (con signo opcional). */
static uint8_t const_nodo(uint16_t n, int16_t *v)
{
    uint16_t nd = n;

    if (ast_tipo(nd) == NODO_ENTERO) {
        *v = (int16_t)ast_valor(nd);
        return 1;
    }
    if (ast_tipo(nd) == NODO_NEG && ast_tipo(ast_a(nd)) == NODO_ENTERO) {
        *v = (int16_t)(-(int32_t)ast_valor(ast_a(nd)));
        return 1;
    }
    return 0;
}

/* Desplazamiento (en elementos, base 0) de un indice con limite inferior lo. */
static uint16_t desp(uint16_t idx, int16_t lo)
{
    if (lo == 0) {
        return idx;
    }
    return aritm(TOP_SUB, idx, tac_entero(lo));
}

/* Desplazamiento del elemento A(i[,j]) del nodo REF n. */
uint16_t gen_desplazamiento(uint16_t n, const Simbolo *s, const Dim *dm)
{
    uint16_t nd = n;
    uint16_t c = ast_b(nd);
    uint16_t o1, o2, m;

    o1 = desp(gen_expr(ast_a(c), 0), dm->inf[0]);
    if (SIM_NDIM(s) == 2) {
        c = ast_b(c);
        o2 = desp(gen_expr(ast_a(c), 0), dm->inf[1]);
        if (dm->svar != 0) {                     /* formal ajustable A(N,*): la 1.a dimension vale N - inf + 1 (se lee al ejecutar) */
            m = TAC_OPER(TAC_VAR, dm->svar);
            if (dm->inf[0] != 1) {
                m = aritm(TOP_SUB, m, tac_entero((int16_t)(dm->inf[0] - 1)));
            }
        } else {
            m = tac_entero((int16_t)(dm->sup[0] - dm->inf[0] + 1));
        }
        m = aritm(TOP_MUL, o2, m);
        o1 = aritm(TOP_ADD, o1, m);
    }
    return o1;
}

/* Capacidad al compilar de una variable CHARACTER: su longitud, o 255 si es un formal CHARACTER*(*) (la longitud se lee al ejecutar). */
#define CAP_ASUM  255
static uint16_t cap_char(const Simbolo *s)
{
    return (s->flags & SIMF_LONG_ASUM) ? CAP_ASUM : s->extra;
}

/* Longitud (SLEN) de la cadena base en un temporal. */
static uint16_t glen(uint16_t base)
{
    uint16_t d = nuevo_temp();

    emitir(TOP_SLEN, TD_INTEGER, d, base, 0);
    return d;
}

/* Subcadena base(lo:hi) de una cadena de capacidad cap_base (asum = CHARACTER*(*): el limite omitido es LEN). rango = nodo RANGO. */
uint16_t gen_subcadena(uint16_t base, uint16_t cap_base, uint16_t rango, uint8_t asum)
{
    uint16_t r = rango;
    int16_t vl, vh;
    uint16_t lo, hi, d, cap;

    lo = ast_a(r) != 0 ? gen_expr(ast_a(r), 0) : tac_entero(1);
    hi = ast_b(r) != 0 ? gen_expr(ast_b(r), 0) : (asum ? glen(base) : tac_entero((int16_t)cap_base));
    g.slo = lo;
    g.shi = hi;
    vl = 1;
    vh = (int16_t)cap_base;
    cap = cap_base;
    if ((ast_a(r) == 0 || const_nodo(ast_a(r), &vl)) && (ast_b(r) == 0 ? !asum : const_nodo(ast_b(r), &vh))) {
        cap = vh >= vl ? (uint16_t)(vh - vl + 1) : 1;
        if (cap > cap_base) {
            cap = cap_base;
        }
    }
    d = nuevo_temp();
    emitir(TOP_TMP, TD_CHARACTER, d, tac_entero((int16_t)(cap == 0 ? 1 : cap)), 0);
    emitir(TOP_SSUB, TD_CHARACTER, d, base, lo);
    emitir(TOP_EXT, 0, hi, 0, 0);
    g.cap = cap;
    return d;
}

/* ------------------------------------------------------------------
 * Funciones intrinsecas enteras
 *
 * No hay rutinas nuevas en el programa generado: se expanden en TAC con las operaciones y los
 * saltos de siempre. Los argumentos ya estan evaluados a operandos (INM, VAR o TEMP), que se
 * pueden usar varias veces. Las de REAL/DOUBLE no llegan aqui (gexpr las rechaza antes).
 * ------------------------------------------------------------------ */

enum { IN_NINGUNA, IN_ABS, IN_MOD, IN_SIGN, IN_DIM, IN_MAX, IN_MIN, IN_LEN, IN_INT, IN_REAL, IN_AINT, IN_ANINT, IN_NINT, IN_SQRT };

static uint8_t intr_clase(const char *n)
{
    if (!strcmp(n, "ABS") || !strcmp(n, "IABS"))   return IN_ABS;
    if (!strcmp(n, "MOD") || !strcmp(n, "AMOD"))   return IN_MOD;
    if (!strcmp(n, "SIGN") || !strcmp(n, "ISIGN")) return IN_SIGN;
    if (!strcmp(n, "DIM") || !strcmp(n, "IDIM"))   return IN_DIM;
    if (!strcmp(n, "MAX") || !strcmp(n, "MAX0") || !strcmp(n, "AMAX1") || !strcmp(n, "AMAX0") || !strcmp(n, "MAX1")) return IN_MAX;
    if (!strcmp(n, "MIN") || !strcmp(n, "MIN0") || !strcmp(n, "AMIN1") || !strcmp(n, "AMIN0") || !strcmp(n, "MIN1")) return IN_MIN;
    if (!strcmp(n, "AINT"))                        return IN_AINT;
    if (!strcmp(n, "ANINT"))                       return IN_ANINT;
    if (!strcmp(n, "NINT"))                        return IN_NINT;
    if (!strcmp(n, "SQRT"))                        return IN_SQRT;
    if (!strcmp(n, "LEN"))                         return IN_LEN;
    if (!strcmp(n, "INT") || !strcmp(n, "IFIX"))   return IN_INT;
    if (!strcmp(n, "REAL") || !strcmp(n, "FLOAT")) return IN_REAL;
    return IN_NINGUNA;
}

/* Longitud de una expresion CHARACTER conocida al compilar (sin generar codigo): constante,
 * variable, elemento de array y concatenaciones de ellos. Una subcadena S(i:j) no: 0. */
static uint8_t len_cte(uint16_t n, uint16_t *len)
{
    uint16_t nd = n;
    uint16_t x, y;
    Simbolo s;
    Dim dm;

    switch (ast_tipo(nd)) {
    case NODO_PAREN:
        return len_cte(ast_a(nd), len);
    case NODO_CADENA:
        *len = (uint16_t)strlen(tabla_texto(ast_a(nd)));
        return 1;
    case NODO_NOMBRE:
        sim_leer(sim_buscar(tabla_texto(ast_a(nd))), &s);
        *len = s.extra;
        return (uint8_t)((s.flags & SIMF_LONG_ASUM) == 0);
    case NODO_REF:
        sim_leer(sim_buscar(tabla_texto(ast_a(nd))), &s);
        if (SIM_CLASE(&s) != SIM_ARRAY) {
            if (ast_tipo(ast_a(ast_b(nd))) == NODO_RANGO) {
                return 0;                            /* S(i:j) */
            }
            *len = s.extra;                          /* F(args): funcion CHARACTER*n del usuario */
            return 1;
        }
        sim_dim_leer(s.extra, &dm);
        *len = dm.len;
        return 1;
    case NODO_CONCAT:
        if (!len_cte(ast_a(nd), &x) || !len_cte(ast_b(nd), &y)) {
            return 0;
        }
        x = (uint16_t)(x + y);
        *len = x > SEM_CHAR_MAX ? SEM_CHAR_MAX : x;
        return 1;
    default:
        return 0;
    }
}

/* Cero del tipo td (INTEGER o REAL). */
static uint16_t cero_de(uint8_t td)
{
    return td == TD_REAL ? tac_real(0) : tac_entero(0);
}

/* r = |x| (td: INTEGER o REAL) */
static void abs_en(uint16_t r, uint16_t x, uint8_t td)
{
    uint16_t c = nuevo_temp();
    uint16_t l = nueva_etq();

    emitir(TOP_MOV, td, r, x, 0);
    emitir(TOP_LT, td, c, x, cero_de(td));
    emitir(TOP_JF, 0, c, l, 0);
    emitir(TOP_NEG, td, r, x, 0);
    emitir(TOP_LBL, 0, l, 0, 0);
    if (td == TD_REAL) {                             /* ABS(-0.0) = +0.0 */
        c = nuevo_temp();
        l = nueva_etq();
        emitir(TOP_EQ, td, c, x, tac_real(0));
        emitir(TOP_JF, 0, c, l, 0);
        emitir(TOP_MOV, td, r, tac_real(0), 0);
        emitir(TOP_LBL, 0, l, 0, 0);
    }
}

/* Constante REAL del texto del nodo (con el signo cambiado si neg); error si no cabe. */
static uint16_t const_real(uint16_t nd, uint8_t neg)
{
    uint32_t bits;

    if (real_de_texto(tabla_texto(ast_a(nd)), &bits) != REAL_OK) {
        mf_diag_pos(SEM_REAL_DESBORDA, ast_pos(nd));
        return 0;
    }
    if (neg) {
        bits ^= 0x80000000UL;
    }
    return tac_real(bits);
}

static uint16_t gintr(uint16_t n)
{
    uint16_t nd = n;
    uint8_t cl = intr_clase(tabla_texto(ast_a(nd)));
    uint16_t lista = ast_b(nd);
    uint16_t a, b, r, c, l;
    uint8_t ta = ast_tdato(ast_a(lista));     /* tipo de los argumentos */
    uint8_t tr = ast_tdato(nd);                                       /* tipo del resultado */

    if (cl == IN_NINGUNA) {
        no_soportado(ast_pos(nd));
        return 0;
    }
    if (cl == IN_LEN) {
        if (!len_cte(ast_a(lista), &a)) {            /* no se sabe al compilar: CHARACTER*(*) o subcadena de limites variables */
            a = gen_expr(ast_a(lista), 0);
            return a == 0 ? 0 : glen(a);
        }
        return tac_entero((int16_t)a);
    }
    if (ta == TD_DOUBLE || tr == TD_DOUBLE) {
        no_soportado(ast_pos(nd));
        return 0;
    }
    a = gen_expr(ast_a(lista), 0);
    lista = ast_b(lista);
    if (a == 0) {
        return 0;
    }
    if (cl == IN_INT) {                              /* INT(I) es I; INT(X) trunca */
        return conv(a, ta, TD_INTEGER);
    }
    if (cl == IN_REAL) {
        return conv(a, ta, TD_REAL);
    }
    if (cl == IN_AINT || cl == IN_ANINT || cl == IN_NINT || cl == IN_SQRT) {    /* argumento REAL */
        r = nuevo_temp();
        emitir(TOP_RFUN, TD_REAL, r, a, tac_entero(cl == IN_AINT ? 0 : cl == IN_SQRT ? 2 : 1));
        return cl == IN_NINT ? conv(r, TD_REAL, TD_INTEGER) : r;
    }
    if (cl == IN_ABS) {
        r = nuevo_temp();
        abs_en(r, a, ta);
        return r;
    }
    b = gen_expr(ast_a(lista), 0);
    lista = ast_b(lista);
    if (b == 0) {
        return 0;
    }
    r = nuevo_temp();
    switch (cl) {
    case IN_MOD:                                     /* a - (a / b) * b, con la division truncada; REAL: resto exacto */
        if (ta == TD_REAL) {
            emitir(TOP_RMOD, TD_REAL, r, a, b);
            return r;
        }
        return aritm(TOP_SUB, a, aritm(TOP_MUL, aritm(TOP_DIV, a, b), b));
    case IN_SIGN:                                    /* |a| con el signo de b (b = 0: positivo) */
        abs_en(r, a, ta);
        c = nuevo_temp();
        l = nueva_etq();
        emitir(TOP_LT, ta, c, b, cero_de(ta));
        emitir(TOP_JF, 0, c, l, 0);
        emitir(TOP_NEG, ta, r, r, 0);
        emitir(TOP_LBL, 0, l, 0, 0);
        return r;
    case IN_DIM:                                     /* a - b si a > b, y 0 si no */
        c = nuevo_temp();
        l = nueva_etq();
        emitir(TOP_MOV, ta, r, cero_de(ta), 0);
        emitir(TOP_GT, ta, c, a, b);
        emitir(TOP_JF, 0, c, l, 0);
        emitir(TOP_SUB, ta, r, a, b);
        emitir(TOP_LBL, 0, l, 0, 0);
        return r;
    default:                                         /* MAX / MIN: r = a; r = b si b > r (o < r) ... */
        emitir(TOP_MOV, ta, r, a, 0);
        for (;;) {
            c = nuevo_temp();
            l = nueva_etq();
            emitir(cl == IN_MAX ? TOP_GT : TOP_LT, ta, c, b, r);
            emitir(TOP_JF, 0, c, l, 0);
            emitir(TOP_MOV, ta, r, b, 0);
            emitir(TOP_LBL, 0, l, 0, 0);
            if (lista == 0) {
                break;
            }
            b = gen_expr(ast_a(lista), 0);
            lista = ast_b(lista);
            if (b == 0) {
                return 0;
            }
        }
        return conv(r, ta, tr);                      /* AMAX0/AMIN0 (INTEGER -> REAL), MAX1/MIN1 (REAL -> INTEGER) */
    }
}

/* Elemento de array o subcadena de variable (nodo REF). */
static uint16_t gref(uint16_t n)
{
    uint16_t nd = n;
    uint16_t id = sim_buscar(tabla_texto(ast_a(nd)));
    Simbolo s;
    Dim dm;
    uint16_t off, d;

    if (id == 0) {                 /* F(X) y no es variable: intrinseca o funcion del usuario */
        if (intr_clase(tabla_texto(ast_a(nd))) == IN_NINGUNA && uni_buscar(tabla_texto(ast_a(nd))) != 0) {
            return gen_funcion(n, uni_buscar(tabla_texto(ast_a(nd))));
        }
        return gintr(n);
    }
    sim_leer(id, &s);
    if (SIM_CLASE(&s) == SIM_FUNCION) {
        return gintr(n);                            /* INTRINSIC declarada */
    }
    if (SIM_CLASE(&s) == SIM_VARIABLE && (SIM_TIPO(&s) != TD_CHARACTER || ast_tipo(ast_a(ast_b(nd))) != NODO_RANGO)) {
        return gen_funcion(n, uni_buscar(tabla_texto(ast_a(nd))));      /* variable con tipo declarado usada como funcion */
    }
    if (SIM_CLASE(&s) == SIM_VARIABLE) {            /* S(i:j) */
        return gen_subcadena(TAC_OPER(TAC_VAR, id), cap_char(&s), ast_a(ast_b(nd)), (uint8_t)((s.flags & SIMF_LONG_ASUM) != 0));
    }
    sim_dim_leer(s.extra, &dm);
    off = gen_desplazamiento(n, &s, &dm);
    d = nuevo_temp();
    if (SIM_TIPO(&s) == TD_CHARACTER) {
        emitir(TOP_TMP, TD_CHARACTER, d, tac_entero((int16_t)dm.len), 0);
        g.cap = dm.len;
    }
    emitir(TOP_LDE, SIM_TIPO(&s), d, TAC_OPER(TAC_VAR, id), off);
    return d;
}

static uint8_t op_de_nodo(uint8_t t)
{
    switch (t) {
    case NODO_ADD:  return TOP_ADD;
    case NODO_SUB:  return TOP_SUB;
    case NODO_MUL:  return TOP_MUL;
    case NODO_DIV:  return TOP_DIV;
    case NODO_POW:  return TOP_POW;
    case NODO_EQ:   return TOP_EQ;
    case NODO_NE:   return TOP_NE;
    case NODO_LT:   return TOP_LT;
    case NODO_LE:   return TOP_LE;
    case NODO_GT:   return TOP_GT;
    case NODO_GE:   return TOP_GE;
    case NODO_AND:  return TOP_AND;
    case NODO_OR:   return TOP_OR;
    case NODO_EQV:  return TOP_EQV;
    default:        return TOP_NEQV;
    }
}

/* Referencia a una funcion de sentencia (sem.c la deja expandida): evalua los argumentos reales (por valor), los ata a
 * los ficticios NODO_ARGF y genera el cuerpo. Las expansiones anidadas guardan y restauran la atadura. */
static uint16_t gsfun(uint16_t n)
{
    uint16_t nd = n;
    uint16_t nuevo[8], viejo[8];
    uint16_t c, r;
    uint8_t k = 0;

    for (c = ast_b(nd); c != 0; c = ast_b(c)) {
        r = gen_expr(ast_a(c), 0);
        if (r == 0 || k >= 8) {
            return 0;
        }
        nuevo[k++] = r;
    }
    memcpy(viejo, g.sfarg, sizeof(viejo));
    memcpy(g.sfarg, nuevo, (size_t)k * sizeof(uint16_t));
    r = gen_expr(ast_a(nd), 0);
    memcpy(g.sfarg, viejo, sizeof(viejo));
    if (r == 0) {
        return 0;
    }
    return conv(r, ast_tdato(ast_a(nd)), ast_tdato(nd));
}

/* Evalua la expresion n y devuelve el operando con su valor. dst (VAR, o 0): si la
 * operacion de la raiz produce el valor, puede escribirlo directamente alli. */
uint16_t gen_expr(uint16_t n, uint16_t dst)
{
    uint16_t nd = n;
    uint8_t t = ast_tdato(nd);
    uint16_t a, b, d;
    uint8_t op;

    if (!seguir()) {
        return 0;
    }
    if (t == TD_DOUBLE) {
        no_soportado(ast_pos(nd));
        return 0;
    }
    switch (ast_tipo(nd)) {
    case NODO_REAL:
        return const_real(nd, 0);
    case NODO_ENTERO:
        return tac_entero((int16_t)ast_valor(nd));
    case NODO_LOGICO:
        return tac_logico((uint8_t)ast_a(nd));
    case NODO_CADENA: {
        const char *txt = tabla_texto(ast_a(nd));
        uint8_t len = (uint8_t)strlen(txt);

        g.cap = len;
        return tac_cadena(txt, len);
    }
    case NODO_NOMBRE: {
        uint16_t id = sim_buscar(tabla_texto(ast_a(nd)));
        Simbolo s;

        if (t == TD_CHARACTER) {
            sim_leer(id, &s);
            g.cap = cap_char(&s);
        }
        return TAC_OPER(TAC_VAR, id);
    }
    case NODO_REF:
        return gref(n);
    case NODO_ARGF:
        return g.sfarg[ast_a(nd)];
    case NODO_SFUN:
        return gsfun(n);
    case NODO_SUBCAD:
        a = gref(ast_a(nd));
        return gen_subcadena(a, g.cap, ast_a(ast_b(nd)), 0);
    case NODO_PAREN:
    case NODO_POS:
        return gen_expr(ast_a(nd), dst);
    case NODO_NEG:
        if (ast_tipo(ast_a(nd)) == NODO_ENTERO) {
            return tac_entero((int16_t)(-(int32_t)ast_valor(ast_a(nd))));
        }
        if (ast_tipo(ast_a(nd)) == NODO_REAL) {              /* -1.5 es una constante */
            return const_real(ast_a(nd), 1);
        }
        a = gen_expr(ast_a(nd), 0);
        d = dst != 0 ? dst : nuevo_temp();
        emitir(TOP_NEG, t, d, a, 0);
        return d;
    case NODO_NOT:
        a = gen_expr(ast_a(nd), 0);
        d = dst != 0 ? dst : nuevo_temp();
        emitir(TOP_NOT, TD_LOGICAL, d, a, 0);
        return d;
    case NODO_CONCAT: {
        uint16_t ca, cb, cap;

        a = gen_expr(ast_a(nd), 0);
        ca = g.cap;
        b = gen_expr(ast_b(nd), 0);
        cb = g.cap;
        cap = (uint16_t)(ca + cb);
        if (cap > SEM_CHAR_MAX) {
            cap = SEM_CHAR_MAX;
        }
        if (cap == 0) {
            cap = 1;
        }
        d = nuevo_temp();
        emitir(TOP_TMP, TD_CHARACTER, d, tac_entero((int16_t)cap), 0);
        emitir(TOP_SCAT, TD_CHARACTER, d, a, b);
        g.cap = cap;
        return d;
    }
    default:
        break;
    }
    /* binarios: aritmeticos, relacionales y logicos */
    op = op_de_nodo(ast_tipo(nd));
    a = gen_expr(ast_a(nd), 0);
    b = gen_expr(ast_b(nd), 0);
    if (ast_tipo(nd) >= NODO_EQ && ast_tipo(nd) <= NODO_GE) {
        t = ast_tdato(ast_a(nd));              /* tipo de los operandos */
        if (ast_tdato(ast_b(nd)) == TD_REAL) {
            t = TD_REAL;
        }
    }
    if (t == TD_REAL) {                             /* modo mixto: el entero se pasa a REAL */
        if (op == TOP_POW) {                        /* REAL ** INTEGER; con exponente REAL (EXP/LOG) en la etapa R5 */
            if (ast_tdato(ast_a(nd)) != TD_REAL || ast_tdato(ast_b(nd)) != TD_INTEGER) {
                no_soportado(ast_pos(nd));
                return 0;
            }
            d = dst != 0 ? dst : nuevo_temp();
            emitir(TOP_POW, TD_REAL, d, a, b);
            return d;
        }
        a = conv(a, ast_tdato(ast_a(nd)), TD_REAL);
        b = conv(b, ast_tdato(ast_b(nd)), TD_REAL);
    }
    d = dst != 0 ? dst : nuevo_temp();
    emitir(op, t, d, a, b);
    return d;
}

/* ------------------------------------------------------------------
 * Sentencias
 * ------------------------------------------------------------------ */

static uint16_t simbolo_de(uint16_t nodo_nombre, Simbolo *s)
{
    uint16_t id = sim_buscar(tabla_texto(ast_a(nodo_nombre)));

    sim_leer(id, s);
    return id;
}

/* Asignacion a una subcadena: S(i:j) = e  o  A(k)(i:j) = e. El elemento de array se copia a un temporal, se modifica y se
 * devuelve con STE. SSET.C base, i, e + EXT j: copia e en base(i:j), rellenando con blancos o truncando. */
static void gsubasig(void)
{
    uint16_t dst = stm.a;
    uint16_t ref = ast_tipo(dst) == NODO_SUBCAD ? ast_a(dst) : dst;
    uint16_t rango = ast_tipo(dst) == NODO_SUBCAD ? ast_a(ast_b(dst)) : ast_a(ast_b(ref));
    uint16_t r = rango;
    uint16_t id = sim_buscar(tabla_texto(ast_a(ref)));
    uint16_t base, capb, lo, hi, e, off = 0;
    Simbolo s;
    Dim dm;

    sim_leer(id, &s);
    if (SIM_CLASE(&s) == SIM_ARRAY) {
        sim_dim_leer(s.extra, &dm);
        capb = dm.len;
        off = gen_desplazamiento(ref, &s, &dm);
        base = nuevo_temp();
        emitir(TOP_TMP, TD_CHARACTER, base, tac_entero((int16_t)capb), 0);
        emitir(TOP_LDE, TD_CHARACTER, base, TAC_OPER(TAC_VAR, id), off);
    } else {
        capb = cap_char(&s);
        base = TAC_OPER(TAC_VAR, id);
    }
    lo = ast_a(r) != 0 ? gen_expr(ast_a(r), 0) : tac_entero(1);
    hi = ast_b(r) != 0 ? gen_expr(ast_b(r), 0) : ((s.flags & SIMF_LONG_ASUM) ? glen(base) : tac_entero((int16_t)capb));
    e = gen_expr(stm.b, 0);
    if (e == 0 || lo == 0 || hi == 0) {
        return;
    }
    emitir(TOP_SSET, TD_CHARACTER, base, lo, e);
    emitir(TOP_EXT, 0, hi, 0, 0);
    if (SIM_CLASE(&s) == SIM_ARRAY) {
        emitir(TOP_STE, TD_CHARACTER, TAC_OPER(TAC_VAR, id), off, base);
    }
}

static void gasignacion(void)
{
    uint16_t dst = stm.a;
    Simbolo s;
    uint16_t id, r, d;
    uint8_t td;

    if (ast_tipo(dst) == NODO_SUBCAD) {
        gsubasig();                                  /* A(I)(2:3) = ... */
        return;
    }
    id = simbolo_de(stm.a, &s);
    td = SIM_TIPO(&s);
    if (td == TD_DOUBLE) {
        no_soportado(ast_pos(dst));
        return;
    }
    if (ast_tipo(dst) == NODO_REF) {
        if (SIM_CLASE(&s) == SIM_VARIABLE) {
            gsubasig();                              /* S(2:3) = ... */
            return;
        }
        {
            Dim dm;
            uint16_t off;

            sim_dim_leer(s.extra, &dm);
            off = gen_desplazamiento(stm.a, &s, &dm);
            r = conv(gen_expr(stm.b, 0), ast_tdato(stm.b), td);
            emitir(TOP_STE, td, TAC_OPER(TAC_VAR, id), off, r);
        }
        return;
    }
    d = TAC_OPER(TAC_VAR, id);
    if (ast_tdato(stm.b) != td) {            /* INTEGER <-> REAL */
        r = conv(gen_expr(stm.b, 0), ast_tdato(stm.b), td);
    } else {
        r = gen_expr(stm.b, d);
    }
    if (r != d) {
        emitir(TOP_MOV, td, d, r, 0);
    }
}

static void gif_bloque(void)
{
    GBloque b;
    uint16_t r;

    if (g.nblq >= SEM_BLOQUES_MAX) {
        return;                                      /* sem lo anotara */
    }
    r = gen_expr(stm.a, 0);
    b.tipo = GB_IF;
    b.oculta = 0;
    b.a = nueva_etq();
    b.b = 0;
    b.c = 0;
    b.v = 0;
    emitir(TOP_JF, 0, r, b.a, 0);
    bpush(&b);
}

static void gelse(uint8_t con_cond)
{
    GBloque b;
    uint16_t r;

    if (g.nblq == 0) {
        return;
    }
    bpeek(&b);
    if (b.tipo != GB_IF || b.a == 0) {
        return;
    }
    if (b.b == 0) {
        b.b = nueva_etq();
    }
    emitir(TOP_JMP, 0, b.b, 0, 0);
    emitir(TOP_LBL, 0, b.a, 0, 0);
    if (con_cond) {
        r = gen_expr(stm.a, 0);
        b.a = nueva_etq();
        emitir(TOP_JF, 0, r, b.a, 0);
    } else {
        b.a = 0;
    }
    bpoke(&b);
}

static void gendif(void)
{
    GBloque b;

    if (g.nblq == 0) {
        return;
    }
    bpeek(&b);
    if (b.tipo != GB_IF) {
        return;
    }
    if (b.a != 0) {
        emitir(TOP_LBL, 0, b.a, 0, 0);
    }
    if (b.b != 0) {
        emitir(TOP_LBL, 0, b.b, 0, 0);
    }
    bpop();
}

static uint16_t etq_usuario(uint32_t etiq)
{
    return TAC_OPER(TAC_ETQ, sem_etq_indice(etiq));
}

static void gif_aritmetico(void)
{
    uint8_t td = ast_tdato(stm.a);
    uint16_t r = gen_expr(stm.a, 0);
    uint16_t t;
    uint16_t cero = td == TD_REAL ? tac_real(0) : tac_entero(0);

    t = nuevo_temp();
    emitir(TOP_LT, td, t, r, cero);
    emitir(TOP_JT, 0, t, etq_usuario(stm.l1), 0);
    t = nuevo_temp();
    emitir(TOP_EQ, td, t, r, cero);
    emitir(TOP_JT, 0, t, etq_usuario(stm.l2), 0);
    emitir(TOP_JMP, 0, etq_usuario(stm.l3), 0, 0);
}

static void gdo(void)
{
    Simbolo s;
    uint16_t id = sim_buscar(tabla_texto(stm.nombre));
    GBloque b;
    uint16_t i, f, p, paso, cnt, t;
    int16_t cp;
    uint8_t td;

    if (g.nblq >= SEM_BLOQUES_MAX) {
        return;
    }
    sim_leer(id, &s);
    td = SIM_TIPO(&s);
    if (td != TD_INTEGER && td != TD_REAL) {
        no_soportado(ast_pos(stm.a));
        return;
    }
    if (stm.c != 0 && const_nodo(stm.c, &cp) && cp == 0) {
        mf_diag_pos(SEM_DO_PASO_CERO, ast_pos(stm.c));
        return;
    }
    i = conv(gen_expr(stm.a, 0), ast_tdato(stm.a), td);
    f = conv(gen_expr(stm.b, 0), ast_tdato(stm.b), td);
    p = stm.c != 0 ? conv(gen_expr(stm.c, 0), ast_tdato(stm.c), td) : (td == TD_REAL ? tac_real(0x3F800000UL) : tac_entero(1));
    b.tipo = GB_DO;
    b.oculta = g.ndo;
    g.ndo = (uint16_t)(g.ndo + (td == TD_REAL ? 3 : 2));
    if ((uint16_t)(g.ndo - g.hbase) > g.hmax) {
        g.hmax = (uint16_t)(g.ndo - g.hbase);
    }
    paso = TAC_OPER(TAC_OCULTA, b.oculta);
    cnt = TAC_OPER(TAC_OCULTA, b.oculta + (td == TD_REAL ? 2 : 1));
    emitir(TOP_MOV, td, paso, p, 0);
    /* iteraciones = (f - i + p) / p, una sola vez, ANTES de tocar la variable (REAL: se calcula en REAL y se trunca) */
    if (td == TD_REAL) {
        t = nuevo_temp();
        emitir(TOP_SUB, TD_REAL, t, f, i);
        emitir(TOP_ADD, TD_REAL, t, t, p);
        emitir(TOP_DIV, TD_REAL, t, t, p);
        t = conv(t, TD_REAL, TD_INTEGER);
    } else {
        if (es_inm(i) && es_inm(p) && tac_inm_valor(i) == tac_inm_valor(p)) {
            t = f;                                   /* f - i + p = f */
        } else {
            t = aritm(TOP_SUB, f, i);
            t = aritm(TOP_ADD, t, p);
        }
        t = aritm(TOP_DIV, t, p);
    }
    emitir(TOP_MOV, TD_INTEGER, cnt, t, 0);
    emitir(TOP_MOV, td, TAC_OPER(TAC_VAR, id), i, 0);
    b.a = nueva_etq();
    b.b = nueva_etq();
    b.c = sem_etq_indice(stm.l1);
    b.v = id;
    emitir(TOP_LBL, 0, b.a, 0, 0);
    t = nuevo_temp();
    emitir(TOP_GT, TD_INTEGER, t, cnt, tac_entero(0));
    emitir(TOP_JF, 0, t, b.b, 0);
    bpush(&b);
}

static void gcerrar_do(void)
{
    GBloque b;
    uint16_t v;
    Simbolo s;
    uint8_t td;
    uint16_t cnt;

    bpeek(&b);
    sim_leer(b.v, &s);
    td = SIM_TIPO(&s);
    v = TAC_OPER(TAC_VAR, b.v);
    cnt = TAC_OPER(TAC_OCULTA, b.oculta + (td == TD_REAL ? 2 : 1));
    emitir(TOP_ADD, td, v, v, TAC_OPER(TAC_OCULTA, b.oculta));
    emitir(TOP_SUB, TD_INTEGER, cnt, cnt, tac_entero(1));
    emitir(TOP_JMP, 0, b.a, 0, 0);
    emitir(TOP_LBL, 0, b.b, 0, 0);
    bpop();
}

static void ggoto_calculado(void)
{
    uint16_t r = gen_expr(stm.b, 0);
    uint16_t k;
    uint16_t e;

    for (k = 0; k < g.ngoto; k++) {
        banco_leer(GEN_PAGINA, (uint16_t)(GEN_GOTO_OFF + k * 2), &e, 2);
        emitir(TOP_JEQ, 0, r, tac_entero((int16_t)(k + 1)), TAC_OPER(TAC_ETQ, e));
    }
    g.ngoto = 0;
}

static void gstop(void)
{
    uint16_t a = 0;

    if (stm.a != 0) {
        uint16_t nd = stm.a;

        if (ast_tipo(nd) == NODO_CADENA) {
            const char *txt = tabla_texto(ast_a(nd));

            a = tac_cadena(txt, (uint8_t)strlen(txt));
        } else {
            a = tac_entero((int16_t)ast_valor(nd));
        }
    }
    emitir(TOP_STOP, 0, a, 0, 0);
}

/* ------------------------------------------------------------------
 * Puntos de entrada
 * ------------------------------------------------------------------ */

void gen_etiqueta(uint32_t etiq)
{
    if (!seguir()) {
        return;
    }
    g.cierre = sem_etq_indice(etiq);
    emitir(TOP_LBL, 0, TAC_OPER(TAC_ETQ, g.cierre), 0, 0);
}

void gen_print_item(void)
{
    uint16_t nd;
    uint16_t r;

    if (!seguir()) {
        return;
    }
    if (stm.modo != 0) {
        no_soportado(0);                             /* FORMAT: aun no */
        return;
    }
    g.ntemp = 0;
    nd = stm.e;
    r = gen_expr(stm.e, 0);
    emitir(TOP_PUT, ast_tdato(nd), r, 0, 0);
}

/* Un elemento de PARAMETER: nombre = expresion, como una asignacion (stm.a, stm.b). */
void gen_param_item(void)
{
    if (!seguir()) {
        return;
    }
    g.ntemp = 0;
    gasignacion();
}

/* READ: un elemento de la lista es una variable o un elemento de array. */
void gen_read(uint8_t item)
{
    uint16_t nd;
    Simbolo s;
    uint16_t id, t, off;
    Dim dm;

    if (!seguir()) {
        return;
    }
    if (!item) {
        emitir(TOP_RDLN, 0, 0, 0, 0);
        return;
    }
    g.ntemp = 0;
    nd = stm.e;
    id = sim_buscar(tabla_texto(ast_a(nd)));
    sim_leer(id, &s);
    if (ast_tipo(nd) == NODO_NOMBRE) {
        emitir(TOP_RD, SIM_TIPO(&s), TAC_OPER(TAC_VAR, id), 0, 0);
        return;
    }
    sim_dim_leer(s.extra, &dm);
    off = gen_desplazamiento(stm.e, &s, &dm);
    t = nuevo_temp();
    if (SIM_TIPO(&s) == TD_CHARACTER) {
        emitir(TOP_TMP, TD_CHARACTER, t, tac_entero((int16_t)dm.len), 0);
    }
    emitir(TOP_LDE, SIM_TIPO(&s), t, TAC_OPER(TAC_VAR, id), off);      /* un valor nulo deja el elemento como estaba */
    emitir(TOP_RD, SIM_TIPO(&s), t, 0, 0);
    emitir(TOP_STE, SIM_TIPO(&s), TAC_OPER(TAC_VAR, id), off, t);
}

void gen_goto_etq(uint32_t etiq, uint16_t pos)
{
    uint16_t e;

    if (!seguir()) {
        return;
    }
    if (g.ngoto >= GEN_GOTO_MAX) {
        mf_diag_pos(SEM_GEN_GOTO_CALC, pos);
        return;
    }
    e = sem_etq_indice(etiq);
    banco_escribir(GEN_PAGINA, (uint16_t)(GEN_GOTO_OFF + g.ngoto * 2), &e, 2);
    g.ngoto++;
}

void gen_sentencia(void)
{
    if (!seguir()) {
        return;
    }
    if (stm.tipo != SENT_CALL) {         /* CALL: los argumentos ya reservaron temporales que la copia de salida sigue usando */
        g.ntemp = 0;
    }
    switch (stm.tipo) {
    case SENT_PROGRAM:
        g.nombre = stm.nombre;
        break;
    case SENT_ASIGNACION:
        gasignacion();
        break;
    case SENT_PRINT:                                 /* los elementos ya salieron */
        if (stm.modo == 2) {
            const char *txt = tabla_texto(ast_a(stm.a));

            emitir(TOP_PUT, TD_CHARACTER, tac_cadena(txt, (uint8_t)strlen(txt)), 0, 0);
        } else if (stm.modo == 1) {
            no_soportado(0);                         /* PRINT con FORMAT: aun no */
            break;
        }
        emitir(TOP_PUTNL, 0, 0, 0, 0);
        break;
    case SENT_IF_BLOQUE:
        gif_bloque();
        break;
    case SENT_ELSEIF:
        gelse(1);
        break;
    case SENT_ELSE:
        gelse(0);
        break;
    case SENT_ENDIF:
        gendif();
        break;
    case SENT_IF_ARITMETICO:
        gif_aritmetico();
        break;
    case SENT_IF_LOGICO: {
        uint16_t r = gen_expr(stm.a, 0);

        g.lbl_if = nueva_etq();
        emitir(TOP_JF, 0, r, g.lbl_if, 0);
        break;
    }
    case SENT_DO:
        gdo();
        break;
    case SENT_GOTO:
        if (stm.modo == 1) {
            ggoto_calculado();
        } else {
            emitir(TOP_JMP, 0, etq_usuario(stm.l1), 0, 0);
        }
        break;
    case SENT_STOP:
        gstop();
        break;
    case SENT_END:
        emitir(g.clase == UK_PROGRAMA ? TOP_HALT : TOP_RET, 0, 0, 0, 0);
        break;
    case SENT_RETURN:
        emitir(TOP_RET, 0, 0, 0, 0);
        break;
    case SENT_CALL:
        gen_llamada_emitir();
        break;
    default:                                         /* declaraciones, SUBROUTINE, CONTINUE, vacia */
        break;
    }
}

void gen_fin_sentencia(void)
{
    GBloque b;

    if (!seguir()) {
        return;
    }
    if (stm.tipo == SENT_IF_LOGICO && !stm.interna) {
        return;                                      /* falta la sentencia interior */
    }
    if (g.lbl_if != 0) {
        emitir(TOP_LBL, 0, g.lbl_if, 0, 0);
        g.lbl_if = 0;
    }
    if (g.cierre != 0) {
        while (g.nblq > 0) {
            bpeek(&b);
            if (b.tipo != GB_DO || b.c != g.cierre) {
                break;
            }
            gcerrar_do();
        }
        g.cierre = 0;
    }
}
