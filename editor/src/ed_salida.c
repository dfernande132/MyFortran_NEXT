/* ed_salida.c - MfSalida del editor (ver ed_salida.h). Portable: usa plat_fich_*. */
#include "ed_salida.h"

#ifdef ED_CON_COMPILADOR
#include "ed_plat.h"

char ed_nex_nombre[ED_NEX_MAX + 1];

static int sh = -1;                     /* manejador del .nex abierto */

static uint8_t s_abrir(void)
{
    sh = plat_fich_abrir_escritura(ed_nex_nombre);
    return (uint8_t)(sh >= 0);
}

static uint8_t s_escribir(const void *d, uint16_t n)
{
    return (uint8_t)(plat_fich_escribir(sh, (const uint8_t *)d, n) == (int)n);
}

static uint8_t s_cerrar(uint8_t ok)
{
    if (sh >= 0) plat_fich_cerrar(sh);
    sh = -1;
    return ok;
}

const MfSalida ed_salida = { s_abrir, s_escribir, s_cerrar };
#else
typedef int ed_salida_vacio;            /* unidad de traduccion no vacia sin compilador */
#endif
