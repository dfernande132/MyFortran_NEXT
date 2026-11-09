/* ast.h - Arbol sintactico (AST) en un pool de tamano fijo.
 *
 * Reglas del nucleo: sin malloc. Todos los nodos de UNA sentencia viven en
 * un array fijo (el "pool"), y se refieren unos a otros por INDICE de 16 bits,
 * no por puntero. Ventajas: un nodo ocupa 8 bytes, el arbol se copia o se
 * mueve de banco sin recolocar nada, y liberar una sentencia entera es
 * poner un contador a 1 (ast_iniciar).
 *
 * El indice 0 es el nodo NULO: "no hay hijo". Por eso el primer nodo real
 * es el 1.
 *
 * Significado de los campos segun el tipo de nodo:
 *
 *   ENTERO       a = 16 bits bajos, b = 16 bits altos   (ast_valor_entero)
 *   REAL, DOBLE  a = indice del texto en la TablaCadenas
 *   CADENA       a = indice del texto en la TablaCadenas
 *   LOGICO       a = 1 (.TRUE.) o 0 (.FALSE.)
 *   NOMBRE       a = indice del nombre en la TablaCadenas   (variable sola)
 *   REF          a = indice del nombre, b = lista de argumentos
 *                (A(I), F(X,Y) o C(2:5): lo que sea, lo decide el semantico)
 *   SUBCAD       a = nodo REF, b = lista con UN RANGO:  C(I)(2:3)
 *   LISTA        a = elemento, b = siguiente celda (0 = fin)
 *   RANGO        a = limite inferior (0 = falta), b = superior (0 = falta)
 *   DECL         a = nodo NOMBRE o REF (A, B(10), C(0:9,2)), b = longitud de
 *                CHARACTER (0 = la de la sentencia o 1)
 *   PAREN        a = expresion entre parentesis (se conserva a proposito:
 *                F((X)) pasa un valor, F(X) pasa una variable)
 *   NEG POS NOT  a = operando
 *   resto        a = operando izquierdo, b = derecho
 *   ARGF SFUN    ver NodoTipo (los crea sem.c al expandir F(X) = expr)
 *
 * pos es la posicion BRUTA en Sentencia.texto del token que origino el nodo,
 * para los mensajes de error del semantico (posicion_fuente la convierte).
 * tdato lo rellenara el analisis semantico (tipo de dato del resultado).
 */
#ifndef MF_AST_H
#define MF_AST_H

#include <stdint.h>
#include "banco.h"

typedef enum {
    NODO_NULO = 0,

    /* Hojas */
    NODO_ENTERO, NODO_REAL, NODO_DOBLE, NODO_CADENA, NODO_LOGICO, NODO_NOMBRE,

    /* Referencias y listas */
    NODO_REF, NODO_SUBCAD, NODO_LISTA, NODO_RANGO, NODO_PAREN,

    /* Solo en declaraciones (stmt.c) */
    NODO_DECL,

    /* Unarios */
    NODO_NEG, NODO_POS, NODO_NOT,

    /* Binarios aritmeticos y de texto */
    NODO_ADD, NODO_SUB, NODO_MUL, NODO_DIV, NODO_POW, NODO_CONCAT,

    /* Relacionales */
    NODO_EQ, NODO_NE, NODO_LT, NODO_LE, NODO_GT, NODO_GE,

    /* Logicos */
    NODO_AND, NODO_OR, NODO_EQV, NODO_NEQV,

    /* Solo los crea el semantico (funciones de sentencia, decision 0014 S2c) */
    NODO_ARGF,      /* a = numero (0..) del ficticio de la funcion de sentencia que se esta expandiendo */
    NODO_SFUN,      /* referencia expandida: a = raiz del cuerpo copiado, b = lista de argumentos reales */
    NODO_ESTRELLA   /* "*" en una declaracion: A(*) (ultima dimension) o CHARACTER*(*) (longitud supuesta); solo en declaraciones */
} NodoTipo;

typedef struct {
    uint8_t  tipo;      /* NodoTipo                                          */
    uint8_t  tdato;     /* tipo de dato del resultado; 0 hasta el semantico  */
    uint16_t a;
    uint16_t b;
    uint16_t pos;
} Nodo;

#define AST_NODOS 64     /* nodos por sentencia (512 B); se ajusta midiendo */

/* El pool NO vive en el residente (ahorra 514 B de bss en el Next): esta en la pagina de datos AST_PAGINA (la de la imagen del programa
 * generado, que no se usa hasta que acaba el frontal). Los nodos se leen y escriben por campos; no hay punteros a nodos. */
#define AST_PAGINA 11

/* Vacia el pool (los indices anteriores dejan de valer). */
void     ast_iniciar(void);

/* Crea un nodo. Devuelve su indice, o 0 si el pool esta lleno. */
uint16_t ast_nuevo(NodoTipo tipo, uint16_t a, uint16_t b, uint16_t pos);

/* Campos de un nodo: ast_get/ast_set con el desplazamiento del campo (solo dos funciones residentes, el resto son macros). */
#define AST_C_TIPO  0
#define AST_C_TDATO 1
#define AST_C_A     2
#define AST_C_B     4
#define AST_C_POS   6
uint16_t ast_get(uint16_t i, uint8_t campo);
void     ast_set(uint16_t i, uint8_t campo, uint16_t v);

#define ast_tipo(i)        ((uint8_t)ast_get((i), AST_C_TIPO))
#define ast_tdato(i)       ((uint8_t)ast_get((i), AST_C_TDATO))
#define ast_a(i)           ast_get((i), AST_C_A)
#define ast_b(i)           ast_get((i), AST_C_B)
#define ast_pos(i)         ast_get((i), AST_C_POS)
#define ast_set_tipo(i, v)  ast_set((i), AST_C_TIPO, (v))
#define ast_set_tdato(i, v) ast_set((i), AST_C_TDATO, (v))
#define ast_set_a(i, v)     ast_set((i), AST_C_A, (v))
#define ast_set_b(i, v)     ast_set((i), AST_C_B, (v))

/* Siguiente indice libre (el 0 esta reservado) y recorte del pool (libera los nodos de un elemento de lista). */
extern uint16_t ast_n;
#define ast_usados()       (ast_n)
#define ast_recortar(v)    (ast_n = (uint16_t)(v))

/* Nodo entero (8 B, el mismo formato que la pagina). */
#define ast_leer(i, p)      banco_leer(AST_PAGINA, (uint16_t)((i) << 3), (p), sizeof(Nodo))
#define ast_escribir(i, p)  banco_escribir(AST_PAGINA, (uint16_t)((i) << 3), (p), sizeof(Nodo))

/* Valor de un nodo ENTERO. */
uint32_t ast_valor(uint16_t i);

#endif
