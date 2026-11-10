/* test_cap07.c - Pruebas del backend Z80 (back.c, z80.c) con el emulador z80emu.
 *
 * La referencia es tacvm: el mismo programa se ejecuta como TAC (interprete) y como
 * Z80 (emulador) y la salida debe ser identica. Asi cualquier discrepancia es un
 * fallo de la traduccion TAC -> Z80, no del frontal.
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
#include "exprgen.h"
#include "nex.h"

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

#define ORG 0x8000

/* Con -DMF_SIN_TEXTO (el Next) los mensajes van vacios: solo se comprueba su contenido si existen. */
#ifdef MF_SIN_TEXTO
#define TEXTO(m, sub) 1
#else
#define TEXTO(m, sub) (strstr((m), (sub)) != NULL)
#endif

static uint8_t mem[65536];
static char listado[262144];

static void sal(const char *l)
{
    strcat(listado, l);
    strcat(listado, "\n");
}

/* Compila a TAC. Devuelve el numero de errores. */
static int a_tac(const char *prog)
{
    banco_iniciar();
    texto_poner(prog);
    return (int)mf_compilar();
}

/* Genera Z80, lo ejecuta y deja la salida en out. Devuelve el estado (0 = bien). */
static int modo_paginas;       /* 1: la imagen se guarda en las paginas de datos y se recompone */

static int ejecutar_z80(char *out, size_t n, BackRes *br, Z80EmuRes *er)
{
    memset(mem, 0, sizeof(mem));
    listado[0] = '\0';
    if (modo_paginas) {
        back_generar(NULL, 16384, ORG, sal, br);
        if (br->err == 0) {
            z_imagen_leer(0, mem + ORG, (uint16_t)(br->fin_img - ORG));
        }
    } else {
        back_generar(mem + ORG, (uint16_t)(0x10000 - ORG - 0x1000), ORG, sal, br);
    }
    if (br->err != 0) {
        out[0] = '\0';
        return br->err;
    }
    z80emu_ejecutar(mem, br->entry, 50000000u, out, n, er);
    return er->estado;
}

/* El programa debe dar la misma salida en TAC y en Z80. */
static int igual(int linea_test, const char *prog)
{
    char o1[4096], o2[4096], msg[96];
    TacVmRes tr;
    BackRes br;
    Z80EmuRes er;
    int e;

    pruebas++;
    if (a_tac(prog) != 0) {
        printf("FALLO (linea %d): no compila\n", linea_test);
        fallos++;
        return 0;
    }
    if (tacvm_validar(msg, sizeof(msg)) != 0) {
        printf("FALLO (linea %d): TAC invalido: %s\n", linea_test, msg);
        fallos++;
        return 0;
    }
    tacvm_ejecutar(o1, sizeof(o1), 5000000u, &tr);
    if (tr.estado == TACVM_STOP && tr.msg[0] != '\0') {      /* el STOP del programa generado imprime "STOP <a>" */
        strcat(o1, "STOP ");
        strcat(o1, tr.msg);
        strcat(o1, "\n");
    }
    e = ejecutar_z80(o2, sizeof(o2), &br, &er);
    if (e != 0) {
        printf("FALLO (linea %d): backend/emulador estado %d (instr TAC %u, pc %04X op %02X)\n%s\n",
               linea_test, e, (unsigned)br.err_inst, (unsigned)er.pc, (unsigned)er.op, prog);
        fallos++;
        return 0;
    }
    if (strcmp(o1, o2) != 0) {
        printf("FALLO (linea %d): salida distinta\n--- TAC\n%s--- Z80\n%s--- programa\n%s", linea_test, o1, o2, prog);
        fallos++;
        return 0;
    }
    return 1;
}

#define IGUAL(p) igual(__LINE__, p)

static void test_programas(void)
{
    IGUAL("      INTEGER I, S\n      S = 0\n      DO 10 I = 1, 10\n         S = S + I\n   10 CONTINUE\n      PRINT *, S\n      END\n");
    IGUAL("      N = 1\n      DO 10 I = 1, 7\n   10 N = N * I\n      PRINT *, N\n      END\n");
    IGUAL("      DO 10 I = 1, 3\n   10 CONTINUE\n      PRINT *, I\n      END\n");
    IGUAL("      K = 0\n      DO 10 I = 10, 1\n   10 K = K + 1\n      PRINT *, K, I\n      END\n");
    IGUAL("      K = 0\n      DO 10 I = 1, 10, 2\n   10 K = K + I\n      PRINT *, K, I\n      END\n");
    IGUAL("      K = 0\n      DO 10 I = 10, 1, -3\n   10 K = K + I\n      PRINT *, K, I\n      END\n");
    IGUAL("      N = 3\n      K = 0\n      DO 10 I = 1, N\n      N = N + 1\n   10 K = K + 1\n      PRINT *, K, N\n      END\n");
    IGUAL("      N = 0\n      DO 10 I = 1, 3\n      DO 10 J = 1, 3\n   10 IF (J .NE. 2) N = N + I * J\n      PRINT *, N\n      END\n");
    IGUAL("      N = 0\n      DO 20 I = 1, 4\n      DO 10 J = I, 4\n   10 N = N + 1\n   20 CONTINUE\n      PRINT *, N\n      END\n");
    IGUAL("      DO 10 I = 1, 3\n      PRINT *, I, I * I, I ** 3\n   10 CONTINUE\n      END\n");

    /* el ejemplo del repositorio */
    IGUAL("      PROGRAM FIBO\n      INTEGER F(23), I\n      F(1) = 1\n      F(2) = 1\n      DO 10 I = 3, 23\n"
          "         F(I) = F(I - 1) + F(I - 2)\n   10 CONTINUE\n      PRINT *, 'FIBONACCI'\n      DO 20 I = 1, 23\n"
          "         PRINT *, I, F(I)\n   20 CONTINUE\n      END\n");

    IGUAL("      LOGICAL P(30)\n      DO 10 I = 2, 30\n   10 P(I) = .TRUE.\n      DO 30 I = 2, 5\n"
          "      IF (.NOT. P(I)) GOTO 30\n      DO 20 J = 2 * I, 30, I\n   20 P(J) = .FALSE.\n   30 CONTINUE\n"
          "      DO 40 I = 2, 30\n   40 IF (P(I)) PRINT *, I\n      END\n");
    IGUAL("      INTEGER M(2:4, 0:2)\n      DO 20 I = 2, 4\n      DO 10 J = 0, 2\n   10 M(I, J) = 10 * I + J\n   20 CONTINUE\n"
          "      PRINT *, M(2, 0), M(4, 0), M(2, 2), M(3, 1), M(4, 2)\n      END\n");
    IGUAL("      DO 10 N = 1, 7, 3\n      IF (N .GT. 5) THEN\n        PRINT *, 'BIG'\n      ELSE IF (N .GT. 2) THEN\n"
          "        PRINT *, 'MID'\n      ELSE\n        PRINT *, 'SMALL'\n      END IF\n   10 CONTINUE\n      END\n");
    IGUAL("      DO 10 N = 0, 4\n      IF (N .EQ. 0) THEN\n        PRINT *, 'ZERO'\n      ELSE IF (N .EQ. 1) THEN\n"
          "        PRINT *, 'ONE'\n      ELSE IF (N .EQ. 2) THEN\n        PRINT *, 'TWO'\n      ELSE\n"
          "        IF (N .EQ. 3) THEN\n          PRINT *, 'THREE'\n        ELSE\n          PRINT *, 'MANY'\n"
          "        END IF\n      END IF\n   10 CONTINUE\n      END\n");
    IGUAL("      N = 0\n   10 N = N + 1\n      IF (N .LT. 5) GOTO 10\n      PRINT *, N\n      END\n");
    IGUAL("      DO 5 N = 0, 4\n      GOTO (10, 20, 30), N\n      PRINT *, 'FUERA'\n      GOTO 5\n"
          "   10 PRINT *, 'UNO'\n      GOTO 5\n   20 PRINT *, 'DOS'\n      GOTO 5\n   30 PRINT *, 'TRES'\n    5 CONTINUE\n      END\n");
    IGUAL("      DO 5 N = -1, 1\n      IF (N) 10, 20, 30\n   10 PRINT *, 'NEG'\n      GOTO 5\n"
          "   20 PRINT *, 'CERO'\n      GOTO 5\n   30 PRINT *, 'POS'\n    5 CONTINUE\n      END\n");

    /* aritmetica de 16 bits */
    IGUAL("      I = 32767\n      J = I + 1\n      K = 200 * 200\n      PRINT *, J, K\n      END\n");
    IGUAL("      PRINT *, -7 / 2, 7 / (-2), -7 / (-2), 7 / 2\n      END\n");
    IGUAL("      PRINT *, 2 ** 10, (-2) ** 3, 2 ** (-1), (-1) ** (-3)\n      PRINT *, 1 ** (-5), 0 ** 0, 3 ** 0\n      END\n");
    IGUAL("      PRINT *, 2 + 3 * 4, (2 + 3) * 4, 2 ** 3 ** 2\n      PRINT *, -2 ** 2, 10 - 4 - 3, 100 / 10 / 5\n      END\n");
    IGUAL("      PRINT *, 0, 1, -1, 9, 10, 99, 100\n      PRINT *, 999, 1000, 9999, 10000, 32767, -32768\n      END\n");
    IGUAL("      PRINT *, 30000, -30000, 12345, -12345, 7000\n      END\n");

    /* logicos y relacionales */
    IGUAL("      LOGICAL A, B\n      A = .TRUE.\n      B = .FALSE.\n"
          "      PRINT *, A .AND. B, A .OR. B, .NOT. A, A .EQV. B, A .NEQV. B\n"
          "      PRINT *, A .OR. B .AND. B, .NOT. B .AND. A\n      PRINT *, A .EQV. B .OR. A\n      END\n");
    IGUAL("      PRINT *, 1 .LT. 2, 1 .LE. 1, 2 .GT. 3\n      PRINT *, 3 .GE. 4, 5 .EQ. 5, 5 .NE. 5\n      END\n");
    IGUAL("      PRINT *, -3 .LT. 2, 2 .LT. (-3), -32768 .LT. 32767\n      PRINT *, 32767 .GT. (-32768), -1 .GE. (-1)\n      END\n");

    /* el acarreo que deja una operacion anterior no debe influir en la comparacion */
    IGUAL("      I = -1\n      J = -1\n      PRINT *, (I + J) .EQ. (-3), (I + J) .NE. (-3), (I + J) .EQ. (-2)\n      END\n");
    IGUAL("      I = -1\n      J = -1\n      K = 5\n      IF ((I + J) .EQ. (-3)) K = 7\n      PRINT *, K\n      END\n");

    /* potencias con exponente negativo: solo +-1 dan algo distinto de cero */
    IGUAL("      PRINT *, (-2) ** (-1), (-3) ** (-2), (-256) ** (-1)\n      PRINT *, 256 ** (-1), 2 ** (-2)\n      PRINT *, (-1) ** (-2), (-1) ** (-1), 1 ** (-3)\n      END\n");

    /* intrinsecas enteras: TAC y Z80 deben dar lo mismo (incluidos los extremos de 16 bits) */
    IGUAL("      I = -32768\n      J = 32767\n      PRINT *, ABS(I), ABS(J), IABS(-5)\n"
          "      PRINT *, MAX(I, J), MIN(I, J), DIM(J, I), DIM(I, J)\n      END\n");
    IGUAL("      DO 10 I = -7, 7, 3\n      DO 10 J = -3, 3, 2\n      IF (J .NE. 0) PRINT *, I, J, MOD(I, J)\n"
          "   10 CONTINUE\n      END\n");
    IGUAL("      DO 10 I = -3, 3\n      DO 10 J = -3, 3\n      PRINT *, SIGN(I, J), ISIGN(J, I), DIM(I, J)\n"
          "   10 CONTINUE\n      END\n");
    IGUAL("      N = 3\n      M = -4\n      K = 0\n      PRINT *, MAX(N, M, K), MIN(N, M, K)\n"
          "      PRINT *, MAX0(K, K, K, M, N), MIN0(N, M, K, 9, -9)\n"
          "      PRINT *, ABS(MAX(M, K - 1)), MOD(ABS(M) * 7, MAX(N, 2))\n      END\n");
    IGUAL("      CHARACTER*5 S\n      CHARACTER*2 T(3)\n      PRINT *, LEN(S), LEN(T(1)), LEN('ABC' // S)\n      PRINT *, LEN(S) + LEN(T(2))\n      END\n");
    /* STOP, PRINT vacio, constante de texto */
    IGUAL("      PRINT *, 'A'\n      STOP 7\n      PRINT *, 'B'\n      END\n");
    IGUAL("      PRINT *\n      PRINT 'HOLA'\n      END\n");
    IGUAL("      PRINT *, 'A'\n      STOP 'FIN'\n      PRINT *, 'B'\n      END\n");
    IGUAL("      N = 3\n      IF (N .GT. 2) STOP 42\n      PRINT *, 'NO'\n      END\n");
    IGUAL("      N = 3\n      IF (N .GT. 5) STOP 7\n      PRINT *, 'SI'\n      STOP\n      END\n");
}

/* CHARACTER aun no se traduce: debe decirlo, no generar basura. */
static void test_no_soportado(void)
{
    BackRes br;

    COMPROBAR(a_tac("      I = 1\n      END\n") == 0);
    back_generar(mem + ORG, 4, ORG, NULL, &br);           /* no cabe */
    COMPROBAR(br.err == BACK_NO_CABE);
}

/* El listado .asm es coherente con los bytes: cuenta de lineas y simbolos. */
static void test_listado(void)
{
    char out[256];
    BackRes br;
    Z80EmuRes er;

    COMPROBAR(a_tac("      I = 5\n      PRINT *, I\n      END\n") == 0);
    COMPROBAR(ejecutar_z80(out, sizeof(out), &br, &er) == 0);
#ifndef MF_SIN_LISTADO
    COMPROBAR(strstr(listado, "ORG $8000") != NULL);
    COMPROBAR(strstr(listado, "LD HL,5") != NULL);
    COMPROBAR(strstr(listado, "LD (V_I),HL") != NULL);
    COMPROBAR(strstr(listado, "CALL rt_puti") != NULL);
    COMPROBAR(strstr(listado, "rt_puti:") != NULL);
    COMPROBAR(strstr(listado, "rt_mul:") == NULL);            /* solo se emite lo que se usa */
    COMPROBAR(strstr(listado, "V_I EQU") != NULL);
#endif
    COMPROBAR(strcmp(out, " 5\n") == 0);
}

/* Diferencial aleatorio de expresiones: Z80 vs referencia en C. */
static void test_diferencial(void)
{
    static char prog[16384];
    char t[4096], out[256], esperado[64];
    int it, k, largo, hechas = 0;

    for (it = 0; it < 1200; it++) {
        Ex *e;
        int logico = (it & 1);
        BackRes br;
        Z80EmuRes er;

        n_ex = 0;
        for (k = 0; k < NV; k++) {
            if (k < 2) valores[k] = (int16_t)(rng() % 5);
            else if (k < 4) valores[k] = (int16_t)((int32_t)(rng() % 65536) - 32768);
            else valores[k] = (int16_t)((int32_t)(rng() % 2000) - 1000 + ((rng() % 2) ? 1 : 0));
            if (k >= 4 && valores[k] == 0) valores[k] = 7;
        }
        e = logico ? gen_log(1 + (int)(rng() % 3)) : gen_ent(1 + (int)(rng() % 4));
        texto(e, t);
        largo = (int)strlen(t);
        if (largo > 60 * 20) continue;
        prog[0] = '\0';
        strcat(prog, "      INTEGER A, B, C, D, E, F, G, H\n      LOGICAL L\n");
        for (k = 0; k < NV; k++) {
            char l[64];

            sprintf(l, "      %c = %d\n", 'A' + k, (int)valores[k]);
            strcat(prog, l);
        }
        strcat(prog, logico ? "      L = " : "      C = ");
        {
            int pos = 0, primera = 1;

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
        if (a_tac(prog) != 0) {
            continue;                                   /* expresion demasiado grande para el AST */
        }
        pruebas++;
        if (ejecutar_z80(out, sizeof(out), &br, &er) != 0) {
            printf("FALLO diferencial Z80: estado %d\n%s", br.err != 0 ? br.err : er.estado, prog);
            fallos++;
            return;
        }
        if (logico) sprintf(esperado, " %c\n", evalua(e) ? 'T' : 'F');
        else sprintf(esperado, " %d\n", (int)(int16_t)evalua(e));
        if (strcmp(out, esperado) != 0) {
            printf("FALLO diferencial Z80 (%d): esperado %s obtenido %s\n%s", it, esperado, out, prog);
            fallos++;
            return;
        }
        hechas++;
    }
    COMPROBAR(hechas > 800);
    printf("   (diferencial Z80: %d expresiones comprobadas)\n", hechas);
}

/* CHARACTER (etapa B): misma salida en TAC y en Z80 */
static void test_caracteres(void)
{
    IGUAL("      CHARACTER*5 A\n      CHARACTER*3 B\n      A = 'AB'\n      B = 'WXYZ'\n      PRINT *, A, B\n      END\n");
    IGUAL("      CHARACTER*8 A\n      A = 'HI' // 'X'\n      IF (A .EQ. 'HIX') PRINT *, 'OK'\n      PRINT *, A(2:3), A(:2), A(8:)\n      END\n");
    IGUAL("      CHARACTER*4 A, B\n      A = 'ABC'\n      B = 'ABD'\n      PRINT *, A .LT. B, A .EQ. 'ABC', A .NE. 'ABC ', A .GT. 'AB'\n      END\n");
    IGUAL("      CHARACTER*3 C(2)\n      LOGICAL L\n      C(2) = 'AB'\n      C(1) = C(2) // 'XYZ'\n      L = C(1) .GT. C(2)\n      PRINT *, C(1), C(2), L\n      PRINT *, 'A''B', C(2)(2:2)\n      END\n");
    IGUAL("      CHARACTER*6 S\n      S = 'ABCDEF'\n      DO 10 I = 1, 6\n   10 PRINT *, S(I:I)\n      END\n");
    IGUAL("      CHARACTER*4 S\n      S = 'ABCD'\n      I = 2\n      PRINT *, S(I:I+1), S(I+1:I)\n      END\n");
    /* comparaciones con relleno de blancos en ambos sentidos y en los seis operadores */
    IGUAL("      CHARACTER*4 A, B\n      A = 'AB'\n      B = 'AB  '\n      PRINT *, A.EQ.B, A.NE.B, A.LT.B, A.LE.B, A.GT.B, A.GE.B\n"
          "      PRINT *, A.EQ.'AB', A.LT.'ABC', A.GT.'AB!'\n      PRINT *, 'B'.GT.A, ' '.LT.A, A.LT.'A'\n      END\n");
    IGUAL("      CHARACTER*3 A, B\n      A = 'ZZZ'\n      B = 'ZZA'\n      IF (A .GT. B) PRINT *, 'GT'\n      IF (B .LT. A) PRINT *, 'LT'\n"
          "      IF (A(1:2) .EQ. B(1:2)) PRINT *, 'EQ'\n      END\n");
    /* concatenacion triple, truncada, con variables, y subcadenas de temporales */
    IGUAL("      CHARACTER*10 A\n      CHARACTER*3 B, C\n      B = 'ABC'\n      C = 'XY'\n      A = B // C // B // 'QQQQ'\n      PRINT *, A\n      A = B // C\n      PRINT *, A, B//C\n      END\n");
    IGUAL("      CHARACTER*5 A\n      CHARACTER*2 B\n      B = 'AB'\n      A = B // B // B\n      PRINT *, A\n      B = A // 'ZZ'\n      PRINT *, B\n      END\n");
    IGUAL("      CHARACTER*8 A\n      A = 'ABCDEFGH'\n      PRINT *, A(3:5), A(1:1), A(8:8), A(2:1), A(1:8)\n      END\n");
    /* arrays de cadenas, 2-D, con indices calculados */
    IGUAL("      CHARACTER*4 M(2,3)\n      DO 10 I = 1, 2\n      DO 10 J = 1, 3\n   10 M(I,J) = 'R' // 'C'\n      M(2,3) = 'LAST!'\n      M(1,1) = M(2,3)(2:3)\n      PRINT *, M(1,1), M(2,3), M(1,2)\n      END\n");
    IGUAL("      CHARACTER*2 T(5)\n      CHARACTER*8 R\n      T(1) = 'A'\n      T(2) = 'B'\n      T(3) = 'C'\n      R = T(1) // T(2) // T(3)\n      PRINT *, R\n      DO 20 K = 1, 3\n   20 T(K+1) = T(K)\n      PRINT *, T(1), T(2), T(3), T(4)\n      END\n");
    /* dos arrays y un escalar a la vez: no se pisan entre si */
    IGUAL("      CHARACTER*3 C(2), D(2)\n      CHARACTER*3 Z\n      C(2) = 'AB'\n      D(2) = 'XY'\n      Z = 'Q'\n      C(1) = 'MN'\n"
          "      PRINT *, C(2), D(2), Z, C(1)\n      END\n");
    /* asignaciones donde origen y destino se solapan o se repiten */
    IGUAL("      CHARACTER*6 A\n      A = 'ABCDEF'\n      A = A\n      PRINT *, A\n      A = A(1:3)\n      PRINT *, A\n      END\n");
    /* cadenas de longitud 1 y 255 */
    IGUAL("      CHARACTER*1 A\n      A = 'XYZ'\n      PRINT *, A, A // A\n      END\n");
    IGUAL("      CHARACTER*200 A\n      CHARACTER*100 B\n      B = 'abcdefghij'\n      A = B // B // 'tail'\n      PRINT *, A(1:12), A(101:110), A(195:200)\n      END\n");
    /* la primera acaba antes que la segunda con caracteres justo por encima y por debajo del blanco */
    IGUAL("      CHARACTER*2 A\n      A = 'AB'\n      PRINT *, A.LT.'AB!', A.EQ.'AB   ', A.LT.'AB  X'\n"
          "      PRINT *, A.GT.'AB  ', A.EQ.'AB', A.LT.'AB~'\n"
          "      PRINT *, A.GT.'AB~', A.GE.'AB  Y', A.LE.'AB'\n      END\n");
    /* concatenar subcadenas cuya longitud real es menor que su capacidad (hueco libre al final) */
    IGUAL("      CHARACTER*6 S\n      S = 'ABCDEF'\n      I = 2\n      J = 4\n"
          "      PRINT *, S(I:I+1) // 'X', S(I:I) // S(3:2) // 'Y'\n"
          "      PRINT *, S(I:J) // S(J:J) // S(1:I)\n      END\n");
    /* resultado de concatenar mas largo que la capacidad: se trunca a 255 */
    IGUAL("      CHARACTER*200 A\n      CHARACTER*100 B\n      CHARACTER*255 R\n      A = 'start'\n      B = 'end'\n"
          "      R = A // B\n      PRINT *, R(1:5), R(201:203), R(255:255)\n      END\n");
    /* mezcla con enteros y logicos en el mismo PRINT; temporales reutilizados con tipos distintos */
    IGUAL("      CHARACTER*3 A\n      A = 'Q'\n      I = 5 + 2*3\n      PRINT *, I, A, I.GT.3, A // 'W', I - 1\n      END\n");
}

/* La imagen guardada en paginas es byte a byte la del buffer, tambien cuando cruza el limite de pagina (8 KB). */
static void test_imagen_identica(void)
{
    static uint8_t a[16384], b[16384];
    char prog[40000];
    BackRes r1, r2;
    int k, i, mal = 0;

    /* programa largo: muchas sentencias para pasar de 8 KB de imagen */
    strcpy(prog, "      INTEGER I, J, K\n      I = 1\n");
    for (k = 0; k < 210; k++) {
        strcat(prog, "      J = I * 3 + K / 2 - I\n      K = J + I\n");
    }
    strcat(prog, "      PRINT *, K\n      END\n");
    COMPROBAR(a_tac(prog) == 0);
    back_generar(a, sizeof(a), ORG, NULL, &r1);
    back_generar(NULL, sizeof(b), ORG, NULL, &r2);
    COMPROBAR(r1.err == 0 && r2.err == 0);
    COMPROBAR(r1.fin_img == r2.fin_img && r1.fin_img - ORG > 8192 + 100);
    z_imagen_leer(0, b, (uint16_t)(r2.fin_img - ORG));
    for (i = 0; i < r1.fin_img - ORG; i++) {
        if (a[i] != b[i]) {
            mal = 1;
            break;
        }
    }
    COMPROBAR(!mal);
}

/* Salida a pantalla (rutina propia sin ROM): la fuente de la ROM se sustituye por una falsa en la que
 * las 8 lineas del caracter c valen c; asi se puede leer el texto de la memoria de pantalla. */
static void pantalla_esperada(const char *txt, uint8_t cel[24][32])
{
    int col = 0, fila = 0;

    memset(cel, 0, 24 * 32);
    for (; *txt; txt++) {
        if (*txt == '\n' || (cel[fila][col] = (uint8_t)*txt, ++col == 32)) {
            col = 0;
            if (fila < 23) {                 /* salto de linea; tras la fila 23 la pantalla SUBE una fila y se borra la ultima */
                fila++;
            } else {
                memmove(cel[0], cel[1], 23 * 32);
                memset(cel[23], 0, 32);
            }
        }
    }
}

static void prueba_pantalla(int linea_test, const char *prog)
{
    char o1[4096], o2[16];
    TacVmRes tr;
    BackRes br;
    Z80EmuRes er;
    uint8_t esp[24][32];
    int f, c, l, mal = 0, ch;

    pruebas++;
    if (a_tac(prog) != 0) {
        printf("FALLO (linea %d): no compila\n", linea_test);
        fallos++;
        return;
    }
    tacvm_ejecutar(o1, sizeof(o1), 5000000u, &tr);
    back_salida_pantalla(1);
    memset(mem, 0, sizeof(mem));
    listado[0] = '\0';
    for (ch = 32; ch < 128; ch++) {
        memset(mem + 0x3C00 + 8 * ch, ch, 8);
    }
    back_generar(mem + ORG, (uint16_t)(0x10000 - ORG - 0x1000), ORG, sal, &br);
    back_salida_pantalla(0);
    if (br.err != 0) {
        printf("FALLO (linea %d): backend %d\n", linea_test, br.err);
        fallos++;
        return;
    }
    z80emu_ejecutar(mem, br.entry, 50000000u, o2, sizeof(o2), &er);
    pantalla_esperada(o1, esp);
    if (er.estado != 0 || o2[0] != '\0') {
        mal = 1;
    }
    for (f = 0; f < 24 && !mal; f++) {
        for (c = 0; c < 32 && !mal; c++) {
            for (l = 0; l < 8; l++) {
                uint16_t a = (uint16_t)(0x4000 + ((f & 0x18) << 8) + ((f & 7) << 5) + c + (l << 8));

                if (mem[a] != esp[f][c]) {
                    printf("   celda fila %d col %d linea %d: %02X esperado %02X\n", f, c, l, mem[a], esp[f][c]);
                    mal = 1;
                    break;
                }
            }
        }
    }
    if (mal) {
        printf("FALLO (linea %d): la pantalla no coincide (estado %d)\n", linea_test, er.estado);
        fallos++;
    }
}

static void test_pantalla(void)
{
    char p[2048];

    prueba_pantalla(__LINE__, "      PROGRAM T\n      PRINT *, 'HOLA'\n      END\n");
    prueba_pantalla(__LINE__, "      PROGRAM T\n      PRINT *, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789ABC'\n      PRINT *, 42\n      END\n");
    strcpy(p, "      PROGRAM T\n      INTEGER I\n      DO 10 I = 1, 40\n      PRINT *, I, I*I\n   10 CONTINUE\n      END\n");
    prueba_pantalla(__LINE__, p);
    /* scroll: mas de 24 lineas (varias veces, con cambio de tercio de pantalla) y una linea que se parte justo en la ultima fila */
    prueba_pantalla(__LINE__, "      PROGRAM T\n      INTEGER I\n      DO 10 I = 1, 24\n      PRINT *, I\n   10 CONTINUE\n      END\n");
    prueba_pantalla(__LINE__, "      PROGRAM T\n      INTEGER I\n      DO 10 I = 1, 23\n      PRINT *, I\n   10 CONTINUE\n"
                              "      PRINT *, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'\n      PRINT *, 'FIN'\n      END\n");
    prueba_pantalla(__LINE__, "      PROGRAM T\n      INTEGER I\n      DO 10 I = 1, 200\n      PRINT *, I, 'x'\n   10 CONTINUE\n      END\n");
    prueba_pantalla(__LINE__, "      PROGRAM T\n      INTEGER I\n      DO 10 I = -3, 3\n      PRINT *, I, I.GT.0\n   10 CONTINUE\n      END\n");
}

/* ---- .nex: mf_salida + mf_compilar ("construir") ---- */
static uint8_t nexb[NEX_TAM + 256];
static uint32_t nex_n, nex_llamadas, nex_abiertos, nex_cerrados;
static int      nex_cerrar_ok;                 /* ultimo argumento de cerrar */
static uint32_t nex_falla_en;                  /* escribir falla en la llamada n (0 = nunca) */
static int      nex_abrir_ok, nex_cerrar_ret;
static int      nex_trozo_max;

static uint8_t n_abrir(void)
{
    nex_abiertos++;
    nex_n = 0;
    return (uint8_t)nex_abrir_ok;
}

static uint8_t n_escribir(const void *d, uint16_t n)
{
    nex_llamadas++;
    if (nex_falla_en != 0 && nex_llamadas == nex_falla_en) {
        return 0;
    }
    if (n > nex_trozo_max) nex_trozo_max = n;
    if (nex_n + n <= sizeof(nexb)) memcpy(nexb + nex_n, d, n);
    nex_n += n;
    return 1;
}

static uint8_t n_cerrar(uint8_t ok)
{
    nex_cerrados++;
    nex_cerrar_ok = ok;
    return (uint8_t)nex_cerrar_ret;
}

static const MfSalida nsal = { n_abrir, n_escribir, n_cerrar };

static void nex_reiniciar(void)
{
    nex_n = nex_llamadas = nex_abiertos = nex_cerrados = 0;
    nex_falla_en = 0; nex_abrir_ok = 1; nex_cerrar_ret = 1; nex_cerrar_ok = -1; nex_trozo_max = 0;
    memset(nexb, 0xEE, sizeof(nexb));
}

static void construir(const char *prog)
{
    nex_reiniciar();
    banco_iniciar();
    texto_poner(prog);
    mf_salida(&nsal);
}

static void test_nex(void)
{
    static const char *progs[] = {
        "      PROGRAM T\n      PRINT *, 'HOLA'\n      END\n",
        "      PROGRAM T\n      INTEGER I, F(23)\n      F(1) = 1\n      F(2) = 1\n      DO 10 I = 3, 23\n"
        "      F(I) = F(I-1) + F(I-2)\n   10 CONTINUE\n      DO 20 I = 1, 23\n      PRINT *, I, F(I)\n   20 CONTINUE\n      END\n",
        "      PROGRAM T\n      CHARACTER*12 S\n      CHARACTER*5 N(3)\n      N(1) = 'ANA'\n      S = 'HOLA ' // N(1)\n"
        "      PRINT *, S\n      IF (N(1) .GT. 'B') PRINT *, 'X'\n      END\n"
    };
    char m[MF_MENSAJE_MAX];
    BackRes br;
    uint32_t i, k;
    int p;

    for (p = 0; p < 3; p++) {
        uint8_t ref[16384];

        construir(progs[p]);
        COMPROBAR(mf_compilar() == 0);
        COMPROBAR(mf.gen == MF_GEN_OK && mf.gen_cod == 0);
        COMPROBAR(nex_abiertos == 1 && nex_cerrados == 1 && nex_cerrar_ok == 1);
        COMPROBAR(nex_n == NEX_TAM && nex_llamadas == NEX_TAM / NEX_TROZO && nex_trozo_max == NEX_TROZO);
        /* tras construir, la salida vuelve al defecto (RST 16): el siguiente back_generar normal no usa la rutina de pantalla */
        {
            BackRes b0, b1, b2;

            back_generar(ref, sizeof(ref), 0x8000, NULL, &b0);
            back_salida_pantalla(0);
            back_generar(ref, sizeof(ref), 0x8000, NULL, &b1);
            back_salida_pantalla(1);
            back_generar(ref, sizeof(ref), 0x8000, NULL, &b2);
            COMPROBAR(b0.err == 0 && b0.fin_img == b1.fin_img && b0.fin_img < b2.fin_img);
        }
        /* referencia independiente: la imagen con salida a pantalla, en un buffer */
        memset(ref, 0, sizeof(ref));
        back_salida_pantalla(1);
        back_generar(ref, sizeof(ref), 0x8000, NULL, &br);
        COMPROBAR(br.err == 0 && br.fin_mem <= 0xBE00);
        COMPROBAR(memcmp(nexb, "NextV1.2", 8) == 0);
        COMPROBAR(nexb[8] == 0 && nexb[9] == 1 && nexb[10] == 0 && nexb[11] == 0);
        COMPROBAR(nexb[12] == 0xFE && nexb[13] == 0xBF && nexb[14] == 0x00 && nexb[15] == 0xBE);
        COMPROBAR(nexb[18 + 2] == 1);
        for (i = 16, k = 0; i < 512; i++) {
            if (i != 20 && nexb[i] != 0) k++;
        }
        COMPROBAR(k == 0 && nexb[16] == 0 && nexb[17] == 0 && nexb[18] == 0 && nexb[19] == 0);
        for (i = 21, k = 0; i < 512; i++) k += nexb[i] != 0;
        COMPROBAR(k == 0);
        /* programa en $8000 + 512 */
        COMPROBAR(memcmp(nexb + 512, ref, (size_t)(br.fin_img - br.org)) == 0);
        /* entre el programa y el arranque, ceros */
        for (i = br.fin_img - 0x8000u, k = 0; i < 0x3E00u; i++) k += nexb[512 + i] != 0;
        COMPROBAR(k == 0);
        /* arranque en $BE00 (posicion 512 + $3E00) */
        {
            const uint8_t *a = nexb + 512 + 0x3E00;

            uint8_t esp[32] = {
                0xF3, 0x21, 0x00, 0x40, 0x11, 0x01, 0x40, 0x01, 0xFF, 0x17, 0x36, 0x00, 0xED, 0xB0,
                0x21, 0x00, 0x58, 0x11, 0x01, 0x58, 0x01, 0xFF, 0x02, 0x36, 0x07, 0xED, 0xB0,
                0xCD, 0, 0, 0x18, 0xFE
            };

            esp[28] = (uint8_t)(br.entry & 0xFF);
            esp[29] = (uint8_t)(br.entry >> 8);
            COMPROBAR(memcmp(a, esp, 32) == 0);
            for (i = 32, k = 0; i < 0x200; i++) k += a[i] != 0;
            COMPROBAR(k == 0);
        }
        COMPROBAR(nexb[NEX_TAM] == 0xEE);                /* no escribe de mas */
    }

    /* "comprobar": sin salida registrada no se genera ni se abre nada */
    construir(progs[0]);
    mf_salida(NULL);
    COMPROBAR(mf_compilar() == 0 && mf.gen == MF_GEN_NO && nex_abiertos == 0 && nex_llamadas == 0);

    /* con errores de compilacion no se genera */
    construir("      PROGRAM T\n      I = (1\n      END\n");
    COMPROBAR(mf_compilar() == 1 && mf.gen == MF_GEN_NO && nex_abiertos == 0);

    /* no se pudo abrir: ni escribir ni cerrar */
    construir(progs[0]);
    nex_abrir_ok = 0;
    COMPROBAR(mf_compilar() == 0 && mf.gen == MF_GEN_ERR && mf.gen_cod == MF_COD_GEN + 6);
    COMPROBAR(nex_abiertos == 1 && nex_llamadas == 0 && nex_cerrados == 0);
    mf_mensaje(mf.gen_cod, m);
    COMPROBAR(TEXTO(m, "create"));

    /* fallo al escribir en la llamada 1, 7 y la ultima: cerrar(0) y ni una llamada mas */
    {
        uint32_t fallos_en[3];
        int q;

        fallos_en[0] = 1; fallos_en[1] = 7; fallos_en[2] = NEX_TAM / NEX_TROZO;
        for (q = 0; q < 3; q++) {
            construir(progs[1]);
            nex_falla_en = fallos_en[q];
            COMPROBAR(mf_compilar() == 0 && mf.gen == MF_GEN_ERR && mf.gen_cod == MF_COD_GEN + 7);
            COMPROBAR(nex_llamadas == fallos_en[q] && nex_cerrados == 1 && nex_cerrar_ok == 0);
            mf_mensaje(mf.gen_cod, m);
            COMPROBAR(TEXTO(m, "writing"));
        }
    }

    /* cerrar falla (todo escrito): error de escritura, cerrar recibio ok=1 */
    construir(progs[0]);
    nex_cerrar_ret = 0;
    COMPROBAR(mf_compilar() == 0 && mf.gen == MF_GEN_ERR && mf.gen_cod == MF_COD_GEN + 7);
    COMPROBAR(nex_n == NEX_TAM && nex_cerrar_ok == 1);

    /* las variables no caben bajo $BE00: no se abre el fichero */
    construir("      PROGRAM T\n      INTEGER A(14000)\n      A(1) = 1\n      PRINT *, A(1)\n      END\n");
    COMPROBAR(mf_compilar() == 0 && mf.gen == MF_GEN_ERR && mf.gen_cod == MF_COD_GEN + 5);
    COMPROBAR(nex_abiertos == 0);
    mf_mensaje(mf.gen_cod, m);
    COMPROBAR(TEXTO(m, "BE00"));

    /* la imagen no cabe en 16 KB (error del backend): codigo 2, sin abrir */
    {
        static char grande[60000];
        int j;

        strcpy(grande, "      PROGRAM T\n      INTEGER I, J\n      I = 0\n      J = 1\n");
        for (j = 0; j < 500; j++) {
            strcat(grande, "      I = (I + J) * (I - J) + J\n");
        }
        strcat(grande, "      END\n");
        construir(grande);
        COMPROBAR(mf_compilar() == 0);
        COMPROBAR(mf.gen == MF_GEN_ERR && mf.gen_cod == MF_COD_GEN + 2 && nex_abiertos == 0);
        mf_mensaje(mf.gen_cod, m);
        COMPROBAR(TEXTO(m, "too large"));
    }

    /* nex_comprobar y nex_escribir directos */
    memset(&br, 0, sizeof br);
    br.org = 0x8000; br.fin_img = 0x8010; br.fin_mem = 0xBE00;
    COMPROBAR(nex_comprobar(&br) == NEX_OK);
    br.fin_mem = 0xBE01;
    COMPROBAR(nex_comprobar(&br) == NEX_NO_CABE);
    nex_reiniciar();
    COMPROBAR(nex_escribir(n_escribir, &br) == NEX_NO_CABE && nex_llamadas == 0);
    br.fin_mem = 0xBE00; br.org = 0x7FFF;
    COMPROBAR(nex_comprobar(&br) == NEX_NO_CABE);
    br.org = 0x8000; br.err = BACK_NO_SOPORTADO;
    COMPROBAR(nex_comprobar(&br) == NEX_RESULTADO);
    br.err = 0; br.fin_img = 0x7000;
    COMPROBAR(nex_comprobar(&br) == NEX_RESULTADO);
    br.fin_img = 0x8010; br.fin_mem = 0x8008;                  /* variables antes del fin de la imagen */
    COMPROBAR(nex_comprobar(&br) == NEX_NO_CABE);
    br.fin_mem = 0x800F;
    COMPROBAR(nex_comprobar(&br) == NEX_NO_CABE);
    br.fin_mem = 0x8010;
    COMPROBAR(nex_comprobar(&br) == NEX_OK);

    /* todos los mensajes de generacion existen y caben */
    for (i = 1; i <= 8; i++) {
        mf_mensaje((uint8_t)(MF_COD_GEN + i), m);
#ifndef MF_SIN_TEXTO
        COMPROBAR(m[0] != '\0');
#endif
        COMPROBAR(strlen(m) < MF_MENSAJE_MAX);
    }
    mf_salida(NULL);
}

int main(void)
{
    test_programas();
    test_caracteres();
    modo_paginas = 1;                         /* lo mismo con la imagen en paginas de datos (Next) */
    test_programas();
    test_caracteres();
    test_imagen_identica();
    modo_paginas = 0;
    test_no_soportado();
    test_listado();
    test_diferencial();
    test_pantalla();
    test_nex();
    printf("%d pruebas, %d fallos\n", pruebas, fallos);
    return fallos != 0;
}
