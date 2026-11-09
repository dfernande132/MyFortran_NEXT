/* ed_teclas.h - Codigos de tecla que entiende el editor.
 *
 * HIPOTESIS H3 (SIN VERIFICAR EN EL NEXT): estos son los codigos de la ROM del
 * Spectrum 48K para las teclas de edicion (CAPS SHIFT + digito):
 *   CAPS+1 EDIT=7   CAPS+5 izq=8   CAPS+8 der=9   CAPS+6 abajo=10
 *   CAPS+7 arriba=11   CAPS+0 DELETE=12   ENTER=13
 * Que los devuelva getchar() de z88dk en el Next es lo que mide el primer test
 * (la barra de estado muestra el codigo crudo de la ultima tecla, en hexadecimal).
 * Si algun codigo es otro, se corrige AQUI o con -DED_K_xxx=n en la compilacion.
 */
#ifndef MF_ED_TECLAS_H
#define MF_ED_TECLAS_H

#ifndef ED_K_IZQ
#define ED_K_IZQ     8
#endif
#ifndef ED_K_DER
#define ED_K_DER     9
#endif
#ifndef ED_K_ABA
#define ED_K_ABA     10
#endif
#ifndef ED_K_ARR
#define ED_K_ARR     11
#endif
#ifndef ED_K_BORRAR
#define ED_K_BORRAR  12      /* borra hacia atras */
#endif
#ifndef ED_K_ENTER
#define ED_K_ENTER   13
#endif
#ifndef ED_K_MENU
#define ED_K_MENU    7       /* EDIT: abre el modo comando */
#endif
#ifndef ED_K_MENU2
#define ED_K_MENU2   27      /* Esc: CAPS+SPACE, lo traduce ed_next.c (comprobado en ZEsarUX) */
#endif
#ifndef ED_K_TAB
#define ED_K_TAB     0       /* SIN DEFINIR: pulsar Tab y mirar `Key XX` en la barra de estado.
                                Mientras tanto: EDIT/Esc y luego I (tabulador) */
#endif

#endif
