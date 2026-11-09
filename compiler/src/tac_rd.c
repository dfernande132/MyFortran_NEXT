/* tac_rd.c - Lectura del almacen TAC (ver tac.h). Solo usa las paginas de datos y las constantes de tac.h,
 * ningun estado: el backend lo compila dentro de SU banco (el generador solo escribe). */
#include "tac.h"
#include "banco.h"

void tac_leer(uint16_t i, TacInst *t)
{
    banco_leer((uint8_t)(TAC_PAGINA + (i >> 10)),
               (uint16_t)((i & (TAC_POR_PAGINA - 1)) << 3), t, sizeof(*t));
}

uint8_t tac_cte_tipo(uint16_t op)
{
    uint8_t cab[2];

    banco_leer(TAC_PAGINA_POOL, (uint16_t)(TAC_VALOR(op) << 1), cab, 2);
    return cab[0];
}

int16_t tac_cte_entero(uint16_t op)
{
    uint8_t cab[2];
    int16_t v = 0;
    uint16_t off = (uint16_t)(TAC_VALOR(op) << 1);

    banco_leer(TAC_PAGINA_POOL, off, cab, 2);
    if (cab[0] == TAC_C_LOGICO) {
        uint8_t b;

        banco_leer(TAC_PAGINA_POOL, (uint16_t)(off + 2), &b, 1);
        return b;
    }
    banco_leer(TAC_PAGINA_POOL, (uint16_t)(off + 2), &v, 2);
    return v;
}

uint8_t tac_cte_cadena(uint16_t op, char *dst)
{
    uint8_t cab[2];
    uint16_t off = (uint16_t)(TAC_VALOR(op) << 1);

    banco_leer(TAC_PAGINA_POOL, off, cab, 2);
    if (cab[1] != 0) {
        banco_leer(TAC_PAGINA_POOL, (uint16_t)(off + 2), dst, cab[1]);
    }
    dst[cab[1]] = '\0';
    return cab[1];
}

uint32_t tac_cte_real(uint16_t op)
{
    uint8_t b[4];

    banco_leer(TAC_PAGINA_POOL, (uint16_t)((TAC_VALOR(op) << 1) + 2), b, 4);
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}
