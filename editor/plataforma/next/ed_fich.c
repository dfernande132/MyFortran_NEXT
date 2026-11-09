/* ed_fich.c - Ficheros del editor en el Next (esxdos). RESIDENTE (fuera del banco 41).
 *
 * Antes estaba en ed_next.c. Ahora va aparte porque MfSalida (ed_salida.c) lo llama
 * mientras el compilador (banco 43) ocupa la ventana del 41: estas funciones tienen que
 * estar en memoria residente, nunca en un banco. Ver ED-002.
 *
 * H8/H9/H10 como en ed_next.c: SIN VERIFICAR con esxdos y el banco 41 mapeado.
 */
#include <stdint.h>
#include <errno.h>
#include <arch/zxn/esxdos.h>
#include "ed_plat.h"

/* ------------------------------------------------------------------
 * Ficheros (solo lectura)
 * ------------------------------------------------------------------ */

int plat_fich_abrir_lectura(const char *nombre)
{
    unsigned char h;

    errno = 0;
    h = esxdos_f_open((void *)nombre, ESXDOS_MODE_OPEN_EXIST | ESXDOS_MODE_R);
    if (errno != 0) return -(int)errno;     /* MyLISP: mirar errno, no el valor devuelto */
    return (int)h;
}

int plat_fich_leer(int h, uint8_t *dst, uint16_t n)
{
    int r;

    r = esxdos_f_read((unsigned char)h, dst, n);
    return (r > 0) ? r : 0;                 /* H9 */
}

void plat_fich_cerrar(int h)
{
    esxdos_f_close((unsigned char)h);
}

/* ------------------------------------------------------------------
 * Ficheros (escritura) - H10
 * ------------------------------------------------------------------ */

int plat_fich_abrir_escritura(const char *nombre)
{
    unsigned char h;

    errno = 0;
    h = esxdos_f_open((void *)nombre, ESXDOS_MODE_CREAT_TRUNC | ESXDOS_MODE_W);
    if (errno != 0) return -(int)errno;
    return (int)h;
}

int plat_fich_escribir(int h, const uint8_t *src, uint16_t n)
{
    int r;

    errno = 0;
    r = (int)esxdos_f_write((unsigned char)h, (void *)src, n);
    if (errno != 0) return -1;
    return r;
}
