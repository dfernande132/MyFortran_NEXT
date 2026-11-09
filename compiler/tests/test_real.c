/* test_real.c - REAL (decision 0013): conversion exacta de constantes decimales a IEEE 754 simple. El oraculo es strtof() de la
 * libreria del PC (correctamente redondeada); el convenio del compilador solo difiere en que no hay denormales (-> 0). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "real.h"

static int pruebas, fallos;

#define COMPROBAR(c) do { pruebas++; if (!(c)) { fallos++; printf("FALLO (linea %d): %s\n", __LINE__, #c); } } while (0)

static uint32_t bits_de(float f)
{
    uint32_t u;

    memcpy(&u, &f, 4);
    return u;
}

/* 1 si coincide con strtof (o con el convenio sin denormales / desbordamiento) */
static int igual(const char *t, int verbose)
{
    uint32_t b = 0xDEADBEEF;
    uint8_t r = real_de_texto(t, &b);
    float f = strtof(t, NULL);
    uint32_t esp;
    uint8_t esp_r = REAL_OK;

    if (isinf(f)) {
        esp_r = REAL_DESBORDA;
        esp = 0;
    } else if (f != 0.0f && fabsf(f) < FLT_MIN) {
        esp = 0;                                       /* denormal -> 0 */
    } else {
        esp = bits_de(f);
    }
    if (r != esp_r || (r == REAL_OK && b != esp)) {
        if (verbose) {
            printf("  '%s': obtenido r=%u %08X, esperado r=%u %08X\n", t, r, (unsigned)b, esp_r, (unsigned)esp);
        }
        return 0;
    }
    return 1;
}

static void casos(void)
{
    static const char *const t[] = {
        "0", "0.0", "1", "1.", ".5", "0.5", "0.1", "0.2", "0.3", "1.5", "2", "3.14159265", "2.718281828459045",
        "100", "1E0", "1E1", "1E3", "1E10", "1E38", "3.4028234E38", "3.4028235E38", "3.4028236E38", "3.4028237E38", "1E39", "9.99E38",
        "1E-1", "1E-5", "1E-10", "1E-37", "1E-38", "1.17549435E-38", "1.1754942E-38", "1E-39", "1E-45", "1E-50", "1E-100",
        "16777216", "16777217", "16777218", "16777219", "33554433", "8388608.5", "8388609.5", "0.000001", "123456789", "1234567890123",
        "0.1234567890123456789012345", "12345678901234567890123456789012345678901234567890", "1E+5", "1.5E-3", "2.5E+2", 
        "00000.00001", "0.00000000000000000000000000000000000000000001", "5E-324", "4.9E-324", "1.00000011920928955078125",
        "1.000000059604644775390625", "1.00000017881393432617187", "1.00000017881393432617188", "16777215.5", "340282346638528859811704183484516925440",
        "340282356779733661637539395458142568447"
    };
    unsigned i;

    for (i = 0; i < sizeof(t) / sizeof(t[0]); i++) {
        COMPROBAR(igual(t[i], 1));
    }
}

/* textos aleatorios: cifras, punto, exponente */
static void aleatorios(unsigned n)
{
    char t[96];
    unsigned i, k, nd, pp;
    int e;

    srand(12345);
    for (i = 0; i < n; i++) {
        char *p = t;

        nd = 1 + (unsigned)(rand() % 22);
        pp = (unsigned)(rand() % (nd + 1));
        for (k = 0; k < nd; k++) {
            if (k == pp) {
                *p++ = '.';
            }
            *p++ = (char)('0' + (k == 0 && rand() % 4 ? 1 + rand() % 9 : rand() % 10));
        }
        if (rand() % 3) {
            e = (rand() % 100) - 55;
            p += sprintf(p, "E%d", e);
        }
        *p = '\0';
        if (!igual(t, 0)) {
            COMPROBAR(igual(t, 1));
        } else {
            pruebas++;
        }
    }
}

/* floats con representacion decimal EXACTA o de 9 cifras: deben volver al mismo float */
static void vuelta(unsigned n)
{
    unsigned i;
    char t[64];

    srand(777);
    for (i = 0; i < n; i++) {
        uint32_t b = ((uint32_t)rand() << 16) ^ (uint32_t)rand();
        float f;
        uint32_t r = 0;

        b = (b & 0x807FFFFFu) | ((uint32_t)(1 + rand() % 253) << 23);
        memcpy(&f, &b, 4);
        snprintf(t, sizeof(t), "%.9E", (double)fabsf(f));
        COMPROBAR(real_de_texto(t, &r) == REAL_OK);
        COMPROBAR(r == (b & 0x7FFFFFFFu));
    }
}

static void enteros(void)
{
    int v;

    for (v = -32768; v <= 32767; v++) {
        float f = (float)v;
        uint32_t b;

        memcpy(&b, &f, 4);
        COMPROBAR(real_de_entero((int16_t)v) == b);
    }
}

int main(void)
{
    enteros();
    casos();
    aleatorios(200000);
    vuelta(100000);
    printf("%d pruebas, %d fallos\n", pruebas, fallos);
    return fallos != 0;
}
