/* z80.c - Capa de emision Z80 (ver z80.h). */
#include <stdio.h>
#include <string.h>
#include "z80.h"
#include "banco.h"

/* Estado de la emision. Con el backend dividido (BACK_DIVIDIDO, el Next) la parte C lleva su propia copia de este fichero y comparte el
 * estado con la A: lo define la A y la C lo declara extern. */
typedef struct {
    uint8_t  *img;
    uint16_t  cap, org, pc;
    uint8_t   pasada, desb;
    void    (*sal)(const char *);
    uint16_t  nbuf, off0;
    uint8_t   buf[16];
} Z80Est;
#if defined(BACK_PARTE_C)
extern Z80Est z_est;
#elif defined(BACK_DIVIDIDO)
Z80Est z_est;
#else
static Z80Est z_est;
#endif

void z_iniciar(uint8_t *img, uint16_t cap, uint16_t org, uint8_t pasada,
               void (*sal)(const char *linea))
{
    z_est.img = img;
    z_est.nbuf = 0;
    z_est.off0 = 0;
    if (img == NULL && cap > Z_IMG_MAX) {
        cap = Z_IMG_MAX;
    }
    z_est.cap = cap;
    z_est.org = org;
    z_est.pc = org;
    z_est.pasada = pasada;
    z_est.desb = 0;
    z_est.sal = sal;
}

uint16_t z_pc(void)
{
    return z_est.pc;
}

uint8_t z_desbordado(void)
{
    return z_est.desb;
}

void z_fin(void)
{
    if (z_est.img == NULL && z_est.nbuf > 0) {
        /* los bloques empiezan en multiplos de 16: nunca cruzan una pagina de 8 KB */
        banco_escribir((uint8_t)(Z_PAG_IMG + (z_est.off0 >> 13)), (uint16_t)(z_est.off0 & (BANCO_PAGINA_TAM - 1)),
                       z_est.buf, z_est.nbuf);
        z_est.off0 = (uint16_t)(z_est.off0 + z_est.nbuf);
        z_est.nbuf = 0;
    }
}

void z_imagen_leer(uint16_t desp, uint8_t *dst, uint16_t n)
{
    while (n > 0) {
        uint16_t en_pag = (uint16_t)(desp & (BANCO_PAGINA_TAM - 1));
        uint16_t k = (uint16_t)(BANCO_PAGINA_TAM - en_pag);

        if (k > n) {
            k = n;
        }
        banco_leer((uint8_t)(Z_PAG_IMG + (desp >> 13)), en_pag, dst, k);
        desp = (uint16_t)(desp + k);
        dst += k;
        n = (uint16_t)(n - k);
    }
}

static void byte(uint8_t b)
{
    uint16_t i = (uint16_t)(z_est.pc - z_est.org);

    if (i >= z_est.cap) {
        z_est.desb = 1;
    } else if (z_est.pasada == 2) {
        if (z_est.img != NULL) {
            z_est.img[i] = b;
        } else {
            z_est.buf[z_est.nbuf++] = b;
            if (z_est.nbuf == sizeof(z_est.buf)) {
                z_fin();
            }
        }
    }
    z_est.pc++;
}

void z_ins(const char *txt, const char *arg, uint8_t n,
           uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
{
#ifndef MF_SIN_LISTADO
    char l[96];

    if (z_est.pasada == 2 && z_est.sal != NULL && txt != NULL) {
        /* txt (<= ~30 B) + arg (<= ~24 B) caben de sobra en l[96]; sprintf porque el printf de z88dk es el basico */
        strcpy(l, "        ");
        sprintf(l + 8, txt, arg != NULL ? arg : "");
        z_est.sal(l);
    }
#endif
    byte(b0);
    if (n > 1) byte(b1);
    if (n > 2) byte(b2);
    if (n > 3) byte(b3);
}

void z_datos(const uint8_t *b, uint16_t n)
{
    uint16_t i;
#ifndef MF_SIN_LISTADO
    char l[96];

    if (z_est.pasada == 2 && z_est.sal != NULL && n > 0) {
        for (i = 0; i < n; i += 8) {
            uint16_t k = (uint16_t)(n - i > 8 ? 8 : n - i);
            uint16_t j;
            int p;

            strcpy(l, "        DEFB ");
            p = 13;
            for (j = 0; j < k; j++) {
                p += sprintf(l + p, "%s%u", j ? "," : "", (unsigned)b[i + j]);
            }
            z_est.sal(l);
        }
    }
#endif
    for (i = 0; i < n; i++) {
        byte(b[i]);
    }
}

void z_linea(const char *txt)
{
#ifndef MF_SIN_LISTADO
    if (z_est.pasada == 2 && z_est.sal != NULL) {
        z_est.sal(txt);
    }
#endif
}
