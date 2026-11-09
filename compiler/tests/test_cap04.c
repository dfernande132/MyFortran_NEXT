/* test_cap04.c - Pruebas del capitulo 4 (analizador de sentencias).
 *
 * Cada prueba analiza UNA sentencia y compara el registro, escrito como
 * expresion-S, con el esperado: "X = 1" -> "(asig X 1)". Los errores se
 * comprueban por codigo y por posicion BRUTA (offset en Sentencia.texto).
 * Las expresiones (capitulo 2) ya estan probadas aparte: aqui se comprueba la
 * ARMADURA de cada sentencia (comas, parentesis, etiquetas, palabras).
 */
#include <stdio.h>
#include <string.h>
#include "source.h"
#include "texto.h"
#include "mf_api.h"
#include "lexer.h"
#include "ast.h"
#include "expr.h"
#include "stmt.h"

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
 * Escritor de registros (solo para pruebas)
 * ------------------------------------------------------------------ */

static char salida[4096];

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


/* Texto de la lista de la sentencia (declaradores, expresiones de PRINT,
 * etiquetas de GOTO calculado), recogido con stmt_elemento(). */
static char lista_txt[2048];
static unsigned max_nodos;        /* maximo de ast_usados() visto en un elemento */

/* Recorre la lista en flujo. Hay que llamarla tras stmt_analizar() y tras
 * stmt_analizar_interna(), ANTES de mirar stm.err: el error de un elemento
 * (o del cierre) solo aparece al avanzar. */
static void drenar(void)
{
    static char guardado[4096];

    lista_txt[0] = '\0';
    if (stm.err != STM_OK) {
        return;
    }
    strcpy(guardado, salida);
    while (stmt_elemento()) {
        uint16_t d = stm.e;

        if (ast_usados() > max_nodos) {
            max_nodos = ast_usados();
        }
        salida[0] = '\0';
        if (stm.tipo == SENT_PRINT || stm.tipo == SENT_GOTO) {
            strcat(salida, " ");
            escribir(stm.e);
        } else {
            strcat(salida, " (d ");
            escribir(ast_a(d));
            if (ast_b(d) != 0) {
                strcat(salida, " len=");
                escribir(ast_b(d));
            }
            strcat(salida, ")");
        }
        strcat(lista_txt, salida);
    }
    strcpy(salida, guardado);
}

static void num(uint32_t v)
{
    char t[16];

    sprintf(t, "%lu", (unsigned long)v);
    strcat(salida, t);
}

/* Escribe el registro actual (stm) a continuacion en 'salida'. */
static void escribir_stm(void)
{
    static const char *const nombre_tipo[] = {
        "?", "vacia", "asig", "program", "end", "subroutine", "call", "return", "integer", "real",
        "doubleprecision", "logical", "character", "print", "continue", "goto",
        "stop", "do", "if", "if-arit", "if-log", "elseif", "else", "endif",
        "dimension", "external", "intrinsic", "parameter", "data", "read", "function"
    };

    if (stm.etiqueta != 0) {
        num(stm.etiqueta);
        strcat(salida, ": ");
    }
    strcat(salida, "(");
    strcat(salida, nombre_tipo[stm.tipo]);
    switch (stm.tipo) {
    case SENT_PROGRAM:
        strcat(salida, " ");
        strcat(salida, tabla_texto(stm.nombre));
        break;
    case SENT_INTEGER: case SENT_REAL: case SENT_DOUBLEPRECISION:
    case SENT_LOGICAL: case SENT_CHARACTER:
        if (stm.c != 0) {
            strcat(salida, " len=");
            escribir(stm.c);
        }
        strcat(salida, lista_txt);
        break;
    case SENT_ASIGNACION:
        strcat(salida, " ");
        escribir(stm.a);
        strcat(salida, " ");
        escribir(stm.b);
        break;
    case SENT_PRINT:
        strcat(salida, " ");
        if (stm.modo == 0) {
            strcat(salida, "*");
        } else if (stm.modo == 1) {
            num(stm.l1);
        } else {
            escribir(stm.a);
        }
        strcat(salida, lista_txt);
        break;
    case SENT_STOP:
        if (stm.a != 0) {
            strcat(salida, " ");
            escribir(stm.a);
        }
        break;
    case SENT_GOTO:
        if (stm.modo == 0) {
            strcat(salida, " ");
            num(stm.l1);
        } else {
            strcat(salida, "-calc (");
            strcat(salida, lista_txt);
            strcat(salida, ") ");
            escribir(stm.b);
        }
        break;
    case SENT_DO:
        strcat(salida, " ");
        num(stm.l1);
        strcat(salida, " ");
        strcat(salida, tabla_texto(stm.nombre));
        strcat(salida, " ");
        escribir(stm.a);
        strcat(salida, " ");
        escribir(stm.b);
        if (stm.c != 0) {
            strcat(salida, " ");
            escribir(stm.c);
        }
        break;
    case SENT_IF_BLOQUE: case SENT_ELSEIF: case SENT_IF_LOGICO:
        strcat(salida, " ");
        escribir(stm.a);
        break;
    case SENT_IF_ARITMETICO:
        strcat(salida, " ");
        escribir(stm.a);
        strcat(salida, " ");
        num(stm.l1);
        strcat(salida, " ");
        num(stm.l2);
        strcat(salida, " ");
        num(stm.l3);
        break;
    default:
        break;
    }
    strcat(salida, ")");
}

/* ------------------------------------------------------------------
 * Ayudas
 * ------------------------------------------------------------------ */

/* Deja en 'sentencia' el texto dado, con etiqueta (0 = ninguna). Si pasa de 66
 * caracteres se reparte en lineas de continuacion. */
static void preparar(uint32_t etiq, const char *texto)
{
    static char fuente[4096];
    static Fuente f;
    size_t n = strlen(texto);
    size_t pos = 0;
    int primera = 1;
    char cab[16];

    fuente[0] = '\0';
    do {
        size_t trozo = n - pos > 66 ? 66 : n - pos;

        if (primera && etiq != 0) {
            sprintf(cab, "%5lu ", (unsigned long)etiq);
            strcat(fuente, cab);
        } else {
            strcat(fuente, primera ? "      " : "     +");
        }
        strncat(fuente, texto + pos, trozo);
        strcat(fuente, "\n");
        pos += trozo;
        primera = 0;
    } while (pos < n);

    fuente_iniciar_texto(&f, fuente);
    if (leer_sentencia(&f) != FUENTE_OK) {
        iniciar_sentencia(etiq != 0 ? "   10 " : "      ", 1);   /* vacia */
        sentencia.etiqueta = etiq;
    }
    tabla_iniciar();
}

static void analizar(uint32_t etiq, const char *texto)
{
    preparar(etiq, texto);
    stmt_analizar();
    drenar();
    salida[0] = '\0';
    if (stm.err == STM_OK) {
        escribir_stm();
    }
}

#define COMPARAR(texto, esperado)                                        \
    do {                                                                 \
        pruebas++;                                                       \
        if (stm.err != STM_OK || strcmp(salida, esperado) != 0) {        \
            fallos++;                                                    \
            printf("FALLO (linea %d): \"%s\"\n   obtenido: %s%s\n"       \
                   "   esperado: %s\n", __LINE__, texto,                 \
                   stm.err == STM_OK ? salida : "ERROR: ",               \
                   stm.err == STM_OK ? "" : stmt_mensaje((ErrorStmt)stm.err), \
                   esperado);                                            \
        }                                                                \
    } while (0)

#define SENT(texto, esperado)                                            \
    do { analizar(0, texto); COMPARAR(texto, esperado); } while (0)

#define SENTL(etiq, texto, esperado)                                     \
    do { analizar(etiq, texto); COMPARAR(texto, esperado); } while (0)

/* Espera un error de sentencia (codigo) en la posicion bruta pos. */
#define ERR(texto, codigo, pos_esp)                                      \
    do {                                                                 \
        analizar(0, texto);                                              \
        pruebas++;                                                       \
        if (stm.err != (codigo) || stm.err_pos != (pos_esp)) {           \
            fallos++;                                                    \
            printf("FALLO (linea %d): \"%s\"\n   obtenido: err=%d pos=%u (%s)\n" \
                   "   esperado: err=%d pos=%u\n", __LINE__, texto,       \
                   (int)stm.err, (unsigned)stm.err_pos,                  \
                   stmt_mensaje((ErrorStmt)stm.err),                     \
                   (int)(codigo), (unsigned)(pos_esp));                  \
        }                                                                \
    } while (0)

/* Idem, para un error que viene de una expresion (codigo de expr.h). */
#define ERR_EXPR(texto, codigo, pos_esp)                                 \
    do {                                                                 \
        analizar(0, texto);                                              \
        pruebas++;                                                       \
        if (stm.err != STM_EXPR || stm.err_expr != (codigo)              \
            || stm.err_pos != (pos_esp)) {                               \
            fallos++;                                                    \
            printf("FALLO (linea %d): \"%s\"\n   obtenido: err=%d expr=%d pos=%u\n" \
                   "   esperado: expr=%d pos=%u\n", __LINE__, texto,      \
                   (int)stm.err, (int)stm.err_expr, (unsigned)stm.err_pos, \
                   (int)(codigo), (unsigned)(pos_esp));                  \
        }                                                                \
    } while (0)

/* IF logico: el registro del IF, " | " y el de la sentencia interior. */
#define IFLOG(texto, esperado)                                           \
    do {                                                                 \
        analizar(0, texto);                                              \
        if (stm.err == STM_OK) {                                         \
            stmt_analizar_interna();                                     \
            drenar();                                                    \
            if (stm.err == STM_OK) {                                     \
                strcat(salida, " | ");                                   \
                escribir_stm();                                          \
            }                                                            \
        }                                                                \
        COMPARAR(texto, esperado);                                       \
    } while (0)

/* Error en la sentencia interior de un IF logico. */
#define ERR_INTERNA(texto, codigo, pos_esp)                              \
    do {                                                                 \
        analizar(0, texto);                                              \
        pruebas++;                                                       \
        if (stm.err == STM_OK) {                                         \
            stmt_analizar_interna();                                     \
            drenar();                                                    \
        }                                                                \
        if (stm.err != (codigo) || stm.err_pos != (pos_esp)) {           \
            fallos++;                                                    \
            printf("FALLO (linea %d): \"%s\"\n   obtenido: err=%d pos=%u (%s)\n" \
                   "   esperado: err=%d pos=%u\n", __LINE__, texto,       \
                   (int)stm.err, (unsigned)stm.err_pos,                  \
                   stmt_mensaje((ErrorStmt)stm.err),                     \
                   (int)(codigo), (unsigned)(pos_esp));                  \
        }                                                                \
    } while (0)


/* ------------------------------------------------------------------
 * Pruebas
 * ------------------------------------------------------------------ */

static void test_simples(void)
{
    SENT("CONTINUE", "(continue)");
    SENT("END", "(end)");
    SENT("ELSE", "(else)");
    SENT("ENDIF", "(endif)");
    SENT("END IF", "(endif)");
    SENTL(10, "CONTINUE", "10: (continue)");
    SENTL(99999, "END", "99999: (end)");
    SENT("PROGRAM HOLA", "(program HOLA)");
    SENT("program Hola", "(program HOLA)");
    SENT("PROGRAM A B", "(program AB)");        /* los blancos no cuentan */

    /* Vacia: solo etiqueta, o blancos */
    SENT("", "(vacia)");
    SENT("   ", "(vacia)");
    SENTL(10, "", "10: (vacia)");

    /* Sobra algo */
    ERR("CONTINUE X", STM_SOBRA, 9);
    ERR("END 5", STM_SOBRA, 4);
    ERR("ELSE X", STM_SOBRA, 5);
    SENT("PROGRAM HOLA X", "(program HOLAX)"); /* HOLAX es UN nombre */
}

static void test_program(void)
{
    ERR("PROGRAM", STM_NOMBRE_ESPERADO, 7);
    ERR("PROGRAM 'X'", STM_NOMBRE_ESPERADO, 8);
    ERR("PROGRAM 5", STM_NOMBRE_ESPERADO, 8);
    ERR("PROGRAM A, B", STM_SOBRA, 9);
}

static void test_declaraciones(void)
{
    SENT("INTEGER I", "(integer (d I))");
    SENT("INTEGER I, J, K", "(integer (d I) (d J) (d K))");
    SENT("INTEGER I J", "(integer (d IJ))");
    SENT("REAL A(10)", "(real (d (ref A 10)))");
    SENT("REAL M(0:9, 2)", "(real (d (ref M (: 0 9) 2)))");
    SENT("REAL X, V(N), W(2, 3)", "(real (d X) (d (ref V N)) (d (ref W 2 3)))");
    SENT("DOUBLE PRECISION X", "(doubleprecision (d X))");
    SENT("DOUBLEPRECISION X, Y", "(doubleprecision (d X) (d Y))");
    SENT("LOGICAL FLAG", "(logical (d FLAG))");

    /* CHARACTER: longitud por defecto y por declarador */
    SENT("CHARACTER C", "(character (d C))");
    SENT("CHARACTER*10 S", "(character len=10 (d S))");
    SENT("CHARACTER*10 S, T", "(character len=10 (d S) (d T))");
    SENT("CHARACTER*10, S", "(character len=10 (d S))");
    SENT("CHARACTER*3 E", "(character len=3 (d E))");              /* "3 E" no es un real */
    SENT("CHARACTER*3 E5, D1", "(character len=3 (d E5) (d D1))");
    SENT("CHARACTER*12 D", "(character len=12 (d D))");
    SENT("CHARACTER*(N+1) S", "(character len=(+ N 1) (d S))");
    SENT("CHARACTER S*10", "(character (d S len=10))");
    SENT("CHARACTER*8 A, B*4", "(character len=8 (d A) (d B len=4))");
    SENT("CHARACTER A*10, B(5)*3, C", "(character (d A len=10) (d (ref B 5) len=3) (d C))");
    SENT("CHARACTER A(5)*(N)", "(character (d (ref A 5) len=N))");
    SENT("CHARACTER*2 M(3, 4)", "(character len=2 (d (ref M 3 4)))");

    /* Errores */
    ERR_EXPR("INTEGER", EXPR_ESPERADA, 7);
    ERR("INTEGER 5", STM_DECLARADOR, 8);
    ERR("INTEGER A+B", STM_DECLARADOR, 9);
    ERR("INTEGER (A)", STM_DECLARADOR, 8);
    ERR("INTEGER A*3", STM_DECLARADOR, 9);        /* la longitud solo en CHARACTER */
    ERR_EXPR("INTEGER A,", EXPR_ESPERADA, 10);
    ERR("REAL*8 X", STM_LONGITUD, 4);
    ERR("CHARACTER*", STM_LONGITUD, 10);
    ERR("CHARACTER A(3)*B", STM_LONGITUD, 15);
    SENT("INTEGER A=1", "(asig INTEGERA 1)");   /* sin palabras reservadas: es una asignacion */
}

static void test_asignacion(void)
{
    SENT("X = 1", "(asig X 1)");
    SENT("X=1", "(asig X 1)");
    SENTL(20, "X = Y + 1", "20: (asig X (+ Y 1))");
    SENT("A(I) = B(I) + 1", "(asig (ref A I) (+ (ref B I) 1))");
    SENT("M(I, J) = 0", "(asig (ref M I J) 0)");
    SENT("S(2:3) = 'AB'", "(asig (ref S (: 2 3)) 'AB')");
    SENT("C(I)(2:3) = 'X'", "(asig (sub (ref C I) (: 2 3)) 'X')");
    SENT("F(X) = X*2", "(asig (ref F X) (* X 2))");   /* funcion-sentencia: lo decide el semantico */
    SENT("DO10I = 1", "(asig DO10I 1)");              /* 'DO10I=1.10' no es un DO */
    SENT("IF(3) = 1", "(asig (ref IF 3) 1)");

    ERR_EXPR("X =", EXPR_ESPERADA, 3);
    ERR_EXPR("X = (1", EXPR_PAR_CIERRA, 6);
    ERR_EXPR("X = 1 +", EXPR_ESPERADA, 7);
    ERR("X = 1 )", STM_SOBRA, 6);
    ERR("A = B = C", STM_SOBRA, 6);
    ERR("3 = X", STM_DESCONOCIDA, 0);
}

static void test_print(void)
{
    SENT("PRINT *, A", "(print * A)");
    SENT("PRINT *,A,B", "(print * A B)");
    SENT("PRINT *, 'HOLA', X + 1", "(print * 'HOLA' (+ X 1))");
    SENT("PRINT *", "(print *)");
    SENT("PRINT 100, A", "(print 100 A)");
    SENT("PRINT '(A)', X", "(print '(A)' X)");
    SENT("PRINT '(A)'", "(print '(A)')");
    SENTL(5, "PRINT *, A(I), B(J) * 2", "5: (print * (ref A I) (* (ref B J) 2))");

    ERR("PRINT A", STM_FORMATO, 6);
    ERR("PRINT", STM_FORMATO, 5);
    ERR("PRINT * A", STM_COMA_ESPERADA, 8);
    ERR("PRINT 0, A", STM_ETIQUETA_RANGO, 6);
    ERR("PRINT 100000, A", STM_ETIQUETA_RANGO, 6);
    ERR_EXPR("PRINT *,", EXPR_ESPERADA, 8);
    ERR_EXPR("PRINT *, A,", EXPR_ESPERADA, 11);
    ERR("PRINT *, A B )", STM_SOBRA, 13);
}

static void test_stop_goto(void)
{
    SENT("STOP", "(stop)");
    SENT("STOP 5", "(stop 5)");
    SENT("STOP 'FIN'", "(stop 'FIN')");
    SENT("STOP 5 6", "(stop 56)");
    ERR("STOP X", STM_ARGUMENTO, 5);
    ERR("STOP 5, 6", STM_SOBRA, 6);

    SENT("GOTO 10", "(goto 10)");
    SENT("GO TO 10", "(goto 10)");
    SENT("GOTO (10, 20, 30), I", "(goto-calc ( 10 20 30) I)");
    SENT("GOTO (10, 20) I", "(goto-calc ( 10 20) I)");
    SENT("GOTO (10), I + 1", "(goto-calc ( 10) (+ I 1))");
    ERR("GOTO", STM_ETIQUETA_ESPERADA, 4);
    ERR("GOTO X", STM_ETIQUETA_ESPERADA, 5);
    ERR("GOTO 0", STM_ETIQUETA_RANGO, 5);
    ERR("GOTO 100000", STM_ETIQUETA_RANGO, 5);
    ERR("GOTO 10, 20", STM_SOBRA, 7);
    ERR_EXPR("GOTO (10, 20)", EXPR_ESPERADA, 13);
    ERR("GOTO (10, X), I", STM_ETIQUETA_ESPERADA, 10);
    ERR("GOTO (10, 20", STM_PAR_CIERRA_ESPERADO, 12);
}

static void test_do(void)
{
    SENT("DO 10 I = 1, 10", "(do 10 I 1 10)");
    SENT("DO 10, I = 1, 10", "(do 10 I 1 10)");
    SENT("DO 20 J = N, 1, -1", "(do 20 J N 1 (neg 1))");
    SENT("DO 30 K = A(1), B + 1, 2 * S", "(do 30 K (ref A 1) (+ B 1) (* 2 S))");
    SENTL(5, "DO 10 I = 1, 10", "5: (do 10 I 1 10)");
    SENT("DO 10 I = 1", "(asig DO10I 1)");     /* sin coma no es un DO (cap. 1) */
    ERR("DO 10 I = 1, 2, 3, 4", STM_SOBRA, 17);
    ERR_EXPR("DO 10 I = , 2", EXPR_ESPERADA, 10);
    ERR_EXPR("DO 10 I = 1,", EXPR_ESPERADA, 12);
}

static void test_if(void)
{
    SENT("IF (X .GT. 0) THEN", "(if (.GT. X 0))");
    SENT("IF (A) THEN", "(if A)");
    SENTL(7, "IF ((A .AND. B) .OR. C) THEN", "7: (if (.OR. (paren (.AND. A B)) C))");
    SENT("ELSEIF (X .LT. 0) THEN", "(elseif (.LT. X 0))");
    SENT("ELSE IF (B) THEN", "(elseif B)");
    SENT("IF (X) 10, 20, 30", "(if-arit X 10 20 30)");
    SENT("IF (X - 1) 10, 10, 30", "(if-arit (- X 1) 10 10 30)");
    SENT("IF (X) 1,2,3", "(if-arit X 1 2 3)");

    ERR_EXPR("IF (X .GT. ) THEN", EXPR_ESPERADA, 11);
    ERR("IF (X) 10, 20", STM_COMA_ESPERADA, 13);
    ERR("IF (X) 10, 20, 30, 40", STM_SOBRA, 17);
    ERR("IF (X) 10, 20, A", STM_ETIQUETA_ESPERADA, 15);
    ERR("IF (X) 0, 1, 2", STM_ETIQUETA_RANGO, 7);
    ERR("ELSEIF (X) THEN Y", STM_DESCONOCIDA, 0);
}

static void test_if_logico(void)
{
    IFLOG("IF (X .GT. 0) Y = 1", "(if-log (.GT. X 0)) | (asig Y 1)");
    IFLOG("IF (X) GOTO 10", "(if-log X) | (goto 10)");
    IFLOG("IF (X) PRINT *, X", "(if-log X) | (print * X)");
    IFLOG("IF (X) STOP", "(if-log X) | (stop)");
    IFLOG("IF (X) STOP 'FIN'", "(if-log X) | (stop 'FIN')");
    IFLOG("IF (X) CONTINUE", "(if-log X) | (continue)");
    IFLOG("IF (A(1)) X(2) = 3", "(if-log (ref A 1)) | (asig (ref X 2) 3)");
    IFLOG("IF (X) IF2 = 3", "(if-log X) | (asig IF2 3)");
    IFLOG("IF(X)Y=1", "(if-log X) | (asig Y 1)");         /* sin blanco tras el ')' */
    IFLOG("IF (X)GOTO 10", "(if-log X) | (goto 10)");
    IFLOG("IF (X) GOTO (1, 2, 3), K", "(if-log X) | (goto-calc ( 1 2 3) K)");

    /* El IF aritmetico si se admite dentro de un IF logico */
    IFLOG("IF (A) IF (B) 10, 20, 30", "(if-log A) | (if-arit B 10 20 30)");

    /* La etiqueta es del IF, no de la interior */
    {
        analizar(30, "IF (X) Y = 1");
        COMPROBAR(stm.tipo == SENT_IF_LOGICO && stm.etiqueta == 30 && stm.interna == 0);
        stmt_analizar_interna();
        COMPROBAR(stm.tipo == SENT_ASIGNACION && stm.etiqueta == 0 && stm.interna == 1);
    }

    /* La condicion del IF sigue valida en el pool tras analizar la interior */
    {
        uint16_t cond;

        analizar(0, "IF (X .GT. 0) Y = 1");
        cond = stm.a;
        stmt_analizar_interna();
        COMPROBAR(ast_tipo(cond) == NODO_GT);
    }

    ERR_INTERNA("IF (A) IF (B) X = 1", STM_NO_EN_IF, 7);
    ERR_INTERNA("IF (A) DO 10 I = 1, 2", STM_NO_EN_IF, 7);
    ERR_INTERNA("IF (A) END", STM_NO_EN_IF, 7);
    ERR_INTERNA("IF (A) INTEGER X", STM_NO_EN_IF, 7);
    ERR_INTERNA("IF (A) ELSE", STM_NO_EN_IF, 7);
    ERR_INTERNA("IF (A) PROGRAM P", STM_NO_EN_IF, 7);
    ERR("IF (A)", STM_DESCONOCIDA, 0);          /* sin nada detras no es un IF logico */
    ERR_INTERNA("IF (A) FOO", STM_DESCONOCIDA, 7);
    ERR_INTERNA("IF (A) GOTO", STM_ETIQUETA_ESPERADA, 11);
    ERR_INTERNA("IF (A) PRINT A", STM_FORMATO, 13);
}

static void test_errores_generales(void)
{
    uint16_t i;
    char texto[600];

    ERR("FOO", STM_DESCONOCIDA, 0);
    ERR("   WRITE (*,*) X", STM_DESCONOCIDA, 3);    /* aun no soportada */
    ERR("X", STM_DESCONOCIDA, 0);

    /* Errores del escaner */
    ERR("X = 1.5E", STM_LEXICO, 4);
    COMPROBAR(stm.err_lex == LEX_REAL_MAL_FORMADO);
    ERR("PRINT *, 'abc", STM_LEXICO, 9);
    COMPROBAR(stm.err_lex == LEX_CADENA_SIN_CERRAR);
    ERR("X = A # B", STM_LEXICO, 6);
    COMPROBAR(stm.err_lex == LEX_CARACTER_ILEGAL);

    /* Las listas van en flujo: 60 declaradores (180 nodos en total, mas de
     * AST_NODOS) caben porque solo hay un elemento vivo cada vez */
    strcpy(texto, "INTEGER A0");
    for (i = 1; i < 60; i++) {
        sprintf(texto + strlen(texto), ",A%u", (unsigned)i);
    }
    analizar(0, texto);
    COMPROBAR(stm.err == STM_OK);
    COMPROBAR(strstr(lista_txt, "(d A59)") != NULL);
    COMPROBAR(ast_usados() <= AST_NODOS);

    /* Idem PRINT con 200 expresiones y GOTO calculado con 100 etiquetas */
    strcpy(texto, "PRINT *, 1");
    for (i = 1; i < 200; i++) {
        strcat(texto, ",1");
    }
    analizar(0, texto);
    COMPROBAR(stm.err == STM_OK);
    strcpy(texto, "GOTO (10");
    for (i = 1; i < 100; i++) {
        strcat(texto, ",10");
    }
    strcat(texto, "), I");
    analizar(0, texto);
    COMPROBAR(stm.err == STM_OK);

    /* El pool se acaba con UN elemento enorme: 40 sumas = 81 nodos > 64 */
    strcpy(texto, "X = 1");
    for (i = 0; i < 40; i++) {
        strcat(texto, "+1");
    }
    analizar(0, texto);
    COMPROBAR(stm.err == STM_POOL_LLENO || stm.err == STM_EXPR);
    printf("   (maximo de nodos vivos en un elemento: %u)\n", max_nodos);

    /* Tras un error en un elemento, la lista no sigue ni con otra llamada */
    analizar(0, "INTEGER A, 5, B");
    COMPROBAR(stm.err == STM_DECLARADOR);
    COMPROBAR(stmt_elemento() == 0);
    COMPROBAR(stm.err == STM_DECLARADOR);

    /* Un error no contamina la sentencia siguiente */
    analizar(0, "X = (1");
    COMPROBAR(stm.err == STM_EXPR);
    SENT("X = 1", "(asig X 1)");
    analizar(0, "PRINT A");
    COMPROBAR(stm.err == STM_FORMATO);
    SENT("PRINT *, A", "(print * A)");

    /* El AST se reinicia en cada sentencia */
    analizar(0, "CONTINUE");
    COMPROBAR(ast_usados() == 1);
    analizar(0, "X = 1");
    COMPROBAR(ast_usados() == 3);       /* NULO, X, 1 */

    /* Todos los mensajes existen */
    for (i = 0; i <= STM_LEXICO; i++) {
        COMPROBAR(strlen(stmt_mensaje((ErrorStmt)i)) > 0);
        COMPROBAR(strcmp(stmt_mensaje((ErrorStmt)i), "unknown error") != 0);
    }
}

int main(void)
{
    test_simples();
    test_program();
    test_declaraciones();
    test_asignacion();
    test_print();
    test_stop_goto();
    test_do();
    test_if();
    test_if_logico();
    test_errores_generales();

    printf("%d pruebas, %d fallos\n", pruebas, fallos);
    return fallos != 0;
}
