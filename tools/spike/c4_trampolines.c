/* c4_trampolines.c - Capitulo 4 (a partir del spike 4): parte RESIDENTE de la paginacion de codigo.
 *
 * 0.11: la MAYORIA de los trampolines t_* (80 de 82) ya no estan aqui sino en e1_tramp.asm: un trampolin generico
 * (__tramp) mas un resguardo de 7 bytes por funcion. Aqui quedan mapear(), banco_actual y los trampolines con logica propia
 * (t_back_generar, t_intr_buscar, t_fuente_iniciar_texto). Ver docs/COORDINACION.md (2026-11-10).
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
uint8_t banco_actual = NINGUNO;      /* idem */

void mapear(uint8_t b)               /* no static: el trampolin generico (e1_tramp.asm) la llama */
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
#ifndef MF_SIN_TEXTO                 /* el .nex integrado trae el ed_linea del editor */
void t_fuente_iniciar(Fuente *f);    /* en e1_tramp.asm */
void t_fuente_iniciar_texto(Fuente *f, const char *texto)
{
    texto_poner(texto);              /* texto.c es RESIDENTE (ed_linea) */
    t_fuente_iniciar(f);
}
#endif

/* Devuelve un puntero a la tabla de cadenas (residente, bss): valido tras
 * desmapear. NO se puede hacer lo mismo con mensajes en RODATA de un banco. */
/* ---- banco 47 (sintaxis) ---- */
