/* ed_plat.h - Lo que el editor necesita de la maquina (pantalla, teclado, ficheros).
 *
 * El editor (edit.c) no sabe nada del Next: solo llama a estas funciones.
 *   - Next: editor/plataforma/next/ed_next.c  (tilemap 80x32, color por caracter)
 *   - Pruebas en PC: editor/tests/test_ed01.c define una pantalla falsa
 *
 * Pantalla: ED_ANCHO x ED_FILAS caracteres, origen (0,0) arriba a la izquierda.
 *   fila 0                 barra de estado
 *   filas 1 .. 29          texto (ED_FILAS_TEXTO filas)
 *   fila 30 (ED_FILA_AYUDA) barra de ayuda
 *   fila 31 (ED_FILA_MSG)   mensajes
 *
 * Cada caracter lleva un ESTILO (ED_E_*): la plataforma lo traduce a colores.
 * Los estilos van de 0 a ED_N_ESTILOS-1.
 */
#ifndef MF_ED_PLAT_H
#define MF_ED_PLAT_H

#include <stdint.h>

#define ED_ANCHO         80
#define ED_FILAS         32
#define ED_FILAS_TEXTO   29
#define ED_FILA_AYUDA    (ED_FILAS - 2)
#define ED_FILA_MSG      (ED_FILAS - 1)

/* Estilos */
#define ED_E_TEXTO    0     /* texto normal */
#define ED_E_COMENT   1     /* linea de comentario (C o * en la columna 1) */
#define ED_E_ETIQ     2     /* etiqueta numerica, columnas 1-5 */
#define ED_E_CONT     3     /* indicador de continuacion, columna 6 */
#define ED_E_CLAVE    4     /* palabra clave */
#define ED_E_CADENA   5     /* constante de cadena entre apostrofes */
#define ED_E_NUM      6     /* numero */
#define ED_E_FUERA    7     /* columnas 73 en adelante: F77 las ignora */
#define ED_E_ESTADO   8     /* barra de estado */
#define ED_E_AYUDA    9     /* barra de ayuda */
#define ED_E_CURSOR   10    /* cursor de bloque */
#define ED_E_ERROR    11    /* mensaje de error */
#define ED_N_ESTILOS  12

/* Prepara el modo de pantalla (y la paleta) y la borra. */
void plat_iniciar(void);

/* Escribe n caracteres ASCII imprimibles (32..126) desde la columna col de la fila;
 * est[i] es el estilo del caracter i. */
void plat_escribir(uint8_t fila, uint8_t col, const uint8_t *txt, const uint8_t *est, uint8_t n);

/* Coloca el cursor visible en (fila, col): la celda pasa a estilo ED_E_CURSOR hasta
 * que se mueva el cursor o se repinte esa celda. */
void plat_cursor(uint8_t fila, uint8_t col);

/* Espera una tecla y devuelve su codigo tal como llega (sin interpretar). */
int plat_tecla(void);

/* Ficheros (esxdos en el Next).
 *   plat_fich_abrir_lectura: manejador >= 0, o un valor NEGATIVO = -(codigo de error, 1..127)
 *   plat_fich_leer:          bytes leidos (0 = fin de fichero), o negativo si falla
 *   plat_fich_cerrar:        cierra el manejador */
int  plat_fich_abrir_lectura(const char *nombre);
int  plat_fich_leer(int h, uint8_t *dst, uint16_t n);
void plat_fich_cerrar(int h);

/*   plat_fich_abrir_escritura: crea el fichero o lo vacia si existe. Manejador >= 0, o
 *                              NEGATIVO = -(codigo de error, 1..127)
 *   plat_fich_escribir:        devuelve los bytes escritos; si es distinto de n, ha fallado */
int  plat_fich_abrir_escritura(const char *nombre);
int  plat_fich_escribir(int h, const uint8_t *src, uint16_t n);

#endif
