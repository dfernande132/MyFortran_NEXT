/* herramienta de depuracion: lee un programa F77 (stdin), lo compila a Z80, muestra el .asm
 * y lo ejecuta en el emulador de PC.
 *   z80.exe [salida.asm [salida.bin [salida.nex [esperado.txt]]]]
 * El texto esperado sale de ejecutar en el emulador la version con RST 16. El .asm, .bin y .nex llevan la
 * version con salida PROPIA a pantalla (rutina rt_putc que dibuja con la fuente de la ROM en $3D00, sin
 * llamar a la ROM). El .nex (NEX V1.2, banco 16K 2 = $8000..$BFFF) lleva el programa en $8000 y un
 * arranque en $BE00 que borra la pantalla, pone los atributos a 7 (tinta blanca, papel negro) y llama
 * al programa. Sin scroll: tras la linea 24 vuelve arriba.
 * El .asm y el .bin se pueden pasar a z88dk-z80asm para comparar byte a byte (-b, org $8000).
 */
#include <stdio.h>
#include <string.h>
#include "mf_api.h"
#include "back.h"
#include "nex.h"
#include "texto.h"
#include "banco.h"
#include "z80emu.h"

static FILE *fasm, *fnex;

static uint8_t sal_nex(const void *d, uint16_t n)
{
    return fnex != NULL && fwrite(d, 1, n, fnex) == n;
}
static void sal(const char *l)
{
    puts(l);
    if (fasm) { fputs(l, fasm); fputc('\n', fasm); }
}

int main(int argc, char **argv)
{
    static char prog[262144];
    static uint8_t img[32768];
    static uint8_t mem[65536];
    size_t n = fread(prog, 1, sizeof(prog) - 1, stdin);
    char sout[8192];
    BackRes br;
    Z80EmuRes zr;
    uint16_t e, i;

    prog[n] = 0;
    banco_iniciar();
    texto_poner(prog);
    e = mf_compilar();
    for (i = 0; i < e && i < MF_DIAG_MAX; i++) {
        char m[MF_MENSAJE_MAX];
        mf_mensaje(mf.diag[i].cod, m);
        printf("ERROR %u:%u %s\n", mf.diag[i].linea, mf.diag[i].columna, m);
    }
    if (e) return 1;
    back_generar(img, sizeof(img), 0x8000, NULL, &br);
    if (br.err != BACK_OK) {
        printf("BACKEND: error %d en la instruccion TAC %u\n", br.err, (unsigned)br.err_inst);
        return 2;
    }
    {
        uint16_t k, cs = 0;

        for (k = 0; k < br.fin_img - br.org; k++) cs = (uint16_t)(cs * 31u + img[k]);
        printf("; --- imagen con RST 16 (la que genera el spike g1 en el Next): %u bytes, memoria hasta %04X, suma %04X\n",
               (unsigned)(br.fin_img - br.org), (unsigned)br.fin_mem, (unsigned)cs);
    }
    memset(mem, 0, sizeof(mem));
    memcpy(mem + br.org, img, br.fin_img - br.org);
    z80emu_ejecutar(mem, br.entry, 50000000u, sout, sizeof sout, &zr);
    back_salida_pantalla(1);
    if (argc > 1) fasm = fopen(argv[1], "w");
    back_generar(img, sizeof(img), 0x8000, sal, &br);
    if (fasm) fclose(fasm);
    if (br.err != BACK_OK) {
        printf("BACKEND: error %d en la instruccion TAC %u\n", br.err, (unsigned)br.err_inst);
        return 2;
    }
    printf("; --- imagen %u bytes ($%04X..$%04X), memoria hasta $%04X\n",
           (unsigned)(br.fin_img - br.org), br.org, br.fin_img, br.fin_mem);
    if (argc > 2) {
        FILE *fb = fopen(argv[2], "wb");
        if (fb) { fwrite(img, 1, br.fin_img - br.org, fb); fclose(fb); }
    }
    if (argc > 3) {
        BackRes bn;
        int16_t en;

        back_salida_pantalla(1);                       /* el .nex dibuja por si mismo, sin la ROM */
        back_generar(NULL, 16384, 0x8000, NULL, &bn);   /* la imagen en las paginas de datos, como en el Next */
        back_salida_pantalla(0);
        fnex = fopen(argv[3], "wb");
        en = bn.err != BACK_OK ? NEX_RESULTADO : nex_escribir(sal_nex, &bn);
        if (fnex) fclose(fnex);
        if (en != NEX_OK) {
            printf("NEX: no se pudo generar (%d); fin de memoria $%04X\n", (int)en, bn.fin_mem);
        }
    }
    printf("; --- emulador: estado %d, %u instrucciones\n%s", zr.estado, (unsigned)zr.pasos, sout);
    if (argc > 4) {
        FILE *fe = fopen(argv[4], "wb");
        if (fe) { fputs(sout, fe); fclose(fe); }
    }
    return 0;
}
