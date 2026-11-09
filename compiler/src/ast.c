/* ast.c - Ver ast.h. El pool esta en la pagina de datos AST_PAGINA: nodo i en el desplazamiento 8 * i
 * (tipo 0, tdato 1, a 2, b 4, pos 6, little-endian). Va en el residente: lo llaman todos los bancos. */
#include <string.h>
#include "ast.h"
#ifdef AST_MEDIR
#include <stdio.h>
#include <stdlib.h>
static unsigned ast_max;
static void ast_fin(void) { fprintf(stderr, "AST nodos maximos usados: %u\n", ast_max); }
#endif

uint16_t ast_n;

uint16_t ast_get(uint16_t i, uint8_t campo)
{
    uint16_t v = 0;

    banco_leer(AST_PAGINA, (uint16_t)((i << 3) + campo), &v, campo < AST_C_A ? 1 : 2);
    return v;
}

void ast_set(uint16_t i, uint8_t campo, uint16_t v)
{
    banco_escribir(AST_PAGINA, (uint16_t)((i << 3) + campo), &v, campo < AST_C_A ? 1 : 2);
}

void ast_iniciar(void)
{
    Nodo n;

    memset(&n, 0, sizeof(n));
    ast_escribir(0, &n);
    ast_n = 1;
}

uint16_t ast_nuevo(NodoTipo tipo, uint16_t a, uint16_t b, uint16_t pos)
{
    Nodo n;
    uint16_t i = ast_n;

    if (i >= AST_NODOS) {
        return 0;
    }
    n.tipo = (uint8_t)tipo;
    n.tdato = 0;
    n.a = a;
    n.b = b;
    n.pos = pos;
    ast_escribir(i, &n);
    ast_n = (uint16_t)(i + 1);
#ifdef AST_MEDIR
    if (ast_max == 0) atexit(ast_fin);
    if (i + 1u > ast_max) ast_max = i + 1u;
#endif
    return i;
}

uint32_t ast_valor(uint16_t i)
{
    return ((uint32_t)ast_get(i, AST_C_B) << 16) | ast_get(i, AST_C_A);
}
