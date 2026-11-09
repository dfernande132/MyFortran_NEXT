/* test_programas.c - Programas completos: compiler/tests/programas/pNN_*.f, examples/*.f y compiler/tests/ejemplos/*.f.
 *
 * Cada programa se compila, se ejecuta como TAC (tacvm) y como Z80 (z80emu), y las dos salidas deben ser iguales entre
 * si y IGUALES a pNN_*.esperado.txt, que genera oraculo.py (Python, sin usar el compilador). Si existe
 * pNN_*.entrada.txt, es lo que "teclea" el programa para sus READ. Se ejecuta desde la raiz del repositorio.
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

static int pruebas = 0;
static int fallos = 0;
static uint8_t mem[65536];

/* Rutas sin extension. El .esperado.txt es obligatorio en programas/ y opcional en ejemplos/ (sin el solo se exige que
 * compile y que TAC y Z80 coincidan). En ejemplos/ de la raiz estan los que se ensenan; en tests/ejemplos/ los que
 * se usaron como prueba en cada etapa del compilador. */
#define PROG "compiler/tests/programas/"
#define EJTE "compiler/tests/ejemplos/"
#define EJEM "examples/"
static const char *const programas[] = {
    PROG "p01_enteros", PROG "p02_control", PROG "p03_primos", PROG "p04_orden", PROG "p05_matrices",
    PROG "p06_cadenas", PROG "p07_subprogramas", PROG "p08_reales", PROG "p09_logicos", PROG "p10_datos",
    PROG "p11_entrada", PROG "p12_algoritmos", PROG "p13_sintaxis", PROG "p14_limites",
    EJEM "fibo", EJEM "leer", EJEM "arrmat", EJEM "primos", EJEM "reales",
    EJTE "argch", EJTE "arrarg", EJTE "cadenas", EJTE "chrast", EJTE "chrsub", EJTE "cp1", EJTE "cp2", EJTE "cp3",
    EJTE "cs1", EJTE "datos", EJTE "dimen", EJTE "fs2", EJTE "fs2b", EJTE "fs2c", EJTE "param", EJTE "scroll",
    EJTE "subprogramas_s1"
};

static int leer(const char *nombre, const char *ext, char *dst, size_t max)
{
    char ruta[160];
    FILE *f;
    size_t n;

    sprintf(ruta, "%s%s", nombre, ext);
    if ((f = fopen(ruta, "rb")) == NULL) {
        return 0;
    }
    n = fread(dst, 1, max - 1, f);
    dst[n] = '\0';
    fclose(f);
    return 1;
}

static void ok(int cond, const char *nombre, const char *que)
{
    pruebas++;
    if (!cond) {
        fallos++;
        printf("FALLO %s: %s\n", nombre, que);
    }
}

static void probar(const char *nombre)
{
    static char prog[65536], esp[16384], ent[4096], o1[16384], o2[16384];
    char msg[96];
    uint16_t e, i;
    TacVmRes tr;
    BackRes br;
    Z80EmuRes er;
    int tiene_esp;

    ent[0] = '\0';
    ok(leer(nombre, ".f", prog, sizeof(prog)), nombre, "no se puede leer el .f");
    tiene_esp = leer(nombre, ".esperado.txt", esp, sizeof(esp));
    ok(tiene_esp || strncmp(nombre, PROG, strlen(PROG)) != 0, nombre, "no se puede leer el .esperado.txt");
    leer(nombre, ".entrada.txt", ent, sizeof(ent));
    banco_iniciar();
    texto_poner(prog);
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
    tacvm_entrada(ent);
    back_entrada_prueba(1);
    tacvm_ejecutar(o1, sizeof(o1), 20000000u, &tr);
    ok(tr.estado != TACVM_ERROR, nombre, tr.estado == TACVM_ERROR ? tr.msg : "");
    memset(mem, 0, sizeof(mem));
    strcpy((char *)mem + 0xF002, ent);
    mem[0xF000] = 0x02;
    mem[0xF001] = 0xF0;
    back_generar(mem + ORG, (uint16_t)(0xF000 - ORG), ORG, NULL, &br);
    ok(br.err == 0, nombre, "el generador Z80 da error");
    if (br.err != 0) {
        return;
    }
    z80emu_ejecutar(mem, br.entry, 200000000u, o2, sizeof(o2), &er);
    ok(strcmp(o1, o2) == 0, nombre, "las salidas del TAC y del Z80 difieren");
    if (!tiene_esp) {
        return;
    }
    ok(strcmp(o1, esp) == 0, nombre, "la salida del TAC difiere de la esperada");
    ok(strcmp(o2, esp) == 0, nombre, "la salida del Z80 difiere de la esperada");
    if (strcmp(o1, esp) != 0 || strcmp(o2, esp) != 0) {
        printf("--- esperado ---\n%s--- TAC ---\n%s--- Z80 ---\n%s", esp, o1, o2);
    }
}

int main(void)
{
    size_t i;

    for (i = 0; i < sizeof(programas) / sizeof(programas[0]); i++) {
        probar(programas[i]);
    }
    printf("%d pruebas, %d fallos\n", pruebas, fallos);
    return fallos != 0;
}
