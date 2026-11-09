/* e1_editor.c - la parte RESIDENTE del editor: edbuf + ed_linea (via g1/i1), la salida del .nex (ed_salida.c) y los
 * ficheros de esxdos (ed_fich.c). Estas dos ultimas las llama el compilador (banco 43) mientras ocupa la ventana
 * del banco 41, asi que NO pueden estar en el 41. */
#include "g1_editor.c"
#include "../../editor/src/ed_salida.c"
#include "../../editor/plataforma/next/ed_fich.c"
