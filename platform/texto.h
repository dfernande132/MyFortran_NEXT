/* texto.h - Fuente en MEMORIA para pruebas y medidas (sustituto del editor).
 *
 * Define `ed_linea` (mf_api.h) sirviendo lineas de un texto terminado en '\0'
 * con lineas separadas por '\n'. El editor real proporciona su propio ed_linea;
 * este fichero NO debe enlazarse en el .nex integrado.
 */
#ifndef MF_TEXTO_H
#define MF_TEXTO_H

#include "source.h"

/* Elige el texto que servira ed_linea (no se copia: debe seguir vivo). */
void texto_poner(const char *texto);

/* texto_poner + fuente_iniciar */
void fuente_iniciar_texto(Fuente *f, const char *texto);

/* SOLO PC (pruebas y medidas): el texto de la sentencia vive en una pagina de datos (source.h); estas dos
 * funciones lo ven como una cadena C y lo reemplazan. */
const char *sent_texto(void);                 /* copia estatica del texto actual, terminada en '\0' */
void sent_poner(const char *texto);           /* texto := texto y longitud := strlen                 */

#endif
