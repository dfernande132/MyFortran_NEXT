/* real.c - Texto decimal -> IEEE 754 simple, exacto y solo con enteros (ver real.h).
 *
 * Se trata el valor como una fraccion A / B de enteros GRANDES (arrays de bytes, little-endian) y se divide en binario, bit a bit:
 *   texto = N x 10^p   ->   A = N x 10^p, B = 1      (p >= 0)
 *                           A = N,        B = 10^-p  (p < 0)
 * Se normaliza (se desplaza A o B hasta que 1 <= A/B < 2, contando el exponente binario), se sacan 25 bits del cociente (24 de
 * mantisa y 1 de guarda) y se redondea al par con el resto de la division como "sticky". */
#include <string.h>
#include "real.h"

#define BN 34                           /* 272 bits: 10^71 x 2 x 2 cabe de sobra */

static void b_cero(uint8_t *a)
{
    memset(a, 0, BN);
}

/* a = a * 10 + d */
static void b_por10(uint8_t *a, uint8_t d)
{
    uint16_t c = d;
    uint8_t i;

    for (i = 0; i < BN; i++) {
        c = (uint16_t)(a[i] * 10u + c);
        a[i] = (uint8_t)c;
        c >>= 8;
    }
}

static void b_shl1(uint8_t *a)
{
    uint8_t c = 0, n, i;

    for (i = 0; i < BN; i++) {
        n = (uint8_t)(a[i] >> 7);
        a[i] = (uint8_t)((a[i] << 1) | c);
        c = n;
    }
}

/* -1, 0, 1 */
static int8_t b_cmp(const uint8_t *a, const uint8_t *b)
{
    uint8_t i = BN;

    while (i-- > 0) {
        if (a[i] != b[i]) {
            return a[i] < b[i] ? -1 : 1;
        }
    }
    return 0;
}

/* a = a - b (a >= b) */
static void b_sub(uint8_t *a, const uint8_t *b)
{
    uint8_t i;
    uint8_t prestamo = 0;
    uint16_t t;

    for (i = 0; i < BN; i++) {
        t = (uint16_t)(0x100u + a[i] - b[i] - prestamo);
        a[i] = (uint8_t)t;
        prestamo = (uint8_t)((t & 0x100u) ? 0 : 1);
    }
}

static uint8_t b_es_cero(const uint8_t *a)
{
    uint8_t i;

    for (i = 0; i < BN; i++) {
        if (a[i] != 0) {
            return 0;
        }
    }
    return 1;
}

uint8_t real_de_texto(const char *s, uint32_t *bits)
{
    uint8_t A[BN], B[BN];
    int16_t p10 = 0;                    /* exponente decimal: valor = N x 10^p10 */
    int16_t nd = 0;                     /* digitos significativos guardados en N   */
    int16_t entera = 0;                 /* digitos significativos ANTES del punto   */
    uint8_t punto = 0, sticky_dec = 0, hay = 0;
    int16_t exp = 0;
    int8_t signo_e = 1;
    int16_t e2 = 0;
    uint32_t q = 0;
    uint8_t i, bit;
    int16_t dec10;

    *bits = 0;
    b_cero(A);
    for (; *s != '\0' && *s != 'E' && *s != 'D'; s++) {
        if (*s == '.') {
            punto = 1;
            continue;
        }
        if (*s < '0' || *s > '9') {
            continue;
        }
        bit = (uint8_t)(*s - '0');
        if (nd == 0 && bit == 0) {                  /* ceros a la izquierda: no son significativos */
            if (punto) {
                p10--;
            }
            continue;
        }
        hay = 1;
        if (nd >= 30) {                             /* mas de 30 cifras (limite): lo que sobra solo actua como "sticky" */
            if (bit != 0) {
                sticky_dec = 1;
            }
            if (!punto) {
                p10++;
            }
            continue;
        }
        b_por10(A, bit);
        nd++;
        if (punto) {
            p10--;
        } else {
            entera++;
        }
    }
    if (*s == 'E' || *s == 'D') {
        s++;
        if (*s == '+' || *s == '-') {
            signo_e = (int8_t)(*s == '-' ? -1 : 1);
            s++;
        }
        for (; *s >= '0' && *s <= '9'; s++) {
            if (exp < 1000) {
                exp = (int16_t)(exp * 10 + (*s - '0'));
            }
        }
        p10 = (int16_t)(p10 + signo_e * exp);
    }
    if (!hay) {
        return REAL_OK;                             /* 0 */
    }
    if (sticky_dec) {                               /* un resto no nulo recortado: un "1" mas, al final, basta para el redondeo */
        b_por10(A, 1);
        nd++;
        p10--;
    }
    /* magnitud aproximada: 10^(nd + p10 - 1) <= valor < 10^(nd + p10) */
    dec10 = (int16_t)(nd + p10);
    if (dec10 > 39) {
        return REAL_DESBORDA;
    }
    if (dec10 < -46) {
        return REAL_OK;                             /* por debajo del menor normal: 0 */
    }
    b_cero(B);
    B[0] = 1;
    if (p10 >= 0) {
        for (i = 0; i < p10 && i < 100; i++) {
            b_por10(A, 0);
        }
    } else {
        for (i = 0; i < -p10 && i < 100; i++) {
            b_por10(B, 0);
        }
    }
    /* normalizar: 1 <= A/B < 2 */
    {
        uint8_t B2[BN];

        for (;;) {
            if (b_cmp(A, B) < 0) {
                b_shl1(A);
                e2--;
                continue;
            }
            memcpy(B2, B, BN);
            b_shl1(B2);
            if (b_cmp(A, B2) >= 0) {
                memcpy(B, B2, BN);
                e2++;
                continue;
            }
            break;
        }
    }
    /* 25 bits del cociente (el primero es 1) */
    for (i = 0; i < 25; i++) {
        q <<= 1;
        if (b_cmp(A, B) >= 0) {
            b_sub(A, B);
            q |= 1;
        }
        b_shl1(A);
    }
    {
        uint32_t m = q >> 1;                        /* 24 bits */
        uint8_t guarda = (uint8_t)(q & 1);
        uint8_t sticky = (uint8_t)!b_es_cero(A);

        if (guarda && (sticky || (m & 1))) {
            m++;
        }
        if (m >= 0x1000000UL) {                     /* el redondeo llego a 2.0 */
            m >>= 1;
            e2++;
        }
        e2 = (int16_t)(e2 + 127);
        if (e2 >= 255) {
            return REAL_DESBORDA;
        }
        if (e2 <= 0) {
            return REAL_OK;                         /* demasiado pequeno: 0 */
        }
        *bits = ((uint32_t)(uint8_t)e2 << 23) | (m & 0x7FFFFFUL);
    }
    return REAL_OK;
}

uint32_t real_de_entero(int16_t v)
{
    uint32_t m, sig = 0;
    uint8_t e = 0;

    if (v == 0) {
        return 0;
    }
    if (v < 0) {
        sig = 0x80000000UL;
        m = (uint32_t)(-(int32_t)v);
    } else {
        m = (uint32_t)v;
    }
    while ((m >> (e + 1)) != 0) {
        e++;
    }
    return sig | ((uint32_t)(e + 127) << 23) | ((m << (23 - e)) & 0x7FFFFFUL);
}
