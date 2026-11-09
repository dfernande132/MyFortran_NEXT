/* edbuf.h - Buffer de texto del editor: buffer de hueco sobre paginas de datos.
 *
 * El texto vive en las paginas logicas ED_PAG_BASE .. ED_PAG_BASE+ED_PAGINAS-1
 * de platform/banco.h (en el Next, ventana MMU2; en PC, un array). Se trata
 * como UN solo vector lineal de ED_CAP bytes ("posicion fisica"), con un HUECO:
 *
 *      [ texto antes del hueco ][ ........hueco........ ][ texto despues ]
 *      0                      g_ini               g_ini+g_len          cap
 *
 * Las posiciones que ve el que llama son LOGICAS: 0 .. longitud, como si el
 * hueco no existiera. Escribir donde esta el cursor cuesta un solo acceso a
 * pagina; mover el cursor lejos cuesta mover el hueco (por bloques de
 * ED_CHUNK bytes, porque cada acceso a pagina tiene un coste fijo, 0007 punto 5).
 *
 * El texto es una secuencia de bytes; las lineas se separan con '\n' (0x0A).
 * No hay '\n' final obligatorio: "A\nB" son dos lineas; "A\n" son dos lineas
 * (la segunda vacia), igual que en cualquier editor.
 *
 * Estado: UNA variable global (decision 0006). Sin malloc. No reentrante:
 * usa un buffer interno compartido (tmp) para mover bloques y escanear.
 */
#ifndef MF_EDBUF_H
#define MF_EDBUF_H

#include <stdint.h>
#include "banco.h"

#if BANCO_PAGINA_TAM != 8192
#error "edbuf.c calcula pagina/desplazamiento con >>13 y &0x1FFF: BANCO_PAGINA_TAM debe ser 8192"
#endif

/* Reparto de paginas logicas de banco.h (decision 0011, peticion del compilador de
 * 2 paginas mas para el codigo intermedio): compilador 0..5, editor 6..9, con
 * BANCO_PAGINAS = 10 (paginas fisicas 50..59). Antes: compilador 0..3, editor 4..7.
 * El numero de paginas del editor ya NO se deduce de BANCO_PAGINAS: son 4 (32 KB)
 * y no crecen aunque banco.h tenga mas. */
#ifndef ED_PAG_BASE
#define ED_PAG_BASE   6
#endif
#ifndef ED_PAGINAS
#define ED_PAGINAS    4
#endif
#if ED_PAG_BASE + ED_PAGINAS > BANCO_PAGINAS
#error "El editor usa las paginas logicas ED_PAG_BASE .. ED_PAG_BASE+ED_PAGINAS-1 (hoy 6..9): platform/banco.h debe tener BANCO_PAGINAS >= 10"
#endif

#if ED_PAGINAS < 1 || ED_PAGINAS > 7
#error "ED_PAGINAS debe estar entre 1 y 7 (las posiciones son uint16_t: 7 x 8192 = 57344 < 65536)"
#endif

#define ED_CAP    ((uint16_t)(ED_PAGINAS * 8192u))   /* bytes de texto + hueco */
#define ED_CHUNK  64                                  /* bloque de movimiento y escaneo */

typedef struct {
    uint16_t cap;     /* tamano fisico usable (<= ED_CAP) */
    uint16_t g_ini;   /* posicion fisica donde empieza el hueco (= bytes de texto antes del hueco) */
    uint16_t g_len;   /* longitud del hueco (= bytes libres) */
    uint16_t ver;     /* cambia con CADA modificacion del texto (insertar, borrar, agregar, iniciar):
                         ed_linea invalida su cache al verlo distinto. Puede desbordar: da igual. */
} BufTexto;

extern BufTexto eb;

/* Vacia el buffer. NO llama a banco_iniciar (lo hace quien arranca el programa,
 * una sola vez, para no borrar paginas de otros usuarios de banco.h). */
void eb_iniciar(void);
/* Igual, con capacidad reducida (pruebas del buffer lleno). cap <= ED_CAP. */
void eb_iniciar_cap(uint16_t cap);

uint16_t eb_longitud(void);                  /* bytes de texto */
uint16_t eb_libre(void);                     /* bytes que aun caben */

/* Inserta c ANTES de la posicion pos (0 .. longitud). 1 = hecho; 0 = lleno o pos invalida. */
uint8_t  eb_insertar(uint16_t pos, uint8_t c);
/* Borra el byte en pos (0 .. longitud-1). 1 = hecho; 0 = pos invalida. */
uint8_t  eb_borrar(uint16_t pos);

/* Anade n bytes al FINAL del texto de una vez (carga de ficheros: un acceso por
 * pagina en vez de uno por byte). Devuelve cuantos entraron: menos que n si se llena. */
uint16_t eb_agregar(const uint8_t *src, uint16_t n);

/* Lectura. eb_leer exige pos + n <= longitud. */
uint8_t  eb_byte(uint16_t pos);
void     eb_leer(uint16_t pos, uint8_t *dst, uint16_t n);

/* Consultas de lineas (separador '\n'). pos en 0 .. longitud.
 *   eb_ini_linea: inicio de la linea que contiene pos (0 o el byte tras un '\n' anterior a pos).
 *   eb_fin_linea: posicion del '\n' que cierra la linea de pos, o longitud si es la ultima.
 *   eb_contar_nl: cuantos '\n' hay en [0, pos); = numero de linea (desde 0) de pos. */
uint16_t eb_ini_linea(uint16_t pos);
uint16_t eb_fin_linea(uint16_t pos);
uint16_t eb_contar_nl(uint16_t pos);

#endif
