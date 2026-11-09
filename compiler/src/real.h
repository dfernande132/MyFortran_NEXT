/* real.h - Constantes REAL: del texto decimal al formato IEEE 754 simple (decision 0013).
 *
 * El lexer guarda la constante real como TEXTO (sin blancos): "1.5", ".5", "1.", "2E-3", "3.14E+2". Aqui se convierte a los 32 bits
 * de un float (bit 31 signo, 30..23 exponente +127, 22..0 mantisa sin el 1 implicito), con redondeo al par mas cercano y EXACTO:
 * la misma respuesta que strtof(), sin usar aritmetica de coma flotante (el Z80 no la tiene). Se leen 30 cifras significativas
 * exactas; las que sobran solo cuentan como "hay algo mas" (solo falla si el texto cae a menos de 1E-30 de un empate de redondeo).
 *
 * Convenio de la implementacion (el runtime del programa generado lo comparte): los resultados por debajo del menor normal
 * (1.17549435E-38) se sustituyen por 0 (no hay numeros denormales) y no hay infinitos ni NaN.
 */
#ifndef MF_REAL_H
#define MF_REAL_H

#include <stdint.h>

#define REAL_OK         0
#define REAL_DESBORDA   1       /* el valor no cabe: mayor que 3.40282347E+38 */

/* texto: constante sin signo (el signo es el operador unario). Devuelve REAL_OK o REAL_DESBORDA; *bits = el float. */
uint8_t real_de_texto(const char *texto, uint32_t *bits);

/* Un entero de 16 bits como float (exacto: caben en los 24 bits de la mantisa). */
uint32_t real_de_entero(int16_t v);

#endif
