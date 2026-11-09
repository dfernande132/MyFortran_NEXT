/* banco.h - Memoria de datos GRANDE, fuera de los 32 KB residentes.
 *
 * Es la unica puerta de entrada a los datos que no caben en memoria normal
 * (tabla de simbolos del programa, codigo intermedio, codigo objeto, texto
 * fuente del editor...). El nucleo del compilador solo conoce esta interfaz:
 *   - en el Next (platform/next/banco_next.c): paginas de 8 KB mapeadas en la
 *     ventana MMU2 ($4000-$5FFF) con DI/EI y restauracion de la pantalla;
 *   - en PC (platform/pc/banco_pc.c): un array. Asi los tests de PC ejercitan
 *     la misma logica sin hardware.
 *
 * Las paginas son LOGICAS (0 .. BANCO_PAGINAS-1); cual es la pagina fisica lo
 * decide cada plataforma. Cada llamada accede a UNA sola pagina: off + n no
 * puede pasar de BANCO_PAGINA_TAM (el que llama parte los bloques grandes).
 * Las lecturas y escrituras COPIAN: no se guarda nunca un puntero a la
 * ventana, porque tras la llamada ya no apunta a nada valido.
 *
 * Regla del Next (docs/referencia, 4.5): antes de llamar a esxDOS la
 * paginacion debe estar en su estado normal. Como cada funcion restaura la
 * ventana antes de volver, se cumple "por construccion".
 */
#ifndef MF_BANCO_H
#define MF_BANCO_H

#include <stdint.h>

#define BANCO_PAGINA_TAM  8192
#define BANCO_PAGINAS     14       /* paginas logicas (0011, 0012, 0014): 0..5 compilador (frontal y TAC), 6..9 editor,
                                      10 tablas del backend Z80, 11..12 imagen del programa generado,
                                      13 directorio de unidades y registro de llamadas (unid.h) */

void banco_iniciar(void);
void banco_leer(uint8_t pag, uint16_t off, void *dst, uint16_t n);
void banco_escribir(uint8_t pag, uint16_t off, const void *src, uint16_t n);

#endif
