/* banco_next.c - Implementacion de banco.h para el Next (z88dk, +zxn).
 *
 * Patron de docs/referencia/CONOCIMIENTO_NEXT_Z88DK..., 4.3, el mismo de MyLISP:
 *   DI -> paginar -> copiar -> RESTAURAR la pantalla -> EI.
 * Cada paso es obligatorio (olvidar el EI deja el teclado muerto; olvidar la
 * restauracion deja la pantalla corrupta).
 *
 * Ventana: MMU2 ($4000-$5FFF). Elegida porque el binario de z88dk siempre
 * arranca en $8000, asi que ahi nunca puede haber codigo ni pila.
 * Paginas fisicas: desde 50, el rango que se sabe que funciona (MyLISP usa
 * 50-93); evitar 0-27 (sistema).
 */
#include <arch/zxn.h>
#include <intrinsic.h>
#include <string.h>
#include "banco.h"

#define PRIMERA_PAGINA   50
#define VENTANA          0x4000
#define PAGINA_PANTALLA  10       /* lo que debe haber en la ventana en reposo */

void banco_iniciar(void)
{
    /* Nada que hacer: las paginas ya existen. Se deja por simetria con PC. */
}

void banco_leer(uint8_t pag, uint16_t off, void *dst, uint16_t n)
{
    intrinsic_di();
    ZXN_WRITE_MMU2(PRIMERA_PAGINA + pag);
    memcpy(dst, (void *)(VENTANA + off), n);
    ZXN_WRITE_MMU2(PAGINA_PANTALLA);
    intrinsic_ei();
}

void banco_escribir(uint8_t pag, uint16_t off, const void *src, uint16_t n)
{
    intrinsic_di();
    ZXN_WRITE_MMU2(PRIMERA_PAGINA + pag);
    memcpy((void *)(VENTANA + off), src, n);
    ZXN_WRITE_MMU2(PAGINA_PANTALLA);
    intrinsic_ei();
}
