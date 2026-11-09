/* back_rt_entrada.c - Las tablas de las rutinas de entrada (back_rt_entrada.h, READ) en su propio banco (solo en el Next, sin listado).
 * Igual que back_rt_real.c: back.c las lee con rt_ent_leer() a traves de un trampolin. */
#include <stdint.h>
#include <string.h>

#ifdef MF_SIN_LISTADO

#ifndef T
#define T(s)  ((const char *)0)
#endif
#include "back_rt_mac.h"
#include "back_rt_entrada.h"

#ifdef MF_PC_CHECK   /* solo en PC: comprueba que rt_n_* coincide con el tamano real de cada tabla */
#define CHK(n)  typedef char chk_##n[(sizeof(rt_c_##n) == rt_n_##n) ? 1 : -1];
CHK(rdd) CHK(keysc) CHK(keyh) CHK(keyt) CHK(rdline) CHK(rdln) CHK(rdtok) CHK(rdi) CHK(rdl) CHK(rds) CHK(rdf)
#endif

#define TAB(n)  { rt_c_##n, rt_n_##n }   /* rt_n_*: generado por rtasm.py (sccz80 no admite sizeof aqui) */

static const struct { const uint8_t *p; uint16_t n; } rt_ent_tab[RT_N - RT_RDD] = {
    TAB(rdd), TAB(keysc), TAB(keyh), TAB(keyt), TAB(rdline), TAB(rdln), TAB(rdtok), TAB(rdi), TAB(rdl), TAB(rds), TAB(rdf)
};

/* Copia hasta n bytes de la tabla de la rutina r desde el desplazamiento off; devuelve cuantos. */
uint8_t rt_ent_leer(uint8_t r, uint16_t off, uint8_t *dst, uint8_t n)
{
    uint16_t tam = rt_ent_tab[r - RT_RDD].n;

    if (off >= tam) {
        return 0;
    }
    if ((uint16_t)(tam - off) < n) {
        n = (uint8_t)(tam - off);
    }
    memcpy(dst, rt_ent_tab[r - RT_RDD].p + off, n);
    return n;
}

#endif
