/* test_cap06.c - Pruebas del generador de TAC (gen.c, tac.c, tac_lst.c) con el interprete tacvm.
 *
 * Tres tipos de prueba:
 *   LISTADO  el texto del TAC es exactamente el esperado (la referencia es docs/especificacion-tac.md)
 *   SALIDA   el TAC, ejecutado en tacvm, imprime lo que imprimiria Fortran
 *   ERROR    el compilador anota el error esperado (linea, columna, codigo)
 * y un diferencial aleatorio: expresiones enteras y logicas generadas al azar se evaluan
 * en C y en tacvm y deben dar lo mismo.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mf_api.h"
#include "gen.h"
#include "tac.h"
#include "sem.h"
#include "texto.h"
#include "banco.h"
#include "tacvm.h"
#include "unid.h"

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

static char lst[262144];

static void sal(const char *l)
{
    strcat(lst, l);
    strcat(lst, "\n");
}

/* Compila; si no hay errores deja el listado en lst. Devuelve el numero de errores. */
static int compilar(const char *prog)
{
    int e;

    banco_iniciar();
    texto_poner(prog);
    e = (int)mf_compilar();
    lst[0] = '\0';
    if (e == 0) {
        tac_listar(sal);
    }
    return e;
}

static void mostrar_errores(void)
{
    int i;

    for (i = 0; i < mf.n && i < MF_DIAG_MAX; i++) {
        printf("   error %u:%u cod %u\n", (unsigned)mf.diag[i].linea,
               (unsigned)mf.diag[i].columna, (unsigned)mf.diag[i].cod);
    }
}

static int es_listado(int linea_test, const char *prog, const char *esperado)
{
    char msg[96];
    int ok;

    pruebas++;
    if (compilar(prog) != 0) {
        printf("FALLO (linea %d): errores al compilar\n", linea_test);
        mostrar_errores();
        fallos++;
        return 0;
    }
    ok = strcmp(lst, esperado) == 0;
    if (!ok) {
        printf("FALLO (linea %d): listado distinto\n--- obtenido\n%s--- esperado\n%s", linea_test, lst, esperado);
        fallos++;
        return 0;
    }
    if (tacvm_validar(msg, sizeof(msg)) != 0) {
        printf("FALLO (linea %d): TAC invalido: %s\n", linea_test, msg);
        fallos++;
        return 0;
    }
    return 1;
}

static int es_salida(int linea_test, const char *prog, const char *esperado)
{
    char msg[96];
    char out[4096];
    TacVmRes r;

    pruebas++;
    if (compilar(prog) != 0) {
        printf("FALLO (linea %d): errores al compilar\n", linea_test);
        mostrar_errores();
        fallos++;
        return 0;
    }
    if (tacvm_validar(msg, sizeof(msg)) != 0) {
        printf("FALLO (linea %d): TAC invalido: %s\n%s", linea_test, msg, lst);
        fallos++;
        return 0;
    }
    tacvm_ejecutar(out, sizeof(out), 2000000, &r);
    if (r.estado == TACVM_ERROR) {
        printf("FALLO (linea %d): error de ejecucion: %s\n", linea_test, r.msg);
        fallos++;
        return 0;
    }
    if (strcmp(out, esperado) != 0) {
        printf("FALLO (linea %d): salida distinta\n--- obtenida\n%s--- esperada\n%s", linea_test, out, esperado);
        fallos++;
        return 0;
    }
    return 1;
}

static int es_error(int linea_test, const char *prog, uint16_t lin, uint8_t col, uint8_t cod)
{
    pruebas++;
    if (compilar(prog) == 0) {
        printf("FALLO (linea %d): deberia dar error\n", linea_test);
        fallos++;
        return 0;
    }
    if (mf.n != 1 || mf.diag[0].linea != lin || mf.diag[0].columna != col || mf.diag[0].cod != cod) {
        printf("FALLO (linea %d): error distinto (esperado %u:%u cod %u)\n", linea_test,
               (unsigned)lin, (unsigned)col, (unsigned)cod);
        mostrar_errores();
        fallos++;
        return 0;
    }
    return 1;
}

#define LISTADO(p, e) es_listado(__LINE__, p, e)
#define SALIDA(p, e)  es_salida(__LINE__, p, e)
#define ERROR(p, l, c, k) es_error(__LINE__, p, l, c, k)

/* ------------------------------------------------------------------
 * Listados: los ejemplos de la especificacion y cada construccion
 * ------------------------------------------------------------------ */

static void test_listados(void)
{
    /* spec 9.1 */
    LISTADO("      INTEGER I, S\n"
            "      S = 0\n"
            "      DO 10 I = 1, 10\n"
            "         S = S + I\n"
            "   10 CONTINUE\n"
            "      PRINT *, S\n"
            "      END\n",
            "UNIT PROGRAM MAIN\n"
            "VAR I INTEGER\n"
            "VAR S INTEGER\n"
            "        MOV.I  S, 0\n"
            "        MOV.I  $0, 1\n"
            "        MOV.I  $1, 10\n"
            "        MOV.I  I, 1\n"
            "        LBL    @@1\n"
            "        GT.I   %0, $1, 0\n"
            "        JF     %0, @@2\n"
            "        ADD.I  S, S, I\n"
            "        LBL    @10\n"
            "        ADD.I  I, I, $0\n"
            "        SUB.I  $1, $1, 1\n"
            "        JMP    @@1\n"
            "        LBL    @@2\n"
            "        PUT.I  S\n"
            "        PUTNL\n"
            "        HALT\n");

    /* spec 9.2 */
    LISTADO("      CHARACTER*8 A\n"
            "      A = 'HI' // 'X'\n"
            "      IF (A .EQ. 'HIX') PRINT *, 'OK'\n"
            "      END\n",
            "UNIT PROGRAM MAIN\n"
            "VAR A CHARACTER*8\n"
            "        TMP.C  %0, 3\n"
            "        SCAT.C %0, 'HI', 'X'\n"
            "        MOV.C  A, %0\n"
            "        EQ.C   %0, A, 'HIX'\n"
            "        JF     %0, @@1\n"
            "        PUT.C  'OK'\n"
            "        PUTNL\n"
            "        LBL    @@1\n"
            "        HALT\n");

    /* spec 9.3 */
    LISTADO("      INTEGER V(10)\n"
            "      V(3) = 7\n"
            "      I = V(3) + 1\n"
            "      END\n",
            "UNIT PROGRAM MAIN\n"
            "VAR V INTEGER (1:10)\n"
            "VAR I INTEGER\n"
            "        STE.I  V, 2, 7\n"
            "        LDE.I  %0, V, 2\n"
            "        ADD.I  I, %0, 1\n"
            "        HALT\n");

    /* nombre del programa, STOP, END sin nada */
    LISTADO("      PROGRAM HOLA\n"
            "      STOP 'FIN'\n"
            "      END\n",
            "UNIT PROGRAM HOLA\n"
            "        STOP   'FIN'\n"
            "        HALT\n");

    /* escritura directa en el destino y MOV */
    LISTADO("      I = 5\n"
            "      J = I\n"
            "      K = -J\n"
            "      L = (I + J) * K\n"
            "      END\n",
            "UNIT PROGRAM MAIN\n"
            "VAR I INTEGER\n"
            "VAR J INTEGER\n"
            "VAR K INTEGER\n"
            "VAR L INTEGER\n"
            "        MOV.I  I, 5\n"
            "        MOV.I  J, I\n"
            "        NEG.I  K, J\n"
            "        ADD.I  %0, I, J\n"
            "        MUL.I  L, %0, K\n"
            "        HALT\n");

    /* constante negativa y entero que no cabe en INM (CTE) */
    LISTADO("      I = -5\n"
            "      J = 30000\n"
            "      K = -32768\n"
            "      END\n",
            "UNIT PROGRAM MAIN\n"
            "VAR I INTEGER\n"
            "VAR J INTEGER\n"
            "VAR K INTEGER\n"
            "        MOV.I  I, -5\n"
            "        MOV.I  J, 30000\n"
            "        MOV.I  K, -32768\n"
            "        HALT\n");

    /* IF / ELSE IF / ELSE / END IF */
    LISTADO("      N = 3\n"
            "      IF (N .GT. 5) THEN\n"
            "         N = 1\n"
            "      ELSE IF (N .GT. 2) THEN\n"
            "         N = 2\n"
            "      ELSE\n"
            "         N = 3\n"
            "      END IF\n"
            "      END\n",
            "UNIT PROGRAM MAIN\n"
            "VAR N INTEGER\n"
            "        MOV.I  N, 3\n"
            "        GT.I   %0, N, 5\n"
            "        JF     %0, @@1\n"
            "        MOV.I  N, 1\n"
            "        JMP    @@2\n"
            "        LBL    @@1\n"
            "        GT.I   %0, N, 2\n"
            "        JF     %0, @@3\n"
            "        MOV.I  N, 2\n"
            "        JMP    @@2\n"
            "        LBL    @@3\n"
            "        MOV.I  N, 3\n"
            "        LBL    @@2\n"
            "        HALT\n");

    /* IF sin ELSE: solo la etiqueta de la rama siguiente */
    LISTADO("      IF (N .EQ. 0) THEN\n"
            "         N = 1\n"
            "      END IF\n"
            "      END\n",
            "UNIT PROGRAM MAIN\n"
            "VAR N INTEGER\n"
            "        EQ.I   %0, N, 0\n"
            "        JF     %0, @@1\n"
            "        MOV.I  N, 1\n"
            "        LBL    @@1\n"
            "        HALT\n");

    /* IF aritmetico y GOTO */
    LISTADO("      IF (N) 10, 20, 30\n"
            "   10 GOTO 40\n"
            "   20 CONTINUE\n"
            "   30 CONTINUE\n"
            "   40 END\n",
            "UNIT PROGRAM MAIN\n"
            "VAR N INTEGER\n"
            "        LT.I   %0, N, 0\n"
            "        JT     %0, @10\n"
            "        EQ.I   %1, N, 0\n"
            "        JT     %1, @20\n"
            "        JMP    @30\n"
            "        LBL    @10\n"
            "        JMP    @40\n"
            "        LBL    @20\n"
            "        LBL    @30\n"
            "        LBL    @40\n"
            "        HALT\n");

    /* GOTO calculado */
    LISTADO("      GOTO (10, 20, 10), N\n"
            "   10 CONTINUE\n"
            "   20 END\n",
            "UNIT PROGRAM MAIN\n"
            "VAR N INTEGER\n"
            "        JEQ    N, 1, @10\n"
            "        JEQ    N, 2, @20\n"
            "        JEQ    N, 3, @10\n"
            "        LBL    @10\n"
            "        LBL    @20\n"
            "        HALT\n");

    /* arrays: 1-D con limite inferior distinto de 1, 2-D por columnas */
    LISTADO("      INTEGER A(0:4), M(2:3, 0:1)\n"
            "      A(I) = M(J, K)\n"
            "      END\n",
            "UNIT PROGRAM MAIN\n"
            "VAR A INTEGER (0:4)\n"
            "VAR M INTEGER (2:3,0:1)\n"
            "VAR I INTEGER\n"
            "VAR J INTEGER\n"
            "VAR K INTEGER\n"
            "        SUB.I  %0, J, 2\n"
            "        MUL.I  %1, K, 2\n"
            "        ADD.I  %2, %0, %1\n"
            "        LDE.I  %3, M, %2\n"
            "        STE.I  A, I, %3\n"
            "        HALT\n");

    /* subcadena y concatenacion de tres */
    LISTADO("      CHARACTER*5 S, T\n"
            "      T = S(2:4) // S // 'Z'\n"
            "      END\n",
            "UNIT PROGRAM MAIN\n"
            "VAR S CHARACTER*5\n"
            "VAR T CHARACTER*5\n"
            "        TMP.C  %0, 3\n"
            "        SSUB.C %0, S, 2\n"
            "        EXT    4\n"
            "        TMP.C  %1, 8\n"
            "        SCAT.C %1, %0, S\n"
            "        TMP.C  %2, 9\n"
            "        SCAT.C %2, %1, 'Z'\n"
            "        MOV.C  T, %2\n"
            "        HALT\n");

    /* IF logico con sentencia terminal de un DO: el IF y el cierre del bucle */
    LISTADO("      DO 10 I = 1, N\n"
            "   10 IF (I .GT. 2) K = K + 1\n"
            "      END\n",
            "UNIT PROGRAM MAIN\n"
            "VAR I INTEGER\n"
            "VAR N INTEGER\n"
            "VAR K INTEGER\n"
            "        MOV.I  $0, 1\n"
            "        MOV.I  $1, N\n"
            "        MOV.I  I, 1\n"
            "        LBL    @@1\n"
            "        GT.I   %0, $1, 0\n"
            "        JF     %0, @@2\n"
            "        LBL    @10\n"
            "        GT.I   %0, I, 2\n"
            "        JF     %0, @@3\n"
            "        ADD.I  K, K, 1\n"
            "        LBL    @@3\n"
            "        ADD.I  I, I, $0\n"
            "        SUB.I  $1, $1, 1\n"
            "        JMP    @@1\n"
            "        LBL    @@2\n"
            "        HALT\n");
}

/* ------------------------------------------------------------------
 * Ejecucion: el TAC hace lo que dice Fortran
 * ------------------------------------------------------------------ */

static void test_ejecucion(void)
{
    SALIDA("      INTEGER I, S\n      S = 0\n      DO 10 I = 1, 10\n         S = S + I\n   10 CONTINUE\n      PRINT *, S\n      END\n",
           " 55\n");

    /* factorial: 7! = 5040 */
    SALIDA("      N = 1\n      DO 10 I = 1, 7\n   10 N = N * I\n      PRINT *, N\n      END\n", " 5040\n");

    /* la variable del DO vale el primer valor que no se ejecuta */
    SALIDA("      DO 10 I = 1, 3\n   10 CONTINUE\n      PRINT *, I\n      END\n", " 4\n");

    /* cero iteraciones, paso 2, paso negativo, paso que no llega al final */
    SALIDA("      K = 0\n      DO 10 I = 10, 1\n   10 K = K + 1\n      PRINT *, K, I\n      END\n", " 0 10\n");
    SALIDA("      K = 0\n      DO 10 I = 1, 10, 2\n   10 K = K + I\n      PRINT *, K, I\n      END\n", " 25 11\n");
    SALIDA("      K = 0\n      DO 10 I = 10, 1, -3\n   10 K = K + I\n      PRINT *, K, I\n      END\n", " 22 -2\n");
    SALIDA("      K = 0\n      DO 10 I = 1, 10, 4\n   10 K = K + 1\n      PRINT *, K, I\n      END\n", " 3 13\n");

    /* los limites se evaluan UNA vez: cambiar N dentro del bucle no cambia las vueltas */
    SALIDA("      N = 3\n      K = 0\n      DO 10 I = 1, N\n      N = N + 1\n   10 K = K + 1\n      PRINT *, K, N\n      END\n",
           " 3 6\n");

    /* limite que usa la propia variable: DO I = 1, I + 2 con I=5 -> inicio 1, fin 7 */
    SALIDA("      I = 5\n      K = 0\n      DO 10 I = 1, I + 2\n   10 K = K + 1\n      PRINT *, K\n      END\n", " 7\n");

    /* bucles anidados con la misma etiqueta final */
    SALIDA("      N = 0\n      DO 10 I = 1, 3\n      DO 10 J = 1, 3\n   10 IF (J .NE. 2) N = N + I * J\n      PRINT *, N\n      END\n",
           " 24\n");

    /* bucles anidados con etiquetas distintas y variables ocultas independientes */
    SALIDA("      N = 0\n      DO 20 I = 1, 4\n      DO 10 J = I, 4\n   10 N = N + 1\n   20 CONTINUE\n      PRINT *, N\n      END\n",
           " 10\n");

    /* tabla de multiplicar con PRINT de varias columnas */
    SALIDA("      DO 10 I = 1, 3\n      PRINT *, I, I * I, I ** 3\n   10 CONTINUE\n      END\n",
           " 1 1 1\n 2 4 8\n 3 9 27\n");

    /* Fibonacci con array */
    SALIDA("      INTEGER F(12)\n      F(1) = 1\n      F(2) = 1\n      DO 10 I = 3, 12\n   10 F(I) = F(I - 1) + F(I - 2)\n      PRINT *, F(12), F(7)\n      END\n",
           " 144 13\n");

    /* criba de Eratostenes con array LOGICAL */
    SALIDA("      LOGICAL P(30)\n"
           "      DO 10 I = 2, 30\n"
           "   10 P(I) = .TRUE.\n"
           "      DO 30 I = 2, 5\n"
           "      IF (.NOT. P(I)) GOTO 30\n"
           "      DO 20 J = 2 * I, 30, I\n"
           "   20 P(J) = .FALSE.\n"
           "   30 CONTINUE\n"
           "      DO 40 I = 2, 30\n"
           "   40 IF (P(I)) PRINT *, I\n"
           "      END\n",
           " 2\n 3\n 5\n 7\n 11\n 13\n 17\n 19\n 23\n 29\n");

    /* matriz 2-D con limites inferiores distintos: M(2:4, 0:2) */
    SALIDA("      INTEGER M(2:4, 0:2)\n"
           "      DO 20 I = 2, 4\n"
           "      DO 10 J = 0, 2\n"
           "   10 M(I, J) = 10 * I + J\n"
           "   20 CONTINUE\n"
           "      PRINT *, M(2, 0), M(4, 0), M(2, 2), M(3, 1), M(4, 2)\n"
           "      END\n",
           " 20 40 22 31 42\n");

    /* IF / ELSE IF / ELSE con tres valores de N */
    SALIDA("      DO 10 N = 1, 7, 3\n"
           "      IF (N .GT. 5) THEN\n        PRINT *, 'BIG'\n"
           "      ELSE IF (N .GT. 2) THEN\n        PRINT *, 'MID'\n"
           "      ELSE\n        PRINT *, 'SMALL'\n"
           "      END IF\n"
           "   10 CONTINUE\n"
           "      END\n",
           " SMALL\n MID\n BIG\n");

    /* IF anidados y cadena larga de ELSE IF */
    SALIDA("      DO 10 N = 0, 4\n"
           "      IF (N .EQ. 0) THEN\n        PRINT *, 'ZERO'\n"
           "      ELSE IF (N .EQ. 1) THEN\n        PRINT *, 'ONE'\n"
           "      ELSE IF (N .EQ. 2) THEN\n        PRINT *, 'TWO'\n"
           "      ELSE\n"
           "        IF (N .EQ. 3) THEN\n          PRINT *, 'THREE'\n"
           "        ELSE\n          PRINT *, 'MANY'\n        END IF\n"
           "      END IF\n"
           "   10 CONTINUE\n"
           "      END\n",
           " ZERO\n ONE\n TWO\n THREE\n MANY\n");

    /* GOTO: bucle hecho a mano; GOTO calculado; IF aritmetico */
    SALIDA("      N = 0\n   10 N = N + 1\n      IF (N .LT. 5) GOTO 10\n      PRINT *, N\n      END\n", " 5\n");
    SALIDA("      DO 5 N = 0, 4\n      GOTO (10, 20, 30), N\n      PRINT *, 'FUERA'\n      GOTO 5\n"
           "   10 PRINT *, 'UNO'\n      GOTO 5\n   20 PRINT *, 'DOS'\n      GOTO 5\n   30 PRINT *, 'TRES'\n    5 CONTINUE\n      END\n",
           " FUERA\n UNO\n DOS\n TRES\n FUERA\n");
    SALIDA("      DO 5 N = -1, 1\n      IF (N) 10, 20, 30\n   10 PRINT *, 'NEG'\n      GOTO 5\n"
           "   20 PRINT *, 'CERO'\n      GOTO 5\n   30 PRINT *, 'POS'\n    5 CONTINUE\n      END\n",
           " NEG\n CERO\n POS\n");

    /* aritmetica de 16 bits: da la vuelta, division hacia cero, potencias */
    SALIDA("      I = 32767\n      J = I + 1\n      K = 200 * 200\n      PRINT *, J, K\n      END\n", " -32768 -25536\n");
    SALIDA("      PRINT *, -7 / 2, 7 / (-2), -7 / (-2), 7 / 2\n      END\n", " -3 -3 3 3\n");
    SALIDA("      PRINT *, 2 ** 10, (-2) ** 3, 2 ** (-1), (-1) ** (-3)\n      PRINT *, 1 ** (-5), 0 ** 0, 3 ** 0\n      END\n",
           " 1024 -8 0 -1\n 1 1 1\n");
    SALIDA("      PRINT *, 2 + 3 * 4, (2 + 3) * 4, 2 ** 3 ** 2\n"
           "      PRINT *, -2 ** 2, 10 - 4 - 3, 100 / 10 / 5\n      END\n",
           " 14 20 512\n -4 3 2\n");

    /* logicos: tablas de verdad y precedencia (.NOT. > .AND. > .OR. > .EQV./.NEQV.) */
    SALIDA("      LOGICAL A, B\n      A = .TRUE.\n      B = .FALSE.\n"
           "      PRINT *, A .AND. B, A .OR. B, .NOT. A, A .EQV. B, A .NEQV. B\n"
           "      PRINT *, A .OR. B .AND. B, .NOT. B .AND. A\n"
           "      PRINT *, A .EQV. B .OR. A\n      END\n",
           " F T F F T\n T T\n T\n");
    SALIDA("      PRINT *, 1 .LT. 2, 1 .LE. 1, 2 .GT. 3\n"
           "      PRINT *, 3 .GE. 4, 5 .EQ. 5, 5 .NE. 5\n      END\n",
           " T T F\n F T F\n");

    /* CHARACTER: asignacion con relleno y truncado, concatenacion, subcadena, comparacion */
    SALIDA("      CHARACTER*5 A\n      CHARACTER*3 B\n      A = 'AB'\n      B = 'WXYZ'\n      PRINT *, A, B\n      END\n",
           " AB    WXY\n");
    SALIDA("      CHARACTER*8 A\n      A = 'HI' // 'X'\n      IF (A .EQ. 'HIX') PRINT *, 'OK'\n      PRINT *, A(2:3), A(:2), A(8:)\n      END\n",
           " OK\n IX HI  \n");
    SALIDA("      CHARACTER*4 A, B\n      A = 'ABC'\n      B = 'ABD'\n"
           "      PRINT *, A .LT. B, A .EQ. 'ABC', A .NE. 'ABC ', A .GT. 'AB'\n      END\n",
           " T T F T\n");
    SALIDA("      CHARACTER*3 C(2)\n      LOGICAL L\n      C(2) = 'AB'\n      C(1) = C(2) // 'XYZ'\n"
           "      L = C(1) .GT. C(2)\n      PRINT *, C(1), C(2), L\n      PRINT *, 'A''B', C(2)(2:2)\n      END\n",
           " AB  AB  F\n A'B B\n");
    SALIDA("      CHARACTER*6 S\n      S = 'ABCDEF'\n      DO 10 I = 1, 6\n   10 PRINT *, S(I:I)\n      END\n",
           " A\n B\n C\n D\n E\n F\n");
    SALIDA("      CHARACTER*4 S\n      S = 'ABCD'\n      I = 2\n      PRINT *, S(I:I+1), S(I+1:I)\n      END\n",
           " BC \n");

    /* funciones intrinsecas enteras (se expanden en TAC) */
    SALIDA("      PRINT *, ABS(5), ABS(-5), ABS(0), IABS(-32767), ABS(-32768)\n      END\n",
           " 5 5 0 32767 -32768\n");
    SALIDA("      I = -9\n      J = 4\n      PRINT *, ABS(I), ABS(J), ABS(I - J), ABS(J - I)\n      END\n",
           " 9 4 13 13\n");
    SALIDA("      PRINT *, MOD(7, 3), MOD(-7, 3), MOD(7, -3), MOD(-7, -3)\n"
           "      PRINT *, MOD(6, 3), MOD(0, 5)\n      END\n",
           " 1 -1 1 -1\n 0 0\n");
    SALIDA("      I = 17\n      J = 5\n      PRINT *, MOD(I, J), MOD(-I, J)\n"
           "      PRINT *, MOD(I, -J), MOD(J, I)\n      END\n",
           " 2 -2\n 2 5\n");
    SALIDA("      I = 5\n      J = -5\n      PRINT *, SIGN(3, 1), SIGN(3, -1), SIGN(-3, 1)\n"
           "      PRINT *, SIGN(-3, -1), SIGN(3, 0)\n"
           "      PRINT *, ISIGN(I, J), ISIGN(J, I), ISIGN(J, J), ISIGN(I, 0)\n      END\n",
           " 3 -3 3\n -3 3\n -5 5 -5 5\n");
    SALIDA("      I = 7\n      J = 3\n      PRINT *, DIM(7, 3), DIM(3, 7), DIM(4, 4)\n"
           "      PRINT *, IDIM(I, J), IDIM(J, I), IDIM(-2, -9)\n      END\n",
           " 4 0 0\n 4 0 7\n");
    SALIDA("      I = 4\n      J = -2\n      K = 9\n"
           "      PRINT *, MAX(I, J), MIN(I, J), MAX0(J, K, I), MIN0(K, J, I)\n"
           "      PRINT *, MAX(1, 2, 3, 4, 5), MAX(5, 4, 3, 2, 1)\n"
           "      PRINT *, MIN(5, 4, 3, 2, 1), MIN(1, 2, 3, 4, 5)\n"
           "      PRINT *, MAX(-1, -2), MIN(-1, -2), MAX(3, 3), MIN0(7, 7, 7)\n      END\n",
           " 4 -2 9 -2\n 5 5\n 1 1\n -1 -2 3 7\n");
    SALIDA("      INTEGER A(3)\n      A(1) = 5\n      A(2) = -8\n      A(3) = 2\n      N = 0\n"
           "      DO 10 I = 1, 3\n      N = MAX(N, ABS(A(I)))\n   10 CONTINUE\n"
           "      PRINT *, N, MOD(ABS(A(2)), A(3)), MIN(A(1), A(2), A(3))\n      END\n",
           " 8 0 -8\n");
    SALIDA("      I = 10\n      IF (MOD(I, 2) .EQ. 0) PRINT *, 'PAR'\n"
           "      IF (ABS(I - 20) .GT. 5) PRINT *, 'LEJOS'\n"
           "      DO 20 J = MIN(3, 5), MAX(4, 1)\n   20 PRINT *, J\n      END\n",
           " PAR\n LEJOS\n 3\n 4\n");
    SALIDA("      PRINT *, INT(5), INT(-3) + 1\n"
           "      PRINT *, ABS(MAX(-7, MIN(2, -9))), MOD(MAX(10, 3), MIN(4, 7))\n      END\n",
           " 5 -2\n 7 2\n");
    /* LEN: longitud conocida al compilar */
    SALIDA("      CHARACTER*7 S\n      CHARACTER*3 T(4)\n      S = 'AB'\n"
           "      PRINT *, LEN(S), LEN(T(2)), LEN('HOLA'), LEN(S // 'XY')\n"
           "      PRINT *, LEN(T(1) // T(2) // S), LEN((S))\n"
           "      I = LEN(S) + 1\n      PRINT *, I, LEN(S) * 2\n      END\n",
           " 7 3 4 9\n 13 7\n 8 14\n");

    /* STOP: con numero, con texto, y cortando el programa */
    {
        char msg[96], out[256];
        TacVmRes r;

        COMPROBAR(compilar("      PRINT *, 'A'\n      STOP 7\n      PRINT *, 'B'\n      END\n") == 0);
        COMPROBAR(tacvm_validar(msg, sizeof(msg)) == 0);
        tacvm_ejecutar(out, sizeof(out), 1000, &r);
        COMPROBAR(r.estado == TACVM_STOP && strcmp(r.msg, "7") == 0 && strcmp(out, " A\n") == 0);
        COMPROBAR(compilar("      STOP 'BYE'\n      END\n") == 0);
        tacvm_ejecutar(out, sizeof(out), 1000, &r);
        COMPROBAR(r.estado == TACVM_STOP && strcmp(r.msg, "BYE") == 0);
        COMPROBAR(compilar("      STOP\n      END\n") == 0);
        tacvm_ejecutar(out, sizeof(out), 1000, &r);
        COMPROBAR(r.estado == TACVM_STOP && r.msg[0] == '\0');
    }

    /* PRINT vacio y PRINT de constante de texto */
    SALIDA("      PRINT *\n      PRINT 'HOLA'\n      END\n", "\n HOLA\n");
}

/* ------------------------------------------------------------------
 * REAL: constantes, modo mixto, relacionales, arrays y PRINT (formato gfortran, 9 cifras)
 * ------------------------------------------------------------------ */

static void test_reales(void)
{
    LISTADO("      REAL X, A(3)\n      I = 2\n      X = I * 1.5 + 0.1\n      A(I) = -X\n      I = X\n"
            "      IF (X .GT. 2) PRINT *, X\n      END\n",
            "UNIT PROGRAM MAIN\n"
            "VAR X REAL\n"
            "VAR A REAL (1:3)\n"
            "VAR I INTEGER\n"
            "        MOV.I  I, 2\n"
            "        ITOF.R %0, I\n"
            "        MUL.R  %1, %0, 1.5\n"
            "        ADD.R  X, %1, 0.100000001\n"
            "        SUB.I  %0, I, 1\n"
            "        NEG.R  %1, X\n"
            "        STE.R  A, %0, %1\n"
            "        FTOI.I %0, X\n"
            "        MOV.I  I, %0\n"
            "        GT.R   %0, X, 2.0\n"
            "        JF     %0, @@1\n"
            "        PUT.R  X\n"
            "        PUTNL\n"
            "        LBL    @@1\n"
            "        HALT\n");
    SALIDA("      X = 1.5\n      PRINT *, X\n      END\n", " 1.50000000\n");
    SALIDA("      X = 0.1\n      PRINT *, X\n      END\n", " 0.100000001\n");
    SALIDA("      X = 1\n      PRINT *, X\n      END\n", " 1.00000000\n");
    SALIDA("      PRINT *, 100.0, -2.5, 0.0, 0.01, 1E10, 123456789.0\n      END\n",
           " 100.000000 -2.50000000 0.00000000 9.99999978E-03 1.00000000E+10 123456792.\n");
    /* modo mixto y asignacion con conversion */
    SALIDA("      I = 7\n      X = I / 2\n      Y = I / 2.0\n      PRINT *, X, Y\n      END\n",
           " 3.00000000 3.50000000\n");
    SALIDA("      X = 2.75\n      I = X\n      J = -X\n      PRINT *, I, J\n      END\n", " 2 -2\n");
    SALIDA("      X = 1.5\n      Y = X * 4 - 1\n      PRINT *, Y, -Y, X + X\n      END\n",
           " 5.00000000 -5.00000000 3.00000000\n");
    /* relacionales con modo mixto */
    SALIDA("      X = 2.5\n      IF (X .GT. 2) PRINT *, 'A'\n      IF (X .LT. 3) PRINT *, 'B'\n"
           "      IF (2 .EQ. 2.0) PRINT *, 'C'\n      IF (X .NE. X) PRINT *, 'D'\n      END\n", " A\n B\n C\n");
    /* IF aritmetico, DO con limites reales, INT/REAL/FLOAT/IFIX */
    SALIDA("      X = -0.5\n      IF (X) 10, 20, 30\n   10 PRINT *, 'NEG'\n      GOTO 40\n   20 PRINT *, 'CERO'\n"
           "      GOTO 40\n   30 PRINT *, 'POS'\n   40 CONTINUE\n      END\n", " NEG\n");
    SALIDA("      DO 10 I = 1, 3.9\n         PRINT *, I\n   10 CONTINUE\n      END\n", " 1\n 2\n 3\n");
    SALIDA("      X = 3.99\n      PRINT *, INT(X), IFIX(-X), REAL(5), FLOAT(I)\n      END\n",
           " 3 -3 5.00000000 0.00000000\n");
    /* arrays y acumulacion */
    SALIDA("      REAL A(3)\n      DO 10 I = 1, 3\n         A(I) = I * 0.5\n   10 CONTINUE\n"
           "      S = 0\n      DO 20 I = 1, 3\n         S = S + A(I)\n   20 CONTINUE\n"
           "      PRINT *, A(1), A(3), S\n      END\n", " 0.500000000 1.50000000 3.00000000\n");
    /* errores de ejecucion segun realfmt.h: el programa imprime el mensaje y PARA (lo anterior ya salio) */
    SALIDA("      X = 1E38\n      PRINT *, 'A'\n      Y = X * 10\n      PRINT *, 'B'\n      END\n", " A\nREAL overflow\n");
    SALIDA("      X = 3E38\n      Y = X + X\n      PRINT *, 'B'\n      END\n", "REAL overflow\n");
    SALIDA("      X = 1E30\n      Y = X / 1E-30\n      PRINT *, 'B'\n      END\n", "REAL overflow\n");
    SALIDA("      Z = 0.0\n      PRINT *, 1.0 / Z\n      END\n", "Division by zero\n");
    SALIDA("      Z = 0.0\n      PRINT *, Z / Z\n      END\n", "0/0 is indeterminate\n");
    SALIDA("      I = 0\n      PRINT *, 'A'\n      J = 5 / I\n      PRINT *, 'B'\n      END\n", " A\nDivision by zero\n");
    SALIDA("      I = 0\n      J = I / I\n      PRINT *, 'B'\n      END\n", "0/0 is indeterminate\n");
    SALIDA("      I = 0\n      J = 7\n      PRINT *, MOD(J, 2), J / 2\n      END\n", " 1 3\n");
    SALIDA("      X = 1E30\n      PRINT *, 1E-30 * 1E-30, X\n      END\n", " 0.00000000 1.00000002E+30\n");
    SALIDA("      X = 40000.0\n      I = X\n      PRINT *, I\n      END\n", " 32767\n");
    /* R4: intrinsecas REAL, ** con exponente entero, DO con variable REAL */
    SALIDA("      X = -2.5\n      PRINT *, ABS(X), AINT(X), ANINT(X), AINT(2.5), ANINT(2.5)\n      END\n",
           " 2.50000000 -2.00000000 -3.00000000 2.00000000 3.00000000\n");
    SALIDA("      PRINT *, ANINT(0.49999997), AINT(0.99), ANINT(1E10), AINT(-0.3)\n      END\n",
           " 0.00000000 0.00000000 1.00000000E+10 -0.00000000\n");
    SALIDA("      PRINT *, SQRT(2.0), SQRT(4.0), SQRT(0.0), SQRT(1E30)\n      END\n",
           " 1.41421354 2.00000000 0.00000000 9.99999987E+14\n");
    SALIDA("      PRINT *, AMOD(7.5, 2.0), AMOD(-7.5, 2.0), AMOD(1.0, 0.1)\n      END\n",
           " 1.50000000 -1.50000000 9.99999866E-02\n");
    SALIDA("      PRINT *, MAX(1.5, 2.5, -1.0), MIN(1.5, 2.5)\n      PRINT *, AMAX1(3.0, 4.0), AMIN1(3.0, 4.0)\n      END\n",
           " 2.50000000 1.50000000\n 4.00000000 3.00000000\n");
    SALIDA("      PRINT *, AMAX0(1, 5, 3), MAX1(1.9, 5.2)\n      PRINT *, AMIN0(4, 2), MIN1(7.5, 6.5)\n      END\n",
           " 5.00000000 5\n 2.00000000 6\n");
    SALIDA("      PRINT *, SIGN(3.0, -1.0), SIGN(3.0, 2.0), DIM(5.0, 3.0)\n      PRINT *, DIM(2.0, 3.0), ABS(-0.0)\n      END\n",
           " -3.00000000 3.00000000 2.00000000\n 0.00000000 0.00000000\n");
    SALIDA("      PRINT *, NINT(2.5), NINT(-2.5), NINT(2.4), NINT(-0.4)\n      END\n", " 3 -3 2 0\n");
    SALIDA("      X = 1.5\n      N = 3\n      PRINT *, 2.0 ** 10, 2.0 ** (-2), X ** N, X ** 0, X ** (-N)\n      END\n",
           " 1024.00000 0.250000000 3.37500000 1.00000000 0.296296299\n");
    SALIDA("      DO 10 X = 0.0, 1.0, 0.25\n         PRINT *, X\n   10 CONTINUE\n      END\n",
           " 0.00000000\n 0.250000000\n 0.500000000\n 0.750000000\n 1.00000000\n");
    SALIDA("      DO 10 X = 1.0, 0.0, -0.5\n         PRINT *, X\n   10 CONTINUE\n      END\n",
           " 1.00000000\n 0.500000000\n 0.00000000\n");
    SALIDA("      DO 10 X = 2.0, 1.0\n         PRINT *, X\n   10 CONTINUE\n      PRINT *, 'D', X\n      END\n", " D 2.00000000\n");
    SALIDA("      S = 0.0\n      DO 10 X = 0.1, 0.5, 0.1\n         S = S + 1.0\n   10 CONTINUE\n      PRINT *, S\n      END\n",
           " 5.00000000\n");
    SALIDA("      DO 20 I = 1, 2\n      DO 10 X = 0.0, 1.0\n         PRINT *, I, X\n   10 CONTINUE\n   20 CONTINUE\n      END\n",
           " 1 0.00000000\n 1 1.00000000\n 2 0.00000000\n 2 1.00000000\n");
    SALIDA("      PRINT *, SQRT(-1.0)\n      END\n", "SQRT of a negative number\n");
    SALIDA("      X = 0.0\n      PRINT *, 'A'\n      PRINT *, X ** 0\n      END\n", " A\nZero to a non-positive power\n");
    SALIDA("      X = 0.0\n      PRINT *, X ** (-1)\n      END\n", "Zero to a non-positive power\n");
    SALIDA("      X = 0.0\n      PRINT *, 3.0E20 ** 2, X ** 2\n      END\n", "REAL overflow\n");
    SALIDA("      PRINT *, AMOD(1.0, 0.0)\n      END\n", "Division by zero\n");
}

/* ------------------------------------------------------------------
 * Errores del generador
 * ------------------------------------------------------------------ */

static void test_errores(void)
{
    /* lo que el generador aun no sabe */
    ERROR("      DOUBLE PRECISION X\n      X = 1\n      END\n", 2, 7, SEM_GEN_NO_SOPORTADO);
    ERROR("      I = 2.5D0\n      END\n", 1, 11, SEM_GEN_NO_SOPORTADO);
    ERROR("      X = 1E39\n      END\n", 1, 11, SEM_REAL_DESBORDA);
    ERROR("      X = 2.0 ** 0.5\n      END\n", 1, 15, SEM_GEN_NO_SOPORTADO);
    ERROR("      X = SIN(1.0)\n      END\n", 1, 11, SEM_GEN_NO_SOPORTADO);
    ERROR("      I = ICHAR('A')\n      END\n", 1, 11, SEM_GEN_NO_SOPORTADO);
    ERROR("      PRINT 'FMT', 5\n      END\n", 1, 7, SEM_GEN_NO_SOPORTADO);

    /* paso cero en un DO */
    ERROR("      DO 10 I = 1, 5, 0\n   10 CONTINUE\n      END\n", 1, 23, SEM_DO_PASO_CERO);

    /* varias unidades (decision 0014): lo que sigue al END abre otra unidad, un segundo programa principal */
    COMPROBAR(compilar("      END\n      I = 1\n      J = 2\n") == 2);
    COMPROBAR(mf.diag[0].cod == SEM_FALTA_END && mf.diag[1].cod == MF_COD_UNI + UNI_VARIOS_PRINCIPALES
              && mf.diag[1].linea == 2 && mf.diag[1].columna == 7);
    COMPROBAR(compilar("      END\nC comentario\n\n") == 0);

    /* un error anterior apaga el generador: solo sale el primero */
    COMPROBAR(compilar("      I = 'A'\n      X = 1.5\n      END\n") == 1);
    COMPROBAR(mf.diag[0].linea == 1 && mf.diag[0].cod == SEM_INCOMPATIBLES);
}

/* ------------------------------------------------------------------
 * Capacidad
 * ------------------------------------------------------------------ */

static void test_capacidad(void)
{
    static char prog[200000];
    int i, n;
    char l[64];

    /* justo por debajo del limite de instrucciones: ENTER + 2046 + HALT = 2048 */
    prog[0] = '\0';
    strcat(prog, "      N = 0\n");
    for (i = 0; i < 2045; i++) {                        /* ENTER + 1 + 2045 + HALT = 2048 */
        strcat(prog, "      N = N + 1\n");
    }
    strcat(prog, "      END\n");
    COMPROBAR(compilar(prog) == 0);
    COMPROBAR(tac_cuenta() == TAC_MAX);

    /* una mas: se desborda; el error sale UNA vez */
    prog[0] = '\0';
    strcat(prog, "      N = 0\n");
    for (i = 0; i < 2047; i++) {
        strcat(prog, "      N = N + 1\n");
    }
    strcat(prog, "      END\n");
    COMPROBAR(compilar(prog) == 1);
    COMPROBAR(mf.diag[0].cod == SEM_GEN_TAC_LLENO);

    /* mas de 4096 instrucciones: sigue siendo un solo error */
    prog[0] = '\0';
    strcat(prog, "      N = 0\n");
    for (i = 0; i < 6000; i++) {
        strcat(prog, "      N = N + 1\n");
    }
    strcat(prog, "      END\n");
    COMPROBAR(compilar(prog) == 1);
    COMPROBAR(mf.diag[0].cod == SEM_GEN_TAC_LLENO);

    /* el pool de constantes se llena: 170 cadenas de 50 caracteres (52 bytes cada una) */
    prog[0] = '\0';
    for (i = 0; i < 170; i++) {
        int k;

        strcat(prog, "      PRINT *, '");
        for (k = 0; k < 50; k++) {
            strcat(prog, "A");
        }
        strcat(prog, "'\n");
    }
    strcat(prog, "      END\n");
    COMPROBAR(compilar(prog) == 1);
    COMPROBAR(mf.diag[0].cod == SEM_GEN_TAC_LLENO);

    /* 150 cadenas de 50 caben (150 x 52 = 7800 bytes) */
    prog[0] = '\0';
    for (i = 0; i < 150; i++) {
        int k;

        strcat(prog, "      PRINT *, '");
        for (k = 0; k < 50; k++) {
            strcat(prog, "B");
        }
        strcat(prog, "'\n");
    }
    strcat(prog, "      END\n");
    COMPROBAR(compilar(prog) == 0);
    COMPROBAR(tacvm_validar(l, sizeof(l)) == 0);

    /* GOTO calculado con 64 etiquetas cabe, con 65 no */
    for (n = 64; n <= 65; n++) {
        prog[0] = '\0';
        strcat(prog, "      GOTO (");
        for (i = 1; i <= n; i++) {
            sprintf(l, "%s10", i > 1 ? "," : "");
            strcat(prog, l);
            if (i % 8 == 0 && i < n) {
                strcat(prog, "\n     +");
            }
        }
        strcat(prog, "), K\n   10 CONTINUE\n      END\n");
        if (n == 64) {
            COMPROBAR(compilar(prog) == 0 && tac_cuenta() == 64 + 3);
        } else {
            COMPROBAR(compilar(prog) == 1 && mf.diag[0].cod == SEM_GEN_GOTO_CALC);
        }
    }

    /* 16 bloques anidados caben */
    prog[0] = '\0';
    for (i = 0; i < 16; i++) {
        sprintf(l, "      DO %d I%d = 1, 2\n", 100 - i, i);
        strcat(prog, l);
    }
    for (i = 15; i >= 0; i--) {
        sprintf(l, "  %3d CONTINUE\n", 100 - i);
        strcat(prog, l);
    }
    strcat(prog, "      END\n");
    COMPROBAR(compilar(prog) == 0);
    COMPROBAR(tacvm_validar(l, sizeof(l)) == 0);
}

/* ------------------------------------------------------------------
 * Diferencial aleatorio: expresiones enteras y logicas
 * ------------------------------------------------------------------ */

#include "exprgen.h"

static void test_diferencial(void)
{
    static char prog[16384];
    char t[4096], msg[96], out[256], esperado[64];
    TacVmRes r;
    int it, k, largo;
    int hechas = 0;

    for (it = 0; it < 1500; it++) {
        Ex *e;
        int logico = (it & 1);

        n_ex = 0;
        for (k = 0; k < NV; k++) {
            /* A,B en 0..4 (exponentes); C,D cualquiera; E..H distintos de cero */
            if (k < 2) valores[k] = (int16_t)(rng() % 5);
            else if (k < 4) valores[k] = (int16_t)((int32_t)(rng() % 65536) - 32768);
            else valores[k] = (int16_t)((int32_t)(rng() % 2000) - 1000 + ((rng() % 2) ? 1 : 0));
            if (k >= 4 && valores[k] == 0) valores[k] = 7;
        }
        e = logico ? gen_log(1 + (int)(rng() % 3)) : gen_ent(1 + (int)(rng() % 4));
        texto(e, t);
        largo = (int)strlen(t);
        if (largo > 60 * 20) continue;      /* las lineas de continuacion tienen limite */
        prog[0] = '\0';
        strcat(prog, "      INTEGER A, B, C, D, E, F, G, H\n      LOGICAL L\n");
        for (k = 0; k < NV; k++) {
            char l[64];

            sprintf(l, "      %c = %d\n", 'A' + k, (int)valores[k]);
            strcat(prog, l);
        }
        /* la expresion en lineas de continuacion de 60 caracteres */
        if (logico) strcat(prog, "      L = "); else strcat(prog, "      C = ");
        {
            int pos = 0;
            int primera = 1;

            while (pos < largo) {
                int n = largo - pos > 60 ? 60 : largo - pos;
                char l[96];

                if (!primera) strcat(prog, "     +");
                memcpy(l, t + pos, (size_t)n);
                l[n] = '\0';
                strcat(prog, l);
                strcat(prog, "\n");
                pos += n;
                primera = 0;
            }
        }
        strcat(prog, logico ? "      PRINT *, L\n      END\n" : "      PRINT *, C\n      END\n");
        if (compilar(prog) != 0 && (mf.diag[0].cod == MF_COD_EXPR + 7 || mf.diag[0].cod == MF_COD_EXPR + 8)) {
            continue;                       /* expresion demasiado grande para el AST (64 nodos) */
        }
        if (mf.n != 0) {
            printf("FALLO diferencial: no compila\n%s", prog);
            mostrar_errores();
            fallos++;
            pruebas++;
            return;
        }
        pruebas++;
        if (tacvm_validar(msg, sizeof(msg)) != 0) {
            printf("FALLO diferencial: TAC invalido %s\n%s", msg, prog);
            fallos++;
            return;
        }
        tacvm_ejecutar(out, sizeof(out), 100000, &r);
        if (logico) sprintf(esperado, " %c\n", evalua(e) ? 'T' : 'F');
        else sprintf(esperado, " %d\n", (int)(int16_t)evalua(e));
        if (r.estado != TACVM_HALT || strcmp(out, esperado) != 0) {
            printf("FALLO diferencial (%d): esperado %s obtenido %s (%s)\n%s\n%s", it, esperado, out, r.msg, prog, lst);
            fallos++;
            return;
        }
        hechas++;
    }
    COMPROBAR(hechas > 1000);
    printf("   (diferencial: %d expresiones comprobadas)\n", hechas);
}


/* ------------------------------------------------------------------
 * Fuzz: programas validos estropeados al azar. Lo que compile DEBE dar un TAC
 * valido y ejecutable (solo se admiten los errores de ejecucion legitimos).
 * ------------------------------------------------------------------ */

static const char *const corpus[] = {
    "      INTEGER I, S\n      S = 0\n      DO 10 I = 1, 10\n         S = S + I\n   10 CONTINUE\n      PRINT *, S\n      END\n",
    "      CHARACTER*8 A\n      A = 'HI' // 'X'\n      IF (A .EQ. 'HIX') PRINT *, 'OK'\n      PRINT *, A(2:3)\n      END\n",
    "      INTEGER V(10), M(3,2)\n      V(3) = 7\n      I = V(3) + 1\n      M(2,2) = I*2\n      J = M(2,2)\n      DO 5 K=1,10\n    5 V(K) = K*K\n      PRINT *, I, J, V(10), V(I)\n      END\n",
    "      N = 3\n      IF (N .GT. 5) THEN\n        PRINT *, 'BIG'\n      ELSE IF (N .GT. 2) THEN\n        PRINT *, 'MID'\n      ELSE\n        PRINT *, 'SMALL'\n      END IF\n      GOTO (10,20,30), N\n   10 PRINT *, 1\n   20 PRINT *, 2\n   30 PRINT *, 3\n      IF (N-3) 40,50,60\n   40 STOP 4\n   50 PRINT *, N .EQ. 3, .NOT. (N .LT. 3)\n   60 STOP\n      END\n",
    "      N=0\n      DO 10 I=1,3\n      DO 10 J=1,3\n   10 IF (J .NE. 2) N = N + I*J\n      PRINT *, N\n      END\n",
    "      CHARACTER*3 C(2)\n      LOGICAL L\n      C(2) = 'AB'\n      C(1) = C(2) // 'XYZ'\n      L = C(1) .GT. C(2)\n      PRINT *, C(1), C(2), L\n      PRINT *, 'A''B', C(2)(2:2)\n      END\n",
    "      LOGICAL P(30)\n      DO 10 I = 2, 30\n   10 P(I) = .TRUE.\n      DO 30 I = 2, 5\n      IF (.NOT. P(I)) GOTO 30\n      DO 20 J = 2 * I, 30, I\n   20 P(J) = .FALSE.\n   30 CONTINUE\n      END\n",
    "      N = 3\n      K = 0\n      DO 10 I = 1, N\n      N = N + 1\n   10 K = K + 1\n      PRINT *, K, N, 2**3, 7/2, -I\n      END\n",
};

static void test_fuzz(void)
{
    static const char alfabeto[] = "()0123456789,'.:*+-/ ABIJKNCL=\n";
    static char p[2048];
    int it, n_corpus = (int)(sizeof(corpus) / sizeof(corpus[0]));
    int compilan = 0;

    for (it = 0; it < 30000; it++) {
        size_t len, pos;
        int m, nm = 1 + (int)(rng() % 4);
        char msg[96], out[512];
        TacVmRes r;

        strcpy(p, corpus[rng() % (uint32_t)n_corpus]);
        for (m = 0; m < nm; m++) {
            len = strlen(p);
            pos = rng() % len;
            switch (rng() % 5) {
            case 0: memmove(p + pos, p + pos + 1, len - pos); break;
            case 1:
                if (len < sizeof(p) - 2) {
                    memmove(p + pos + 1, p + pos, len - pos + 1);
                    p[pos] = alfabeto[rng() % (sizeof(alfabeto) - 1)];
                }
                break;
            case 2: p[pos] = alfabeto[rng() % (sizeof(alfabeto) - 1)]; break;
            case 3: {                                   /* duplica el resto desde una linea */
                char *ini = p + pos;
                size_t resto;

                while (ini > p && ini[-1] != '\n') ini--;
                resto = strlen(ini);
                if (len + resto < sizeof(p) - 1) {
                    memmove(ini + resto, ini, resto + 1);      /* el resto aparece dos veces */
                }
                break;
            }
            default: {                                  /* cambia un digito */
                size_t k;

                for (k = pos; k < len; k++) {
                    if (p[k] >= '0' && p[k] <= '9') { p[k] = (char)('0' + rng() % 10); break; }
                }
                break;
            }
            }
        }
        if (compilar(p) != 0) continue;
        compilan++;
        pruebas++;
        if (tacvm_validar(msg, sizeof(msg)) != 0) {
            printf("FALLO fuzz: TAC invalido: %s\n%s", msg, p);
            fallos++;
            return;
        }
        tacvm_ejecutar(out, sizeof(out), 20000, &r);
        if (r.estado == TACVM_ERROR && strstr(r.msg, "division") == NULL
            && strstr(r.msg, "fuera de rango") == NULL && strstr(r.msg, "limite") == NULL) {
            printf("FALLO fuzz: la ejecucion fallo: %s\n%s\n%s", r.msg, p, lst);
            fallos++;
            return;
        }
    }
    COMPROBAR(compilan > 500);
    printf("   (fuzz: %d programas estropeados siguen compilando)\n", compilan);
}

int main(void)
{
    COMPROBAR(sizeof(TacInst) == 8);
    test_listados();
    test_ejecucion();
    test_reales();
    test_errores();
    test_capacidad();
    test_diferencial();
    test_fuzz();
    printf("%d pruebas, %d fallos\n", pruebas, fallos);
    return fallos != 0;
}
