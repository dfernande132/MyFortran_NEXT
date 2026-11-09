/* test_cap09.c - Subprogramas (decision 0014): SUBROUTINE, CALL, RETURN, varias unidades en un fichero.
 *
 * Cada programa se ejecuta como TAC (tacvm) y como Z80 (z80emu); las dos salidas deben ser IGUALES y, ademas, iguales a la
 * salida escrita a mano en la prueba (asi no basta con que el TAC y el Z80 se equivoquen igual). Los errores se comprueban
 * con su linea, columna y codigo.
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
    if (strcmp(o1, esperado) != 0) {
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
 * Orden de las unidades
 * ------------------------------------------------------------------ */
static void test_orden(void)
{
    SALIDA("      CALL HOLA\n"
           "      END\n"
           "      SUBROUTINE HOLA\n"
           "      PRINT *, 'HOLA'\n"
           "      RETURN\n"
           "      END\n", " HOLA\n");
    /* el subprograma antes del principal */
    SALIDA("      SUBROUTINE HOLA\n"
           "      PRINT *, 'A'\n"
           "      END\n"
           "      PROGRAM P\n"
           "      CALL HOLA\n"
           "      CALL HOLA\n"
           "      END\n", " A\n A\n");
    /* el principal en medio, y dos subprogramas que se llaman entre si */
    SALIDA("      SUBROUTINE UNO\n"
           "      PRINT *, 'UNO'\n"
           "      CALL DOS\n"
           "      END\n"
           "      PROGRAM P\n"
           "      PRINT *, 'P'\n"
           "      CALL UNO\n"
           "      CALL DOS\n"
           "      END\n"
           "      SUBROUTINE DOS\n"
           "      PRINT *, 'DOS'\n"
           "      END\n", " P\n UNO\n DOS\n DOS\n");
    /* cadena de llamadas: el programa llama a A, A a B, B a C (definidas al reves) */
    SALIDA("      SUBROUTINE C\n"
           "      PRINT *, 3\n"
           "      END\n"
           "      SUBROUTINE B\n"
           "      PRINT *, 2\n"
           "      CALL C\n"
           "      PRINT *, 22\n"
           "      END\n"
           "      SUBROUTINE A\n"
           "      PRINT *, 1\n"
           "      CALL B\n"
           "      PRINT *, 11\n"
           "      END\n"
           "      CALL A\n"
           "      PRINT *, 0\n"
           "      END\n", " 1\n 2\n 3\n 22\n 11\n 0\n");
    /* sin PROGRAM y con el principal el ultimo */
    SALIDA("      SUBROUTINE S\n"
           "      PRINT *, 7\n"
           "      END\n"
           "      I = 1\n"
           "      CALL S\n"
           "      PRINT *, I\n"
           "      END\n", " 7\n 1\n");
    /* comentarios y lineas en blanco entre unidades */
    SALIDA("C primera\n"
           "      CALL S\n"
           "      END\n"
           "\n"
           "C segunda\n"
           "      SUBROUTINE S\n"
           "      PRINT *, 5\n"
           "      END\n"
           "\n", " 5\n");
    /* parentesis vacios y sin parentesis */
    SALIDA("      CALL S()\n"
           "      CALL S\n"
           "      END\n"
           "      SUBROUTINE S()\n"
           "      PRINT *, 6\n"
           "      END\n", " 6\n 6\n");
    /* CALL como sentencia etiquetada y como final de un DO */
    SALIDA("      DO 10 I = 1, 3\n"
           "   10 CALL P(I)\n"
           "      PRINT *, 99\n"
           "      END\n"
           "      SUBROUTINE P(K)\n"
           "      PRINT *, K\n"
           "      END\n", " 1\n 2\n 3\n 99\n");
}

/* ------------------------------------------------------------------
 * Paso de argumentos
 * ------------------------------------------------------------------ */
static void test_argumentos(void)
{
    /* por referencia: el subprograma modifica las variables de quien llama */
    SALIDA("      INTEGER A, B, C\n"
           "      A = 3\n"
           "      B = 4\n"
           "      CALL SUMA(A, B, C)\n"
           "      PRINT *, C, A, B\n"
           "      END\n"
           "      SUBROUTINE SUMA(X, Y, Z)\n"
           "      INTEGER X, Y, Z\n"
           "      Z = X + Y\n"
           "      END\n", " 7 3 4\n");
    SALIDA("      A = 1\n"
           "      B = 2\n"
           "      CALL SWAP(A, B)\n"
           "      PRINT *, A, B\n"
           "      END\n"
           "      SUBROUTINE SWAP(X, Y)\n"
           "      T = X\n"
           "      X = Y\n"
           "      Y = T\n"
           "      END\n", " 2.00000000 1.00000000\n");
    /* tipos implicitos del formal: I-N entero, el resto real */
    SALIDA("      INTEGER N\n"
           "      REAL X\n"
           "      N = 5\n"
           "      X = 1.5\n"
           "      CALL F(N, X)\n"
           "      PRINT *, N, X\n"
           "      END\n"
           "      SUBROUTINE F(I, Y)\n"
           "      I = I * 2\n"
           "      Y = Y * 2.0\n"
           "      END\n", " 10 3.00000000\n");
    /* LOGICAL */
    SALIDA("      LOGICAL L\n"
           "      L = .TRUE.\n"
           "      CALL NEG(L)\n"
           "      PRINT *, L\n"
           "      CALL NEG(L)\n"
           "      PRINT *, L\n"
           "      END\n"
           "      SUBROUTINE NEG(B)\n"
           "      LOGICAL B\n"
           "      B = .NOT. B\n"
           "      END\n", " F\n T\n");
    /* las constantes se pasan por valor: modificar el formal no las cambia */
    SALIDA("      CALL S(5)\n"
           "      CALL S(5)\n"
           "      CALL S(-7)\n"
           "      CALL R(2.5)\n"
           "      CALL R(2.5)\n"
           "      CALL L(.TRUE.)\n"
           "      CALL L(.TRUE.)\n"
           "      END\n"
           "      SUBROUTINE S(K)\n"
           "      K = K + 1\n"
           "      PRINT *, K\n"
           "      END\n"
           "      SUBROUTINE R(X)\n"
           "      X = X * 2.0\n"
           "      PRINT *, X\n"
           "      END\n"
           "      SUBROUTINE L(B)\n"
           "      LOGICAL B\n"
           "      B = .NOT. B\n"
           "      PRINT *, B\n"
           "      END\n", " 6\n 6\n -6\n 5.00000000\n 5.00000000\n F\n F\n");
    /* expresiones y (X) entre parentesis: valor, no variable */
    SALIDA("      A = 1.0\n"
           "      B = 2.0\n"
           "      I = 3\n"
           "      CALL R(A + B)\n"
           "      CALL R((A))\n"
           "      CALL S(I * 2)\n"
           "      CALL S((I))\n"
           "      PRINT *, A, B, I\n"
           "      END\n"
           "      SUBROUTINE R(X)\n"
           "      X = X + 10.0\n"
           "      PRINT *, X\n"
           "      END\n"
           "      SUBROUTINE S(K)\n"
           "      K = K + 10\n"
           "      PRINT *, K\n"
           "      END\n", " 13.0000000\n 11.0000000\n 16\n 13\n 1.00000000 2.00000000 3\n");
    /* elementos de array: se pasan por referencia (copia de vuelta al elemento) */
    SALIDA("      INTEGER V(3), M(2, 2)\n"
           "      REAL R(2)\n"
           "      V(1) = 10\n"
           "      V(2) = 20\n"
           "      V(3) = 30\n"
           "      M(1, 2) = 5\n"
           "      R(2) = 0.5\n"
           "      I = 2\n"
           "      CALL INC(V(I))\n"
           "      CALL INC(V(I + 1))\n"
           "      CALL INC(M(1, 2))\n"
           "      CALL DOBLA(R(2))\n"
           "      PRINT *, V(1), V(2), V(3)\n"
           "      PRINT *, M(1, 2), R(2), I\n"
           "      END\n"
           "      SUBROUTINE INC(K)\n"
           "      K = K + 1\n"
           "      END\n"
           "      SUBROUTINE DOBLA(X)\n"
           "      X = X * 2.0\n"
           "      END\n", " 10 21 31\n 6 1.00000000 2\n");
    /* los temporales de quien llama no los pisa el subprograma (el elemento esta en un temporal cuando se llama) */
    SALIDA("      INTEGER V(3)\n"
           "      V(1) = 1\n"
           "      V(2) = 2\n"
           "      V(3) = 3\n"
           "      K = 7\n"
           "      CALL G(V(2), K + 1)\n"
           "      PRINT *, V(1), V(2), V(3), K\n"
           "      END\n"
           "      SUBROUTINE G(X, Y)\n"
           "      INTEGER X, Y\n"
           "      X = (X + 1) * (Y + 2) - (Y - 3) * (X - 4)\n"
           "      END\n", " 1 40 3 7\n");
    /* 15 argumentos (el maximo) */
    SALIDA("      CALL M(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)\n"
           "      END\n"
           "      SUBROUTINE M(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O)\n"
           "      INTEGER A, B, C, D, E, F, G, H, I, J, K, L, M, N, O\n"
           "      PRINT *, A, B, C, D, E\n"
           "      PRINT *, F, G, H, I, J\n"
           "      PRINT *, K, L, M, N, O\n"
           "      END\n", " 1 2 3 4 5\n 6 7 8 9 10\n 11 12 13 14 15\n");
}

/* ------------------------------------------------------------------
 * Variables, etiquetas, bucles y control de flujo en cada unidad
 * ------------------------------------------------------------------ */
static void test_unidades_independientes(void)
{
    /* los mismos nombres en unidades distintas son variables distintas */
    SALIDA("      INTEGER X, A\n"
           "      X = 1\n"
           "      A = 2\n"
           "      CALL S\n"
           "      PRINT *, X, A\n"
           "      END\n"
           "      SUBROUTINE S\n"
           "      INTEGER X, A\n"
           "      X = 10\n"
           "      A = 20\n"
           "      PRINT *, X, A\n"
           "      END\n", " 10 20\n 1 2\n");
    /* un nombre que en una unidad es un array y en la otra un escalar */
    SALIDA("      INTEGER M(2)\n"
           "      M(1) = 5\n"
           "      M(2) = 6\n"
           "      CALL S\n"
           "      PRINT *, M(1), M(2)\n"
           "      END\n"
           "      SUBROUTINE S\n"
           "      M = 99\n"
           "      PRINT *, M\n"
           "      END\n", " 99\n 5 6\n");
    /* las mismas etiquetas en varias unidades */
    SALIDA("      K = 0\n"
           "      GO TO 10\n"
           "   20 PRINT *, 'FIN'\n"
           "      GO TO 30\n"
           "   10 K = K + 1\n"
           "      CALL S(K)\n"
           "      GO TO 20\n"
           "   30 CONTINUE\n"
           "      END\n"
           "      SUBROUTINE S(N)\n"
           "      GO TO 10\n"
           "   20 PRINT *, 'NO'\n"
           "   10 PRINT *, N\n"
           "      IF (N .GT. 5) GO TO 20\n"
           "   30 CONTINUE\n"
           "      END\n", " 1\n FIN\n");
    /* DO en las dos unidades: los contadores ocultos de quien llama no los pisa el subprograma */
    SALIDA("      DO 10 I = 1, 3\n"
           "         CALL S(I)\n"
           "   10 CONTINUE\n"
           "      PRINT *, I\n"
           "      END\n"
           "      SUBROUTINE S(K)\n"
           "      DO 20 J = 1, 2\n"
           "         PRINT *, K, J\n"
           "   20 CONTINUE\n"
           "      END\n", " 1 1\n 1 2\n 2 1\n 2 2\n 3 1\n 3 2\n 4\n");
    /* DO anidados en quien llama con paso y DO REAL en el subprograma */
    SALIDA("      DO 10 I = 6, 1, -2\n"
           "         DO 5 L = 1, 2\n"
           "            CALL S(I, L)\n"
           "    5    CONTINUE\n"
           "   10 CONTINUE\n"
           "      END\n"
           "      SUBROUTINE S(K, M)\n"
           "      DO 20 X = 0.0, 0.5, 0.25\n"
           "         PRINT *, K, M, X\n"
           "   20 CONTINUE\n"
           "      END\n",
           " 6 1 0.00000000\n 6 1 0.250000000\n 6 1 0.500000000\n"
           " 6 2 0.00000000\n 6 2 0.250000000\n 6 2 0.500000000\n"
           " 4 1 0.00000000\n 4 1 0.250000000\n 4 1 0.500000000\n"
           " 4 2 0.00000000\n 4 2 0.250000000\n 4 2 0.500000000\n"
           " 2 1 0.00000000\n 2 1 0.250000000\n 2 1 0.500000000\n"
           " 2 2 0.00000000\n 2 2 0.250000000\n 2 2 0.500000000\n");
    /* llamadas dentro de IF logico, de IF de bloque y de ELSE */
    SALIDA("      DO 10 I = 1, 4\n"
           "         IF (I .EQ. 2) CALL P(I)\n"
           "         IF (I .GT. 2) THEN\n"
           "            CALL Q(I)\n"
           "         ELSE\n"
           "            CALL Q(-I)\n"
           "         END IF\n"
           "   10 CONTINUE\n"
           "      END\n"
           "      SUBROUTINE P(K)\n"
           "      PRINT *, 'P'\n"
           "      END\n"
           "      SUBROUTINE Q(K)\n"
           "      PRINT *, K\n"
           "      END\n", " -1\n P\n -2\n 3\n 4\n");
}

static void test_flujo(void)
{
    /* RETURN en medio, varios RETURN */
    SALIDA("      DO 10 I = -1, 2\n"
           "   10 CALL S(I)\n"
           "      END\n"
           "      SUBROUTINE S(K)\n"
           "      IF (K .LT. 0) RETURN\n"
           "      IF (K .EQ. 0) THEN\n"
           "         PRINT *, 'CERO'\n"
           "         RETURN\n"
           "      END IF\n"
           "      PRINT *, K\n"
           "      RETURN\n"
           "      PRINT *, 'NUNCA'\n"
           "      END\n", " CERO\n 1\n 2\n");
    /* STOP dentro de un subprograma para el programa entero */
    SALIDA("      PRINT *, 'A'\n"
           "      CALL S\n"
           "      PRINT *, 'NO'\n"
           "      END\n"
           "      SUBROUTINE S\n"
           "      PRINT *, 'B'\n"
           "      STOP\n"
           "      END\n", " A\n B\n");
    /* STOP con codigo dentro de un subprograma anidado */
    SALIDA("      CALL A\n"
           "      PRINT *, 'NO'\n"
           "      END\n"
           "      SUBROUTINE A\n"
           "      CALL B\n"
           "      PRINT *, 'NO'\n"
           "      END\n"
           "      SUBROUTINE B\n"
           "      STOP 7\n"
           "      END\n", "STOP 7\n");
    /* el mismo subprograma llamado muchas veces y desde sitios distintos */
    SALIDA("      INTEGER N\n"
           "      N = 0\n"
           "      DO 10 I = 1, 5\n"
           "   10 CALL INC(N)\n"
           "      CALL TRES(N)\n"
           "      PRINT *, N\n"
           "      END\n"
           "      SUBROUTINE TRES(K)\n"
           "      CALL INC(K)\n"
           "      CALL INC(K)\n"
           "      CALL INC(K)\n"
           "      END\n"
           "      SUBROUTINE INC(K)\n"
           "      K = K + 1\n"
           "      END\n", " 8\n");
    /* algoritmos: factorial, mcd y ordenar tres valores con subprogramas */
    SALIDA("      INTEGER R\n"
           "      CALL FACT(5, R)\n"
           "      PRINT *, R\n"
           "      CALL MCD(84, 36, R)\n"
           "      PRINT *, R\n"
           "      A = 3.5\n"
           "      B = 1.5\n"
           "      C = 2.5\n"
           "      CALL ORD(A, B)\n"
           "      CALL ORD(B, C)\n"
           "      CALL ORD(A, B)\n"
           "      PRINT *, A, B, C\n"
           "      END\n"
           "      SUBROUTINE FACT(N, R)\n"
           "      INTEGER N, R\n"
           "      R = 1\n"
           "      DO 10 I = 2, N\n"
           "   10 R = R * I\n"
           "      END\n"
           "      SUBROUTINE MCD(A, B, R)\n"
           "      INTEGER A, B, R, X, Y, T\n"
           "      X = A\n"
           "      Y = B\n"
           "   10 IF (Y .EQ. 0) GO TO 20\n"
           "      T = MOD(X, Y)\n"
           "      X = Y\n"
           "      Y = T\n"
           "      GO TO 10\n"
           "   20 R = X\n"
           "      END\n"
           "      SUBROUTINE ORD(X, Y)\n"
           "      IF (X .LE. Y) RETURN\n"
           "      T = X\n"
           "      X = Y\n"
           "      Y = T\n"
           "      END\n", " 120\n 12\n 1.50000000 2.50000000 3.50000000\n");
    /* cadena larga de 20 llamadas anidadas */
    {
        char prog[8192], l[160], esp[256];
        int i;

        strcpy(prog, "      CALL S1(0)\n      END\n");
        strcpy(esp, " 20\n");
        for (i = 1; i <= 20; i++) {
            if (i < 20) {
                sprintf(l, "      SUBROUTINE S%d(K)\n      CALL S%d(K + 1)\n      END\n", i, i + 1);
            } else {
                sprintf(l, "      SUBROUTINE S%d(K)\n      PRINT *, K + 1\n      END\n", i);
            }
            strcat(prog, l);
        }
        SALIDA(prog, esp);
    }
}

/* ------------------------------------------------------------------
 * Diagnosticos
 * ------------------------------------------------------------------ */
static void test_errores(void)
{
    /* numero de argumentos */
    ERROR("      CALL S(1)\n      END\n      SUBROUTINE S(A, B)\n      END\n", 1, 1, 12, U(UNI_NUM_ARGS));
    ERROR("      CALL S(1, 2)\n      END\n      SUBROUTINE S(A)\n      END\n", 1, 1, 12, U(UNI_NUM_ARGS));
    ERROR("      CALL S\n      END\n      SUBROUTINE S(A)\n      END\n", 1, 1, 12, U(UNI_NUM_ARGS));
    ERROR("      CALL S(1)\n      END\n      SUBROUTINE S\n      END\n", 1, 1, 12, U(UNI_NUM_ARGS));
    /* la definicion antes de la llamada */
    ERROR("      SUBROUTINE S(A, B)\n      END\n      CALL S(1)\n      END\n", 1, 3, 12, U(UNI_NUM_ARGS));
    /* tipo de argumento */
    ERROR("      CALL S(1.5)\n      END\n      SUBROUTINE S(I)\n      END\n", 1, 1, 12, U(UNI_TIPO_ARG));
    ERROR("      CALL S(1)\n      END\n      SUBROUTINE S(X)\n      END\n", 1, 1, 12, U(UNI_TIPO_ARG));
    ERROR("      CALL S(.TRUE.)\n      END\n      SUBROUTINE S(I)\n      END\n", 1, 1, 12, U(UNI_TIPO_ARG));
    ERROR("      CALL S(1)\n      END\n      SUBROUTINE S(L)\n      LOGICAL L\n      END\n", 1, 1, 12, U(UNI_TIPO_ARG));
    ERROR("      INTEGER N\n      CALL S(N)\n      END\n      SUBROUTINE S(N)\n      REAL N\n      END\n",
          1, 2, 12, U(UNI_TIPO_ARG));
    ERROR("      CALL S(1, 2.0, 3)\n      END\n      SUBROUTINE S(I, X, J)\n      REAL J\n      END\n",
          1, 1, 12, U(UNI_TIPO_ARG));
    /* el tipo explicito del formal manda sobre el implicito */
    SALIDA("      CALL S(1.5)\n      END\n      SUBROUTINE S(I)\n      REAL I\n      PRINT *, I\n      END\n", " 1.50000000\n");
    SALIDA("      CALL S(7)\n      END\n      SUBROUTINE S(X)\n      INTEGER X\n      PRINT *, X\n      END\n", " 7\n");
    /* sin definir */
    ERROR("      CALL FOO\n      END\n", 1, 1, 12, U(UNI_INDEFINIDA));
    ERROR("      CALL FOO\n      CALL FOO\n      END\n", 1, 1, 12, U(UNI_INDEFINIDA));
    /* duplicados */
    ERROR("      END\n      SUBROUTINE S\n      END\n      SUBROUTINE S\n      END\n", 1, 4, 7, U(UNI_YA_DEFINIDA));
    ERROR("      PROGRAM P\n      CALL P\n      END\n", 2, 2, 12, U(UNI_NO_SUBRUTINA));   /* y ademas recursiva */
    /* programa principal */
    ERROR("      SUBROUTINE S\n      END\n", 1, 1, 1, U(UNI_SIN_PRINCIPAL));
    ERROR("      END\n      END\n", 1, 2, 7, U(UNI_VARIOS_PRINCIPALES));
    ERROR("      PROGRAM A\n      END\n      PROGRAM B\n      END\n", 1, 3, 7, U(UNI_VARIOS_PRINCIPALES));
    /* RETURN en el programa principal */
    ERROR("      RETURN\n      END\n", 1, 1, 7, U(UNI_RETURN_PRINCIPAL));
    ERROR("      IF (.TRUE.) RETURN\n      END\n", 1, 1, 7, U(UNI_RETURN_PRINCIPAL));
    /* la cabecera debe ser la primera sentencia */
    ERROR("      I = 1\n      SUBROUTINE S\n      END\n", 1, 2, 7, U(UNI_CABECERA));
    ERROR("      INTEGER I\n      PROGRAM P\n      END\n", 1, 2, 7, U(UNI_CABECERA));
    /* recursion */
    ERROR("      END\n      SUBROUTINE S\n      CALL S\n      END\n", 1, 2, 7, U(UNI_RECURSIVA));
    ERROR("      END\n      SUBROUTINE A\n      CALL B\n      END\n      SUBROUTINE B\n      CALL A\n      END\n",
          2, 2, 7, U(UNI_RECURSIVA));
    ERROR("      END\n      SUBROUTINE A\n      CALL B\n      END\n      SUBROUTINE B\n      CALL C\n      END\n"
          "      SUBROUTINE C\n      CALL A\n      END\n", 3, 2, 7, U(UNI_RECURSIVA));
    /* argumentos formales repetidos y demasiados */
    ERROR("      END\n      SUBROUTINE S(A, B, A)\n      END\n", 1, 2, 26, U(UNI_ARG_DUPLICADO));
    ERROR("      END\n      SUBROUTINE S(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P)\n      END\n", 1, 2, 65, U(UNI_MUCHOS_ARGS));
    /* todavia no soportado: cadenas y arrays como argumento */
    ERROR("      CALL S('A')\n      END\n      SUBROUTINE S(A)\n      END\n", 1, 1, 12, U(UNI_TIPO_ARG));
    ERROR("      INTEGER V(3)\n      CALL S(V)\n      END\n      SUBROUTINE S(A)\n      END\n", 1, 2, 12, U(UNI_TIPO_ARG));
    /* etiqueta en la cabecera */
    ERROR("      END\n   10 SUBROUTINE S\n      END\n", 1, 2, 7, SEM_ETIQUETA_EN_DECL);
    /* sintaxis */
    ERROR("      END\n      SUBROUTINE\n      END\n", 1, 2, 17, MF_COD_STMT + STM_NOMBRE_ESPERADO);
    ERROR("      CALL\n      END\n", 1, 1, 11, MF_COD_STMT + STM_NOMBRE_ESPERADO);
    ERROR("      CALL S(1\n      END\n      SUBROUTINE S(I)\n      END\n", 1, 1, 15, MF_COD_STMT + STM_PAR_CIERRA_ESPERADO);
    ERROR("      CALL S(1,)\n      END\n      SUBROUTINE S(I)\n      END\n", 1, 1, 16, MF_COD_EXPR + EXPR_ESPERADA);
    ERROR("      CALL S(1) X\n      END\n      SUBROUTINE S(I)\n      END\n", 1, 1, 17, MF_COD_STMT + STM_SOBRA);
    ERROR("      END\n      SUBROUTINE S(1)\n      END\n", 1, 2, 20, MF_COD_STMT + STM_NOMBRE_ESPERADO);
    ERROR("      END\n      SUBROUTINE S(A,\n      END\n", 1, 2, 22, MF_COD_STMT + STM_NOMBRE_ESPERADO);
    /* un error de sintaxis en la cabecera no llena de errores el cuerpo */
    ERROR("      END\n      SUBROUTINE 1X(A)\n      A = 1\n      END\n", 1, 2, 18, MF_COD_STMT + STM_NOMBRE_ESPERADO);
}


/* ------------------------------------------------------------------
 * S2a: FUNCTION (INTEGER, REAL, LOGICAL) y referencias a funciones en expresiones
 * ------------------------------------------------------------------ */
static void test_funciones(void)
{
    /* tipos y contextos */
    SALIDA("      INTEGER FUNCTION DOBLE(N)\n"
           "      INTEGER N\n"
           "      DOBLE = N * 2\n"
           "      END\n"
           "      REAL FUNCTION MITAD(X)\n"
           "      REAL X\n"
           "      MITAD = X * 0.5\n"
           "      END\n"
           "      LOGICAL FUNCTION POS(N)\n"
           "      INTEGER N\n"
           "      POS = N .GT. 0\n"
           "      END\n"
           "      PROGRAM P\n"
           "      INTEGER DOBLE, A(4), I, J, K, S\n"
           "      REAL MITAD, R\n"
           "      LOGICAL POS\n"
           "      A(1) = 5\n"
           "      A(2) = 7\n"
           "      I = 1\n"
           "      J = DOBLE(I) + A(DOBLE(I))\n"
           "      PRINT *, J\n"
           "      R = MITAD(5.0) + 1\n"
           "      PRINT *, R\n"
           "      PRINT *, POS(I), POS(-I)\n"
           "      IF (POS(J) .AND. .NOT. POS(0)) PRINT *, 'OK'\n"
           "      S = 0\n"
           "      DO 10 K = 1, DOBLE(2)\n"
           "         S = S + K\n"
           "   10 CONTINUE\n"
           "      PRINT *, S\n"
           "      PRINT *, DOBLE(DOBLE(DOBLE(1)))\n"
           "      PRINT *, -DOBLE(3), DOBLE(2) ** 2\n"
           "      PRINT *, DOBLE(2) .EQ. 4\n"
           "      PRINT *, MAX(DOBLE(1), DOBLE(3)), ABS(-DOBLE(2))\n"
           "      PRINT *, MITAD(REAL(DOBLE(3)))\n"
           "      END\n",
           " 9\n 3.50000000\n T F\n OK\n 10\n 8\n -6 16\n T\n 6 4\n 3.00000000\n");
    /* tipo implicito del nombre (I-N entero, el resto real) */
    SALIDA("      FUNCTION KK(N)\n"
           "      KK = N + 1\n"
           "      END\n"
           "      FUNCTION F(X)\n"
           "      F = X * 2\n"
           "      END\n"
           "      PRINT *, KK(4), F(1.5)\n"
           "      END\n",
           " 5 3.00000000\n");
    /* sin argumentos, y las locales son estaticas */
    SALIDA("      INTEGER FUNCTION CUENTA(R)\n"
           "      INTEGER R, N\n"
           "      IF (R .EQ. 0) N = 0\n"
           "      N = N + 1\n"
           "      CUENTA = N\n"
           "      END\n"
           "      INTEGER FUNCTION CERO()\n"
           "      CERO = 0\n"
           "      END\n"
           "      INTEGER CUENTA, CERO\n"
           "      PRINT *, CUENTA(CERO()), CUENTA(1), CUENTA(1)\n"
           "      PRINT *, CUENTA(CERO())\n"
           "      END\n",
           " 1 2 3\n 1\n");
    /* RETURN anticipado y lectura del propio resultado */
    SALIDA("      INTEGER FUNCTION SIGNO(N)\n"
           "      INTEGER N\n"
           "      SIGNO = 0\n"
           "      IF (N .EQ. 0) RETURN\n"
           "      SIGNO = 1\n"
           "      IF (N .GT. 0) RETURN\n"
           "      SIGNO = SIGNO - 2\n"
           "      END\n"
           "      INTEGER SIGNO\n"
           "      PRINT *, SIGNO(-9), SIGNO(0), SIGNO(8)\n"
           "      END\n",
           " -1 0 1\n");
    /* por referencia: la funcion cambia su argumento (variable si, expresion no) */
    SALIDA("      INTEGER FUNCTION SIG(N)\n"
           "      INTEGER N\n"
           "      N = N + 1\n"
           "      SIG = N\n"
           "      END\n"
           "      INTEGER SIG, I, J\n"
           "      I = 5\n"
           "      J = SIG(I)\n"
           "      PRINT *, I, J\n"
           "      J = SIG(I + 0)\n"
           "      PRINT *, I, J\n"
           "      END\n",
           " 6 6\n 6 7\n");
    /* la funcion llama a subrutinas y a otras funciones; subprogramas en cualquier orden */
    /* (la funcion escribe: no se la llama desde la lista de un PRINT, porque su E/S se mezclaria con la del PRINT) */
    SALIDA("      INTEGER SUMA3, J1, J2\n"
           "      J1 = SUMA3(1, 2, 3)\n"
           "      J2 = SUMA3(SUMA3(1, 1, 1), 10, 100)\n"
           "      PRINT *, J1, J2\n"
           "      END\n"
           "      INTEGER FUNCTION SUMA3(A, B, C)\n"
           "      INTEGER A, B, C, SUMA2\n"
           "      CALL TRAZA(A + B + C)\n"
           "      SUMA3 = SUMA2(SUMA2(A, B), C)\n"
           "      END\n"
           "      SUBROUTINE TRAZA(N)\n"
           "      INTEGER N\n"
           "      PRINT *, 'T', N\n"
           "      END\n"
           "      INTEGER FUNCTION SUMA2(A, B)\n"
           "      INTEGER A, B\n"
           "      SUMA2 = A + B\n"
           "      END\n",
           " T 6\n T 3\n T 113\n 6 113\n");
    /* el tipo del resultado se declara dentro de la funcion */
    SALIDA("      FUNCTION F(N)\n"
           "      INTEGER F, N\n"
           "      F = N * 3\n"
           "      END\n"
           "      INTEGER F\n"
           "      PRINT *, F(4)\n"
           "      END\n",
           " 12\n");
    /* temporales vivos durante la llamada: A(I) + F(J) * A(K) con elementos de array */
    SALIDA("      INTEGER FUNCTION F(N)\n"
           "      INTEGER N, A(3)\n"
           "      A(1) = 100\n"
           "      A(2) = 200\n"
           "      A(3) = 300\n"
           "      F = A(N)\n"
           "      END\n"
           "      INTEGER A(3), F, I\n"
           "      A(1) = 1\n"
           "      A(2) = 2\n"
           "      A(3) = 3\n"
           "      I = 2\n"
           "      PRINT *, A(I) + F(I) * A(I + 1), A(1) - F(1) + A(3)\n"
           "      END\n",
           " 602 -96\n");
    /* argumento que es un elemento de array: copia de salida (A(2) cambia) */
    SALIDA("      INTEGER FUNCTION INCR(N)\n"
           "      INTEGER N\n"
           "      N = N + 10\n"
           "      INCR = N\n"
           "      END\n"
           "      INTEGER A(3), INCR, I\n"
           "      A(1) = 1\n"
           "      A(2) = 2\n"
           "      I = 2\n"
           "      PRINT *, INCR(A(I)), A(1), A(2)\n"
           "      END\n",
           " 12 1 12\n");
    /* funciones REAL y LOGICAL con argumentos de varios tipos; conversion del INTEGER al usar el resultado */
    SALIDA("      REAL FUNCTION PROM(N, X, Y)\n"
           "      INTEGER N\n"
           "      REAL X, Y\n"
           "      PROM = (X + Y) / N\n"
           "      END\n"
           "      LOGICAL FUNCTION MAYOR(X, Y)\n"
           "      REAL X, Y\n"
           "      MAYOR = X .GT. Y\n"
           "      END\n"
           "      REAL PROM\n"
           "      LOGICAL MAYOR\n"
           "      INTEGER K\n"
           "      K = PROM(2, 1.0, 4.0) + 0.5\n"
           "      PRINT *, K, PROM(4, 1.0, 1.0), MAYOR(PROM(2, 5.0, 1.0), 2.5)\n"
           "      END\n",
           " 3 0.500000000 T\n");
}

/* Diagnosticos de FUNCTION y de las referencias a funciones */
static void test_funciones_errores(void)
{
    static const char *F1 = "      INTEGER FUNCTION F(N)\n      INTEGER N\n      F = N\n      END\n";

    /* no definida */
    ERROR("      X = FOO(1)\n      END\n", 1, 1, 11, U(UNI_FUNCION_INDEFINIDA));
    ERROR("      PROGRAM P\n      INTEGER F\n      I = F(1)\n      END\n", 1, 3, 11, U(UNI_FUNCION_INDEFINIDA));
    ERROR("      I = 3\n      J = I(2)\n      END\n", 1, 2, 11, U(UNI_FUNCION_INDEFINIDA));   /* escalar usado con parentesis */
    ERROR("      X = 1.0\n      Y = FOO(1) + FOO(2)\n      END\n", 1, 2, 11, U(UNI_FUNCION_INDEFINIDA)); /* una sola vez */
    /* subrutina usada como funcion y al reves */
    ERROR("      SUBROUTINE S(N)\n      END\n      I = S(1)\n      END\n", 1, 3, 11, U(UNI_NO_FUNCION));
    {
        char p[256];

        sprintf(p, "%s      CALL F(1)\n      END\n", F1);
        ERROR(p, 1, 5, 12, U(UNI_NO_SUBRUTINA));
        sprintf(p, "%s      INTEGER F\n      I = F(1, 2)\n      END\n", F1);
        ERROR(p, 1, 6, 11, U(UNI_NUM_ARGS));
        sprintf(p, "%s      INTEGER F\n      I = F()\n      END\n", F1);
        ERROR(p, 1, 6, 11, U(UNI_NUM_ARGS));
        sprintf(p, "%s      INTEGER F\n      I = F(2.5)\n      END\n", F1);
        ERROR(p, 1, 6, 11, U(UNI_TIPO_ARG));
        sprintf(p, "%s      INTEGER F\n      I = F(.TRUE.)\n      END\n", F1);
        ERROR(p, 1, 6, 11, U(UNI_TIPO_ARG));
        /* el llamador ve el nombre como REAL (tipo implicito) y la funcion devuelve INTEGER */
        sprintf(p, "%s      I = F(1)\n      END\n", F1);
        ERROR(p, 1, 5, 11, U(UNI_TIPO_FUNCION));
        sprintf(p, "%s      LOGICAL F, L\n      L = F(1)\n      END\n", F1);
        ERROR(p, 1, 6, 11, U(UNI_TIPO_FUNCION));
        /* en cambio, bien declarada */
        sprintf(p, "%s      INTEGER F\n      X = F(1)\n      PRINT *, X\n      END\n", F1);
        SALIDA(p, " 1.00000000\n");
        /* definida dos veces */
        sprintf(p, "%s%s      INTEGER F\n      I = F(1)\n      END\n", F1, F1);
        ERROR(p, 1, 5, 7, U(UNI_YA_DEFINIDA));
        /* una funcion y una subrutina con el mismo nombre */
        sprintf(p, "%s      SUBROUTINE F(N)\n      END\n      END\n", F1);
        ERROR(p, 1, 5, 7, U(UNI_YA_DEFINIDA));
        /* una funcion y el programa */
        sprintf(p, "%s      PROGRAM F\n      END\n", F1);
        ERROR(p, 1, 5, 7, U(UNI_YA_DEFINIDA));
        /* el tipo del resultado dos veces (cabecera y declaracion) */
        ERROR("      INTEGER FUNCTION F(N)\n      INTEGER F\n      F = 1\n      END\n      END\n", 1, 2, 15, SEM_YA_DEFINIDO);
    }
    /* recursion */
    ERROR("      INTEGER FUNCTION F(N)\n      F = F(N)\n      END\n      INTEGER F\n      I = F(1)\n      END\n", 1, 1, 7,
          U(UNI_RECURSIVA));
    ERROR("      INTEGER FUNCTION F(N)\n      INTEGER G\n      F = G(N)\n      END\n"
          "      INTEGER FUNCTION G(N)\n      INTEGER F\n      G = F(N)\n      END\n      INTEGER F\n      I = F(1)\n      END\n",
          2, 1, 7, U(UNI_RECURSIVA));
    /* cabecera */
    ERROR("      INTEGER FUNCTION F(N, N)\n      END\n      END\n", 1, 1, 29, U(UNI_ARG_DUPLICADO));
    ERROR("      INTEGER FUNCTION F(F)\n      END\n      END\n", 1, 1, 24, U(UNI_ARG_DUPLICADO));
    ERROR("   10 FUNCTION F(N)\n      END\n      END\n", 1, 1, 7, SEM_ETIQUETA_EN_DECL);
    ERROR("      END\n      INTEGER FUNCTION F(N)\n      FUNCTION G(N)\n      END\n", 1, 3, 7, U(UNI_CABECERA));
    ERROR("      INTEGER FUNCTION F(N\n      END\n      END\n", 1, 1, 27, MF_COD_STMT + STM_PAR_CIERRA_ESPERADO);
    ERROR("      FUNCTION F(1)\n      END\n      END\n", 1, 1, 18, MF_COD_STMT + STM_NOMBRE_ESPERADO);
    ERROR("      FUNCTION F(N) X\n      END\n      END\n", 1, 1, 21, MF_COD_STMT + STM_SOBRA);
    ERROR("      LOGICAL FUNCTION F(N)\n      F = N\n      END\n      END\n", 1, 2, 7, SEM_INCOMPATIBLES);
    /* argumentos */
    ERROR("      INTEGER FUNCTION F(N)\n      INTEGER N, G\n      F = N\n"
          "      N = G(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)\n      END\n      END\n",
          1, 4, 64, U(UNI_MUCHOS_ARGS));
    ERROR("      I = F(1:2)\n      END\n", 1, 1, 13, SEM_RANGO_NO_PERMITIDO);
    ERROR("      INTEGER F\n      I = F(1:2)\n      END\n", 1, 2, 11, SEM_SUBCADENA);
    /* todavia no soportado: funciones DOUBLE PRECISION, y cadenas como argumento */
    ERROR("      DOUBLE PRECISION FUNCTION F(N)\n      END\n      END\n", 1, 1, 33, SEM_GEN_NO_SOPORTADO);
    ERROR("      INTEGER F\n      I = F('A')\n      END\n", 1, 2, 11, U(UNI_FUNCION_INDEFINIDA));
    /* una funcion no es una variable que se asigna con argumentos */
    ERROR("      F(1) = 2\n      END\n", 1, 1, 7, SEM_NO_ES_ARRAY);
    /* un error en un argumento no genera errores en cascada */
    {
        char p[256];

        sprintf(p, "%s      INTEGER F\n      I = F(1 + .TRUE.)\n      END\n", F1);
        ERROR(p, 1, 6, 15, SEM_NUMERICO_ESPERADO);
    }
}


/* ------------------------------------------------------------------
 * S2b: CHARACTER*n FUNCTION
 * ------------------------------------------------------------------ */
static void test_funciones_char(void)
{
    /* longitud en la cabecera: se rellena con blancos y se trunca */
    SALIDA("      CHARACTER*4 FUNCTION NOMBRE(N)\n"
           "      INTEGER N\n"
           "      IF (N .EQ. 1) THEN\n"
           "         NOMBRE = 'UNO'\n"
           "      ELSE IF (N .EQ. 2) THEN\n"
           "         NOMBRE = 'DOS'\n"
           "      ELSE\n"
           "         NOMBRE = 'MUCHOS'\n"
           "      END IF\n"
           "      END\n"
           "      CHARACTER*4 NOMBRE\n"
           "      CHARACTER*10 S\n"
           "      INTEGER I\n"
           "      DO 10 I = 1, 3\n"
           "         PRINT *, NOMBRE(I)\n"
           "   10 CONTINUE\n"
           "      S = NOMBRE(1) // '+' // NOMBRE(2)\n"
           "      PRINT *, S\n"
           "      IF (NOMBRE(2) .EQ. 'DOS') PRINT *, 'IGUAL'\n"
           "      IF (NOMBRE(1) .GT. NOMBRE(2)) PRINT *, 'MAYOR'\n"
           "      PRINT *, NOMBRE(3) // '|'\n"
           "      PRINT *, LEN(NOMBRE(1))\n"
           "      END\n",
           " UNO \n DOS \n MUCH\n UNO +DOS  \n IGUAL\n MAYOR\n MUCH|\n 4\n");
    /* longitud 1 por omision, longitud entre parentesis, y declarada dentro de la funcion */
    SALIDA("      CHARACTER FUNCTION A1()\n"
           "      A1 = 'XYZ'\n"
           "      END\n"
           "      CHARACTER*(3) FUNCTION B3(K)\n"
           "      B3 = 'ABCDE'\n"
           "      END\n"
           "      FUNCTION C5(K)\n"
           "      CHARACTER C5*5\n"
           "      C5 = 'PQ'\n"
           "      END\n"
           "      CHARACTER A1, B3*3, C5*5\n"
           "      PRINT *, A1(), '|', B3(1), '|', C5(2), '|'\n"
           "      END\n",
           " X | ABC | PQ    |\n");
    /* subcadenas con limites variables dentro de la funcion y funciones que se llaman entre si */
    SALIDA("      CHARACTER*6 FUNCTION GUARDA(N)\n"
           "      INTEGER N\n"
           "      CHARACTER*6 P, A\n"
           "      P = '......'\n"
           "      A = '######'\n"
           "      GUARDA = A(1:N) // P(1:6-N)\n"
           "      END\n"
           "      CHARACTER*3 FUNCTION CORTA(N)\n"
           "      INTEGER N\n"
           "      CHARACTER*6 GUARDA\n"
           "      CORTA = GUARDA(N)\n"
           "      END\n"
           "      CHARACTER*6 GUARDA\n"
           "      CHARACTER*3 CORTA\n"
           "      PRINT *, GUARDA(1), GUARDA(3), GUARDA(5)\n"
           "      PRINT *, CORTA(4), CORTA(1)\n"
           "      END\n",
           " #..... ###... #####.\n ### #..\n");
    /* el resultado de una llamada sirve de dato de otra */
    SALIDA("      CHARACTER*2 FUNCTION PAR(N)\n"
           "      INTEGER N\n"
           "      IF (MOD(N, 2) .EQ. 0) THEN\n"
           "         PAR = 'SI'\n"
           "      ELSE\n"
           "         PAR = 'NO'\n"
           "      END IF\n"
           "      END\n"
           "      CHARACTER*2 PAR\n"
           "      CHARACTER*8 T\n"
           "      T = PAR(1) // PAR(2) // PAR(3) // PAR(4)\n"
           "      PRINT *, T\n"
           "      END\n",
           " NOSINOSI\n");
}

static void test_funciones_char_errores(void)
{
    static const char *F4 = "      CHARACTER*4 FUNCTION F(N)\n      F = 'A'\n      END\n";
    char p[256];

    /* la longitud que ve el llamador debe ser la de la funcion */
    sprintf(p, "%s      CHARACTER*3 F\n      PRINT *, F(1)\n      END\n", F4);
    ERROR(p, 1, 5, 16, U(UNI_LONG_FUNCION));
    sprintf(p, "%s      CHARACTER F\n      PRINT *, F(1)\n      END\n", F4);
    ERROR(p, 1, 5, 16, U(UNI_LONG_FUNCION));
    /* y el tipo */
    sprintf(p, "%s      PRINT *, F(1)\n      END\n", F4);
    ERROR(p, 1, 4, 16, U(UNI_TIPO_FUNCION));
    sprintf(p, "%s      INTEGER F\n      PRINT *, F(1)\n      END\n", F4);
    ERROR(p, 1, 5, 16, U(UNI_TIPO_FUNCION));
    /* una funcion numerica usada como CHARACTER */
    ERROR("      INTEGER FUNCTION F(N)\n      F = N\n      END\n      CHARACTER*4 F\n      PRINT *, F(1)\n      END\n",
          1, 5, 16, U(UNI_TIPO_FUNCION));
    /* longitud de la cabecera */
    ERROR("      CHARACTER*0 FUNCTION F(N)\n      END\n      END\n", 1, 1, 16, MF_COD_STMT + STM_LONGITUD);
}


/* ------------------------------------------------------------------
 * S2c: funciones de sentencia  F(X) = expr
 * ------------------------------------------------------------------ */
static void test_funciones_sentencia(void)
{
    /* de los cuatro tipos, con argumentos que son expresiones y llamadas anidadas */
    SALIDA("      REAL X, CUAD, MEDIA\n"
           "      INTEGER I, DOBLE, SUMA3\n"
           "      LOGICAL ESPAR\n"
           "      CUAD(X) = X * X\n"
           "      MEDIA(X, Y) = (X + Y) / 2.0\n"
           "      DOBLE(I) = 2 * I\n"
           "      SUMA3(I, J, K) = I + J + K\n"
           "      ESPAR(I) = MOD(I, 2) .EQ. 0\n"
           "      N = 5\n"
           "      PRINT *, CUAD(3.0), CUAD(1.5 + 0.5)\n"
           "      PRINT *, MEDIA(1.0, 4.0)\n"
           "      PRINT *, DOBLE(N), DOBLE(DOBLE(3))\n"
           "      PRINT *, SUMA3(1, 2, 3), SUMA3(N, N, DOBLE(N))\n"
           "      IF (ESPAR(4)) PRINT *, 'PAR'\n"
           "      IF (.NOT. ESPAR(N)) PRINT *, 'IMPAR'\n"
           "      END\n",
           " 9.00000000 4.00000000\n 2.50000000\n 10 12\n 6 20\n PAR\n IMPAR\n");
    /* el cuerpo ve las variables de la unidad; el ficticio oculta una variable del mismo nombre; en bucles, subindices,
     * funciones intrinsecas y del usuario; en una subrutina y anidadas */
    SALIDA("      INTEGER FUNCTION TRIPLE(N)\n"
           "      INTEGER N\n"
           "      TRIPLE = 3 * N\n"
           "      END\n"
           "      SUBROUTINE S(A)\n"
           "      INTEGER A, SQ, CUBO\n"
           "      SQ(A) = A * A\n"
           "      CUBO(K) = K * SQ(K)\n"
           "      PRINT *, SQ(A), CUBO(2), CUBO(SQ(2))\n"
           "      END\n"
           "      PROGRAM MAIN\n"
           "      INTEGER X, Y, F, G, H, V(3), I, TRIPLE\n"
           "      F(X) = X + 100\n"
           "      G(Y) = F(Y) * 2 + X\n"
           "      H(I) = V(I) + TRIPLE(I) + ABS(-I)\n"
           "      X = 7\n"
           "      Y = 1\n"
           "      V(1) = 10\n"
           "      V(2) = 20\n"
           "      V(3) = 30\n"
           "      PRINT *, F(1), F(Y), F(X), G(5), G(X)\n"
           "      PRINT *, H(1), H(2), H(3)\n"
           "      DO 10 I = F(0) - 98, F(1) - 98\n"
           "         PRINT *, I, V(F(I) - 100)\n"
           "   10 CONTINUE\n"
           "      CALL S(4)\n"
           "      PRINT *, F(F(F(0)))\n"
           "      END\n",
           " 101 101 107 217 221\n 14 28 42\n 2 20\n 3 30\n 16 8 64\n 300\n");
    /* cada unidad tiene las suyas, aunque se llamen igual */
    SALIDA("      SUBROUTINE A\n"
           "      INTEGER F\n"
           "      F(K) = K + 1\n"
           "      PRINT *, F(1)\n"
           "      END\n"
           "      SUBROUTINE B\n"
           "      INTEGER F\n"
           "      F(K) = K * 10\n"
           "      PRINT *, F(1)\n"
           "      END\n"
           "      INTEGER FUNCTION C(N)\n"
           "      INTEGER N, F\n"
           "      F(K) = K - N\n"
           "      C = F(10)\n"
           "      END\n"
           "      INTEGER C\n"
           "      CALL A\n"
           "      CALL B\n"
           "      PRINT *, C(3), C(4)\n"
           "      END\n",
           " 2\n 10\n 7 6\n");
    /* las expansiones anidadas no pisan los argumentos de la que las contiene (G ata X := 6 mientras H ata X := 5) */
    SALIDA("      INTEGER G, H, X\n      G(X) = X + 1\n      H(X) = G(X + 1) * 10 + X\n      PRINT *, H(5), G(H(1))\n      END\n",
           " 75 32\n");
    /* el valor del cuerpo se convierte al tipo de la funcion, como en una asignacion */
    SALIDA("      INTEGER F\n      REAL R\n      F(I) = I * 1.5\n      R(I) = I\n      PRINT *, F(3), R(3), F(-3)\n      END\n",
           " 4 3.00000000 -4\n");
    /* un ficticio sin declarar toma el tipo implicito de su nombre y el de la funcion lo da el llamador (aqui, declarado) */
    SALIDA("      INTEGER F\n      F(X) = X * 2\n      PRINT *, F(1.5)\n      END\n", " 3\n");
}

static void test_funciones_sentencia_errores(void)
{
    char p[1024];
    char cuerpo[64];
    int k;

    ERROR("      F(X) = X\n      F(X) = X + 1\n      END\n", 1, 2, 7, U(UNI_FSENT_DUPLICADA));
    ERROR("      F(X) = X\n      I = F(1, 2)\n      END\n", 1, 2, 11, U(UNI_NUM_ARGS));
    ERROR("      F(X) = X\n      I = F()\n      END\n", 1, 2, 11, U(UNI_NUM_ARGS));
    ERROR("      F(X) = X\n      I = F(1)\n      END\n", 1, 2, 13, U(UNI_TIPO_ARG));
    ERROR("      INTEGER F\n      F(I) = I\n      X = F(2.5)\n      END\n", 1, 3, 13, U(UNI_TIPO_ARG));
    ERROR("      LOGICAL F\n      F(I) = I\n      END\n", 1, 2, 7, SEM_INCOMPATIBLES);
    ERROR("      F(X, X) = X\n      END\n", 1, 1, 12, U(UNI_ARG_DUPLICADO));
    ERROR("      F(A, B, C, D, E, G, H, K, L) = A\n      END\n", 1, 1, 33, U(UNI_MUCHOS_ARGS));
    /* CHARACTER: etapa S3 */
    ERROR("      CHARACTER*3 F\n      F(I) = 1\n      END\n", 1, 2, 7, SEM_GEN_NO_SOPORTADO);
    /* tras una sentencia ejecutable ya no es una definicion; tampoco se asigna a una funcion */
    ERROR("      I = 1\n      F(X) = X\n      END\n", 1, 2, 7, SEM_NO_ES_ARRAY);
    ERROR("      F(X) = X\n      F(1) = 2\n      END\n", 1, 2, 7, SEM_NO_ES_ARRAY);
    ERROR("      F(1) = 2\n      END\n", 1, 1, 7, SEM_NO_ES_ARRAY);
    ERROR("      ABS(X) = X\n      END\n", 1, 1, 7, SEM_NO_ES_ARRAY);
    /* no hay recursion */
    ERROR("      F(X) = F(X)\n      END\n", 1, 1, 14, U(UNI_FUNCION_INDEFINIDA));
    /* y un array no es una funcion de sentencia */
    SALIDA("      INTEGER A(3)\n      A(2) = 5\n      I = 2\n      A(I) = A(I) + 1\n      PRINT *, A(2)\n      END\n", " 6\n");
    /* demasiado grande: el cuerpo copiado no cabe en el arbol de la sentencia, o no caben mas en la tabla */
    strcpy(cuerpo, "I");
    for (k = 1; k < 11; k++) {
        strcat(cuerpo, " + I");
    }
    sprintf(p, "      F(I) = %s\n      J = F(1) + F(2) + F(3) + F(4)\n      END\n", cuerpo);
    ERROR(p, 2, 2, 25, U(UNI_FSENT_GRANDE));
    p[0] = 0;
    for (k = 1; k <= 14; k++) {
        sprintf(p + strlen(p), "      F%d(I) = %s\n", k, cuerpo);
    }
    strcat(p, "      END\n");
    ERROR(p, 1, 14, 7, U(UNI_FSENT_GRANDE));
}

/* ------------------------------------------------------------------ limites de tablas */
static void test_limites(void)
{
    static char prog[40000], esp[2048];
    char l[96];
    int i, n;

    /* 63 unidades (el maximo): el principal y 62 subrutinas */
    for (n = 62; n <= 63; n++) {
        prog[0] = '\0';
        esp[0] = '\0';
        for (i = 1; i <= n; i++) {
            sprintf(l, "      CALL S%d\n", i);
            strcat(prog, l);
        }
        strcat(prog, "      END\n");
        for (i = 1; i <= n; i++) {
            sprintf(l, "      SUBROUTINE S%d\n      PRINT *, %d\n      END\n", i, i);
            strcat(prog, l);
            if (n == 62) {
                sprintf(l, " %d\n", i);
                strcat(esp, l);
            }
        }
        if (n == 62) {
            SALIDA(prog, esp);
        } else {
            pruebas++;
            if (a_tac(prog) < 1 || mf.diag[0].cod != U(UNI_TABLA_LLENA)) {
                printf("FALLO (linea %d): 64 unidades: %d errores, primero cod %d\n", __LINE__, (int)mf.n, (int)mf.diag[0].cod);
                fallos++;
            }
        }
    }
    /* demasiadas llamadas en el programa */
    prog[0] = '\0';
    for (i = 0; i < 800; i++) {
        strcat(prog, "      CALL S\n");
    }
    strcat(prog, "      END\n      SUBROUTINE S\n      END\n");
    pruebas++;
    if (a_tac(prog) < 1 || mf.diag[0].cod != U(UNI_MUCHAS_LLAMADAS)) {
        printf("FALLO (linea %d): 800 llamadas: %d errores, primero cod %d linea %d\n", __LINE__, (int)mf.n, (int)mf.diag[0].cod,
               (int)mf.diag[0].linea);
        fallos++;
    }
}

/* ------------------------------------------------------------------
 * Fuzzer con oraculo independiente
 *
 * Se genera un arbol de programa aleatorio (DAG de subrutinas, en orden aleatorio dentro del fichero) que se IMPRIME como Fortran
 * y se EJECUTA en C con la semantica real de F77: paso POR REFERENCIA (el formal es un puntero a la celda del llamador),
 * variables estaticas, INTEGER de 16 bits con desbordamiento ciclico y REAL de 32 bits. No pasa por el TAC ni por el backend.
 * Reglas para que el programa sea legal en F77 (sin alias): un formal que la subrutina puede escribir recibe SIEMPRE una variable
 * del llamador que se pueda escribir, distinta de todo lo demas de esa llamada; los demas formales reciben cualquier expresion.
 * ------------------------------------------------------------------ */
#include "realfmt.h"

enum { TI, TR, TL };
enum { X_CI, X_CR, X_CL, X_VAR, X_ADD, X_SUB, X_MUL, X_DIV, X_MOD, X_NEG, X_RC, X_AND, X_OR, X_NOT, X_CMPI, X_CMPR, X_CALL };
enum { SA_ASSIGN, SA_IF, SA_LOGRET, SA_DO, SA_SKIP, SA_CALL, SA_PRINT, SA_FASSIGN };
#define FNV 13
#define FNU 7
#define FK1 10
#define FK2 11
#define FRES 12                 /* variable resultado de una FUNCTION */

typedef struct { uint8_t op, t; int16_t a, b; int32_t iv; float fv, k; } FEx;
typedef struct { int32_t i; float f; } Val;
typedef struct St {
    uint8_t k, v;
    int16_t e, n, callee, na;
    int16_t arg[16];
    struct St *a, *b, *next;
} St;
typedef struct { int nf, fv[4], fw[4], isf[FNV]; int isfn, rt, anyw; St *body; } FU;

static const char *fvn[FNV] = {"A1", "A2", "A3", "B1", "B2", "C1", "C2", "V(1)", "V(2)", "V(3)", "K1", "K2", "?"};
static const char *frel[6] = {".LT.", ".LE.", ".GT.", ".GE.", ".EQ.", ".NE."};
static const float fcs[2] = {0.25f, 0.5f};
static const float fks[6] = {0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f};
static const char *ftn[3] = {"INTEGER", "REAL", "LOGICAL"};

static FEx fex[8192];
static int nfex;
static St stp[3000];
static int nst;
static int16_t fca[600][4];     /* argumentos de las referencias a funcion en expresiones */
static int nfca;
static FU fu[FNU];
static int fnu;
static Val cel[FNU][FNV];
static Val *bnd[FNU][FNV];
static uint32_t fs;
static int fabort, fsteps, fcalls, fol;
static char fo[8192];
static int ulbl;
static int curu;                /* unidad que se esta imprimiendo */
static int est_fn, est_xcall, est_fasg;   /* cobertura del fuzzer: programas con funciones, con referencias en expresiones y con V = F(...) */
static int fpuro;               /* se esta generando el cuerpo de una funcion: sin E/S ni CALL */
static char ubuf[FNU][12288];

static int fvt(int u, int v)
{
    return v == FRES ? fu[u].rt : v <= 2 ? TI : v <= 4 ? TR : v <= 6 ? TL : TI;
}

static uint32_t fr(void)
{
    fs = fs * 1664525u + 1013904223u;
    return fs >> 10;
}

static int mk(int op, int t, int a, int b, int iv, float fv, float k)
{
    FEx *x;

    if (nfex >= 8190) {
        fabort = 1;
        return 0;
    }
    x = &fex[nfex];
    x->op = (uint8_t)op;
    x->t = (uint8_t)t;
    x->a = (int16_t)a;
    x->b = (int16_t)b;
    x->iv = iv;
    x->fv = fv;
    x->k = k;
    return nfex++;
}

static St *ns(int k)
{
    St *s;

    if (nst >= 2990) {
        fabort = 1;
        nst = 0;
    }
    s = &stp[nst++];
    memset(s, 0, sizeof(*s));
    s->k = (uint8_t)k;
    return s;
}

static int fwr(int u, int v) /* el programa puede escribir esta variable de la unidad u */
{
    if (v > 9 && v != FRES) {
        return 0;
    }
    if (v == FRES) {
        return fu[u].isfn;
    }
    if (!fu[u].isf[v]) {
        return 1;
    }
    return fu[u].fw[fu[u].isf[v] - 1];
}

static int cands(int u, int t, int ld, uint32_t fb, int *out)
{
    int v, n = 0;

    for (v = 0; v < FNV; v++) {
        if (v == FRES && !fu[u].isfn) {
            continue;
        }
        if (fvt(u, v) != t || ((fb >> v) & 1u)) {
            continue;
        }
        if ((v == FK1 && ld < 1) || (v == FK2 && ld < 2)) {
            continue;
        }
        out[n++] = v;
    }
    return n;
}

static int konst(int t)
{
    int r;

    if (t == TI) {
        r = (int)(fr() % 10);
        return mk(X_CI, TI, 0, 0, r == 0 ? 32000 : r == 1 ? -32000 : (int)(fr() % 21) - 10, 0, 0);
    }
    if (t == TR) {
        return mk(X_CR, TR, 0, 0, 0, fks[fr() % 6], 0);
    }
    return mk(X_CL, TL, 0, 0, (int)(fr() & 1), 0, 0);
}

static int genex(int u, int t, int d, int ld, uint32_t fb);

/* Nodos del AST que ocupara la expresion (el compilador tiene 64 por sentencia: "expression too large"). Cota por exceso. */
static int est(int e)
{
    FEx *x = &fex[e];
    int i, n;

    switch (x->op) {
    case X_CI: return x->iv < 0 ? 3 : 1;
    case X_CR: case X_CL: return 1;
    case X_VAR: return x->iv >= 7 && x->iv <= 9 ? 3 : 1;
    case X_CALL:
        n = 1 + 2 * fu[x->iv].nf;
        for (i = 0; i < fu[x->iv].nf; i++) {
            n += est(fca[x->a][i]);
        }
        return n;
    case X_DIV: return 3 + est(x->a);
    case X_MOD: return 5 + est(x->a);
    case X_NEG: case X_NOT: return 2 + est(x->a);
    case X_RC: return 7 + est(x->a) + est(x->b);
    default: return 2 + est(x->a) + est(x->b);
    }
}

/* Profundidad de parser que gastara (EXPR_PROF_MAX, hoy pequena por la pila del Next). Cota por exceso: todo
 * operador va entre parentesis (1), una referencia o un elemento de vector gasta 2 + su argumento. */
static int prof(int e)
{
    FEx *x = &fex[e];
    int i, n, m;

    switch (x->op) {
    case X_CI: case X_CR: case X_CL: return 0;
    case X_VAR: return x->iv >= 7 && x->iv <= 9 ? 3 : 0;
    case X_CALL:
        m = 0;
        for (i = 0; i < fu[x->iv].nf; i++) {
            n = prof(fca[x->a][i]);
            m = n > m ? n : m;
        }
        return 3 + m;
    case X_DIV: case X_MOD: case X_NEG: case X_NOT: return 1 + prof(x->a);
    default:
        n = prof(x->a);
        m = prof(x->b);
        return 1 + (n > m ? n : m);
    }
}

#define EST_MAX 50

static int gen_top(int u, int t, int d, int ld)
{
    int k, e = 0;

    for (k = 0; k < 6; k++) {
        e = genex(u, t, d, ld, 0);
        if (est(e) <= EST_MAX && prof(e) + 3 <= EXPR_PROF_MAX) {
            return e;
        }
    }
    return konst(t);
}

/* Argumentos de una llamada a la unidad c desde u: los que la unidad puede escribir, variables distintas del llamador;
 * los demas, cualquier expresion. 0 si no se pueden formar. */
static int gen_args(int u, int c, int ld, int16_t *arg)
{
    int i, v, n, k, m, cd[FNV];
    uint32_t used = 0;

    for (i = 0; i < fu[c].nf; i++) { /* primero los que se escriben */
        int t = fvt(c, fu[c].fv[i]);

        if (!fu[c].fw[i]) {
            continue;
        }
        n = cands(u, t, 0, used, cd);
        for (k = 0, m = 0; k < n; k++) {
            if (fwr(u, cd[k])) {
                cd[m++] = cd[k];
            }
        }
        if (m == 0) {
            return 0;
        }
        v = cd[fr() % (uint32_t)m];
        used |= 1u << v;
        arg[i] = (int16_t)mk(X_VAR, t, 0, 0, v, 0, 0);
    }
    for (i = 0; i < fu[c].nf; i++) {
        int t = fvt(c, fu[c].fv[i]);

        if (fu[c].fw[i]) {
            continue;
        }
        n = cands(u, t, ld, used, cd);
        if (n > 0 && fr() % 3 == 0) {
            arg[i] = (int16_t)mk(X_VAR, t, 0, 0, cd[fr() % (uint32_t)n], 0, 0);
        } else {
            arg[i] = (int16_t)genex(u, t, 1, ld, used);
        }
    }
    return 1;
}

/* Una referencia a una funcion pura (ningun formal que escriba) de tipo t y mayor indice que u; -1 si no hay. */
static int gen_fcall(int u, int t, int ld)
{
    int c, tries, i, slot;

    if (nfca >= 590) {
        return -1;
    }
    for (tries = 0; tries < 4; tries++) {
        if (u >= fnu - 1) {
            return -1;
        }
        c = u + 1 + (int)(fr() % (uint32_t)(fnu - u - 1));
        if (!fu[c].isfn || fu[c].rt != t || fu[c].anyw) {
            continue;
        }
        slot = nfca++;                      /* se reserva antes: los argumentos pueden contener otras llamadas */
        for (i = 0; i < 4; i++) {
            fca[slot][i] = 0;
        }
        if (nfca >= 590 || !gen_args(u, c, ld, fca[slot])) {
            continue;
        }
        return mk(X_CALL, t, slot, 0, c, 0, 0);
    }
    return -1;
}

static int leaf(int u, int t, int ld, uint32_t fb)
{
    int c[FNV], n = cands(u, t, ld, fb, c), r;

    if (fr() % 5 == 0 && (r = gen_fcall(u, t, ld)) >= 0) {
        return r;
    }
    if (n > 0 && fr() % 3 != 0) {
        return mk(X_VAR, t, 0, 0, c[fr() % (uint32_t)n], 0, 0);
    }
    return konst(t);
}

static int genex(int u, int t, int d, int ld, uint32_t fb)
{
    int a, b;

    if (d == 0 || fr() % 4 == 0) {
        return leaf(u, t, ld, fb);
    }
    if (t == TI) {
        switch (fr() % 7) {
        case 0: a = genex(u, TI, d - 1, ld, fb); b = genex(u, TI, d - 1, ld, fb); return mk(X_ADD, TI, a, b, 0, 0, 0);
        case 1: a = genex(u, TI, d - 1, ld, fb); b = genex(u, TI, d - 1, ld, fb); return mk(X_SUB, TI, a, b, 0, 0, 0);
        case 2: a = genex(u, TI, d - 1, ld, fb); b = genex(u, TI, d - 1, ld, fb); return mk(X_MUL, TI, a, b, 0, 0, 0);
        case 3: a = genex(u, TI, d - 1, ld, fb); return mk(X_DIV, TI, a, 0, 2 + (int)(fr() % 4), 0, 0);
        case 4: a = genex(u, TI, d - 1, ld, fb); return mk(X_MOD, TI, a, 0, 2 + (int)(fr() % 4), 0, 0);
        default: a = genex(u, TI, d - 1, ld, fb); return mk(X_NEG, TI, a, 0, 0, 0, 0);
        }
    }
    if (t == TR) {
        a = genex(u, TR, d - 1, ld, fb);
        b = genex(u, TR, d - 1, ld, fb);
        return mk(X_RC, TR, a, b, 0, fcs[fr() % 2], fks[fr() % 6]);
    }
    switch (fr() % 5) {
    case 0: a = genex(u, TL, d - 1, ld, fb); return mk(X_NOT, TL, a, 0, 0, 0, 0);
    case 1: a = genex(u, TL, d - 1, ld, fb); b = genex(u, TL, d - 1, ld, fb); return mk(X_AND, TL, a, b, 0, 0, 0);
    case 2: a = genex(u, TL, d - 1, ld, fb); b = genex(u, TL, d - 1, ld, fb); return mk(X_OR, TL, a, b, 0, 0, 0);
    case 3: a = genex(u, TI, d - 1, ld, fb); b = genex(u, TI, d - 1, ld, fb); return mk(X_CMPI, TL, a, b, (int)(fr() % 6), 0, 0);
    default: a = genex(u, TR, d - 1, ld, fb); b = konst(TR); return mk(X_CMPR, TL, a, b, (int)(fr() % 6), 0, 0);
    }
}

static St *gen_list(int u, int n, int ld);

static void dump(St **tail, int u)
{
    St *s;
    int i;
    static const int v1[6] = {0, 1, 2, 7, 8, 9};
    static const int v2[4] = {3, 4, 5, 6};

    (void)u;
    s = ns(SA_PRINT);
    for (i = 0; i < 6; i++) {
        s->arg[s->na++] = (int16_t)mk(X_VAR, TI, 0, 0, v1[i], 0, 0);
    }
    (*tail)->next = s;
    *tail = s;
    s = ns(SA_PRINT);
    for (i = 0; i < 4; i++) {
        s->arg[s->na++] = (int16_t)mk(X_VAR, fvt(u, v2[i]), 0, 0, v2[i], 0, 0);
    }
    (*tail)->next = s;
    *tail = s;
}

/* CALL a una subrutina (kind = SA_CALL) o V = FUNCION(...) con una funcion que escribe sus argumentos (SA_FASSIGN). */
static St *gen_call(int u, int ld, int fasg)
{
    St *s;
    int c, tries, v, m, k, n, cd[FNV];

    if (u >= fnu - 1) {
        return NULL;
    }
    for (tries = 0; tries < 4; tries++) {
        c = u + 1 + (int)(fr() % (uint32_t)(fnu - u - 1));
        if (fasg ? (!fu[c].isfn || !fu[c].anyw) : fu[c].isfn) {
            continue;
        }
        s = ns(fasg ? SA_FASSIGN : SA_CALL);
        s->callee = (int16_t)c;
        s->na = (int16_t)fu[c].nf;
        if (!gen_args(u, c, ld, s->arg)) {
            continue;
        }
        for (k = 0, m = 0; k < s->na; k++) {
            m += est(s->arg[k]);
        }
        if (m > EST_MAX - 2 * s->na) {
            continue;
        }
        if (fasg) {
            n = cands(u, fu[c].rt, 0, 0, cd);
            for (k = 0, m = 0; k < n; k++) {
                if (fwr(u, cd[k])) {
                    cd[m++] = cd[k];
                }
            }
            if (m == 0) {
                continue;
            }
            v = cd[fr() % (uint32_t)m];
            s->v = (uint8_t)v;
        }
        return s;
    }
    return NULL;
}

static St *gen_stmt(int u, int ld)
{
    St *s, *t;
    int r = (int)(fr() % 14), v, n, c[FNV], k, m;

    switch (r) {
    case 0: case 1: case 2: case 3:
        for (v = 0, m = 0; v < FNV; v++) {
            if ((v < 10 || v == FRES) && (v != FRES || fu[u].isfn) && fwr(u, v)) {
                c[m++] = v;
            }
        }
        v = c[fr() % (uint32_t)m];
        s = ns(SA_ASSIGN);
        s->v = (uint8_t)v;
        s->e = (int16_t)gen_top(u, fvt(u, v), 2, ld);
        return s;
    case 4:
        s = ns(SA_IF);
        s->e = (int16_t)gen_top(u, TL, 2, ld);
        s->a = gen_list(u, 1 + (int)(fr() % 2), ld);
        s->b = (fr() & 1) ? gen_list(u, 1 + (int)(fr() % 2), ld) : NULL;
        return s;
    case 5:
        if (u == 0) {
            break;
        }
        s = ns(SA_LOGRET);
        s->e = (int16_t)gen_top(u, TL, 2, ld);
        return s;
    case 6:
        if (ld >= 2) {
            break;
        }
        s = ns(SA_DO);
        s->v = (uint8_t)(ld == 0 ? FK1 : FK2);
        s->n = (int16_t)(1 + fr() % 3);
        s->a = gen_list(u, 1 + (int)(fr() % 3), ld + 1);
        return s;
    case 7:
        s = ns(SA_SKIP);
        s->e = (int16_t)gen_top(u, TL, 2, ld);
        s->a = gen_list(u, 1 + (int)(fr() % 2), ld);
        return s;
    case 8: case 9:
        if (fpuro) {
            break;
        }
        s = gen_call(u, ld, 0);
        if (s != NULL) {
            if (fr() & 1) {
                t = s;
                dump(&t, u);
            }
            return s;
        }
        break;
    case 10:
        s = gen_call(u, ld, 1);                      /* V = F(...) con F que escribe sus argumentos */
        if (s != NULL) {
            if (!fpuro && (fr() & 1)) {
                t = s;
                dump(&t, u);
            }
            return s;
        }
        break;
    default:
        break;
    }
    if (fpuro) {                                      /* una funcion no escribe: asigna */
        for (v = 0, m = 0; v < FNV; v++) {
            if ((v < 10 || v == FRES) && (v != FRES || fu[u].isfn) && fwr(u, v)) {
                c[m++] = v;
            }
        }
        v = c[fr() % (uint32_t)m];
        s = ns(SA_ASSIGN);
        s->v = (uint8_t)v;
        s->e = (int16_t)gen_top(u, fvt(u, v), 2, ld);
        return s;
    }
    s = ns(SA_PRINT);
    n = 1 + (int)(fr() % 3);
    for (k = 0; k < n; k++) {
        int t2 = (int)(fr() % 3);

        s->arg[s->na++] = (int16_t)gen_top(u, t2, 2, ld);
    }
    return s;
}

static St *gen_list(int u, int n, int ld)
{
    St *h = NULL, *t = NULL, *s;
    int i;

    for (i = 0; i < n; i++) {
        s = gen_stmt(u, ld);
        if (h == NULL) {
            h = s;
        } else {
            t->next = s;
        }
        for (t = s; t->next != NULL; t = t->next) {
        }
    }
    return h;
}

static void gen_unit(int u)
{
    FU *f = &fu[u];
    St *h = NULL, *t = NULL, *s;
    int v, i, j;

    memset(f, 0, sizeof(*f));
    if (u > 0) {
        f->isfn = (fr() % 5) < 2;
        f->rt = (int)(fr() % 3);
        f->nf = (int)(fr() % 5);
        for (i = 0; i < f->nf; i++) {
            do {
                v = (int)(fr() % 7);
            } while (f->isf[v]);
            f->fv[i] = v;
            f->isf[v] = i + 1;
            f->fw[i] = (int)(fr() & 1);
            if (f->fw[i]) {
                f->anyw = 1;
            }
        }
    }
    fpuro = f->isfn;
    for (v = 0; v <= FRES; v++) { /* los locales se inicializan al entrar */
        if (v >= 10 && v < FRES) {
            continue;
        }
        if (v == FRES && !f->isfn) {
            continue;
        }
        if (v < 10 && f->isf[v]) {
            continue;
        }
        s = ns(SA_ASSIGN);
        s->v = (uint8_t)v;
        s->e = (int16_t)konst(fvt(u, v));
        if (h == NULL) {
            h = t = s;
        } else {
            t->next = s;
            t = s;
        }
    }
    if (h == NULL) {
        h = t = ns(SA_PRINT);
        h->arg[h->na++] = (int16_t)konst(TI);
    }
    j = 3 + (int)(fr() % 5);
    t->next = gen_list(u, j, 0);
    for (; t->next != NULL; t = t->next) {
    }
    if (!f->isfn) {
        dump(&t, u);
    }
    f->body = h;
    fpuro = 0;
}

/* ---------- oraculo ---------- */
static int exec(int u, St *s);
static void do_call(int u, int c, const int16_t *arg, int na);
static Val eval(int u, int e);

static Val eval(int u, int e)
{
    FEx *x = &fex[e];
    Val r, a, b;
    volatile float vf;

    r.i = 0;
    r.f = 0;
    switch (x->op) {
    case X_CI: case X_CL: r.i = x->iv; break;
    case X_CR: r.f = x->fv; break;
    case X_VAR: return *bnd[u][x->iv];
    case X_CALL:
        do_call(u, (int)x->iv, fca[x->a], fu[x->iv].nf);
        return *bnd[x->iv][FRES];
    case X_ADD: a = eval(u, x->a); b = eval(u, x->b); r.i = (int16_t)(a.i + b.i); break;
    case X_SUB: a = eval(u, x->a); b = eval(u, x->b); r.i = (int16_t)(a.i - b.i); break;
    case X_MUL: a = eval(u, x->a); b = eval(u, x->b); r.i = (int16_t)(a.i * b.i); break;
    case X_DIV: a = eval(u, x->a); r.i = (int16_t)(a.i / x->iv); break;
    case X_MOD: a = eval(u, x->a); r.i = (int16_t)(a.i % x->iv); break;
    case X_NEG: a = eval(u, x->a); r.i = (int16_t)(-a.i); break;
    case X_RC:
        a = eval(u, x->a);
        b = eval(u, x->b);
        vf = a.f + b.f;
        vf = vf * x->fv;
        vf = vf + x->k;
        r.f = vf;
        break;
    case X_AND: a = eval(u, x->a); b = eval(u, x->b); r.i = a.i && b.i; break;
    case X_OR: a = eval(u, x->a); b = eval(u, x->b); r.i = a.i || b.i; break;
    case X_NOT: a = eval(u, x->a); r.i = !a.i; break;
    case X_CMPI:
    case X_CMPR: {
        int c;

        a = eval(u, x->a);
        b = eval(u, x->b);
        if (x->op == X_CMPI) {
            c = a.i < b.i ? -1 : a.i > b.i;
        } else {
            c = a.f < b.f ? -1 : a.f > b.f;
        }
        switch (x->iv) {
        case 0: r.i = c < 0; break;
        case 1: r.i = c <= 0; break;
        case 2: r.i = c > 0; break;
        case 3: r.i = c >= 0; break;
        case 4: r.i = c == 0; break;
        default: r.i = c != 0; break;
        }
        break;
    }
    }
    return r;
}

/* Llamada con paso POR REFERENCIA: el formal es un puntero a la celda del llamador (o a un temporal si es una expresion). */
static void do_call(int u, int c, const int16_t *arg, int na)
{
    Val tmp[4], *ptr[4];
    int i;

    if (++fcalls > 2500) {
        fabort = 1;
        return;
    }
    for (i = 0; i < na; i++) {
        if (fex[arg[i]].op == X_VAR) {
            ptr[i] = bnd[u][fex[arg[i]].iv];
        } else {
            tmp[i] = eval(u, arg[i]);
            ptr[i] = &tmp[i];
        }
    }
    for (i = 0; i < na; i++) {
        bnd[c][fu[c].fv[i]] = ptr[i];
    }
    exec(c, fu[c].body);
}

static int exec(int u, St *s)
{
    for (; s != NULL; s = s->next) {
        if (++fsteps > 60000 || fabort) {
            fabort = 1;
            return 1;
        }
        switch (s->k) {
        case SA_ASSIGN: *bnd[u][s->v] = eval(u, s->e); break;
        case SA_IF:
            if (exec(u, eval(u, s->e).i ? s->a : s->b)) {
                return 1;
            }
            break;
        case SA_LOGRET:
            if (eval(u, s->e).i) {
                return 1;
            }
            break;
        case SA_DO: {
            int k;

            for (k = 1; k <= s->n; k++) {
                bnd[u][s->v]->i = k;
                if (exec(u, s->a)) {
                    return 1;
                }
            }
            bnd[u][s->v]->i = s->n + 1;
            break;
        }
        case SA_SKIP:
            if (!eval(u, s->e).i && exec(u, s->a)) {
                return 1;
            }
            break;
        case SA_CALL:
            do_call(u, s->callee, s->arg, s->na);
            break;
        case SA_FASSIGN:
            do_call(u, s->callee, s->arg, s->na);
            *bnd[u][s->v] = *bnd[s->callee][FRES];
            break;
        case SA_PRINT: {
            int i;

            for (i = 0; i < s->na; i++) {
                Val x = eval(u, s->arg[i]);
                int t = fex[s->arg[i]].t;
                char b[48];

                if (fol > 6000) {
                    fabort = 1;
                    return 1;
                }
                if (t == TI) {
                    sprintf(b, " %d", (int)x.i);
                } else if (t == TL) {
                    sprintf(b, " %c", x.i ? 'T' : 'F');
                } else {
                    uint32_t bits;
                    char r[24];

                    memcpy(&bits, &x.f, 4);
                    real_formato(bits, r);
                    sprintf(b, " %s", r);
                }
                strcpy(fo + fol, b);
                fol += (int)strlen(b);
            }
            fo[fol++] = '\n';
            fo[fol] = '\0';
            break;
        }
        }
        if (fabort) {
            return 1;
        }
    }
    return 0;
}

/* ---------- impresion como Fortran ---------- */
static void pe(char *d, int e)
{
    FEx *x = &fex[e];
    char b[32];
    static const char *opn[16] = {0, 0, 0, 0, "+", "-", "*", "/", 0, 0, 0, ".AND.", ".OR.", 0, 0, 0};

    switch (x->op) {
    case X_CI: sprintf(b, x->iv < 0 ? "(%d)" : "%d", (int)x->iv); strcat(d, b); return;
    case X_CR: sprintf(b, "%.2f", (double)x->fv); strcat(d, b); return;
    case X_CL: strcat(d, x->iv ? ".TRUE." : ".FALSE."); return;
    case X_VAR:
        if (x->iv == FRES) {
            sprintf(b, "FUN%d", curu);
            strcat(d, b);
        } else {
            strcat(d, fvn[x->iv]);
        }
        return;
    case X_CALL: {
        int i;

        sprintf(b, "FUN%d(", (int)x->iv);
        strcat(d, b);
        for (i = 0; i < fu[x->iv].nf; i++) {
            if (i > 0) {
                strcat(d, ", ");
            }
            pe(d, fca[x->a][i]);
        }
        strcat(d, ")");
        return;
    }
    case X_ADD: case X_SUB: case X_MUL: case X_AND: case X_OR:
        strcat(d, "(");
        pe(d, x->a);
        strcat(d, " ");
        strcat(d, opn[x->op]);
        strcat(d, " ");
        pe(d, x->b);
        strcat(d, ")");
        return;
    case X_DIV: strcat(d, "("); pe(d, x->a); sprintf(b, " / %d)", (int)x->iv); strcat(d, b); return;
    case X_MOD: strcat(d, "MOD("); pe(d, x->a); sprintf(b, ", %d)", (int)x->iv); strcat(d, b); return;
    case X_NEG: strcat(d, "(-"); pe(d, x->a); strcat(d, ")"); return;
    case X_NOT: strcat(d, "(.NOT. "); pe(d, x->a); strcat(d, ")"); return;
    case X_RC:
        strcat(d, "((");
        pe(d, x->a);
        strcat(d, " + ");
        pe(d, x->b);
        sprintf(b, ") * %.2f + %.2f)", (double)x->fv, (double)x->k);
        strcat(d, b);
        return;
    default:
        strcat(d, "(");
        pe(d, x->a);
        strcat(d, " ");
        strcat(d, frel[x->iv]);
        strcat(d, " ");
        pe(d, x->b);
        strcat(d, ")");
        return;
    }
}

static void vname(char *d, int u, int v)
{
    if (v == FRES) {
        sprintf(d + strlen(d), "FUN%d", u);
    } else {
        strcat(d, fvn[v]);
    }
}

static void outl(int u, const char *l)
{
    char buf[1024];
    char *q;

    strcpy(buf, l);
    q = buf;
    while (strlen(q) > 72) {
        char *sp = q + 70;
        char save;

        while (sp > q + 8 && *sp != ' ') {
            sp--;
        }
        save = *sp;
        *sp = '\0';
        strcat(ubuf[u], q);
        strcat(ubuf[u], "\n");
        *sp = save;
        memmove(sp - 5, "     +", 6); /* col. 6 = continuacion; el resto del texto sigue */
        q = sp - 5;
        if (q < buf) {
            break;
        }
    }
    strcat(ubuf[u], q);
    strcat(ubuf[u], "\n");
}

static void emit_args(char *l, const int16_t *arg, int na)
{
    int i;

    if (na > 0) {
        strcat(l, "(");
        for (i = 0; i < na; i++) {
            if (i > 0) {
                strcat(l, ", ");
            }
            pe(l, arg[i]);
        }
        strcat(l, ")");
    }
}

static void emit_list(int u, St *s, int ind)
{
    char l[1024], pad[64];
    int i;

    memset(pad, ' ', (size_t)ind);
    pad[ind] = '\0';
    for (; s != NULL; s = s->next) {
        strcpy(l, "      ");
        strcat(l, pad);
        switch (s->k) {
        case SA_ASSIGN: vname(l, u, s->v); strcat(l, " = "); pe(l, s->e); outl(u, l); break;
        case SA_IF:
            strcat(l, "IF (");
            pe(l, s->e);
            strcat(l, ") THEN");
            outl(u, l);
            emit_list(u, s->a, ind + 3);
            if (s->b != NULL) {
                sprintf(l, "      %sELSE", pad);
                outl(u, l);
                emit_list(u, s->b, ind + 3);
            }
            sprintf(l, "      %sEND IF", pad);
            outl(u, l);
            break;
        case SA_LOGRET: strcat(l, "IF ("); pe(l, s->e); strcat(l, ") RETURN"); outl(u, l); break;
        case SA_DO: {
            int lb = 10 * (++ulbl);

            sprintf(l + strlen(l), "DO %d %s = 1, %d", lb, fvn[s->v], (int)s->n);
            outl(u, l);
            emit_list(u, s->a, ind + 3);
            sprintf(l, "%5d CONTINUE", lb);
            outl(u, l);
            break;
        }
        case SA_SKIP: {
            int lb = 10 * (++ulbl);

            strcat(l, "IF (");
            pe(l, s->e);
            sprintf(l + strlen(l), ") GOTO %d", lb);
            outl(u, l);
            emit_list(u, s->a, ind + 3);
            sprintf(l, "%5d CONTINUE", lb);
            outl(u, l);
            break;
        }
        case SA_CALL:
            sprintf(l + strlen(l), "CALL SUB%d", (int)s->callee);
            emit_args(l, s->arg, s->na);
            outl(u, l);
            break;
        case SA_FASSIGN:
            vname(l, u, s->v);
            sprintf(l + strlen(l), " = FUN%d", (int)s->callee);
            emit_args(l, s->arg, s->na);
            outl(u, l);
            break;
        case SA_PRINT:
            strcat(l, "PRINT *");
            for (i = 0; i < s->na; i++) {
                strcat(l, ", ");
                pe(l, s->arg[i]);
            }
            outl(u, l);
            break;
        }
    }
}

/* Genera la semilla `seed`; 0 si se descarta (demasiado grande o larga de ejecutar). */
static int gen_programa(uint32_t seed, char *prog, char *esp)
{
    int u, v, i, ord[FNU], j, t;

    fs = seed * 2654435761u + 12345u;
    nfex = 0;
    nfca = 0;
    nst = 0;
    fabort = 0;
    fsteps = 0;
    fcalls = 0;
    fol = 0;
    fo[0] = '\0';
    mk(X_CI, TI, 0, 0, 0, 0, 0);
    fnu = 1 + (int)(fr() % 6);
    if (fnu == 1 && (fr() & 1)) {
        fnu = 2;
    }
    for (u = fnu - 1; u >= 0; u--) {
        gen_unit(u);
    }
    if (fabort) {
        return 0;
    }
    memset(cel, 0, sizeof(cel));
    for (u = 0; u < FNU; u++) {
        for (v = 0; v < FNV; v++) {
            bnd[u][v] = &cel[u][v];
        }
    }
    exec(0, fu[0].body);
    if (fabort) {
        return 0;
    }
    for (u = 0; u < fnu; u++) {
        char l[512];

        ubuf[u][0] = '\0';
        ulbl = 0;
        curu = u;
        if (u == 0) {
            if (fr() & 1) {
                outl(u, "      PROGRAM MAIN");
            }
        } else {
            if (fu[u].isfn) {
                sprintf(l, "      %s FUNCTION FUN%d", ftn[fu[u].rt], u);
            } else {
                sprintf(l, "      SUBROUTINE SUB%d", u);
            }
            if (fu[u].nf > 0 || fu[u].isfn) {
                strcat(l, "(");
                for (i = 0; i < fu[u].nf; i++) {
                    sprintf(l + strlen(l), "%s%s", i ? ", " : "", fvn[fu[u].fv[i]]);
                }
                strcat(l, ")");
            }
            outl(u, l);
        }
        outl(u, "      INTEGER A1, A2, A3, V(3), K1, K2");
        outl(u, "      REAL B1, B2");
        outl(u, "      LOGICAL C1, C2");
        for (t = 0; t < 3; t++) {            /* las funciones que esta unidad puede llamar, con su tipo */
            char l[256];
            int n = 0;

            sprintf(l, "      %s ", ftn[t]);
            for (j = u + 1; j < fnu; j++) {
                if (fu[j].isfn && fu[j].rt == t) {
                    sprintf(l + strlen(l), "%sFUN%d", n++ ? ", " : "", j);
                }
            }
            if (n > 0) {
                outl(u, l);
            }
        }
        emit_list(u, fu[u].body, 0);
        outl(u, "      END");
    }
    for (u = 0; u < fnu; u++) {
        ord[u] = u;
    }
    for (u = fnu - 1; u > 0; u--) {
        j = (int)(fr() % (uint32_t)(u + 1));
        t = ord[u];
        ord[u] = ord[j];
        ord[j] = t;
    }
    prog[0] = '\0';
    for (u = 0; u < fnu; u++) {
        strcat(prog, ubuf[ord[u]]);
    }
    strcpy(esp, fo);
    {
        int f = 0, x = 0, a = 0;

        for (u = 1; u < fnu; u++) {
            f |= fu[u].isfn;
        }
        for (i = 1; i < nfex; i++) {
            x |= fex[i].op == X_CALL;
        }
        for (i = 0; i < nst; i++) {
            a |= stp[i].k == SA_FASSIGN;
        }
        est_fn += f;
        est_xcall += x;
        est_fasg += a;
    }
    return 1;
}

/* S3a: argumentos CHARACTER escalares (por copia de entrada y de salida con la longitud del formal) */
static void test_args_char(void)
{
    SALIDA("      CHARACTER*5 A\n"
          "      A = 'abcde'\n"
          "      CALL S(A)\n"
          "      PRINT *, A\n"
          "      END\n"
          "      SUBROUTINE S(X)\n"
          "      CHARACTER*5 X\n"
          "      X(2:3) = 'ZZ'\n"
          "      END\n",
           " aZZde\n");
    SALIDA("      CHARACTER*5 A\n"
          "      A = 'abcde'\n"
          "      CALL S(A)\n"
          "      PRINT *, A\n"
          "      END\n"
          "      SUBROUTINE S(X)\n"
          "      CHARACTER*2 X\n"
          "      X = 'q'\n"
          "      END\n",
           " q cde\n");
    SALIDA("      CHARACTER*2 A, B\n"
          "      A = 'xy'\n"
          "      B = 'pq'\n"
          "      CALL SW(A, B)\n"
          "      PRINT *, A, B\n"
          "      END\n"
          "      SUBROUTINE SW(P, Q)\n"
          "      CHARACTER*2 P, Q, T\n"
          "      T = P\n"
          "      P = Q\n"
          "      Q = T\n"
          "      END\n",
           " pq xy\n");
    SALIDA("      CHARACTER*4 T(3)\n"
          "      T(1) = 'aaaa'\n"
          "      T(2) = 'bbbb'\n"
          "      T(3) = 'cccc'\n"
          "      K = 2\n"
          "      CALL F(T(K))\n"
          "      CALL F(T(K+1))\n"
          "      PRINT *, T(1), T(2), T(3)\n"
          "      END\n"
          "      SUBROUTINE F(S)\n"
          "      CHARACTER*2 S\n"
          "      S(2:2) = '!'\n"
          "      END\n",
           " aaaa b!bb c!cc\n");
    SALIDA("      CHARACTER*3 A\n"
          "      A = 'abc'\n"
          "      CALL P('lit')\n"
          "      CALL P(A // 'XY')\n"
          "      CALL P('a' // 'b' // 'c')\n"
          "      CALL P((A))\n"
          "      END\n"
          "      SUBROUTINE P(S)\n"
          "      CHARACTER*3 S\n"
          "      PRINT *, '[', S, ']'\n"
          "      END\n",
           " [ lit ]\n"
          " [ abc ]\n"
          " [ abc ]\n"
          " [ abc ]\n");
    SALIDA("      CHARACTER*8 R\n"
          "      CHARACTER*8 JOIN\n"
          "      R = JOIN('ab', 'cd')\n"
          "      PRINT *, R\n"
          "      END\n"
          "      CHARACTER*8 FUNCTION JOIN(A, B)\n"
          "      CHARACTER*2 A, B\n"
          "      JOIN = A // '-' // B\n"
          "      END\n",
           " ab-cd   \n");
    SALIDA("      CHARACTER*4 A\n"
          "      A = 'wxyz'\n"
          "      CALL OUTER(A)\n"
          "      PRINT *, A\n"
          "      END\n"
          "      SUBROUTINE OUTER(S)\n"
          "      CHARACTER*4 S\n"
          "      CALL INNER(S)\n"
          "      S(1:1) = '<'\n"
          "      END\n"
          "      SUBROUTINE INNER(T)\n"
          "      CHARACTER*4 T\n"
          "      T(4:4) = '>'\n"
          "      END\n",
           " <xy>\n");
    SALIDA("      CHARACTER*3 A, N\n"
          "      INTEGER I, K\n"
          "      A = 'abc'\n"
          "      I = 7\n"
          "      CALL M(A, I, 2.5, N)\n"
          "      PRINT *, A, I, N\n"
          "      END\n"
          "      SUBROUTINE M(S, J, X, R)\n"
          "      CHARACTER*3 S, R\n"
          "      INTEGER J\n"
          "      REAL X\n"
          "      J = J + INT(X)\n"
          "      S(1:1) = 'Z'\n"
          "      R = S\n"
          "      END\n",
           " Zbc 9 Zbc\n");
    SALIDA("      CHARACTER*6 L\n"
          "      LOGICAL EQ\n"
          "      L = 'hello'\n"
          "      IF (EQ(L, 'hello')) PRINT *, 'si'\n"
          "      IF (.NOT. EQ(L, 'bye  ')) PRINT *, 'no'\n"
          "      END\n"
          "      LOGICAL FUNCTION EQ(A, B)\n"
          "      CHARACTER*5 A, B\n"
          "      EQ = A .EQ. B\n"
          "      END\n",
           " si\n"
          " no\n");
    SALIDA("      CHARACTER*5 A\n"
          "      INTEGER N\n"
          "      A = 'abcde'\n"
          "      N = LN(A)\n"
          "      PRINT *, N\n"
          "      N = LN('xyz')\n"
          "      PRINT *, N\n"
          "      END\n"
          "      INTEGER FUNCTION LN(S)\n"
          "      CHARACTER*3 S\n"
          "      LN = LEN(S)\n"
          "      END\n",
           " 3\n"
          " 3\n");
    SALIDA("      CHARACTER C\n"
          "      C = 'k'\n"
          "      CALL T(C)\n"
          "      PRINT *, C\n"
          "      END\n"
          "      SUBROUTINE T(Z)\n"
          "      CHARACTER Z\n"
          "      Z = 'abc'\n"
          "      END\n",
           " a\n");
    SALIDA("      CHARACTER*4 W(3)\n"
          "      INTEGER I\n"
          "      DO 10 I = 1, 3\n"
          "         W(I) = 'abcd'\n"
          "         CALL Z(W(I))\n"
          "   10 CONTINUE\n"
          "      PRINT *, W(1), W(2), W(3)\n"
          "      END\n"
          "      SUBROUTINE Z(S)\n"
          "      CHARACTER*4 S\n"
          "      S(1:1) = 'x'\n"
          "      S(4:4) = 'y'\n"
          "      END\n",
           " xbcy xbcy xbcy\n");
    SALIDA("      CHARACTER*3 UP, R\n"
          "      R = UP(UP('abc'))\n"
          "      PRINT *, R\n"
          "      END\n"
          "      CHARACTER*3 FUNCTION UP(S)\n"
          "      CHARACTER*3 S\n"
          "      UP = S\n"
          "      UP(1:1) = '#'\n"
          "      END\n",
           " #bc\n");
    SALIDA("      CHARACTER*3 A\n"
          "      A = 'abc'\n"
          "      CALL P(A, A)\n"
          "      PRINT *, A\n"
          "      END\n"
          "      SUBROUTINE P(X, Y)\n"
          "      CHARACTER*3 X, Y\n"
          "      PRINT *, X, Y\n"
          "      END\n",
           " abc abc\n"
          " abc\n");
    SALIDA("      CHARACTER*2 A, B\n"
          "      A = 'ab'\n"
          "      B = 'zz'\n"
          "      CALL CMP(A, B)\n"
          "      CALL CMP(B, A)\n"
          "      CALL CMP(A, 'ab')\n"
          "      END\n"
          "      SUBROUTINE CMP(X, Y)\n"
          "      CHARACTER*2 X, Y\n"
          "      IF (X .LT. Y) PRINT *, 'lt'\n"
          "      IF (X .EQ. Y) PRINT *, 'eq'\n"
          "      IF (X .GT. Y) PRINT *, 'gt'\n"
          "      END\n",
           " lt\n"
          " gt\n"
          " eq\n");
    SALIDA("      CHARACTER*10 A, GR\n"
          "      A = GR('bob')\n"
          "      PRINT *, A\n"
          "      END\n"
          "      CHARACTER*10 FUNCTION GR(N)\n"
          "      CHARACTER*3 N\n"
          "      GR = 'hi ' // N\n"
          "      END\n",
           " hi bob    \n");
    SALIDA("      CHARACTER*2 S\n"
          "      S = 'ab'\n"
          "      CALL A1(S)\n"
          "      PRINT *, S\n"
          "      END\n"
          "      SUBROUTINE A1(P)\n"
          "      CHARACTER*2 P\n"
          "      CALL A2(P, 3)\n"
          "      END\n"
          "      SUBROUTINE A2(Q, N)\n"
          "      CHARACTER*2 Q\n"
          "      INTEGER N, I\n"
          "      DO 5 I = 1, N\n"
          "         Q(1:1) = Q(2:2)\n"
          "         IF (I .EQ. 2) Q(2:2) = '#'\n"
          "    5 CONTINUE\n"
          "      END\n",
           " ##\n");
    /* asignacion a subcadena */
    SALIDA("      CHARACTER*8 S\n"
          "      CHARACTER*4 A(3)\n"
          "      INTEGER I\n"
          "      S = 'ABCDEFGH'\n"
          "      S(2:4) = 'xy'\n"
          "      PRINT *, S\n"
          "      S(7:8) = 'LONGER'\n"
          "      PRINT *, S\n"
          "      S(:3) = '123'\n"
          "      S(6:) = 'zz'\n"
          "      PRINT *, S\n"
          "      I = 3\n"
          "      S(I:I+1) = '++'\n"
          "      PRINT *, S\n"
          "      A(2) = 'WXYZ'\n"
          "      A(2)(2:3) = '--'\n"
          "      I = 2\n"
          "      A(I)(4:) = '!'\n"
          "      PRINT *, A(2)\n"
          "      END\n",
           " Axy EFGH\n"
          " Axy EFLO\n"
          " 123 Ezz \n"
          " 12++Ezz \n"
          " W--!\n");
}

static void test_args_char_errores(void)
{
    ERROR("      CALL S(1)\n      END\n      SUBROUTINE S(C)\n      CHARACTER C\n      END\n", 1, 1, 12, U(UNI_TIPO_ARG));
    ERROR("      CALL S('ab')\n      END\n      SUBROUTINE S(I)\n      INTEGER I\n      END\n", 1, 1, 12, U(UNI_TIPO_ARG));
    ERROR("      CHARACTER*2 A\n      CALL S(A)\n      END\n      SUBROUTINE S(I)\n      END\n", 1, 2, 12, U(UNI_TIPO_ARG));
    ERROR("      CALL S('ab')\n      END\n      SUBROUTINE S(X)\n      CHARACTER*3 X\n      END\n", 1, 1, 12, U(UNI_LONG_ARG));
    ERROR("      CHARACTER*4 A\n      CALL S(A)\n      END\n      SUBROUTINE S(X)\n      CHARACTER*5 X\n      END\n",
          1, 2, 12, U(UNI_LONG_ARG));
    ERROR("      CALL S('a' // 'b')\n      END\n      SUBROUTINE S(X)\n      CHARACTER*3 X\n      END\n", 1, 1, 12, U(UNI_LONG_ARG));
    ERROR("      CHARACTER*2 T(2)\n      CALL S(T(1))\n      END\n      SUBROUTINE S(X)\n      CHARACTER*3 X\n      END\n",
          1, 2, 12, U(UNI_LONG_ARG));
    ERROR("      INTEGER K, LN\n      K = LN('xy')\n      END\n      INTEGER FUNCTION LN(S)\n      CHARACTER*3 S\n      END\n",
          1, 2, 11, U(UNI_LONG_ARG));
}

#include "tests_arrays.h"

static void test_fuzz(int n)
{
    static char prog[65536], esp[8192];
    int ok = 0, desc = 0, malos = 0;
    uint32_t seed;

    for (seed = 1; ok < n && seed < 200000u && malos < 3; seed++) {
        if (!gen_programa(seed, prog, esp)) {
            desc++;
            continue;
        }
        if (!salida(__LINE__, prog, esp)) {
            printf("   (semilla %u)\n", (unsigned)seed);
            malos++;
        }
        ok++;
    }
    printf("fuzzer: %d programas, %d descartados (con funciones %d, con F() en expresiones %d, con V = F(...) que escribe %d)\n", ok, desc, est_fn, est_xcall, est_fasg);
}

int main(void)
{
    test_orden();
    test_argumentos();
    test_unidades_independientes();
    test_flujo();
    test_errores();
    test_funciones();
    test_funciones_errores();
    test_funciones_char();
    test_funciones_char_errores();
    test_funciones_sentencia();
    test_funciones_sentencia_errores();
    test_args_char();
    test_args_char_errores();
    test_arrays();
    test_limites();
    test_fuzz(FUZZ_N);
    printf("%d pruebas, %d fallos\n", pruebas, fallos);
    return fallos != 0;
}
