/* test_cap08.c - REAL en el backend Z80 (back.c, back_rt_real.h) con el emulador z80emu.
 *
 * La referencia es tacvm: el mismo programa se ejecuta como TAC (interprete) y como
 * Z80 (emulador) y la salida debe ser identica. Asi cualquier discrepancia es un
 * fallo de la traduccion TAC -> Z80, no del frontal.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mf_api.h"
#include "gen.h"
#include "tac.h"
#include "back.h"
#include "z80.h"
#include "texto.h"
#include "banco.h"
#include "tacvm.h"
#include "z80emu.h"
#include "exprgen.h"
#include "nex.h"

static int pruebas = 0;
static int fallos = 0;

#define COMPROBAR(cond)                                              \
    do {                                                             \
        pruebas++;                                                   \
        if (!(cond)) {                                               \
            fallos++;                                                \
            printf("FALLO (linea %d): %s\n", __LINE__, #cond);       \
        }                                                            \
    } while (0)

#define ORG 0x8000

/* Con -DMF_SIN_TEXTO (el Next) los mensajes van vacios: solo se comprueba su contenido si existen. */
#ifdef MF_SIN_TEXTO
#define TEXTO(m, sub) 1
#else
#define TEXTO(m, sub) (strstr((m), (sub)) != NULL)
#endif

static uint8_t mem[65536];
static char listado[262144];

static void sal(const char *l)
{
    strcat(listado, l);
    strcat(listado, "\n");
}

/* Compila a TAC. Devuelve el numero de errores. */
static int a_tac(const char *prog)
{
    banco_iniciar();
    texto_poner(prog);
    return (int)mf_compilar();
}

/* Genera Z80, lo ejecuta y deja la salida en out. Devuelve el estado (0 = bien). */
static int modo_paginas;       /* 1: la imagen se guarda en las paginas de datos y se recompone */

static int ejecutar_z80(char *out, size_t n, BackRes *br, Z80EmuRes *er)
{
    memset(mem, 0, sizeof(mem));
    listado[0] = '\0';
    if (modo_paginas) {
        back_generar(NULL, 16384, ORG, sal, br);
        if (br->err == 0) {
            z_imagen_leer(0, mem + ORG, (uint16_t)(br->fin_img - ORG));
        }
    } else {
        back_generar(mem + ORG, (uint16_t)(0x10000 - ORG - 0x1000), ORG, sal, br);
    }
    if (br->err != 0) {
        out[0] = '\0';
        return br->err;
    }
    z80emu_ejecutar(mem, br->entry, 50000000u, out, n, er);
    return er->estado;
}

/* El programa debe dar la misma salida en TAC y en Z80. */
static int igual(int linea_test, const char *prog)
{
    char o1[4096], o2[4096], msg[96];
    TacVmRes tr;
    BackRes br;
    Z80EmuRes er;
    int e;

    pruebas++;
    if (a_tac(prog) != 0) {
        char m[96];

        mf_mensaje(mf.diag[0].cod, m);
        printf("FALLO (linea %d): no compila: %d:%d %s\n%s", linea_test, (int)mf.diag[0].linea, (int)mf.diag[0].columna, m, prog);
        fallos++;
        return 0;
    }
    if (tacvm_validar(msg, sizeof(msg)) != 0) {
        printf("FALLO (linea %d): TAC invalido: %s\n", linea_test, msg);
        fallos++;
        return 0;
    }
    tacvm_ejecutar(o1, sizeof(o1), 5000000u, &tr);
    if (tr.estado == TACVM_STOP && tr.msg[0] != '\0') {      /* el STOP del programa generado imprime "STOP <a>" */
        strcat(o1, "STOP ");
        strcat(o1, tr.msg);
        strcat(o1, "\n");
    }
    e = ejecutar_z80(o2, sizeof(o2), &br, &er);
    if (e != 0) {
        printf("FALLO (linea %d): backend/emulador estado %d (instr TAC %u, pc %04X op %02X)\n%s\n",
               linea_test, e, (unsigned)br.err_inst, (unsigned)er.pc, (unsigned)er.op, prog);
        fallos++;
        return 0;
    }
    if (strcmp(o1, o2) != 0) {
        printf("FALLO (linea %d): salida distinta\n--- TAC\n%s--- Z80\n%s--- programa\n%s", linea_test, o1, o2, prog);
        fallos++;
        return 0;
    }
    return 1;
}

#define IGUAL(p) igual(__LINE__, p)

#include <math.h>
#include <float.h>
#include "realfmt.h"
#include "real.h"

/* ---------------------------------------------------------------- programas completos (TAC contra Z80) */

static void test_programas_real(void)
{
    IGUAL("      X = 1.5\n      PRINT *, X\n      END\n");
    IGUAL("      X = 0.1\n      PRINT *, X, -X\n      END\n");
    IGUAL("      PRINT *, 100.0, -2.5, 0.0, 0.01, 1E10, 123456789.0\n      END\n");
    IGUAL("      PRINT *, 1.0E-30, 3.4E38, 1.2E-38, 12345.678, 0.99999999, 9.9999999E9\n      END\n");
    IGUAL("      I = 7\n      X = I / 2\n      Y = I / 2.0\n      PRINT *, X, Y\n      END\n");
    IGUAL("      X = 2.75\n      I = X\n      J = -X\n      PRINT *, I, J\n      END\n");
    IGUAL("      X = 1.5\n      Y = X * 4 - 1\n      PRINT *, Y, -Y, X + X\n      END\n");
    IGUAL("      X = 2.5\n      IF (X .GT. 2) PRINT *, 'A'\n      IF (X .LT. 3) PRINT *, 'B'\n"
          "      IF (2 .EQ. 2.0) PRINT *, 'C'\n      IF (X .NE. X) PRINT *, 'D'\n      END\n");
    IGUAL("      X = -0.5\n      IF (X) 10, 20, 30\n   10 PRINT *, 'NEG'\n      GOTO 40\n   20 PRINT *, 'CERO'\n"
          "      GOTO 40\n   30 PRINT *, 'POS'\n   40 CONTINUE\n      END\n");
    IGUAL("      DO 10 I = 1, 3.9\n         PRINT *, I\n   10 CONTINUE\n      END\n");
    IGUAL("      X = 3.99\n      PRINT *, INT(X), IFIX(-X), REAL(5), FLOAT(I)\n      END\n");
    IGUAL("      REAL A(3)\n      DO 10 I = 1, 3\n         A(I) = I * 0.5\n   10 CONTINUE\n"
          "      S = 0\n      DO 20 I = 1, 3\n         S = S + A(I)\n   20 CONTINUE\n"
          "      PRINT *, A(1), A(3), S\n      END\n");
    IGUAL("      X = 1E38\n      Y = X * 10\n      Z = 0.0\n      PRINT *, Y, 1.0 / Z, Z / Z\n      END\n");
    IGUAL("      X = 1E30\n      Y = X * X\n      PRINT *, 1E-30 * 1E-30, Y\n      END\n");
    IGUAL("      X = 40000.0\n      I = X\n      PRINT *, I\n      END\n");
    /* serie armonica y un bucle con acumulacion */
    IGUAL("      S = 0.0\n      DO 10 I = 1, 20\n         S = S + 1.0 / I\n   10 CONTINUE\n      PRINT *, S\n      END\n");
    IGUAL("      X = 1.0\n      DO 10 I = 1, 12\n         X = X * 2.5\n         PRINT *, X\n   10 CONTINUE\n      END\n");
    IGUAL("      X = 1.0E-35\n      DO 10 I = 1, 6\n         X = X / 7.0\n         PRINT *, X\n   10 CONTINUE\n      END\n");
}

/* ---------------------------------------------------------------- valores aleatorios: bit a bit contra el float del PC */

static uint32_t rnd_estado = 12345;

static uint32_t rnd(void)
{
    rnd_estado = rnd_estado * 1664525u + 1013904223u;
    return rnd_estado >> 8 ^ (rnd_estado << 16);
}

static uint32_t f2b(float f) { uint32_t b; memcpy(&b, &f, 4); return b; }
static float    b2f(uint32_t b) { float f; memcpy(&f, &b, 4); return f; }

/* Un REAL valido (normal o cero) con el exponente e (0 = cero). */
static uint32_t valor(uint32_t e, uint32_t s)
{
    uint32_t m = rnd() & 0x7FFFFF;
    static const uint32_t esp[] = { 0, 0x7FFFFF, 0x400000, 0x000001, 0x7FFFFE, 0x200000, 0x7FFFFC };

    if (rnd() % 6 == 0) {
        m = esp[rnd() % 7];
    }
    if (e == 0) {
        return s << 31;
    }
    return (s << 31) | (e << 23) | m;
}

static uint32_t valor_aleatorio(void)
{
    uint32_t t = rnd() % 16, s = rnd() & 1, e;

    switch (t) {
    case 0: return valor(0, s);
    case 1: return (s << 31) | (254u << 23) | 0x7FFFFF;                 /* maximo */
    case 2: return (s << 31) | (1u << 23);                               /* menor normal */
    case 3: return f2b(s ? -1.0f : 1.0f);
    case 4: return f2b((float)(int)(rnd() % 2000) - 1000.0f);             /* enteros pequenos */
    case 5: e = 100 + rnd() % 56; break;                                  /* exponentes cercanos a 0 */
    case 6: e = 1 + rnd() % 30; break;                                    /* muy pequenos */
    case 7: e = 225 + rnd() % 30; break;                                  /* muy grandes */
    default: e = 1 + rnd() % 254; break;
    }
    return valor(e, s);
}

/* Segundo operando: a menudo del mismo orden que el primero (cancelaciones, redondeos). */
static uint32_t segundo(uint32_t a)
{
    uint32_t t = rnd() % 8, ea = (a >> 23) & 255, e;

    if (t < 4 && ea != 0) {
        int d = (int)(rnd() % 7) - 3;

        if (t == 0) {
            d = (int)(rnd() % 40) - 20;
        }
        e = (uint32_t)((int)ea + d);
        if (e < 1) e = 1;
        if (e > 254) e = 254;
        return valor(e, rnd() & 1);
    }
    if (t == 4 && ea != 0) {                                              /* a o -a */
        return (rnd() & 1) ? a : (a ^ 0x80000000u);
    }
    if ((t == 5 || t == 6) && ea != 0) {                                  /* cerca del desbordamiento y del agotamiento */
        int d = (int)(rnd() % 7) - 3, eb;
        int mul = rnd() & 1;

        if (t == 5) {
            eb = mul ? 127 - (int)ea + d : (int)ea + 127 + d;
        } else {
            eb = mul ? 381 - (int)ea + d : (int)ea - 127 - d;
        }
        if (eb < 1) eb = 1;
        if (eb > 254) eb = 254;
        return valor((uint32_t)eb, rnd() & 1);
    }
    return valor_aleatorio();
}

static uint32_t esp_regla(float r)
{
    return real_regla(r, signbit(r) != 0);
}

static int16_t esp_ftoi(uint32_t b)
{
    float f = b2f(b);

    if (f >= 32767.0f) return 32767;
    if (f <= -32768.0f) return -32768;
    return (int16_t)f;
}

#ifndef MF_SIN_LISTADO
static uint16_t buscar(const char *nombre)
{
    char pat[32];
    const char *p;

    snprintf(pat, sizeof(pat), "V_%s EQU $", nombre);
    p = strstr(listado, pat);
    if (p == NULL) {
        return 0;
    }
    return (uint16_t)strtoul(p + strlen(pat), NULL, 16);
}

static void poke32(uint16_t a, uint32_t b) { memcpy(mem + a, &b, 4); }
static uint32_t peek32(uint16_t a) { uint32_t b; memcpy(&b, mem + a, 4); return b; }

static unsigned long pasos_max;
static unsigned cubre[4][4];            /* [operacion][RERR_*] comprobados contra la Z80 */

static void test_operaciones(int pruebas_n)
{
    static const char prog[] =
        "      REAL X, Y, RADD, RSUB, RMUL, RDIV, RNEG, RI\n"
        "      INTEGER I, J\n"
        "      LOGICAL LLT, LLE, LEQ, LNE, LGT, LGE\n"
        "      RADD = X + Y\n"
        "      RSUB = X - Y\n"
        "      RMUL = X * Y\n"
        "      RDIV = X / Y\n"
        "      RNEG = -X\n"
        "      J = X\n"
        "      RI = I\n"
        "      LLT = X .LT. Y\n"
        "      LLE = X .LE. Y\n"
        "      LEQ = X .EQ. Y\n"
        "      LNE = X .NE. Y\n"
        "      LGT = X .GT. Y\n"
        "      LGE = X .GE. Y\n"
        "      END\n";
    static uint8_t pristina[65536];
    BackRes br;
    Z80EmuRes er;
    char out[64];
    uint16_t vx, vy, vi, vj, vri, vadd, vsub, vmul, vdiv, vneg, vlt, vle, veq, vne, vgt, vge;
    int k, malos = 0;
    uint16_t lo, hi;

    pruebas++;
    if (a_tac(prog) != 0) {
        printf("FALLO (linea %d): no compila el programa de operaciones\n", __LINE__);
        fallos++;
        return;
    }
    memset(mem, 0, sizeof(mem));
    listado[0] = '\0';
    back_generar(mem + ORG, (uint16_t)(0x10000 - ORG - 0x1000), ORG, sal, &br);
    if (br.err != 0) {
        printf("FALLO (linea %d): backend %d (instr TAC %u)\n", __LINE__, br.err, (unsigned)br.err_inst);
        fallos++;
        return;
    }
    vx = buscar("X"); vy = buscar("Y"); vi = buscar("I"); vj = buscar("J"); vri = buscar("RI");
    vadd = buscar("RADD"); vsub = buscar("RSUB"); vmul = buscar("RMUL"); vdiv = buscar("RDIV"); vneg = buscar("RNEG");
    vlt = buscar("LLT"); vle = buscar("LLE"); veq = buscar("LEQ"); vne = buscar("LNE"); vgt = buscar("LGT"); vge = buscar("LGE");
    if (!vx || !vy || !vi || !vj || !vri || !vadd || !vsub || !vmul || !vdiv || !vneg || !vlt || !veq || !vge) {
        printf("FALLO (linea %d): no encuentro las variables en el listado\n", __LINE__);
        fallos++;
        return;
    }
    lo = ORG;
    hi = br.fin_mem;
    memcpy(pristina, mem, sizeof(mem));
    for (k = 0; k < pruebas_n && malos < 12; k++) {
        uint32_t a = valor_aleatorio(), b = segundo(a);
        int16_t ii = (int16_t)(rnd() & 0xFFFF);
        float fa, fb;
        uint32_t ev[4], e_neg, pr;
        int ce[4], f = 4, q4;
        uint16_t dir4[4];
        int16_t e_j;
        int cmp;
        uint16_t lt, le, eq, ne, gt, ge;
        int bad = 0;

        if (k == 0) { a = 0; b = 0; }
        if (k % 7 == 0) { ii = (int16_t)((int)(rnd() % 40) - 20); }
        if (k == 1) { ii = -32768; }
        if (k == 2) { ii = 32767; }
        memcpy(mem + lo, pristina + lo, (size_t)(hi - lo));
        poke32(vx, a);
        poke32(vy, b);
        memcpy(mem + vi, &ii, 2);
        z80emu_ejecutar(mem, br.entry, 50000000u, out, sizeof(out), &er);
        if (er.estado != 0) {
            printf("FALLO (linea %d): emulador estado %d pc %04X op %02X (X=%08X Y=%08X)\n", __LINE__, er.estado,
                   (unsigned)er.pc, (unsigned)er.op, (unsigned)a, (unsigned)b);
            fallos++;
            return;
        }
        if (er.pasos > pasos_max) pasos_max = er.pasos;
        fa = b2f(a);
        fb = b2f(b);
        ce[0] = real_op('+', a, b, &ev[0]);
        ce[1] = real_op('-', a, b, &ev[1]);
        ce[2] = real_op('*', a, b, &ev[2]);
        ce[3] = real_op('/', a, b, &ev[3]);
        for (q4 = 3; q4 >= 0; q4--) {
            if (ce[q4] != RERR_NINGUNO) f = q4;     /* la primera operacion que falla, en el orden del programa */
        }
        e_neg = a ^ 0x80000000u;
        e_j = esp_ftoi(a);
        cmp = fa < fb ? -1 : (fa > fb ? 1 : 0);
        lt = cmp < 0; le = cmp <= 0; eq = cmp == 0; ne = cmp != 0; gt = cmp > 0; ge = cmp >= 0;
        pruebas++;
        dir4[0] = vadd; dir4[1] = vsub; dir4[2] = vmul; dir4[3] = vdiv;
        for (q4 = 0; q4 < 4; q4++) {
            static const char sig[4] = { '+', '-', '*', '/' };

            memcpy(&pr, pristina + dir4[q4], 4);
            if (peek32(dir4[q4]) != (q4 < f ? ev[q4] : pr)) {      /* despues de un error el programa ya no llega: queda como estaba */
                printf("%c %08X %c %08X = %08X, esperado %08X\n", sig[q4], (unsigned)a, sig[q4], (unsigned)b,
                       (unsigned)peek32(dir4[q4]), (unsigned)(q4 < f ? ev[q4] : pr));
                bad = 1;
            }
        }
        for (q4 = 0; q4 <= (f < 4 ? f : 3); q4++) {
            cubre[q4][ce[q4]]++;
        }
        if (f < 4) {                                                 /* error de ejecucion: mensaje y parada */
            char esp[40];

            snprintf(esp, sizeof(esp), "%s\n", real_err_texto(ce[f]));
            if (strcmp(out, esp) != 0) {
                printf("error de ejecucion en op %d con %08X y %08X: salida \"%s\", esperada \"%s\"\n", f, (unsigned)a, (unsigned)b, out, esp);
                bad = 1;
            }
            if (bad) { fallos++; malos++; }
            continue;
        }
        if (out[0] != '\0') { printf("salida inesperada \"%s\" con %08X y %08X\n", out, (unsigned)a, (unsigned)b); bad = 1; }
        if (peek32(vneg) != e_neg) { printf("NEG %08X = %08X\n", (unsigned)a, (unsigned)peek32(vneg)); bad = 1; }
        {
            int16_t j; uint32_t ri = peek32(vri);

            memcpy(&j, mem + vj, 2);
            if (j != e_j) { printf("FTOI %08X = %d, esperado %d\n", (unsigned)a, (int)j, (int)e_j); bad = 1; }
            if (ri != f2b((float)ii)) { printf("ITOF %d = %08X\n", (int)ii, (unsigned)ri); bad = 1; }
        }
        {
            uint16_t g[6] = { 0, 0, 0, 0, 0, 0 }, ex[6] = { lt, le, eq, ne, gt, ge };
            uint16_t ad[6] = { vlt, vle, veq, vne, vgt, vge };
            int q;

            for (q = 0; q < 6; q++) {
                g[q] = (uint16_t)(mem[ad[q]] | (mem[ad[q] + 1] << 8));
                if (g[q] != ex[q]) { printf("CMP %d de %08X y %08X: %d, esperado %d\n", q, (unsigned)a, (unsigned)b, (int)g[q], (int)ex[q]); bad = 1; }
            }
        }
        if (bad) {
            fallos++;
            malos++;
        }
    }
    printf("   (operaciones REAL: %d pares comprobados, maximo %lu instrucciones Z80 por vuelta)\n", k, pasos_max);
    printf("   (errores de ejecucion comprobados [+ - * /]: overflow %u %u %u %u, division por cero %u, 0/0 %u)\n",
           cubre[0][RERR_OVF], cubre[1][RERR_OVF], cubre[2][RERR_OVF], cubre[3][RERR_OVF], cubre[3][RERR_DIV0], cubre[3][RERR_0DIV0]);
}

/* Una funcion REAL del programa generado (R = <expresion de X, Y, N>) contra su oraculo de PC (realfmt.c), con valores aleatorios:
 * mismo resultado bit a bit, o el mismo mensaje de error y la parada. */
typedef int (*Oraculo)(uint32_t x, uint32_t y, int16_t n, uint32_t *r);

static unsigned cubre_fun[6];

static void test_funcion(const char *nombre, const char *expr, int pruebas_n, Oraculo orc, int enteros_grandes)
{
    char prog[256];
    static uint8_t pristina[65536];
    BackRes br;
    Z80EmuRes er;
    char out[64];
    uint16_t vx, vy, vn, vr, lo, hi;
    int k, malos = 0;

    snprintf(prog, sizeof(prog), "      REAL X, Y, R\n      INTEGER N\n      R = %s\n      END\n", expr);
    pruebas++;
    if (a_tac(prog) != 0) {
        printf("FALLO (linea %d): no compila %s\n", __LINE__, prog);
        fallos++;
        return;
    }
    memset(mem, 0, sizeof(mem));
    listado[0] = '\0';
    back_generar(mem + ORG, (uint16_t)(0x10000 - ORG - 0x1000), ORG, sal, &br);
    vx = buscar("X"); vy = buscar("Y"); vn = buscar("N"); vr = buscar("R");
    if (br.err != 0 || !vx || !vy || !vn || !vr) {
        printf("FALLO (linea %d): backend %d (%s)\n", __LINE__, br.err, nombre);
        fallos++;
        return;
    }
    lo = ORG;
    hi = br.fin_mem;
    memcpy(pristina, mem, sizeof(mem));
    for (k = 0; k < pruebas_n && malos < 8; k++) {
        uint32_t x = valor_aleatorio(), y = segundo(x), r = 0, got;
        int16_t n = (int16_t)((int)(rnd() % 14) - 5);
        int e;

        if (k == 0) { x = 0; y = 0; n = 0; }
        if (k == 1) { x = 0; n = -1; }
        if (k % 9 == 0) x = f2b((float)((int)(rnd() % 2000) - 1000) / (float)(1 + rnd() % 8));      /* con parte fraccionaria */
        if (k % 13 == 0) x = (rnd() & 1) ? 0x3F000000u : 0xBF000000u;                                 /* +-0.5 */
        if (k % 17 == 0) x = f2b((float)(int)(rnd() % 100) + 0.5f) ;                                   /* mitades */
        if (enteros_grandes && k % 11 == 0) n = (int16_t)(rnd() % 2000) - 1000;
        if (enteros_grandes && k % 31 == 0) n = (int16_t)-32768;
        if (!enteros_grandes && k % 7 == 0) y = f2b((float)(1 + rnd() % 9) / 10.0f);
        memcpy(mem + lo, pristina + lo, (size_t)(hi - lo));
        poke32(vx, x);
        poke32(vy, y);
        memcpy(mem + vn, &n, 2);
        z80emu_ejecutar(mem, br.entry, 50000000u, out, sizeof(out), &er);
        pruebas++;
        if (er.estado != 0) {
            printf("FALLO %s: emulador estado %d pc %04X op %02X (X=%08X Y=%08X N=%d)\n", nombre, er.estado, (unsigned)er.pc, (unsigned)er.op,
                   (unsigned)x, (unsigned)y, (int)n);
            fallos++;
            return;
        }
        if (er.pasos > pasos_max) pasos_max = er.pasos;
        e = orc(x, y, n, &r);
        got = peek32(vr);
        cubre_fun[e]++;
        if (e == RERR_NINGUNO) {
            if (got != r || out[0] != '\0') {
                printf("%s X=%08X Y=%08X N=%d: R=%08X salida '%s', esperado R=%08X\n", nombre, (unsigned)x, (unsigned)y, (int)n, (unsigned)got, out, (unsigned)r);
                fallos++;
                malos++;
            }
        } else {
            char esp[48];

            snprintf(esp, sizeof(esp), "%s\n", real_err_texto(e));
            if (strcmp(out, esp) != 0 || got != 0) {
                printf("%s X=%08X Y=%08X N=%d: salida '%s' R=%08X, esperado el error '%s'\n", nombre, (unsigned)x, (unsigned)y, (int)n, out,
                       (unsigned)got, real_err_texto(e));
                fallos++;
                malos++;
            }
        }
    }
}

static int orc_aint(uint32_t x, uint32_t y, int16_t n, uint32_t *r) { (void)y; (void)n; return real_fun(0, x, r); }
static int orc_anint(uint32_t x, uint32_t y, int16_t n, uint32_t *r) { (void)y; (void)n; return real_fun(1, x, r); }
static int orc_sqrt(uint32_t x, uint32_t y, int16_t n, uint32_t *r) { (void)y; (void)n; return real_fun(2, x, r); }
static int orc_sqrtabs(uint32_t x, uint32_t y, int16_t n, uint32_t *r) { (void)y; (void)n; return real_fun(2, x & 0x7FFFFFFFu, r); }
static int orc_amod(uint32_t x, uint32_t y, int16_t n, uint32_t *r) { (void)n; return real_mod(x, y, r); }
static int orc_pow(uint32_t x, uint32_t y, int16_t n, uint32_t *r) { (void)y; return real_powi(x, n, r); }

static void test_funciones(int n)
{
    memset(cubre_fun, 0, sizeof(cubre_fun));
    test_funcion("AINT", "AINT(X)", n, orc_aint, 0);
    test_funcion("ANINT", "ANINT(X)", n, orc_anint, 0);
    test_funcion("SQRT(ABS)", "SQRT(ABS(X))", n, orc_sqrtabs, 0);
    test_funcion("SQRT", "SQRT(X)", n, orc_sqrt, 0);
    test_funcion("AMOD", "AMOD(X, Y)", n, orc_amod, 0);
    test_funcion("POW", "X ** N", n, orc_pow, 1);
    printf("   (funciones REAL contra su oraculo: %d valores por funcion; casos sin error %u, con error: division %u, 0/0 %u, overflow %u, SQRT negativa %u, 0**n %u)\n",
           n, cubre_fun[RERR_NINGUNO], cubre_fun[RERR_DIV0], cubre_fun[RERR_0DIV0], cubre_fun[RERR_OVF], cubre_fun[RERR_SQRTNEG], cubre_fun[RERR_POW0]);
}

/* PRINT * de un REAL: el texto del programa generado es el de real_formato(). */
static void test_impresion(int pruebas_n)
{
    static const char prog[] = "      REAL X\n      PRINT *, X\n      END\n";
    static uint8_t pristina[65536];
    BackRes br;
    Z80EmuRes er;
    char out[64], esp[64], fb[24];
    uint16_t vx, lo, hi;
    int k, malos = 0;
    unsigned long mx = 0;

    pruebas++;
    if (a_tac(prog) != 0) {
        printf("FALLO (linea %d): no compila\n", __LINE__);
        fallos++;
        return;
    }
    memset(mem, 0, sizeof(mem));
    listado[0] = '\0';
    back_generar(mem + ORG, (uint16_t)(0x10000 - ORG - 0x1000), ORG, sal, &br);
    vx = buscar("X");
    if (br.err != 0 || !vx) {
        printf("FALLO (linea %d): backend %d\n", __LINE__, br.err);
        fallos++;
        return;
    }
    lo = ORG; hi = br.fin_mem;
    memcpy(pristina, mem, sizeof(mem));
    for (k = 0; k < pruebas_n && malos < 12; k++) {
        uint32_t a = valor_aleatorio();

        if (k % 5 == 1) a = f2b((float)((int)(rnd() % 100000) - 50000) / (float)(1 + rnd() % 1000));
        if (k % 11 == 2) a = f2b(powf(10.0f, (float)((int)(rnd() % 70) - 35)) * (float)(1 + rnd() % 9));
        memcpy(mem + lo, pristina + lo, (size_t)(hi - lo));
        poke32(vx, a);
        z80emu_ejecutar(mem, br.entry, 50000000u, out, sizeof(out), &er);
        real_formato(a, fb);
        snprintf(esp, sizeof(esp), " %s\n", fb);
        pruebas++;
        if (er.estado != 0) {
            printf("FALLO (linea %d): emulador estado %d pc %04X op %02X (%08X)\n", __LINE__, er.estado, (unsigned)er.pc, (unsigned)er.op, (unsigned)a);
            fallos++;
            return;
        }
        if (er.pasos > mx) mx = er.pasos;
        if (strcmp(out, esp) != 0) {
            printf("PRINT %08X: '%s' esperado '%s'\n", (unsigned)a, out, esp);
            fallos++;
            malos++;
        }
    }
    printf("   (impresion REAL: %d valores, maximo %lu instrucciones Z80)\n", k, mx);
}

#endif


/* ---------------------------------------------------------------- programas aleatorios con REAL (TAC contra Z80) */

static const char *const cte_real[] = { "1.5", "0.1", "100.25", "3.0E10", "2E-5", "0.0", "7.0", "1E38", "3.3E-30", "123456.789",
                                        "0.333333", "65536.0", "2.5E5", "9.99E-3" };

static void expr_r(char *dst, int prof);

static void expr_i(char *dst, int prof)
{
    char a[96], b[96];
    static const char *const nombres[] = { "I", "J", "K" };
    uint32_t t = rnd() % (prof <= 0 ? 3 : 10);

    switch (t) {
    case 0: snprintf(dst, 96, "%d", (int)(rnd() % 40)); return;
    case 1: case 2: snprintf(dst, 96, "%s", nombres[rnd() % 3]); return;
    case 3: case 4: case 5:
        expr_i(a, prof - 1);
        expr_i(b, prof - 1);
        snprintf(dst, 96, "(%s %c %s)", a, "+-*"[rnd() % 3], b);
        return;
    case 6: expr_r(a, prof - 1); snprintf(dst, 96, "INT(%s)", a); return;
    case 7: expr_i(a, prof - 1); snprintf(dst, 96, "(-%s)", a); return;
    case 9: expr_r(a, prof - 1); snprintf(dst, 96, "NINT(%s)", a); return;
    default: expr_r(a, prof - 1); snprintf(dst, 96, "IFIX(%s)", a); return;
    }
}

static void expr_r(char *dst, int prof)
{
    char a[96], b[96];
    static const char *const nombres[] = { "A", "B", "C" };
    uint32_t t = rnd() % (prof <= 0 ? 3 : 17);

    switch (t) {
    case 0: snprintf(dst, 96, "%s", cte_real[rnd() % 14]); return;
    case 1: case 2: snprintf(dst, 96, "%s", nombres[rnd() % 3]); return;
    case 3: case 4: case 5: case 6:
        if (rnd() & 1) {
            expr_r(a, prof - 1);
            expr_r(b, prof - 1);
        } else {                                   /* modo mixto */
            if (rnd() & 1) { expr_r(a, prof - 1); expr_i(b, prof - 1); } else { expr_i(a, prof - 1); expr_r(b, prof - 1); }
        }
        snprintf(dst, 96, "(%s %c %s)", a, "+-*/"[rnd() % 4], b);
        return;
    case 7: expr_r(a, prof - 1); snprintf(dst, 96, "(-%s)", a); return;
    case 8: expr_i(a, prof - 1); snprintf(dst, 96, "REAL(%s)", a); return;
    case 10: expr_r(a, prof - 1); snprintf(dst, 96, "ABS(%s)", a); return;
    case 11: expr_r(a, prof - 1); snprintf(dst, 96, "%s(%s)", (rnd() & 1) ? "AINT" : "ANINT", a); return;
    case 12: expr_r(a, prof - 1); snprintf(dst, 96, "SQRT(ABS(%s))", a); return;
    case 13: expr_r(a, prof - 1); expr_r(b, prof - 1); snprintf(dst, 96, "AMOD(%s, %s)", a, b); return;
    case 14: expr_r(a, prof - 1); expr_r(b, prof - 1);
        snprintf(dst, 96, "%s(%s, %s)", (const char *const[]){ "MAX", "MIN", "SIGN", "DIM" }[rnd() % 4], a, b); return;
    case 15: expr_r(a, prof - 1); snprintf(dst, 96, "(%s ** (%d))", a, (int)(rnd() % 7) - 2); return;
    case 16: expr_r(a, prof - 1); expr_i(b, prof - 1); snprintf(dst, 96, "(%s ** (%s))", a, b); return;
    default: expr_i(a, prof - 1); snprintf(dst, 96, "FLOAT(%s)", a); return;
    }
}

static void test_fuzz_real(int n)
{
    int k, hechos = 0;

    for (k = 0; k < n; k++) {
        char prog[3000], e[96], f[96];
        static const char *const rel[] = { ".LT.", ".LE.", ".EQ.", ".NE.", ".GT.", ".GE." };
        int len = 0;

        len += snprintf(prog + len, sizeof(prog) - (size_t)len, "      REAL A, B, C\n      INTEGER I, J, K\n");
        len += snprintf(prog + len, sizeof(prog) - (size_t)len, "      A = %s\n      B = %s\n      C = %s\n", cte_real[rnd() % 14],
                        cte_real[rnd() % 14], cte_real[rnd() % 14]);
        len += snprintf(prog + len, sizeof(prog) - (size_t)len, "      I = %d\n      J = %d\n      K = %d\n", (int)(rnd() % 50) - 20,
                        (int)(rnd() % 3000) - 1000, (int)(rnd() % 7));
        for (int q = 0; q < 4; q++) {
            expr_r(e, rnd() % 3 == 0 ? 3 : 2);
            len += snprintf(prog + len, sizeof(prog) - (size_t)len, "      %c = %s\n", "ABC"[rnd() % 3], e);
            expr_i(e, 2);
            len += snprintf(prog + len, sizeof(prog) - (size_t)len, "      %c = %s\n", "IJK"[rnd() % 3], e);
            expr_r(e, 1);
            expr_r(f, 1);
            len += snprintf(prog + len, sizeof(prog) - (size_t)len, "      IF (%s %s %s) PRINT *, 'T%d'\n", e, rel[rnd() % 6], f, q);
            len += snprintf(prog + len, sizeof(prog) - (size_t)len, "      PRINT *, A, B, C\n      PRINT *, I, J, K\n");
        }
        len += snprintf(prog + len, sizeof(prog) - (size_t)len, "      END\n");
        {
            const char *l, *l2;
            int ok = 1;

            /* algunas lineas pueden pasar de 72 columnas: se comprueba que compila antes de comparar */
            for (l = prog; *l; l = l2 + 1) {
                l2 = strchr(l, '\n');
                if (l2 == NULL || l2 - l > 72) { ok = 0; break; }
            }
            if (!ok) { continue; }
        }
        igual(__LINE__, prog);
        hechos++;
    }
    printf("   (programas aleatorios con REAL: %d de %d)\n", hechos, n);
}

int main(int argc, char **argv)
{
    int n = argc > 1 ? atoi(argv[1]) : 3000;

    test_programas_real();
    modo_paginas = 1;
    test_programas_real();
    modo_paginas = 0;
    test_fuzz_real(n / 5);
#ifndef MF_SIN_LISTADO                            /* las direcciones de las variables se leen del listado */
    test_operaciones(n);
    test_funciones(n);
    test_impresion(n / 2);
#endif
    printf("%d pruebas, %d fallos\n", pruebas, fallos);
    return fallos != 0;
}
