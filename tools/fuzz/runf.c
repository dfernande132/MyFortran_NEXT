/* runf.c - Probador para los fuzzers: compila un .f, lo ejecuta como TAC (tacvm) y como Z80 (z80emu) y escribe la salida.
 * La ultima linea es IGUALES si el TAC y el Z80 coinciden (y no hubo errores), y DISTINTOS / ERROR ... si no.
 *
 *   gcc -std=c99 -Icompiler/src -Icompiler/include -Iplatform -o build/pc/runf compiler/src/*.c platform/pc/*.c platform/texto.c \
 *       tools/fuzz/runf.c -lm
 *   runf programa.f
 *
 * Para READ: MF_ENTRADA=fichero.txt runf programa.f   (el texto que "teclea" el programa; el TAC y el Z80 lo leen igual)
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
static uint8_t mem[65536];
static char entrada_z80[4096];

int main(int argc, char **argv)
{
    static char prog[65536], o1[16384], o2[16384];
    char msg[96];
    FILE *f;
    size_t n;
    uint16_t e, i;
    TacVmRes tr;
    BackRes br;
    Z80EmuRes er;

    if (argc < 2 || (f = fopen(argv[1], "rb")) == NULL) {
        fprintf(stderr, "uso: runf programa.f\n");
        return 2;
    }
    n = fread(prog, 1, sizeof(prog) - 1, f);
    prog[n] = '\0';
    fclose(f);
    banco_iniciar();
    texto_poner(prog);
    e = mf_compilar();
    for (i = 0; i < e && i < MF_DIAG_MAX; i++) {
        char m[MF_MENSAJE_MAX];

        mf_mensaje(mf.diag[i].cod, m);
        printf("ERROR %u:%u %s\n", (unsigned)mf.diag[i].linea, (unsigned)mf.diag[i].columna, m);
    }
    if (e) {
        return 1;
    }
    if (tacvm_validar(msg, sizeof(msg)) != 0) {
        printf("TAC INVALIDO %s\n", msg);
        return 1;
    }
    {
        static char ent[4096];
        const char *fe = getenv("MF_ENTRADA");

        ent[0] = '\0';
        if (fe != NULL && (f = fopen(fe, "rb")) != NULL) {
            n = fread(ent, 1, sizeof(ent) - 1, f);
            ent[n] = '\0';
            fclose(f);
        }
        tacvm_entrada(ent);
        back_entrada_prueba(1);
        strcpy(entrada_z80, ent);
    }
    tacvm_ejecutar(o1, sizeof(o1), 20000000u, &tr);
    if (tr.estado == TACVM_ERROR) {
        printf("ERROR tacvm: %s\n", tr.msg);
        return 1;
    }
    memset(mem, 0, sizeof(mem));
    strcpy((char *)mem + 0xF002, entrada_z80);
    mem[0xF000] = 0x02;
    mem[0xF001] = 0xF0;
    back_generar(mem + ORG, (uint16_t)(0xF000 - ORG), ORG, NULL, &br);
    if (br.err != 0) {
        printf("ERROR backend %d\n", (int)br.err);
        return 1;
    }
    z80emu_ejecutar(mem, br.entry, 200000000u, o2, sizeof(o2), &er);
    fputs(o1, stdout);
    printf(strcmp(o1, o2) == 0 ? "IGUALES\n" : "DISTINTOS\n%s", o2);
    return 0;
}
