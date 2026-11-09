/* test_cap05.c - Pruebas del capitulo 5 (semantico): tabla de simbolos. */
#include <stdio.h>
#include <string.h>
#include "source.h"
#include "texto.h"
#include "mf_api.h"
#include "lexer.h"
#include "ast.h"
#include "expr.h"
#include "stmt.h"
#include "sim.h"
#include "sem.h"
#include "intr.h"
#include "banco.h"

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

static void reiniciar(void)
{
    banco_iniciar();
    sim_iniciar();
}

static void test_registro(void)
{
    COMPROBAR(sizeof(Simbolo) == 16);
    COMPROBAR(SIM_MAX * sizeof(Simbolo) == BANCO_PAGINA_TAM);
    COMPROBAR((SIM_CUBETAS & (SIM_CUBETAS - 1)) == 0);
    COMPROBAR(SIM_TIPO(&(Simbolo){.tc = SIM_TC(TD_CHARACTER, SIM_ARRAY)}) == TD_CHARACTER);
    COMPROBAR(SIM_CLASE(&(Simbolo){.tc = SIM_TC(TD_CHARACTER, SIM_ARRAY)}) == SIM_ARRAY);
}

static void test_alta_busqueda(void)
{
    Simbolo s;
    uint16_t a, b;

    reiniciar();
    COMPROBAR(sim_cuenta() == 0);
    COMPROBAR(sim_buscar("X") == 0);

    a = sim_alta("X", TD_REAL, SIM_VARIABLE);
    b = sim_alta("I", TD_INTEGER, SIM_ARRAY);
    COMPROBAR(a == 1 && b == 2);
    COMPROBAR(sim_cuenta() == 2);
    COMPROBAR(sim_buscar("X") == a);
    COMPROBAR(sim_buscar("I") == b);
    COMPROBAR(sim_buscar("Y") == 0);

    sim_leer(b, &s);
    COMPROBAR(strcmp(s.nombre, "I") == 0);
    COMPROBAR(SIM_TIPO(&s) == TD_INTEGER && SIM_CLASE(&s) == SIM_ARRAY);
    COMPROBAR(s.flags == 0 && s.extra == 0 && s.dir == 0);

    /* duplicado */
    COMPROBAR(sim_alta("X", TD_INTEGER, SIM_VARIABLE) == 0);
    COMPROBAR(sim_cuenta() == 2);
    sim_leer(a, &s);
    COMPROBAR(SIM_TIPO(&s) == TD_REAL);        /* no se pisa */

    /* escritura */
    s.flags = SIMF_USADO | SIMF_ASIGNADO;
    s.extra = 1234;
    s.dir = 0xABCD;
    sim_escribir(a, &s);
    sim_leer(a, &s);
    COMPROBAR(s.flags == (SIMF_USADO | SIMF_ASIGNADO) && s.extra == 1234 && s.dir == 0xABCD);
    COMPROBAR(sim_buscar("X") == a);           /* el enlace sigue intacto */
}

static void test_nombres(void)
{
    uint16_t id8, id7, pref;

    reiniciar();
    id8 = sim_alta("ABCDEFGH", TD_INTEGER, SIM_VARIABLE);     /* NOMBRE_MAX justo */
    id7 = sim_alta("ABCDEFG", TD_INTEGER, SIM_VARIABLE);
    pref = sim_alta("AB", TD_INTEGER, SIM_VARIABLE);
    COMPROBAR(id8 != 0 && id7 != 0 && pref != 0);
    COMPROBAR(id8 != id7 && id7 != pref);
    COMPROBAR(sim_buscar("ABCDEFGH") == id8);
    COMPROBAR(sim_buscar("ABCDEFG") == id7);
    COMPROBAR(sim_buscar("AB") == pref);
    COMPROBAR(sim_buscar("ABC") == 0);
    COMPROBAR(sim_buscar("ABCDEFGX") == 0);             /* solo difiere el 8.o */
    COMPROBAR(sim_alta("ABCDEFGX", TD_INTEGER, SIM_VARIABLE) != id8);
    COMPROBAR(sim_buscar("ABCDEFGX") != id8);
    /* 'a' (97) y '!' (33) difieren en 64: misma cubeta, solo cambia el 8.o */
    COMPROBAR(sim_alta("ABCDEFGa", TD_INTEGER, SIM_VARIABLE) != 0);
    COMPROBAR(sim_buscar("ABCDEFG!") == 0);
    COMPROBAR(sim_alta("ABCDEFGHI", TD_INTEGER, SIM_VARIABLE) == 0);   /* 9 letras */
    COMPROBAR(sim_buscar("ABCDEFGHI") == 0);
    COMPROBAR(sim_alta("", TD_INTEGER, SIM_VARIABLE) == 0);
    COMPROBAR(sim_buscar("") == 0);
    COMPROBAR(sim_cuenta() == 5);
}

static void test_llena(void)
{
    char n[16];
    uint16_t i;
    uint16_t id;

    reiniciar();
    for (i = 0; i < SIM_MAX; i++) {
        sprintf(n, "V%u", (unsigned)i);
        id = sim_alta(n, TD_INTEGER, SIM_VARIABLE);
        COMPROBAR(id == i + 1);
    }
    COMPROBAR(sim_cuenta() == SIM_MAX);
    COMPROBAR(sim_alta("OTRO", TD_INTEGER, SIM_VARIABLE) == 0);     /* tabla llena */
    COMPROBAR(sim_cuenta() == SIM_MAX);

    /* con las 64 cubetas muy cargadas todo se sigue encontrando */
    for (i = 0; i < SIM_MAX; i++) {
        sprintf(n, "V%u", (unsigned)i);
        COMPROBAR(sim_buscar(n) == i + 1);
    }
    COMPROBAR(sim_buscar("V512") == 0);

    sim_iniciar();                      /* nueva unidad: vacia */
    COMPROBAR(sim_cuenta() == 0);
    COMPROBAR(sim_buscar("V0") == 0);
    COMPROBAR(sim_alta("V0", TD_REAL, SIM_VARIABLE) == 1);
}

/* ------------------------------------------------------------------
 * Semantico: se analiza un programa entero ("\n" entre lineas; las lineas
 * llevan 6 blancos delante; una linea que empieza por '#' lleva la etiqueta
 * en las columnas 1-5, p. ej. "#10 CONTINUE").
 * ------------------------------------------------------------------ */

static unsigned sintaxis;       /* sentencias con error de sintaxis */

static void programa(const char *lineas)
{
    static char fuente[8192];
    static Fuente f;
    const char *p = lineas;

    fuente[0] = '\0';
    while (*p != '\0') {
        const char *fin = strchr(p, '\n');
        size_t n = fin != NULL ? (size_t)(fin - p) : strlen(p);

        if (*p == '#') {                           /* "#10 CONTINUE" */
            char cab[8];
            const char *q = p + 1;
            size_t k = 0;

            while (q < p + n && *q != ' ') {
                k++;
                q++;
            }
            sprintf(cab, "%-5.*s ", (int)k, p + 1);
            strcat(fuente, cab);
            strncat(fuente, q + 1, n - (size_t)(q + 1 - p));
        } else {
            strcat(fuente, "      ");
            strncat(fuente, p, n);
        }
        strcat(fuente, "\n");
        p += n + (fin != NULL);
    }
    banco_iniciar();
    sem_iniciar();
    tabla_iniciar();
    fuente_iniciar_texto(&f, fuente);
    sintaxis = 0;
    while (leer_sentencia(&f) == FUENTE_OK) {
        stmt_analizar();
        if (stm.err == STM_OK) {
            sem_sentencia();
        }
        if (stm.err == STM_OK && stm.tipo == SENT_IF_LOGICO) {
            stmt_analizar_interna();
            if (stm.err == STM_OK) {
                sem_sentencia();
            }
        }
        if (stm.err != STM_OK) {
            sintaxis++;
        }
    }
}

/* Sin errores. */
#define LIMPIO(prog)                                                       \
    do {                                                                   \
        programa(prog);                                                    \
        pruebas++;                                                         \
        if (sintaxis != 0 || mf.n != 0) {                             \
            fallos++;                                                      \
            printf("FALLO (linea %d): esperaba limpio, sintaxis=%u sem=%u (%s)\n   %s\n", \
                   __LINE__, sintaxis, (unsigned)mf.n,                \
                   mf.n ? sem_mensaje((ErrorSem)mf.diag[0].cod) : "", prog); \
        }                                                                  \
    } while (0)

/* Exactamente n errores semanticos, el primero cod en (linea, columna). */
#define ERRSEM(prog, nerr, esp, lin, col)                                     \
    do {                                                                   \
        programa(prog);                                                    \
        pruebas++;                                                         \
        if (sintaxis != 0 || mf.n != (nerr) || mf.diag[0].cod != (esp)   \
            || mf.diag[0].linea != (lin) || mf.diag[0].columna != (col)) { \
            fallos++;                                                      \
            printf("FALLO (linea %d): sintaxis=%u n=%u (esp %d) cod=%d (esp %d) pos=%u:%u (esp %d:%d)\n   %s\n", \
                   __LINE__, sintaxis, (unsigned)mf.n, (int)(nerr),     \
                   (int)mf.diag[0].cod, (int)(esp),                        \
                   (unsigned)mf.diag[0].linea, (unsigned)mf.diag[0].columna, \
                   (int)(lin), (int)(col), prog);                          \
        }                                                                  \
    } while (0)

static uint8_t tipo_de(const char *nombre)
{
    Simbolo s;
    uint16_t id = sim_buscar(nombre);

    if (id == 0) {
        return 99;
    }
    sim_leer(id, &s);
    return SIM_TIPO(&s);
}

static void test_implicito(void)
{
    LIMPIO("X = 1\nI = 2\nK = X");
    COMPROBAR(tipo_de("X") == TD_REAL);
    COMPROBAR(tipo_de("I") == TD_INTEGER);
    COMPROBAR(tipo_de("K") == TD_INTEGER);
    LIMPIO("A = 1\nH = 1\nO = 1\nZ = 1\nJ = 1\nN = 1");
    COMPROBAR(tipo_de("A") == TD_REAL && tipo_de("H") == TD_REAL);
    COMPROBAR(tipo_de("O") == TD_REAL && tipo_de("Z") == TD_REAL);
    COMPROBAR(tipo_de("J") == TD_INTEGER && tipo_de("N") == TD_INTEGER);
    COMPROBAR(sim_cuenta() == 6);
}

static void test_declaraciones(void)
{
    Simbolo s;
    Dim d;

    LIMPIO("INTEGER A, B(10), C(0:9, 2)\nREAL X\nLOGICAL L\nDOUBLE PRECISION D\n"
           "CHARACTER*8 S, T(3)*2, U*5");
    COMPROBAR(tipo_de("A") == TD_INTEGER && tipo_de("X") == TD_REAL);
    COMPROBAR(tipo_de("L") == TD_LOGICAL && tipo_de("D") == TD_DOUBLE);
    sim_leer(sim_buscar("B"), &s);
    COMPROBAR(SIM_CLASE(&s) == SIM_ARRAY && SIM_NDIM(&s) == 1);
    COMPROBAR((s.flags & SIMF_TIPO_EXPLICITO) != 0);
    sim_dim_leer(s.extra, &d);
    COMPROBAR(d.inf[0] == 1 && d.sup[0] == 10 && d.len == 1);
    sim_leer(sim_buscar("C"), &s);
    COMPROBAR(SIM_NDIM(&s) == 2);
    sim_dim_leer(s.extra, &d);
    COMPROBAR(d.inf[0] == 0 && d.sup[0] == 9 && d.inf[1] == 1 && d.sup[1] == 2);
    sim_leer(sim_buscar("S"), &s);
    COMPROBAR(SIM_TIPO(&s) == TD_CHARACTER && s.extra == 8);
    sim_leer(sim_buscar("U"), &s);
    COMPROBAR(s.extra == 5);
    sim_leer(sim_buscar("T"), &s);
    COMPROBAR(SIM_CLASE(&s) == SIM_ARRAY);
    sim_dim_leer(s.extra, &d);
    COMPROBAR(d.len == 2 && d.sup[0] == 3);

    LIMPIO("CHARACTER Z\n");                 /* longitud 1 por omision */
    sim_leer(sim_buscar("Z"), &s);
    COMPROBAR(s.extra == 1);
    LIMPIO("INTEGER P(5:5)");
    LIMPIO("INTEGER M(-5:5)");
    sim_leer(sim_buscar("M"), &s);
    sim_dim_leer(s.extra, &d);
    COMPROBAR(d.inf[0] == -5 && d.sup[0] == 5);

    /* errores */
    ERRSEM("INTEGER A\nREAL A", 1, SEM_YA_DEFINIDO, 2, 12);
    ERRSEM("X = 1\nREAL X", 1, SEM_YA_DEFINIDO, 2, 12);
    ERRSEM("INTEGER A(0)", 1, SEM_DIMENSION, 1, 17);
    ERRSEM("INTEGER A(5:1)", 1, SEM_DIMENSION, 1, 17);
    ERRSEM("INTEGER A(N)", 1, SEM_DIMENSION, 1, 17);
    ERRSEM("INTEGER A(2,2,2)", 1, SEM_DIMENSIONES, 1, 21);
    ERRSEM("CHARACTER*0 S", 1, SEM_LONGITUD, 1, 17);
    ERRSEM("CHARACTER*256 S", 1, SEM_LONGITUD, 1, 17);
    programa("INTEGER A*4");               /* lo rechaza el parser */
    COMPROBAR(sintaxis == 1 && mf.n == 0);
    LIMPIO("CHARACTER*255 S");
    LIMPIO("INTEGER A(32767)");
    ERRSEM("INTEGER A(32768)", 1, SEM_DIMENSION, 1, 17);
}

static void test_tipos_expresion(void)
{
    LIMPIO("INTEGER I\nREAL X\nDOUBLE PRECISION D\nX = I + 1\nD = X * 2.5D0\nI = I ** 2");
    LIMPIO("LOGICAL L, M\nL = .TRUE.\nM = L .AND. .NOT. L .OR. L .EQV. M\nL = 1 .LT. 2.5");
    LIMPIO("CHARACTER*4 S, T\nLOGICAL L\nS = 'AB' // T\nL = S .EQ. T");
    LIMPIO("X = 1\nI = X");                           /* conversion REAL->INTEGER permitida */
    ERRSEM("LOGICAL L\nX = L + 1", 1, SEM_NUMERICO_ESPERADO, 2, 13);
    ERRSEM("LOGICAL L\nL = 1 .AND. L", 1, SEM_LOGICO_ESPERADO, 2, 13);
    ERRSEM("LOGICAL L\nL = .NOT. 1", 1, SEM_LOGICO_ESPERADO, 2, 11);
    ERRSEM("CHARACTER S\nX = -S", 1, SEM_NUMERICO_ESPERADO, 2, 11);
    ERRSEM("CHARACTER S\nS = S // 1", 1, SEM_TEXTO_ESPERADO, 2, 13);
    ERRSEM("CHARACTER S\nLOGICAL L\nL = S .EQ. 1", 1, SEM_INCOMPATIBLES, 3, 13);
    ERRSEM("LOGICAL L, M\nL = L .LT. M", 1, SEM_INCOMPATIBLES, 2, 13);
    ERRSEM("LOGICAL L\nI = L", 1, SEM_INCOMPATIBLES, 2, 7);
    ERRSEM("CHARACTER S\nS = 1", 1, SEM_INCOMPATIBLES, 2, 7);
    ERRSEM("LOGICAL L\nL = 1.5", 1, SEM_INCOMPATIBLES, 2, 7);
    LIMPIO("I = 32767\nI = -32768");
    ERRSEM("I = 32768", 1, SEM_ENTERO_GRANDE, 1, 11);
    ERRSEM("I = 100000", 1, SEM_ENTERO_GRANDE, 1, 11);
    ERRSEM("I = -32769", 1, SEM_ENTERO_GRANDE, 1, 12);
}

static void test_tdato(void)
{
    /* el tipo del resultado queda en Nodo.tdato (INTEGER < REAL < DOUBLE) */
    programa("DOUBLE PRECISION D\nD = I + 2.5D0");
    COMPROBAR(ast_tdato(stm.b) == TD_DOUBLE);
    programa("X = I + 2");
    COMPROBAR(mf.n == 0 && ast_tdato(stm.b) == TD_INTEGER);
    programa("X = I * 2.5");
    COMPROBAR(ast_tdato(stm.b) == TD_REAL);
    programa("X = 2.5 * I");
    COMPROBAR(ast_tdato(stm.b) == TD_REAL);
    programa("LOGICAL L\nL = I .LT. 2");
    COMPROBAR(ast_tdato(stm.b) == TD_LOGICAL);
    programa("CHARACTER S\nS = S // S");
    COMPROBAR(ast_tdato(stm.b) == TD_CHARACTER);
}

static void test_flags(void)
{
    Simbolo s;

    programa("INTEGER A(3)\nCHARACTER*4 S, T\nA(1) = 2\nX = A(2)\nS(1:2) = T(2:3)\nDO 10 I = 1, 3\n#10 CONTINUE");
    sim_leer(sim_buscar("A"), &s);
    COMPROBAR((s.flags & (SIMF_USADO | SIMF_ASIGNADO)) == (SIMF_USADO | SIMF_ASIGNADO));
    sim_leer(sim_buscar("X"), &s);
    COMPROBAR((s.flags & (SIMF_USADO | SIMF_ASIGNADO)) == SIMF_ASIGNADO);
    sim_leer(sim_buscar("S"), &s);
    COMPROBAR((s.flags & (SIMF_USADO | SIMF_ASIGNADO)) == SIMF_ASIGNADO);
    sim_leer(sim_buscar("T"), &s);
    COMPROBAR((s.flags & (SIMF_USADO | SIMF_ASIGNADO)) == SIMF_USADO);
    sim_leer(sim_buscar("I"), &s);
    COMPROBAR((s.flags & (SIMF_USADO | SIMF_ASIGNADO)) == (SIMF_USADO | SIMF_ASIGNADO));
    programa("Z = W");
    sim_leer(sim_buscar("W"), &s);
    COMPROBAR((s.flags & (SIMF_USADO | SIMF_ASIGNADO)) == SIMF_USADO);
    programa("INTEGER A(3)\nA(1) = 2");
    sim_leer(sim_buscar("A"), &s);
    COMPROBAR(SIM_NDIM(&s) == 1);              /* marcar no pisa las dimensiones */
}

static void test_arrays(void)
{
    LIMPIO("INTEGER A(10), M(3,4)\nA(1) = 5\nM(I, J + 1) = A(2) * 3\nI = M(1, 1)");
    ERRSEM("INTEGER A(10)\nA = 1", 1, SEM_ARRAY_SIN_INDICES, 2, 7);
    ERRSEM("INTEGER A(10)\nX = A", 1, SEM_ARRAY_SIN_INDICES, 2, 11);
    ERRSEM("INTEGER A(10)\nA(1, 2) = 1", 1, SEM_NUM_INDICES, 2, 7);
    ERRSEM("INTEGER M(3,4)\nM(1) = 1", 1, SEM_NUM_INDICES, 2, 7);
    ERRSEM("INTEGER A(10)\nA(2.5) = 1", 1, SEM_ENTERO_ESPERADO, 2, 9);
    ERRSEM("INTEGER A(10)\nA(2:3) = 1", 1, SEM_RANGO_NO_PERMITIDO, 2, 9);
    ERRSEM("X = 1\nX(1) = 2", 1, SEM_NO_ES_ARRAY, 2, 7);
    ERRSEM("F(1) = 2", 1, SEM_NO_ES_ARRAY, 1, 7);
    ERRSEM("INTEGER A(10)\nA(1) = A(2.0)", 1, SEM_ENTERO_ESPERADO, 2, 16);
}

static void test_cadenas(void)
{
    LIMPIO("CHARACTER*8 S, T(3)\nS(2:3) = 'AB'\nT(1)(1:2) = S(3:4)\nS(:2) = S(3:)");
    LIMPIO("CHARACTER*8 S\nI = 3\nS(I:I+1) = 'XY'");
    ERRSEM("CHARACTER*8 S\nS(1.5:3) = 'A'", 1, SEM_ENTERO_ESPERADO, 2, 9);
    ERRSEM("CHARACTER S\nS(1:2, 3) = 'A'", 1, SEM_SUBCADENA, 2, 7);
    ERRSEM("CHARACTER*4 T(3)\nT(1)(1,2) = 'A'", 1, SEM_SUBCADENA, 2, 11);
    ERRSEM("CHARACTER*4 T(3)\nT(1)(2) = 'A'", 1, SEM_SUBCADENA, 2, 11);
    ERRSEM("CHARACTER*4 T(3)\nS = T(1)(I)", 1, SEM_SUBCADENA, 2, 15);
    ERRSEM("INTEGER K\nK(1:2) = 1", 1, SEM_SUBCADENA, 2, 7);
    ERRSEM("INTEGER K(3)\nK(1)(1:2) = 1", 1, SEM_SUBCADENA, 2, 11);
}

static void test_control(void)
{
    LIMPIO("LOGICAL L\nIF (L) X = 1\nIF (X .GT. 1 .AND. L) THEN\nENDIF\nIF (I) 10, 20, 30");
    ERRSEM("IF (1) THEN\nENDIF", 1, SEM_LOGICO_ESPERADO, 1, 11);
    ERRSEM("IF (X) Y = 1", 1, SEM_LOGICO_ESPERADO, 1, 11);
    ERRSEM("LOGICAL L\nIF (L) 10, 20, 30", 1, SEM_NUMERICO_ESPERADO, 2, 11);
    LIMPIO("DO 10 I = 1, 10\n#10 CONTINUE");
    LIMPIO("DO 10 X = 1.0, 2.0, 0.5\n#10 CONTINUE");
    ERRSEM("LOGICAL L\nDO 10 L = 1, 2\n#10 CONTINUE", 1, SEM_VARIABLE_DO, 2, 17);
    ERRSEM("INTEGER A(3)\nDO 10 A = 1, 2\n#10 CONTINUE", 1, SEM_VARIABLE_DO, 2, 17);
    ERRSEM("DO 10 I = 1, .TRUE.\n#10 CONTINUE", 1, SEM_NUMERICO_ESPERADO, 1, 20);
    ERRSEM("DO 10 I = 1, 2, .TRUE.\n#10 CONTINUE", 1, SEM_NUMERICO_ESPERADO, 1, 23);
    LIMPIO("GOTO (10, 20), I\n#10 CONTINUE\n#20 CONTINUE");
    ERRSEM("GOTO (10, 20), 1.5\n#10 CONTINUE\n#20 CONTINUE", 1, SEM_ENTERO_ESPERADO, 1, 22);
    LIMPIO("PRINT *, 1, X, 'T', I + 2\nPRINT 10, I\nSTOP 'FIN'\nSTOP 5");
}

static void test_bloques(void)
{
    char t[4096];
    unsigned i;

    LIMPIO("PROGRAM T\nINTEGER I, J\nLOGICAL L\nDO 10 I = 1, 3\nDO 10 J = 1, 3\n#10 CONTINUE\n"
           "IF (I .GT. 1) THEN\nX = 1\nELSEIF (I .LT. 0) THEN\nX = 2\nELSE\nX = 3\nENDIF\n"
           "GOTO 20\n#20 CONTINUE\nEND");
    LIMPIO("DO 20 I = 1, 2\nDO 10 J = 1, 2\n#10 CONTINUE\n#20 CONTINUE\nEND");
    LIMPIO("LOGICAL L\nIF (L) THEN\nDO 10 I = 1, 2\n#10 CONTINUE\nENDIF\nEND");
    LIMPIO("LOGICAL L\nDO 10 I = 1, 2\nIF (L) THEN\nENDIF\n#10 CONTINUE\nEND");

    ERRSEM("ELSE\nEND", 1, SEM_SIN_IF, 1, 7);
    ERRSEM("ENDIF\nEND", 1, SEM_SIN_IF, 1, 7);
    ERRSEM("LOGICAL L\nELSEIF (L) THEN\nEND", 1, SEM_SIN_IF, 2, 7);
    ERRSEM("LOGICAL L\nIF (L) THEN\nELSE\nELSE\nENDIF\nEND", 1, SEM_ELSE_REPETIDO, 4, 7);
    ERRSEM("LOGICAL L\nIF (L) THEN\nELSE\nELSEIF (L) THEN\nENDIF\nEND", 1, SEM_ELSE_REPETIDO, 4, 7);
    ERRSEM("LOGICAL L\nIF (L) THEN\nEND", 1, SEM_IF_SIN_CERRAR, 2, 7);
    ERRSEM("DO 10 I = 1, 2\nEND", 2, SEM_DO_SIN_CERRAR, 1, 7);
    COMPROBAR(mf.diag[1].cod == SEM_ETIQUETA_INDEFINIDA && mf.diag[1].linea == 1);
    ERRSEM("LOGICAL L\nIF (L) THEN\nDO 10 I = 1, 2\nENDIF\n#10 CONTINUE\nEND", 1, SEM_DO_SIN_CERRAR, 3, 7);
    ERRSEM("LOGICAL L\nIF (L) THEN\nDO 10 I = 1, 2\nELSE\n#10 CONTINUE\nENDIF\nEND", 1, SEM_DO_SIN_CERRAR, 3, 7);
    ERRSEM("LOGICAL L\nDO 10 I = 1, 2\nIF (L) THEN\n#10 CONTINUE\nENDIF\nEND", 2, SEM_ANIDAMIENTO, 4, 7);
    COMPROBAR(mf.diag[1].cod == SEM_SIN_IF && mf.diag[1].linea == 5);
    ERRSEM("DO 10 I = 1, 2\nDO 20 J = 1, 2\n#10 CONTINUE\n#20 CONTINUE\nEND", 1, SEM_ANIDAMIENTO, 3, 7);

    /* etiquetas */
    ERRSEM("#10 CONTINUE\n#10 CONTINUE\nEND", 1, SEM_ETIQUETA_DUPLICADA, 2, 7);
    ERRSEM("GOTO 99\nEND", 1, SEM_ETIQUETA_INDEFINIDA, 1, 7);
    ERRSEM("GOTO (10, 20), I\n#10 CONTINUE\nEND", 1, SEM_ETIQUETA_INDEFINIDA, 1, 17);
    ERRSEM("IF (I) 10, 20, 30\n#10 CONTINUE\n#30 CONTINUE\nEND", 1, SEM_ETIQUETA_INDEFINIDA, 1, 7);
    ERRSEM("IF (I) 10, 20, 30\n#20 CONTINUE\n#30 CONTINUE\nEND", 1, SEM_ETIQUETA_INDEFINIDA, 1, 7);
    ERRSEM("IF (I) 10, 20, 30\n#10 CONTINUE\n#20 CONTINUE\nEND", 1, SEM_ETIQUETA_INDEFINIDA, 1, 7);
    ERRSEM("X = 1\nGOTO 99\nGOTO 99\nEND", 1, SEM_ETIQUETA_INDEFINIDA, 2, 7);   /* la 1.a referencia */
    ERRSEM("IF (I .GT. 1) GOTO 30\nEND", 1, SEM_ETIQUETA_INDEFINIDA, 1, 7);
    LIMPIO("IF (I .GT. 1) GOTO 30\n#30 CONTINUE\nEND");
    LIMPIO("IF (I) 10, 20, 30\n#10 CONTINUE\n#20 CONTINUE\n#30 CONTINUE\nEND");
    LIMPIO("#10 X = 1\nGOTO 10\nEND");                   /* referencia hacia atras */
    ERRSEM("#10 INTEGER A\nEND", 1, SEM_ETIQUETA_EN_DECL, 1, 7);
    ERRSEM("#10 PROGRAM T\nEND", 1, SEM_ETIQUETA_EN_DECL, 1, 7);
    LIMPIO("#10 END");

    /* DO: etiqueta y sentencia terminal */
    ERRSEM("#10 CONTINUE\nDO 10 I = 1, 2\nEND", 2, SEM_DO_ETIQUETA_ATRAS, 2, 7);
    COMPROBAR(mf.diag[1].cod == SEM_DO_SIN_CERRAR && mf.diag[1].linea == 2);
    ERRSEM("DO 10 I = 1, 2\n#10 GOTO 10\nEND", 1, SEM_DO_TERMINAL, 2, 7);
    ERRSEM("DO 10 I = 1, 2\n#10 STOP\nEND", 1, SEM_DO_TERMINAL, 2, 7);
    ERRSEM("DO 10 I = 1, 2\n#10 IF (I) 10, 10, 10\nEND", 1, SEM_DO_TERMINAL, 2, 7);
    ERRSEM("LOGICAL L\nIF (L) THEN\nDO 10 I = 1, 2\n#10 ENDIF\nEND", 1, SEM_DO_TERMINAL, 4, 7);
    LIMPIO("DO 10 I = 1, 2\n#10 IF (I .GT. 1) GOTO 10\nEND");
    LIMPIO("DO 10 I = 1, 2\n#10 GOTO (10, 10), I\nEND");
    LIMPIO("DO 10 I = 1, 2\n#10 X = 1\nEND");
    LIMPIO("DO 10 I = 1, 2\n#10 PRINT *, I\nEND");

    /* profundidad */
    strcpy(t, "LOGICAL L\n");
    for (i = 0; i < SEM_BLOQUES_MAX; i++) strcat(t, "IF (L) THEN\n");
    for (i = 0; i < SEM_BLOQUES_MAX; i++) strcat(t, "ENDIF\n");
    strcat(t, "END");
    LIMPIO(t);
    strcpy(t, "LOGICAL L\n");
    for (i = 0; i < SEM_BLOQUES_MAX + 1; i++) strcat(t, "IF (L) THEN\n");
    for (i = 0; i < SEM_BLOQUES_MAX + 1; i++) strcat(t, "ENDIF\n");
    strcat(t, "END");
    ERRSEM(t, 2, SEM_BLOQUES_PROFUNDOS, SEM_BLOQUES_MAX + 2, 7);
    COMPROBAR(mf.diag[1].cod == SEM_SIN_IF);

    /* demasiadas etiquetas */
    t[0] = '\0';
    for (i = 1; i <= SEM_ETIQUETAS_MAX + 1; i++) {
        sprintf(t + strlen(t), "GOTO %u\n", i);
    }
    programa(t);
    COMPROBAR(mf.n == 1 && mf.diag[0].cod == SEM_MUCHAS_ETIQUETAS
              && mf.diag[0].linea == SEM_ETIQUETAS_MAX + 1);

    /* falta el END */
    programa("X = 1");
    sem_fin_fichero(1);
    COMPROBAR(mf.n == 1 && mf.diag[0].cod == SEM_FALTA_END && mf.diag[0].linea == 1);
    programa("X = 1\nEND");
    sem_fin_fichero(2);
    COMPROBAR(mf.n == 0 && sem.fin == 1);
}

static void test_acumulacion(void)
{
    unsigned i;

    /* tres errores en tres sentencias: se siguen contando */
    ERRSEM("LOGICAL L\nL = 1\nX = L\nI = .TRUE.", 3, SEM_INCOMPATIBLES, 2, 7);
    COMPROBAR(mf.diag[1].cod == SEM_NUMERICO_ESPERADO || mf.diag[1].cod == SEM_INCOMPATIBLES);
    COMPROBAR(mf.diag[1].linea == 3 && mf.diag[2].linea == 4);

    /* el mismo error no se repite en cascada: B ya es REAL */
    programa("LOGICAL L\nX = L + 1 + 2 + 3");
    COMPROBAR(mf.n == 1);

    /* mas errores que MF_DIAG_MAX: se cuentan todos, solo se guardan N */
    {
        char t[2048] = "LOGICAL L\n";

        for (i = 0; i < MF_DIAG_MAX + 5; i++) {
            strcat(t, "L = 1\n");
        }
        programa(t);
        COMPROBAR(mf.n == MF_DIAG_MAX + 5);
        COMPROBAR(mf.diag[MF_DIAG_MAX - 1].linea == MF_DIAG_MAX + 1);
    }

    /* sem_sentencia no hace nada tras un error de sintaxis */
    programa("X = 1");
    {
        Fuente f;
        unsigned antes = mf.n;

        fuente_iniciar_texto(&f, "      X = (1\n");
        leer_sentencia(&f);
        stmt_analizar();
        COMPROBAR(stm.err != STM_OK);
        sem_sentencia();
        COMPROBAR(mf.n == antes);
    }

    /* sem_iniciar vacia errores y simbolos */
    sem_iniciar();
    COMPROBAR(mf.n == 0 && sim_cuenta() == 0);

    /* un error de sintaxis tardio en una lista no llega al semantico como error propio */
    programa("INTEGER A, , B");
    COMPROBAR(sintaxis == 1);

    /* todos los mensajes existen */
    for (i = 0; i <= SEM_FALTA_END; i++) {
        COMPROBAR(strcmp(sem_mensaje((ErrorSem)i), "unknown error") != 0);
    }
}

/* Tipo de la expresion (lado derecho) de la ULTIMA sentencia, que debe ser una asignacion. */
static uint8_t tipo_ult(const char *prog)
{
    programa(prog);
    if (stm.tipo != SENT_ASIGNACION || mf.n != 0 || sintaxis != 0) {
        return 99;
    }
    return ast_tdato(stm.b);
}

static void test_intrinsecas(void)
{
    /* usos correctos */
    LIMPIO("X = SQRT(2.0)\nI = MOD(7, 3)\nX = MAX(1.0, X, 3.0)\nI = MAX0(1, 2, I)");
    LIMPIO("LOGICAL L\nCHARACTER*3 S\nL = LGE('A', 'B')\nS = CHAR(65)\nI = INDEX('ABC', 'B') + LEN('XY')");
    LIMPIO("DOUBLE PRECISION D\nD = DSQRT(2D0)\nD = DPROD(X, X)\nX = SQRT(ABS(X))");
    LIMPIO("REAL A(5)\nINTEGER K(5)\nX = SQRT(A(I))\nX = MAX(A(1), A(2))\nI = K(ABS(I) + 1)");
    LIMPIO("X = SIN(X)**2 + COS(X)**2\nI = INT(X) + NINT(X) + IFIX(X)\nX = FLOAT(I) + REAL(I)");
    /* tipo del resultado */
    COMPROBAR(tipo_ult("X = ABS(I)") == TD_INTEGER);
    COMPROBAR(tipo_ult("X = ABS(X)") == TD_REAL);
    COMPROBAR(tipo_ult("DOUBLE PRECISION D\nX = ABS(D)") == TD_DOUBLE);
    COMPROBAR(tipo_ult("X = SQRT(X)") == TD_REAL);
    COMPROBAR(tipo_ult("X = INT(X)") == TD_INTEGER);
    COMPROBAR(tipo_ult("X = DBLE(I)") == TD_DOUBLE);
    COMPROBAR(tipo_ult("X = REAL(I)") == TD_REAL);
    COMPROBAR(tipo_ult("X = MOD(I, 2)") == TD_INTEGER);
    COMPROBAR(tipo_ult("X = MOD(X, 2.0)") == TD_REAL);
    COMPROBAR(tipo_ult("X = AMAX0(I, 2)") == TD_REAL);
    COMPROBAR(tipo_ult("X = MAX1(X, 2.0)") == TD_INTEGER);
    COMPROBAR(tipo_ult("LOGICAL L\nL = LGT('A', 'B')") == TD_LOGICAL);
    COMPROBAR(tipo_ult("CHARACTER*2 S\nS = CHAR(I)") == TD_CHARACTER);
    COMPROBAR(tipo_ult("X = ICHAR('A')") == TD_INTEGER);
    COMPROBAR(tipo_ult("X = SIGN(X, -1.0)") == TD_REAL);
    COMPROBAR(tipo_ult("X = SQRT(2.0) * 3") == TD_REAL);
    /* se usa el nombre en mayusculas/minusculas igual: el lexer pasa a mayusculas */
    COMPROBAR(tipo_ult("x = sqrt(x)") == TD_REAL);
    /* errores */
    ERRSEM("X = SQRT(I)", 1, SEM_INTR_TIPO, 1, 16);
    ERRSEM("X = SQRT('A')", 1, SEM_INTR_TIPO, 1, 16);
    ERRSEM("X = MOD(1, 2.0)", 1, SEM_INTR_TIPO, 1, 18);
    ERRSEM("X = MAX(1, 2.0)", 1, SEM_INTR_TIPO, 1, 18);
    ERRSEM("X = MAX0(1, 2.0)", 1, SEM_INTR_TIPO, 1, 19);
    ERRSEM("X = ABS(1, 2)", 1, SEM_INTR_ARGS, 1, 11);
    ERRSEM("X = MAX(1)", 1, SEM_INTR_ARGS, 1, 11);
    ERRSEM("X = ATAN2(X)", 1, SEM_INTR_ARGS, 1, 11);
    ERRSEM("X = LEN(1)", 1, SEM_INTR_TIPO, 1, 15);
    ERRSEM("X = INT('A')", 1, SEM_INTR_TIPO, 1, 15);
    ERRSEM("X = DBLE(.TRUE.)", 1, SEM_INTR_TIPO, 1, 16);
    ERRSEM("X = CHAR(1.5)", 1, SEM_INTR_TIPO, 1, 16);
    ERRSEM("X = SQRT(1:2)", 1, SEM_RANGO_NO_PERMITIDO, 1, 16);
    ERRSEM("SQRT(1.0) = 2", 1, SEM_NO_ES_ARRAY, 1, 7);
    ERRSEM("LOGICAL L\nL = SQRT(2.0)", 1, SEM_INCOMPATIBLES, 2, 7);
    ERRSEM("X = SQRT(Y + 'A')", 1, SEM_NUMERICO_ESPERADO, 1, 18);   /* un solo error */
    /* las funciones del usuario (FOO(1)) se anotan y se comprueban al final del fichero: ver test_cap09.c */
}

/* Cada entrada de la tabla, llamada con argumentos validos, da su tipo; con uno
 * de menos o de mas (si cabe) da error de numero de argumentos. */
static void test_tabla_intrinsecas(void)
{
    uint8_t i, j;
    char prog[200], args[120];
    const char *lit;
    uint8_t esperado;

    for (i = 0; i < intr_total(); i++) {
        const Intrinseca *a = intr_numero(i);
        uint8_t n;

        /* nombre en mayusculas, unico, con minimo <= maximo */
        COMPROBAR(a->nombre[0] >= 'A' && a->nombre[0] <= 'Z' && a->nmin >= 1 && a->nmin <= a->nmax);
        COMPROBAR(strlen(a->nombre) <= NOMBRE_MAX);
        COMPROBAR(intr_buscar(a->nombre) == a);
        for (j = (uint8_t)(i + 1); j < intr_total(); j++) {
            COMPROBAR(strncmp(a->nombre, intr_numero(j)->nombre, NOMBRE_MAX) != 0);
        }
        switch (a->atipo) {
        case TD_INTEGER:   lit = "1"; break;
        case TD_DOUBLE:    lit = "1D0"; break;
        case TD_CHARACTER: lit = "'A'"; break;
        default:           lit = "1.0"; break;     /* NUM, FLT, NUMX, TD_REAL */
        }
        esperado = a->rtipo == INTR_R_ARG
                   ? (a->atipo >= INTR_A_NUM ? TD_REAL : a->atipo) : a->rtipo;
        for (n = a->nmin; n <= a->nmax && n <= (uint8_t)(a->nmin + 1); n++) {
            args[0] = '\0';
            for (j = 0; j < n; j++) {
                if (j) strcat(args, ", ");
                strcat(args, lit);
            }
            sprintf(prog, "LOGICAL L\nCHARACTER*2 S\nDOUBLE PRECISION D\n%s = %s(%s)",
                    esperado == TD_INTEGER ? "I" : esperado == TD_DOUBLE ? "D"
                    : esperado == TD_LOGICAL ? "L" : esperado == TD_CHARACTER ? "S" : "X",
                    a->nombre, args);
            COMPROBAR(tipo_ult(prog) == esperado);
        }
        /* uno de menos */
        if (a->nmin > 1) {
            args[0] = '\0';
            for (j = 0; j + 1 < a->nmin; j++) {
                if (j) strcat(args, ", ");
                strcat(args, lit);
            }
            sprintf(prog, "X = %s(%s)", a->nombre, args);
            programa(prog);
            COMPROBAR(mf.n == 1 && mf.diag[0].cod == SEM_INTR_ARGS);
        }
        /* uno de mas */
        args[0] = '\0';
        for (j = 0; j < (uint8_t)(a->nmax + 1); j++) {
            if (j) strcat(args, ", ");
            strcat(args, lit);
        }
        if (strlen(args) < 50) {
            sprintf(prog, "X = %s(%s)", a->nombre, args);
            programa(prog);
            COMPROBAR(mf.n == 1 && mf.diag[0].cod == SEM_INTR_ARGS);
        }
    }
}

int main(void)
{
    test_registro();
    test_alta_busqueda();
    test_nombres();
    test_llena();
    test_implicito();
    test_declaraciones();
    test_tipos_expresion();
    test_tdato();
    test_intrinsecas();
    test_tabla_intrinsecas();
    test_flags();
    test_arrays();
    test_cadenas();
    test_control();
    test_bloques();
    test_acumulacion();
    printf("%d pruebas, %d fallos\n", pruebas, fallos);
    return fallos != 0;
}
