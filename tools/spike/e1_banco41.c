/* e1_banco41.c - Integracion del EDITOR: el editor (logica, pantalla/teclado del Next y el bucle) en el banco 41,
 * como propone ED-002. Unity build: se incluyen los .c reales del editor sin modificarlos, para que el gancho de
 * pruebas (e1_bucle.h) vea sus funciones static (mb_*, msgbuf...). RESIDENTES (e1_editor.c):
 * ed_salida.c (MfSalida) y ed_fich.c (esxdos): los llama el compilador con el banco 41 desmapeado. */
#pragma bank 41

/* 0.11: edbuf.c y ed_linea.c (antes residentes) viven aqui; el compilador llama a ed_linea por trampolin (t_ed_linea).
 * Su bss (eb, cache de ed_linea) sigue siendo residente. */
#include "../../editor/src/edbuf.c"
#include "../../editor/src/ed_linea.c"
#include "../../editor/src/edit.c"
#include "../../editor/plataforma/next/ed_next.c"
#include "e1_bucle.h"                       /* define ED_GANCHO_TECLA (Esc+M mide) antes del bucle */
#include "../../editor/src/ed_bucle.c"
