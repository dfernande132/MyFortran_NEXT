/* ed_salida.h - MfSalida del editor: escribe el .nex que genera el compilador.
 *
 * Solo con -DED_CON_COMPILADOR. Todo esto va en memoria RESIDENTE (no en el banco 41):
 * nex_generar (banco 43) llama a abrir/escribir/cerrar mientras ocupa la ventana del 41. */
#ifndef MF_ED_SALIDA_H
#define MF_ED_SALIDA_H

#ifdef ED_CON_COMPILADOR
#include "mf_api.h"

#define ED_NEX_MAX 36                   /* caracteres del nombre del .nex (sin el NUL) */

extern char ed_nex_nombre[ED_NEX_MAX + 1];  /* nombre del .nex a escribir (lo fija edit.c) */
extern const MfSalida ed_salida;
#endif

#endif
