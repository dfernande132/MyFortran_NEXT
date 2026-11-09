/* sem_sf.h - Puertas entre sem.c y sem_sf.c (funciones de sentencia; ver sem_sf.c). */
#ifndef MF_SEM_SF_H
#define MF_SEM_SF_H

#include <stdint.h>

/* sem.c -> sem_sf.c */
#define SF_OP_ES_DEF   0        /* ¿stm.a = stm.b es la definicion de una funcion de sentencia?                    */
#define SF_OP_DEFINIR  1        /* la define                                                                       */
#define SF_OP_REF      2        /* n = NODO_REF F(args), destino: tipo, TD_NINGUNO si error o 0xFF si no es una     */
#define SF_OP_FICT     3        /* n = NODO_NOMBRE: (indice << 4) | tipo si es un ficticio de la definicion, o 0xFF  */
uint8_t sf_entrada(uint8_t op, uint16_t n, uint8_t destino);

/* sem_sf.c -> sem.c */
#define SEMCB_ERROR      0      /* a = ErrorSem, b = posicion                                                      */
#define SEMCB_ERROR_UNI  1      /* a = ErrorUni, b = posicion                                                      */
#define SEMCB_TIPAR      2      /* a = nodo: su tipo                                                               */
uint16_t sem_cb(uint8_t op, uint16_t a, uint16_t b);

#endif
