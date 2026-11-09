/* stmt.h - Analizador sintactico de SENTENCIAS de Fortran 77 (capitulo 4).
 *
 * Para cada sentencia ya leida (source.h) hace tres cosas: la CLASIFICA
 * (clasif.h), comprueba que lo que sigue a la palabra clave tiene la forma que
 * la gramatica de esa clase exige, y deja el resultado en un REGISTRO (Stmt):
 * clase, etiquetas, nombres y raices de arboles de expresion (ast.h).
 *
 * Es SINTAXIS, no semantica: no se mira si un nombre esta declarado, ni de que
 * tipo es, ni si la etiqueta existe. Eso es del capitulo 5.
 *
 * El analizador es PLANO: cada sentencia se analiza aislada. IF-THEN, ELSE,
 * ENDIF, DO y CONTINUE llegan como registros sueltos; emparejarlos es trabajo
 * de la fase siguiente (una pila de bloques). Asi el estado cabe en un registro
 * y no hace falta guardar el programa entero.
 *
 * IF LOGICO ("IF (cond) sentencia"): son dos registros EN SECUENCIA.
 *     stmt_analizar();                       -> tipo = SENT_IF_LOGICO, a = cond
 *     ...el consumidor usa la condicion...
 *     stmt_analizar_interna();               -> la sentencia interior
 * El registro de la interior sustituye al del IF (su condicion sigue valida
 * en el pool del AST hasta la siguiente sentencia). interna = 1 en ese caso.
 *
 * LISTAS EN FLUJO: las declaraciones, las expresiones de PRINT y las etiquetas del
 * GOTO calculado NO se guardan enteras (el AST es pequeno): tras stmt_analizar
 * (o stmt_analizar_interna) se pide un elemento cada vez:
 *     stmt_analizar();
 *     while (stmt_elemento()) { ...usar stm.e (raiz de un arbol)... }
 *     if (stm.err != STM_OK) ...        // un error de la lista sale aqui
 * Cada llamada invalida el arbol del elemento anterior. Quien no recorra la lista
 * no validara el resto de la sentencia. Al acabar, stm.b (GOTO calculado) ya tiene
 * el selector.
 *
 * Contenido de Stmt segun tipo (campos no citados = 0):
 *
 *   PROGRAM        nombre
 *   END ELSE ENDIF CONTINUE   (nada)
 *   INTEGER REAL DOUBLEPRECISION LOGICAL CHARACTER
 *                  elementos: nodos DECL (a = NOMBRE o REF, b = longitud);
 *                  c = longitud por defecto de CHARACTER*n (nodo; 0 si no hay)
 *   ASIGNACION     a = destino (NOMBRE, REF o SUBCAD), b = expresion
 *   PRINT          modo 0: formato '*'; 1: etiqueta (l1); 2: constante de
 *                  texto (a = nodo CADENA). elementos: las expresiones (puede
 *                  no haber ninguna)
 *   GOTO           modo 0: l1 = etiqueta;  modo 1 (calculado): elementos = nodos
 *                  ENTERO (etiquetas); al acabar la lista, b = expresion selectora
 *   STOP           a = nodo ENTERO o CADENA (0 = sin argumento)
 *   DO             l1 = etiqueta final, nombre = variable, a/b/c = inicio, fin,
 *                  paso (c = 0 si no hay paso)
 *   IF_BLOQUE ELSEIF   a = condicion
 *   IF_ARITMETICO  a = expresion, l1/l2/l3 = etiquetas (negativo, cero, positivo)
 *   IF_LOGICO      a = condicion; resto = donde empieza la sentencia interior
 *   VACIA          (solo etiqueta o blancos)
 *
 * Casos que NO son errores aqui, porque el clasificador ya los separa: "DO10I=1"
 * (sin coma) es una asignacion a la variable DO10I; "INTEGER A=1" es una
 * asignacion a INTEGERA; "IF (A)" sin nada detras no se reconoce (DESCONOCIDA).
 * Por eso no hay error de "destino de asignacion invalido" (el clasificador
 * garantiza nombre [parentesis...] '=') ni de "falta la sentencia tras IF".
 *
 * ERRORES: se anota el PRIMERO (err, err_pos = posicion BRUTA). En F77 no hace
 * falta recuperacion: la unidad es la sentencia y la siguiente empieza en otra
 * linea.
 */
#ifndef MF_STMT_H
#define MF_STMT_H

#include <stdint.h>
#include "lexer.h"
#include "clasif.h"
#include "ast.h"
#include "expr.h"

typedef enum {
    STM_OK,
    STM_DESCONOCIDA,          /* "unrecognized or unsupported statement"      */
    STM_NOMBRE_ESPERADO,      /* "name expected"                              */
    STM_IGUAL_ESPERADO,       /* "'=' expected"                               */
    STM_COMA_ESPERADA,        /* "',' expected"                               */
    STM_PAR_ABRE_ESPERADO,    /* "'(' expected"                               */
    STM_PAR_CIERRA_ESPERADO,  /* "')' expected"                               */
    STM_ETIQUETA_ESPERADA,    /* "statement label expected"                   */
    STM_ETIQUETA_RANGO,       /* "label must be 1 to 99999"                   */
    STM_THEN_ESPERADO,        /* "THEN expected"                              */
    STM_SOBRA,                /* "unexpected text at end of statement"        */
    STM_DECLARADOR,           /* "invalid declarator"                         */
    STM_FORMATO,              /* "format must be *, a label or a text constant" */
    STM_LONGITUD,             /* "invalid length specifier"                   */
    STM_NO_EN_IF,             /* "statement not allowed in logical IF"        */
    STM_ARGUMENTO,            /* "invalid STOP argument"                      */
    STM_POOL_LLENO,           /* "statement too large"                        */
    STM_EXPR,                 /* error en una expresion: ver err_expr         */
    STM_LEXICO,               /* error del escaner: ver err_lex               */
    STM_BARRA_ESPERADA,       /* "'/' expected"                               */
    STM_CONSTANTE_ESPERADA,   /* "constant expected"                          */
    STM_ENTRADA               /* "only list-directed READ from the keyboard is supported" */
} ErrorStmt;

typedef struct {
    uint8_t   tipo;         /* TipoSentencia                                  */
    uint8_t   modo;         /* variante (PRINT, GOTO)                         */
    uint8_t   interna;      /* 1 = sentencia interior de un IF logico         */
    uint8_t   err;          /* ErrorStmt                                      */
    uint8_t   err_expr;     /* ErrorExpr, si err == STM_EXPR                  */
    uint8_t   err_lex;      /* ErrorLex, si err == STM_LEXICO                 */
    uint16_t  err_pos;      /* posicion BRUTA del error                       */
    uint16_t  resto;        /* IF logico: posicion de la sentencia interior   */
    uint16_t  nombre;       /* offset en la tabla de cadenas                  */
    uint8_t   ftipo;        /* SENT_FUNCTION: tipo de la cabecera (SENT_INTEGER...) o 0 */
    uint16_t  flen;         /* SENT_FUNCTION CHARACTER*n: la longitud n (0 = no se indico)  */
    uint16_t  a, b, c;      /* raices en stm_ast                              */
    uint16_t  e;            /* elemento actual de la lista (stmt_elemento)    */
    uint32_t  etiqueta;     /* etiqueta de la propia sentencia (0 = ninguna)  */
    uint32_t  l1, l2, l3;   /* etiquetas operando                             */
} Stmt;

/* Registro y AST de la sentencia en curso: UNICOS y globales (decision 0006). */
extern Stmt stm;

/* Mensaje en ingles. Para STM_EXPR usar expr_mensaje(stm.err_expr), y para
 * STM_LEXICO, lex_mensaje(stm.err_lex). */
const char *stmt_mensaje(ErrorStmt e);

/* Analiza la sentencia que acaba de dejar leer_sentencia (global `sentencia`).
 * Inicia el escaner y el AST. Siempre rellena stm; mirar stm.err. */
void stmt_analizar(void);

/* Solo tras un stm.tipo == SENT_IF_LOGICO sin error: analiza la sentencia
 * interior (no reinicia el AST). */
void stmt_analizar_interna(void);

/* Siguiente elemento de la lista de la sentencia (ver LISTAS EN FLUJO).
 * 1 = hay uno, en stm.e; 0 = no hay mas, o hubo un error (mirar stm.err). */
uint8_t stmt_elemento(void);

/* DATA nombres /valores/ {[,] nombres /valores/}: los elementos llegan de uno en uno, y stm.modo dice cual es:
 *   1 = un nombre (stm.e: NOMBRE o REF)   2 = un valor (stm.e: la constante, con NEG si lleva signo; stm.c: el factor de repeticion
 *   (NODO_ENTERO o NOMBRE) o 0)   3 = fin de un grupo "/.../" (el siguiente elemento empieza otro grupo).
 * 1 = hay un elemento; 0 = se acabo la sentencia, o hubo un error (mirar stm.err). */
uint8_t stmt_dato(void);

#endif
