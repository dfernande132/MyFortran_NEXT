/* g1_banco46.c - Generador+backend: fase "lectura" en el banco 46 (igual que i1). */
#include "i1_banco46.c"
#include "../../compiler/src/back_rt_real.c"      /* tablas del runtime REAL (antes en el 45, que se quedo corto con S1); las lee el backend (43) por trampolin */
#include "../../compiler/src/real.c"      /* constantes REAL (texto -> IEEE 754); las llama gen.c (banco 44) por trampolin: el 44 se quedo corto con S2 */
