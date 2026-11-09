/* expr.h - Analizador sintactico de EXPRESIONES de Fortran 77.
 *
 * Tecnica: descenso recursivo con "precedence climbing". Una sola funcion
 * (expresion, en expr.c) maneja todos los operadores binarios consultando una
 * tabla de precedencias; los operandos (numeros, nombres, parentesis, signos,
 * .NOT.) los resuelve otra funcion. Anadir un operador = anadir una fila.
 *
 * Precedencias (de menor a mayor):
 *   1 .EQV. .NEQV.   2 .OR.   3 .AND.   4 .NOT. (unario)
 *   5 relacionales (NO asociativos)   6 //   7 + -   8 * /   9 ** (a derechas)
 *
 * Reglas del estandar que se aplican (ANSI X3.9-1978, cap. 6):
 *   - un signo unario solo puede ir AL PRINCIPIO de una expresion o tras "(" o
 *     ",": A*-B y A**-B son errores; hay que escribir A*(-B). El signo abarca
 *     todo el termino: -A**2 es -(A**2) y -A*B es -(A*B).
 *   - .NOT. solo va donde cabe un operando de .AND.: "A .AND. .NOT. B" si,
 *     "A .EQ. .NOT. B" y ".NOT. .NOT. A" no.
 *   - A .LT. B .LT. C es un error (no asociativos).
 *
 * Diseno para el Z80: la recursion se acota con un contador de profundidad
 * (EXPR_PROF_MAX). Si se supera es un ERROR ("expression too complex"), no
 * un desbordamiento de pila. El pool del AST tambien es finito.
 *
 * NOTA: A(I) puede ser un elemento de array, una llamada a funcion o una
 * funcion-sentencia; el parser no puede saberlo (necesita la tabla de
 * simbolos) y lo representa siempre como NODO_REF. Lo resuelve el semantico.
 */
#ifndef MF_EXPR_H
#define MF_EXPR_H

#include <stdint.h>
#include "lexer.h"
#include "ast.h"

#define EXPR_PROF_MAX 12    /* presupuesto de profundidad: 1 por expresion()/parentesis y 2 por nivel de
                               argumentos F(...) (pesan el doble en la pila); medido en el Next */

typedef enum {
    EXPR_OK,
    EXPR_ESPERADA,          /* "expression expected"                        */
    EXPR_PAR_CIERRA,        /* "')' expected"                               */
    EXPR_COMA_O_PAR,        /* "',' or ')' expected"                        */
    EXPR_SIGNO,             /* signo unario donde no se admite              */
    EXPR_NOT,               /* .NOT. donde no se admite                     */
    EXPR_REL_ENCADENADO,    /* A .LT. B .LT. C                              */
    EXPR_COMPLEJA,          /* se supero EXPR_PROF_MAX                      */
    EXPR_POOL_LLENO,        /* no caben mas nodos                           */
    EXPR_LEXICO,            /* error del escaner; ver Parser.err_lex        */
    EXPR_SOBRA              /* quedan tokens tras la expresion              */
} ErrorExpr;

typedef struct {
    Token      tok;         /* token actual (lookahead de uno)               */
    ErrorExpr  err;         /* EXPR_OK mientras todo vaya bien; el 1.o gana  */
    ErrorLex   err_lex;     /* valido si err == EXPR_LEXICO                  */
    uint16_t   err_pos;     /* posicion BRUTA del token donde fallo          */
    uint8_t    prof;        /* profundidad actual                            */
    uint8_t    estrella;    /* 1 = un "*" es un primario (declaraciones: A(*), CHARACTER*(*)) */
} Parser;

/* El estado del parser. Es una variable global unica: se lee desde fuera
 * (err, err_pos, tok) tras llamar a expr_parsear. */
extern Parser parser;

/* Mensaje en ingles. Para EXPR_LEXICO usar lex_mensaje(p->err_lex). */
const char *expr_mensaje(ErrorExpr e);

/* Prepara el parser y lee el primer token desde lexer.pos (quien llama lo habra
 * colocado, p. ej. tras el "=" de una asignacion). NO vacia el AST: una
 * sentencia puede tener varias expresiones (ast_iniciar lo hace quien llama). */
void parser_iniciar(void);

/* Analiza UNA expresion y devuelve el indice de su nodo raiz (0 si hubo error;
 * mirar parser.err). Se detiene en el primer token que no puede continuarla y lo
 * deja en parser.tok: asi un parser de sentencias puede seguir (una coma, un ")"). */
uint16_t expr_parsear(void);

/* Igual, pero exige que no quede nada tras la expresion (EXPR_SOBRA). */
uint16_t expr_parsear_completa(void);

/* Analiza SOLO una referencia NOMBRE[(args)[(args)]] (sin operadores): los destinos de DATA, donde "/" no es una division. */
uint16_t expr_referencia(void);

#endif
