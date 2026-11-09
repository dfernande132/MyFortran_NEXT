/* mf_diag.c - Tabla comun de diagnosticos (mf_api.h). La escriben source/stmt/sem
 * (a traves de mf.c y sem.c) y la lee el editor. Aparte de mf.c para que las fases
 * de un banco puedan anotar errores sin arrastrar el driver. */
#include <string.h>
#include "mf_api.h"
#include "source.h"

Mf mf;

void mf_diag_iniciar(void)
{
    memset(&mf, 0, sizeof(mf));
}

void mf_diag(uint8_t cod, uint16_t linea, uint16_t columna)
{
    if (mf.n < MF_DIAG_MAX) {
        MfDiag *d = &mf.diag[mf.n];

        d->cod = cod;
        d->linea = linea;
        d->columna = (uint8_t)(columna > 255 ? 255 : columna);
    }
    mf.n++;
}

void mf_diag_pos(uint8_t cod, uint16_t pos_bruta)
{
    uint16_t linea, col;

    posicion_fuente(pos_bruta, &linea, &col);
    mf_diag(cod, linea, col);
}
