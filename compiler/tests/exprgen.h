/* exprgen.h - Generador de expresiones enteras y logicas al azar, con su valor de
 * referencia calculado en C. Lo comparten test_cap06 (TAC) y test_cap07 (Z80). */
#ifndef MF_TEST_EXPRGEN_H
#define MF_TEST_EXPRGEN_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint32_t rng_estado = 12345u;

static uint32_t rng(void)
{
    rng_estado ^= rng_estado << 13;
    rng_estado ^= rng_estado >> 17;
    rng_estado ^= rng_estado << 5;
    return rng_estado;
}

#define NV 8        /* variables A..H */
static int16_t valores[NV];

typedef struct Ex {
    int    tipo;      /* 0 const, 1 var, 2 neg, 3 + 4 - 5 * 6 / 7 ** , 8 relacional, 9 not, 10 and 11 or 12 eqv 13 neqv */
    int    v;
    struct Ex *a, *b;
} Ex;

static Ex pool_ex[4096];
static int n_ex;

static Ex *nuevo(int tipo, int v, Ex *a, Ex *b)
{
    Ex *e = &pool_ex[n_ex++];

    e->tipo = tipo; e->v = v; e->a = a; e->b = b;
    return e;
}

static int16_t pot(int16_t a, int16_t b)
{
    int16_t r = 1;
    int16_t i;

    for (i = 0; i < b; i++) {
        r = (int16_t)((int32_t)r * a);
    }
    return r;
}

/* entero aleatorio de profundidad d */
static Ex *gen_ent(int d)
{
    int k = d <= 0 ? (int)(rng() % 2) : (int)(rng() % 8);

    switch (k) {
    case 0: return nuevo(0, (int)(rng() % 2000) - 1000, NULL, NULL);
    case 1: return nuevo(1, (int)(rng() % NV), NULL, NULL);
    case 2: return nuevo(2, 0, gen_ent(d - 1), NULL);
    case 3: return nuevo(3, 0, gen_ent(d - 1), gen_ent(d - 1));
    case 4: return nuevo(4, 0, gen_ent(d - 1), gen_ent(d - 1));
    case 5: return nuevo(5, 0, gen_ent(d - 1), gen_ent(d - 1));
    case 6: {
        /* divisor: constante distinta de cero o variable E..H (nunca cero) */
        Ex *dv = (rng() & 1) ? nuevo(0, (int)(rng() % 9) + 1, NULL, NULL) : nuevo(1, 4 + (int)(rng() % 4), NULL, NULL);

        return nuevo(6, 0, gen_ent(d - 1), dv);
    }
    default: {
        /* exponente 0..4: constante o variable A,B (valores 0..4) */
        Ex *ex = (rng() & 1) ? nuevo(0, (int)(rng() % 5), NULL, NULL) : nuevo(1, (int)(rng() % 2), NULL, NULL);

        return nuevo(7, 0, gen_ent(d - 1), ex);
    }
    }
}

static Ex *gen_log(int d)
{
    int k = d <= 0 ? 8 : 8 + (int)(rng() % 6);

    switch (k) {
    case 8: {
        Ex *e = nuevo(8, (int)(rng() % 6), gen_ent(d > 0 ? d - 1 : 0), gen_ent(d > 0 ? d - 1 : 0));
        return e;
    }
    case 9:  return nuevo(9, 0, gen_log(d - 1), NULL);
    default: return nuevo(k, 0, gen_log(d - 1), gen_log(d - 1));
    }
}

/* Evalua. */
static int32_t evalua(const Ex *e)
{
    int32_t x, y;

    switch (e->tipo) {
    case 0: return (int16_t)e->v;
    case 1: return valores[e->v];
    case 2: return (int16_t)(-evalua(e->a));
    case 3: x = evalua(e->a); y = evalua(e->b); return (int16_t)(x + y);
    case 4: x = evalua(e->a); y = evalua(e->b); return (int16_t)(x - y);
    case 5: x = evalua(e->a); y = evalua(e->b); return (int16_t)(x * y);
    case 6: x = evalua(e->a); y = evalua(e->b); return (int16_t)(x / y);
    case 7: x = evalua(e->a); y = evalua(e->b); return pot((int16_t)x, (int16_t)y);
    case 8: {
        int32_t p = evalua(e->a), q = evalua(e->b);

        switch (e->v) {
        case 0: return p == q;
        case 1: return p != q;
        case 2: return p < q;
        case 3: return p <= q;
        case 4: return p > q;
        default: return p >= q;
        }
    }
    case 9:  return !evalua(e->a);
    case 10: x = evalua(e->a); y = evalua(e->b); return x && y;
    case 11: x = evalua(e->a); y = evalua(e->b); return x || y;
    case 12: x = evalua(e->a); y = evalua(e->b); return x == y;
    default: x = evalua(e->a); y = evalua(e->b); return x != y;
    }
}

/* Escribe la expresion con TODOS los parentesis (la precedencia se prueba aparte). */
static void texto(const Ex *e, char *dst)
{
    char a[2048], b[2048];
    static const char *const opn[] = { "", "", "", "+", "-", "*", "/", "**" };
    static const char *const rel[] = { ".EQ.", ".NE.", ".LT.", ".LE.", ".GT.", ".GE." };
    static const char *const lg[] = { ".AND.", ".OR.", ".EQV.", ".NEQV." };

    switch (e->tipo) {
    case 0:
        sprintf(dst, "(%d)", e->v);
        break;
    case 1: sprintf(dst, "%c", 'A' + e->v); break;
    case 2: texto(e->a, a); sprintf(dst, "(-%s)", a); break;
    case 3: case 4: case 5: case 6: case 7:
        texto(e->a, a); texto(e->b, b);
        sprintf(dst, "(%s%s%s)", a, opn[e->tipo], b);
        break;
    case 8: texto(e->a, a); texto(e->b, b); sprintf(dst, "(%s%s%s)", a, rel[e->v], b); break;
    case 9: texto(e->a, a); sprintf(dst, "(.NOT.%s)", a); break;
    default:
        texto(e->a, a); texto(e->b, b);
        sprintf(dst, "(%s%s%s)", a, lg[e->tipo - 10], b);
        break;
    }
}


#endif
