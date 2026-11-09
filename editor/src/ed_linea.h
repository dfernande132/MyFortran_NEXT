/* ed_linea.h - Servicio de lineas del editor para el compilador (mf_api.h).
 * Mismo prototipo que compiler/include/mf_api.h (si se incluyen los dos, coinciden). */
#ifndef MF_ED_LINEA_H
#define MF_ED_LINEA_H

#include <stdint.h>

/* RESIDENTE. Copia la linea fisica n (la 1.a es la 1) en dst (73 bytes): sin '\n',
 * como mucho 72 caracteres + '\0'. 1 = existe; 0 = n pasa del final (o n == 0). */
int ed_linea(uint16_t n, char *dst);

#endif
