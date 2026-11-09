/* tac.c - Almacen del codigo de tres direcciones (ver tac.h). */
#include "tac.h"
#include "banco.h"

uint16_t tac_n;                /* instrucciones emitidas (declaradas en tac.h)  */
uint16_t tac_pool_n;           /* bytes del pool ocupados (siempre par)         */
uint8_t  tac_lle;

void tac_iniciar(void)
{
    tac_n = 0;
    tac_pool_n = 0;
    tac_lle = 0;
}

void tac_emitir(uint8_t op, uint8_t tipo, uint16_t a, uint16_t b, uint16_t c)
{
    TacInst t;

    if (tac_lle) {
        return;
    }
    if (tac_n >= TAC_MAX) {
        tac_lle = 1;
        return;
    }
    t.op = op;
    t.tipo = tipo;
    t.a = a;
    t.b = b;
    t.c = c;
    banco_escribir((uint8_t)(TAC_PAGINA + (tac_n >> 10)),
                   (uint16_t)((tac_n & (TAC_POR_PAGINA - 1)) << 3), &t, sizeof(t));
    tac_n++;
}

/* Entrada del pool: tipo (1 B), longitud (1 B), datos, relleno a par. El valor del
 * operando CTE es el desplazamiento / 2. 0 si no cabe (y marca "tac_lle"). */
static uint16_t cte_nueva(uint8_t tipo, const void *datos, uint8_t len)
{
    uint8_t cab[2];
    uint16_t tam = (uint16_t)((2u + len + 1u) & ~1u);
    uint16_t off = tac_pool_n;
    uint8_t cero = 0;

    if (tac_lle) {
        return 0;
    }
    if (off + tam > BANCO_PAGINA_TAM) {
        tac_lle = 1;
        return 0;
    }
    cab[0] = tipo;
    cab[1] = len;
    banco_escribir(TAC_PAGINA_POOL, off, cab, 2);
    if (len != 0) {
        banco_escribir(TAC_PAGINA_POOL, (uint16_t)(off + 2), datos, len);
    }
    if (len & 1) {
        banco_escribir(TAC_PAGINA_POOL, (uint16_t)(off + 2 + len), &cero, 1);
    }
    tac_pool_n = (uint16_t)(off + tam);
    return TAC_OPER(TAC_CTE, off >> 1);
}

uint16_t tac_entero(int16_t v)
{
    if (v >= -4096 && v <= 4095) {
        return TAC_OPER(TAC_INM, (uint16_t)v);
    }
    return cte_nueva(TAC_C_ENTERO, &v, 2);
}

uint16_t tac_logico(uint8_t v)
{
    uint8_t b = v ? 1 : 0;

    return cte_nueva(TAC_C_LOGICO, &b, 1);
}

uint16_t tac_cadena(const char *s, uint8_t len)
{
    return cte_nueva(TAC_C_CADENA, s, len);
}


uint16_t tac_real(uint32_t bits)
{
    uint8_t b[4];

    b[0] = (uint8_t)bits;
    b[1] = (uint8_t)(bits >> 8);
    b[2] = (uint8_t)(bits >> 16);
    b[3] = (uint8_t)(bits >> 24);
    return cte_nueva(TAC_C_REAL, b, 4);
}
