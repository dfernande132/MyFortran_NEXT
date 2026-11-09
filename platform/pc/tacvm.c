/* tacvm.c - Interprete del TAC para pruebas (ver tacvm.h). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tacvm.h"
#include "tac.h"
#include "sim.h"
#include "realfmt.h"
#include "real.h"
#include "unid.h"
#include <math.h>

#define NTEMP 512
#define NOCULTAS 256
#define NETQ 8192

typedef struct {
    uint16_t len;              /* longitud real */
    uint16_t cap;              /* capacidad (0 = sin declarar) */
    uint8_t  s[256];
} CV;

typedef struct {
    uint8_t  tipo, clase;
    uint16_t len;              /* CHARACTER: longitud de cada elemento */
    uint32_t n;                /* elementos (1 si escalar) */
    int16_t  lo[2], hi[2];
    uint8_t asum;              /* formal CHARACTER*(*): len la pone cada PARAM de entrada */
    uint8_t alias;             /* formal array: i/f/c apuntan al almacenamiento del array real (no se liberan) */
    int16_t *i;                /* INTEGER / LOGICAL */
    uint32_t *f;               /* REAL: los 32 bits de cada float */
    uint8_t *c;                /* CHARACTER: n * len bytes */
} Var;

static Var     vars[SIM_MAX + 1];
static int16_t ti[NTEMP];
static uint32_t tf[NTEMP];
static CV      tc[NTEMP];
static int16_t hid[NOCULTAS];
static uint32_t hidf[NOCULTAS];         /* ocultas REAL (el DO con variable REAL: paso) */
static int32_t etq_pc[NETQ];
static uint16_t nvars;

static void liberar(void)
{
    uint16_t k;

    for (k = 1; k <= nvars; k++) {
        if (!vars[k].alias) {
            free(vars[k].i);
            free(vars[k].f);
            free(vars[k].c);
        }
    }
    memset(vars, 0, sizeof(vars));
}

static int mal(char *msg, size_t n, const char *fmt, unsigned a, unsigned b)
{
    snprintf(msg, n, fmt, a, b);
    return 1;
}

/* ETQ -> posicion en etq_pc */
static unsigned etq_clave(uint16_t op)
{
    return TAC_VALOR(op);
}

int tacvm_validar(char *msg, size_t n)
{
    uint16_t cnt = tac_cuenta();
    uint16_t i;
    TacInst t;
    static int32_t def[NETQ];
    int k;
    uint16_t *ops;

    memset(def, 0xFF, sizeof(def));
    msg[0] = '\0';
    if (tac_lleno()) {
        snprintf(msg, n, "TAC lleno");
        return 1;
    }
    for (i = 0; i < cnt; i++) {
        tac_leer(i, &t);
        if (t.op == TOP_LBL) {
            unsigned e = etq_clave(t.a);

            if (TAC_CLASE(t.a) != TAC_ETQ) {
                return mal(msg, n, "instr %u: LBL sin etiqueta%.0u", i, 0);
            }
            if (def[e] != -1) {
                return mal(msg, n, "etiqueta %u definida dos veces (instr %u)", e, i);
            }
            def[e] = i;
        }
    }
    for (i = 0; i < cnt; i++) {
        uint16_t o[3];

        tac_leer(i, &t);
        o[0] = t.a; o[1] = t.b; o[2] = t.c;
        ops = o;
        if (t.op >= TOP_TOTAL) {
            return mal(msg, n, "instr %u: operacion %u desconocida", i, t.op);
        }
        for (k = 0; k < 3; k++) {
            if (TAC_CLASE(ops[k]) == TAC_ETQ && t.op != TOP_LBL && def[etq_clave(ops[k])] == -1) {
                return mal(msg, n, "instr %u: salto a la etiqueta %u que no existe", i, etq_clave(ops[k]));
            }
        }
        if (t.op == TOP_SSUB || t.op == TOP_SSET) {
            TacInst x;

            if (i + 1 >= cnt) {
                return mal(msg, n, "instr %u: SSUB sin EXT%.0u", i, 0);
            }
            tac_leer(i + 1, &x);
            if (x.op != TOP_EXT) {
                return mal(msg, n, "instr %u: SSUB sin EXT%.0u", i, 0);
            }
        }
        if (t.op == TOP_EXT) {
            TacInst x;

            if (i == 0) {
                return mal(msg, n, "instr %u: EXT suelto%.0u", i, 0);
            }
            tac_leer(i - 1, &x);
            if (x.op != TOP_SSUB && x.op != TOP_SSET && x.op != TOP_DATA && !(x.op == TOP_PARAM && (tac_inm_valor(x.b) & 1024))) {
                return mal(msg, n, "instr %u: EXT suelto%.0u", i, 0);
            }
        }
    }
    if (cnt == 0) {
        snprintf(msg, n, "TAC vacio");
        return 1;
    }
    tac_leer((uint16_t)(cnt - 1), &t);
    if (t.op != TOP_HALT && t.op != TOP_STOP && t.op != TOP_JMP && t.op != TOP_RET) {
        snprintf(msg, n, "el TAC no acaba en HALT/STOP/JMP/RET");
        return 1;
    }
    /* unidades: cada una se abre con ENTER; las llamadas apuntan a unidades definidas y los PARAM a formales que existen */
    {
        static int32_t uni_pc[UNI_MAX + 1];
        uint8_t u;
        Unidad x;

        memset(uni_pc, 0xFF, sizeof(uni_pc));
        for (i = 0; i < cnt; i++) {
            tac_leer(i, &t);
            if (t.op == TOP_ENTER) {
                u = (uint8_t)tac_inm_valor(t.a);
                if (u == 0 || u > UNI_MAX || uni_pc[u] != -1) {
                    return mal(msg, n, "instr %u: ENTER de una unidad invalida o repetida (%u)", i, u);
                }
                uni_pc[u] = i;
            } else if (i == 0) {
                return mal(msg, n, "instr %u: el TAC no empieza con ENTER%.0u", i, 0);
            }
        }
        for (i = 0; i < cnt; i++) {
            tac_leer(i, &t);
            if (t.op == TOP_FUNR) {
                u = (uint8_t)tac_inm_valor(t.b);
                if (u == 0 || u > UNI_MAX || uni_pc[u] == -1) {
                    return mal(msg, n, "instr %u: FUNR de la unidad %u que no esta definida", i, u);
                }
                uni_leer(u, &x);
                if (x.clase != UK_FUNCION || x.sim_res == 0) {
                    return mal(msg, n, "instr %u: FUNR de la unidad %u que no es una funcion", i, u);
                }
            }
            if (t.op == TOP_CALL || t.op == TOP_PARAM) {
                u = (uint8_t)tac_inm_valor(t.op == TOP_CALL ? t.a : t.c);
                if (u == 0 || u > UNI_MAX || uni_pc[u] == -1) {
                    return mal(msg, n, "instr %u: llamada a la unidad %u que no esta definida", i, u);
                }
                if (t.op == TOP_PARAM) {
                    unsigned kk = (unsigned)tac_inm_valor(t.b);
                    unsigned k = kk & 255u;

                    uni_leer(u, &x);
                    if (k >= x.nformales) {
                        return mal(msg, n, "instr %u: PARAM del formal %u que no existe", i, k);
                    }
                    if (kk & 1024u) {
                        TacInst e;

                        if (i + 1 >= cnt) {
                            return mal(msg, n, "instr %u: PARAM de elemento sin EXT%.0u", i, 0);
                        }
                        tac_leer(i + 1, &e);
                        if (e.op != TOP_EXT) {
                            return mal(msg, n, "instr %u: PARAM de elemento sin EXT%.0u", i, 0);
                        }
                    }
                    if ((kk & 512u) && ((x.ftipo[k] & UA_ARRAY) == 0 || TAC_CLASE(t.a) != TAC_VAR)) {
                        return mal(msg, n, "instr %u: PARAM de array a un formal que no es array%.0u", i, 0);
                    }
                }
            }
        }
    }
    return 0;
}

static void preparar(void)
{
    uint16_t k;
    Simbolo s;
    Dim d;

    nvars = sim_cuenta();
    for (k = 1; k <= nvars; k++) {
        Var *v = &vars[k];

        sim_leer(k, &s);
        v->tipo = SIM_TIPO(&s);
        v->clase = SIM_CLASE(&s);
        v->len = 1;
        v->n = 1;
        if (v->clase == SIM_ARRAY) {
            sim_dim_leer(s.extra, &d);
            v->lo[0] = d.inf[0]; v->hi[0] = d.sup[0];
            v->lo[1] = d.inf[1]; v->hi[1] = d.sup[1];
            v->n = (uint32_t)(d.sup[0] - d.inf[0] + 1);
            if (SIM_NDIM(&s) == 2) {
                v->n *= (uint32_t)(d.sup[1] - d.inf[1] + 1);
            }
            v->len = d.len;
        } else if (v->tipo == TD_CHARACTER) {
            v->len = s.extra;
        }
        if (s.flags & SIMF_LONG_ASUM) {
            v->asum = 1;
            v->len = 0;
            v->c = (uint8_t *)calloc(256, 1);
        } else if (v->clase == SIM_ARRAY && (s.flags & SIMF_FORMAL)) {
            v->alias = 1;                      /* su almacenamiento es el del array real (PARAM de direccion) */
            v->n = 0;
        } else if (v->tipo == TD_CHARACTER) {
            v->c = (uint8_t *)malloc(v->n * v->len);
            memset(v->c, ' ', v->n * v->len);
        } else if (v->tipo == TD_REAL) {
            v->f = (uint32_t *)calloc(v->n, sizeof(uint32_t));
        } else {
            v->i = (int16_t *)calloc(v->n, sizeof(int16_t));
        }
    }
    memset(ti, 0, sizeof(ti));
    memset(tf, 0, sizeof(tf));
    memset(tc, 0, sizeof(tc));
    memset(hid, 0, sizeof(hid));
    memset(hidf, 0, sizeof(hidf));
}

static int16_t entero(uint16_t op, int *err)
{
    uint16_t v = TAC_VALOR(op);

    switch (TAC_CLASE(op)) {
    case TAC_TEMP:   return ti[v % NTEMP];
    case TAC_VAR:    return vars[v].i != NULL ? vars[v].i[0] : 0;
    case TAC_INM:    return tac_inm_valor(op);
    case TAC_CTE:    return tac_cte_entero(op);
    case TAC_OCULTA: return hid[v % NOCULTAS];
    default:         *err = 1; return 0;
    }
}

static void poner(uint16_t op, int16_t x, int *err)
{
    uint16_t v = TAC_VALOR(op);

    switch (TAC_CLASE(op)) {
    case TAC_TEMP:   ti[v % NTEMP] = x; break;
    case TAC_VAR:    if (vars[v].i != NULL) vars[v].i[0] = x; else *err = 1; break;
    case TAC_OCULTA: hid[v % NOCULTAS] = x; break;
    default:         *err = 1; break;
    }
}

/* Valor REAL (los 32 bits) de un operando. */
static uint32_t realv(uint16_t op, int *err)
{
    uint16_t v = TAC_VALOR(op);

    switch (TAC_CLASE(op)) {
    case TAC_TEMP: return tf[v % NTEMP];
    case TAC_OCULTA: return hidf[v % NOCULTAS];
    case TAC_VAR:  return vars[v].f != NULL ? vars[v].f[0] : (*err = 1, 0);
    case TAC_CTE:
        if (tac_cte_tipo(op) == TAC_C_REAL) return tac_cte_real(op);
        *err = 1;
        return 0;
    default:       *err = 1; return 0;
    }
}

static void poner_real(uint16_t op, uint32_t x, int *err)
{
    uint16_t v = TAC_VALOR(op);

    switch (TAC_CLASE(op)) {
    case TAC_TEMP: tf[v % NTEMP] = x; break;
    case TAC_OCULTA: hidf[v % NOCULTAS] = x; break;
    case TAC_VAR:  if (vars[v].f != NULL) vars[v].f[0] = x; else *err = 1; break;
    default:       *err = 1; break;
    }
}

static float aflt(uint32_t b)
{
    float f;

    memcpy(&f, &b, 4);
    return f;
}

static uint32_t bits_de(float r)
{
    return real_regla(r, signbit(r) != 0);
}

/* Aritmetica REAL del programa generado (reglas en realfmt.h). Devuelve RERR_*. */
static int aritm_real(uint8_t op, uint32_t xa, uint32_t xb, uint32_t *r)
{
    return real_op(op == TOP_ADD ? '+' : op == TOP_SUB ? '-' : op == TOP_MUL ? '*' : '/', xa, xb, r);
}

static int16_t real_a_entero(uint32_t b)
{
    float f = aflt(b);

    if (f >= 32767.0f) return 32767;
    if (f <= -32768.0f) return -32768;
    return (int16_t)f;
}

/* Valor CHARACTER de un operando. */
static void cadena(uint16_t op, CV *cv, int *err)
{
    uint16_t v = TAC_VALOR(op);
    char buf[256];

    switch (TAC_CLASE(op)) {
    case TAC_TEMP:
        *cv = tc[v % NTEMP];
        break;
    case TAC_VAR:
        if (vars[v].c == NULL) {
            *err = 1;
            return;
        }
        cv->len = vars[v].len;
        cv->cap = vars[v].len;
        memcpy(cv->s, vars[v].c, vars[v].len);
        break;
    case TAC_CTE:
        cv->len = tac_cte_cadena(op, buf);
        cv->cap = cv->len;
        memcpy(cv->s, buf, cv->len);
        break;
    default:
        *err = 1;
        break;
    }
}

static int cmp_cadenas(const CV *a, const CV *b)
{
    uint16_t n = a->len > b->len ? a->len : b->len;
    uint16_t i;

    for (i = 0; i < n; i++) {
        int x = i < a->len ? a->s[i] : ' ';
        int y = i < b->len ? b->s[i] : ' ';

        if (x != y) {
            return x < y ? -1 : 1;
        }
    }
    return 0;
}

static int16_t potencia(int16_t a, int16_t b)
{
    int32_t r = 1;
    int16_t i;

    if (b >= 0) {
        for (i = 0; i < b; i++) {
            r = (int16_t)(r * a);
        }
        return (int16_t)r;
    }
    if (a == 1) return 1;
    if (a == -1) return (b & 1) ? -1 : 1;
    return 0;
}

/* ---- entrada dirigida por lista (READ). Debe hacer lo mismo que las rutinas Z80 (tools/rtgen/rt_real.asm: RDLN, RDTOK, ...) ---- */
#define IN_LINEA_MAX 79
static const char *in_texto;
static size_t in_pos;
static char in_linea[IN_LINEA_MAX + 2];
static int in_lpos, in_need, in_slash;

void tacvm_entrada(const char *texto)
{
    in_texto = texto;
}

static void anadir(char *salida, size_t n, const char *txt, size_t len);

/* Lee la linea siguiente (con su eco). 0 = no hay mas entrada. */
static int in_linea_nueva(char *salida, size_t nsal)
{
    int n = 0;

    if (in_texto == NULL || in_texto[in_pos] == '\0') {
        return 0;
    }
    while (in_texto[in_pos] != '\0' && in_texto[in_pos] != '\n') {
        if (n < IN_LINEA_MAX) {
            in_linea[n++] = in_texto[in_pos];
        }
        in_pos++;
    }
    if (in_texto[in_pos] == '\n') {
        in_pos++;
    }
    in_linea[n] = '\0';
    in_lpos = 0;
    in_need = 0;
    anadir(salida, nsal, in_linea, (size_t)n);                  /* el eco de lo que se teclea */
    anadir(salida, nsal, "\n", 1);
    return 1;
}

/* Siguiente elemento de la entrada. Devuelve 0 = valor (tok/len; len puede ser 0 solo en una cadena entre comillas vacia), 1 = nulo
 * (la variable no cambia), -1 = error de dato, -2 = no hay mas entrada. q = 1 si el valor iba entre comillas. */
static int in_token(char *tok, int *len, int *q, char *salida, size_t nsal)
{
    *q = 0;
    *len = 0;
    if (in_slash) {
        return 1;
    }
    for (;;) {
        char c;

        if (in_need) {
            if (!in_linea_nueva(salida, nsal)) {
                return -2;
            }
        }
        while (in_linea[in_lpos] == ' ') {
            in_lpos++;
        }
        c = in_linea[in_lpos];
        if (c == '\0') {
            in_need = 1;
            continue;
        }
        if (c == ',') {
            in_lpos++;
            return 1;
        }
        if (c == '/') {
            in_slash = 1;
            return 1;
        }
        if (c == '\'' || c == '"') {
            in_lpos++;
            for (;;) {
                char d = in_linea[in_lpos];

                if (d == '\0') {
                    return -1;
                }
                in_lpos++;
                if (d == c) {
                    if (in_linea[in_lpos] != c) {
                        break;
                    }
                    in_lpos++;
                }
                tok[(*len)++] = d;
            }
            *q = 1;
        } else {
            while ((c = in_linea[in_lpos]) != '\0' && c != ' ' && c != ',' && c != '/') {
                tok[(*len)++] = c;
                in_lpos++;
            }
        }
        while (in_linea[in_lpos] == ' ') {
            in_lpos++;
        }
        if (in_linea[in_lpos] == ',') {
            in_lpos++;
        }
        tok[*len] = '\0';
        return 0;
    }
}

static void anadir(char *salida, size_t n, const char *txt, size_t len)
{
    size_t u = strlen(salida);

    if (u + len + 1 > n) {
        len = n > u + 1 ? n - u - 1 : 0;
    }
    memcpy(salida + u, txt, len);
    salida[u + len] = '\0';
}

void tacvm_ejecutar(char *salida, size_t nsal, uint32_t max_pasos, TacVmRes *res)
{
    uint16_t cnt = tac_cuenta();
    int32_t pc = 0;
    TacInst t, x;
    uint16_t i;
    int err = 0;
    int rerr = RERR_NINGUNO;
    char buf[64];
    int32_t uni_pc[UNI_MAX + 1];
    int32_t pila[256];
    int sp = 0;

    salida[0] = '\0';
    in_pos = 0;
    in_need = 1;
    in_slash = 0;
    memset(res, 0, sizeof(*res));
    res->estado = TACVM_ERROR;
    for (i = 0; i < NETQ; i++) {
        etq_pc[i] = -1;
    }
    for (i = 0; i <= UNI_MAX; i++) {
        uni_pc[i] = -1;
    }
    for (i = 0; i < cnt; i++) {
        tac_leer(i, &t);
        if (t.op == TOP_LBL) {
            etq_pc[etq_clave(t.a)] = i;
        } else if (t.op == TOP_ENTER) {
            uni_pc[(uint8_t)tac_inm_valor(t.a)] = i;
        }
    }
    {                                   /* se empieza por el programa principal */
        uint8_t u;
        Unidad xu;

        for (u = 1; u <= uni_cuenta(); u++) {
            uni_leer(u, &xu);
            if (xu.clase == UK_PROGRAMA && uni_pc[u] >= 0) {
                pc = uni_pc[u];
                break;
            }
        }
    }
    preparar();
    for (i = 0; i < cnt; i++) {            /* DATA: se aplica antes de ejecutar nada (como la inicializacion del programa) */
        TacInst x;

        tac_leer(i, &t);
        if (t.op != TOP_DATA) {
            continue;
        }
        tac_leer((uint16_t)(i + 1), &x);
        {
            Var *v = &vars[TAC_VALOR(t.a)];
            int e2 = 0;
            int16_t n = entero(t.c, &e2);
            int16_t off = entero(x.a, &e2);
            int16_t k;

            for (k = 0; k < n && !e2; k++) {
                uint32_t el = (uint32_t)off + (uint32_t)k;

                if (el >= v->n) {
                    break;
                }
                if (t.tipo == TD_CHARACTER) {
                    CV c;
                    uint16_t q;

                    cadena(t.b, &c, &e2);
                    for (q = 0; q < v->len; q++) {
                        v->c[el * v->len + q] = q < c.len ? c.s[q] : ' ';
                    }
                } else if (t.tipo == TD_REAL) {
                    v->f[el] = realv(t.b, &e2);
                } else {
                    v->i[el] = entero(t.b, &e2);
                }
            }
        }
    }
#define FALLA(m) do { snprintf(res->msg, sizeof(res->msg), "instr %d: %s", (int)pc, m); goto fin; } while (0)
    while (pc < cnt) {
        int16_t a, b, r;

        if (res->pasos++ >= max_pasos) {
            FALLA("limite de pasos");
        }
        tac_leer((uint16_t)pc, &t);
        err = 0;
        switch (t.op) {
        case TOP_NOP: case TOP_LBL: case TOP_EXT: case TOP_DATA:
            break;
        case TOP_RDLN:                                  /* cada READ lee un registro nuevo, tenga o no elementos */
            in_slash = 0;
            if (!in_linea_nueva(salida, nsal)) { rerr = RERR_FINENT; goto error_ejec; }
            break;
        case TOP_RD: {
            char tk[IN_LINEA_MAX + 2];
            int ln, q, k;
            int r = in_token(tk, &ln, &q, salida, nsal);
            int bad = 0;

            if (r == -2) { rerr = RERR_FINENT; goto error_ejec; }
            if (r == -1) { rerr = RERR_ENTRADA; goto error_ejec; }
            if (r == 1) break;                                    /* valor nulo: no cambia */
            if (t.tipo == TD_CHARACTER) {
                uint8_t *dst;
                uint16_t cap;

                if (TAC_CLASE(t.a) == TAC_TEMP) {
                    CV *d = &tc[TAC_VALOR(t.a) % NTEMP];

                    d->len = d->cap;
                    dst = d->s;
                    cap = d->cap;
                } else {
                    dst = vars[TAC_VALOR(t.a)].c;
                    cap = vars[TAC_VALOR(t.a)].len;
                }
                for (k = 0; k < cap; k++) {
                    dst[k] = k < ln ? (uint8_t)tk[k] : ' ';
                }
                break;
            }
            if (q || ln == 0) bad = 1;
            if (!bad && t.tipo == TD_LOGICAL) {
                k = tk[0] == '.' ? 1 : 0;
                if (tk[k] == 't' || tk[k] == 'T') poner(t.a, 1, &err);
                else if (tk[k] == 'f' || tk[k] == 'F') poner(t.a, 0, &err);
                else bad = 1;
            } else if (!bad && t.tipo == TD_INTEGER) {
                long v = 0;

                k = 0;
                if (tk[0] == '+' || tk[0] == '-') k = 1;
                if (tk[k] == '\0') bad = 1;
                for (; !bad && tk[k] != '\0'; k++) {
                    if (tk[k] < '0' || tk[k] > '9') { bad = 1; break; }
                    v = v * 10 + (tk[k] - '0');
                    if (v > 32768L) { bad = 1; break; }
                }
                if (!bad && tk[0] == '-') v = -v;
                if (!bad && (v > 32767L || v < -32768L)) bad = 1;
                if (!bad) poner(t.a, (int16_t)v, &err);
            } else if (!bad && t.tipo == TD_REAL) {
                char num[IN_LINEA_MAX + 2];
                int nd = 0, neg = 0, n = 0;
                uint32_t bits = 0;

                k = 0;
                if (tk[0] == '+' || tk[0] == '-') { neg = tk[0] == '-'; k = 1; }
                while (tk[k] >= '0' && tk[k] <= '9') { num[n++] = tk[k++]; nd++; }
                if (tk[k] == '.') {
                    num[n++] = tk[k++];
                    while (tk[k] >= '0' && tk[k] <= '9') { num[n++] = tk[k++]; nd++; }
                }
                if (nd == 0) bad = 1;
                if (!bad && (tk[k] == 'e' || tk[k] == 'E' || tk[k] == 'd' || tk[k] == 'D')) {
                    int ed = 0;

                    num[n++] = 'E';
                    k++;
                    if (tk[k] == '+' || tk[k] == '-') num[n++] = tk[k++];
                    while (tk[k] >= '0' && tk[k] <= '9') { num[n++] = tk[k++]; ed++; }
                    if (ed == 0) bad = 1;
                }
                if (!bad && tk[k] != '\0') bad = 1;
                num[n] = '\0';
                if (!bad && real_de_texto(num, &bits) != REAL_OK) bad = 1;
                if (!bad) poner_real(t.a, neg ? (bits ^ 0x80000000u) : bits, &err);
            } else {
                bad = 1;
            }
            if (bad) { rerr = RERR_ENTRADA; goto error_ejec; }
            break;
        }
        case TOP_TMP: {
            uint16_t v = TAC_VALOR(t.a) % NTEMP;

            tc[v].cap = (uint16_t)tac_inm_valor(t.b);
            tc[v].len = 0;
            if (tc[v].cap < 1 || tc[v].cap > 255) FALLA("TMP fuera de 1..255");
            break;
        }
        case TOP_ENTER:
            break;
        case TOP_CALL:
            if (sp >= 256) FALLA("pila de llamadas llena");
            pila[sp++] = pc + 1;
            pc = uni_pc[(uint8_t)tac_inm_valor(t.a)];
            continue;
        case TOP_FUNR: {
            Unidad xf;

            uni_leer((uint8_t)tac_inm_valor(t.b), &xf);
            t.b = TAC_OPER(TAC_VAR, xf.sim_res);
            if (t.tipo == TD_CHARACTER) {          /* el resultado CHARACTER va a un temporal */
                CV *d = &tc[TAC_VALOR(t.a) % NTEMP];
                Var *rv = &vars[TAC_VALOR(t.b)];

                if (d->cap < rv->len || rv->c == NULL) FALLA("FUNR.C sin TMP suficiente");
                d->len = rv->len;
                memcpy(d->s, rv->c, rv->len);
                break;
            }
        }
            goto mov;
        case TOP_RET:
            if (sp == 0) FALLA("RET sin llamada");
            pc = pila[--sp];
            continue;
        case TOP_PARAM: {
            Unidad xu;
            unsigned k = (unsigned)tac_inm_valor(t.b);
            uint16_t formal;
            uint16_t actual = t.a;

            uni_leer((uint8_t)tac_inm_valor(t.c), &xu);
            formal = TAC_OPER(TAC_VAR, (uint16_t)(xu.sim_ini + (k & 255u)));
            if (k & (512u | 1024u)) {                  /* array entero (512) o elemento (1024, con EXT arr, off) */
                Var *fv = &vars[TAC_VALOR(formal)];
                int fa = fv->clase == SIM_ARRAY;
                Var *av;
                int16_t off = 0;

                if (k & 512u) {
                    av = &vars[TAC_VALOR(actual)];
                } else {
                    tac_leer((uint16_t)(pc + 1), &x);
                    av = &vars[TAC_VALOR(x.a)];
                    off = entero(x.b, &err);
                    if (err || off < 0 || (uint32_t)off >= av->n) FALLA("indice de array fuera de rango");
                }
                if (fa && (k & 256u) == 0) {           /* el formal pasa a ser el array real (desde el elemento off) */
                    fv->alias = 1;
                    fv->tipo = av->tipo;
                    fv->len = av->len;
                    fv->n = av->n - (uint32_t)off;
                    fv->lo[0] = fv->lo[1] = 0;
                    fv->i = av->i != NULL ? av->i + off : NULL;
                    fv->f = av->f != NULL ? av->f + off : NULL;
                    fv->c = av->c != NULL ? av->c + (size_t)off * av->len : NULL;
                    break;
                }
                if (fa) {                              /* copia de salida de un elemento: el formal escribio en el array; se recarga el temporal */
                    t.op = TOP_LDE;
                    t.b = x.a;
                    t.c = x.b;
                    goto lde;
                }
                k &= ~(512u | 1024u);                  /* formal escalar: copia de entrada / salida de siempre */
            }
            if (k < 256 && t.tipo == TD_CHARACTER && vars[TAC_VALOR(formal)].asum) {    /* CHARACTER*(*): toma la longitud del real */
                CV c0;

                cadena(actual, &c0, &err);
                vars[TAC_VALOR(formal)].len = c0.len;
            }
            if (k >= 256 && t.tipo == TD_CHARACTER) {   /* copia de salida CHARACTER: solo los caracteres del formal */
                CV c;
                uint8_t *dst = NULL;
                uint16_t cap = 0, q;

                cadena(formal, &c, &err);
                if (TAC_CLASE(actual) == TAC_VAR && vars[TAC_VALOR(actual)].c != NULL) {
                    dst = (uint8_t *)vars[TAC_VALOR(actual)].c;
                    cap = vars[TAC_VALOR(actual)].len;
                } else if (TAC_CLASE(actual) == TAC_TEMP) {
                    dst = (uint8_t *)tc[TAC_VALOR(actual) % NTEMP].s;
                    cap = tc[TAC_VALOR(actual) % NTEMP].len;
                }
                if (err || dst == NULL || c.len > cap) FALLA("PARAM.C de salida invalido");
                for (q = 0; q < c.len; q++) {
                    dst[q] = (uint8_t)c.s[q];
                }
                break;
            }
            if (k >= 256) {            /* copia de salida: actual := formal */
                t.a = actual;
                t.b = formal;
            } else {                   /* copia de entrada: formal := actual */
                t.a = formal;
                t.b = actual;
            }
        }
            /* fall through */
        case TOP_MOV:
        mov:
            if (t.tipo == TD_CHARACTER) {
                CV c;
                Var *d = &vars[TAC_VALOR(t.a)];
                uint16_t k;

                cadena(t.b, &c, &err);
                if (err || TAC_CLASE(t.a) != TAC_VAR || d->c == NULL) FALLA("MOV.C invalido");
                for (k = 0; k < d->len; k++) {
                    d->c[k] = k < c.len ? c.s[k] : ' ';
                }
            } else if (t.tipo == TD_REAL) {
                poner_real(t.a, realv(t.b, &err), &err);
            } else {
                poner(t.a, entero(t.b, &err), &err);
            }
            break;
        case TOP_ITOF:
            poner_real(t.a, bits_de((float)entero(t.b, &err)), &err);
            break;
        case TOP_FTOI:
            poner(t.a, real_a_entero(realv(t.b, &err)), &err);
            break;
        case TOP_RFUN:
        case TOP_RMOD: {
            uint32_t xr = 0;

            rerr = t.op == TOP_RFUN ? real_fun(entero(t.c, &err), realv(t.b, &err), &xr)
                                    : real_mod(realv(t.b, &err), realv(t.c, &err), &xr);
            if (rerr != RERR_NINGUNO) goto error_ejec;
            poner_real(t.a, xr, &err);
            break;
        }
        case TOP_NEG:
            if (t.tipo == TD_REAL) {
                poner_real(t.a, realv(t.b, &err) ^ 0x80000000u, &err);
            } else {
                poner(t.a, (int16_t)(-entero(t.b, &err)), &err);
            }
            break;
        case TOP_NOT:
            poner(t.a, (int16_t)(entero(t.b, &err) == 0), &err);
            break;
        case TOP_ADD: case TOP_SUB: case TOP_MUL: case TOP_DIV: case TOP_POW:
            if (t.tipo == TD_REAL) {
                {
                    uint32_t xr = 0;

                    if (t.op == TOP_POW) {
                        rerr = real_powi(realv(t.b, &err), entero(t.c, &err), &xr);
                        if (rerr != RERR_NINGUNO) goto error_ejec;
                        poner_real(t.a, xr, &err);
                        break;
                    }

                    rerr = aritm_real(t.op, realv(t.b, &err), realv(t.c, &err), &xr);
                    if (rerr != RERR_NINGUNO) goto error_ejec;
                    poner_real(t.a, xr, &err);
                }
                break;
            }
            a = entero(t.b, &err);
            b = entero(t.c, &err);
            switch (t.op) {
            case TOP_ADD: r = (int16_t)((int32_t)a + b); break;
            case TOP_SUB: r = (int16_t)((int32_t)a - b); break;
            case TOP_MUL: r = (int16_t)((int32_t)a * b); break;
            case TOP_DIV:
                if (b == 0) { rerr = a == 0 ? RERR_0DIV0 : RERR_DIV0; goto error_ejec; }
                r = (int16_t)((int32_t)a / b);
                break;
            default: r = potencia(a, b); break;
            }
            poner(t.a, r, &err);
            break;
        case TOP_EQ: case TOP_NE: case TOP_LT: case TOP_LE: case TOP_GT: case TOP_GE: {
            int c;

            if (t.tipo == TD_CHARACTER) {
                CV p, q;

                cadena(t.b, &p, &err);
                cadena(t.c, &q, &err);
                c = cmp_cadenas(&p, &q);
            } else if (t.tipo == TD_REAL) {
                float p = aflt(realv(t.b, &err)), q = aflt(realv(t.c, &err));

                c = p < q ? -1 : (p > q ? 1 : 0);
            } else {
                a = entero(t.b, &err);
                b = entero(t.c, &err);
                c = a < b ? -1 : (a > b ? 1 : 0);
            }
            switch (t.op) {
            case TOP_EQ: r = c == 0; break;
            case TOP_NE: r = c != 0; break;
            case TOP_LT: r = c < 0; break;
            case TOP_LE: r = c <= 0; break;
            case TOP_GT: r = c > 0; break;
            default:     r = c >= 0; break;
            }
            poner(t.a, r, &err);
            break;
        }
        case TOP_AND: case TOP_OR: case TOP_EQV: case TOP_NEQV:
            a = entero(t.b, &err) != 0;
            b = entero(t.c, &err) != 0;
            r = t.op == TOP_AND ? (a && b) : t.op == TOP_OR ? (a || b) : t.op == TOP_EQV ? (a == b) : (a != b);
            poner(t.a, r, &err);
            break;
        case TOP_JMP:
            pc = etq_pc[etq_clave(t.a)];
            continue;
        case TOP_JT: case TOP_JF:
            if ((entero(t.a, &err) != 0) == (t.op == TOP_JT)) {
                pc = etq_pc[etq_clave(t.b)];
                continue;
            }
            break;
        case TOP_JEQ:
            if (entero(t.a, &err) == entero(t.b, &err)) {
                pc = etq_pc[etq_clave(t.c)];
                continue;
            }
            break;
        case TOP_LDE: case TOP_STE: lde: {
            uint16_t vid = TAC_VALOR(t.op == TOP_LDE ? t.b : t.a);
            int16_t off = entero(t.op == TOP_LDE ? t.c : t.b, &err);
            Var *v = &vars[vid];

            if (err || off < 0 || (uint32_t)off >= v->n) FALLA("indice de array fuera de rango");
            if (t.tipo == TD_CHARACTER) {
                if (t.op == TOP_LDE) {
                    CV *d = &tc[TAC_VALOR(t.a) % NTEMP];

                    if (d->cap < v->len) FALLA("LDE.C sin TMP suficiente");
                    d->len = v->len;
                    memcpy(d->s, v->c + (size_t)off * v->len, v->len);
                } else {
                    CV c;
                    uint16_t k;

                    cadena(t.c, &c, &err);
                    for (k = 0; k < v->len; k++) {
                        v->c[(size_t)off * v->len + k] = k < c.len ? c.s[k] : ' ';
                    }
                }
            } else if (t.tipo == TD_REAL) {
                if (t.op == TOP_LDE) {
                    poner_real(t.a, v->f[off], &err);
                } else {
                    v->f[off] = realv(t.c, &err);
                }
            } else if (t.op == TOP_LDE) {
                poner(t.a, v->i[off], &err);
            } else {
                v->i[off] = entero(t.c, &err);
            }
            break;
        }
        case TOP_SCAT: {
            CV p, q;
            CV *d = &tc[TAC_VALOR(t.a) % NTEMP];
            uint16_t k, n = 0;

            cadena(t.b, &p, &err);
            cadena(t.c, &q, &err);
            if (d->cap == 0) FALLA("SCAT.C sin TMP");
            for (k = 0; k < p.len && n < d->cap; k++) d->s[n++] = p.s[k];
            for (k = 0; k < q.len && n < d->cap; k++) d->s[n++] = q.s[k];
            d->len = n;
            break;
        }
        case TOP_SSUB: {
            CV p;
            CV *d = &tc[TAC_VALOR(t.a) % NTEMP];
            int16_t lo, hi, k;

            tac_leer((uint16_t)(pc + 1), &x);
            cadena(t.b, &p, &err);
            lo = entero(t.c, &err);
            hi = entero(x.a, &err);
            if (d->cap == 0) FALLA("SSUB.C sin TMP");
            d->len = 0;
            if (hi >= lo) {
                if (lo < 1 || hi > p.len) FALLA("subcadena fuera de rango");
                for (k = lo; k <= hi && d->len < d->cap; k++) d->s[d->len++] = p.s[k - 1];
            }
            break;
        }
        case TOP_SSET: {                          /* base(i:j) := e, con blancos o truncando */
            CV e;
            uint8_t *dp = NULL;
            uint16_t dcap = 0;
            int16_t lo, hi, k;

            tac_leer((uint16_t)(pc + 1), &x);
            cadena(t.c, &e, &err);
            lo = entero(t.b, &err);
            hi = entero(x.a, &err);
            if (TAC_CLASE(t.a) == TAC_VAR && vars[TAC_VALOR(t.a)].c != NULL) {
                dp = (uint8_t *)vars[TAC_VALOR(t.a)].c;
                dcap = vars[TAC_VALOR(t.a)].len;
            } else if (TAC_CLASE(t.a) == TAC_TEMP && tc[TAC_VALOR(t.a) % NTEMP].cap != 0) {
                dp = tc[TAC_VALOR(t.a) % NTEMP].s;
                dcap = tc[TAC_VALOR(t.a) % NTEMP].len;
            }
            if (err || dp == NULL) FALLA("SSET.C invalido");
            if (hi >= lo) {
                if (lo < 1 || hi > (int16_t)dcap) FALLA("subcadena fuera de rango");
                for (k = 0; k <= hi - lo; k++) dp[lo - 1 + k] = k < (int16_t)e.len ? e.s[k] : ' ';
            }
            break;
        }
        case TOP_SLEN: {
            CV p;

            cadena(t.b, &p, &err);
            poner(t.a, (int16_t)p.len, &err);
            break;
        }
        case TOP_PUT:
            if (t.tipo == TD_CHARACTER) {
                CV p;

                cadena(t.a, &p, &err);
                anadir(salida, nsal, " ", 1);
                anadir(salida, nsal, (const char *)p.s, p.len);
            } else if (t.tipo == TD_REAL) {
                char fb[24];

                real_formato(realv(t.a, &err), fb);
                anadir(salida, nsal, " ", 1);
                anadir(salida, nsal, fb, strlen(fb));
            } else if (t.tipo == TD_LOGICAL) {
                anadir(salida, nsal, entero(t.a, &err) ? " T" : " F", 2);
            } else {
                snprintf(buf, sizeof(buf), " %d", (int)entero(t.a, &err));
                anadir(salida, nsal, buf, strlen(buf));
            }
            break;
        case TOP_PUTNL:
            anadir(salida, nsal, "\n", 1);
            break;
        case TOP_STOP:
            res->estado = TACVM_STOP;
            if (TAC_CLASE(t.a) == TAC_CTE && tac_cte_tipo(t.a) == TAC_C_CADENA) {
                tac_cte_cadena(t.a, res->msg);
            } else if (TAC_CLASE(t.a) != TAC_NADA) {
                snprintf(res->msg, sizeof(res->msg), "%d", (int)entero(t.a, &err));
            }
            goto fin;
        case TOP_HALT:
            res->estado = TACVM_HALT;
            goto fin;
        default:
            FALLA("operacion desconocida");
        }
        if (err) FALLA("operando invalido");
        pc++;
    }
    FALLA("se salio del final del TAC");
error_ejec:                             /* el programa generado imprime el mensaje y un salto de linea, y para */
    res->estado = TACVM_ERREJEC;
    snprintf(res->msg, sizeof(res->msg), "%s", real_err_texto(rerr));
    anadir(salida, nsal, res->msg, strlen(res->msg));
    anadir(salida, nsal, "\n", 1);
fin:
    liberar();
}
