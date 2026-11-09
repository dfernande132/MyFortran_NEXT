/* z80.h - Capa de emision de codigo Z80 (decision 0012).
 *
 * Una sola capa con DOS salidas, para que el listado .asm nunca discrepe de los
 * bytes: cada instruccion se emite UNA vez con sus bytes y su texto.
 *   - bytes: se escriben en una imagen en memoria que empieza en `org`;
 *   - texto: si hay una funcion de salida, cada instruccion llega como una linea
 *     de ensamblador (se puede ensamblar con z80asm para contrastar los bytes).
 *
 * El backend recorre el TAC DOS veces ("pasadas"). En la 1 solo se mide (no se
 * escribe nada) y se aprenden las direcciones de etiquetas y de datos; en la 2 se
 * emite de verdad con todas las direcciones ya conocidas. Asi no hacen falta
 * listas de saltos pendientes de resolver.
 */
#ifndef MF_Z80_H
#define MF_Z80_H

#include <stdint.h>

/* Con img == NULL la imagen no va a un buffer sino a las paginas de datos Z_PAG_IMG.. (banco.h), a
 * traves de un buffer de 64 B; z_fin() vuelca lo que quede. Se lee con z_imagen_leer(). */
#define Z_PAG_IMG        11
#define Z_IMG_MAX        16384u

/* pasada: 1 = medir, 2 = emitir. sal puede ser NULL (sin listado). */
void     z_iniciar(uint8_t *img, uint16_t cap, uint16_t org, uint8_t pasada,
                   void (*sal)(const char *linea));
uint16_t z_pc(void);
uint8_t  z_desbordado(void);
void     z_fin(void);                 /* fin de la pasada 2: vuelca el buffer en modo paginas */
void     z_imagen_leer(uint16_t desp, uint8_t *dst, uint16_t n);   /* copia n bytes de la imagen en paginas */          /* 1 si la imagen no cabia */

/* Una instruccion de n (1..4) bytes. txt es el formato del listado con a lo sumo un
 * "%s" que se sustituye por arg. */
void     z_ins(const char *txt, const char *arg, uint8_t n,
               uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3);

/* Bytes de datos (DEFB) y lineas de listado sin bytes (etiquetas, EQU, comentarios). */
void     z_datos(const uint8_t *b, uint16_t n);
void     z_linea(const char *txt);

#endif
