/* texto.c - ed_linea sobre un texto en memoria (ver texto.h). */
#include <string.h>
#include "texto.h"
#include "mf_api.h"

static const char *base = "";
static const char *cursor = "";     /* inicio de la linea `cursor_n`        */
static uint16_t cursor_n = 1;

void texto_poner(const char *texto)
{
    base = texto;
    cursor = texto;
    cursor_n = 1;
}

void fuente_iniciar_texto(Fuente *f, const char *texto)
{
    texto_poner(texto);
    fuente_iniciar(f);
}

int ed_linea(uint16_t n, char *dst)
{
    uint16_t k = 0;

    if (n < cursor_n) {                 /* hacia atras: otra vez desde el principio */
        cursor = base;
        cursor_n = 1;
    }
    while (cursor_n < n) {              /* avanzar hasta la linea n */
        while (*cursor != '\0' && *cursor != '\n') {
            cursor++;
        }
        if (*cursor == '\0') {
            return 0;
        }
        cursor++;
        cursor_n++;
    }
    if (*cursor == '\0') {
        return 0;                       /* detras del ultimo '\n' no hay linea */
    }
    {
        const char *p = cursor;

        while (*p != '\0' && *p != '\n') {
            if (*p != '\r' && k < 72) {     /* '\r' no cuenta; maximo 72 columnas */
                dst[k++] = *p;
            }
            p++;
        }
    }
    dst[k] = '\0';
    return 1;
}

const char *sent_texto(void)
{
    static char buf[MAX_SENTENCIA + 1];

    sent_leer(0, buf, (uint16_t)(sentencia.longitud + 1));
    buf[sentencia.longitud] = '\0';
    return buf;
}

void sent_poner(const char *texto)
{
    uint16_t n = (uint16_t)strlen(texto);

    if (n > MAX_SENTENCIA) {
        n = MAX_SENTENCIA;
    }
    sent_escribir(0, texto, n);
    sent_escribir(n, "", 1);
    sentencia.longitud = n;
}
