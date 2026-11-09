/* back.c - TAC -> Z80 (ver back.h). Plantillas sencillas, sin optimizar: el acumulador
 * es HL (y DE el segundo operando); las variables viven en memoria fija. */
#include <stdio.h>
#include <string.h>
#include "back.h"
#include "z80.h"
#include "tac.h"
#include "sim.h"
#include "sem.h"
#include "banco.h"
#include "lexer.h"
#include "source.h"
#include "unid.h"

/* El backend puede compilarse en DOS trozos (BACK_DIVIDIDO, el Next: el banco 43 se quedaba corto):
 *   parte A (este fichero sin BACK_PARTE_C): todo menos las plantillas CHARACTER; llama a la parte C por la puerta t_back_c.
 *   parte C (back_c.c, que incluye este fichero con BACK_PARTE_C): cadena_info, ld_str, array_char, direccion_elemento,
 *     traducir_c, stop_mensaje y los PARAM CHARACTER, mas copias de los ayudantes pequenos que necesitan.
 * El estado que comparten (bk_pasada, bk_err, ...) lo define la parte A; la C lo declara extern. Sin BACK_DIVIDIDO (PC) todo es un solo
 * trozo y el estado es static. */
#ifdef BACK_DIVIDIDO
#  ifdef BACK_PARTE_C
#    define BK_A 0
#    define BK_C 1
#    define BK_DEF extern
#  else
#    define BK_A 1
#    define BK_C 0
#    define BK_DEF
#  endif
#else
#  define BK_A 1
#  define BK_C 1
#  define BK_DEF static
#endif

#if BK_A && !BK_C
void t_back_c(uint8_t cual, const TacInst *t, uint16_t x, uint16_t y);      /* puerta a la parte C (trampolin del banco 48) */
#define traducir_c(t)      t_back_c(0, (t), 0, 0)
#define stop_mensaje(a)    t_back_c(1, 0, (a), 0)
#define param_c(t, f, k)   t_back_c(2, (t), (f), (k))
#endif

/* ------------------------------------------------------------------ estado */

#define ETQ_TAB 2400                    /* 256 de usuario + hasta 2048 internas      */

/* Texto del listado .asm. Con -DMF_SIN_LISTADO (el Next) los literales ni se compilan: ahorran unos 3 KB de
 * banco y solo hacen falta si hay listado. Sin listado `sal` debe ser NULL (back_generar lo fuerza). */
#ifdef MF_SIN_LISTADO
#define T(s)  ((const char *)0)
/* Todo el texto auxiliar (nombres de simbolos, numeros) solo sirve para el listado: sin el, ni se genera ni se
 * enlaza sprintf (unos 250 B residentes en el Next). */
#undef sprintf
#undef strcpy
#undef strcat
#define sprintf(...)   ((void)0)
#define strcpy(d, s)   ((void)0)
#define strcat(d, s)   ((void)0)
#define nombre_v(d, s, id) ((void)0)
#define num32(d, v)    ((void)0)
#else
#define T(s)  (s)
#endif

BK_DEF uint8_t  bk_pasada;
BK_DEF int16_t  bk_err;
BK_DEF uint16_t bk_err_inst;
BK_DEF uint16_t bk_dpool, bk_hbase;
BK_DEF uint16_t bk_tmax, bk_hmax;
BK_DEF uint16_t bk_cur;                 /* instruccion TAC en curso (para errores)    */
#ifdef MF_TARGET_NEXT
#define bk_ent_prueba 0
#else
BK_DEF uint8_t  bk_ent_prueba;          /* 1 = pruebas de PC: la tecla sale de un texto en memoria (RT_KEYT) */
#endif
#if BK_A
static uint16_t dvars, tbase, fin_mem;
static uint16_t spsave;                 /* varias unidades: direccion de la palabra donde se guarda SP al entrar (STOP lo recupera) */
static uint8_t  cur_u;                  /* unidad en curso (ENTER); solo para los nombres del listado */
#endif

#include "back_rt_mac.h"
#ifdef MF_SIN_LISTADO
#define RT_NOM(r)  ((const char *)0)    /* los nombres solo salen en el listado */
#else
static const char *const rt_nom[RT_N] = {
    "rt_putc", "rt_neghl", "rt_negde", "rt_mul", "rt_div", "rt_pow", "rt_lt", "rt_ge",
    "rt_gt", "rt_le", "rt_eq", "rt_ne", "rt_puti", "rt_putl", "rt_puts",
    "rt_smov", "rt_sadd", "rt_scpy", "rt_clamp", "rt_scmp",
    "rt_fs", "rt_bshl", "rt_bshr", "rt_badd", "rt_bsub", "rt_funp", "rt_fnorm", "rt_fadd", "rt_fmul",
    "rt_fdiv", "rt_fcmp", "rt_itof", "rt_ftoi", "rt_fneg", "rt_fput", "rt_fint", "rt_fsqrt", "rt_fmod", "rt_fpow", "rt_err",
    "rt_rd", "rt_keysc", "rt_keyh", "rt_keyt", "rt_rdline", "rt_rdln", "rt_rdtok", "rt_rdi", "rt_rdl", "rt_rds", "rt_rdf"
};
#define RT_NOM(r)  rt_nom[r]
#endif
#define NTMP 128

/* Tablas del backend: viven en la pagina de datos BACK_PAG_TABLAS (no en el residente, que en el Next no
 * tiene sitio: son ~6,5 KB). Cada acceso copia por banco_leer/banco_escribir.
 *   etq:  direccion de cada etiqueta (ETQ_TAB x 2 B)
 *   var:  direccion de cada variable (SIM_MAX+1 x 2 B)
 *   tdir: direccion de cada temporal; tsz: bytes reservados (2, o 2 + capacidad si es CHARACTER);
 *   tcap: capacidad CHARACTER vigente (la de la ultima TMP) */
#define OFF_ETQ   0
#define OFF_VAR   (OFF_ETQ + 2 * ETQ_TAB)
#define OFF_TDIR  (OFF_VAR + 2 * (SIM_MAX + 1))
#define OFF_TSZ   (OFF_TDIR + 2 * NTMP)
#define OFF_TCAP  (OFF_TSZ + 2 * NTMP)
#define OFF_LOC   (OFF_TCAP + NTMP)                /* etiquetas locales de las rutinas (44 x 2 B: ids 0..43) */
#define OFF_RTDIR (OFF_LOC + 2 * 44)               /* direccion de cada rutina de apoyo (RT_N x 2 B)   */
#define OFF_RTUSA (OFF_RTDIR + 2 * RT_N)           /* rutina usada (RT_N x 1 B)                        */
#define OFF_FIN   (OFF_RTUSA + RT_N)

/* El texto de la sentencia comparte esta pagina (source.h): sus tablas terminan antes de SENT_OFF. */
#ifndef MF_TARGET_NEXT   /* comprobacion solo en PC: el plegado de constantes de sccz80 no admite la expresion */
typedef char back_tablas_no_pisan_el_texto[(OFF_FIN <= SENT_OFF && BACK_PAG_TABLAS == SENT_PAGINA) ? 1 : -1];
#endif

static uint16_t tab16_get(uint16_t off)
{
    uint16_t v;

    banco_leer(BACK_PAG_TABLAS, off, &v, 2);
    return v;
}

static void tab16_set(uint16_t off, uint16_t v)
{
    banco_escribir(BACK_PAG_TABLAS, off, &v, 2);
}

#if BK_A
static void tab_cero(uint16_t off, uint16_t n)
{
    uint8_t cero[32];                   /* en la pila: un static const a cero cae en el bss residente */
    uint16_t k;

    memset(cero, 0, sizeof(cero));
    while (n > 0) {
        k = n > sizeof(cero) ? (uint16_t)sizeof(cero) : n;
        banco_escribir(BACK_PAG_TABLAS, off, cero, k);
        off = (uint16_t)(off + k);
        n = (uint16_t)(n - k);
    }
}
#endif

#define loc_get(i)       tab16_get((uint16_t)(OFF_LOC + 2 * (i)))
#define loc_set(i, v)    tab16_set((uint16_t)(OFF_LOC + 2 * (i)), (v))
#define rt_dir_get(r)    tab16_get((uint16_t)(OFF_RTDIR + 2 * (r)))
#define rt_dir_set(r, v) tab16_set((uint16_t)(OFF_RTDIR + 2 * (r)), (v))
#define etq_get(i)       tab16_get((uint16_t)(OFF_ETQ + 2 * (i)))
#define etq_set(i, v)    tab16_set((uint16_t)(OFF_ETQ + 2 * (i)), (v))
#define var_get(i)       tab16_get((uint16_t)(OFF_VAR + 2 * (i)))
#define var_set(i, v)    tab16_set((uint16_t)(OFF_VAR + 2 * (i)), (v))
#define tdir_get(i)      tab16_get((uint16_t)(OFF_TDIR + 2 * (i)))
#define tdir_set(i, v)   tab16_set((uint16_t)(OFF_TDIR + 2 * (i)), (v))
#define tsz_get(i)       tab16_get((uint16_t)(OFF_TSZ + 2 * (i)))
#define tsz_set(i, v)    tab16_set((uint16_t)(OFF_TSZ + 2 * (i)), (v))

static uint8_t tcap_get(uint16_t i)
{
    uint8_t v;

    banco_leer(BACK_PAG_TABLAS, (uint16_t)(OFF_TCAP + i), &v, 1);
    return v;
}

static void tcap_set(uint16_t i, uint8_t v)
{
    banco_escribir(BACK_PAG_TABLAS, (uint16_t)(OFF_TCAP + i), &v, 1);
}

static uint8_t rt_usa_get(uint8_t r)
{
    uint8_t v;

    banco_leer(BACK_PAG_TABLAS, (uint16_t)(OFF_RTUSA + r), &v, 1);
    return v;
}

static void rt_usa_set(uint8_t r, uint8_t v)
{
    banco_escribir(BACK_PAG_TABLAS, (uint16_t)(OFF_RTUSA + r), &v, 1);
}
#if BK_A
static uint8_t  salida_pant;
static uint16_t rt_cur_dir;             /* cursor (columna, fila) de la salida a pantalla */
#endif


/* Texto sin printf de formatos "raros" (precision de %s, %lu, %04X): el printf de z88dk no los garantiza. */
#ifndef MF_SIN_LISTADO
/* Nombre de una variable en el listado: V_X; con varias unidades, V<u>_X (las unidades pueden repetir nombres). */
static void nombre_v(char *dst, const Simbolo *s, uint16_t id)
{
    uint8_t i = 0;
    uint8_t k = 1;
    uint8_t u;
    Unidad x;

    dst[0] = 'V';
    if (uni_cuenta() > 1) {
        for (u = 1; u <= uni_cuenta(); u++) {
            uni_leer(u, &x);
            if (id >= x.sim_ini && id <= x.sim_fin) {
                if (u >= 10) {
                    dst[k++] = (char)('0' + u / 10);
                }
                dst[k++] = (char)('0' + u % 10);
                break;
            }
        }
    }
    dst[k++] = '_';
    while (i < 8 && s->nombre[i] != '\0') {
        dst[k + i] = s->nombre[i];
        i++;
    }
    dst[k + i] = '\0';
}
#endif

#ifndef MF_SIN_LISTADO
#if BK_A
static void num32(char *dst, uint32_t v)
{
    char t[11];
    uint8_t n = 0;

    do {
        t[n++] = (char)('0' + (uint8_t)(v % 10u));
        v /= 10u;
    } while (v != 0);
    while (n > 0) {
        *dst++ = t[--n];
    }
    *dst = '\0';
}
#endif
#endif

#ifndef MF_SIN_LISTADO
#if BK_A
static void hex4(char *dst, uint16_t v)
{
    uint8_t i;

    for (i = 0; i < 4; i++) {
        uint8_t d = (uint8_t)((v >> (12 - 4 * i)) & 15);

        dst[i] = (char)(d < 10 ? '0' + d : 'A' + d - 10);
    }
    dst[4] = '\0';
}
#endif
#endif

static void fallo(int16_t e)
{
    if (bk_err == 0) {
        bk_err = e;
        bk_err_inst = bk_cur;
    }
}

/* Marca la rutina y las que ella necesita. */
static void necesita(uint8_t r)
{
    if (r == RT_KEY && bk_ent_prueba) {
        r = RT_KEYT;
    }
    if (rt_usa_get(r)) {
        return;
    }
    rt_usa_set(r, 1);
    switch (r) {
    case RT_NEGHL: case RT_NEGDE: case RT_MUL: case RT_LT: case RT_EQ: break;
    case RT_DIV:  necesita(RT_NEGHL); necesita(RT_NEGDE); necesita(RT_ERR); break;
    case RT_POW:  necesita(RT_MUL); break;
    case RT_GE: case RT_GT: necesita(RT_LT); break;
    case RT_LE:   necesita(RT_GE); break;
    case RT_NE:   necesita(RT_EQ); break;
    case RT_PUTI: necesita(RT_PUTC); necesita(RT_NEGHL); break;
    case RT_PUTL: case RT_PUTS: necesita(RT_PUTC); break;
    case RT_SMOV: case RT_SADD: case RT_SCPY: case RT_CLAMP: case RT_SCMP: break;
    case RT_FUNP: case RT_FTOI: case RT_FCMP: necesita(RT_FPD); necesita(RT_FUNP); break;
    case RT_FNORM: case RT_ITOF: necesita(RT_FPD); necesita(RT_BSHL); necesita(RT_FNORM); necesita(RT_ERR); break;
    case RT_FADD: necesita(RT_FUNP); necesita(RT_FNORM); necesita(RT_BADD); necesita(RT_BSUB); necesita(RT_BSHR); necesita(RT_BSHL); break;
    case RT_FMUL: necesita(RT_FUNP); necesita(RT_FNORM); necesita(RT_BADD); necesita(RT_BSHR); necesita(RT_BSHL); break;
    case RT_FDIV: necesita(RT_FUNP); necesita(RT_FNORM); necesita(RT_BADD); necesita(RT_BSUB); necesita(RT_BSHL); necesita(RT_ERR); break;
    case RT_FPUT: necesita(RT_PUTC); necesita(RT_FUNP); necesita(RT_BSHL); necesita(RT_BADD); break;
    case RT_FINT: necesita(RT_FUNP); necesita(RT_FNORM); break;
    case RT_FSQRT: necesita(RT_FUNP); necesita(RT_FNORM); necesita(RT_BSHL); necesita(RT_BSHR); necesita(RT_BADD); necesita(RT_BSUB);
        necesita(RT_ERR); break;
    case RT_FMOD: necesita(RT_FUNP); necesita(RT_FNORM); necesita(RT_BADD); necesita(RT_BSUB); necesita(RT_BSHL); necesita(RT_ERR); break;
    case RT_FPOW: necesita(RT_FMUL); necesita(RT_FDIV); necesita(RT_ERR); break;
    case RT_ERR:  necesita(RT_PUTC); break;
    case RT_KEYH: necesita(RT_KEYSC); break;
    case RT_RDF:  necesita(RT_FMUL); necesita(RT_FADD); necesita(RT_FDIV); necesita(RT_ITOF); necesita(RT_FPOW); necesita(RT_FNEG);
        /* fall through */
    case RT_RDI: case RT_RDL: case RT_RDS: necesita(RT_RDD); necesita(RT_RDTOK); necesita(RT_RDLINE); necesita(RT_KEY);
        necesita(RT_PUTC); necesita(RT_ERR); break;
    case RT_RDLN: necesita(RT_RDD); necesita(RT_RDLINE); necesita(RT_KEY); necesita(RT_PUTC); necesita(RT_ERR); break;
    default: break;
    }
}

/* -------------------------------------------------------- instrucciones */

static void i1(const char *t, uint8_t a)
{
    z_ins(t, NULL, 1, a, 0, 0, 0);
}

static void i2(const char *t, uint8_t a, uint8_t b)
{
    z_ins(t, NULL, 2, a, b, 0, 0);
}

static void i3(const char *t, const char *arg, uint8_t a, uint16_t w)
{
    z_ins(t, arg, 3, a, (uint8_t)(w & 0xFF), (uint8_t)(w >> 8), 0);
}

static void i4(const char *t, const char *arg, uint8_t a, uint8_t b, uint16_t w)
{
    z_ins(t, arg, 4, a, b, (uint8_t)(w & 0xFF), (uint8_t)(w >> 8));
}

#if BK_A
static void nom_local(uint8_t id, char *b)
{
    sprintf(b, "_l%u", (unsigned)id);
}
#endif

#if BK_A
static void loc_def(uint8_t id)
{
    char b[16];

    loc_set(id, z_pc());
    sprintf(b, "_l%u:", (unsigned)id);
    z_linea(b);
}
#endif

#if BK_A
/* JR cc,local: fmt lleva un %s */
static void jr(const char *fmt, uint8_t op, uint8_t id)
{
    char b[16];
    int d = (int)loc_get(id) - (int)(z_pc() + 2);

    if (bk_pasada == 2 && (d < -128 || d > 127)) {
        fallo(BACK_INTERNO);
    }
    nom_local(id, b);
    z_ins(fmt, b, 2, op, (uint8_t)d, 0, 0);
}
#endif

#if BK_A
static void jp_loc(const char *fmt, uint8_t op, uint8_t id)
{
    char b[16];

    nom_local(id, b);
    i3(fmt, b, op, loc_get(id));
}
#endif

static void call_rt(uint8_t r)
{
    if (r == RT_KEY && bk_ent_prueba) {
        r = RT_KEYT;
    }
    necesita(r);
    if (bk_pasada == 2 && rt_dir_get(r) == 0) {
        fallo(BACK_INTERNO);
    }
    i3(T("CALL %s"), RT_NOM(r), 0xCD, rt_dir_get(r));
}

#if BK_A
static void jp_rt(uint8_t r)
{
    necesita(r);
    i3(T("JP %s"), RT_NOM(r), 0xC3, rt_dir_get(r));
}
#endif

/* ------------------------------------------------------------- operandos */

static uint8_t es_imm(uint16_t op)
{
    uint8_t c = TAC_CLASE(op);

    return c == TAC_INM || (c == TAC_CTE && tac_cte_tipo(op) != TAC_C_CADENA);
}

static int16_t imm_val(uint16_t op)
{
    return TAC_CLASE(op) == TAC_INM ? tac_inm_valor(op) : tac_cte_entero(op);
}

#ifdef MF_SIN_LISTADO
static char simbolo[1];                 /* sin listado los nombres no se construyen (ver los macros de arriba) */
#else
static char simbolo[32];
#endif

/* Direccion y nombre de un operando en memoria (TEMP, VAR, OCULTA). */
static uint16_t dir_mem(uint16_t op)
{
    uint16_t v = TAC_VALOR(op);
    Simbolo s;

    switch (TAC_CLASE(op)) {
    case TAC_TEMP:
        if (v >= NTMP) {
            fallo(BACK_NO_SOPORTADO);
            v = 0;
        }
        if (v + 1 > bk_tmax) bk_tmax = (uint16_t)(v + 1);
        sprintf(simbolo, "T%u", (unsigned)v);
        return tdir_get(v);
    case TAC_OCULTA:
        if (v + 1 > bk_hmax) bk_hmax = (uint16_t)(v + 1);
        sprintf(simbolo, "H%u", (unsigned)v);
        return (uint16_t)(bk_hbase + 2 * v);
    case TAC_VAR:
        sim_leer(v, &s);
        nombre_v(simbolo, &s, v);
        return var_get(v);
    default:
        fallo(BACK_NO_SOPORTADO);
        strcpy(simbolo, "?");
        return 0;
    }
}

static void ld_hl(uint16_t op)
{
    char b[16];

    if (es_imm(op)) {
        sprintf(b, "%d", (int)imm_val(op));
        i3(T("LD HL,%s"), b, 0x21, (uint16_t)imm_val(op));
    } else {
        uint16_t d = dir_mem(op);

        i3(T("LD HL,(%s)"), simbolo, 0x2A, d);
    }
}

static void ld_de(uint16_t op)
{
    char b[16];

    if (es_imm(op)) {
        sprintf(b, "%d", (int)imm_val(op));
        i3(T("LD DE,%s"), b, 0x11, (uint16_t)imm_val(op));
    } else {
        uint16_t d = dir_mem(op);

        i4(T("LD DE,(%s)"), simbolo, 0xED, 0x5B, d);
    }
}

static void st_hl(uint16_t op)
{
    uint16_t d = dir_mem(op);

    i3(T("LD (%s),HL"), simbolo, 0x22, d);
}

/* Un array formal no tiene datos propios: su celda de 2 bytes guarda la direccion del array real (PARAM de direccion).
 * ld_de_base / ld_hl_base cargan la base del array arr: su direccion, o el contenido de la celda si es formal. */
static uint8_t es_formal(uint16_t arr)
{
    Simbolo s;

    sim_leer(TAC_VALOR(arr), &s);
    return (uint8_t)((s.flags & SIMF_FORMAL) != 0);
}

static void ld_de_base(uint16_t arr)
{
    uint16_t d = dir_mem(arr);

    if (es_formal(arr)) {
        i4(T("LD DE,(%s)"), simbolo, 0xED, 0x5B, d);
    } else {
        i3(T("LD DE,%s"), simbolo, 0x11, d);
    }
}

#if BK_A
static void ld_hl_base(uint16_t arr)
{
    uint16_t d = dir_mem(arr);

    if (es_formal(arr)) {
        i3(T("LD HL,(%s)"), simbolo, 0x2A, d);
    } else {
        i3(T("LD HL,%s"), simbolo, 0x21, d);
    }
}
#endif

/* ------------------------------------------------------------- etiquetas */

#if BK_A
static uint16_t etq_slot(uint16_t op)
{
    uint16_t v = TAC_VALOR(op);
    uint16_t s = v < TAC_ETQ_INTERNA ? v : (uint16_t)(257 + v - TAC_ETQ_INTERNA);

    if (s >= ETQ_TAB) {
        fallo(BACK_ETIQUETAS);
        return 0;
    }
    return s;
}
#endif

#if BK_A
static void etq_nombre(uint16_t op, char *b)
{
    uint16_t v = TAC_VALOR(op);

    if (v < TAC_ETQ_INTERNA) {
        if (uni_cuenta() > 1) {                 /* una etiqueta 10 puede estar en varias unidades: L<u>_10 */
            sprintf(b, "L%u_", (unsigned)cur_u);
            num32(b + strlen(b), sem_etq_valor(v));
        } else {
            strcpy(b, "L_");
            num32(b + 2, sem_etq_valor(v));
        }
    } else {
        sprintf(b, "I_%u", (unsigned)(v - TAC_ETQ_INTERNA));
    }
}
#endif

#if BK_A
static uint16_t etq_direccion(uint16_t op)
{
    return etq_get(etq_slot(op));
}
#endif

/* ------------------------------------------------------------ el programa */

#if BK_A
static void salto_cond(uint16_t x, uint16_t etq, uint8_t si_cero)
{
    char n[24];

    ld_hl(x);
    i1(T("LD A,H"), 0x7C);
    i1(T("OR L"), 0xB5);
    etq_nombre(etq, n);
    i3(T(si_cero ? "JP Z,%s" : "JP NZ,%s"), n, si_cero ? 0xCA : 0xC2, etq_direccion(etq));
}
#endif

/* ------------------------------------------------------------ CHARACTER */
/* Una cadena se pasa a las rutinas como puntero a los datos + longitud de 1 byte (1..255):
 * primera cadena HL,B; segunda DE,C. Variables: V_x (longitud fija); constantes: K_n+2;
 * temporales: Tn = [longitud (2 B, solo se usa el bajo)][datos], longitud en tiempo de ejecucion. */
#if BK_C
/* Cadenas de longitud variable (temporales y formales CHARACTER*(*)): [longitud (2 B)][datos]. cadena_info deja aqui el nombre y
 * la direccion de la celda de la longitud de la ultima cadena de ese tipo que describio. */
#ifdef MF_SIN_LISTADO
static char ci_cel[1];
#else
static char ci_cel[24];
#endif
static uint16_t ci_cel_dir;
#endif

#if BK_C
static void cadena_info(uint16_t op, char *nom, uint16_t *dir, int *len)
{
    uint16_t v = TAC_VALOR(op);
    Simbolo s;
    uint8_t cab[2];

    *len = 0;
    *dir = 0;
    strcpy(nom, "?");
    switch (TAC_CLASE(op)) {
    case TAC_VAR:
        sim_leer(v, &s);
        if (SIM_TIPO(&s) != TD_CHARACTER || SIM_CLASE(&s) == SIM_ARRAY) {
            fallo(BACK_NO_SOPORTADO);
            return;
        }
        nombre_v(nom, &s, v);
        *dir = var_get(v);
        *len = s.extra;
        if (s.flags & SIMF_LONG_ASUM) {                 /* CHARACTER*(*): como un temporal, la longitud va en la celda */
            strcpy(ci_cel, nom);
            strcat(nom, "+2");
            ci_cel_dir = *dir;
            *dir = (uint16_t)(*dir + 2);
            *len = -1;
        }
        break;
    case TAC_CTE:
        if (tac_cte_tipo(op) != TAC_C_CADENA) {
            fallo(BACK_NO_SOPORTADO);
            return;
        }
        banco_leer(TAC_PAGINA_POOL, (uint16_t)(v << 1), cab, 2);
        sprintf(nom, "K_%u+2", (unsigned)(v << 1));
        *dir = (uint16_t)(bk_dpool + (v << 1) + 2);
        *len = cab[1];
        break;
    case TAC_TEMP:
        if (v >= NTMP) {
            fallo(BACK_NO_SOPORTADO);
            return;
        }
        if (v + 1 > bk_tmax) bk_tmax = (uint16_t)(v + 1);
        sprintf(nom, "T%u+2", (unsigned)v);
        sprintf(ci_cel, "T%u", (unsigned)v);
        ci_cel_dir = tdir_get(v);
        *dir = (uint16_t)(ci_cel_dir + 2);
        *len = -1;
        break;
    default:
        fallo(BACK_NO_SOPORTADO);
        break;
    }
}
#endif

#if BK_C
/* Carga la cadena: seg 0 -> HL (datos), B (longitud); seg 1 -> DE, C. Solo toca A, HL/DE y B/C. */
static void ld_str(uint16_t op, int seg)
{
    char nom[24], tx[24];
    uint16_t dir;
    int len;

    cadena_info(op, nom, &dir, &len);
    i3(T(seg ? "LD DE,%s" : "LD HL,%s"), nom, seg ? 0x11 : 0x21, dir);
    if (len >= 0) {
        sprintf(tx, "LD %c,%d", seg ? 'C' : 'B', len);
        i2(tx, seg ? 0x0E : 0x06, (uint8_t)len);
    } else {
        i3(T("LD A,(%s)"), ci_cel, 0x3A, ci_cel_dir);
        if (seg) i1(T("LD C,A"), 0x4F); else i1(T("LD B,A"), 0x47);
    }
}
#endif

#if BK_C
/* Datos del array CHARACTER: longitud de elemento y direccion base. */
static uint8_t array_char(uint16_t op, char *nom, uint16_t *dir)
{
    uint16_t v = TAC_VALOR(op);
    Simbolo s;
    Dim d;

    sim_leer(v, &s);
    if (TAC_CLASE(op) != TAC_VAR || SIM_TIPO(&s) != TD_CHARACTER || SIM_CLASE(&s) != SIM_ARRAY) {
        fallo(BACK_NO_SOPORTADO);
        strcpy(nom, "?");
        *dir = 0;
        return 1;
    }
    sim_dim_leer(s.extra, &d);
    nombre_v(nom, &s, v);
    *dir = var_get(v);
    return (uint8_t)d.len;
}
#endif

#if BK_C
static void direccion_elemento(uint16_t arr, uint16_t off, uint8_t elen)
{
    char nom[24], tx[16];
    uint16_t dir;

    array_char(arr, nom, &dir);
    ld_hl(off);
    sprintf(tx, "%u", (unsigned)elen);
    i3(T("LD DE,%s"), tx, 0x11, elen);
    call_rt(RT_MUL);
    (void)dir;
    ld_de_base(arr);
    i1(T("ADD HL,DE"), 0x19);
}
#endif

#if BK_C
static void traducir_c(const TacInst *t)
{
    char nom[24], tx[40];
    uint16_t dir, v;
    int len;
    uint8_t elen;

    switch (t->op) {
    case TOP_EXT:
        break;
    case TOP_TMP:
        v = TAC_VALOR(t->a);
        if (v >= NTMP) {
            fallo(BACK_NO_SOPORTADO);
            break;
        }
        if (v + 1 > bk_tmax) bk_tmax = (uint16_t)(v + 1);
        tcap_set(v, (uint8_t)tac_inm_valor(t->b));
        if (bk_pasada == 1 && (uint16_t)(2 + tcap_get(v)) > tsz_get(v)) {
            tsz_set(v, (uint16_t)(2 + tcap_get(v)));
        }
        break;
    case TOP_PUT:
        ld_str(t->a, 0);
        call_rt(RT_PUTS);
        break;
    case TOP_RD:                                 /* DE = donde van los caracteres, C = capacidad */
        cadena_info(t->a, nom, &dir, &len);
        if (TAC_CLASE(t->a) == TAC_TEMP && len < 0) {
            len = tcap_get(TAC_VALOR(t->a));
            sprintf(tx, "LD A,%d", len);
            i2(tx, 0x3E, (uint8_t)len);
            i3(T("LD (%s),A"), ci_cel, 0x32, ci_cel_dir);
            i3(T("LD DE,%s"), nom, 0x11, dir);
            sprintf(tx, "LD C,%d", len);
            i2(tx, 0x0E, (uint8_t)len);
        } else if (len < 0) {                    /* CHARACTER*(*): la longitud vigente */
            i3(T("LD DE,%s"), nom, 0x11, dir);
            i3(T("LD A,(%s)"), ci_cel, 0x3A, ci_cel_dir);
            i1(T("LD C,A"), 0x4F);
        } else {
            i3(T("LD DE,%s"), nom, 0x11, dir);
            sprintf(tx, "LD C,%d", len);
            i2(tx, 0x0E, (uint8_t)len);
        }
        call_rt(RT_RDS);
        break;
    case TOP_MOV:
        ld_str(t->b, 0);
        cadena_info(t->a, nom, &dir, &len);
        if (len < 0 && TAC_CLASE(t->a) != TAC_VAR) {
            fallo(BACK_NO_SOPORTADO);
            break;
        }
        i3(T("LD DE,%s"), nom, 0x11, dir);
        if (len < 0) {                           /* CHARACTER*(*): la longitud vigente */
            i3(T("LD A,(%s)"), ci_cel, 0x3A, ci_cel_dir);
            i1(T("LD C,A"), 0x4F);
        } else {
            sprintf(tx, "LD C,%d", len);
            i2(tx, 0x0E, (uint8_t)len);
        }
        call_rt(RT_SMOV);
        break;
    case TOP_EQ: case TOP_NE: case TOP_LT: case TOP_LE: case TOP_GT: case TOP_GE:
        ld_str(t->b, 0);
        ld_str(t->c, 1);
        call_rt(RT_SCMP);
        i3(T("LD DE,%s"), "0", 0x11, 0);
        call_rt(t->op == TOP_EQ ? RT_EQ : t->op == TOP_NE ? RT_NE : t->op == TOP_LT ? RT_LT :
                t->op == TOP_LE ? RT_LE : t->op == TOP_GT ? RT_GT : RT_GE);
        st_hl(t->a);
        break;
    case TOP_FUNR:                               /* a = temporal, b = variable CHARACTER resultado de la funcion */
        cadena_info(t->b, nom, &dir, &len);
        v = TAC_VALOR(t->a);
        if (len < 0 || TAC_CLASE(t->a) != TAC_TEMP || v >= NTMP) {
            fallo(BACK_NO_SOPORTADO);
            break;
        }
        sprintf(tx, "LD A,%d", len);
        i2(tx, 0x3E, (uint8_t)len);
        sprintf(tx, "T%u", (unsigned)v);
        i3(T("LD (%s),A"), tx, 0x32, tdir_get(v));
        i3(T("LD HL,%s"), nom, 0x21, dir);
        sprintf(tx, "T%u+2", (unsigned)v);
        i3(T("LD DE,%s"), tx, 0x11, (uint16_t)(tdir_get(v) + 2));
        sprintf(tx, "%d", len);
        i3(T("LD BC,%s"), tx, 0x01, (uint16_t)len);
        i2(T("LDIR"), 0xED, 0xB0);
        break;
    case TOP_LDE:
        elen = array_char(t->b, nom, &dir);
        v = TAC_VALOR(t->a);
        if (TAC_CLASE(t->a) != TAC_TEMP || v >= NTMP) {
            fallo(BACK_NO_SOPORTADO);
            break;
        }
        direccion_elemento(t->b, t->c, elen);
        sprintf(tx, "T%u", (unsigned)v);
        sprintf(nom, "%u", (unsigned)elen);
        sprintf(tx + 8, "LD A,%u", (unsigned)elen);
        i2(tx + 8, 0x3E, elen);
        i3(T("LD (%s),A"), tx, 0x32, tdir_get(v));
        sprintf(tx + 8, "T%u+2", (unsigned)v);
        i3(T("LD DE,%s"), tx + 8, 0x11, (uint16_t)(tdir_get(v) + 2));
        i3(T("LD BC,%s"), nom, 0x01, elen);
        i2(T("LDIR"), 0xED, 0xB0);
        break;
    case TOP_STE:
        elen = array_char(t->a, nom, &dir);
        direccion_elemento(t->a, t->b, elen);
        i1(T("PUSH HL"), 0xE5);
        ld_str(t->c, 0);
        i1(T("POP DE"), 0xD1);
        sprintf(tx, "LD C,%u", (unsigned)elen);
        i2(tx, 0x0E, elen);
        call_rt(RT_SMOV);
        break;
    case TOP_SCAT: {
        uint8_t cap;

        v = TAC_VALOR(t->a);
        if (TAC_CLASE(t->a) != TAC_TEMP || v >= NTMP || tcap_get(v) == 0) {
            fallo(BACK_NO_SOPORTADO);
            break;
        }
        cap = tcap_get(v);
        sprintf(tx, "T%u+2", (unsigned)v);
        i3(T("LD DE,%s"), tx, 0x11, (uint16_t)(tdir_get(v) + 2));
        sprintf(tx, "LD C,%u", (unsigned)cap);
        i2(tx, 0x0E, cap);
        ld_str(t->b, 0);
        call_rt(RT_SADD);
        ld_str(t->c, 0);
        call_rt(RT_SADD);
        sprintf(tx, "LD A,%u", (unsigned)cap);
        i2(tx, 0x3E, cap);
        i1(T("SUB C"), 0x91);
        sprintf(tx, "T%u", (unsigned)v);
        i3(T("LD (%s),A"), tx, 0x32, tdir_get(v));
        break;
    }
    case TOP_SSUB: {
        TacInst x;
        uint8_t cap;
        char base[24];
        uint16_t bdir;
        int blen;

        v = TAC_VALOR(t->a);
        if (TAC_CLASE(t->a) != TAC_TEMP || v >= NTMP || tcap_get(v) == 0) {
            fallo(BACK_NO_SOPORTADO);
            break;
        }
        cap = tcap_get(v);
        tac_leer((uint16_t)(bk_cur + 1), &x);
        cadena_info(t->b, base, &bdir, &blen);
        ld_hl(x.a);
        ld_de(t->c);
        i1(T("OR A"), 0xB7);
        i2(T("SBC HL,DE"), 0xED, 0x52);
        i1(T("INC HL"), 0x23);
        sprintf(tx, "LD C,%u", (unsigned)cap);
        i2(tx, 0x0E, cap);
        call_rt(RT_CLAMP);
        sprintf(tx, "T%u", (unsigned)v);
        i3(T("LD (%s),A"), tx, 0x32, tdir_get(v));
        i1(T("LD C,A"), 0x4F);
        i2(T("LD B,0"), 0x06, 0);
        ld_hl(t->c);
        i1(T("DEC HL"), 0x2B);
        i3(T("LD DE,%s"), base, 0x11, bdir);
        i1(T("ADD HL,DE"), 0x19);
        sprintf(tx, "T%u+2", (unsigned)v);
        i3(T("LD DE,%s"), tx, 0x11, (uint16_t)(tdir_get(v) + 2));
        call_rt(RT_SCPY);
        break;
    }
    case TOP_SSET: {                             /* base(i:j) := e: DE = base + i - 1, C = longitud j-i+1 acotada (RT_CLAMP) */
        TacInst x;
        uint8_t cap;
        char base[24];
        uint16_t bdir;
        int blen;

        tac_leer((uint16_t)(bk_cur + 1), &x);
        cadena_info(t->a, base, &bdir, &blen);
        v = TAC_VALOR(t->a);
        if (blen >= 0) {
            cap = (uint8_t)blen;
        } else if (TAC_CLASE(t->a) == TAC_TEMP && v < NTMP && tcap_get(v) != 0) {
            cap = tcap_get(v);
        } else if (TAC_CLASE(t->a) == TAC_VAR) {
            cap = 255;                           /* CHARACTER*(*) */
        } else {
            fallo(BACK_NO_SOPORTADO);
            break;
        }
        ld_hl(x.a);
        ld_de(t->b);
        i1(T("OR A"), 0xB7);
        i2(T("SBC HL,DE"), 0xED, 0x52);
        i1(T("INC HL"), 0x23);
        sprintf(tx, "LD C,%u", (unsigned)cap);
        i2(tx, 0x0E, cap);
        call_rt(RT_CLAMP);
        i1(T("PUSH AF"), 0xF5);
        ld_hl(t->b);
        i1(T("DEC HL"), 0x2B);
        i3(T("LD DE,%s"), base, 0x11, bdir);
        i1(T("ADD HL,DE"), 0x19);
        i1(T("PUSH HL"), 0xE5);
        ld_str(t->c, 0);
        i1(T("POP DE"), 0xD1);
        i1(T("POP AF"), 0xF1);
        i1(T("LD C,A"), 0x4F);
        call_rt(RT_SMOV);
        break;
    }
    case TOP_SLEN:                               /* a = LEN(b): la longitud vigente de una cadena */
        ld_str(t->b, 0);
        i1(T("LD A,B"), 0x78);
        i1(T("LD L,A"), 0x6F);
        i2(T("LD H,0"), 0x26, 0);
        st_hl(t->a);
        break;
    default:
        fallo(BACK_NO_SOPORTADO);
        break;
    }
}
#endif

#if BK_C
/* STOP n / STOP 'texto': imprime "STOP n" o "STOP texto" y un salto de linea (spec del TAC, STOP [a]).
 * RT_PUTI y RT_PUTS ya escriben el blanco que separa. */
static void stop_mensaje(uint16_t a)
{
    const char *p;
    char tx[16];

    for (p = "STOP"; *p != '\0'; p++) {
        sprintf(tx, "LD A,%u", (unsigned)*p);
        i2(tx, 0x3E, (uint8_t)*p);
        call_rt(RT_PUTC);
    }
    if (TAC_CLASE(a) == TAC_CTE && tac_cte_tipo(a) == TAC_C_CADENA) {
        ld_str(a, 0);
        call_rt(RT_PUTS);
    } else {
        ld_hl(a);
        call_rt(RT_PUTI);
    }
    i2(T("LD A,13"), 0x3E, 13);
    call_rt(RT_PUTC);
}

static void param_c(const TacInst *t, uint16_t formal, uint16_t k)
{
    TacInst m;

    if (k < 256) {    /* CHARACTER*(*): la copia de entrada fija tambien la longitud del formal */
        Simbolo fs;

        sim_leer(TAC_VALOR(formal), &fs);
        if (fs.flags & SIMF_LONG_ASUM) {
            char fn[24];
            uint16_t fd;
            int fl;

            ld_str(t->a, 0);
            cadena_info(formal, fn, &fd, &fl);
            i1(T("LD A,B"), 0x78);
            i3(T("LD (%s),A"), ci_cel, 0x32, ci_cel_dir);
            i3(T("LD DE,%s"), fn, 0x11, fd);
            i1(T("LD C,A"), 0x4F);
            call_rt(RT_SMOV);
            return;
        }
    }
    if (k >= 256) {   /* copia de salida CHARACTER: solo los caracteres del formal (LDIR) */
        char fn[24], an[24], tx[16];
        uint16_t fd, ad;
        int fl, al;

        cadena_info(t->a, an, &ad, &al);
        cadena_info(formal, fn, &fd, &fl);
        i3(T("LD HL,%s"), fn, 0x21, fd);
        i3(T("LD DE,%s"), an, 0x11, ad);
        if (fl < 0) {                            /* CHARACTER*(*): la longitud vigente (0 no copia nada) */
            i3(T("LD A,(%s)"), ci_cel, 0x3A, ci_cel_dir);
            i1(T("OR A"), 0xB7);
            i1(T("LD C,A"), 0x4F);
            i2(T("LD B,0"), 0x06, 0);
            i2(T("JR Z,$+4"), 0x28, 2);
        } else {
            sprintf(tx, "%d", fl);
            i3(T("LD BC,%s"), tx, 0x01, (uint16_t)fl);
        }
        i2(T("LDIR"), 0xED, 0xB0);
        return;
    }
    memset(&m, 0, sizeof(m));                    /* copia de entrada de siempre: MOV.C formal := actual (longitud del formal) */
    m.op = TOP_MOV;
    m.tipo = t->tipo;
    m.a = formal;
    m.b = t->a;
    traducir_c(&m);
}

#if BK_C && !BK_A
/* Puerta de la parte C: cual 0 = traducir_c(t), 1 = stop_mensaje(x), 2 = param_c(t, formal x, k y). */
void back_c_entrada(uint8_t cual, const TacInst *t, uint16_t x, uint16_t y)
{
    if (cual == 0) {
        traducir_c(t);
    } else if (cual == 1) {
        stop_mensaje(x);
    } else {
        param_c(t, x, y);
    }
}
#endif
#endif

/* ----------------------------------------------------------------- REAL */
/* Un REAL ocupa 4 bytes (variable, elemento de array, temporal, constante del pool). Las operaciones se llaman por DIRECCION:
 * HL = a, DE = b, BC = resultado (las rutinas de back_rt_real.h). */

#if BK_A
/* Direccion del REAL de un operando (y su nombre en `simbolo`). Un temporal usado como REAL reserva 4 bytes. */
static uint16_t dir_real(uint16_t op)
{
    uint16_t v = TAC_VALOR(op);

    if (TAC_CLASE(op) == TAC_CTE && tac_cte_tipo(op) == TAC_C_REAL) {
        sprintf(simbolo, "K_%u+2", (unsigned)(v << 1));
        return (uint16_t)(bk_dpool + (v << 1) + 2);
    }
    if (TAC_CLASE(op) == TAC_TEMP && v < NTMP && bk_pasada == 1 && tsz_get(v) < 4) {
        tsz_set(v, 4);
    }
    if (TAC_CLASE(op) == TAC_OCULTA && v + 2 > bk_hmax) {      /* una oculta REAL ocupa dos palabras */
        bk_hmax = (uint16_t)(v + 2);
    }
    return dir_mem(op);
}
#endif

#if BK_A
/* LD rr,<direccion del REAL>: rr = 0x21 (HL), 0x11 (DE), 0x01 (BC) */
static void ld_dir(uint8_t opcode, const char *reg, uint16_t op)
{
    uint16_t d = dir_real(op);
    char tx[16];

    sprintf(tx, "LD %s,%%s", reg);
    i3(tx, simbolo, opcode, d);
}
#endif

#define LD_HL_R(op) ld_dir(0x21, T("HL"), op)
#define LD_DE_R(op) ld_dir(0x11, T("DE"), op)
#define LD_BC_R(op) ld_dir(0x01, T("BC"), op)

#if BK_A
static void traducir_r(const TacInst *t)
{
    uint8_t rt;

    switch (t->op) {
    case TOP_MOV:
        LD_HL_R(t->b);
        LD_DE_R(t->a);
        i3(T("LD BC,%s"), "4", 0x01, 4);
        i2(T("LDIR"), 0xED, 0xB0);
        break;
    case TOP_NEG:
        LD_HL_R(t->b);
        LD_BC_R(t->a);
        call_rt(RT_FNEG);
        break;
    case TOP_ADD: case TOP_SUB: case TOP_MUL: case TOP_DIV:
        LD_HL_R(t->b);
        LD_DE_R(t->c);
        LD_BC_R(t->a);
        if (t->op == TOP_ADD || t->op == TOP_SUB) {
            if (t->op == TOP_ADD) {
                i1(T("XOR A"), 0xAF);
            } else {
                i2(T("LD A,1"), 0x3E, 1);
            }
            rt = RT_FADD;
        } else {
            rt = t->op == TOP_MUL ? RT_FMUL : RT_FDIV;
        }
        call_rt(rt);
        break;
    case TOP_POW:                       /* REAL ** INTEGER */
        LD_HL_R(t->b);
        ld_de(t->c);
        LD_BC_R(t->a);
        call_rt(RT_FPOW);
        break;
    case TOP_RMOD:
        LD_HL_R(t->b);
        LD_DE_R(t->c);
        LD_BC_R(t->a);
        call_rt(RT_FMOD);
        break;
    case TOP_RFUN:                      /* c = 0 AINT, 1 ANINT, 2 SQRT */
        LD_HL_R(t->b);
        LD_BC_R(t->a);
        if (imm_val(t->c) == 2) {
            call_rt(RT_FSQRT);
        } else {
            char tx[16];

            sprintf(tx, "LD A,%d", (int)imm_val(t->c));
            i2(tx, 0x3E, (uint8_t)imm_val(t->c));
            call_rt(RT_FINT);
        }
        break;
    case TOP_ITOF:
        ld_hl(t->b);
        LD_BC_R(t->a);
        call_rt(RT_ITOF);
        break;
    case TOP_FTOI:
        LD_HL_R(t->b);
        call_rt(RT_FTOI);
        st_hl(t->a);
        break;
    case TOP_EQ: case TOP_NE: case TOP_LT: case TOP_LE: case TOP_GT: case TOP_GE:
        LD_HL_R(t->b);
        LD_DE_R(t->c);
        call_rt(RT_FCMP);
        i3(T("LD DE,%s"), "0", 0x11, 0);
        call_rt(t->op == TOP_EQ ? RT_EQ : t->op == TOP_NE ? RT_NE : t->op == TOP_LT ? RT_LT :
                t->op == TOP_LE ? RT_LE : t->op == TOP_GT ? RT_GT : RT_GE);
        st_hl(t->a);
        break;
    case TOP_PUT:
        LD_HL_R(t->a);
        call_rt(RT_FPUT);
        break;
    case TOP_LDE: case TOP_STE: {
        uint16_t arr = t->op == TOP_LDE ? t->b : t->a;
        uint16_t off = t->op == TOP_LDE ? t->c : t->b;

        ld_hl(off);
        i1(T("ADD HL,HL"), 0x29);
        i1(T("ADD HL,HL"), 0x29);
        ld_de_base(arr);
        i1(T("ADD HL,DE"), 0x19);
        if (t->op == TOP_LDE) {
            LD_DE_R(t->a);
        } else {
            i1(T("EX DE,HL"), 0xEB);
            LD_HL_R(t->c);
        }
        i3(T("LD BC,%s"), "4", 0x01, 4);
        i2(T("LDIR"), 0xED, 0xB0);
        break;
    }
    default:
        fallo(BACK_NO_SOPORTADO);
        break;
    }
}
#endif

/* Unidad llamada por un PARAM / CALL: su formal k es el simbolo (sim_ini + k); la copia es un MOV del TAC. */
#if BK_A
static void traducir_param(const TacInst *t);
#endif

#if BK_A
static void traducir(const TacInst *t)
{
    char n[24];

    if (t->op == TOP_DATA) {
        return;                              /* ya salio en datos_iniciales */
    }
    if (t->op == TOP_PARAM) {
        traducir_param(t);
        return;
    }
    if (t->op == TOP_RDLN) {
        call_rt(RT_RDLN);
        return;
    }
    if (t->op == TOP_RD && t->tipo != TD_CHARACTER) {     /* HL = direccion del destino */
        if (t->tipo == TD_REAL) {
            LD_HL_R(t->a);
        } else {
            uint16_t d = dir_mem(t->a);

            i3(T("LD HL,%s"), simbolo, 0x21, d);
        }
        call_rt(t->tipo == TD_REAL ? RT_RDF : t->tipo == TD_LOGICAL ? RT_RDL : RT_RDI);
        return;
    }
    if (t->op == TOP_FUNR) {                 /* d = resultado de la funcion: MOV de su variable resultado */
        TacInst m;
        Unidad x;

        uni_leer((uint8_t)tac_inm_valor(t->b), &x);
        memset(&m, 0, sizeof(m));
        m.op = t->tipo == TD_CHARACTER ? TOP_FUNR : TOP_MOV;
        m.tipo = t->tipo;
        m.a = t->a;
        m.b = TAC_OPER(TAC_VAR, x.sim_res);
        if (m.op == TOP_FUNR) {
            traducir_c(&m);                      /* el resultado CHARACTER se copia a un temporal */
        } else {
            traducir(&m);
        }
        return;
    }
    if (t->tipo == TD_CHARACTER || t->op == TOP_SLEN || t->op == TOP_EXT || t->op == TOP_TMP || t->op == TOP_SSET) {
        traducir_c(t);
        return;
    }
    if (t->tipo == TD_REAL || t->op == TOP_ITOF || t->op == TOP_FTOI) {
        traducir_r(t);
        return;
    }
    switch (t->op) {
    case TOP_NOP:
        break;
    case TOP_LBL:
        etq_nombre(t->a, n);
        if (bk_pasada == 1) {
            etq_set(etq_slot(t->a), z_pc());
        }
        strcat(n, ":");
        z_linea(n);
        break;
    case TOP_MOV:
        ld_hl(t->b);
        st_hl(t->a);
        break;
    case TOP_NEG:
        ld_de(t->b);
        i3(T("LD HL,%s"), "0", 0x21, 0);
        i1(T("OR A"), 0xB7);
        i2(T("SBC HL,DE"), 0xED, 0x52);
        st_hl(t->a);
        break;
    case TOP_NOT:
        ld_hl(t->b);
        i1(T("LD A,L"), 0x7D);
        i2(T("XOR 1"), 0xEE, 0x01);
        i1(T("LD L,A"), 0x6F);
        st_hl(t->a);
        break;
    case TOP_ADD: case TOP_SUB: case TOP_MUL: case TOP_DIV: case TOP_POW:
        ld_hl(t->b);
        ld_de(t->c);
        switch (t->op) {
        case TOP_ADD: i1(T("ADD HL,DE"), 0x19); break;
        case TOP_SUB: i1(T("OR A"), 0xB7); i2(T("SBC HL,DE"), 0xED, 0x52); break;
        case TOP_MUL: call_rt(RT_MUL); break;
        case TOP_DIV: call_rt(RT_DIV); break;
        default:      call_rt(RT_POW); break;
        }
        st_hl(t->a);
        break;
    case TOP_EQ: case TOP_NE: case TOP_LT: case TOP_LE: case TOP_GT: case TOP_GE:
        ld_hl(t->b);
        ld_de(t->c);
        call_rt(t->op == TOP_EQ ? RT_EQ : t->op == TOP_NE ? RT_NE : t->op == TOP_LT ? RT_LT :
                t->op == TOP_LE ? RT_LE : t->op == TOP_GT ? RT_GT : RT_GE);
        st_hl(t->a);
        break;
    case TOP_AND: case TOP_OR: case TOP_EQV: case TOP_NEQV:
        ld_hl(t->b);
        ld_de(t->c);
        i1(T("LD A,L"), 0x7D);
        if (t->op == TOP_AND) {
            i1(T("AND E"), 0xA3);
        } else if (t->op == TOP_OR) {
            i1(T("OR E"), 0xB3);
        } else {
            i1(T("XOR E"), 0xAB);
            if (t->op == TOP_EQV) {
                i2(T("XOR 1"), 0xEE, 0x01);
            }
        }
        i1(T("LD L,A"), 0x6F);
        st_hl(t->a);
        break;
    case TOP_JMP:
        etq_nombre(t->a, n);
        i3(T("JP %s"), n, 0xC3, etq_direccion(t->a));
        break;
    case TOP_JT:
        salto_cond(t->a, t->b, 0);
        break;
    case TOP_JF:
        salto_cond(t->a, t->b, 1);
        break;
    case TOP_JEQ:
        ld_hl(t->a);
        ld_de(t->b);
        i1(T("OR A"), 0xB7);
        i2(T("SBC HL,DE"), 0xED, 0x52);
        etq_nombre(t->c, n);
        i3(T("JP Z,%s"), n, 0xCA, etq_direccion(t->c));
        break;
    case TOP_LDE: case TOP_STE: {
        uint16_t arr = t->op == TOP_LDE ? t->b : t->a;
        uint16_t off = t->op == TOP_LDE ? t->c : t->b;

        ld_hl(off);
        i1(T("ADD HL,HL"), 0x29);
        ld_de_base(arr);
        i1(T("ADD HL,DE"), 0x19);
        if (t->op == TOP_LDE) {
            i1(T("LD E,(HL)"), 0x5E);
            i1(T("INC HL"), 0x23);
            i1(T("LD D,(HL)"), 0x56);
            i1(T("EX DE,HL"), 0xEB);
            st_hl(t->a);
        } else {
            i1(T("EX DE,HL"), 0xEB);
            ld_hl(t->c);
            i1(T("EX DE,HL"), 0xEB);
            i1(T("LD (HL),E"), 0x73);
            i1(T("INC HL"), 0x23);
            i1(T("LD (HL),D"), 0x72);
        }
        break;
    }
    case TOP_PUT:
        ld_hl(t->a);
        call_rt(t->tipo == TD_LOGICAL ? RT_PUTL : RT_PUTI);
        break;
    case TOP_PUTNL:
        i2(T("LD A,13"), 0x3E, 13);
        call_rt(RT_PUTC);
        break;
    case TOP_STOP: case TOP_HALT: case TOP_RET:
        if (t->op == TOP_STOP && t->a != 0) {
            stop_mensaje(t->a);
        }
        if (t->op == TOP_STOP && uni_cuenta() > 1) {                /* STOP en un subprograma: se desapilan las llamadas */
            i4(T("LD SP,(%s)"), "SPSAVE", 0xED, 0x7B, spsave);
        }
        i1(T("RET"), 0xC9);
        break;
    case TOP_ENTER:
        cur_u = (uint8_t)tac_inm_valor(t->a);
        uni_dir_set(cur_u, z_pc());
#ifndef MF_SIN_LISTADO
        {
            Unidad x;

            uni_leer(cur_u, &x);
            sprintf(n, "U_%.8s:", x.nombre[0] != '\0' ? x.nombre : "MAIN");
            z_linea(n);
        }
#endif
        break;
    case TOP_CALL:
        {
#ifndef MF_SIN_LISTADO
            Unidad x;

            uni_leer((uint8_t)tac_inm_valor(t->a), &x);
            sprintf(n, "U_%.8s", x.nombre[0] != '\0' ? x.nombre : "MAIN");
#endif
            i3(T("CALL %s"), n, 0xCD, uni_dir_get((uint8_t)tac_inm_valor(t->a)));
        }
        break;
    default:                            /* TMP, EXT, SCAT, SSUB, SLEN: CHARACTER */
        fallo(BACK_NO_SOPORTADO);
        break;
    }
}
#endif

#if BK_A
static void traducir_param(const TacInst *t)
{
    TacInst m;
    Unidad x;
    uint16_t k = (uint16_t)tac_inm_valor(t->b);
    uint16_t formal;

    uni_leer((uint8_t)tac_inm_valor(t->c), &x);
    formal = TAC_OPER(TAC_VAR, (uint16_t)(x.sim_ini + (k & 255u)));
    if (k & (512u | 1024u)) {                    /* array entero (512) o elemento (1024, con EXT arr, off) */
        Simbolo fs;
        TacInst e;

        sim_leer(TAC_VALOR(formal), &fs);
        if (SIM_CLASE(&fs) == SIM_ARRAY) {
            if (k & 512u) {
                ld_hl_base(t->a);
            } else {
                tac_leer((uint16_t)(bk_cur + 1), &e);
                if (k & 256u) {                  /* salida: el formal escribio en el array real; se recarga el temporal */
                    memset(&m, 0, sizeof(m));
                    m.op = TOP_LDE;
                    m.tipo = t->tipo;
                    m.a = t->a;
                    m.b = e.a;
                    m.c = e.b;
                    traducir(&m);
                    return;
                }
                ld_hl(e.b);                      /* HL = direccion del elemento */
                if (t->tipo == TD_CHARACTER) {
                    Dim dm;
                    Simbolo as;
                    char n_el[8];

                    sim_leer(TAC_VALOR(e.a), &as);
                    sim_dim_leer(as.extra, &dm);
                    sprintf(n_el, "%u", (unsigned)dm.len);
                    i3(T("LD DE,%s"), n_el, 0x11, dm.len);
                    call_rt(RT_MUL);
                } else {
                    i1(T("ADD HL,HL"), 0x29);
                    if (t->tipo == TD_REAL) {
                        i1(T("ADD HL,HL"), 0x29);
                    }
                }
                ld_de_base(e.a);
                i1(T("ADD HL,DE"), 0x19);
            }
            st_hl(formal);
            return;
        }
        k = (uint16_t)(k & ~(512u | 1024u));     /* formal escalar: copia de entrada / salida de siempre */
    }
    if (t->tipo == TD_CHARACTER) {               /* copia de entrada / salida de una cadena: plantillas CHARACTER */
        param_c(t, formal, k);
        return;
    }
    memset(&m, 0, sizeof(m));
    m.op = TOP_MOV;
    m.tipo = t->tipo;
    if (k >= 256) {                         /* copia de salida: actual := formal */
        m.a = t->a;
        m.b = formal;
    } else {                                /* copia de entrada: formal := actual */
        m.a = formal;
        m.b = t->a;
    }
    traducir(&m);
}
#endif

/* ---------------------------------------------------------------- rutinas */

#if BK_A
#include "back_rt.h"
#endif

#if BK_A
/* n bytes a cero (el bloque de trabajo REAL). Con listado, una sola linea de datos; sin listado, de dos en dos. */
static void datos_cero(uint8_t n)
{
#ifdef MF_SIN_LISTADO
    static const uint8_t cero[2] = { 0, 0 };

    for (; n >= 2; n = (uint8_t)(n - 2)) {
        z_datos(cero, 2);
    }
    if (n != 0) {
        z_datos(cero, 1);
    }
#else
    static const uint8_t cero[256];

    z_datos(cero, n);
#endif
}
#endif

#ifdef MF_SIN_LISTADO
/* Lectura de una tabla REAL que vive en otro banco: ventana de 24 bytes que se rellena por trampolin. */
uint8_t rt_real_leer(uint8_t r, uint16_t off, uint8_t *dst, uint8_t n);
uint8_t rt_ent_leer(uint8_t r, uint16_t off, uint8_t *dst, uint8_t n);

typedef struct {
    uint8_t  r, n;
    uint16_t lo, pos;
    uint8_t  buf[24];
} RtRd;

#if BK_A
static uint8_t rd_byte(RtRd *s)
{
    if (s->n == 0 || s->pos < s->lo || (uint16_t)(s->pos - s->lo) >= s->n) {
        s->lo = s->pos;
        s->n = s->r >= RT_RDD ? rt_ent_leer(s->r, s->pos, s->buf, 24) : rt_real_leer(s->r, s->pos, s->buf, 24);
    }
    return s->buf[s->pos++ - s->lo];
}
#endif
#endif

#if BK_A
/* Emite la rutina r recorriendo su tabla (back_rt.h). */
static void rutina(uint8_t r)
{
    const RtI *p;
    uint8_t k, b0, b1, b2;
    const char *t;
    const char *a;
#ifndef MF_SIN_LISTADO
    char b[24];
#else
    RtRd rd;                            /* las rutinas REAL se leen de su banco (rt_real_leer) */
    uint8_t ext = (uint8_t)(r >= RT_FPD);

    rd.r = r;
    rd.pos = 0;
    rd.n = 0;
#endif

    rt_dir_set(r, z_pc());
#ifndef MF_SIN_LISTADO
    sprintf(b, "%s:", rt_nom[r]);
    z_linea(b);
#endif
    if (r != RT_PUTC) {
        p = rt_cod[r];
    } else if (salida_pant) {
        p = rt_c_putc_pant;
    } else {
        p = rt_c_putc_rst;
    }
    for (;;) {
#ifdef MF_SIN_LISTADO
        k = ext ? rd_byte(&rd) : *p++;
        if (k == 0) {
            break;
        }
        b0 = b1 = b2 = 0;
        t = a = NULL;
        if ((k & 7) > 0) { b0 = ext ? rd_byte(&rd) : *p++; }
        if ((k & 7) > 1) { b1 = ext ? rd_byte(&rd) : *p++; }
        if ((k & 7) > 2) { b2 = ext ? rd_byte(&rd) : *p++; }
#else
        k = p->k;
        if (k == 0) {
            break;
        }
        b0 = p->b[0]; b1 = p->b[1]; b2 = p->b[2];
        t = p->t;
        a = p->a;
        p++;
#endif
        switch (k) {
        case RK_JR:    jr(t, b0, b1);     break;
        case RK_JPLOC: jp_loc(t, b0, b1); break;
        case RK_LOC:   loc_def(b0);       break;
        case RK_CALL:  call_rt(b0);       break;
        case RK_JP:    jp_rt(b0);         break;
        case RK_REF:   i3(t, a, b0, rt_cur_dir); break;
        case RK_CUR:
            z_linea(T("rt_cur:"));
            rt_cur_dir = z_pc();
            {
                static const uint8_t cero[2] = { 0, 0 };

                z_datos(cero, 2);
            }
            break;
        case RK_NEC:   necesita(b0);      break;
        case RK_RTOP:  i3(t, RT_NOM(b1), b0, rt_dir_get(b1)); break;
        case RK_REFO:  i3(t, a, b0, (uint16_t)(rt_dir_get(RT_FPD) + b1)); break;
        case RK_REFE:  i4(t, a, b0, b1, (uint16_t)(rt_dir_get(RT_FPD) + b2)); break;
        case RK_REFD:  i3(t, a, b0, (uint16_t)(rt_dir_get(RT_RDD) + b1)); break;
        case RK_REFDE: i4(t, a, b0, b1, (uint16_t)(rt_dir_get(RT_RDD) + b2)); break;
        case RK_DATA:  datos_cero(b0); break;
        case RK_DB1: case RK_DB2: case RK_DB3:
            {
                uint8_t d[3];

                d[0] = b0; d[1] = b1; d[2] = b2;
                z_datos(d, (uint16_t)(k & 7));
            }
            break;
        case RK_REFR:  i3(t, a, b0, (uint16_t)(rt_dir_get(r) + b1)); break;
        default:       z_ins(t, a, k, b0, b1, b2, 0); break;   /* 1..3 bytes tal cual */
        }
    }
}
#endif

#if BK_A
static const uint8_t zeros2[2] = { 0, 0 };
#endif

/* DATA: la inicializacion corre al arrancar el programa, antes de la primera unidad. Cada TOP_DATA (+ EXT con el elemento inicial) escribe
 * el primer valor con cargas inmediatas (o LDIR desde el pool, si es texto) y, si son varios elementos, los replica con un LDIR solapado. */
#if BK_A
static void dato_nombre(char *nm, const Simbolo *s, uint16_t id, uint16_t boff)
{
#ifndef MF_SIN_LISTADO
    nombre_v(nm, s, id);
    if (boff != 0) {
        sprintf(nm + strlen(nm), "+%u", (unsigned)boff);
    }
#else
    (void)nm; (void)s; (void)id; (void)boff;
#endif
}

static void datos_iniciales(void)
{
    uint16_t i, n = tac_cuenta();
    TacInst t, x;
    Simbolo s;
    Dim d;
    char nm[40];

    for (i = 0; (uint16_t)(i + 1) < n && bk_err == 0; i++) {
        uint16_t id, el, cnt, off, dst, boff, len, src;

        tac_leer(i, &t);
        if (t.op != TOP_DATA) {
            continue;
        }
        tac_leer((uint16_t)(i + 1), &x);
        id = TAC_VALOR(t.a);
        sim_leer(id, &s);
        el = t.tipo == TD_REAL ? 4 : 2;
        if (t.tipo == TD_CHARACTER) {
            if (SIM_CLASE(&s) == SIM_ARRAY) {
                sim_dim_leer(s.extra, &d);
                el = d.len;
            } else {
                el = s.extra;
            }
        }
        cnt = (uint16_t)imm_val(t.c);
        off = (uint16_t)imm_val(x.a);
        boff = (uint16_t)(off * el);
        dst = (uint16_t)(var_get(id) + boff);
        dato_nombre(nm, &s, id, boff);
        if (t.tipo == TD_CHARACTER) {
            uint8_t cab[2];

            banco_leer(TAC_PAGINA_POOL, (uint16_t)(TAC_VALOR(t.b) << 1), cab, 2);
            len = cab[1];
            src = (uint16_t)(bk_dpool + (TAC_VALOR(t.b) << 1) + 2);
            i3(T("LD HL,K_pool"), "", 0x21, src);
            i3(T("LD DE,%s"), nm, 0x11, dst);
            i3(T("LD BC,len"), "", 0x01, len);
            i2(T("LDIR"), 0xED, 0xB0);
            if (el > len) {                                 /* se rellena con blancos */
                i2(T("LD A,32"), 0x3E, 0x20);
                i1(T("LD (DE),A"), 0x12);
                if (el - len > 1) {
                    i1(T("LD H,D"), 0x62);
                    i1(T("LD L,E"), 0x6B);
                    i1(T("INC DE"), 0x13);
                    i3(T("LD BC,pad"), "", 0x01, (uint16_t)(el - len - 1));
                    i2(T("LDIR"), 0xED, 0xB0);
                }
            }
        } else if (t.tipo == TD_REAL) {
            uint32_t bits = tac_cte_real(t.b);

            i3(T("LD HL,lo"), "", 0x21, (uint16_t)(bits & 0xFFFF));
            i3(T("LD (%s),HL"), nm, 0x22, dst);
            i3(T("LD HL,hi"), "", 0x21, (uint16_t)(bits >> 16));
            i3(T("LD (%s+2),HL"), nm, 0x22, (uint16_t)(dst + 2));
        } else {
            i3(T("LD HL,valor"), "", 0x21, (uint16_t)imm_val(t.b));
            i3(T("LD (%s),HL"), nm, 0x22, dst);
        }
        if (cnt > 1) {
            i3(T("LD HL,%s"), nm, 0x21, dst);
            i3(T("LD DE,dst+el"), "", 0x11, (uint16_t)(dst + el));
            i3(T("LD BC,el*(n-1)"), "", 0x01, (uint16_t)(el * (cnt - 1)));
            i2(T("LDIR"), 0xED, 0xB0);
        }
    }
}
#endif

#if BK_A
static void todo(void)
{
    uint16_t i, n = tac_cuenta();
    uint8_t r;
    TacInst t;

    datos_iniciales();
    if (uni_cuenta() > 1) {                 /* varias unidades: se entra por el programa principal, este donde este */
        Unidad x;
        uint8_t u;
        char nm[16];

        for (u = 1; u <= uni_cuenta(); u++) {
            uni_leer(u, &x);
            if (x.clase == UK_PROGRAMA) {
                break;
            }
        }
        sprintf(nm, "U_%.8s", x.nombre[0] != '\0' ? x.nombre : "MAIN");
        spsave = (uint16_t)(z_pc() + 7);
        i4(T("LD (%s),SP"), "SPSAVE", 0xED, 0x73, spsave);
        i3(T("JP %s"), nm, 0xC3, uni_dir_get(u));
        z_linea(T("SPSAVE:"));
        z_datos(zeros2, 2);
    }
    for (i = 0; i < n && bk_err == 0; i++) {
        bk_cur = i;
        tac_leer(i, &t);
        traducir(&t);
    }
    for (r = 0; r < RT_N && bk_err == 0; r++) {
        if (rt_usa_get(r)) {
            rutina(r);
        }
    }
}
#endif

#if BK_A
/* Constantes: copia del pool con una etiqueta por entrada. */
static void emitir_pool(void)
{
    uint16_t usado = tac_pool_usado();
    uint16_t off = 0;
    uint8_t buf[260];
#ifndef MF_SIN_LISTADO
    char b[24];
#endif

    while (off < usado) {
        uint8_t cab[2];
        uint16_t tam;

        banco_leer(TAC_PAGINA_POOL, off, cab, 2);
        tam = (uint16_t)((2u + cab[1] + 1u) & ~1u);
        banco_leer(TAC_PAGINA_POOL, off, buf, tam);
#ifndef MF_SIN_LISTADO
        sprintf(b, "K_%u:", (unsigned)off);
        z_linea(b);
#endif
        z_datos(buf, tam);
        off = (uint16_t)(off + tam);
    }
}
#endif

#if BK_A
/* Producto de 16 bits saturado a 0xFFFF y numero de elementos de una dimension (sin aritmetica de 32 bits). */
static uint16_t mul_sat(uint16_t a, uint16_t b)
{
    if (b != 0 && a > (uint16_t)(0xFFFFu / b)) {
        return 0xFFFF;
    }
    return (uint16_t)(a * b);
}
#endif

#if BK_A
static uint16_t dim_n(const Dim *d, uint8_t k)
{
    uint16_t m = (uint16_t)(d->sup[k] - d->inf[k]);

    return m == 0xFFFF ? 0xFFFF : (uint16_t)(m + 1);
}
#endif

#if BK_A
static uint16_t tam_simbolo(uint16_t id)
{
    Simbolo s;
    Dim d;
    uint16_t n = 1;
    uint16_t el;

    sim_leer(id, &s);
    el = SIM_TIPO(&s) == TD_CHARACTER ? 0 : (SIM_TIPO(&s) == TD_REAL ? 4 : 2);
    if (SIM_CLASE(&s) == SIM_ARRAY && (s.flags & SIMF_FORMAL)) {
        return 2;                                    /* celda con la direccion del array real */
    }
    if (SIM_CLASE(&s) == SIM_ARRAY) {
        sim_dim_leer(s.extra, &d);
        n = dim_n(&d, 0);
        if (SIM_NDIM(&s) == 2) {
            n = mul_sat(n, dim_n(&d, 1));
        }
        if (el == 0) el = d.len;
    } else if (el == 0) {
        el = s.extra;
        if (s.flags & SIMF_LONG_ASUM) {
            el = (uint16_t)(2 + (s.extra == 0 ? 1 : s.extra));       /* CHARACTER*(*): [longitud][datos hasta el mayor real] */
        }
    }
    return mul_sat(n, el);
}
#endif

#ifndef MF_SIN_LISTADO
#if BK_A
static void equ(const char *nombre, uint16_t v)
{
    char b[48];

    sprintf(b, "%s EQU $", nombre);
    hex4(b + strlen(b), v);
    z_linea(b);
}
#endif
#endif

#if BK_A
void back_salida_pantalla(uint8_t si)
{
    salida_pant = si;
}

#ifndef MF_TARGET_NEXT
void back_entrada_prueba(uint8_t si)
{
    bk_ent_prueba = si;
}
#endif
#endif

#if BK_A
void back_generar(uint8_t *img, uint16_t cap, uint16_t org,
                  void (*sal)(const char *linea), BackRes *r)
{
    uint16_t id, n, fin_img;
    uint32_t a;
#ifndef MF_SIN_LISTADO
    char b[32];
#endif
#ifndef MF_SIN_LISTADO
    Simbolo s;
#endif

#ifdef MF_SIN_LISTADO
    sal = NULL;
#endif
    memset(r, 0, sizeof(*r));
    rt_cur_dir = 0;
    tab_cero(0, OFF_FIN);
    bk_err = 0;
    bk_err_inst = 0;
    bk_tmax = bk_hmax = 0;
    for (id = 0; id < NTMP; id++) {
        tsz_set(id, 2);
    }
    bk_dpool = dvars = tbase = bk_hbase = fin_mem = 0;

    /* bk_pasada 1: medir */
    bk_pasada = 1;
    z_iniciar(img, cap, org, 1, NULL);
    todo();
    if (bk_err == 0) {
        bk_dpool = z_pc();
        dvars = (uint16_t)(bk_dpool + tac_pool_usado());
        a = dvars;
        n = sim_cuenta();
        for (id = 1; id <= n; id++) {
            var_set(id, (uint16_t)a);
            a += tam_simbolo(id);
        }
        tbase = (uint16_t)a;
        for (id = 0; id < bk_tmax; id++) {
            tdir_set(id, (uint16_t)a);
            a += tsz_get(id);
        }
        bk_hbase = (uint16_t)a;
        a += 2u * bk_hmax;
        fin_mem = (uint16_t)a;
        fin_img = dvars;
        if (a > 0xFFFF || z_desbordado() || (uint16_t)(fin_img - org) > cap) {
            fallo(BACK_NO_CABE);
        }
    }
    if (bk_err == 0) {
        /* bk_pasada 2: emitir con todas las direcciones conocidas */
        bk_pasada = 2;
        z_iniciar(img, cap, org, 2, sal);
#ifndef MF_SIN_LISTADO
        strcpy(b, "        ORG $");
        hex4(b + strlen(b), org);
        z_linea(b);
#endif
        todo();
        emitir_pool();
        z_fin();
        if (z_pc() != dvars) {
            fallo(BACK_INTERNO);
        }
#ifndef MF_SIN_LISTADO
        if (sal != NULL) {
            z_linea(T("; variables, temporales y ocultas (memoria sin inicializar)"));
            for (id = 1; id <= n; id++) {
                sim_leer(id, &s);
                nombre_v(b, &s, id);
                equ(b, var_get(id));
            }
            for (id = 0; id < bk_tmax; id++) {
                sprintf(b, "T%u", (unsigned)id);
                equ(b, tdir_get(id));
            }
            for (id = 0; id < bk_hmax; id++) {
                sprintf(b, "H%u", (unsigned)id);
                equ(b, (uint16_t)(bk_hbase + 2 * id));
            }
        }
#endif
        if (z_desbordado()) {
            fallo(BACK_NO_CABE);
        }
    }
    r->err = bk_err;
    r->err_inst = bk_err_inst;
    r->org = org;
    r->entry = org;
    r->fin_img = dvars;
    r->fin_mem = fin_mem;
}
#endif
