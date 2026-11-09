/* sem_decl.h - Puerta entre sem.c y sem_decl.c (declaraciones; ver sem_decl.c). */
#ifndef MF_SEM_DECL_H
#define MF_SEM_DECL_H

#include <stdint.h>

#define DECL_OP_TIPO   0        /* declarar el elemento e de una sentencia de tipo (tipo = TD_*)                    */
#define DECL_OP_DIM    1        /* DIMENSION: elemento e = nombre(limites)                                          */
#define DECL_OP_EXT    2        /* EXTERNAL: elemento e = nombre                                                    */
#define DECL_OP_INT    3        /* INTRINSIC: elemento e = nombre                                                   */
#define DECL_OP_PARAM     4     /* PARAMETER: comprobaciones previas de nombre = valor (devuelve 1 si se puede asignar)       */
#define DECL_OP_PARAM_FIN 5     /* PARAMETER: marca el nombre como constante (tras asignarlo sin errores)                    */
#define DECL_OP_DATA_DEST 6     /* DATA: un nombre del grupo en curso (e = NOMBRE o REF)                                     */
#define DECL_OP_DATA_VAL  7     /* DATA: un valor (e = constante; stm.c = factor de repeticion o 0): emite el TOP_DATA        */
#define DECL_OP_DATA_FIN  8     /* DATA: fin del grupo "/.../": comprueba que no sobran nombres                              */
uint8_t decl_entrada(uint8_t op, uint8_t tipo, uint16_t e);

#endif
