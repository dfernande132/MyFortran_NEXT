/* back_rt_mac.h - Numeracion de las rutinas de apoyo y macros de las tablas de back_rt.h / back_rt_real.h.
 * Aparte para que las tablas REAL (back_rt_real.c) puedan vivir en otro banco que back.c. */
#ifndef MF_BACK_RT_MAC_H
#define MF_BACK_RT_MAC_H

#include <stdint.h>

enum { RT_PUTC, RT_NEGHL, RT_NEGDE, RT_MUL, RT_DIV, RT_POW, RT_LT, RT_GE, RT_GT, RT_LE,
       RT_EQ, RT_NE, RT_PUTI, RT_PUTL, RT_PUTS, RT_SMOV, RT_SADD, RT_SCPY, RT_CLAMP, RT_SCMP,
       /* REAL (decision 0013): bloque de trabajo, cadenas de bytes, operaciones */
       RT_FPD, RT_BSHL, RT_BSHR, RT_BADD, RT_BSUB, RT_FUNP, RT_FNORM, RT_FADD, RT_FMUL, RT_FDIV, RT_FCMP,
       RT_ITOF, RT_FTOI, RT_FNEG, RT_FPUT,
       /* R4: AINT/ANINT, SQRT, AMOD, REAL ** INTEGER */
       RT_FINT, RT_FSQRT, RT_FMOD, RT_FPOW,
       /* errores de ejecucion (division por cero, 0/0, overflow REAL): imprime el mensaje y para */
       RT_ERR,
       /* READ (entrada dirigida por lista): bloque de trabajo, teclado, linea, valor, conversion por tipo */
       RT_RDD, RT_KEYSC, RT_KEYH, RT_KEYT, RT_RDLINE, RT_RDLN, RT_RDTOK, RT_RDI, RT_RDL, RT_RDS, RT_RDF, RT_N };
#define RT_KEY RT_KEYH             /* la tecla: RT_KEYH (teclado) o, en las pruebas, RT_KEYT (texto en memoria) */

/* Tipo de elemento: byte bajo = n.o de operandos (0..3), el resto = clase. 1..3 = instruccion de ese
 * numero de bytes tal cual. */
#define RK_JR      0x0A         /* (1 << 3) | 2 operandos: opcode, id          */
#define RK_JPLOC   0x12         /* (2 << 3) | 2 operandos: opcode, id          */
#define RK_LOC     0x19         /* (3 << 3) | 1 operando:  id                  */
#define RK_CALL    0x21         /* (4 << 3) | 1 operando:  rutina              */
#define RK_JP      0x29         /* (5 << 3) | 1 operando:  rutina              */
#define RK_REF     0x31         /* (6 << 3) | 1 operando:  opcode              */
#define RK_CUR     0x38         /* (7 << 3) | 0 operandos                      */
#define RK_NEC     0x41         /* (8 << 3) | 1 operando:  rutina              */
#define RK_RTOP    0x4A         /* (9 << 3) | 2 operandos: opcode, rutina      */
#define RK_REFO    0x52         /* (10 << 3) | 2 operandos: opcode, desplazamiento en el bloque RT_FPD  */
#define RK_REFE    0x5B         /* (11 << 3) | 3 operandos: prefijo, opcode, desplazamiento             */
#define RK_DATA    0x61         /* (12 << 3) | 1 operando:  n bytes a cero                              */
#define RK_DB1     0x69         /* (13 << 3) | 1 operando:  1 byte de datos (2 y 3 bytes: RK_DB1 + 1, + 2) */
#define RK_DB2     0x6A
#define RK_DB3     0x6B
#define RK_REFD    0x7A         /* (15 << 3) | 2 operandos: opcode, desplazamiento en el bloque RT_RDD */
#define RK_REFDE   0x83         /* (16 << 3) | 3 operandos: prefijo, opcode, desplazamiento             */
#define RK_REFR    0x72         /* (14 << 3) | 2 operandos: opcode, desplazamiento dentro de la propia rutina  */

#ifdef MF_SIN_LISTADO
typedef uint8_t RtI;
#define I1(t, a)               1, a,
#define I2(t, a, b)            2, a, b,
#define I3(t, arg, a, lo, hi)  3, a, lo, hi,
#define IJR(t, op, id)         RK_JR, op, id,
#define IJPLOC(t, op, id)      RK_JPLOC, op, id,
#define ILOC(id)               RK_LOC, id,
#define ICALLRT(r)             RK_CALL, r,
#define IJPRT(r)               RK_JP, r,
#define IREF(t, arg, op)       RK_REF, op,
#define ICURDEF                RK_CUR,
#define INECESITA(r)           RK_NEC, r,
#define IRTOP(t, op, r)        RK_RTOP, op, r,
#define IREFO(t, arg, op, off) RK_REFO, op, off,
#define IREFE(t, arg, pf, op, off) RK_REFE, pf, op, off,
#define IDATA(n)               RK_DATA, n,
#define IDB1(a)                RK_DB1, a,
#define IDB2(a, b)             RK_DB2, a, b,
#define IDB3(a, b, c)          RK_DB3, a, b, c,
#define IREFR(t, arg, op, off) RK_REFR, op, off,
#define IREFD(t, arg, op, off) RK_REFD, op, off,
#define IREFDE(t, arg, pf, op, off) RK_REFDE, pf, op, off,
#define IFIN                   0
#else
typedef struct {
    uint8_t     k;
    uint8_t     b[3];
    const char *t;
    const char *a;
} RtI;
#define RTI(k, t, a, b0, b1, b2)  { k, { b0, b1, b2 }, t, a },
#define I1(t, a)               RTI(1, t, 0, a, 0, 0)
#define I2(t, a, b)            RTI(2, t, 0, a, b, 0)
#define I3(t, arg, a, lo, hi)  RTI(3, t, arg, a, lo, hi)
#define IJR(t, op, id)         RTI(RK_JR, t, 0, op, id, 0)
#define IJPLOC(t, op, id)      RTI(RK_JPLOC, t, 0, op, id, 0)
#define ILOC(id)               RTI(RK_LOC, 0, 0, id, 0, 0)
#define ICALLRT(r)             RTI(RK_CALL, 0, 0, r, 0, 0)
#define IJPRT(r)               RTI(RK_JP, 0, 0, r, 0, 0)
#define IREF(t, arg, op)       RTI(RK_REF, t, arg, op, 0, 0)
#define ICURDEF                RTI(RK_CUR, 0, 0, 0, 0, 0)
#define INECESITA(r)           RTI(RK_NEC, 0, 0, r, 0, 0)
#define IRTOP(t, op, r)        RTI(RK_RTOP, t, 0, op, r, 0)
#define IREFO(t, arg, op, off) RTI(RK_REFO, t, arg, op, off, 0)
#define IREFE(t, arg, pf, op, off) RTI(RK_REFE, t, arg, pf, op, off)
#define IDATA(n)               RTI(RK_DATA, 0, 0, n, 0, 0)
#define IDB1(a)                RTI(RK_DB1, 0, 0, a, 0, 0)
#define IDB2(a, b)             RTI(RK_DB2, 0, 0, a, b, 0)
#define IDB3(a, b, c)          RTI(RK_DB3, 0, 0, a, b, c)
#define IREFR(t, arg, op, off) RTI(RK_REFR, t, arg, op, off, 0)
#define IREFD(t, arg, op, off) RTI(RK_REFD, t, arg, op, off, 0)
#define IREFDE(t, arg, pf, op, off) RTI(RK_REFDE, t, arg, pf, op, off)
#define IFIN                   { 0, { 0, 0, 0 }, 0, 0 }
#endif

#endif
