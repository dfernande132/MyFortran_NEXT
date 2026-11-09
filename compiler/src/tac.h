/* tac.h - Codigo de tres direcciones (TAC): formato binario y almacen.
 *
 * Implementa docs/especificacion-tac.md (v0.1): registros de 8 bytes
 *     op, tipo, a, b, c        (a, b, c = operandos de 16 bits: clase 3 bits + valor 13 bits)
 * guardados en paginas de datos (banco.h) -NUNCA en memoria residente- mas un
 * pool de constantes (enteros grandes, cadenas, logicos) en otra pagina.
 *
 * Reparto de paginas logicas del compilador (decision 0011):
 *   0 simbolos, 1 dimensiones, 2 etiquetas (+ pila de bloques del generador),
 *   3..4 instrucciones, 5 pool de constantes. 6..9: editor.
 */
#ifndef MF_TAC_H
#define MF_TAC_H

#include <stdint.h>

#define TAC_PAGINA        3                          /* primera pagina de instrucciones */
#define TAC_PAGINAS       2
#define TAC_POR_PAGINA    1024                       /* 8192 / 8                        */
#define TAC_MAX           (TAC_PAGINAS * TAC_POR_PAGINA)
#define TAC_PAGINA_POOL   5

/* Clases de operando (bits 15-13). */
#define TAC_NADA    0
#define TAC_TEMP    1
#define TAC_VAR     2
#define TAC_INM     3
#define TAC_CTE     4
#define TAC_ETQ     5
#define TAC_OCULTA  6

#define TAC_CLASE(x)   ((uint8_t)((x) >> 13))
#define TAC_VALOR(x)   ((uint16_t)((x) & 0x1FFF))
#define TAC_OPER(c, v) ((uint16_t)(((uint16_t)(c) << 13) | ((v) & 0x1FFF)))

#define TAC_ETQ_INTERNA  4096      /* ETQ >= 4096: etiqueta interna del generador */

/* Operaciones (numeracion provisional, spec seccion 5). */
enum {
    TOP_NOP = 0, TOP_LBL, TOP_MOV, TOP_NEG, TOP_ADD, TOP_SUB, TOP_MUL, TOP_DIV, TOP_POW,
    TOP_EQ, TOP_NE, TOP_LT, TOP_LE, TOP_GT, TOP_GE,
    TOP_AND, TOP_OR, TOP_EQV, TOP_NEQV, TOP_NOT,
    TOP_JMP, TOP_JT, TOP_JF, TOP_JEQ,
    TOP_LDE, TOP_STE, TOP_SCAT, TOP_SSUB, TOP_SLEN,
    TOP_PUT, TOP_PUTNL, TOP_STOP, TOP_HALT, TOP_TMP, TOP_EXT,
    TOP_ITOF, TOP_FTOI,            /* conversion entero -> real y real -> entero (trunca)       */
    TOP_RFUN, TOP_RMOD,            /* REAL: AINT/ANINT/SQRT d,x,k (k INM 0/1/2) y AMOD d,x,y     */
    TOP_ENTER,                     /* ENTER u: abre la unidad u (INM = numero de unidad, unid.h) */
    TOP_PARAM,                     /* PARAM.t x, k, u: copia x al formal k de u (k < 256) o, con k >= 256, del formal k-256 de u a x */
    TOP_CALL,                      /* CALL u                                                     */
    TOP_RET,                       /* RET: vuelve al que llamo (RETURN y END de subprograma)     */
    TOP_FUNR,                      /* FUNR.t d, u: d = resultado de la funcion u (etapa S2)      */
    TOP_SSET,                      /* SSET.C base, i, e + EXT j: base(i:j) := e (rellena con blancos / trunca) (etapa S3) */
    TOP_DATA,                      /* DATA.t v, k, n + EXT off: inicializa n elementos de v, desde el elemento off, con el valor k (INM/CTE) - 0.1 */
    TOP_RDLN,                      /* RDLN: empieza una sentencia READ (descarta lo que quede del registro de entrada)       */
    TOP_RD,                        /* RD.t d: lee el siguiente valor (entrada dirigida por lista) en d (variable o temporal)  */
    TOP_TOTAL
};

typedef struct {
    uint8_t  op;
    uint8_t  tipo;       /* TipoDato (sim.h); 0 = sin tipo */
    uint16_t a, b, c;
} TacInst;               /* 8 bytes */

/* Constantes del pool. */
#define TAC_C_ENTERO   1
#define TAC_C_CADENA   2
#define TAC_C_LOGICO   3
#define TAC_C_REAL     4       /* 4 bytes: el float IEEE 754 simple, little-endian */

/* Vacia el almacen (inicio de cada compilacion). */
void     tac_iniciar(void);

/* Estado del almacen: variables GLOBALES (residentes en el Next) para que lo lean tanto el generador (banco 44)
 * como el backend (banco 43) sin trampolines; solo tac.c las escribe. */
extern uint16_t tac_n;           /* instrucciones emitidas                      */
extern uint16_t tac_pool_n;      /* bytes ocupados del pool (siempre par)       */
extern uint8_t  tac_lle;         /* 1 si se agoto el espacio                    */

/* Numero de instrucciones. */
#define tac_cuenta()     (tac_n)

/* 1 si se agoto el espacio (instrucciones o pool): lo anotado ya no es valido. */
#define tac_lleno()      (tac_lle)

/* Anade una instruccion. Si no cabe, marca "lleno" y no hace nada. */
void     tac_emitir(uint8_t op, uint8_t tipo, uint16_t a, uint16_t b, uint16_t c);

void     tac_leer(uint16_t i, TacInst *t);                 /* tac_rd.c */

/* Operandos constantes. Un entero entre -4096 y 4095 va como INM; el resto, al pool. */
uint16_t tac_entero(int16_t v);
uint16_t tac_logico(uint8_t v);
uint16_t tac_cadena(const char *s, uint8_t len);
uint16_t tac_real(uint32_t bits);                 /* constante REAL: los 32 bits del float */

/* Bytes ocupados del pool (para copiarlo al programa). */
#define tac_pool_usado() (tac_pool_n)

/* Lectura de un operando INM / CTE. tac_inm_valor es macro: sin llamadas, sirve en cualquier banco. */
#define tac_inm_valor(op) ((int16_t)((TAC_VALOR(op) & 0x1000) ? (int16_t)(TAC_VALOR(op) - 0x2000) : (int16_t)TAC_VALOR(op)))
/* El resto (tac_rd.c) solo lee las paginas de datos: el backend lo lleva en SU banco. */
uint8_t  tac_cte_tipo(uint16_t op);                /* TAC_C_xxx                      */
int16_t  tac_cte_entero(uint16_t op);              /* TAC_C_ENTERO o TAC_C_LOGICO    */
uint8_t  tac_cte_cadena(uint16_t op, char *dst);   /* copia + '\0' (dst: 256 B); devuelve la longitud */
uint32_t tac_cte_real(uint16_t op);                /* TAC_C_REAL: los 32 bits del float */

#endif
