/* expr.c - Ver expr.h.
 *
 * Estructura (de fuera adentro):
 *   expresion()   bucle de precedence climbing sobre los operadores binarios
 *   operando()    signo unario, .NOT. o un primario
 *   primario()    constante, referencia, o expresion entre parentesis
 *   referencia()  NOMBRE, NOMBRE(args) o NOMBRE(args)(rango)
 *   argumentos(), argumento()   la lista entre parentesis
 *
 * Todas comprueban parser.err al entrar o al salir: el primer error gana, se
 * devuelve 0 y la pila de llamadas se deshace sola.
 */
#include "expr.h"

/* El estado del parser es GLOBAL (uno solo en todo el compilador): en el Z80
 * una variable global se direcciona con 3 bytes de codigo, mientras que
 * parser.campo con p en la pila cuesta ~10 y obliga a pasar p en cada llamada. */
Parser parser;

/* ------------------------------------------------------------------
 * Tabla de operadores binarios
 * ------------------------------------------------------------------ */

#define PREC_MIN   1
#define PREC_NOT   4
#define PREC_REL   5
#define PREC_MUL   8
#define PREC_POW   9    /* el unico asociativo por la derecha */

typedef struct {
    uint8_t token;      /* TipoToken */
    uint8_t prec;
    uint8_t nodo;       /* NodoTipo  */
} Operador;

static const Operador OPERADORES[] = {
    { TOK_EQV,      1, NODO_EQV    },
    { TOK_NEQV,     1, NODO_NEQV   },
    { TOK_OR,       2, NODO_OR     },
    { TOK_AND,      3, NODO_AND    },
    { TOK_EQ,       PREC_REL, NODO_EQ },
    { TOK_NE,       PREC_REL, NODO_NE },
    { TOK_LT,       PREC_REL, NODO_LT },
    { TOK_LE,       PREC_REL, NODO_LE },
    { TOK_GT,       PREC_REL, NODO_GT },
    { TOK_GE,       PREC_REL, NODO_GE },
    { TOK_CONCAT,   6, NODO_CONCAT },
    { TOK_MAS,      7, NODO_ADD    },
    { TOK_MENOS,    7, NODO_SUB    },
    { TOK_POR,      PREC_MUL, NODO_MUL },
    { TOK_DIV,      PREC_MUL, NODO_DIV },
    { TOK_POTENCIA, PREC_POW, NODO_POW }
};

#define N_OPERADORES  (sizeof(OPERADORES) / sizeof(OPERADORES[0]))

static const Operador *buscar_operador(TipoToken t)
{
    uint8_t i;

    for (i = 0; i < N_OPERADORES; i++) {
        if (OPERADORES[i].token == (uint8_t)t) {
            return &OPERADORES[i];
        }
    }
    return 0;
}

/* ------------------------------------------------------------------
 * Mensajes
 * ------------------------------------------------------------------ */

const char *expr_mensaje(ErrorExpr e)
{
    switch (e) {
    case EXPR_OK:              return "no error";
    case EXPR_ESPERADA:        return "expression expected";
    case EXPR_PAR_CIERRA:      return "')' expected";
    case EXPR_COMA_O_PAR:      return "',' or ')' expected";
    case EXPR_SIGNO:           return "sign not allowed here; use parentheses";
    case EXPR_NOT:             return ".NOT. not allowed here; use parentheses";
    case EXPR_REL_ENCADENADO:  return "relational operators cannot be chained";
    case EXPR_COMPLEJA:        return "expression too complex";
    case EXPR_POOL_LLENO:      return "expression too large";
    case EXPR_LEXICO:          return "lexical error";
    case EXPR_SOBRA:           return "unexpected token after expression";
    }
    return "unknown error";
}

/* ------------------------------------------------------------------
 * Utilidades
 * ------------------------------------------------------------------ */

/* Anota el primer error y devuelve 0 para poder escribir "return fallo(...)". */
static uint16_t fallo(ErrorExpr e)
{
    if (parser.err == EXPR_OK) {
        parser.err = e;
        parser.err_pos = parser.tok.pos;
    }
    return 0;
}

/* Pasa al token siguiente. Un error del escaner es un error del parser. */
static void avanzar(void)
{
    if (parser.err != EXPR_OK) {
        return;
    }
    lex_siguiente(&parser.tok);
    if (parser.tok.tipo == TOK_ERROR) {
        parser.err_lex = (ErrorLex)parser.tok.valor;
        fallo(EXPR_LEXICO);
    }
}

static uint16_t crear(NodoTipo tipo, uint16_t a, uint16_t b, uint16_t pos)
{
    uint16_t n;

    if (parser.err != EXPR_OK) {
        return 0;
    }
    n = ast_nuevo(tipo, a, b, pos);
    if (n == 0) {
        fallo(EXPR_POOL_LLENO);
    }
    return n;
}

/* Crea una hoja con el token actual y avanza. */
static uint16_t hoja(NodoTipo tipo, uint16_t a, uint16_t b)
{
    uint16_t n = crear(tipo, a, b, parser.tok.pos);

    avanzar();
    return n;
}

/* ------------------------------------------------------------------
 * Gramatica
 * ------------------------------------------------------------------ */

static uint16_t expresion(uint8_t min, uint8_t inicio);

/* argumento := [expr] [ ':' [expr] ]      (lo segundo es un RANGO) */
static uint16_t argumento(void)
{
    uint16_t lo = 0;
    uint16_t hi = 0;
    uint16_t pos = parser.tok.pos;

    if (parser.tok.tipo != TOK_DOS_PUNTOS) {
        lo = expresion(PREC_MIN, 1);
        if (parser.err != EXPR_OK) {
            return 0;
        }
        if (parser.tok.tipo != TOK_DOS_PUNTOS) {
            return lo;
        }
    }
    avanzar();                                   /* el ':' */
    if (parser.tok.tipo != TOK_COMA && parser.tok.tipo != TOK_PAR_CIERRA) {
        hi = expresion(PREC_MIN, 1);
        if (parser.err != EXPR_OK) {
            return 0;
        }
    }
    return crear(NODO_RANGO, lo, hi, pos);
}

/* argumentos := '(' [ argumento { ',' argumento } ] ')'
 * Devuelve la primera celda de la lista (0 si esta vacia o hubo error). */
static uint16_t argumentos(void)
{
    uint16_t primero = 0;
    uint16_t ultimo = 0;
    uint16_t celda;
    uint16_t elem;
    uint16_t pos;

    /* Un nivel de argumentos recorre 3 funciones (referencia, argumentos,
     * argumento) ademas de las de expresion(): en el Next pesa ~74 B frente a
     * ~38 B de un parentesis (medido: 67 B/nivel, c5). Por eso cuenta DOBLE en
     * la profundidad: sin esto, 22 niveles de F(F(...)) desbordaban la pila. */
    if (parser.prof >= EXPR_PROF_MAX) {
        return fallo(EXPR_COMPLEJA);
    }
    parser.prof++;
    avanzar();                                   /* el '(' */
    if (parser.tok.tipo != TOK_PAR_CIERRA) {
        for (;;) {
            pos = parser.tok.pos;
            elem = argumento();
            if (parser.err != EXPR_OK) {
                parser.prof--;
                return 0;
            }
            celda = crear(NODO_LISTA, elem, 0, pos);
            if (parser.err != EXPR_OK) {
                parser.prof--;
                return 0;
            }
            if (ultimo != 0) {
                ast_set_b(ultimo, celda);
            } else {
                primero = celda;
            }
            ultimo = celda;
            if (parser.tok.tipo != TOK_COMA) {
                break;
            }
            avanzar();
        }
    }
    parser.prof--;
    if (parser.tok.tipo != TOK_PAR_CIERRA) {
        return fallo(EXPR_COMA_O_PAR);
    }
    avanzar();
    return primero;
}

/* referencia := NOMBRE [ argumentos [ argumentos ] ] */
static uint16_t referencia(void)
{
    uint16_t nombre = parser.tok.indice;
    uint16_t pos = parser.tok.pos;
    uint16_t args;
    uint16_t n;

    avanzar();
    if (parser.tok.tipo != TOK_PAR_ABRE) {
        return crear(NODO_NOMBRE, nombre, 0, pos);
    }
    args = argumentos();
    n = crear(NODO_REF, nombre, args, pos);
    if (parser.err == EXPR_OK && parser.tok.tipo == TOK_PAR_ABRE) {
        /* elemento de array seguido de subcadena: C(I)(2:3) */
        pos = parser.tok.pos;
        args = argumentos();
        n = crear(NODO_SUBCAD, n, args, pos);
    }
    return n;
}

/* primario := constante | referencia | '(' expresion ')' */
static uint16_t primario(void)
{
    uint16_t n;
    uint16_t pos;

    switch (parser.tok.tipo) {
    case TOK_ENTERO:
        return hoja(NODO_ENTERO, (uint16_t)parser.tok.valor,
                    (uint16_t)(parser.tok.valor >> 16));
    case TOK_REAL:
        return hoja(NODO_REAL, parser.tok.indice, 0);
    case TOK_DOBLE:
        return hoja(NODO_DOBLE, parser.tok.indice, 0);
    case TOK_CADENA:
        return hoja(NODO_CADENA, parser.tok.indice, 0);
    case TOK_LOGICO:
        return hoja(NODO_LOGICO, (uint16_t)parser.tok.valor, 0);
    case TOK_NOMBRE:
        return referencia();
    case TOK_POR:
        if (parser.estrella) {
            return hoja(NODO_ESTRELLA, 0, 0);
        }
        return fallo(EXPR_ESPERADA);
    case TOK_PAR_ABRE:
        pos = parser.tok.pos;
        avanzar();
        n = expresion(PREC_MIN, 1);
        if (parser.err != EXPR_OK) {
            return 0;
        }
        if (parser.tok.tipo != TOK_PAR_CIERRA) {
            return fallo(EXPR_PAR_CIERRA);
        }
        avanzar();
        return crear(NODO_PAREN, n, 0, pos);
    default:
        return fallo(EXPR_ESPERADA);
    }
}

/* operando := '+' termino | '-' termino | '.NOT.' operando-relacional | primario
 *
 * min es la precedencia minima que pidio quien nos llama; la usamos para saber
 * si .NOT. cabe aqui. inicio vale 1 solo al principio de una expresion o de
 * un argumento: es el unico sitio donde el estandar admite un signo. */
static uint16_t operando(uint8_t min, uint8_t inicio)
{
    uint16_t hijo;
    uint16_t pos = parser.tok.pos;
    TipoToken t = parser.tok.tipo;

    if (t == TOK_MAS || t == TOK_MENOS) {
        if (!inicio) {
            return fallo(EXPR_SIGNO);
        }
        avanzar();
        /* El signo abarca todo el termino: -A**2 es -(A**2), -A*B es -(A*B). */
        hijo = expresion(PREC_MUL, 0);
        return crear(t == TOK_MAS ? NODO_POS : NODO_NEG, hijo, 0, pos);
    }
    if (t == TOK_NOT) {
        if (min > PREC_NOT) {
            return fallo(EXPR_NOT);
        }
        avanzar();
        /* .NOT. A .EQ. B  es  .NOT. (A .EQ. B): su operando llega hasta los
         * relacionales inclusive. */
        hijo = expresion(PREC_REL, 0);
        return crear(NODO_NOT, hijo, 0, pos);
    }
    return primario();
}

/* expresion := operando { operador-binario operando }   (precedence climbing)
 *
 * Solo consume operadores con precedencia >= min. Tras un operador de
 * precedencia P, el operando derecho se analiza con min = P + 1 (asociativo
 * por la izquierda: A-B-C = (A-B)-C) o con min = P (por la derecha, solo **:
 * A**B**C = A**(B**C)). */
static uint16_t expresion(uint8_t min, uint8_t inicio)
{
    uint16_t izq;
    uint16_t der;
    uint16_t pos;
    const Operador *op;
    uint8_t rel_visto = 0;

    if (parser.err != EXPR_OK) {
        return 0;
    }
    if (parser.prof >= EXPR_PROF_MAX) {
        return fallo(EXPR_COMPLEJA);
    }
    parser.prof++;

    izq = operando(min, inicio);
    while (parser.err == EXPR_OK) {
        op = buscar_operador(parser.tok.tipo);
        if (op == 0 || op->prec < min) {
            break;
        }
        if (op->prec == PREC_REL) {
            if (rel_visto) {
                fallo(EXPR_REL_ENCADENADO);
                break;
            }
            rel_visto = 1;
        }
        pos = parser.tok.pos;
        avanzar();
        der = expresion(op->prec == PREC_POW ? op->prec : (uint8_t)(op->prec + 1), 0);
        if (parser.err != EXPR_OK) {
            break;
        }
        izq = crear((NodoTipo)op->nodo, izq, der, pos);
    }

    parser.prof--;
    return parser.err == EXPR_OK ? izq : 0;
}

/* ------------------------------------------------------------------
 * Interfaz publica
 * ------------------------------------------------------------------ */

void parser_iniciar(void)
{
    parser.err = EXPR_OK;
    parser.err_lex = LEX_CARACTER_ILEGAL;
    parser.err_pos = 0;
    parser.prof = 0;
    parser.estrella = 0;
    parser.tok.tipo = TOK_FIN;
    parser.tok.pos = 0;
    parser.tok.valor = 0;
    parser.tok.indice = 0;
    avanzar();
}

uint16_t expr_parsear(void)
{
    return expresion(PREC_MIN, 1);
}

uint16_t expr_referencia(void)
{
    if (parser.tok.tipo != TOK_NOMBRE) {
        return fallo(EXPR_ESPERADA);
    }
    return referencia();
}

uint16_t expr_parsear_completa(void)
{
    uint16_t n = expresion(PREC_MIN, 1);

    if (parser.err == EXPR_OK && parser.tok.tipo != TOK_FIN) {
        return fallo(EXPR_SOBRA);
    }
    return n;
}
