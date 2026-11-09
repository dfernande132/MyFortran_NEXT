/* e1_trampolines.c - los de g1 mas la entrada al editor (banco 41). Mismo mapear()/banco_actual: al volver de
 * cualquier llamada al compilador (que mapea 43..47) cada trampolin restaura lo que habia, es decir, el 41. */
#include "g1_trampolines.c"

#define BANCO_ED 41

uint8_t ed_ejecutar(void);

uint8_t t_ed_ejecutar(void)
{
    uint8_t ant = banco_actual, r;
    mapear(BANCO_ED); r = ed_ejecutar(); mapear(ant);
    return r;
}
