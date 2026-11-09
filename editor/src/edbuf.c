/* edbuf.c - Buffer de hueco sobre paginas de datos (ver edbuf.h). */
#include <string.h>
#include "edbuf.h"

BufTexto eb;

/* Buffer compartido de bloques: lo usan mover_hueco y los escaneos de lineas.
 * Ninguna funcion de este fichero se llama a si misma ni a otra que lo use
 * mientras lo tiene ocupado, asi que basta uno. */
static uint8_t tmp[ED_CHUNK];

/* ------------------------------------------------------------------
 * Acceso fisico: un vector de cap bytes repartido en paginas de 8 KB.
 * Cada llamada a banco_* toca UNA pagina, asi que se parte en el limite.
 * ------------------------------------------------------------------ */

static void fis_leer(uint16_t i, uint8_t *dst, uint16_t n)
{
    uint16_t off, k;

    while (n != 0) {
        off = i & 0x1FFF;
        k = 8192 - off;
        if (k > n) k = n;
        banco_leer((uint8_t)(ED_PAG_BASE + (i >> 13)), off, dst, k);
        i += k;
        dst += k;
        n -= k;
    }
}

static void fis_escribir(uint16_t i, const uint8_t *src, uint16_t n)
{
    uint16_t off, k;

    while (n != 0) {
        off = i & 0x1FFF;
        k = 8192 - off;
        if (k > n) k = n;
        banco_escribir((uint8_t)(ED_PAG_BASE + (i >> 13)), off, src, k);
        i += k;
        src += k;
        n -= k;
    }
}

/* ------------------------------------------------------------------
 * Hueco
 * ------------------------------------------------------------------ */

void eb_iniciar_cap(uint16_t cap)
{
    eb.cap = cap;
    eb.g_ini = 0;
    eb.g_len = cap;
    eb.ver++;
}

void eb_iniciar(void)
{
    eb_iniciar_cap(ED_CAP);
}

uint16_t eb_longitud(void)
{
    return eb.cap - eb.g_len;
}

uint16_t eb_libre(void)
{
    return eb.g_len;
}

/* Deja el hueco empezando en la posicion logica pos. Mueve el texto que hay
 * entre la posicion actual del hueco y pos, por bloques de ED_CHUNK.
 *
 * pos < g_ini: el texto [pos, g_ini) pasa a [pos+g_len, g_ini+g_len). Origen y
 *   destino pueden solaparse, y el destino esta MAS ARRIBA: se copia empezando
 *   por el bloque alto, asi nunca se pisa un byte que falte por leer.
 * pos > g_ini: el texto [g_ini+g_len, pos+g_len) pasa a [g_ini, pos). El
 *   destino esta MAS ABAJO: se copia empezando por el bloque bajo. */
static void mover_hueco(uint16_t pos)
{
    uint16_t n, k, src, dst;

    if (pos < eb.g_ini) {
        n = eb.g_ini - pos;
        while (n != 0) {
            k = (n > ED_CHUNK) ? ED_CHUNK : n;
            src = pos + n - k;
            fis_leer(src, tmp, k);
            fis_escribir(src + eb.g_len, tmp, k);
            n -= k;
        }
    } else if (pos > eb.g_ini) {
        n = pos - eb.g_ini;
        src = eb.g_ini + eb.g_len;
        dst = eb.g_ini;
        while (n != 0) {
            k = (n > ED_CHUNK) ? ED_CHUNK : n;
            fis_leer(src, tmp, k);
            fis_escribir(dst, tmp, k);
            src += k;
            dst += k;
            n -= k;
        }
    }
    eb.g_ini = pos;
}

uint8_t eb_insertar(uint16_t pos, uint8_t c)
{
    if (eb.g_len == 0 || pos > eb_longitud()) return 0;
    mover_hueco(pos);
    fis_escribir(eb.g_ini, &c, 1);
    eb.g_ini++;
    eb.g_len--;
    eb.ver++;
    return 1;
}

uint16_t eb_agregar(const uint8_t *src, uint16_t n)
{
    if (n > eb.g_len) n = eb.g_len;
    mover_hueco(eb_longitud());      /* no mueve nada si el hueco ya esta al final */
    fis_escribir(eb.g_ini, src, n);
    eb.g_ini += n;
    eb.g_len -= n;
    eb.ver++;
    return n;
}

uint8_t eb_borrar(uint16_t pos)
{
    if (pos >= eb_longitud()) return 0;
    mover_hueco(pos);
    eb.g_len++;          /* el byte en pos queda dentro del hueco */
    eb.ver++;
    return 1;
}

/* ------------------------------------------------------------------
 * Lectura logica
 * ------------------------------------------------------------------ */

void eb_leer(uint16_t pos, uint8_t *dst, uint16_t n)
{
    uint16_t a;

    if (pos < eb.g_ini) {
        a = eb.g_ini - pos;
        if (a > n) a = n;
        fis_leer(pos, dst, a);
        pos += a;
        dst += a;
        n -= a;
    }
    if (n != 0) fis_leer(pos + eb.g_len, dst, n);
}

uint8_t eb_byte(uint16_t pos)
{
    uint8_t c;

    eb_leer(pos, &c, 1);
    return c;
}

/* ------------------------------------------------------------------
 * Lineas
 * ------------------------------------------------------------------ */

uint16_t eb_fin_linea(uint16_t pos)
{
    uint16_t len, k;
    uint8_t *p;

    len = eb_longitud();
    while (pos < len) {
        k = len - pos;
        if (k > ED_CHUNK) k = ED_CHUNK;
        eb_leer(pos, tmp, k);
        p = (uint8_t *)memchr(tmp, '\n', k);
        if (p != 0) return pos + (uint16_t)(p - tmp);
        pos += k;
    }
    return len;
}

uint16_t eb_ini_linea(uint16_t pos)
{
    uint16_t k, i;

    while (pos > 0) {
        k = (pos > ED_CHUNK) ? ED_CHUNK : pos;
        eb_leer(pos - k, tmp, k);
        i = k;
        while (i > 0) {
            i--;
            if (tmp[i] == '\n') return pos - k + i + 1;
        }
        pos -= k;
    }
    return 0;
}

uint16_t eb_contar_nl(uint16_t pos)
{
    uint16_t p, k, i, cuenta;

    cuenta = 0;
    p = 0;
    while (p < pos) {
        k = pos - p;
        if (k > ED_CHUNK) k = ED_CHUNK;
        eb_leer(p, tmp, k);
        for (i = 0; i < k; i++) {
            if (tmp[i] == '\n') cuenta++;
        }
        p += k;
    }
    return cuenta;
}
