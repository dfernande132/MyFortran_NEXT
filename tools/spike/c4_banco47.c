/* c4_banco47.c - Capitulo 4: FASE "analisis sintactico" en el banco 47 =
 * ast + expr + stmt.
 *
 * Lo que estos modulos llaman y vive en el banco 46 (lexer, clasificador) se
 * redirige con #define a trampolines residentes t_* (c4_trampolines.c), ANTES
 * de incluir las cabeceras. El codigo del compilador no se modifica.
 */
#pragma bank 47

#define lex_siguiente         t_lex_siguiente
#define lex_iniciar           t_lex_iniciar
#define lex_saltar_blancos    t_lex_saltar_blancos
#define tabla_texto           t_tabla_texto
#define clasificar_sentencia  t_clasificar_sentencia

#include "../../compiler/src/expr.c"
#include "../../compiler/src/stmt.c"
