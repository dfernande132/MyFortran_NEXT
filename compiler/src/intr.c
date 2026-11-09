/* intr.c - Tabla de funciones intrinsecas (ver intr.h). */
#include <string.h>
#include "intr.h"
#include "sim.h"

#define I TD_INTEGER
#define R TD_REAL
#define D TD_DOUBLE
#define C TD_CHARACTER
#define L TD_LOGICAL
#define NUM  INTR_A_NUM
#define FLT  INTR_A_FLT
#define NUMX INTR_A_NUMX
#define ARG  INTR_R_ARG
#define MUCHOS 20       /* MAX/MIN: hasta 20 argumentos */

static const Intrinseca tabla[] = {
    /* ---- genericas numericas ---- */
    { "ABS",    1, 1, NUM, ARG },
    { "MOD",    2, 2, NUM, ARG },
    { "SIGN",   2, 2, NUM, ARG },
    { "DIM",    2, 2, NUM, ARG },
    { "MAX",    2, MUCHOS, NUM, ARG },
    { "MIN",    2, MUCHOS, NUM, ARG },
    { "AINT",   1, 1, FLT, ARG },
    { "ANINT",  1, 1, FLT, ARG },
    { "SQRT",   1, 1, FLT, ARG },
    { "EXP",    1, 1, FLT, ARG },
    { "LOG",    1, 1, FLT, ARG },
    { "LOG10",  1, 1, FLT, ARG },
    { "SIN",    1, 1, FLT, ARG },
    { "COS",    1, 1, FLT, ARG },
    { "TAN",    1, 1, FLT, ARG },
    { "ASIN",   1, 1, FLT, ARG },
    { "ACOS",   1, 1, FLT, ARG },
    { "ATAN",   1, 1, FLT, ARG },
    { "ATAN2",  2, 2, FLT, ARG },
    { "SINH",   1, 1, FLT, ARG },
    { "COSH",   1, 1, FLT, ARG },
    { "TANH",   1, 1, FLT, ARG },
    /* ---- conversiones ---- */
    { "INT",    1, 1, NUMX, I },
    { "REAL",   1, 1, NUMX, R },
    { "DBLE",   1, 1, NUMX, D },
    { "NINT",   1, 1, FLT, I },
    { "FLOAT",  1, 1, I, R },
    { "IFIX",   1, 1, R, I },
    { "SNGL",   1, 1, D, R },
    { "IDINT",  1, 1, D, I },
    { "IDNINT", 1, 1, D, I },
    /* ---- especificas INTEGER ---- */
    { "IABS",   1, 1, I, I },
    { "ISIGN",  2, 2, I, I },
    { "IDIM",   2, 2, I, I },
    { "MAX0",   2, MUCHOS, I, I },
    { "MIN0",   2, MUCHOS, I, I },
    { "AMAX0",  2, MUCHOS, I, R },
    { "AMIN0",  2, MUCHOS, I, R },
    /* ---- especificas REAL ---- */
    { "AMAX1",  2, MUCHOS, R, R },
    { "AMIN1",  2, MUCHOS, R, R },
    { "MAX1",   2, MUCHOS, R, I },
    { "MIN1",   2, MUCHOS, R, I },
    { "ALOG",   1, 1, R, R },
    { "ALOG10", 1, 1, R, R },
    { "AMOD",   2, 2, R, R },
    /* ---- especificas DOUBLE PRECISION ---- */
    { "DABS",   1, 1, D, D },
    { "DSQRT",  1, 1, D, D },
    { "DEXP",   1, 1, D, D },
    { "DLOG",   1, 1, D, D },
    { "DLOG10", 1, 1, D, D },
    { "DSIN",   1, 1, D, D },
    { "DCOS",   1, 1, D, D },
    { "DTAN",   1, 1, D, D },
    { "DASIN",  1, 1, D, D },
    { "DACOS",  1, 1, D, D },
    { "DATAN",  1, 1, D, D },
    { "DATAN2", 2, 2, D, D },
    { "DSINH",  1, 1, D, D },
    { "DCOSH",  1, 1, D, D },
    { "DTANH",  1, 1, D, D },
    { "DMOD",   2, 2, D, D },
    { "DSIGN",  2, 2, D, D },
    { "DDIM",   2, 2, D, D },
    { "DINT",   1, 1, D, D },
    { "DNINT",  1, 1, D, D },
    { "DMAX1",  2, MUCHOS, D, D },
    { "DMIN1",  2, MUCHOS, D, D },
    { "DPROD",  2, 2, R, D },
    /* ---- CHARACTER ---- */
    { "LEN",    1, 1, C, I },
    { "ICHAR",  1, 1, C, I },
    { "INDEX",  2, 2, C, I },
    { "CHAR",   1, 1, I, C },
    { "LGE",    2, 2, C, L },
    { "LGT",    2, 2, C, L },
    { "LLE",    2, 2, C, L },
    { "LLT",    2, 2, C, L }
};

uint8_t intr_total(void)
{
    return (uint8_t)(sizeof(tabla) / sizeof(tabla[0]));
}

const Intrinseca *intr_numero(uint8_t i)
{
    return &tabla[i];
}

const Intrinseca *intr_buscar(const char *nombre)
{
    uint8_t i;

    for (i = 0; i < sizeof(tabla) / sizeof(tabla[0]); i++) {
        if (strncmp(nombre, tabla[i].nombre, NOMBRE_MAX) == 0) {
            return &tabla[i];
        }
    }
    return 0;
}
