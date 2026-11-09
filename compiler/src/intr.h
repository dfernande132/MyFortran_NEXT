/* intr.h - Funciones INTRINSECAS de Fortran 77 (decision 0010).
 *
 * Tabla fija de las genericas y especificas que usan los tipos del compilador
 * (INTEGER, REAL, DOUBLE PRECISION, LOGICAL, CHARACTER; no hay COMPLEX).
 * El semantico la consulta cuando F(...) nombra algo que NO es una variable ni
 * un array: si esta en la tabla comprueba numero y tipo de los argumentos y
 * deduce el tipo del resultado. Es solo datos (RODATA del banco del semantico:
 * los punteros que devuelve no deben salir de ese banco).
 */
#ifndef MF_INTR_H
#define MF_INTR_H

#include <stdint.h>
#include "lexer.h"

/* Tipo de los argumentos (campo atipo). Un valor TD_xxx (sim.h) = TODOS los
 * argumentos de ese tipo. */
#define INTR_A_NUM   0x10   /* numericos, todos del mismo tipo                 */
#define INTR_A_FLT   0x11   /* REAL o DOUBLE, todos del mismo tipo             */
#define INTR_A_NUMX  0x12   /* numericos, cualquier mezcla (conversiones)      */

/* Tipo del resultado (campo rtipo): TD_xxx, o INTR_R_ARG = el del 1.er argumento. */
#define INTR_R_ARG   0

typedef struct {
    char    nombre[NOMBRE_MAX];   /* en mayusculas, con '\0' de relleno         */
    uint8_t nmin, nmax;           /* numero de argumentos                       */
    uint8_t atipo;                /* INTR_A_xxx o TD_xxx                        */
    uint8_t rtipo;                /* INTR_R_ARG o TD_xxx                        */
} Intrinseca;

/* Recorrido de la tabla (pruebas): numero de entradas y la entrada i. */
uint8_t intr_total(void);
const Intrinseca *intr_numero(uint8_t i);

/* NULL si nombre no es una intrinseca. */
const Intrinseca *intr_buscar(const char *nombre);

#endif
