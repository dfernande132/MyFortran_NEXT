/* back_rt_real.c - Las tablas de las rutinas REAL (back_rt_real.h) en su propio banco (solo en el Next, sin listado).
 * back.c (banco 43) las lee con rt_real_leer() a traves de un trampolin: asi no ocupan sitio en el banco del backend. */
#include <stdint.h>
#include <string.h>

#ifdef MF_SIN_LISTADO

#ifndef T
#define T(s)  ((const char *)0)
#endif
#include "back_rt_mac.h"
#include "back_rt_real.h"


#ifdef MF_PC_CHECK   /* solo en PC: comprueba que rt_n_* coincide con el tamano real de cada tabla */
#define CHK(n)  typedef char chk_##n[(sizeof(rt_c_##n) == rt_n_##n) ? 1 : -1];
CHK(fpd) CHK(bshl) CHK(bshr) CHK(badd) CHK(bsub) CHK(funp) CHK(fnorm) CHK(fadd)
CHK(fmul) CHK(fdiv) CHK(fcmp) CHK(itof) CHK(ftoi) CHK(fneg) CHK(fput) CHK(fint) CHK(fsqrt) CHK(fmod) CHK(fpow) CHK(err)
#endif

#define TAB(n)  { rt_c_##n, rt_n_##n }   /* rt_n_*: generado por rtasm.py (sccz80 no admite sizeof aqui) */

static const struct { const uint8_t *p; uint16_t n; } rt_real_tab[RT_ERR - RT_FPD + 1] = {
    TAB(fpd), TAB(bshl), TAB(bshr), TAB(badd), TAB(bsub), TAB(funp), TAB(fnorm), TAB(fadd),
    TAB(fmul), TAB(fdiv), TAB(fcmp), TAB(itof), TAB(ftoi), TAB(fneg), TAB(fput),
    TAB(fint), TAB(fsqrt), TAB(fmod), TAB(fpow), TAB(err)
};

/* Copia hasta n bytes de la tabla de la rutina r desde el desplazamiento off; devuelve cuantos. */
uint8_t rt_real_leer(uint8_t r, uint16_t off, uint8_t *dst, uint8_t n)
{
    uint16_t tam = rt_real_tab[r - RT_FPD].n;

    if (off >= tam) {
        return 0;
    }
    if ((uint16_t)(tam - off) < n) {
        n = (uint8_t)(tam - off);
    }
    memcpy(dst, rt_real_tab[r - RT_FPD].p + off, n);
    return n;
}

#endif
