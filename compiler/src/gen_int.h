/* gen_int.h - Parte INTERNA del generador, compartida por gen.c y gen_ll.c (llamadas). No es API: la API es gen.h.
 * El generador se parte en dos ficheros para que en el Next quepa en dos bancos (gen.c en el 44, gen_ll.c en el 42): el estado `g`
 * es UNICO y global (residente), y las funciones pequenas de apoyo se copian en cada fichero (static). */
#ifndef MF_GEN_INT_H
#define MF_GEN_INT_H

#include <string.h>
#include "gen.h"
#include "tac.h"
#include "sem.h"
#include "sim.h"
#include "stmt.h"
#include "lexer.h"
#include "mf_api.h"
#include "banco.h"
#include "real.h"
#include "unid.h"

/* Argumento de un CALL (en la pagina de datos, GEN_ARG_OFF): lo que se pasa y como se devuelve. */
#define GA_ENTRADA 0            /* expresion: solo copia de entrada                                 */
#define GA_VAR     1            /* variable: copia de salida a op                                   */
#define GA_ELEM    2            /* elemento de array: op es un temporal; copia de salida y STE arr, off, op */
#define GA_ARRAY   3            /* array entero: se pasa su direccion (PARAM con 512); sin copia de salida */
#define GA_SUB     4            /* subcadena S(lo:hi) de una variable: op es un temporal; copia de salida y SSET arr, lo, op + EXT hi */
#define GA_SUBE    5            /* subcadena A(off)(lo:hi) de un elemento: ademas se recarga el elemento y se devuelve con STE */
/* tm = tipo | modo << 4. Sin relleno (11 B): gcc lo necesita pedido; sccz80 no alinea. */
#ifdef __GNUC__
#define GEN_EMPAQUETADO __attribute__((packed))
#else
#define GEN_EMPAQUETADO
#endif
typedef struct {
    uint16_t op;
    uint16_t arr, off;
    uint16_t lo, hi;        /* GA_SUB / GA_SUBE: limites de la subcadena (operandos) */
    uint8_t  tm;
} GEN_EMPAQUETADO GArg;
#define GA_TIPO(a)  ((uint8_t)((a)->tm & 15))
#define GA_MODO(a)  ((uint8_t)((a)->tm >> 4))
#define GA_PON(a, t, m)  ((a)->tm = (uint8_t)((t) | ((m) << 4)))

typedef struct {
    uint8_t  activo;        /* gen_activar                                          */
    uint8_t  ntemp;         /* temporales de la sentencia                           */
    uint16_t tbase;         /* primer temporal de la unidad (los de las unidades anteriores van antes) */
    uint16_t tmax;          /* temporales que ha necesitado la unidad                */
    uint16_t hbase;         /* primera palabra oculta de la unidad                   */
    uint16_t hmax;          /* palabras ocultas que ha necesitado la unidad (indice maximo + 1 - hbase) */
    uint8_t  clase;         /* unidad en curso: UK_PROGRAMA / UK_SUBRUTINA           */
    uint8_t  llamado;       /* CALL / referencia a funcion en curso: unidad llamada  */
    uint8_t  base;          /* primera ranura del buffer de argumentos de esa llamada */
    uint8_t  top;           /* primera ranura libre del buffer (las llamadas anidadas siguen a partir de aqui) */
    uint8_t  nblq;          /* bloques abiertos                                     */
    uint16_t ndo;           /* siguiente palabra oculta libre (absoluta: indice del programa) */
    uint8_t  ngoto;         /* etiquetas acumuladas del GOTO calculado              */
    uint16_t cap;           /* capacidad del ultimo resultado CHARACTER             */
    uint16_t lbl_if;        /* IF logico: etiqueta a poner tras la sentencia interior */
    uint16_t cierre;        /* indice de la etiqueta de la sentencia en curso (cierra DO) */
    uint16_t nombre;        /* nombre del PROGRAM                                   */
    uint16_t slo, shi;      /* limites (operandos) de la ultima subcadena generada: los necesita la copia de salida de un argumento S(i:j) */
    uint16_t sfarg[8];      /* operandos de los argumentos de la funcion de sentencia que se expande (NODO_ARGF) */
} Gen;

extern Gen gen_g;
#define g gen_g

#define GEN_ARG_MAX  93          /* ranuras del buffer de argumentos (todas las llamadas anidadas de una sentencia) */
#ifndef MF_TARGET_NEXT           /* sccz80 no pliega sizeof en una constante */
typedef char gen_comprobar_args3[(GEN_ARG_OFF + GEN_ARG_MAX * sizeof(GArg) <= GEN_SF_OFF) ? 1 : -1];
#endif

/* De gen.c (banco 44) a gen_ll.c (banco 42) y de vuelta. */
uint16_t gen_expr(uint16_t n, uint16_t dst);
uint16_t gen_desplazamiento(uint16_t n, const Simbolo *s, const Dim *dm);
uint16_t gen_subcadena(uint16_t base, uint16_t cap_base, uint16_t rango, uint8_t asum);
uint16_t gen_funcion(uint16_t n, uint8_t u);          /* F(args) con F del usuario */
void     gen_llamada_emitir(void);                   /* PARAM de entrada, CALL y PARAM de salida de la llamada en curso */

/* 1 si se puede seguir generando. Un error anterior (o falta de espacio) lo apaga. */
static uint8_t seguir(void)
{
    if (!g.activo || mf.n != 0) {
        return 0;
    }
    if (tac_lleno()) {
        mf_diag_pos(SEM_GEN_TAC_LLENO, 0);
        return 0;
    }
    return 1;
}

static void no_soportado(uint16_t pos)
{
    mf_diag_pos(SEM_GEN_NO_SOPORTADO, pos);
}

static uint16_t nuevo_temp(void)
{
    uint16_t v = (uint16_t)(g.tbase + g.ntemp++);

    if (g.ntemp > g.tmax) {
        g.tmax = g.ntemp;
    }
    return TAC_OPER(TAC_TEMP, v);
}

static void emitir(uint8_t op, uint8_t tipo, uint16_t a, uint16_t b, uint16_t c)
{
    tac_emitir(op, tipo, a, b, c);
}

#endif
