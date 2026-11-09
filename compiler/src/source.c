/* source.c - Ver source.h.
 *
 * Reglas del formato fijo (columnas contadas desde 1):
 *   col 1      'C', 'c' o '*'  -> toda la linea es comentario
 *   col 1-5    etiqueta de la sentencia (opcional)
 *   col 6      distinto de blanco y de '0' -> linea de CONTINUACION
 *   col 7-72   la sentencia
 * Una linea vacia o solo con blancos tambien se ignora.
 *
 * Nota de diseno: este modulo NO usa stdio ni malloc (reglas del nucleo).
 * Recibe texto ya leido; quien lee el fichero es la capa platform/.
 */
#include <string.h>
#include "source.h"
#include "banco.h"
#include "mf_api.h"

Sentencia sentencia;    /* la sentencia en curso; unica y global (ver source.h) */

/* Devuelve 1 si la cadena esta vacia o solo tiene blancos, 0 si no. */
static int es_blanca(const char *linea)
{
    while (*linea != '\0') {
        if (*linea != ' ' && *linea != '\t') {
            return 0;
        }
        linea++;
    }
    return 1;
}

/* Devuelve el caracter de la columna 6 (indice 5 en C, que cuenta desde 0).
 *
 * TRAMPA: si la linea es mas corta que 6 caracteres, linea[5] esta FUERA
 * de la cadena y leeriamos memoria que no es nuestra. Por eso recorremos
 * los 6 primeros caracteres comprobando que no llegamos al '\0' antes.
 * Si la linea es corta, la columna 6 "no existe" y la tratamos como blanco. */
static char columna6(const char *linea)
{
    int i;

    for (i = 0; i < 6; i++) {
        if (linea[i] == '\0') {
            return ' ';
        }
    }
    return linea[5];
}

TipoLinea clasificar_linea(const char *linea)
{
    char c1;
    char c6;

    if (es_blanca(linea)) {
        return LINEA_IGNORADA;
    }

    c1 = linea[0];                       /* columna 1 */
    if (c1 == 'C' || c1 == 'c' || c1 == '*') {
        return LINEA_IGNORADA;
    }

    c6 = columna6(linea);
    if (c6 != ' ' && c6 != '0') {
        return LINEA_CONTINUACION;
    }
    return LINEA_INICIAL;
}

/* ------------------------------------------------------------------
 * Sentencias: iniciar_sentencia y anadir_continuacion
 * ------------------------------------------------------------------ */

/* Texto de la sentencia en la pagina de datos (ver source.h). La ventana evita una llamada a banco_leer por caracter. */
#define SC_TAM 32
static char     sc[SC_TAM];
static uint16_t sc_ini;
static uint8_t  sc_n;                   /* 0 = ventana vacia */

void sent_leer(uint16_t desp, char *dst, uint16_t n)
{
    banco_leer(SENT_PAGINA, (uint16_t)(SENT_OFF + desp), dst, n);
}

void sent_escribir(uint16_t desp, const char *src, uint16_t n)
{
    banco_escribir(SENT_PAGINA, (uint16_t)(SENT_OFF + desp), src, n);
    sc_n = 0;
}

char sent_car(uint16_t i)
{
    if (i >= sentencia.longitud) {
        return '\0';
    }
    if (sc_n == 0 || i < sc_ini || i >= (uint16_t)(sc_ini + sc_n)) {
        uint16_t n = (uint16_t)(sentencia.longitud - i);

        if (n > SC_TAM) {
            n = SC_TAM;
        }
        sent_leer(i, sc, n);
        sc_ini = i;
        sc_n = (uint8_t)n;
    }
    return sc[i - sc_ini];
}

#define COL_ULTIMA_ETIQUETA  5    /* la etiqueta ocupa las columnas 1-5       */
#define COL_PRIMERA_TEXTO    6    /* indice C de la columna 7 (cuenta desde 0) */
#define COL_ULTIMA_TEXTO     72   /* a partir de la 73 se ignora todo          */

/* Copia las columnas 7-72 de linea al final de sentencia.texto. Nunca desborda:
 * son 66 caracteres como mucho y quien llama ya ha reservado hueco.
 *
 * El bucle avanza por la linea entera y SOLO copia desde el indice 6. Asi
 * nunca miramos mas alla del '\0' aunque la linea sea corta (la trampa de
 * columna6, otra vez). */
static void copiar_texto(const char *linea)
{
    char tmp[COL_ULTIMA_TEXTO - COL_PRIMERA_TEXTO + 1];     /* 66 + '\0' */
    uint16_t i, n = 0;

    for (i = 0; i < COL_ULTIMA_TEXTO && linea[i] != '\0'; i++) {
        if (i >= COL_PRIMERA_TEXTO) {
            tmp[n++] = linea[i];
        }
    }
    tmp[n] = '\0';
    sent_escribir(sentencia.longitud, tmp, (uint16_t)(n + 1));
    sentencia.longitud = (uint16_t)(sentencia.longitud + n);
}

ErrorFuente iniciar_sentencia(const char *linea, uint16_t num_linea)
{
    /* uint32_t y no uint16_t: una etiqueta llega hasta 99999 y uint16_t
     * solo llega a 65535. Costara algo en el Z80 (aritmetica de 32 bits);
     * lo mediremos antes de decidir si merece la pena recortar. */
    uint32_t etiqueta = 0;
    int hay_digito = 0;
    uint16_t i;
    char c;

    /* Etiqueta: columnas 1-5 = indices 0..4. Los blancos no cuentan
     * ("1 0" vale 10). Cualquier otra cosa que no sea un digito es error. */
    for (i = 0; i < COL_ULTIMA_ETIQUETA; i++) {
        c = linea[i];
        if (c == '\0') {
            break;               /* linea corta: no hay mas columnas */
        }
        if (c == ' ') {
            continue;
        }
        if (c < '0' || c > '9') {
            return FUENTE_ETIQUETA_INVALIDA;
        }
        etiqueta = etiqueta * 10 + (uint32_t)(c - '0');
        hay_digito = 1;
    }
    /* Las etiquetas validas van de 1 a 99999: "00000" no vale. */
    if (hay_digito && etiqueta == 0) {
        return FUENTE_ETIQUETA_INVALIDA;
    }

    sentencia.etiqueta = etiqueta;
    sentencia.linea = num_linea;
    sentencia.longitud = 0;
    sentencia.segmentos = 1;
    sentencia.linea_seg[0] = num_linea;
    copiar_texto(linea);
    return FUENTE_OK;
}

ErrorFuente anadir_continuacion(const char *linea, uint16_t num_linea)
{
    uint16_t i;
    char c;

    /* En una continuacion las columnas 1-5 deben estar en blanco (no hay
     * etiqueta). La columna 6 ya sabemos que es el marcador. */
    for (i = 0; i < COL_ULTIMA_ETIQUETA; i++) {
        c = linea[i];
        if (c == '\0') {
            break;
        }
        if (c != ' ') {
            return FUENTE_CONTINUACION_INVALIDA;
        }
    }

    if (sentencia.segmentos >= MAX_SEGMENTOS) {
        return FUENTE_SENTENCIA_LARGA;
    }

    /* Rellenar la linea anterior con blancos hasta la columna 72: el estandar
     * lo exige y asi cada linea ocupa exactamente SEGMENTO caracteres
     * (necesario para posicion_fuente). */
    while (sentencia.longitud < (uint16_t)(sentencia.segmentos * SEGMENTO)) {
        char blancos[16];
        uint16_t k = (uint16_t)(sentencia.segmentos * SEGMENTO - sentencia.longitud);

        if (k > sizeof(blancos)) {
            k = sizeof(blancos);
        }
        memset(blancos, ' ', k);
        sent_escribir(sentencia.longitud, blancos, k);
        sentencia.longitud = (uint16_t)(sentencia.longitud + k);
    }

    sentencia.linea_seg[sentencia.segmentos] = num_linea;
    sentencia.segmentos++;
    copiar_texto(linea);
    return FUENTE_OK;
}

void posicion_fuente(uint16_t offset,
                     uint16_t *linea, uint16_t *columna)
{
    uint16_t seg = (uint16_t)(offset / SEGMENTO);

    if (seg >= sentencia.segmentos) {
        seg = (uint16_t)(sentencia.segmentos - 1);
    }
    *linea = sentencia.linea_seg[seg];
    *columna = (uint16_t)(offset - seg * SEGMENTO + COL_PRIMERA_TEXTO + 1);
}

/* ------------------------------------------------------------------
 * Lector de sentencias
 * ------------------------------------------------------------------ */

void fuente_iniciar(Fuente *f)
{
    f->num_linea = 1;
    f->linea_error = 0;
    f->tiene = 0;
    f->fin = 0;
    f->linea[0] = '\0';
}

/* Ajusta la linea que acaba de dar ed_linea (hasta LINEA_MAX caracteres, sin
 * '\n'): quita '\r' y expande el primer TABULADOR de las columnas 1-6.
 *
 * TABULADORES (extension de DEC/g77; el estandar no los contempla, ver 0004):
 *   - Un tabulador en las columnas 1-6 se expande con blancos hasta la
 *     columna 6, de modo que el siguiente caracter cae en la columna 7.
 *   - Si es un tabulador en el campo de etiqueta (cols 1-5) y le sigue un
 *     digito 1-9, ese digito es el marcador de continuacion: se coloca en la
 *     columna 6 y el texto empieza en la 7.
 *   - Un tabulador a partir de la columna 7 se deja tal cual (el escaner lo
 *     trata como un blanco). Expandirlo desplazaria las columnas.
 * El editor no manda ni '\t' ni '\r'; esto solo protege de otras fuentes. */
static void normalizar_linea(char *buf)
{
    uint16_t i, n, k;
    char d;

    for (i = 0, n = 0; buf[i] != '\0'; i++) {
        if (buf[i] != '\r') {
            buf[n++] = buf[i];
        }
    }
    buf[n] = '\0';

    for (i = 0; i < COL_PRIMERA_TEXTO && buf[i] != '\0' && buf[i] != '\t'; i++) {
    }
    if (i >= COL_PRIMERA_TEXTO || buf[i] != '\t') {
        return;                         /* sin tabulador en las columnas 1-6 */
    }
    d = buf[i + 1];
    if (i < COL_ULTIMA_ETIQUETA && d >= '1' && d <= '9') {
        k = (uint16_t)(i + 2);          /* el digito tambien se salta */
    } else {
        d = '\0';
        k = (uint16_t)(i + 1);
    }
    n = (uint16_t)strlen(buf + k);
    if (n > (uint16_t)(LINEA_MAX - COL_PRIMERA_TEXTO)) {
        n = (uint16_t)(LINEA_MAX - COL_PRIMERA_TEXTO);
    }
    memmove(buf + COL_PRIMERA_TEXTO, buf + k, n);
    buf[COL_PRIMERA_TEXTO + n] = '\0';
    for (k = i; k < COL_PRIMERA_TEXTO; k++) {
        buf[k] = ' ';
    }
    if (d != '\0') {
        buf[COL_ULTIMA_ETIQUETA] = d;   /* marcador, columna 6 */
    }
}

/* Deja en f->linea la linea num_linea (sin consumirla). 0 = no hay mas. */
static uint8_t pedir(Fuente *f)
{
    if (f->tiene) {
        return 1;
    }
    if (f->fin) {
        return 0;
    }
    if (!ed_linea(f->num_linea, f->linea)) {
        f->fin = 1;
        f->linea[0] = '\0';
        return 0;
    }
    f->linea[LINEA_MAX] = '\0';         /* defensa: el contrato da 72 + '\0' */
    normalizar_linea(f->linea);
    f->tiene = 1;
    return 1;
}

static void consumir(Fuente *f)
{
    f->tiene = 0;
    f->num_linea++;
}

ErrorFuente leer_sentencia(Fuente *f)
{
    TipoLinea tipo;
    ErrorFuente err;
    ErrorFuente e;
    uint16_t inicio;

    /* 1. Saltar comentarios y lineas en blanco hasta la primera linea INICIAL. */
    for (;;) {
        if (!pedir(f)) {
            return FUENTE_FIN;
        }
        tipo = clasificar_linea(f->linea);
        inicio = f->num_linea;
        consumir(f);

        if (tipo == LINEA_INICIAL) {
            break;
        }
        if (tipo == LINEA_CONTINUACION) {
            /* Continuacion "huerfana": no hay sentencia que continuar. */
            f->linea_error = inicio;
            return FUENTE_CONTINUACION_INVALIDA;
        }
    }

    err = iniciar_sentencia(f->linea, inicio);
    if (err != FUENTE_OK) {
        f->linea_error = inicio;
    }

    /* 2. Unir las continuaciones. Se MIRA la linea siguiente antes de
     *    consumirla: si resulta ser una sentencia nueva, no se consume y la
     *    leera la proxima llamada (queda en f->linea, f->tiene = 1). Los
     *    comentarios entre continuaciones son validos y se saltan. Tras un
     *    error se siguen CONSUMIENDO las continuaciones (sin anadirlas) para
     *    no dar errores falsos en cascada. */
    while (pedir(f)) {
        tipo = clasificar_linea(f->linea);
        if (tipo == LINEA_INICIAL) {
            break;
        }
        if (tipo == LINEA_CONTINUACION && err == FUENTE_OK) {
            e = anadir_continuacion(f->linea, f->num_linea);
            if (e != FUENTE_OK) {
                err = e;
                f->linea_error = f->num_linea;
            }
        }
        consumir(f);
    }

    return err;
}
