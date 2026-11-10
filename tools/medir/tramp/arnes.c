#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <z80ex/z80ex.h>

static uint8_t mem[65536];
static uint16_t A_MAPEAR;
static int fallos = 0, pruebas = 0;
static unsigned mapeos = 0;
static uint16_t A_BANCO_ACTUAL, A_MMU;

static Z80EX_BYTE mr(Z80EX_CONTEXT *c, Z80EX_WORD a, int m1, void *u) { (void)c;(void)m1;(void)u; return mem[a]; }
static void mw(Z80EX_CONTEXT *c, Z80EX_WORD a, Z80EX_BYTE v, void *u) { (void)c;(void)u; mem[a] = v; }
static Z80EX_BYTE pr(Z80EX_CONTEXT *c, Z80EX_WORD p, void *u) { (void)c;(void)p;(void)u; return 0xFF; }
static void pw(Z80EX_CONTEXT *c, Z80EX_WORD p, Z80EX_BYTE v, void *u) { (void)c;(void)p;(void)v;(void)u; }
static Z80EX_BYTE ir(Z80EX_CONTEXT *c, void *u) { (void)c;(void)u; return 0xFF; }

static uint16_t rd16(uint16_t a) { return mem[a] | (mem[a + 1] << 8); }
static void ok(int c, const char *w) { pruebas++; if (!c) { fallos++; printf("FALLO: %s\n", w); } }

#define NINGUNO 0xFF
static uint16_t sym(const char *n)
{
    FILE *f = fopen("prueba.sym", "r"); char l[128], nm[64]; unsigned v;
    while (fgets(l, sizeof l, f)) { if (sscanf(l, "%63s EQU %xH", nm, &v) == 2 && !strcmp(nm, n)) { fclose(f); return (uint16_t)v; } }
    fclose(f); fprintf(stderr, "sin simbolo %s\n", n); exit(2);
}

/* ejecuta la rutina en `entrada` hasta que vuelve a la direccion centinela; emula mapear() como la de C */
static void ejecutar(Z80EX_CONTEXT *cpu, uint16_t entrada)
{
    const uint16_t CENT = 0x9000, SP0 = 0xBFF0;
    mem[CENT] = 0x76;                                   /* HALT */
    mem[SP0 - 2] = CENT & 255; mem[SP0 - 1] = CENT >> 8;
    z80ex_set_reg(cpu, regSP, SP0 - 2);
    z80ex_set_reg(cpu, regPC, entrada);
    for (long n = 0; n < 2000000; n++) {
        uint16_t pc = z80ex_get_reg(cpu, regPC);
        if (pc == A_MAPEAR) {
            uint16_t sp = z80ex_get_reg(cpu, regSP);
            uint8_t b = mem[(uint16_t)(sp + 2)];        /* argumento: palabra empujada, byte bajo */
            mem[A_MMU] = (b == NINGUNO) ? 0x77 : b;
            mem[A_BANCO_ACTUAL] = b;
            mapeos++;
            z80ex_set_reg(cpu, regPC, rd16(sp));        /* RET */
            z80ex_set_reg(cpu, regSP, sp + 2);
            continue;
        }
        if (pc == CENT) return;
        z80ex_step(cpu);
    }
    ok(0, "no termino");
}

int main(void)
{
    FILE *f = fopen("prueba.bin", "rb"); size_t n = fread(mem + 0x8000, 1, 0x1000, f); fclose(f); (void)n;
    A_MAPEAR = sym("_mapear"); A_BANCO_ACTUAL = sym("_banco_actual"); A_MMU = sym("_mmu_bank");
    uint16_t TR_PILA = sym("__tr_pila"), TR_PTR = sym("__tr_ptr");
    Z80EX_CONTEXT *cpu = z80ex_create(mr, 0, mw, 0, pr, 0, pw, 0, ir, 0);
    z80ex_reset(cpu);
    mem[A_MMU] = 0x77;                                  /* "lo que habia al arrancar" */

    /* 1. dos argumentos, resultado de 16 bits, IX/IY intactos, banco correcto dentro y restaurado fuera */
    ejecutar(cpu, sym("_test_suma"));
    ok(rd16(sym("_res1")) == 1234, "suma(1000,234) = 1234");
    ok(mem[sym("_seen1")] == 44, "dentro de suma el banco es 44");
    ok(rd16(sym("_ixsave")) == 0x1234, "IX intacto");
    ok(rd16(sym("_iysave")) == 0x5678, "IY intacto");
    ok(mem[A_MMU] == 0x77 && mem[A_BANCO_ACTUAL] == NINGUNO, "tras la llamada se restaura NINGUNO (0xFF)");
    ok(rd16(TR_PTR) == TR_PILA, "pila de trampolines vuelve vacia");
    ok(mapeos == 2, "dos mapeos (ir y volver)");
    ok(z80ex_get_reg(cpu, regSP) == 0xBFF0, "SP final = SP inicial (los argumentos los saca el llamador)");
    /* los argumentos empujados por el llamador siguen en su sitio tras la llamada: se comprueba via SP y la pila
       (el llamador hace pop bc ×2; si __tramp perdiera una palabra, SP no coincidiria) */

    /* 2. anidamiento: anida -> suma */
    mapeos = 0;
    ejecutar(cpu, sym("_test_anida"));
    ok(rd16(sym("_res2")) == 7 + 5 + 100, "anida(7) = 112");
    ok(mem[sym("_seen2")] == 45, "dentro de anida: 45");
    ok(mem[sym("_seen1")] == 44, "dentro de la suma anidada: 44");
    ok(mem[sym("_seen3")] == 45, "tras la suma anidada se restaura 45");
    ok(mem[A_MMU] == 0x77 && mem[A_BANCO_ACTUAL] == NINGUNO, "al salir, NINGUNO");
    ok(rd16(TR_PTR) == TR_PILA, "pila de trampolines vacia tras anidar");
    ok(mapeos == 4, "cuatro mapeos");

    /* 3. valor de 32 bits en DEHL */
    ejecutar(cpu, sym("_test_largo"));
    ok(rd16(sym("_res3")) == 0x2222 || rd16(sym("_res3")) == 0x1111, "HL conservado");
    ok(rd16(sym("_res3")) == 0x1111 && rd16(sym("_res4")) == 0x2222, "DEHL = (b:a) conservado");

    /* 4. profundidad: n llamadas anidadas a traves de trampolines */
    for (int d = 0; d <= 39; d++) {
        mem[sym("_nprof")] = d; mem[sym("_nprof") + 1] = 0;
        mapeos = 0;
        ejecutar(cpu, sym("_test_prof"));
        char m[64]; sprintf(m, "prof(%d)", d); ok(rd16(sym("_res1")) == d, m);
        ok(rd16(TR_PTR) == TR_PILA, "pila de trampolines vacia tras prof");
        ok(mem[A_MMU] == 0x77 && mem[A_BANCO_ACTUAL] == NINGUNO, "NINGUNO tras prof");
        ok(z80ex_get_reg(cpu, regSP) == 0xBFF0, "SP final tras prof");
    }
    printf("%d pruebas, %d fallos\n", pruebas, fallos);
    return fallos != 0;
}
