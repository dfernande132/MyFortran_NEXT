/* e1_main.c - main() RESIDENTE de la integracion con el editor: pinta la pila (para medirla desde el editor, tecla M)
 * y entra en el editor por su unico punto de entrada (ED-002, opcion A). */
#include <stdint.h>
#include "banco.h"

extern unsigned char _BSS_END_tail;
uint8_t t_ed_ejecutar(void);

#define PILA_PINTADA 2048
#define PILA_HUECO   96
#define MARGEN_BSS   64

static unsigned char *pila_ini;
static unsigned char *pila_base;

/* Bytes de pila usados desde que main pinto (el rango pintado en *rango). */
unsigned int e1_pila_usada(unsigned int *rango)
{
    unsigned char *q = pila_ini;

    *rango = (unsigned int)(pila_base - pila_ini);
    while (q < pila_base - PILA_HUECO && *q == 0xA5) {
        q++;
    }
    return (unsigned int)(pila_base - q);
}

int main(void)
{
    unsigned char marca;
    unsigned char *q;

    pila_base = &marca;
    pila_ini = pila_base - PILA_PINTADA;
    if (pila_ini < &_BSS_END_tail + MARGEN_BSS) {
        pila_ini = &_BSS_END_tail + MARGEN_BSS;
    }
    for (q = pila_ini; q < pila_base - PILA_HUECO; q++) {
        *q = 0xA5;
    }
    banco_iniciar();
    for (;;) {
        if (t_ed_ejecutar() == 2) {
            break;
        }
    }
    return 0;
}
