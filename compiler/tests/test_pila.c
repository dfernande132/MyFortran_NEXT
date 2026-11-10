/* test_pila.c - Pila del Z80 que usan los programas Fortran GENERADOS (no la del compilador).
 *
 * En el Next el programa generado arranca con SP=$BFFE y su codigo/datos llegan como mucho a $BE00
 * (arranque en $BE00..$BE1F, nex.c): quedan 478 B de pila. F77 no tiene recursion, asi que la profundidad
 * maxima de llamadas es el numero de unidades (UNI_MAX = 63, programa principal incluido).
 *
 * Aqui se generan cadenas de llamadas de la profundidad maxima (SUBROUTINE con argumentos, FUNCTION entera,
 * FUNCTION REAL dentro de expresiones, y PRINT/READ de REAL en el nivel mas profundo), se ejecutan en z80emu y se
 * mide la pila (SP inicial - SP minimo). Cada programa debe dar la misma salida en tacvm y en z80emu.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mf_api.h"
#include "tac.h"
#include "back.h"
#include "texto.h"
#include "banco.h"
#include "tacvm.h"
#include "z80emu.h"

#define ORG 0x8000
#define PRESUPUESTO 478u
#define SUBS 62                         /* UNI_MAX (63) menos el programa principal */

static int pruebas, fallos;
static uint8_t mem[65536];
static char prog[65536], o1[16384], o2[16384];

static void ok(int cond, const char *nombre, const char *que)
{
    pruebas++;
    if (!cond) {
        fallos++;
        printf("FALLO %s: %s\n", nombre, que);
    }
}

static void probar(const char *nombre, const char *fuente)
{
    char msg[96];
    uint16_t e, i;
    TacVmRes tr;
    BackRes br;
    Z80EmuRes er;

    banco_iniciar();
    texto_poner(fuente);
    e = mf_compilar();
    for (i = 0; i < e && i < MF_DIAG_MAX; i++) {
        char m[MF_MENSAJE_MAX];

        mf_mensaje(mf.diag[i].cod, m);
        printf("   %s: %u:%u %s\n", nombre, (unsigned)mf.diag[i].linea, (unsigned)mf.diag[i].columna, m);
    }
    ok(e == 0, nombre, "no compila");
    if (e != 0) {
        return;
    }
    ok(tacvm_validar(msg, sizeof(msg)) == 0, nombre, "TAC invalido");
    tacvm_entrada("");
    back_entrada_prueba(1);
    tacvm_ejecutar(o1, sizeof(o1), 20000000u, &tr);
    ok(tr.estado != TACVM_ERROR, nombre, tr.estado == TACVM_ERROR ? tr.msg : "");
    memset(mem, 0, sizeof(mem));
    mem[0xF000] = 0x02;
    mem[0xF001] = 0xF0;
    back_generar(mem + ORG, (uint16_t)(0xF000 - ORG), ORG, NULL, &br);
    if (br.err != 0) { printf("   %s: back err=%d inst=%u\n", nombre, (int)br.err, (unsigned)br.err_inst); }
    ok(br.err == 0, nombre, "el generador Z80 da error");
    if (br.err != 0) {
        return;
    }
    z80emu_ejecutar(mem, br.entry, 200000000u, o2, sizeof(o2), &er);
    ok(er.estado == Z80EMU_OK, nombre, "el emulador no termino bien");
    ok(strcmp(o1, o2) == 0, nombre, "las salidas del TAC y del Z80 difieren");
    ok(er.pila_usada <= PRESUPUESTO, nombre, "pila superior al presupuesto");
    printf("   %-28s pila %3u B (presupuesto %u)  salida: %.30s", nombre, (unsigned)er.pila_usada, PRESUPUESTO, o2);
    if (strchr(o2, '\n') == NULL) {
        printf("\n");
    }
}

/* Limites de complejidad de una sentencia (programas de tools/medir/pila): el maximo compila y la siguiente forma da el error
 * controlado (nunca otra cosa). Son los programas que se miden en el Next con Esc+C / Esc+O (LEEME.md de esa carpeta). */
static void limite(const char *fichero, int debe_compilar)
{
    char ruta[160];
    FILE *f;
    size_t n;
    uint16_t e;

    sprintf(ruta, "tools/medir/pila/%s", fichero);
    f = fopen(ruta, "rb");
    ok(f != NULL, fichero, "no se puede leer el programa");
    if (f == NULL) {
        return;
    }
    n = fread(prog, 1, sizeof(prog) - 1, f);
    prog[n] = '\0';
    fclose(f);
    banco_iniciar();
    texto_poner(prog);
    e = mf_compilar();
    ok((e == 0) == (debe_compilar != 0), fichero, debe_compilar ? "deberia compilar (limite maximo)" : "deberia dar un error controlado");
    if (!debe_compilar && e > 0) {
        char m[MF_MENSAJE_MAX];

        mf_mensaje(mf.diag[0].cod, m);
        printf("   %-10s error esperado: %u:%u %s\n", fichero, (unsigned)mf.diag[0].linea, (unsigned)mf.diag[0].columna, m);
    }
}

int main(void)
{
    int n, k;

    limite("s28.f", 1);
    limite("s30.f", 1);
    limite("s31.f", 1);          /* 31 operandos: el maximo (63 nodos del AST: 31 + 30 + el nodo de la asignacion) */
    limite("s32.f", 0);          /* 32: "expression too large" (EXPR_POOL_LLENO), nunca una pila corrompida */
    limite("q10.f", 1);          /* 10 funciones-sentencia encadenadas: el maximo */
    limite("q11.f", 0);          /* 11: "statement function too large" */

    /* A) SUBROUTINE encadenadas con 2 argumentos enteros y uno REAL; la ultima imprime. */
    n = sprintf(prog, "      PROGRAM CAD\n      INTEGER A, B\n      REAL R\n      A = 1\n      B = 2\n      R = 1.5\n      CALL S1(A, B, R)\n      PRINT *, A, B, R\n      END\n");
    for (k = 1; k <= SUBS; k++) {
        n += sprintf(prog + n, "      SUBROUTINE S%d(A, B, R)\n      INTEGER A, B\n      REAL R\n", k);
        if (k < SUBS) {
            n += sprintf(prog + n, "      A = A + 1\n      CALL S%d(A, B, R)\n      B = B + A\n", k + 1);
        } else {
            n += sprintf(prog + n, "      A = A + 1\n      R = R * 2.0 + REAL(A)\n      PRINT *, A, R\n");
        }
        n += sprintf(prog + n, "      END\n");
    }
    probar("A_subrutinas_62", prog);

    /* B) FUNCTION entera encadenada: F(k) = F(k+1) + argumento, en una expresion con otra llamada pendiente. */
    n = sprintf(prog, "      PROGRAM CADF\n      INTEGER F1, I\n      I = F1(3) + F1(4)\n      PRINT *, I\n      END\n");
    for (k = 1; k <= SUBS; k++) {
        if (k < SUBS) {
            n += sprintf(prog + n, "      INTEGER FUNCTION F%d(X)\n      INTEGER X, F%d\n      F%d = F%d(X + 1) + X\n      END\n", k, k + 1, k, k + 1);
        } else {
            n += sprintf(prog + n, "      INTEGER FUNCTION F%d(X)\n      INTEGER X\n      F%d = X\n      END\n", k, k);
        }
    }
    probar("B_funciones_enteras_62", prog);

    /* C) FUNCTION REAL encadenada (usa las rutinas REAL en cada nivel) y PRINT de REAL al fondo. Un temporal por unidad:
     * el backend reserva los temporales de TODAS las unidades a la vez (NTMP = 128 en total; ver COORDINACION), asi que
     * 62 unidades con 4 temporales cada una no caben. */
    n = sprintf(prog, "      PROGRAM CADR\n      REAL G1, R\n      R = G1(1.0) * G1(2.0)\n      PRINT *, R\n      END\n");
    for (k = 1; k <= SUBS; k++) {
        if (k < SUBS) {
            n += sprintf(prog + n, "      REAL FUNCTION G%d(X)\n      REAL X, G%d\n      G%d = G%d(X) + X\n      END\n", k, k + 1, k, k + 1);
        } else {
            n += sprintf(prog + n, "      REAL FUNCTION G%d(X)\n      REAL X\n      G%d = SQRT(X) + X\n      PRINT *, G%d\n      END\n", k, k, k);
        }
    }
    probar("C_funciones_reales_62", prog);

    /* D) Argumentos CHARACTER y arrays en la cadena. */
    n = sprintf(prog, "      PROGRAM CADC\n      CHARACTER*8 T\n      INTEGER V(4)\n      T = 'HOLA'\n      V(1) = 7\n      CALL H1(T, V)\n      PRINT *, T, V(1)\n      END\n");
    for (k = 1; k <= SUBS; k++) {
        n += sprintf(prog + n, "      SUBROUTINE H%d(T, V)\n      CHARACTER*8 T\n      INTEGER V(4)\n", k);
        if (k < SUBS) {
            n += sprintf(prog + n, "      V(1) = V(1) + 1\n      CALL H%d(T, V)\n", k + 1);
        } else {
            n += sprintf(prog + n, "      V(1) = V(1) + 1\n      T = T // 'X'\n      PRINT *, T\n");
        }
        n += sprintf(prog + n, "      END\n");
    }
    probar("D_cadenas_arrays_62", prog);

    /* E) Demasiados temporales en todo el programa (NTMP = 128 entre todas las unidades): error propio, no "no soportado". */
    n = sprintf(prog, "      PROGRAM TMP\n      REAL H1, R\n      R = H1(1.0)\n      PRINT *, R\n      END\n");
    for (k = 1; k <= 50; k++) {
        if (k < 50) {
            n += sprintf(prog + n, "      REAL FUNCTION H%d(X)\n      REAL X, H%d\n      H%d = H%d(X * 0.5 + 1.0) / 2.0 + X * X - X\n      END\n", k, k + 1, k, k + 1);
        } else {
            n += sprintf(prog + n, "      REAL FUNCTION H%d(X)\n      REAL X\n      H%d = X * X + X * 2.0 - X / 3.0\n      END\n", k, k);
        }
    }
    {
        BackRes br;

        banco_iniciar();
        texto_poner(prog);
        ok(mf_compilar() == 0, "E_temporales", "no compila");
        memset(mem, 0, sizeof(mem));
        back_generar(mem + ORG, (uint16_t)(0xF000 - ORG), ORG, NULL, &br);
        ok(br.err == BACK_TEMPORALES, "E_temporales", "debe dar BACK_TEMPORALES");
    }

    printf("%d pruebas, %d fallos\n", pruebas, fallos);
    return fallos != 0;
}
