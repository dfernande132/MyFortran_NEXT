/* source.h - Lector de fuente Fortran 77 en formato fijo (nivel 1 del lexer).
 *
 * Convierte TEXTO FUENTE en SENTENCIAS LOGICAS:
 *
 *   texto  --tomar_linea-->  linea fisica --clasificar_linea--> tipo
 *          --iniciar_sentencia / anadir_continuacion-->  sentencia
 *
 * La sentencia conserva el texto TAL CUAL (columnas 7-72, con blancos y
 * minusculas). Quien lo lee (escaner, clasificador) salta los blancos y pasa
 * a mayusculas al leer. Asi cada caracter conserva su posicion original, que
 * necesitan los mensajes de error y el resaltado de sintaxis del editor.
 *
 * Para que esa posicion se calcule sin tablas, cada linea ocupa EXACTAMENTE
 * SEGMENTO (66) caracteres en el texto: las lineas que van seguidas de una
 * continuacion se rellenan con blancos hasta la columna 72, como pide el
 * estandar. Asi el caracter en la posicion p viene de la linea p / 66, columna
 * p % 66 + 7 (ver posicion_fuente).
 *
 * leer_sentencia() encadena todo. Es lo unico que usara el nivel 2.
 */
#ifndef MF_SOURCE_H
#define MF_SOURCE_H

#include <stdint.h>

/* ------------------------------------------------------------------
 * Lineas fisicas
 * ------------------------------------------------------------------ */

typedef enum {
    LINEA_IGNORADA,       /* comentario o linea en blanco: no produce nada  */
    LINEA_INICIAL,        /* empieza una sentencia nueva                    */
    LINEA_CONTINUACION    /* sigue la sentencia de la linea anterior        */
} TipoLinea;

/* Recibe una linea SIN el salto de linea final y devuelve su tipo. */
TipoLinea clasificar_linea(const char *linea);

/* ------------------------------------------------------------------
 * Sentencias logicas
 * ------------------------------------------------------------------ */

#define SEGMENTO       66    /* columnas 7-72: lo que aporta cada linea        */
#define MAX_SEGMENTOS  20    /* lineas por sentencia (1 inicial + 19 continuac., X3.9-1978 Full) */

/* Tamano maximo del texto. Fijo, sin malloc (regla del nucleo). Son 20 lineas
 * completas (1320 bytes), el maximo del estandar Full; coste medido en cap01 §8. */
#define MAX_SENTENCIA  (MAX_SEGMENTOS * SEGMENTO)

typedef enum {
    FUENTE_OK,
    FUENTE_FIN,                     /* no quedan mas sentencias               */
    FUENTE_ETIQUETA_INVALIDA,       /* algo que no es digito ni blanco, cols 1-5 */
    FUENTE_CONTINUACION_INVALIDA,   /* sin sentencia previa, o con etiqueta   */
    FUENTE_SENTENCIA_LARGA          /* no cabe en MAX_SEGMENTOS lineas        */
} ErrorFuente;

typedef struct {
    uint32_t etiqueta;                   /* 1..99999; 0 = sin etiqueta         */
    uint16_t linea;                      /* linea fisica donde empieza         */
    uint16_t longitud;                   /* caracteres usados en texto         */
    uint8_t  segmentos;                  /* lineas fisicas que aportan texto   */
    uint16_t linea_seg[MAX_SEGMENTOS];   /* numero de linea fisica de cada una
                                            (hay comentarios intercalados que
                                            hacen que no sean consecutivas)    */
} Sentencia;

/* El TEXTO de la sentencia (MAX_SENTENCIA + 1 B, con su '\0' final) no esta en el residente: vive en la pagina de
 * datos SENT_PAGINA a partir de SENT_OFF (el final de la pagina de tablas del backend, que usa el principio:
 * back.c comprueba que no se pisan). Se lee con sent_car / sent_leer; solo source.c lo escribe. */
#define SENT_PAGINA  10
#define SENT_OFF     6720

/* Caracter i del texto; '\0' si i >= longitud. Lleva una ventana de 32 B: lo normal es leer en orden. */
char sent_car(uint16_t i);

/* Copia n bytes del texto desde la posicion desp (el '\0' final cuenta si se pide). */
void sent_leer(uint16_t desp, char *dst, uint16_t n);

/* Escribe n bytes en el texto desde desp (no cambia longitud). */
void sent_escribir(uint16_t desp, const char *src, uint16_t n);

/* La sentencia en curso. UNICA y global (decision 0006): el compilador procesa
 * una sentencia a la vez, y asi no se pasa un puntero en cada llamada. */
extern Sentencia sentencia;

/* Rellena s a partir de una linea INICIAL: lee la etiqueta (cols 1-5) y copia
 * las columnas 7-72 en s->texto. */
ErrorFuente iniciar_sentencia(const char *linea, uint16_t num_linea);

/* Anade a s una linea de CONTINUACION (num_linea es su numero de linea
 * fisica): rellena con blancos la linea anterior hasta SEGMENTO y copia las
 * columnas 7-72 de esta. Error si las columnas 1-5 no estan en blanco o si no
 * caben mas lineas. */
ErrorFuente anadir_continuacion(const char *linea, uint16_t num_linea);

/* Posicion en el FICHERO del caracter s->texto[offset]: linea fisica y columna
 * (la columna cuenta desde 1; el texto empieza en la 7). Un offset == longitud
 * (el final) se atribuye a la ultima linea. Solo para diagnosticos: contiene
 * una division por 66, que en el Z80 es lenta, pero no esta en el camino
 * caliente. Los tabuladores expandidos al leer
 * (ver tomar_linea) cuentan segun las columnas resultantes; los del cuerpo, como uno. */
void posicion_fuente(uint16_t offset,
                     uint16_t *linea, uint16_t *columna);

/* ------------------------------------------------------------------
 * Lector de sentencias
 * ------------------------------------------------------------------ */

#define LINEA_MAX 72    /* lo que pasa de la columna 72 se descarta al leer */

typedef struct {
    uint16_t    num_linea;      /* numero de la proxima linea por leer (1 = la primera) */
    uint16_t    linea_error;    /* linea fisica del ultimo error devuelto     */
    uint8_t     tiene;          /* 1 = `linea` guarda la linea num_linea ya pedida */
    uint8_t     fin;            /* 1 = ed_linea ya dijo que no hay mas        */
    char        linea[LINEA_MAX + 1];    /* buffer de trabajo: la linea actual */
} Fuente;

/* Las lineas se piden de una en una a `ed_linea` (mf_api.h), que proporciona el
 * EDITOR (o platform/texto.c para pruebas: texto en memoria). Sin '\n'. */
void fuente_iniciar(Fuente *f);

/* Lee la siguiente sentencia logica.
 *   FUENTE_OK   -> s esta lista
 *   FUENTE_FIN  -> no quedan mas
 *   otro        -> error; f->linea_error dice en que linea. Tras un error la
 *                  siguiente llamada continua con la sentencia siguiente, sin
 *                  encadenar errores falsos por las lineas de continuacion. */
ErrorFuente leer_sentencia(Fuente *f);

#endif
