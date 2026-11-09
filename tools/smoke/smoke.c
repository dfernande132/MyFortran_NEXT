/* smoke.c - Comprueba que la cadena de herramientas funciona.
 *
 * Se compila dos veces con el MISMO fichero:
 *   - en PC con gcc            (tarea "PC: smoke")
 *   - para el Next con z88dk   (tarea "Next: smoke"), que define MF_TARGET_NEXT
 *
 * Es la primera aplicacion del principio del proyecto: un unico codigo C
 * portable, y las diferencias de plataforma aisladas con #ifdef (despues, en platform/).
 */
#include <stdio.h>

int main(void)
{
    printf("MyFortran: cadena de herramientas OK\n");
#ifdef MF_TARGET_NEXT
    /* Un .nex no vuelve al Browser de NextZXOS al terminar: nos quedamos aqui. */
    for (;;) { }
#endif
    return 0;
}
