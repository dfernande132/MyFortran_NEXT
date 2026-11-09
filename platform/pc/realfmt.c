/* realfmt.c - ver realfmt.h. Solo para PC (usa printf del sistema como oraculo de las cifras). */
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "realfmt.h"

void real_formato(uint32_t bits, char *dst)
{
    float f;
    char t[32];
    int e;
    char d[10];
    char *p = dst;
    int i;

    memcpy(&f, &bits, 4);
    if (bits & 0x80000000u) {
        *p++ = '-';
    }
    if ((bits & 0x7FFFFFFFu) == 0) {
        strcpy(p, "0.00000000");
        return;
    }
    snprintf(t, sizeof(t), "%.8e", (double)fabsf(f));     /* d.dddddddde+XX */
    d[0] = t[0];
    for (i = 0; i < 8; i++) {
        d[1 + i] = t[2 + i];
    }
    d[9] = '\0';
    e = atoi(t + 11);
    if (e >= -1 && e <= 8) {
        if (e == -1) {
            strcpy(p, "0.");
            strcat(p, d);
        } else {
            memcpy(p, d, (size_t)e + 1);
            p += e + 1;
            *p++ = '.';
            strcpy(p, d + e + 1);
        }
        return;
    }
    *p++ = d[0];
    *p++ = '.';
    memcpy(p, d + 1, 8);
    p += 8;
    sprintf(p, "E%c%02d", e < 0 ? '-' : '+', abs(e));
}

/* Un resultado calculado con el float del PC, llevado a las reglas del runtime del Z80. */
uint32_t real_regla(float r, int zero_negativo)
{
    uint32_t b;

    if (isnan(r)) {
        return 0;
    }
    if (isinf(r)) {
        r = r < 0 ? -FLT_MAX : FLT_MAX;
    }
    if (r != 0.0f && fabsf(r) < FLT_MIN) {
        r = 0.0f;
        b = 0;
        return zero_negativo ? 0x80000000u : b;
    }
    memcpy(&b, &r, 4);
    return b;
}

const char *real_err_texto(int codigo)
{
    return codigo == RERR_DIV0 ? "Division by zero" : codigo == RERR_0DIV0 ? "0/0 is indeterminate" :
           codigo == RERR_SQRTNEG ? "SQRT of a negative number" : codigo == RERR_POW0 ? "Zero to a non-positive power" :
           codigo == RERR_ENTRADA ? "Invalid input" : codigo == RERR_FINENT ? "End of input" : "REAL overflow";
}

int real_op(char op, uint32_t xa, uint32_t xb, uint32_t *res)
{
    float a, b, r;
    double x;

    memcpy(&a, &xa, 4);
    memcpy(&b, &xb, 4);
    if (op == '/' && b == 0.0f) {
        return a == 0.0f ? RERR_0DIV0 : RERR_DIV0;
    }
    switch (op) {
    case '+': r = a + b; x = (double)a + (double)b; break;
    case '-': r = a - b; x = (double)a - (double)b; break;
    case '*': r = a * b; x = (double)a * (double)b; break;
    default:  r = a / b; x = (double)a / (double)b; break;
    }
    if (isinf(r) || isnan(r)) {
        return RERR_OVF;
    }
    if (fabsf(r) == FLT_MIN) {
        /* el float del PC redondeo a un denormal y salio el menor normal; aqui se redondea a 24 bits y luego se aplica "< menor normal = 0" */
        float s = (float)(x * 0x1p100);

        if (fabsf(s) < FLT_MIN * 0x1p100f) {
            r = signbit(r) ? -0.0f : 0.0f;
        }
    }
    *res = real_regla(r, signbit(r) != 0);
    return RERR_NINGUNO;
}

static float aflt(uint32_t b)
{
    float f;

    memcpy(&f, &b, 4);
    return f;
}

int real_fun(int k, uint32_t x, uint32_t *res)
{
    float f = aflt(x), r;

    if (k == 2) {
        if (f < 0.0f) {
            return RERR_SQRTNEG;
        }
        r = sqrtf(f);
    } else {
        r = k == 0 ? truncf(f) : roundf(f);
    }
    *res = real_regla(r, signbit(r) != 0);
    return RERR_NINGUNO;
}

int real_mod(uint32_t x, uint32_t p, uint32_t *res)
{
    float a = aflt(x), b = aflt(p), r;

    if (b == 0.0f) {
        return a == 0.0f ? RERR_0DIV0 : RERR_DIV0;
    }
    r = fmodf(a, b);
    *res = real_regla(r, signbit(r) != 0);
    return RERR_NINGUNO;
}

int real_powi(uint32_t x, int16_t n, uint32_t *res)
{
    uint32_t m = n < 0 ? (uint32_t)(-(int32_t)n) : (uint32_t)n;
    uint32_t y = 0x3F800000u, t;
    int e;

    if (aflt(x) == 0.0f && n <= 0) {
        return RERR_POW0;
    }
    if (m & 1) {
        y = x;
    }
    while (m >>= 1) {
        e = real_op('*', x, x, &t);
        if (e != RERR_NINGUNO) return e;
        x = t;
        if (m & 1) {
            e = real_op('*', y, x, &t);
            if (e != RERR_NINGUNO) return e;
            y = t;
        }
    }
    if (n < 0) {
        e = real_op('/', 0x3F800000u, y, &t);
        if (e != RERR_NINGUNO) return e;
        y = t;
    }
    *res = y;
    return RERR_NINGUNO;
}
