/* test_cap03.c - Pruebas del capitulo 3 (arquitectura): la API banco.h en PC.
 * En el Next la misma API se prueba con tools/spike/s1_banco.c. */
#include <stdio.h>
#include <string.h>
#include "banco.h"

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

static void test_banco(void)
{
    uint8_t a[16];
    uint8_t b[16];
    uint8_t p;
    uint16_t i;

    banco_iniciar();

    /* Lo recien iniciado esta a cero */
    banco_leer(0, 0, a, 16);
    for (i = 0; i < 16; i++) COMPROBAR(a[i] == 0);

    /* Ida y vuelta */
    for (i = 0; i < 16; i++) a[i] = (uint8_t)(i + 1);
    banco_escribir(0, 100, a, 16);
    memset(b, 0xFF, 16);
    banco_leer(0, 100, b, 16);
    COMPROBAR(memcmp(a, b, 16) == 0);

    /* Las paginas son independientes */
    for (p = 1; p < BANCO_PAGINAS; p++) {
        banco_leer(p, 100, b, 16);
        for (i = 0; i < 16; i++) COMPROBAR(b[i] == 0);
    }
    for (i = 0; i < 16; i++) a[i] = 0xAA;
    banco_escribir(BANCO_PAGINAS - 1, 100, a, 16);
    banco_leer(0, 100, b, 16);
    COMPROBAR(b[0] == 1 && b[15] == 16);

    /* Limites: primer y ultimo byte de una pagina */
    a[0] = 0x5A;
    banco_escribir(2, 0, a, 1);
    banco_escribir(2, BANCO_PAGINA_TAM - 1, a, 1);
    banco_leer(2, 0, b, 1);
    COMPROBAR(b[0] == 0x5A);
    banco_leer(2, BANCO_PAGINA_TAM - 1, b, 1);
    COMPROBAR(b[0] == 0x5A);

    /* Pagina entera de una vez */
    {
        static uint8_t gran[BANCO_PAGINA_TAM];
        static uint8_t gran2[BANCO_PAGINA_TAM];

        for (i = 0; i < BANCO_PAGINA_TAM; i++) gran[i] = (uint8_t)(i * 7);
        banco_escribir(1, 0, gran, BANCO_PAGINA_TAM);
        banco_leer(1, 0, gran2, BANCO_PAGINA_TAM);
        COMPROBAR(memcmp(gran, gran2, BANCO_PAGINA_TAM) == 0);
    }
}

int main(void)
{
    test_banco();
    printf("%d pruebas, %d fallos\n", pruebas, fallos);
    return fallos != 0;
}
