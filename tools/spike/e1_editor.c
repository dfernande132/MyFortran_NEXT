/* e1_editor.c - la parte RESIDENTE del editor: la salida del .nex (ed_salida.c) y los ficheros de esxdos (ed_fich.c).
 * Las llama el compilador (banco 43) mientras ocupa la ventana del banco 41, asi que NO pueden estar en el 41.
 * 0.11: edbuf.c y ed_linea.c ya no estan aqui sino en el banco 41 (e1_banco41.c); ed_linea se llama por t_ed_linea. */
#include "../../editor/src/ed_salida.c"
#include "../../editor/plataforma/next/ed_fich.c"
