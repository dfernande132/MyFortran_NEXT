/* test_cap02.c - Pruebas del capitulo 2 (analizador de expresiones y AST).
 *
 * Cada prueba analiza un texto y compara el arbol, escrito como expresion-S
 * (prefija, con parentesis), con el esperado. Asi el test se lee como la
 * gramatica: "A+B*C" -> "(+ A (* B C))" dice que * liga mas que +.
 */
#include <stdio.h>
#include <string.h>
#include "source.h"
#include "texto.h"
#include "mf_api.h"
#include "lexer.h"
#include "ast.h"
#include "expr.h"

static int pruebas = 0;
static int fallos = 0;

#define COMPROBAR(cond)                                              \
    do {                                                             \
        pruebas++;                                                   \
        if (!(cond)) {                                               \
            fallos++;                                                \
            printf("FALLO (linea %d): %s\n", __LINE__, #cond);       \
        }                                                            \
    } while (0)

/* ------------------------------------------------------------------
 * Escritor de arboles (solo para pruebas; el nucleo no lo necesita)
 * ------------------------------------------------------------------ */

static char         salida[2048];

static const char *simbolo(uint8_t tipo)
{
    switch (tipo) {
    case NODO_NEG:    return "neg";
    case NODO_POS:    return "pos";
    case NODO_NOT:    return ".NOT.";
    case NODO_ADD:    return "+";
    case NODO_SUB:    return "-";
    case NODO_MUL:    return "*";
    case NODO_DIV:    return "/";
    case NODO_POW:    return "**";
    case NODO_CONCAT: return "//";
    case NODO_EQ:     return ".EQ.";
    case NODO_NE:     return ".NE.";
    case NODO_LT:     return ".LT.";
    case NODO_LE:     return ".LE.";
    case NODO_GT:     return ".GT.";
    case NODO_GE:     return ".GE.";
    case NODO_AND:    return ".AND.";
    case NODO_OR:     return ".OR.";
    case NODO_EQV:    return ".EQV.";
    case NODO_NEQV:   return ".NEQV.";
    }
    return "?";
}

static void escribir(uint16_t n);

static void escribir_hijo_o_guion(uint16_t n)
{
    if (n == 0) {
        strcat(salida, "_");
    } else {
        escribir(n);
    }
}

static void escribir(uint16_t n)
{
    uint16_t nd = n;
    char num[16];
    uint16_t c;

    switch (ast_tipo(nd)) {
    case NODO_ENTERO:
        sprintf(num, "%lu", (unsigned long)ast_valor(nd));
        strcat(salida, num);
        break;
    case NODO_REAL:
    case NODO_DOBLE:
        strcat(salida, tabla_texto(ast_a(nd)));
        break;
    case NODO_CADENA:
        strcat(salida, "'");
        strcat(salida, tabla_texto(ast_a(nd)));
        strcat(salida, "'");
        break;
    case NODO_LOGICO:
        strcat(salida, ast_a(nd) ? ".T." : ".F.");
        break;
    case NODO_NOMBRE:
        strcat(salida, tabla_texto(ast_a(nd)));
        break;
    case NODO_REF:
        strcat(salida, "(ref ");
        strcat(salida, tabla_texto(ast_a(nd)));
        for (c = ast_b(nd); c != 0; c = ast_b(c)) {
            strcat(salida, " ");
            escribir(ast_a(c));
        }
        strcat(salida, ")");
        break;
    case NODO_SUBCAD:
        strcat(salida, "(sub ");
        escribir(ast_a(nd));
        for (c = ast_b(nd); c != 0; c = ast_b(c)) {
            strcat(salida, " ");
            escribir(ast_a(c));
        }
        strcat(salida, ")");
        break;
    case NODO_RANGO:
        strcat(salida, "(: ");
        escribir_hijo_o_guion(ast_a(nd));
        strcat(salida, " ");
        escribir_hijo_o_guion(ast_b(nd));
        strcat(salida, ")");
        break;
    case NODO_PAREN:
        strcat(salida, "(paren ");
        escribir(ast_a(nd));
        strcat(salida, ")");
        break;
    case NODO_NEG:
    case NODO_POS:
    case NODO_NOT:
        strcat(salida, "(");
        strcat(salida, simbolo(ast_tipo(nd)));
        strcat(salida, " ");
        escribir(ast_a(nd));
        strcat(salida, ")");
        break;
    default:
        strcat(salida, "(");
        strcat(salida, simbolo(ast_tipo(nd)));
        strcat(salida, " ");
        escribir(ast_a(nd));
        strcat(salida, " ");
        escribir(ast_b(nd));
        strcat(salida, ")");
        break;
    }
}

/* ------------------------------------------------------------------
 * Ayudas
 * ------------------------------------------------------------------ */

/* Prepara sentencia/lexer/ast/parser para el texto dado. Si pasa de 66 caracteres se
 * reparte en lineas de continuacion (los tests largos no parten cadenas). */
static void preparar(const char *texto)
{
    static char fuente[4096];
    static Fuente f;
    size_t n = strlen(texto);
    size_t pos = 0;
    int primera = 1;

    fuente[0] = '\0';
    do {
        size_t trozo = n - pos > 66 ? 66 : n - pos;

        strcat(fuente, primera ? "      " : "     +");
        strncat(fuente, texto + pos, trozo);
        strcat(fuente, "\n");
        pos += trozo;
        primera = 0;
    } while (pos < n);

    fuente_iniciar_texto(&f, fuente);
    if (leer_sentencia(&f) != FUENTE_OK) {
        iniciar_sentencia("      ", 1);      /* texto vacio: la linea en blanco se ignora */
    }
    tabla_iniciar();
    lex_iniciar();
    ast_iniciar();
    parser_iniciar();
}

/* Analiza el texto completo; devuelve la raiz. El arbol queda en 'salida'. */
static uint16_t analizar(const char *texto)
{
    uint16_t raiz;

    preparar(texto);
    raiz = expr_parsear_completa();
    salida[0] = '\0';
    if (parser.err == EXPR_OK) {
        escribir(raiz);
    }
    return raiz;
}

#define ARBOL(texto, esperado)                                           \
    do {                                                                 \
        analizar(texto);                                                 \
        pruebas++;                                                       \
        if (parser.err != EXPR_OK || strcmp(salida, esperado) != 0) {    \
            fallos++;                                                    \
            printf("FALLO (linea %d): \"%s\"\n   obtenido: %s%s\n"       \
                   "   esperado: %s\n", __LINE__, texto,                 \
                   parser.err == EXPR_OK ? salida : "ERROR: ",           \
                   parser.err == EXPR_OK ? "" : expr_mensaje(parser.err),\
                   esperado);                                            \
        }                                                                \
    } while (0)

/* Espera el error (mensaje exacto) en la posicion bruta pos. */
#define ERROR(texto, mensaje, pos_esp)                                   \
    do {                                                                 \
        uint16_t r_ = analizar(texto);                                   \
        pruebas++;                                                       \
        if (r_ != 0 || parser.err == EXPR_OK ||                          \
            strcmp(expr_mensaje(parser.err), mensaje) != 0 ||            \
            parser.err_pos != (pos_esp)) {                               \
            fallos++;                                                    \
            printf("FALLO (linea %d): \"%s\"\n   obtenido: err=%d \"%s\" pos=%u raiz=%u\n" \
                   "   esperado: \"%s\" pos=%u\n", __LINE__, texto,      \
                   (int)parser.err, expr_mensaje(parser.err),            \
                   (unsigned)parser.err_pos, (unsigned)r_, mensaje,      \
                   (unsigned)(pos_esp));                                 \
        }                                                                \
    } while (0)

/* ------------------------------------------------------------------
 * Pruebas
 * ------------------------------------------------------------------ */

static void test_hojas(void)
{
    ARBOL("42", "42");
    ARBOL("70000", "70000");                 /* no cabe en 16 bits */
    ARBOL("2147483647", "2147483647");
    ARBOL("1.5E3", "1.5E3");
    ARBOL("'AB'", "'AB'");
    ARBOL(".TRUE.", ".T.");
    ARBOL(".FALSE.", ".F.");
    ARBOL("X", "X");
    ARBOL("x", "X");                         /* minusculas */
    ARBOL("A B", "AB");                      /* blancos dentro del nombre */
    ARBOL("  ( 7 )  ", "(paren 7)");
}

static void test_precedencia(void)
{
    ARBOL("A+B*C", "(+ A (* B C))");
    ARBOL("A*B+C", "(+ (* A B) C)");
    ARBOL("A-B-C", "(- (- A B) C)");
    ARBOL("A/B/C", "(/ (/ A B) C)");
    ARBOL("A*B/C", "(/ (* A B) C)");
    ARBOL("A**B**C", "(** A (** B C))");     /* a derechas */
    ARBOL("A*B**C", "(* A (** B C))");
    ARBOL("A**B*C", "(* (** A B) C)");
    ARBOL("A+B//C", "(// (+ A B) C)");       /* // liga menos que + */
    ARBOL("A//B.EQ.C", "(.EQ. (// A B) C)");
    ARBOL("A+B.LT.C*D", "(.LT. (+ A B) (* C D))");
    ARBOL("A.OR.B.AND.C", "(.OR. A (.AND. B C))");
    ARBOL("A.AND.B.OR.C", "(.OR. (.AND. A B) C)");
    ARBOL("A.EQV.B.OR.C", "(.EQV. A (.OR. B C))");
    ARBOL("A.EQV.B.NEQV.C", "(.NEQV. (.EQV. A B) C)");
    ARBOL("X.LT.Y.AND.Y.LT.Z", "(.AND. (.LT. X Y) (.LT. Y Z))");
    ARBOL("A.GE.B.OR.C.LE.D", "(.OR. (.GE. A B) (.LE. C D))");
    ARBOL("A.NE.B", "(.NE. A B)");
    ARBOL("A.GT.B", "(.GT. A B)");
}

static void test_unarios(void)
{
    ARBOL("-A", "(neg A)");
    ARBOL("+A", "(pos A)");
    ARBOL("-A**2", "(neg (** A 2))");        /* el signo abarca el termino */
    ARBOL("-A*B", "(neg (* A B))");
    ARBOL("-A+B", "(+ (neg A) B)");
    ARBOL("-A-B", "(- (neg A) B)");
    ARBOL("-A/B+C", "(+ (neg (/ A B)) C)");
    ARBOL("A*(-B)", "(* A (paren (neg B)))");
    ARBOL("A**(-B)", "(** A (paren (neg B)))");
    ARBOL("(-A)", "(paren (neg A))");
    ARBOL(".NOT.A", "(.NOT. A)");
    ARBOL(".NOT.A.EQ.B", "(.NOT. (.EQ. A B))");   /* abarca el relacional */
    ARBOL(".NOT.A.AND.B", "(.AND. (.NOT. A) B)");
    ARBOL("A.AND..NOT.B", "(.AND. A (.NOT. B))");
    ARBOL("A.OR..NOT.B.AND.C", "(.OR. A (.AND. (.NOT. B) C))");
    ARBOL("A.EQV..NOT.B", "(.EQV. A (.NOT. B))");
    ARBOL(".NOT.(A.AND.B)", "(.NOT. (paren (.AND. A B)))");
}

static void test_parentesis(void)
{
    ARBOL("(A+B)*C", "(* (paren (+ A B)) C)");
    ARBOL("((A))", "(paren (paren A))");
    ARBOL("A*(B+C)*D", "(* (* A (paren (+ B C))) D)");
    ARBOL("A/(B/C)", "(/ A (paren (/ B C)))");
}

static void test_referencias(void)
{
    ARBOL("A(I)", "(ref A I)");
    ARBOL("A(I,J+1)", "(ref A I (+ J 1))");
    ARBOL("F()", "(ref F)");                 /* llamada sin argumentos */
    ARBOL("F(G(X),2)", "(ref F (ref G X) 2)");
    ARBOL("A(I)+B(J)", "(+ (ref A I) (ref B J))");
    ARBOL("X(-1)", "(ref X (neg 1))");       /* el signo vale al inicio de un argumento */
    ARBOL("SIN(-X)*2", "(* (ref SIN (neg X)) 2)");
    ARBOL("A(I,-J,+K)", "(ref A I (neg J) (pos K))");
    ARBOL("F((X))", "(ref F (paren X))");
    ARBOL("C(2:5)", "(ref C (: 2 5))");
    ARBOL("C(:5)", "(ref C (: _ 5))");
    ARBOL("C(2:)", "(ref C (: 2 _))");
    ARBOL("C(:)", "(ref C (: _ _))");
    ARBOL("C(I+1:J-1)", "(ref C (: (+ I 1) (- J 1)))");
    ARBOL("C(I)(2:3)", "(sub (ref C I) (: 2 3))");
    ARBOL("C(2:5)//D(1:1)", "(// (ref C (: 2 5)) (ref D (: 1 1)))");
    ARBOL("A(B(C(D(1))))", "(ref A (ref B (ref C (ref D 1))))");
}

static void test_se_detiene(void)
{
    /* expr_parsear (sin _completa) deja el token que no puede continuarla */
    preparar("A+B,C");
    COMPROBAR(expr_parsear() != 0);
    COMPROBAR(parser.err == EXPR_OK);
    COMPROBAR(parser.tok.tipo == TOK_COMA);
    COMPROBAR(parser.tok.pos == 3);

    preparar("X)");
    COMPROBAR(expr_parsear() != 0);
    COMPROBAR(parser.err == EXPR_OK);
    COMPROBAR(parser.tok.tipo == TOK_PAR_CIERRA);

    /* Varias expresiones en una misma sentencia (p. ej. los limites de un DO) */
    preparar("1,N,2");
    ast_iniciar();
    COMPROBAR(expr_parsear() == 1);
    COMPROBAR(parser.tok.tipo == TOK_COMA);
    lex_siguiente(&parser.tok);               /* el escaner sigue desde donde quedo */
    COMPROBAR(parser.tok.tipo == TOK_NOMBRE);
    COMPROBAR(expr_parsear() == 2);         /* N */
    COMPROBAR(ast_usados() == 3);
}

static void test_errores(void)
{
    ERROR("", "expression expected", 0);
    ERROR("A+", "expression expected", 2);
    ERROR("A+*B", "expression expected", 2);
    ERROR("*A", "expression expected", 0);
    ERROR("A**", "expression expected", 3);
    ERROR("(A+B", "')' expected", 4);
    ERROR("()", "expression expected", 1);
    ERROR("(A))", "unexpected token after expression", 3);
    ERROR("A,B", "unexpected token after expression", 1);
    ERROR("A B)", "unexpected token after expression", 3);
    ERROR("F(1,", "expression expected", 4);
    ERROR("F(1,)", "expression expected", 4);
    ERROR("F(,1)", "expression expected", 2);
    ERROR("F(1,2", "',' or ')' expected", 5);
    ERROR("F(1:", "expression expected", 4);

    /* Signos: solo al principio de una expresion o argumento */
    ERROR("A*-B", "sign not allowed here; use parentheses", 2);
    ERROR("A**-B", "sign not allowed here; use parentheses", 3);
    ERROR("A+-B", "sign not allowed here; use parentheses", 2);
    ERROR("A/+B", "sign not allowed here; use parentheses", 2);
    ERROR("A.EQ.-B", "sign not allowed here; use parentheses", 5);
    ERROR(".NOT.-A", "sign not allowed here; use parentheses", 5);

    /* .NOT. */
    ERROR(".NOT..NOT.A", ".NOT. not allowed here; use parentheses", 5);
    ERROR("A.EQ..NOT.B", ".NOT. not allowed here; use parentheses", 5);
    ERROR("A+.NOT.B", ".NOT. not allowed here; use parentheses", 2);

    /* Relacionales no asociativos */
    ERROR("A.LT.B.LT.C", "relational operators cannot be chained", 6);
    ERROR("A.EQ.B.NE.C", "relational operators cannot be chained", 6);
    ERROR("A+B.GT.C.LE.D", "relational operators cannot be chained", 8);

    /* Errores del escaner llegan como errores del parser */
    ERROR("A#B", "lexical error", 1);
    COMPROBAR(parser.err_lex == LEX_CARACTER_ILEGAL);
    ERROR("'abc", "lexical error", 0);
    COMPROBAR(parser.err_lex == LEX_CADENA_SIN_CERRAR);
    ERROR("A+1.5E", "lexical error", 2);
    COMPROBAR(parser.err_lex == LEX_REAL_MAL_FORMADO);
}

static void test_limites(void)
{
    char buf[1400];
    int i;
    int n;

    /* Anidamiento maximo: cada parentesis cuesta un nivel, y la expresion
     * de fuera otro. Con EXPR_PROF_MAX - 1 parentesis cabe justo. */
    n = EXPR_PROF_MAX - 1;
    for (i = 0; i < n; i++) buf[i] = '(';
    buf[n] = 'A';
    for (i = 0; i < n; i++) buf[n + 1 + i] = ')';
    buf[2 * n + 1] = '\0';
    analizar(buf);
    COMPROBAR(parser.err == EXPR_OK);
    COMPROBAR(parser.prof == 0);             /* la cuenta vuelve a cero */

    n = EXPR_PROF_MAX;
    for (i = 0; i < n; i++) buf[i] = '(';
    buf[n] = 'A';
    for (i = 0; i < n; i++) buf[n + 1 + i] = ')';
    buf[2 * n + 1] = '\0';
    analizar(buf);
    COMPROBAR(parser.err == EXPR_COMPLEJA);

    /* Los argumentos de una referencia cuentan DOBLE (cada nivel de F(F(...)) pesa
     * ~2 parentesis en la pila del Z80): caben (EXPR_PROF_MAX - 2) / 2 niveles. */
    n = (EXPR_PROF_MAX - 2) / 2;
    buf[0] = '\0';
    for (i = 0; i < n; i++) strcat(buf, "F(");
    strcat(buf, "1");
    for (i = 0; i < n; i++) strcat(buf, ")");
    analizar(buf);
    COMPROBAR(parser.err == EXPR_OK);
    COMPROBAR(parser.prof == 0);

    n = (EXPR_PROF_MAX - 2) / 2 + 1;
    buf[0] = '\0';
    for (i = 0; i < n; i++) strcat(buf, "F(");
    strcat(buf, "1");
    for (i = 0; i < n; i++) strcat(buf, ")");
    analizar(buf);
    COMPROBAR(parser.err == EXPR_COMPLEJA);

    /* mezcla: parentesis y referencias comparten el mismo presupuesto:
     * 1 (la expresion) + p + 2k <= EXPR_PROF_MAX */
    for (n = 0; n < 2; n++) {
        int k = 3;
        int p = EXPR_PROF_MAX - 1 - 2 * k + (int)n;   /* 1 + p + 2k = EXPR_PROF_MAX: cabe; con n = 1 no */

        buf[0] = '\0';
        for (i = 0; i < p; i++) strcat(buf, "(");
        for (i = 0; i < k; i++) strcat(buf, "F(");
        strcat(buf, "1");
        for (i = 0; i < k + p; i++) strcat(buf, ")");
        analizar(buf);
        COMPROBAR(n == 0 ? parser.err == EXPR_OK : parser.err == EXPR_COMPLEJA);
    }

    /* Una cadena de ** a derechas tambien recurre: tambien esta acotada */
    buf[0] = '\0';
    for (i = 0; i < EXPR_PROF_MAX + 2; i++) strcat(buf, "A**");
    strcat(buf, "A");
    analizar(buf);
    COMPROBAR(parser.err == EXPR_COMPLEJA);

    /* En cambio una cadena larga a izquierdas es un bucle: no consume profundidad */
    buf[0] = '\0';
    for (i = 0; i < 30; i++) strcat(buf, "A+");
    strcat(buf, "A");
    analizar(buf);
    COMPROBAR(parser.err == EXPR_OK);
    COMPROBAR(ast_usados() == 1 + 31 + 30);       /* 62 de AST_NODOS (64) */

    /* Pool lleno: 100 terminos son 199 nodos (AST_NODOS es 64) */
    buf[0] = '\0';
    for (i = 0; i < 100; i++) strcat(buf, "A+");
    strcat(buf, "A");
    analizar(buf);
    COMPROBAR(parser.err == EXPR_POOL_LLENO);

    /* Tras un error el parser es "pegajoso": el 1.o gana */
    preparar("A+*B#");
    COMPROBAR(expr_parsear_completa() == 0);
    COMPROBAR(parser.err == EXPR_ESPERADA);
}

static void test_ast(void)
{
    uint16_t i;
    uint16_t n;

    ast_iniciar();
    COMPROBAR(ast_usados() == 1);
    COMPROBAR(ast_tipo(0) == NODO_NULO);
    n = ast_nuevo(NODO_ADD, 5, 6, 9);
    COMPROBAR(n == 1);
    COMPROBAR(ast_tipo(1) == NODO_ADD && ast_a(1) == 5 && ast_b(1) == 6
              && ast_pos(1) == 9 && ast_tdato(1) == 0);

    /* Llenado hasta el final: el pool admite AST_NODOS - 1 nodos reales */
    for (i = 2; i < AST_NODOS; i++) {
        COMPROBAR(ast_nuevo(NODO_ENTERO, 0, 0, 0) == i);
    }
    COMPROBAR(ast_nuevo(NODO_ENTERO, 0, 0, 0) == 0);
    ast_iniciar();
    COMPROBAR(ast_nuevo(NODO_ENTERO, 0, 0, 0) == 1);

    /* Entero de 32 bits en dos mitades */
    n = ast_nuevo(NODO_ENTERO, 0x4321, 0x8765, 0);
    COMPROBAR(ast_valor(n) == 0x87654321UL);
}

int main(void)
{
    test_ast();
    test_hojas();
    test_precedencia();
    test_unarios();
    test_parentesis();
    test_referencias();
    test_se_detiene();
    test_errores();
    test_limites();

    printf("%d pruebas, %d fallos\n", pruebas, fallos);
    return fallos != 0;
}
