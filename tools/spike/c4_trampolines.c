/* c4_trampolines.c - Capitulo 4 (a partir del spike 4): parte RESIDENTE de la paginacion de codigo.
 *
 * Mismo mecanismo que S3 (sin DI, decision provisional de S3b): cada trampolin
 * guarda el banco que habia, mapea el destino en MMU6/7, llama y restaura.
 * Los t_* son los puntos de entrada a un banco desde fuera de el.
 *
 * REGLA (consecuencia de S4): los punteros que un banco devuelve o guarda NO
 * pueden apuntar a su RODATA (dejaria de estar mapeado). Aqui no ocurre:
 * ningun t_* devuelve punteros.
 */
#include <stdint.h>
#include <arch/zxn.h>
#include "source.h"
#ifndef MF_SIN_TEXTO
#include "texto.h"
#endif
#include "lexer.h"
#include "clasif.h"
#include "ast.h"
#include "expr.h"
#include "stmt.h"
#include "../../compiler/src/ast.c"   /* residente: el pool vive en la pagina de datos AST_PAGINA y lo usan todos los bancos */

#define NINGUNO   0xFF
#define BANCO_LEC 46
#define BANCO_EXP 47

unsigned long c4_mapeos;            /* cuantas veces se ha mapeado un banco */

static uint8_t orig6;
static uint8_t orig7;
static uint8_t capturado;
static uint8_t banco_actual = NINGUNO;

static void mapear(uint8_t b)
{
    if (!capturado) {                /* lo que habia al arrancar */
        orig6 = ZXN_READ_REG(0x56);
        orig7 = ZXN_READ_REG(0x57);
        capturado = 1;
    }
    if (b == NINGUNO) {
        ZXN_WRITE_MMU6(orig6);
        ZXN_WRITE_MMU7(orig7);
    } else {
        ZXN_WRITE_MMU6(2 * b);
        ZXN_WRITE_MMU7(2 * b + 1);
    }
    banco_actual = b;
    c4_mapeos++;
}

/* ---- banco 46 (lectura) ---- */
void t_tabla_iniciar(void)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_LEC); tabla_iniciar(); mapear(ant);
}

void t_fuente_iniciar(Fuente *f)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_LEC); fuente_iniciar(f); mapear(ant);
}

#ifndef MF_SIN_TEXTO                 /* el .nex integrado trae el ed_linea del editor */
void t_fuente_iniciar_texto(Fuente *f, const char *texto)
{
    texto_poner(texto);              /* texto.c es RESIDENTE (ed_linea) */
    t_fuente_iniciar(f);
}
#endif

ErrorFuente t_leer_sentencia(Fuente *f)
{
    uint8_t ant = banco_actual;
    ErrorFuente r;
    mapear(BANCO_LEC); r = leer_sentencia(f); mapear(ant);
    return r;
}

void t_lex_iniciar(void)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_LEC); lex_iniciar(); mapear(ant);
}

void t_lex_siguiente(Token *t)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_LEC); lex_siguiente(t); mapear(ant);
}

void t_clasificar_sentencia(Clasificacion *c)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_LEC); clasificar_sentencia(c); mapear(ant);
}

uint16_t t_lex_saltar_blancos(uint16_t p)
{
    uint8_t ant = banco_actual;
    uint16_t r;
    mapear(BANCO_LEC); r = lex_saltar_blancos(p); mapear(ant);
    return r;
}

/* Devuelve un puntero a la tabla de cadenas (residente, bss): valido tras
 * desmapear. NO se puede hacer lo mismo con mensajes en RODATA de un banco. */
const char *t_tabla_texto(uint16_t offset)
{
    uint8_t ant = banco_actual;
    const char *r;
    mapear(BANCO_LEC); r = tabla_texto(offset); mapear(ant);
    return r;
}

/* ---- banco 47 (sintaxis) ---- */
void t_parser_iniciar(void)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_EXP); parser_iniciar(); mapear(ant);
}

uint16_t t_expr_parsear_completa(void)
{
    uint8_t ant = banco_actual;
    uint16_t r;
    mapear(BANCO_EXP); r = expr_parsear_completa(); mapear(ant);
    return r;
}

void t_stmt_analizar(void)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_EXP); stmt_analizar(); mapear(ant);
}

uint8_t t_stmt_elemento(void)
{
    uint8_t ant = banco_actual;
    uint8_t r;
    mapear(BANCO_EXP); r = stmt_elemento(); mapear(ant);
    return r;
}

void t_stmt_analizar_interna(void)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_EXP); stmt_analizar_interna(); mapear(ant);
}
