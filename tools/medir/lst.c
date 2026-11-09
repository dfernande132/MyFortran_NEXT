/* herramienta de depuracion: lee un programa F77 (stdin), lo compila y muestra errores, TAC y ejecucion */
#include <stdio.h>
#include <string.h>
#include "mf_api.h"
#include "gen.h"
#include "tac.h"
#include "texto.h"
#include "banco.h"
#include "tacvm.h"
static void sal(const char *l) { puts(l); }
int main(void)
{
    static char prog[262144];
    size_t n = fread(prog, 1, sizeof(prog) - 1, stdin);
    char sout[8192], msg[96];
    TacVmRes r;
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
    tac_listar(sal);
    if (tacvm_validar(msg, sizeof msg)) { printf("INVALIDO: %s\n", msg); return 2; }
    tacvm_ejecutar(sout, sizeof sout, 1000000, &r);
    printf("--- estado %d pasos %u %s\n%s", r.estado, (unsigned)r.pasos, r.msg, sout);
    return 0;
}
