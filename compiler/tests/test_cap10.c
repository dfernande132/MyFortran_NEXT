/* test_cap10.c - Declaraciones de la 0.1: DIMENSION, EXTERNAL, INTRINSIC (S4a), PARAMETER, DATA y READ.
 *
 * Cada programa se ejecuta como TAC (tacvm) y como Z80 (z80emu); las dos salidas deben ser IGUALES y ademas iguales a la salida
 * escrita a mano. Los errores se comprueban con su linea, columna y codigo.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mf_api.h"
#include "gen.h"
#include "tac.h"
#include "back.h"
#include "z80.h"
#include "texto.h"
#include "banco.h"
#include "tacvm.h"
#include "z80emu.h"
#include "nex.h"
#include "unid.h"
#include "sem.h"
#include "stmt.h"
#include "expr.h"

static int pruebas = 0;
static int fallos = 0;

#define ORG 0x8000
#ifndef FUZZ_N
#define FUZZ_N 300
#endif

static uint8_t mem[65536];
static char listado[262144];

static void sal(const char *l)
{
    strcat(listado, l);
    strcat(listado, "\n");
}

static int a_tac(const char *prog)
{
    banco_iniciar();
    texto_poner(prog);
    return (int)mf_compilar();
}

static int ejecutar_z80(char *out, size_t n, BackRes *br, Z80EmuRes *er)
{
    memset(mem, 0, sizeof(mem));
    listado[0] = '\0';
    back_generar(mem + ORG, (uint16_t)(0x10000 - ORG - 0x1000), ORG, sal, br);
    if (br->err != 0) {
        out[0] = '\0';
        return br->err;
    }
    z80emu_ejecutar(mem, br->entry, 50000000u, out, n, er);
    return er->estado;
}

/* El programa debe compilar, dar `esperado` en TAC y la misma salida en Z80. */
static int salida(int linea_test, const char *prog, const char *esperado)
{
    char o1[8192], o2[8192], msg[96];
    TacVmRes tr;
    BackRes br;
    Z80EmuRes er;
    int e;

    pruebas++;
    if (a_tac(prog) != 0) {
        char b[MF_MENSAJE_MAX];

        mf_mensaje(mf.diag[0].cod, b);
        printf("FALLO (linea %d): no compila: %u:%u %s\n%s", linea_test, (unsigned)mf.diag[0].linea,
               (unsigned)mf.diag[0].columna, b, prog);
        fallos++;
        return 0;
    }
    if (tacvm_validar(msg, sizeof(msg)) != 0) {
        printf("FALLO (linea %d): TAC invalido: %s\n", linea_test, msg);
        fallos++;
        return 0;
    }
    tacvm_ejecutar(o1, sizeof(o1), 5000000u, &tr);
    if (tr.estado == TACVM_ERROR) {
        printf("FALLO (linea %d): tacvm: %s\n%s", linea_test, tr.msg, prog);
        fallos++;
        return 0;
    }
    if (tr.estado == TACVM_STOP && tr.msg[0] != '\0') {
        strcat(o1, "STOP ");
        strcat(o1, tr.msg);
        strcat(o1, "\n");
    }
    if (esperado != NULL && strcmp(o1, esperado) != 0) {
        printf("FALLO (linea %d): TAC da\n%s--- y se esperaba\n%s--- programa\n%s", linea_test, o1, esperado, prog);
        fallos++;
        return 0;
    }
    e = ejecutar_z80(o2, sizeof(o2), &br, &er);
    if (e != 0) {
        printf("FALLO (linea %d): backend/emulador estado %d (instr TAC %u, pc %04X op %02X)\n%s\n",
               linea_test, e, (unsigned)br.err_inst, (unsigned)er.pc, (unsigned)er.op, prog);
        fallos++;
        return 0;
    }
    if (strcmp(o1, o2) != 0) {
        printf("FALLO (linea %d): Z80 da\n%s--- y el TAC\n%s--- programa\n%s", linea_test, o2, o1, prog);
        fallos++;
        return 0;
    }
    return 1;
}

#define SALIDA(p, e) salida(__LINE__, p, e)

/* El programa debe dar el PRIMER error (linea, columna, codigo) y ninguno mas si cuantos > 0 lo pide. */
static int error_es(int linea_test, const char *prog, int cuantos, int lin, int col, int cod)
{
    pruebas++;
    if ((int)a_tac(prog) != cuantos || mf.diag[0].linea != lin || mf.diag[0].columna != col || mf.diag[0].cod != cod) {
        printf("FALLO (linea %d): %d errores, primero %u:%u cod %u (esperado %d errores, %d:%d cod %d)\n%s", linea_test,
               (int)mf.n, (unsigned)mf.diag[0].linea, (unsigned)mf.diag[0].columna, (unsigned)mf.diag[0].cod,
               cuantos, lin, col, cod, prog);
        fallos++;
        return 0;
    }
    return 1;
}

#define ERROR(p, n, l, c, k) error_es(__LINE__, p, n, l, c, k)
#define U(e)  (MF_COD_UNI + (e))

/* ------------------------------------------------------------------
 * DIMENSION, EXTERNAL, INTRINSIC
 * ------------------------------------------------------------------ */
static void test_dimension(void)
{
    SALIDA("      PROGRAM T\n      DIMENSION A(5)\n      INTEGER K\n      DO 10 K = 1, 5\n      A(K) = K*K\n   10 CONTINUE\n"
           "      PRINT *, A(1), A(5)\n      END\n", " 1.00000000 25.0000000\n");
    /* A es INTEGER por la declaracion previa, B REAL implicito, I implicito INTEGER */
    SALIDA("      PROGRAM T\n      INTEGER A\n      DIMENSION A(3), B(2,2), C(0:2), IA(2)\n      A(2) = 7\n      B(2,2) = 1.5\n"
           "      C(0) = 2.5\n      IA(1) = 9\n      PRINT *, A(2), B(2,2), C(0), IA(1)\n      END\n",
           " 7 1.50000000 2.50000000 9\n");
    /* DIMENSION primero y el tipo despues */
    SALIDA("      PROGRAM T\n      DIMENSION X(2)\n      INTEGER X\n      X(1) = 3\n      X(2) = X(1)/2\n      PRINT *, X(2)\n      END\n", " 1\n");
    SALIDA("      PROGRAM T\n      DIMENSION S(2)\n      CHARACTER*3 S\n      S(1) = 'AB'\n      S(2) = 'XYZ'\n      PRINT *, S(1), S(2)\n      END\n",
           " AB  XYZ\n");
    /* CHARACTER*n previo y DIMENSION despues: la longitud se conserva */
    SALIDA("      PROGRAM T\n      CHARACTER*4 S\n      DIMENSION S(3)\n      S(2) = 'AB'\n      PRINT *, S(2)\n      END\n", " AB  \n");
    /* varias sentencias DIMENSION, mezcla con declaraciones de tipo */
    SALIDA("      PROGRAM T\n      DIMENSION A(2)\n      INTEGER K\n      DIMENSION B(2)\n      A(1) = 1.0\n      B(1) = 2.0\n      K = 3\n"
           "      PRINT *, A(1), B(1), K\n      END\n", " 1.00000000 2.00000000 3\n");
    /* formal ajustable y de tamano supuesto con DIMENSION */
    SALIDA("      PROGRAM T\n      INTEGER V(4)\n      V(1) = 1\n      V(2) = 2\n      V(3) = 3\n      V(4) = 4\n      CALL S(V, 4)\n      END\n"
           "      SUBROUTINE S(A, N)\n      INTEGER A, N\n      DIMENSION A(N)\n      INTEGER K, T\n      T = 0\n      DO 10 K = 1, N\n"
           "      T = T + A(K)\n   10 CONTINUE\n      PRINT *, T\n      END\n", " 10\n");
    SALIDA("      PROGRAM T\n      INTEGER V(3)\n      V(1) = 5\n      V(2) = 6\n      V(3) = 7\n      CALL S(V)\n      END\n"
           "      SUBROUTINE S(A)\n      DIMENSION A(*)\n      INTEGER A\n      PRINT *, A(3)\n      END\n", " 7\n");
}

static void test_dimension_errores(void)
{
    /* array repetido */
    ERROR("      PROGRAM T\n      DIMENSION A(3)\n      DIMENSION A(4)\n      END\n", 1, 3, 17, SEM_YA_DIMENSIONADO);
    ERROR("      PROGRAM T\n      INTEGER A(3)\n      DIMENSION A(4)\n      END\n", 1, 3, 17, SEM_YA_DIMENSIONADO);
    ERROR("      PROGRAM T\n      DIMENSION A(3)\n      INTEGER A(4)\n      END\n", 1, 3, 15, SEM_YA_DIMENSIONADO);
    /* despues de una ejecutable */
    ERROR("      PROGRAM T\n      X = 1\n      DIMENSION A(3)\n      END\n", 1, 3, 17, SEM_DECL_TARDE);
    /* sin limites */
    ERROR("      PROGRAM T\n      DIMENSION A\n      END\n", 1, 2, 17, SEM_DIM_SIN_LIMITES);
    /* limites invalidos y tres dimensiones */
    ERROR("      PROGRAM T\n      DIMENSION A(0)\n      END\n", 1, 2, 19, SEM_DIMENSION);
    ERROR("      PROGRAM T\n      DIMENSION A(2,2,2)\n      END\n", 1, 2, 23, SEM_DIMENSIONES);
    /* tamano supuesto solo en formales */
    ERROR("      PROGRAM T\n      DIMENSION A(*)\n      END\n", 1, 2, 19, SEM_DIMENSION);
    /* con etiqueta */
    ERROR("      PROGRAM T\n   10 DIMENSION A(3)\n      END\n", 1, 2, 7, SEM_ETIQUETA_EN_DECL);
    /* el nombre de la funcion no se dimensiona */
    ERROR("      PROGRAM T\n      END\n      INTEGER FUNCTION F(N)\n      DIMENSION F(2)\n      F = N\n      END\n", 1, 4, 17, SEM_YA_DEFINIDO);
}

static void test_procedimientos(void)
{
    /* INTRINSIC: se llama como siempre */
    SALIDA("      PROGRAM T\n      INTRINSIC SQRT, ABS\n      PRINT *, SQRT(16.0), ABS(-3)\n      END\n", " 4.00000000 3\n");
    /* EXTERNAL con tipo antes y despues */
    SALIDA("      PROGRAM T\n      INTEGER F\n      EXTERNAL F\n      PRINT *, F(2)\n      END\n      INTEGER FUNCTION F(N)\n      INTEGER N\n"
           "      F = N + 1\n      END\n", " 3\n");
    SALIDA("      PROGRAM T\n      EXTERNAL F\n      INTEGER F\n      PRINT *, F(2)\n      END\n      INTEGER FUNCTION F(N)\n      INTEGER N\n"
           "      F = N + 1\n      END\n", " 3\n");
    /* EXTERNAL de una funcion de nombre intrinseco: manda la del usuario */
    SALIDA("      PROGRAM T\n      EXTERNAL SQRT\n      REAL SQRT\n      PRINT *, SQRT(16.0)\n      END\n      REAL FUNCTION SQRT(X)\n      REAL X\n"
           "      SQRT = X + 1.0\n      END\n", " 17.0000000\n");
}

static void test_procedimientos_errores(void)
{
    ERROR("      PROGRAM T\n      EXTERNAL F\n      CALL S(F)\n      END\n      SUBROUTINE S(G)\n      END\n", 1, 3, 14, SEM_PROC_ARG);
    ERROR("      PROGRAM T\n      INTRINSIC SIN\n      CALL S(SIN)\n      END\n      SUBROUTINE S(G)\n      END\n", 1, 3, 14, SEM_PROC_ARG);
    ERROR("      PROGRAM T\n      INTRINSIC FOO\n      END\n", 1, 2, 17, SEM_NO_INTRINSECA);
    ERROR("      PROGRAM T\n      X = 1\n      EXTERNAL F\n      END\n", 1, 3, 16, SEM_DECL_TARDE);
    ERROR("      SUBROUTINE S(G)\n      EXTERNAL G\n      END\n", 1, 2, 16, SEM_PROC_ARG);
    ERROR("      PROGRAM T\n      DIMENSION A(2)\n      EXTERNAL A\n      END\n", 1, 3, 16, SEM_YA_DEFINIDO);
    ERROR("      PROGRAM T\n      EXTERNAL F(2)\n      END\n", 1, 2, 16, SEM_NOMBRE_SOLO);
}

/* ------------------------------------------------------------------
 * PARAMETER
 * ------------------------------------------------------------------ */
static void test_parameter(void)
{
    SALIDA("      PROGRAM T\n      INTEGER N, M\n      PARAMETER (N = 5, M = N*2+1)\n      REAL PI\n      PARAMETER (PI = 3.5)\n"
           "      CHARACTER*5 S\n      PARAMETER (S = 'HELLO')\n      LOGICAL L\n      PARAMETER (L = .TRUE.)\n"
           "      INTEGER A(N), B(M)\n      CHARACTER*(N-2) C\n      A(N) = 4\n      B(M) = 99\n      C = 'ABCDE'\n"
           "      PRINT *, N, M, PI, S, L, A(5), B(11), C\n      PRINT *, 2*PI\n      END\n",
           " 5 11 3.50000000 HELLO T 4 99 ABC\n 7.00000000\n");
    /* tipo implicito; valor usado en DO y en indices; constantes derivadas de otras */
    SALIDA("      PROGRAM T\n      PARAMETER (NMAX = 4, X = 1.5, Y = X*2.0)\n      INTEGER V(NMAX), K\n      DO 10 K = 1, NMAX\n"
           "      V(K) = K*NMAX\n   10 CONTINUE\n      PRINT *, V(NMAX), Y\n      END\n", " 16 3.00000000\n");
    /* un PARAMETER en un subprograma se reinicia en cada llamada */
    SALIDA("      PROGRAM T\n      CALL S\n      CALL S\n      END\n      SUBROUTINE S\n      INTEGER K\n      PARAMETER (K = 3)\n"
           "      PRINT *, K\n      END\n", " 3\n 3\n");
    /* limites de DIMENSION con PARAMETER y negativos */
    SALIDA("      PROGRAM T\n      PARAMETER (LO = -2, KHI = 2)\n      INTEGER A\n      DIMENSION A(LO:KHI)\n      A(LO) = 1\n      A(KHI) = 2\n"
           "      PRINT *, A(-2), A(2)\n      END\n", " 1 2\n");
}

static void test_parameter_errores(void)
{
    ERROR("      PROGRAM T\n      PARAMETER (N = 5)\n      N = 6\n      END\n", 1, 3, 7, SEM_PARAM_ASIGNA);
    ERROR("      PROGRAM T\n      PARAMETER (N = 5)\n      DO 10 N = 1, 2\n   10 CONTINUE\n      END\n", 1, 3, 17, SEM_PARAM_ASIGNA);
    ERROR("      PROGRAM T\n      INTEGER K\n      PARAMETER (N = K+1)\n      END\n", 1, 3, 23, SEM_PARAM_CTE);
    ERROR("      PROGRAM T\n      X = 1\n      PARAMETER (N = 2)\n      END\n", 1, 3, 18, SEM_DECL_TARDE);
    ERROR("      PROGRAM T\n      PARAMETER (N = 2)\n      PARAMETER (N = 3)\n      END\n", 1, 3, 18, SEM_YA_DEFINIDO);
    ERROR("      PROGRAM T\n      PARAMETER (N = 2)\n      INTEGER N\n      END\n", 1, 3, 15, SEM_YA_DEFINIDO);
    ERROR("      PROGRAM T\n      PARAMETER (N = 5/0)\n      END\n", 1, 2, 23, SEM_PARAM_CTE);
    ERROR("      PROGRAM T\n      INTEGER N\n      PARAMETER (N = 2.5)\n      END\n", 1, 3, 22, SEM_PARAM_CTE);
    ERROR("      SUBROUTINE S(K)\n      PARAMETER (K = 2)\n      END\n", 1, 2, 18, SEM_YA_DEFINIDO);
}

/* ------------------------------------------------------------------
 * DATA
 * ------------------------------------------------------------------ */
static void test_data(void)
{
    SALIDA("      PROGRAM T\n      INTEGER A(5), N, M(2,2)\n      REAL X, Y(3)\n      CHARACTER*4 S, T(2)\n      LOGICAL L\n"
           "      DATA A /1, 2, 3*7/, N /-5/\n      DATA X /2.5/, Y /3*1.5/\n      DATA S /'AB'/, T /'WXYZQ', 'CD'/\n"
           "      DATA L /.TRUE./\n      DATA M /4*0/\n      DATA M(2,1) /9/\n"
           "      PRINT *, A(1), A(2), A(3), A(5), N\n      PRINT *, X, Y(1), Y(3)\n      PRINT *, S, T(1), T(2), L\n"
           "      PRINT *, M(1,1), M(2,1), M(1,2)\n      END\n",
           " 1 2 7 7 -5\n 2.50000000 1.50000000 1.50000000\n AB   WXYZ CD   T\n 0 9 0\n");
    /* varios nombres con un solo grupo de valores, entero para un REAL, PARAMETER como valor y como factor */
    SALIDA("      PROGRAM T\n      PARAMETER (K = 3)\n      REAL A, B\n      INTEGER I, J(3)\n      DATA A, B, I /1, -2.5, K/\n      DATA J /K*K/\n"
           "      PRINT *, A, B, I, J(1), J(3)\n      END\n", " 1.00000000 -2.50000000 3 3 3\n");
    /* un nombre sin dimensionar de tipo implicito, y DATA tras sentencias ejecutables */
    SALIDA("      PROGRAM T\n      K = 1\n      DATA N /8/, R /0.25/\n      PRINT *, K, N, R\n      END\n", " 1 8 0.250000000\n");
    /* lo que se modifica en una llamada se conserva en la siguiente (variables estaticas) */
    SALIDA("      PROGRAM T\n      CALL S\n      CALL S\n      END\n      SUBROUTINE S\n      INTEGER C\n      DATA C /10/\n      C = C + 1\n"
           "      PRINT *, C\n      END\n", " 11\n 12\n");
    /* cadenas: relleno, truncado y elementos de un array de cadenas */
    SALIDA("      PROGRAM T\n      CHARACTER*3 S(2)\n      CHARACTER*6 U\n      DATA S(2), S(1) /'A', 'BCDEF'/, U /'HI'/\n"
           "      PRINT *, S(1), S(2), U\n      END\n", " BCD A   HI    \n");
    /* arrays con limites negativos y 2 dimensiones por elementos */
    SALIDA("      PROGRAM T\n      INTEGER V(-1:1), M(2,3)\n      DATA V /10, 20, 30/\n      DATA M(1,1), M(2,3) /1, 6/\n"
           "      PRINT *, V(-1), V(0), V(1), M(1,1), M(2,3)\n      END\n", " 10 20 30 1 6\n");
    /* los dos formatos de separacion entre grupos y el valor sin signo en una lista larga */
    SALIDA("      PROGRAM T\n      INTEGER A, B, C\n      DATA A /1/ B /2/, C /3/\n      PRINT *, A, B, C\n      END\n", " 1 2 3\n");
}

static void test_data_errores(void)
{
    ERROR("      PROGRAM T\n      INTEGER A(2)\n      DATA A /1,2,3/\n      END\n", 1, 3, 19, SEM_DATA_CUENTA);
    ERROR("      PROGRAM T\n      INTEGER A(3)\n      DATA A /1,2/\n      END\n", 1, 3, 7, SEM_DATA_CUENTA);
    ERROR("      PROGRAM T\n      INTEGER A\n      DATA A /'X'/\n      END\n", 1, 3, 15, SEM_DATA_TIPO);
    ERROR("      PROGRAM T\n      INTEGER A\n      DATA A /2.5/\n      END\n", 1, 3, 15, SEM_DATA_TIPO);
    ERROR("      PROGRAM T\n      INTEGER A, B\n      DATA A /B/\n      END\n", 1, 3, 15, SEM_DATA_VALOR);
    ERROR("      PROGRAM T\n      INTEGER A(2)\n      DATA A(3) /1/\n      END\n", 1, 3, 12, SEM_NUM_INDICES);
    ERROR("      PROGRAM T\n      INTEGER A\n      DATA A(1) /1/\n      END\n", 1, 3, 12, SEM_GEN_NO_SOPORTADO);
    ERROR("      PROGRAM T\n      INTEGER A\n      DATA A /0*1/\n      END\n", 1, 3, 15, SEM_DATA_REPET);
    ERROR("      PROGRAM T\n      PARAMETER (N = 1)\n      DATA N /2/\n      END\n", 1, 3, 12, SEM_DATA_DEST);
    ERROR("      SUBROUTINE S(K)\n      DATA K /2/\n      END\n", 1, 2, 12, SEM_DATA_DEST);
    ERROR("      PROGRAM T\n      INTEGER A\n      DATA A\n      END\n", 1, 3, 13, MF_COD_STMT + STM_BARRA_ESPERADA);
    ERROR("      PROGRAM T\n      INTEGER A\n      DATA A /1\n      END\n", 1, 3, 16, MF_COD_STMT + STM_BARRA_ESPERADA);
    ERROR("      PROGRAM T\n      INTEGER A\n      DATA A /X+1/\n      END\n", 2, 3, 15, SEM_DATA_VALOR);
}

/* ------------------------------------------------------------------
 * READ (entrada dirigida por lista). La entrada es el texto que "teclea" el programa; el eco de cada linea leida sale en la salida.
 * ------------------------------------------------------------------ */
static int z80_ejecutadas = 0;
static int z80_entrada = 1;     /* 1 = tambien se ejecuta en el Z80 (la tecla sale del texto de entrada: back_entrada_prueba) */

static int lectura(int linea_test, const char *prog, const char *entrada, const char *esperado)
{
    char o1[8192], msg[96];
    TacVmRes tr;

    pruebas++;
    if (a_tac(prog) != 0) {
        char b[MF_MENSAJE_MAX];

        mf_mensaje(mf.diag[0].cod, b);
        printf("FALLO (linea %d): no compila: %u:%u %s\n%s", linea_test, (unsigned)mf.diag[0].linea, (unsigned)mf.diag[0].columna, b, prog);
        fallos++;
        return 0;
    }
    if (tacvm_validar(msg, sizeof(msg)) != 0) {
        printf("FALLO (linea %d): TAC invalido: %s\n", linea_test, msg);
        fallos++;
        return 0;
    }
    tacvm_entrada(entrada);
    tacvm_ejecutar(o1, sizeof(o1), 5000000u, &tr);
    if (tr.estado == TACVM_ERROR) {
        printf("FALLO (linea %d): tacvm: %s\n%s", linea_test, tr.msg, prog);
        fallos++;
        return 0;
    }
    if (esperado != NULL && strcmp(o1, esperado) != 0) {
        printf("FALLO (linea %d): TAC da\n%s--- y se esperaba\n%s--- programa\n%s", linea_test, o1, esperado, prog);
        fallos++;
        return 0;
    }
    if (z80_entrada) {
        char o2[8192];
        BackRes br;
        Z80EmuRes er;
        int e;

        memset(mem, 0, sizeof(mem));
        listado[0] = '\0';
        back_entrada_prueba(1);
        back_generar(mem + ORG, (uint16_t)(0xF000 - ORG), ORG, sal, &br);
        if (br.err != 0) {
            printf("FALLO (linea %d): backend estado %d (instr TAC %u)\n%s\n", linea_test, br.err, (unsigned)br.err_inst, prog);
            fallos++;
            return 0;
        }
        strcpy((char *)mem + 0xF002, entrada);
        mem[0xF000] = 0x02;                                   /* puntero de la "tecla" (RT_KEYT) */
        mem[0xF001] = 0xF0;
        z80emu_ejecutar(mem, br.entry, 50000000u, o2, sizeof(o2), &er);
        e = er.estado;
        if (e != 0) {
            printf("FALLO (linea %d): emulador estado %d (pc %04X op %02X)\n%s\n", linea_test, e, (unsigned)er.pc, (unsigned)er.op, prog);
            fallos++;
            return 0;
        }
        if (strcmp(o1, o2) != 0) {
            printf("FALLO (linea %d): Z80 da\n%s--- y el TAC\n%s--- programa\n%s", linea_test, o2, o1, prog);
            fallos++;
            return 0;
        }
        z80_ejecutadas++;
    }
    return 1;
}

#define LECTURA(p, i, e) lectura(__LINE__, p, i, e)

static void test_read(void)
{
    /* enteros, separados por blancos, por comas y en lineas distintas */
    LECTURA("      PROGRAM T\n      INTEGER A, B, C\n      READ *, A, B\n      READ (*,*) C\n      PRINT *, A, B, C\n      END\n",
            "1 2\n3\n", "1 2\n3\n 1 2 3\n");
    LECTURA("      PROGRAM T\n      INTEGER A, B, C\n      READ *, A, B, C\n      PRINT *, A, B, C\n      END\n",
            "10,-20\n  +30\n", "10,-20\n  +30\n 10 -20 30\n");
    /* una sentencia READ descarta lo que sobre de la linea */
    LECTURA("      PROGRAM T\n      INTEGER A, B\n      READ *, A\n      READ *, B\n      PRINT *, A, B\n      END\n",
            "1 2 3\n4 5\n", "1 2 3\n4 5\n 1 4\n");
    /* valores nulos (",,") y '/' */
    LECTURA("      PROGRAM T\n      INTEGER A, B, C\n      A = 7\n      B = 8\n      C = 9\n      READ *, A, B, C\n      PRINT *, A, B, C\n      END\n",
            "1,,3\n", "1,,3\n 1 8 3\n");
    LECTURA("      PROGRAM T\n      INTEGER A, B, C\n      A = 7\n      B = 8\n      C = 9\n      READ *, A, B, C\n      PRINT *, A, B, C\n      END\n",
            "1 / 3\n", "1 / 3\n 1 8 9\n");
    /* reales, logicos y cadenas */
    LECTURA("      PROGRAM T\n      REAL X, Y, Z\n      READ *, X, Y, Z\n      PRINT *, X, Y, Z\n      END\n",
            "2.5 -0.5 1.5E2\n", "2.5 -0.5 1.5E2\n 2.50000000 -0.500000000 150.000000\n");
    LECTURA("      PROGRAM T\n      REAL X\n      READ *, X\n      PRINT *, X\n      END\n", "7\n", "7\n 7.00000000\n");
    LECTURA("      PROGRAM T\n      LOGICAL P, Q, R\n      READ *, P, Q, R\n      PRINT *, P, Q, R\n      END\n",
            "T .FALSE. .true.\n", "T .FALSE. .true.\n T F T\n");
    LECTURA("      PROGRAM T\n      CHARACTER*5 S, U\n      CHARACTER*3 V\n      READ *, S, U, V\n      PRINT *, S, U, V\n      END\n",
            "'AB' HELLO 'it''s'\n", "'AB' HELLO 'it''s'\n AB    HELLO it'\n");
    /* elementos de array e indices que son expresiones */
    LECTURA("      PROGRAM T\n      INTEGER V(3), K\n      CHARACTER*2 C(2)\n      REAL R(2)\n      K = 1\n      READ *, V(K), V(K+1), V(3)\n"
            "      READ *, C(2), C(1), R(2), R(1)\n      PRINT *, V(1), V(2), V(3)\n      PRINT *, C(1), C(2), R(1), R(2)\n      END\n",
            "4 5 6\nxy zw 1.5 2.5\n", "4 5 6\nxy zw 1.5 2.5\n 4 5 6\n zw xy 2.50000000 1.50000000\n");
    /* READ dentro de un bucle, con datos en varias lineas */
    LECTURA("      PROGRAM T\n      INTEGER I, N, S\n      READ *, N\n      S = 0\n      DO 10 I = 1, N\n      READ *, K\n      S = S + K\n   10 CONTINUE\n"
            "      PRINT *, S\n      END\n", "3\n10\n20\n30\n", "3\n10\n20\n30\n 60\n");
    /* sin lista: salta un registro */
    LECTURA("      PROGRAM T\n      INTEGER A\n      READ *\n      READ *, A\n      PRINT *, A\n      END\n", "9\n8\n", "9\n8\n 8\n");
    /* errores de ejecucion */
    LECTURA("      PROGRAM T\n      INTEGER A\n      READ *, A\n      PRINT *, A\n      END\n", "12x\n", "12x\nInvalid input\n");
    LECTURA("      PROGRAM T\n      INTEGER A\n      READ *, A\n      PRINT *, A\n      END\n", "40000\n", "40000\nInvalid input\n");
    LECTURA("      PROGRAM T\n      INTEGER A, B\n      READ *, A, B\n      PRINT *, A, B\n      END\n", "1\n", "1\nEnd of input\n");
    LECTURA("      PROGRAM T\n      LOGICAL L\n      READ *, L\n      END\n", "X\n", "X\nInvalid input\n");
    /* mas casos: limites de INTEGER, formas de REAL, LOGICAL y cadenas */
    LECTURA("      PROGRAM T\n      INTEGER A, B, C\n      READ *, A, B, C\n      PRINT *, A, B, C\n      END\n",
            "-32768 32767 +0\n", "-32768 32767 +0\n -32768 32767 0\n");
    LECTURA("      PROGRAM T\n      INTEGER A\n      READ *, A\n      PRINT *, A\n      END\n", "32768\n", "32768\nInvalid input\n");
    LECTURA("      PROGRAM T\n      INTEGER A\n      READ *, A\n      PRINT *, A\n      END\n", "-\n", "-\nInvalid input\n");
    LECTURA("      PROGRAM T\n      INTEGER A\n      READ *, A\n      PRINT *, A\n      END\n", "'5'\n", "'5'\nInvalid input\n");
    LECTURA("      PROGRAM T\n      INTEGER A\n      READ *, A\n      PRINT *, A\n      END\n", "1.5\n", "1.5\nInvalid input\n");
    LECTURA("      PROGRAM T\n      REAL A, B, C, D, E, F\n      READ *, A, B, C, D, E, F\n      PRINT *, A\n      PRINT *, B\n      PRINT *, C\n"
            "      PRINT *, D\n      PRINT *, E\n      PRINT *, F\n      END\n",
            "-1.25 .5 5. 1E3 2.5D-1 +123.456\n", "-1.25 .5 5. 1E3 2.5D-1 +123.456\n -1.25000000\n 0.500000000\n 5.00000000\n 1000.00000\n"
            " 0.250000000\n 123.456001\n");
    LECTURA("      PROGRAM T\n      REAL A\n      READ *, A\n      PRINT *, A\n      END\n", "1.5E\n", "1.5E\nInvalid input\n");
    LECTURA("      PROGRAM T\n      REAL A\n      READ *, A\n      PRINT *, A\n      END\n", ".\n", ".\nInvalid input\n");
    LECTURA("      PROGRAM T\n      REAL A\n      READ *, A\n      PRINT *, A\n      END\n", "1.2.3\n", "1.2.3\nInvalid input\n");
    LECTURA("      PROGRAM T\n      LOGICAL P, Q, R, S\n      READ *, P, Q, R, S\n      PRINT *, P, Q, R, S\n      END\n",
            ".T. false .f. TRUE\n", ".T. false .f. TRUE\n T F F T\n");
    LECTURA("      PROGRAM T\n      CHARACTER*4 S, U, V\n      READ *, S, U, V\n      PRINT *, S, U, V\n      END\n",
            "\"a b\" '' abcdefgh\n", "\"a b\" '' abcdefgh\n a b       abcd\n");
    LECTURA("      PROGRAM T\n      CHARACTER*4 S\n      READ *, S\n      PRINT *, S\n      END\n", "'abc\n", "'abc\nInvalid input\n");
    /* un valor nulo no toca el elemento del array */
    LECTURA("      PROGRAM T\n      INTEGER V(2)\n      CHARACTER*2 C(2)\n      REAL R(2)\n      V(1) = 5\n      V(2) = 6\n      C(1) = 'ab'\n      C(2) = 'cd'\n"
            "      R(1) = 1.5\n      R(2) = 2.5\n      READ *, V(1), V(2), C(1), C(2), R(1), R(2)\n      PRINT *, V(1), V(2), C(1), C(2), R(1), R(2)\n      END\n",
            ",9,,xy,,7\n", ",9,,xy,,7\n 5 9 ab xy 1.50000000 7.00000000\n");
    /* el eco de una linea larga se corta a 79 caracteres */
    LECTURA("      PROGRAM T\n      INTEGER A\n      READ *, A\n      PRINT *, A\n      END\n",
            "1                                                                                  2\n",
            "1                                                                              \n 1\n");
}

/* Entradas al azar (semilla fija): el TAC y el Z80 deben dar exactamente lo mismo, con datos buenos y malos. Los REAL llevan a lo
 * sumo 7 cifras y |exponente decimal| <= 6, donde la conversion del Z80 (mantisa exacta, una sola multiplicacion o division) es exacta. */
static unsigned long azar_est = 12345;

static unsigned azar(unsigned n)
{
    azar_est = azar_est * 1103515245UL + 12345UL;
    return (unsigned)((azar_est >> 16) % n);
}

static void azar_ficha(char *d)
{
    static const char *const lista[] = { "0", "7", "-3", "+12", "32767", "-32768", "32768", "99999", "1x", "T", "F", ".TRUE.", ".f.", "tr", "x",
                                         "'ab'", "\"c d\"", "'it''s'", "abcdefghi", "''", "/", "", "." };
    unsigned k = azar(10);

    if (k < 2) {                                   /* un REAL */
        char t[40];
        unsigned nd = 1 + azar(7), i, f = azar(4), pos = 0;

        if (azar(3) == 0) t[pos++] = azar(2) ? '-' : '+';
        for (i = 0; i < nd; i++) {
            if (i == nd - f && f > 0 && f < nd) t[pos++] = '.';
            t[pos++] = (char)('0' + azar(10));
        }
        if (f == 0 && azar(3) == 0) t[pos++] = '.';
        if (azar(3) == 0) {
            t[pos++] = "EeDd"[azar(4)];
            if (azar(2)) t[pos++] = azar(2) ? '-' : '+';
            t[pos++] = (char)('0' + azar(4));
        }
        t[pos] = '\0';
        strcpy(d, t);
    } else if (k < 3) {
        sprintf(d, "%d", (int)azar(2000) - 1000);
    } else {
        strcpy(d, lista[azar((unsigned)(sizeof(lista) / sizeof(lista[0])))]);
    }
}

static void test_read_azar(void)
{
    static const char prog[] = "      PROGRAM T\n      INTEGER A, B\n      REAL X, Y\n      LOGICAL P\n      CHARACTER*6 S\n      INTEGER V(2)\n"
        "      A = 1\n      B = 2\n      X = 3.5\n      Y = -1.25\n      P = .TRUE.\n      S = 'init'\n      V(1) = 5\n      V(2) = 6\n"
        "      READ *, A, X, P, S\n      PRINT *, A, X, P, S\n      READ *, B, Y, V(2)\n      PRINT *, B, Y, V(1), V(2)\n      END\n";
    int i, nazar = 400;
    const char *e = getenv("MF_AZAR_N");

    if (e != NULL) nazar = atoi(e);
    if (getenv("MF_AZAR_SEMILLA") != NULL) azar_est = (unsigned long)atol(getenv("MF_AZAR_SEMILLA"));
    for (i = 0; i < nazar; i++) {
        char ent[600], f[80];
        int l, n;

        ent[0] = '\0';
        for (l = 0; l < 3; l++) {
            for (n = 0; n < 4; n++) {
                azar_ficha(f);
                strcat(ent, f);
                strcat(ent, azar(3) == 0 ? "," : azar(5) == 0 ? ", " : " ");
            }
            strcat(ent, "\n");
        }
        lectura(__LINE__, prog, ent, NULL);
    }
}

static void test_read_errores(void)
{
    ERROR("      PROGRAM T\n      INTEGER A(2)\n      READ *, A\n      END\n", 1, 3, 15, SEM_GEN_NO_SOPORTADO);
    ERROR("      PROGRAM T\n      PARAMETER (N = 1)\n      READ *, N\n      END\n", 1, 3, 15, SEM_PARAM_ASIGNA);
    ERROR("      PROGRAM T\n      READ *, X+1\n      END\n", 1, 2, 16, SEM_LECTURA_ITEM);
    ERROR("      PROGRAM T\n      READ 10, X\n      END\n", 1, 2, 12, MF_COD_STMT + STM_ENTRADA);
    ERROR("      PROGRAM T\n      READ (6,*) X\n      END\n", 1, 2, 13, MF_COD_STMT + STM_ENTRADA);
    ERROR("      PROGRAM T\n      READ (*,'(I3)') X\n      END\n", 1, 2, 15, MF_COD_STMT + STM_ENTRADA);
    ERROR("      PROGRAM T\n      INTEGER A\n      READ *, A(1)\n      END\n", 1, 3, 15, SEM_NO_ES_ARRAY);
}

int main(void)
{
    test_read();
    test_read_errores();
    test_read_azar();
    test_data();
    test_data_errores();
    test_parameter();
    test_parameter_errores();
    test_dimension();
    test_dimension_errores();
    test_procedimientos();
    test_procedimientos_errores();
    printf("%d pruebas (%d tambien en el Z80), %d fallos\n", pruebas, z80_ejecutadas, fallos);
    return fallos != 0;
}
