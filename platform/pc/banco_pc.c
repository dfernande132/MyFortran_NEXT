/* banco_pc.c - Implementacion de banco.h para PC: un array (ver banco.h). */
#include <assert.h>
#include <string.h>
#include "banco.h"

static uint8_t datos[BANCO_PAGINAS][BANCO_PAGINA_TAM];

void banco_iniciar(void)
{
    memset(datos, 0, sizeof(datos));
}

void banco_leer(uint8_t pag, uint16_t off, void *dst, uint16_t n)
{
    assert(pag < BANCO_PAGINAS && (uint32_t)off + n <= BANCO_PAGINA_TAM);
    memcpy(dst, &datos[pag][off], n);
}

void banco_escribir(uint8_t pag, uint16_t off, const void *src, uint16_t n)
{
    assert(pag < BANCO_PAGINAS && (uint32_t)off + n <= BANCO_PAGINA_TAM);
    memcpy(&datos[pag][off], src, n);
}
