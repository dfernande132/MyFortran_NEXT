/* nex.h - Fichero .nex (NEX V1.2) del programa generado por el backend Z80.
 *
 * Portable y SIN ficheros: construye los bytes y los entrega, en trozos de NEX_TROZO bytes, a una
 * funcion de salida que pone quien llama (en el Next, el editor con esxDOS; en PC, fwrite).
 *
 * Contenido (16896 bytes): cabecera de 512 B (un banco de 16 KB, el 2 = $8000..$BFFF, SP=$BFFE, PC=$BE00)
 * y el banco: el programa en su `org`, y en $BE00 un arranque de 32 B que desactiva interrupciones, borra
 * la pantalla, pone los atributos a 7 y hace CALL al programa (entry); al volver se queda en JR $.
 * Requiere r->org >= $8000 y r->fin_mem <= $BE00 (el programa y sus variables no pisan el arranque).
 * La imagen se lee de las paginas de datos (z_imagen_leer), es decir, la generada con back_generar(NULL,..).
 * Para que el programa dibuje por si mismo (sin la ROM) llamar antes a back_salida_pantalla(1).
 */
#ifndef MF_NEX_H
#define MF_NEX_H

#include <stdint.h>
#include "back.h"
#include "mf_api.h"

#define NEX_TROZO     64              /* bytes por llamada a la salida (viven en la pila)          */
#define NEX_TAM       16896u          /* 512 + 16384                                               */
#define NEX_ARRANQUE  0xBE00u

#define NEX_OK           0
#define NEX_NO_CABE     (-1)          /* el programa no deja sitio al arranque; no se llamo a sal */
#define NEX_ESCRITURA   (-2)          /* sal devolvio 0                                           */
#define NEX_RESULTADO   (-3)          /* BackRes con error o sin imagen                           */

/* Salida: escribe n bytes; devuelve 1 si fue bien y 0 si fallo. */
typedef uint8_t (*NexSalida)(const void *datos, uint16_t n);

/* NEX_OK si el programa cabe en un .nex (no escribe nada). */
int16_t nex_comprobar(const BackRes *r);

/* Comprueba y emite los NEX_TAM bytes por `sal`. Tras un fallo de sal no se llama mas. */
int16_t nex_escribir(NexSalida sal, const BackRes *r);

/* Genera el Z80 del TAC actual y escribe el .nex con la salida del editor (mf_api.h, MfSalida). Devuelve 0 si
 * todo fue bien; si no, el codigo para mf_mensaje (MF_COD_GEN + k). Lo llama mf_compilar. */
uint8_t nex_generar(const MfSalida *s);

#endif
