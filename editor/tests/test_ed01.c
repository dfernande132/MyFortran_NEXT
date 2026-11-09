/* test_ed01.c - Pruebas del editor, paso 1: buffer de hueco, logica de edicion y pintado.
 *
 * Tres bloques:
 *   1. Buffer (edbuf.c) contra un modelo ingenuo (vector + memmove), con
 *      operaciones aleatorias que cruzan paginas de 8 KB y bloques de 64 bytes.
 *   2. Editor (edit.c): escenarios con resultado exacto conocido.
 *   3. Editor contra un pintor ingenuo: miles de teclas aleatorias; despues de
 *      cada una se comprueban las invariantes (ed_verificar), la pantalla
 *      completa y la posicion del cursor.
 * La pantalla es falsa (abajo): no hay Next, pero SI se comprueba que nunca se
 * escribe en la ultima celda de la ultima fila ni un caracter no imprimible.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "banco.h"
#include "edbuf.h"
#include "ed_plat.h"
#include "ed_teclas.h"
#include "edit.h"
#include "ed_linea.h"
#include "mf_api.h"        /* el compilador SIMULADO de abajo implementa mf_compilar/mf_mensaje */

static int pruebas = 0;
static int fallos = 0;

#define COMPROBAR(cond)                                              \
    do {                                                             \
        pruebas++;                                                   \
        if (!(cond)) {                                               \
            fallos++;                                                \
            printf("FALLO (linea %d): %s\n", __LINE__, #cond);       \
        }                                                            \
    } while (0)

/* ------------------------------------------------------------------
 * Pantalla falsa (implementa ed_plat.h)
 * ------------------------------------------------------------------ */

static uint8_t pantalla[ED_FILAS][ED_ANCHO];
static uint8_t estilos[ED_FILAS][ED_ANCHO];
static int cur_f, cur_c;
static int violaciones;
static int escrituras[ED_FILAS];

void plat_iniciar(void)
{
    memset(pantalla, ' ', sizeof(pantalla));
    memset(estilos, 0, sizeof(estilos));
    memset(escrituras, 0, sizeof(escrituras));
    violaciones = 0;
    cur_f = cur_c = -1;
}

void plat_escribir(uint8_t fila, uint8_t col, const uint8_t *txt, const uint8_t *est, uint8_t n)
{
    uint8_t i;

    if (fila >= ED_FILAS || col + n > ED_ANCHO) {
        violaciones++;
        return;
    }
    for (i = 0; i < n; i++) {
        if (txt[i] < 32 || txt[i] > 126) violaciones++;
        if (est[i] >= ED_N_ESTILOS) violaciones++;
        pantalla[fila][col + i] = txt[i];
        estilos[fila][col + i] = est[i];
    }
    escrituras[fila]++;
}

void plat_cursor(uint8_t fila, uint8_t col)
{
    cur_f = fila;
    cur_c = col;
}

int plat_tecla(void)
{
    return -1;
}

/* ------------------------------------------------------------------
 * Ficheros falsos (implementa la parte de ficheros de ed_plat.h)
 * ------------------------------------------------------------------ */

static const uint8_t *fich_datos;
static uint32_t fich_len, fich_pos;
static int fich_abrir_err;        /* 0 = se abre; otro valor = se devuelve -valor */
static uint32_t fich_fallo_en;    /* 0 = nunca; si no, leer falla al llegar a esa posicion */
static uint16_t fich_trozo;       /* 0 = sin limite; si no, lecturas cortas de ese maximo */
static int fich_abiertos;

static char fich_nom_lect[64];    /* ultimo nombre pedido para leer */
static int fich_abrir_n;          /* veces que se ha intentado abrir para leer */

int plat_fich_abrir_lectura(const char *nombre)
{
    strncpy(fich_nom_lect, nombre, sizeof(fich_nom_lect) - 1);
    fich_abrir_n++;
    if (fich_abrir_err != 0) return -fich_abrir_err;
    fich_pos = 0;
    fich_abiertos++;
    return 3;
}

int plat_fich_leer(int h, uint8_t *dst, uint16_t n)
{
    uint32_t resta;

    if (h != 3) return -1;
    if (fich_fallo_en != 0 && fich_pos >= fich_fallo_en) return -1;
    resta = fich_len - fich_pos;
    if (fich_fallo_en != 0 && fich_fallo_en - fich_pos < resta) resta = fich_fallo_en - fich_pos;
    if (n > resta) n = (uint16_t)resta;
    if (fich_trozo != 0 && n > fich_trozo) n = fich_trozo;
    memcpy(dst, fich_datos + fich_pos, n);
    fich_pos += n;
    return n;
}

/* Escritura: lo escrito queda en esc_datos */
static char fich_nom_esc[64];     /* ultimo nombre pedido para escribir */
static int fich_crear_err;        /* 0 = se crea; otro valor = se devuelve -valor */
static uint32_t fich_esc_limite;  /* 0 = sin limite; si no, no caben mas bytes que estos (escritura corta) */
static int fich_crear_n;          /* veces que se ha creado un fichero */
static int esc_abiertos;
static uint8_t esc_datos[ED_CAP + 64];
static uint32_t esc_len;

int plat_fich_abrir_escritura(const char *nombre)
{
    if (fich_crear_err != 0) return -fich_crear_err;
    strncpy(fich_nom_esc, nombre, sizeof(fich_nom_esc) - 1);
    fich_crear_n++;
    esc_abiertos++;
    esc_len = 0;
    return 4;
}

int plat_fich_escribir(int h, const uint8_t *src, uint16_t n)
{
    if (h != 4) return -1;
    if (fich_esc_limite != 0 && esc_len + n > fich_esc_limite) n = (uint16_t)(fich_esc_limite - esc_len);
    memcpy(esc_datos + esc_len, src, n);
    esc_len += n;
    return n;
}

void plat_fich_cerrar(int h)
{
    if (h == 3) fich_abiertos--;
    if (h == 4) esc_abiertos--;
}

static void fich_poner(const uint8_t *d, uint32_t len)
{
    fich_datos = d;
    fich_len = len;
    fich_abrir_err = 0;
    fich_fallo_en = 0;
    fich_trozo = 0;
}

/* ------------------------------------------------------------------
 * Utilidades
 * ------------------------------------------------------------------ */

static uint32_t semilla = 12345;

static uint32_t rnd(uint32_t n)
{
    semilla = semilla * 1664525u + 1013904223u;
    return (semilla >> 8) % n;
}

static void preparar(void)
{
    banco_iniciar();
    plat_iniciar();
    ed_iniciar();
}

static void teclas(const char *s)
{
    while (*s != 0) ed_tecla((unsigned char)*s++);
}

/* Fila de pantalla como cadena (sin el final de blancos) */
static void fila_txt(int f, char *dst)
{
    int n = ED_ANCHO;

    memcpy(dst, pantalla[f], ED_ANCHO);
    while (n > 0 && dst[n - 1] == ' ') n--;
    dst[n] = 0;
}


/* ------------------------------------------------------------------
 * Compilador SIMULADO (mf_api.h): el editor lo llama con Esc+C
 * ------------------------------------------------------------------ */

Mf mf;
static uint16_t fk_n;                 /* errores que "encuentra" */
static MfDiag fk_diag[MF_DIAG_MAX];
static int fk_llamadas;
static int fk_lineas;                 /* lineas que ha leido con ed_linea en la ultima llamada */
static uint32_t fk_suma;              /* suma de comprobacion de lo leido */
static char fk_pantalla[ED_ANCHO + 1];/* fila de mensajes tal como estaba al llamar */
static int fk_aleatorio;              /* 1 = inventa los errores a partir del texto */

static const MfSalida *fk_salida;     /* lo registrado con mf_salida */
static int fk_reg_en_llamada;         /* habia salida registrada dentro de mf_compilar */
static int fk_reg_n;                  /* llamadas a mf_salida */
static int fk_no_gen;                 /* 1 = no genera aunque haya salida */
static int fk_abrir_falla;            /* 1 = el compilador simula que abrir devuelve 0 */

void mf_salida(const MfSalida *s)
{
    fk_salida = s;
    fk_reg_n++;
}

uint16_t mf_compilar(void)
{
    char dst[73];
    int n, i;

    fk_reg_en_llamada = (fk_salida != 0);

    fk_llamadas++;
    memcpy(fk_pantalla, pantalla[ED_FILA_MSG], ED_ANCHO);
    fk_pantalla[ED_ANCHO] = 0;
    fk_lineas = 0;
    fk_suma = 0;
    for (n = 1; ed_linea((uint16_t)n, dst); n++) {
        fk_lineas++;
        for (i = 0; dst[i] != 0; i++) fk_suma += (uint8_t)dst[i];
    }
    if (fk_aleatorio) {
        uint16_t len = eb_longitud();
        fk_n = (uint16_t)(len % 7);
        for (i = 0; i < MF_DIAG_MAX; i++) {
            fk_diag[i].linea = (uint16_t)((i * 13 + len) % (fk_lineas + 3) + 1);
            fk_diag[i].columna = (uint8_t)((i * 31 + len) % 90 + 1);
            fk_diag[i].cod = (uint8_t)(i % 5);
        }
    }
    mf.n = fk_n;
    for (i = 0; i < MF_DIAG_MAX; i++) mf.diag[i] = fk_diag[i];
    mf.gen = MF_GEN_NO;
    mf.gen_cod = 0;
    if (mf.n == 0 && fk_salida != 0 && !fk_no_gen) {                 /* "construir": 264 trozos de 64 B, como el .nex real */
        uint8_t buf[64], ok = 1;
        int k;

        if (fk_abrir_falla || !fk_salida->abrir()) {
            mf.gen = MF_GEN_ERR;
            mf.gen_cod = MF_COD_GEN + 5;
        } else {
            for (k = 0; k < 264 && ok; k++) {
                memset(buf, k & 0xFF, sizeof(buf));
                ok = fk_salida->escribir(buf, 64);
            }
            ok = fk_salida->cerrar(ok);
            mf.gen = ok ? MF_GEN_OK : MF_GEN_ERR;
            if (!ok) mf.gen_cod = MF_COD_GEN + 7;
        }
    }
    return mf.n;
}

void mf_mensaje(uint8_t cod, char *dst)
{
    snprintf(dst, MF_MENSAJE_MAX, "msg %u", (unsigned)cod);
    if (cod == 8) strcpy(dst, "wrong number of subscripts");
    if (cod == MF_COD_GEN + 7) strcpy(dst, "cannot write file");
    if (cod == MF_COD_GEN + 5) strcpy(dst, "cannot create file");
}

/* ------------------------------------------------------------------
 * 1. Buffer contra modelo
 * ------------------------------------------------------------------ */

static uint8_t ref[ED_CAP + 1];
static uint16_t rlen;

static uint16_t ref_fin(uint16_t pos)
{
    while (pos < rlen && ref[pos] != '\n') pos++;
    return pos;
}

static uint16_t ref_ini(uint16_t pos)
{
    while (pos > 0 && ref[pos - 1] != '\n') pos--;
    return pos;
}

static uint16_t ref_nl(uint16_t pos)
{
    uint16_t i, c = 0;

    for (i = 0; i < pos; i++) if (ref[i] == '\n') c++;
    return c;
}

static int igual_modelo(void)
{
    static uint8_t t[ED_CAP + 1];
    uint16_t p, k;

    if (eb_longitud() != rlen) return 0;
    if (eb_libre() != (uint16_t)(eb.cap - rlen)) return 0;
    p = 0;
    while (p < rlen) {
        k = (uint16_t)(1 + rnd(300));
        if (k > rlen - p) k = rlen - p;
        eb_leer(p, t + p, k);
        p += k;
    }
    return memcmp(t, ref, rlen) == 0;
}

static void ref_insertar(uint16_t pos, uint8_t c)
{
    memmove(ref + pos + 1, ref + pos, rlen - pos);
    ref[pos] = c;
    rlen++;
}

static void ref_borrar(uint16_t pos)
{
    memmove(ref + pos, ref + pos + 1, rlen - pos - 1);
    rlen--;
}

static void test_buffer_basico(void)
{
    uint8_t b[8];

    banco_iniciar();
    eb_iniciar();
    COMPROBAR(eb_longitud() == 0);
    COMPROBAR(eb_libre() == ED_CAP);
    COMPROBAR(eb_insertar(1, 'x') == 0);            /* pos > longitud */
    COMPROBAR(eb_borrar(0) == 0);                   /* vacio */
    COMPROBAR(eb_insertar(0, 'B') == 1);
    COMPROBAR(eb_insertar(0, 'A') == 1);
    COMPROBAR(eb_insertar(2, 'D') == 1);
    COMPROBAR(eb_insertar(2, 'C') == 1);
    eb_leer(0, b, 4);
    COMPROBAR(memcmp(b, "ABCD", 4) == 0);
    COMPROBAR(eb_byte(2) == 'C');
    COMPROBAR(eb_borrar(1) == 1);                   /* quita B */
    eb_leer(0, b, 3);
    COMPROBAR(memcmp(b, "ACD", 3) == 0);
    COMPROBAR(eb_borrar(3) == 0);                   /* pos == longitud */
    COMPROBAR(eb_longitud() == 3);

    /* Lleno */
    eb_iniciar_cap(5);
    COMPROBAR(eb_insertar(0, '1') && eb_insertar(1, '2') && eb_insertar(2, '3')
              && eb_insertar(3, '4') && eb_insertar(4, '5'));
    COMPROBAR(eb_libre() == 0);
    COMPROBAR(eb_insertar(5, '6') == 0);
    COMPROBAR(eb_borrar(2) == 1);
    COMPROBAR(eb_insertar(0, 'z') == 1);
    eb_leer(0, b, 5);
    COMPROBAR(memcmp(b, "z1245", 5) == 0);
    eb_iniciar();
}

/* Relleno hasta el limite exacto de la capacidad, cruzando las 4 paginas */
static void test_buffer_lleno_total(void)
{
    uint16_t i;
    int ok = 1;

    banco_iniciar();
    eb_iniciar();
    for (i = 0; i < ED_CAP; i++) {
        if (!eb_insertar(i, (uint8_t)(i * 7 + (i >> 8)))) { ok = 0; break; }
    }
    COMPROBAR(ok);
    COMPROBAR(eb_longitud() == ED_CAP);
    COMPROBAR(eb_libre() == 0);
    COMPROBAR(eb_insertar(0, 'x') == 0);
    for (i = 0; i < ED_CAP; i += 251) {
        if (eb_byte(i) != (uint8_t)(i * 7 + (i >> 8))) ok = 0;
    }
    COMPROBAR(ok);
    /* Vaciarlo desde el principio (mueve el hueco lo minimo) */
    for (i = 0; i < ED_CAP; i++) {
        if (!eb_borrar(0)) { ok = 0; break; }
    }
    COMPROBAR(ok && eb_longitud() == 0);
}

static void test_buffer_aleatorio(uint32_t sem, uint16_t pnl, int ops)
{
    int i, ok = 1;
    uint16_t pos, cursor = 0, q;
    uint8_t c;

    semilla = sem;
    banco_iniciar();
    eb_iniciar();
    rlen = 0;
    for (i = 0; i < ops && ok; i++) {
        /* Primera parte: crece (cruza paginas); despues, equilibrado. */
        int p_ins = (i < ops / 2) ? 75 : 50;
        /* Escritura "tecleando" (cerca del ultimo sitio) o salto lejano */
        if (rnd(10) < 7) {
            pos = cursor;
        } else {
            pos = (uint16_t)rnd((uint32_t)rlen + 1);
        }
        if (pos > rlen) pos = rlen;
        if ((int)rnd(100) < p_ins) {
            c = (rnd(pnl) == 0) ? '\n' : (uint8_t)('a' + rnd(26));
            if (rlen < eb.cap) {
                ok &= eb_insertar(pos, c) == 1;
                ref_insertar(pos, c);
                cursor = pos + 1;
            } else {
                ok &= eb_insertar(pos, c) == 0;
            }
        } else if (rlen > 0) {
            if (pos == rlen) pos--;
            ok &= eb_borrar(pos) == 1;
            ref_borrar(pos);
            cursor = pos;
        }
        if (i % 97 == 0) {
            ok &= igual_modelo();
            q = (uint16_t)rnd((uint32_t)rlen + 1);
            ok &= eb_fin_linea(q) == ref_fin(q);
            ok &= eb_ini_linea(q) == ref_ini(q);
            if (i % 1941 == 0) ok &= eb_contar_nl(q) == ref_nl(q);
        }
    }
    COMPROBAR(ok);
    COMPROBAR(igual_modelo());
    /* Consultas de linea en TODAS las posiciones de una muestra */
    for (q = 0; q <= rlen; q += (uint16_t)(1 + rnd(37))) {
        if (eb_fin_linea(q) != ref_fin(q) || eb_ini_linea(q) != ref_ini(q)) ok = 0;
    }
    COMPROBAR(ok);
    COMPROBAR(eb_contar_nl(rlen) == ref_nl(rlen));
}

/* Una linea larga (varios bloques y paginas, sin '\n') con el hueco en medio */
static void test_buffer_linea_larga(void)
{
    uint16_t i;
    int ok = 1;

    banco_iniciar();
    eb_iniciar();
    rlen = 0;
    for (i = 0; i < 20000; i++) {
        ref_insertar(i, (uint8_t)('a' + i % 26));
        ok &= eb_insertar(i, (uint8_t)('a' + i % 26));
    }
    COMPROBAR(ok);
    /* hueco en 7000, en 8192 (limite de pagina), en 100 */
    eb_insertar(7000, '\n'); ref_insertar(7000, '\n');
    COMPROBAR(eb_fin_linea(0) == 7000 && eb_ini_linea(7000) == 0);
    COMPROBAR(eb_ini_linea(19999) == 7001 && eb_fin_linea(7001) == 20001);
    eb_insertar(8192, '\n'); ref_insertar(8192, '\n');
    COMPROBAR(eb_fin_linea(7001) == 8192 && eb_ini_linea(8193) == 8193);
    COMPROBAR(eb_contar_nl(20002) == 2);
    COMPROBAR(igual_modelo());
    eb_insertar(100, '\n'); ref_insertar(100, '\n');
    COMPROBAR(eb_fin_linea(5) == 100);
    COMPROBAR(eb_ini_linea(6000) == 101);
    COMPROBAR(igual_modelo());
}

/* ------------------------------------------------------------------
 * 2. Editor: escenarios exactos
 * ------------------------------------------------------------------ */

static void ed_refrescar_ok(void)
{
    ed_refrescar();
    COMPROBAR(ed_verificar() == 0);
    COMPROBAR(violaciones == 0);
}

static void test_escribir(void)
{
    char f[ED_ANCHO + 1];

    preparar();
    ed_refrescar_ok();
    COMPROBAR(cur_f == 1 && cur_c == 0);
    teclas("AB");
    ed_refrescar_ok();
    fila_txt(1, f);
    COMPROBAR(strcmp(f, "AB") == 0);
    COMPROBAR(ed.pos == 2 && ed.linea == 0 && ed.modificado == 1);
    COMPROBAR(cur_f == 1 && cur_c == 2);

    ed_tecla(ED_K_ENTER);
    teclas("C");
    ed_refrescar_ok();
    fila_txt(1, f); COMPROBAR(strcmp(f, "AB") == 0);
    fila_txt(2, f); COMPROBAR(strcmp(f, "C") == 0);
    COMPROBAR(ed.linea == 1 && ed.ini_lin == 3 && cur_f == 2 && cur_c == 1);

    ed_tecla(ED_K_BORRAR);                         /* quita C */
    ed_refrescar_ok();
    fila_txt(2, f); COMPROBAR(f[0] == 0);
    ed_tecla(ED_K_BORRAR);                         /* une con la anterior */
    ed_refrescar_ok();
    COMPROBAR(eb_longitud() == 2 && ed.linea == 0 && ed.pos == 2 && ed.ini_lin == 0);
    fila_txt(1, f); COMPROBAR(strcmp(f, "AB") == 0);
    ed_tecla(ED_K_BORRAR);
    ed_tecla(ED_K_BORRAR);
    ed_tecla(ED_K_BORRAR);                         /* ya vacio: no pasa nada */
    ed_refrescar_ok();
    COMPROBAR(eb_longitud() == 0 && ed.pos == 0);
}

static void test_cursor(void)
{
    preparar();
    ed_insertar_texto("ABCDEF\nAB\nABCDEF");
    ed_refrescar_ok();
    COMPROBAR(ed.linea == 2 && ed.pos == 6 + 1 + 2 + 1 + 6);
    ed_tecla(ED_K_ARR);                            /* a la linea corta */
    COMPROBAR(ed.linea == 1 && ed.pos == 7 + 2);   /* final de "AB" */
    ed_tecla(ED_K_ARR);                            /* la columna deseada (6) se recupera */
    COMPROBAR(ed.linea == 0 && ed.pos == 6);
    ed_tecla(ED_K_ARR);                            /* ya en la primera linea: nada */
    COMPROBAR(ed.linea == 0 && ed.pos == 6);
    ed_tecla(ED_K_ABA);
    ed_tecla(ED_K_ABA);
    COMPROBAR(ed.linea == 2 && ed.pos == 16);
    ed_tecla(ED_K_ABA);                            /* ultima linea: nada */
    COMPROBAR(ed.linea == 2 && ed.pos == 16);

    ed_tecla(ED_K_MENU); ed_tecla('t');            /* inicio del texto */
    COMPROBAR(ed.pos == 0 && ed.linea == 0);
    ed_tecla(ED_K_IZQ);                            /* ya en el inicio */
    COMPROBAR(ed.pos == 0);
    ed_tecla(ED_K_MENU); ed_tecla('E');            /* fin de linea */
    COMPROBAR(ed.pos == 6);
    ed_tecla(ED_K_DER);                            /* salta al principio de la siguiente */
    COMPROBAR(ed.pos == 7 && ed.linea == 1 && ed.ini_lin == 7);
    ed_tecla(ED_K_IZQ);                            /* vuelve al final de la anterior */
    COMPROBAR(ed.pos == 6 && ed.linea == 0 && ed.ini_lin == 0);
    ed_tecla(ED_K_MENU); ed_tecla('B');            /* fin del texto */
    COMPROBAR(ed.pos == 16 && ed.linea == 2 && ed.ini_lin == 10);
    ed_tecla(ED_K_MENU); ed_tecla('H');
    COMPROBAR(ed.pos == 10);
    ed_tecla(ED_K_DER);
    ed_tecla(ED_K_MENU); ed_tecla('7');            /* plan B: 7 = arriba */
    COMPROBAR(ed.linea == 1 && ed.pos == 7 + 1);
    ed_tecla(ED_K_MENU); ed_tecla('6');
    COMPROBAR(ed.linea == 2 && ed.pos == 11);
    ed_tecla(ED_K_MENU); ed_tecla('5');
    ed_tecla(ED_K_MENU); ed_tecla('8');
    COMPROBAR(ed.pos == 11);
    ed_refrescar_ok();
}

static void test_comando_desconocido(void)
{
    preparar();
    ed_tecla(ED_K_MENU);
    COMPROBAR(ed.modo == 1);
    ed_tecla('Z');
    COMPROBAR(ed.modo == 0 && ed.msg != 0);
    ed_refrescar_ok();
    ed_tecla('a');
    COMPROBAR(ed.msg == 0 && eb_longitud() == 1);
    ed_tecla(27);                                   /* Esc tambien abre el modo comando */
    COMPROBAR(ed.modo == 1);
    ed_tecla(27);                                   /* en modo comando, Esc cancela sin error */
    COMPROBAR(ed.modo == 0 && ed.msg == 0 && eb_longitud() == 1);
}

static void test_limite_columna(void)
{
    int i;
    char f[ED_ANCHO + 1];

    preparar();
    for (i = 0; i < 80; i++) ed_tecla('x');
    ed_refrescar_ok();
    COMPROBAR(ed.pos == ED_MAXCOL && eb_longitud() == ED_MAXCOL && ed.msg != 0);
    /* la linea (72) cabe en las 80 columnas: no hay desplazamiento horizontal */
    COMPROBAR(ed.izq == 0 && cur_f == 1 && cur_c == ED_MAXCOL);
    fila_txt(1, f);
    COMPROBAR((int)strlen(f) == ED_MAXCOL);
    ed_tecla(ED_K_ENTER);                           /* en otra linea se puede seguir */
    ed_tecla('y');
    ed_refrescar_ok();
    COMPROBAR(ed.pos == ED_MAXCOL + 2 && ed.izq == 0);
    fila_txt(1, f);
    COMPROBAR((int)strlen(f) == ED_MAXCOL && f[0] == 'x');
}

/* Resaltado: casos concretos escritos a mano (independientes del clasificador de referencia) */
static void test_resaltado(void)
{
    static const char *const lineas[] = {
        "C     COMENTARIO DO 10 'X'",
        "   10 CONTINUE",
        "      DO 10 I = 1, N",
        "     +  X = 2.5E3 + A1",
        "      PRINT *, 'SI ES DO', I",
        "      end if",
        "      ENDIF",
        "      ENDX = 1"
    };
    /* estilos esperados, un digito por columna: 0 texto 1 comentario 2 etiqueta
     * 3 continuacion 4 clave 5 cadena 6 numero (ed_plat.h) */
    static const char *const esperado[] = {
        "111111111111111111111111",
        "00022044444444",
        "00000044066000006000",
        "000003000000606" "0000000",
        "000000" "44444" "0000" "5555555555" "000",
        "000000444" "0" "44",
        "000000" "44444",
        "0000000000000" "6"
    };
    int i, j, ok;
    char f[ED_ANCHO + 1];

    for (i = 0; i < (int)(sizeof(lineas) / sizeof(lineas[0])); i++) {
        preparar();
        ed_insertar_texto(lineas[i]);
        ed_tecla(ED_K_MENU); ed_tecla('T');
        ed_refrescar_ok();
        ok = 1;
        for (j = 0; esperado[i][j] != 0; j++) {
            if (esperado[i][j] == ' ') continue;
            if (estilos[1][j] != esperado[i][j] - '0') {
                printf("  linea %d columna %d: estilo %d, esperado %c\n", i, j, estilos[1][j], esperado[i][j]);
                ok = 0;
            }
        }
        COMPROBAR(ok);
    }
    fila_txt(1, f);

    /* columnas 73 en adelante (indices 72..): gris tenue, aunque parezcan palabras clave */
    {
        static uint8_t g[100];
        memset(g, ' ', 80);
        memcpy(g + 6, "END", 3);
        memcpy(g + 70, "XYZ", 3);          /* indices 70, 71 | 72 */
        memcpy(g + 74, "DO", 2);
        g[80] = '\n';
        preparar();
        fich_poner(g, 81);
        COMPROBAR(ed_cargar("f.f") == 0);
        ed_refrescar_ok();
        COMPROBAR(estilos[1][71] == ED_E_TEXTO && estilos[1][72] == ED_E_FUERA);
        COMPROBAR(estilos[1][74] == ED_E_FUERA && estilos[1][75] == ED_E_FUERA);
        COMPROBAR(estilos[1][6] == ED_E_CLAVE && estilos[1][8] == ED_E_CLAVE);
    }
}

/* Una linea cargada de 100 caracteres (mas larga que la pantalla) desplaza la vista */
static void test_scroll_horizontal(void)
{
    static uint8_t g[102];
    char f[ED_ANCHO + 1];
    int i;

    for (i = 0; i < 100; i++) g[i] = (uint8_t)('a' + (i % 26));
    g[100] = '\n';
    g[101] = 'z';
    preparar();
    fich_poner(g, 102);
    COMPROBAR(ed_cargar("l.f") == 0);
    ed_refrescar_ok();
    COMPROBAR(ed.izq == 0);
    fila_txt(1, f);
    COMPROBAR((int)strlen(f) == ED_ANCHO && f[0] == 'a');
    ed_tecla(ED_K_MENU); ed_tecla('E');
    ed_refrescar_ok();
    COMPROBAR(ed.pos == 100 && ed.izq == 100 + 9 - ED_ANCHO);
    COMPROBAR(cur_f == 1 && cur_c == 100 - (int)ed.izq);
    fila_txt(1, f);
    COMPROBAR(f[0] == (char)('a' + (ed.izq % 26)));
    /* la columna absoluta 72 (indice) cae en la columna de pantalla 72 - izq: gris tenue */
    COMPROBAR(estilos[1][ED_MAXCOL - 1 - ed.izq] != ED_E_FUERA);
    COMPROBAR(estilos[1][ED_MAXCOL - ed.izq] == ED_E_FUERA);
    ed_tecla(ED_K_MENU); ed_tecla('H');
    ed_refrescar_ok();
    COMPROBAR(ed.izq == 0);
}

static void test_scroll_vertical(void)
{
    int i;
    char f[ED_ANCHO + 1];

    preparar();
    for (i = 0; i < 60; i++) {
        teclas("L");
        ed_tecla((i / 10) + '0');
        ed_tecla((i % 10) + '0');
        ed_tecla(ED_K_ENTER);
        ed_refrescar();                                          /* como el bucle real: repinta tras cada tecla */
    }
    ed_refrescar_ok();
    COMPROBAR(ed.linea == 60);
    COMPROBAR(ed.top_linea == 60 - ED_FILAS_TEXTO + 1);          /* ha ido desplazando de una en una */
    COMPROBAR(cur_f == ED_FILAS_TEXTO);
    fila_txt(1, f); COMPROBAR(strcmp(f, "L32") == 0);
    fila_txt(ED_FILAS_TEXTO - 1, f); COMPROBAR(strcmp(f, "L59") == 0);
    ed_tecla(ED_K_MENU); ed_tecla('T');
    ed_refrescar_ok();
    COMPROBAR(ed.top_linea == 0 && ed.linea == 0 && cur_f == 1);
    fila_txt(1, f); COMPROBAR(strcmp(f, "L00") == 0);
    ed_tecla(ED_K_MENU); ed_tecla('D');                         /* pagina abajo: 21 lineas */
    ed_refrescar_ok();
    COMPROBAR(ed.linea == ED_FILAS_TEXTO - 1);
    COMPROBAR(ed.top_linea == 0);
    ed_tecla(ED_K_ABA);                                          /* una mas: desplaza una fila */
    ed_refrescar_ok();
    COMPROBAR(ed.top_linea == 1);
    ed_tecla(ED_K_MENU); ed_tecla('D');
    ed_tecla(ED_K_MENU); ed_tecla('D');
    ed_refrescar_ok();
    ed_tecla(ED_K_MENU); ed_tecla('U');
    ed_refrescar_ok();
    ed_tecla(ED_K_MENU); ed_tecla('B');                         /* salto largo: centra */
    ed_refrescar_ok();
    COMPROBAR(ed.linea == 60);
    ed_tecla(ED_K_MENU); ed_tecla('T');
    ed_tecla(ED_K_DER); ed_tecla(ED_K_DER);
    ed_refrescar_ok();
}

static void test_caracteres_raros(void)
{
    char f[ED_ANCHO + 1];

    preparar();
    eb_insertar(0, 'A');
    eb_insertar(1, 9);                               /* tabulador cargado de un fichero */
    eb_insertar(2, 200);
    eb_insertar(3, 'B');
    ed.sucio_todo = 1;
    ed_refrescar_ok();
    fila_txt(1, f);
    COMPROBAR(strcmp(f, "A..B") == 0);
}

static void test_estado(void)
{
    char f[ED_ANCHO + 1];
    int i, ok;

    preparar();
    teclas("AB");
    ed_tecla(ED_K_ENTER);
    ed_tecla(ED_K_IZQ);
    ed_refrescar_ok();
    fila_txt(0, f);
    COMPROBAR(strncmp(f, " MYFORTRAN ", 11) == 0);
    COMPROBAR(strncmp(f + 40, "L:1 C:3  Free ", 14) == 0);
    COMPROBAR(strstr(f, "  Key 08") != 0);
    COMPROBAR(strstr(f, "  *") != 0);                 /* modificado */
    ed_tecla(ED_K_MENU);
    ed_refrescar_ok();
    fila_txt(0, f);
    COMPROBAR(strstr(f, "  CMD") != 0);
    fila_txt(ED_FILA_AYUDA, f);
    COMPROBAR(strstr(f, "CMD:") != 0 && strstr(f, "U/D") != 0 && (int)strlen(f) <= ED_ANCHO);
    fila_txt(ED_FILA_MSG, f);                           /* segunda fila de ayuda */
    COMPROBAR(strstr(f, "save as") != 0 && (int)strlen(f) <= ED_ANCHO);
    ed_tecla('H');
    ed_refrescar_ok();
    fila_txt(ED_FILA_AYUDA, f);
    COMPROBAR(strstr(f, "CMD:") == 0 && strstr(f, "U/D") != 0);

    /* estilos de las barras: toda la fila */
    ok = 1;
    for (i = 0; i < ED_ANCHO; i++) {
        if (estilos[0][i] != ED_E_ESTADO) ok = 0;
        if (estilos[ED_FILA_AYUDA][i] != ED_E_AYUDA) ok = 0;
        if (estilos[ED_FILA_MSG][i] != ED_E_TEXTO) ok = 0;
    }
    COMPROBAR(ok);

    /* un error pinta toda la fila de mensajes con el estilo de error; el siguiente teclazo lo quita */
    ed_tecla(ED_K_MENU); ed_tecla(ED_K_MENU2);          /* Esc dos veces: cancela, sin error */
    COMPROBAR(ed.modo == 0 && ed.msg == 0);
    ed_tecla(ED_K_MENU); ed_tecla(ED_K_MENU);
    COMPROBAR(ed.modo == 0 && ed.msg == 0);
    ed_tecla(ED_K_MENU); ed_tecla('Q');
    ed_refrescar_ok();
    fila_txt(ED_FILA_MSG, f);
    COMPROBAR(strstr(f, "Unknown command") != 0);
    ok = 1;
    for (i = 0; i < ED_ANCHO; i++) if (estilos[ED_FILA_MSG][i] != ED_E_ERROR) ok = 0;
    COMPROBAR(ok);
    ed_tecla('q');
    ed_refrescar_ok();
    ok = 1;
    for (i = 0; i < ED_ANCHO; i++) if (estilos[ED_FILA_MSG][i] != ED_E_TEXTO) ok = 0;
    COMPROBAR(ok);

    /* nombre del fichero cargado en la barra de estado; mensaje de carga normal, no error */
    {
        static const uint8_t d[] = "      END\n";
        preparar();
        fich_poner(d, sizeof(d) - 1);
        COMPROBAR(ed_cargar("prueba.f") == 0);
        ed_refrescar_ok();
        fila_txt(0, f);
        COMPROBAR(strncmp(f, " MYFORTRAN  prueba.f", 20) == 0);
        fila_txt(ED_FILA_MSG, f);
        COMPROBAR(strstr(f, "Loaded prueba.f") != 0);
        COMPROBAR(estilos[ED_FILA_MSG][1] == ED_E_TEXTO);
        /* y un fallo de apertura si es un error */
        fich_abrir_err = 5;
        COMPROBAR(ed_cargar("no.f") == 1);
        ed_refrescar_ok();
        COMPROBAR(estilos[ED_FILA_MSG][1] == ED_E_ERROR);
    }
}

/* Solo la fila del cursor se repinta al teclear un caracter */
static void test_repintado_minimo(void)
{
    int i, n;

    preparar();
    ed_insertar_texto("uno\ndos\ntres");
    ed_refrescar_ok();
    memset(escrituras, 0, sizeof(escrituras));
    ed_tecla('x');
    ed_refrescar_ok();
    n = 0;
    for (i = 1; i <= ED_FILAS_TEXTO; i++) n += escrituras[i];
    COMPROBAR(n == 1 && escrituras[3] == 1);
    memset(escrituras, 0, sizeof(escrituras));
    ed_tecla(ED_K_ARR);                             /* mover sin cambiar el texto: no repinta texto */
    ed_refrescar_ok();
    n = 0;
    for (i = 1; i <= ED_FILAS_TEXTO; i++) n += escrituras[i];
    COMPROBAR(n == 0);
}

/* ------------------------------------------------------------------
 * 2b. Carga de ficheros
 * ------------------------------------------------------------------ */

static uint8_t grande[ED_CAP + 200];

static void test_cargar_basico(void)
{
    static const char f[] = "C HOLA\r\n      END\r\n";
    uint8_t b[32];

    preparar();
    teclas("Z");
    fich_poner((const uint8_t *)f, sizeof(f) - 1);
    COMPROBAR(ed_cargar("f.f") == 0);
    COMPROBAR(eb_longitud() == 17);                  /* sin los 2 '\r' */
    eb_leer(0, b, 17);
    COMPROBAR(memcmp(b, "C HOLA\n      END\n", 17) == 0);
    COMPROBAR(ed.pos == 0 && ed.linea == 0 && ed.modificado == 0);
    COMPROBAR(ed.msg != 0 && strcmp(ed.msg, "Loaded f.f: 17 bytes") == 0);
    COMPROBAR(fich_abiertos == 0);
    ed_refrescar_ok();
    {
        char r[ED_ANCHO + 1];
        fila_txt(1, r); COMPROBAR(strcmp(r, "C HOLA") == 0);
        fila_txt(2, r); COMPROBAR(strcmp(r, "      END") == 0);
    }
    ed_tecla('x');                                   /* el mensaje desaparece al teclear */
    COMPROBAR(ed.msg == 0);
}

static void test_cargar_vacio_y_sin_final(void)
{
    static const char f[] = "A\nB";
    uint8_t b[4];

    preparar();
    fich_poner((const uint8_t *)f, 0);
    COMPROBAR(ed_cargar("v.f") == 0 && eb_longitud() == 0);
    ed_refrescar_ok();
    fich_poner((const uint8_t *)f, 3);               /* sin '\n' final */
    COMPROBAR(ed_cargar("v.f") == 0 && eb_longitud() == 3);
    eb_leer(0, b, 3);
    COMPROBAR(memcmp(b, "A\nB", 3) == 0);
    ed_refrescar_ok();
    ed_tecla(ED_K_MENU); ed_tecla('B');
    ed_tecla('!');
    ed_refrescar_ok();
    COMPROBAR(eb_longitud() == 4 && eb_byte(3) == '!');
}

static void test_cargar_error_apertura(void)
{
    static const char f[] = "NO";

    preparar();
    teclas("Z");
    fich_poner((const uint8_t *)f, 2);
    fich_abrir_err = 5;
    COMPROBAR(ed_cargar("falta.f") == 1);
    COMPROBAR(strcmp(ed.msg, "Cannot open falta.f (err 5)") == 0);
    COMPROBAR(eb_longitud() == 1 && eb_byte(0) == 'Z' && ed.pos == 1);   /* texto intacto */
    COMPROBAR(fich_abiertos == 0);
    ed_refrescar_ok();
}

static void test_cargar_grande(void)
{
    uint32_t i, n = 20000;
    static uint8_t t[ED_CAP];
    uint16_t trozos[3] = { 0, 7, 61 };
    int k, ok;

    for (i = 0; i < n; i++) {
        grande[i] = (i % 51 == 50) ? '\n' : (uint8_t)('a' + i % 26);
        if (i % 700 == 3) grande[i] = '\r';         /* '\r' sueltos: se descartan */
    }
    for (k = 0; k < 3; k++) {
        uint32_t esperados = 0, j;

        preparar();
        fich_poner(grande, n);
        fich_trozo = trozos[k];
        COMPROBAR(ed_cargar("g.f") == 0);
        for (j = 0; j < n; j++) if (grande[j] != '\r') t[esperados++] = grande[j];
        COMPROBAR(eb_longitud() == esperados);
        ok = 1;
        {
            uint8_t blq[64];
            uint32_t p = 0;
            while (p < esperados) {
                uint16_t c = (uint16_t)((esperados - p) < 64 ? (esperados - p) : 64);
                eb_leer((uint16_t)p, blq, c);
                if (memcmp(blq, t + p, c) != 0) { ok = 0; break; }
                p += c;
            }
        }
        COMPROBAR(ok);
        COMPROBAR(ed_verificar() == 0);
        ed_refrescar_ok();
        ed_tecla(ED_K_MENU); ed_tecla('B');
        ed_refrescar_ok();
        COMPROBAR(ed.linea == eb_contar_nl(eb_longitud()));
    }
}

static void test_cargar_no_cabe(void)
{
    uint32_t i;

    for (i = 0; i < sizeof(grande); i++) grande[i] = (i % 40 == 39) ? '\n' : 'q';
    preparar();
    fich_poner(grande, sizeof(grande));
    COMPROBAR(ed_cargar("g.f") == 3);
    COMPROBAR(eb_longitud() == ED_CAP && eb_libre() == 0);
    COMPROBAR(strcmp(ed.msg, "File too big: loaded 32768 bytes") == 0);
    COMPROBAR(fich_abiertos == 0);
    ed_refrescar_ok();
    ed_tecla('x');                                   /* lleno: aviso, sin romper nada */
    ed_refrescar_ok();
    COMPROBAR(eb_longitud() == ED_CAP);
}

static void test_cargar_fallo_lectura(void)
{
    static uint8_t d[300];
    int i;

    for (i = 0; i < 300; i++) d[i] = (i % 30 == 29) ? '\n' : 'k';
    preparar();
    fich_poner(d, 300);
    fich_fallo_en = 100;
    COMPROBAR(ed_cargar("r.f") == 2);
    COMPROBAR(eb_longitud() == 100);
    COMPROBAR(strcmp(ed.msg, "Read error after 100 bytes") == 0);
    COMPROBAR(fich_abiertos == 0);
    ed_refrescar_ok();
}

/* Tabuladores: paradas cada 3 columnas (0, 3, 6, 9...) */
static void test_tabuladores(void)
{
    static const char f[] = "\tX\n\t\tY\nAB\tC\n   \t\tD\n123456789\tE\r\n\t";
    static const char esperado[] =
        "   X\n"                    /* TAB en col 0 -> 3 blancos */
        "      Y\n"                 /* dos TAB -> col 6 (el codigo empieza en la columna 7) */
        "AB C\n"                    /* col 2 -> 3 */
        "         D\n"              /* col 3 -> 6 -> 9 */
        "123456789   E\n"           /* col 9 -> 12 */
        "   ";                       /* TAB final */
    uint8_t b[128];
    uint16_t n = (uint16_t)strlen(esperado);

    /* Teclado: la tecla Tab (en las pruebas ED_K_TAB se define con -D) */
    preparar();
    ed_tecla(ED_K_TAB);
    COMPROBAR(eb_longitud() == 3 && ed.pos == 3);
    ed_tecla(ED_K_TAB);
    COMPROBAR(eb_longitud() == 6 && ed.pos == 6);
    ed_tecla(ED_K_TAB);
    COMPROBAR(eb_longitud() == 9);                   /* 6 -> 9 */
    ed_tecla(ED_K_TAB);
    COMPROBAR(eb_longitud() == 12);
    ed_tecla('x');
    ed_tecla(ED_K_TAB);
    COMPROBAR(eb_longitud() == 15);                  /* 13 -> 15 */
    ed_refrescar_ok();

    /* En medio de una linea y con el limite de columnas */
    preparar();
    teclas("AB");
    ed_tecla(ED_K_TAB);
    COMPROBAR(ed.pos == 3);
    while (ed.pos < ED_MAXCOL - 1) ed_tecla('z');
    ed_tecla(ED_K_TAB);              /* col 71 -> 72: justo la parada (72 es multiplo de 3) */
    COMPROBAR(ed.pos == ED_MAXCOL && ed.msg == 0);
    ed_tecla(ED_K_TAB);              /* ya en el limite: se rechaza */
    ed_refrescar_ok();
    COMPROBAR(ed.pos == ED_MAXCOL && ed.msg != 0);

    /* Carga: los tabuladores se expanden igual (tambien entre dos lecturas) */
    for (n = 1; n <= 64; n += 21) {
        uint16_t m = (uint16_t)strlen(esperado);
        preparar();
        fich_poner((const uint8_t *)f, (uint32_t)sizeof(f) - 1);
        fich_trozo = n;
        COMPROBAR(ed_cargar("t.f") == 0);
        COMPROBAR(eb_longitud() == m);
        eb_leer(0, b, m);
        COMPROBAR(memcmp(b, esperado, m) == 0);
        ed_refrescar_ok();
    }
    /* Carga que no cabe en medio de un tabulador */
    {
        static uint8_t g[ED_CAP + 8];
        uint32_t i;
        for (i = 0; i < sizeof(g); i++) g[i] = '\t';
        preparar();
        fich_poner(g, sizeof(g));
        COMPROBAR(ed_cargar("t.f") == 3);
        COMPROBAR(eb_longitud() <= ED_CAP && eb_libre() < 3);
        ed_refrescar_ok();
    }
}

/* ------------------------------------------------------------------
 * 3. Editor contra pintor ingenuo (teclas aleatorias)
 * ------------------------------------------------------------------ */

static uint8_t todo[ED_CAP + 1];

/* Fila de pantalla esperada para la linea numero `l` del texto */
static void esperada(uint16_t len, uint16_t l, uint16_t izq, char *dst)
{
    uint16_t p = 0, n = 0, i, fin;

    while (n < l) {
        while (p < len && todo[p] != '\n') p++;
        if (p >= len) { memset(dst, ' ', ED_ANCHO); dst[ED_ANCHO] = 0; return; }   /* no existe */
        p++;
        n++;
    }
    fin = p;
    while (fin < len && todo[fin] != '\n') fin++;
    for (i = 0; i < ED_ANCHO; i++) {
        uint16_t q = p + izq + i;
        if (q < fin) dst[i] = (todo[q] >= 32 && todo[q] <= 126) ? (char)todo[q] : '.';
        else dst[i] = ' ';
    }
    dst[ED_ANCHO] = 0;
}

/* Clasificador de referencia: reescrito aparte, sobre la linea ENTERA y en columnas absolutas */
static const char *const kw_ref[] = {
    "PROGRAM", "SUBROUTINE", "FUNCTION", "END", "INTEGER", "REAL", "DOUBLE",
    "PRECISION", "LOGICAL", "CHARACTER", "COMPLEX", "DIMENSION", "COMMON",
    "PARAMETER", "DATA", "IF", "THEN", "ELSE", "ELSEIF", "ENDIF", "DO",
    "CONTINUE", "GOTO", "CALL", "RETURN", "STOP", "PRINT", "READ", "WRITE",
    "FORMAT", "OPEN", "CLOSE"
};

static void ref_estilos(const uint8_t *s, int n, uint8_t *e)
{
    int i, j, k, w, instr = 0;
    char pal[16];

    for (i = 0; i < n; i++) e[i] = ED_E_TEXTO;
    if (n > 0 && (s[0] == 'C' || s[0] == 'c' || s[0] == '*')) {
        for (i = 0; i < n; i++) e[i] = ED_E_COMENT;
        return;
    }
    for (i = 0; i < n && i < 5; i++) if (isdigit(s[i])) e[i] = ED_E_ETIQ;
    if (n > 5 && s[5] != ' ') e[5] = ED_E_CONT;
    for (i = 6; i < n; i++) {
        if (i >= ED_MAXCOL) { e[i] = ED_E_FUERA; continue; }
        if (s[i] == '\'') { e[i] = ED_E_CADENA; instr = !instr; continue; }
        if (instr) { e[i] = ED_E_CADENA; continue; }
        if (isalpha(s[i]) && (i == 0 || !isalpha(s[i - 1]))) {
            for (j = i; j < n && isalpha(s[j]); j++) { }
            w = j - i;
            if (w < (int)sizeof(pal)) {
                for (k = 0; k < w; k++) pal[k] = (char)toupper(s[i + k]);
                pal[w] = 0;
                for (k = 0; k < (int)(sizeof(kw_ref) / sizeof(kw_ref[0])); k++) {
                    if (strcmp(pal, kw_ref[k]) == 0) {
                        for (k = i; k < j && k < ED_MAXCOL; k++) e[k] = ED_E_CLAVE;
                        break;
                    }
                }
            }
        } else if (isdigit(s[i]) && !isalpha(s[i - 1])) {
            e[i] = ED_E_NUM;
        }
    }
}

static uint8_t lin_ref[4200], est_ref[4200];

/* Estilos de la fila de pantalla `f` (0..ED_FILAS_TEXTO-1) frente a la referencia */
static int estilos_fila_ok(uint16_t len, uint16_t l, int f)
{
    uint16_t p = 0, n = 0, fin, i;
    int nl, q;

    while (n < l) {
        while (p < len && todo[p] != '\n') p++;
        if (p >= len) {                                     /* la linea no existe: toda en blanco */
            for (i = 0; i < ED_ANCHO; i++) if (estilos[1 + f][i] != ED_E_TEXTO) return 0;
            return 1;
        }
        p++;
        n++;
    }
    fin = p;
    while (fin < len && todo[fin] != '\n') fin++;
    nl = fin - p;
    for (q = 0; q < nl; q++) lin_ref[q] = (todo[p + q] >= 32 && todo[p + q] <= 126) ? todo[p + q] : '.';
    ref_estilos(lin_ref, nl, est_ref);
    for (i = 0; i < ED_ANCHO; i++) {
        q = ed.izq + i;
        if (q >= nl) {
            if (estilos[1 + f][i] != ED_E_TEXTO) return 0;
        } else if (ed.izq == 0 && nl <= ED_ANCHO) {
            if (estilos[1 + f][i] != est_ref[q]) return 0;
        } else if (est_ref[q] == ED_E_COMENT || est_ref[q] == ED_E_FUERA) {
            /* con desplazamiento o linea cortada por el borde, solo lo que no depende del contexto */
            if (estilos[1 + f][i] != est_ref[q]) return 0;
        }
    }
    return 1;
}

static int pantalla_correcta(void)
{
    uint16_t len = eb_longitud();
    char esp[ED_ANCHO + 1], real[ED_ANCHO + 1], est[ED_ANCHO + 1];
    int f, i;
    uint8_t esperado;

    eb_leer(0, todo, len);
    for (f = 0; f < ED_FILAS_TEXTO; f++) {
        esperada(len, ed.top_linea + f, ed.izq, esp);
        memcpy(real, pantalla[1 + f], ED_ANCHO);
        real[ED_ANCHO] = 0;
        if (strcmp(esp, real) != 0) return 0;
        if (!estilos_fila_ok(len, ed.top_linea + f, f)) return 0;
    }
    if (cur_f != 1 + (int)(ed.linea - ed.top_linea)) return 0;
    if (cur_c != (int)(ed.pos - ed.ini_lin) - (int)ed.izq) return 0;
    snprintf(est, sizeof(est), " MYFORTRAN  %s", ed.nombre != 0 ? ed.nombre : "");
    if (strncmp((char *)pantalla[0], est, strlen(est)) != 0) return 0;
    snprintf(est, sizeof(est), "L:%u C:%u  Free %u",
             (unsigned)ed.linea + 1, (unsigned)(ed.pos - ed.ini_lin) + 1, (unsigned)eb_libre());
    if (strncmp((char *)pantalla[0] + 40, est, strlen(est)) != 0) return 0;
    /* estilos de las barras y de la fila de mensajes */
    esperado = (ed.msg != 0 && ed.msg_err) ? ED_E_ERROR : ED_E_TEXTO;
    for (i = 0; i < ED_ANCHO; i++) {
        if (estilos[0][i] != ED_E_ESTADO) return 0;
        if (estilos[ED_FILA_AYUDA][i] != ED_E_AYUDA) return 0;
        if (estilos[ED_FILA_MSG][i] != (ed.modo == 1 ? ED_E_AYUDA : esperado)) return 0;   /* modo comando: 2a fila de ayuda */
    }
    return 1;
}

static const int cursores[4] = { ED_K_IZQ, ED_K_DER, ED_K_ARR, ED_K_ABA };
static const char cmds[] = "HETBUDK5678CXZO";

static void tecla_aleatoria(void)
{
    uint32_t r = rnd(1000);
    int i, n;

    if (r < 450) {
        ed_tecla(32 + (int)rnd(95));
    } else if (r < 540) {
        ed_tecla(ED_K_ENTER);
    } else if (r < 680) {
        ed_tecla(ED_K_BORRAR);
    } else if (r < 900) {
        ed_tecla(cursores[rnd(4)]);
    } else if (r < 985) {
        ed_tecla(ED_K_MENU);
        ed_tecla(cmds[rnd(sizeof(cmds) - 1)]);
    } else if (r < 992) {
        n = 60 + (int)rnd(30);                          /* ráfaga: linea larga */
        for (i = 0; i < n; i++) ed_tecla('a' + (int)rnd(26));
    } else if (r < 997) {
        n = 5 + (int)rnd(40);                           /* ráfaga: muchas lineas */
        for (i = 0; i < n; i++) ed_tecla(ED_K_ENTER);
    } else {
        ed_tecla((int)rnd(256));                        /* cualquier codigo */
    }
}

static void test_aleatorio(uint32_t sem, uint16_t cap, int teclas_n)
{
    int i, ok = 1;

    semilla = sem;
    preparar();
    fk_aleatorio = 1;                                   /* Esc+C/X/Z del fuzz: el compilador falso inventa errores */
    if (cap == 1) {
        /* parte de un fichero cargado: lineas largas y cortas, '\r' incluidos */
        for (i = 0; i < 3000; i++) grande[i] = (rnd(30) == 0) ? '\n' : (rnd(97) == 0) ? '\r' : (uint8_t)(32 + rnd(95));
        fich_poner(grande, 3000);
        ed_cargar("a.f");
    } else {
        eb_iniciar_cap(cap);
    }
    ed_refrescar();
    for (i = 0; i < teclas_n && ok; i++) {
        tecla_aleatoria();
        if (ed.modo >= 2) ed_tecla(27);                 /* los menus de ficheros tienen su propia prueba */
        ed_refrescar();
        if (ed_verificar() != 0) { printf("  invariante %d tras la tecla %d (sem %u)\n", ed_verificar(), i, (unsigned)sem); ok = 0; }
        else if (violaciones != 0) { printf("  violacion de pantalla tras la tecla %d\n", i); ok = 0; }
        else if (!pantalla_correcta()) { printf("  pantalla distinta del pintor ingenuo tras la tecla %d (sem %u)\n", i, (unsigned)sem); ok = 0; }
    }
    COMPROBAR(ok);
}

/* ------------------------------------------------------------------
 * 4. ed_linea (contrato de compiler/include/mf_api.h)
 * ------------------------------------------------------------------ */

static uint16_t todo_ver;
static uint16_t todo_len;
static uint8_t todo_ok;

/* Referencia: parte `todo` (copia del texto) por '\n' sin usar nada del editor */
static int ref_linea(uint16_t n, char *dst)
{
    uint16_t p = 0, i = 1, f, k;

    if (!todo_ok || todo_ver != eb.ver) {
        todo_len = eb_longitud();
        eb_leer(0, todo, todo_len);
        todo_ver = eb.ver;
        todo_ok = 1;
    }
    if (n == 0) return 0;
    while (i < n) {
        while (p < todo_len && todo[p] != '\n') p++;
        if (p >= todo_len) return 0;
        p++;
        i++;
    }
    if (p >= todo_len) return 0;             /* detras del ultimo '\n' no hay linea */
    f = p;
    while (f < todo_len && todo[f] != '\n') f++;
    k = (uint16_t)(f - p);
    if (k > 72) k = 72;
    memcpy(dst, todo + p, k);
    dst[k] = 0;
    return 1;
}

/* Pide la linea n al editor y a la referencia: mismo resultado y sin escribir mas alla de dst[72] */
static int linea_igual(uint16_t n)
{
    char a[96], b[96];
    int ra, rb;

    memset(a, 0x7E, sizeof(a));
    memset(b, 0, sizeof(b));
    ra = ed_linea(n, a);
    rb = ref_linea(n, b);
    if (ra != rb) return 0;
    if (ra) {
        int i;
        if (strcmp(a, b) != 0) return 0;
        for (i = 73; i < (int)sizeof(a); i++) if (a[i] != 0x7E) return 0;   /* nada fuera de los 73 bytes */
    }
    return 1;
}

static void test_ed_linea(void)
{
    char d[96];
    uint16_t i, n;
    int s, ok;
    static uint8_t g[400];

    /* Casos del contrato */
    preparar();
    COMPROBAR(ed_linea(1, d) == 0);                          /* texto vacio: no hay linea 1 */
    ed_insertar_texto("A");
    COMPROBAR(ed_linea(1, d) == 1 && strcmp(d, "A") == 0);
    COMPROBAR(ed_linea(2, d) == 0);
    ed_insertar_texto("\n");
    COMPROBAR(ed_linea(1, d) == 1 && strcmp(d, "A") == 0);   /* "A\n": detras del ultimo '\n' no hay linea */
    COMPROBAR(ed_linea(2, d) == 0);
    ed_insertar_texto("\nB");
    COMPROBAR(ed_linea(2, d) == 1 && d[0] == 0);             /* una linea vacia EXISTE */
    COMPROBAR(ed_linea(3, d) == 1 && strcmp(d, "B") == 0);
    COMPROBAR(ed_linea(0, d) == 0 && ed_linea(4, d) == 0);
    COMPROBAR(ed_linea(1, d) == 1 && strcmp(d, "A") == 0);   /* hacia atras: otra compilacion */
    COMPROBAR(ed_linea(3, d) == 1 && strcmp(d, "B") == 0);

    /* El texto cambia entre dos compilaciones: la cache no sirve lineas viejas */
    preparar();
    ed_insertar_texto("uno\ndos\ntres\ncuatro\n");
    COMPROBAR(ed_linea(1, d) == 1 && ed_linea(2, d) == 1 && ed_linea(3, d) == 1 && strcmp(d, "tres") == 0);
    ed_tecla(ED_K_MENU); ed_tecla('T');                       /* al principio del texto */
    ed_tecla('X');                                            /* "Xuno": cambia el texto, la cache apunta a la linea 3 */
    COMPROBAR(ed_linea(3, d) == 1 && strcmp(d, "tres") == 0);
    COMPROBAR(ed_linea(1, d) == 1 && strcmp(d, "Xuno") == 0);
    ed_tecla(ED_K_ENTER);                                     /* "X" y "uno": las demas bajan una linea */
    COMPROBAR(ed_linea(4, d) == 1 && strcmp(d, "tres") == 0);
    COMPROBAR(ed_linea(5, d) == 1 && strcmp(d, "cuatro") == 0);
    COMPROBAR(ed_linea(6, d) == 0);
    ed_iniciar();                                             /* vaciar */
    COMPROBAR(ed_linea(1, d) == 0);

    /* Lineas largas: como mucho 72 caracteres */
    for (i = 0; i < 100; i++) g[i] = (uint8_t)('a' + (i % 26));
    g[100] = '\n';
    memcpy(g + 101, "fin", 3);
    preparar();
    fich_poner(g, 104);
    COMPROBAR(ed_cargar("l.f") == 0);
    memset(d, 0x7E, sizeof(d));
    COMPROBAR(ed_linea(1, d) == 1 && strlen(d) == 72 && d[71] == (char)('a' + (71 % 26)));
    COMPROBAR(d[73] == 0x7E && d[95] == 0x7E);
    COMPROBAR(ed_linea(2, d) == 1 && strcmp(d, "fin") == 0);
    COMPROBAR(linea_igual(1) && linea_igual(2) && linea_igual(3));

    /* Aleatorio: ediciones y consultas (secuenciales, con saltos y hacia atras) contra la referencia */
    ok = 1;
    for (s = 0; s < 8 && ok; s++) {
        int k, q;
        semilla = 700 + (uint32_t)s;
        preparar();
        if (s >= 6) {
            for (i = 0; i < 3000; i++) grande[i] = (rnd(30) == 0) ? '\n' : (uint8_t)(32 + rnd(95));
            fich_poner(grande, 3000);
            ed_cargar("a.f");
        } else {
            eb_iniciar_cap((uint16_t)(s % 2 ? 600 : 5000));
        }
        todo_ok = 0;
        for (k = 0; k < 4000 && ok; k++) {
            tecla_aleatoria();
            if (rnd(4) == 0) {
                n = (uint16_t)(1 + rnd(3));
                if (rnd(2)) n = (uint16_t)(1 + eb_contar_nl(eb_longitud()) + rnd(3));
                for (q = 0; q < 6 && ok; q++) {            /* un recorrido secuencial corto desde n */
                    if (!linea_igual((uint16_t)(n + q))) ok = 0;
                }
                if (ok && !linea_igual((uint16_t)(1 + rnd(40)))) ok = 0;   /* y un salto cualquiera */
            }
        }
    }
    COMPROBAR(ok);
}


/* ------------------------------------------------------------------
 * Guardar, nuevo y menus de ficheros
 * ------------------------------------------------------------------ */

static void fich_reiniciar(void)
{
    fich_abrir_err = 2;               /* por defecto, "no existe" */
    fich_crear_err = 0;
    fich_esc_limite = 0;
    fich_abrir_n = 0;
    fich_crear_n = 0;
    fich_nom_lect[0] = 0;
    fich_nom_esc[0] = 0;
    esc_len = 0;
}

/* El texto del editor como bloque, para compararlo con lo guardado */
static int texto_igual(const uint8_t *d, uint32_t n)
{
    static uint8_t t[ED_CAP + 64];
    uint32_t p = 0;

    if (eb_longitud() != n) return 0;
    while (p < n) {
        uint16_t k = (uint16_t)((n - p) < 200 ? (n - p) : 200);
        eb_leer((uint16_t)p, t + p, k);
        p += k;
    }
    return memcmp(t, d, n) == 0;
}

static void test_guardar_basico(void)
{
    static const char txt[] = "      PROGRAM X\n      END";

    preparar();
    fich_reiniciar();
    ed_insertar_texto(txt);
    COMPROBAR(ed.modificado == 1);
    COMPROBAR(ed_guardar("a.f") == 0);
    COMPROBAR(esc_len == sizeof(txt) - 1 && memcmp(esc_datos, txt, esc_len) == 0);
    COMPROBAR(strcmp(fich_nom_esc, "a.f") == 0);
    COMPROBAR(ed.nombre != 0 && strcmp(ed.nombre, "a.f") == 0);
    COMPROBAR(ed.modificado == 0 && ed.msg_err == 0);
    COMPROBAR(ed.msg != 0 && strcmp(ed.msg, "Saved a.f: 25 bytes") == 0);
    COMPROBAR(esc_abiertos == 0);
    ed_refrescar_ok();

    /* el texto vacio es un fichero vacio */
    ed_nuevo();
    COMPROBAR(ed_guardar("v.f") == 0 && esc_len == 0 && esc_abiertos == 0);
    COMPROBAR(strcmp(ed.msg, "Saved v.f: 0 bytes") == 0);
}

static void test_guardar_errores(void)
{
    static uint8_t largo[300];
    int i;

    for (i = 0; i < 300; i++) largo[i] = (i % 40 == 39) ? '\n' : 'a';

    /* no se puede crear: nada cambia */
    preparar();
    fich_reiniciar();
    ed_insertar_texto("hola");
    fich_crear_err = 5;
    COMPROBAR(ed_guardar("x.f") == 1);
    COMPROBAR(ed.modificado == 1 && ed.nombre == 0 && ed.msg_err == 1);
    COMPROBAR(strcmp(ed.msg, "Cannot create x.f (err 5)") == 0);
    COMPROBAR(esc_abiertos == 0);

    /* escritura corta a mitad: se avisa, sigue modificado, el nombre no cambia, se cierra */
    preparar();
    fich_reiniciar();
    fich_poner(largo, 300);
    COMPROBAR(ed_cargar("l.f") == 0);
    ed_tecla('q');
    fich_esc_limite = 100;
    COMPROBAR(ed_guardar("otro.f") == 2);
    COMPROBAR(ed.modificado == 1 && ed.msg_err == 1);
    COMPROBAR(strstr(ed.msg, "Write error") != 0);
    COMPROBAR(strcmp(ed.nombre, "l.f") == 0);
    COMPROBAR(esc_abiertos == 0 && esc_len == 100);

    /* nombre de 31 caracteres: cabe el mensaje completo (no se corta el "bytes") */
    preparar();
    fich_reiniciar();
    ed_insertar_texto("hola");
    COMPROBAR(ed_guardar("0123456789012345678901234567890") == 0);
    COMPROBAR(strstr(ed.msg, ": 4 bytes") != 0);
    fich_crear_err = 127;
    COMPROBAR(ed_guardar("0123456789012345678901234567890") == 1);
    COMPROBAR(strstr(ed.msg, "(err 127)") != 0);
}

static void test_guardar_ida_y_vuelta(void)
{
    static uint8_t gran[ED_CAP + 64];
    uint32_t i, n;
    uint8_t *copia;
    int k;

    /* fichero que casi llena el buffer, con una linea de 300 caracteres */
    n = ED_CAP - 40;
    for (i = 0; i < n; i++) gran[i] = (i % 33 == 32) ? '\n' : (uint8_t)('A' + i % 26);
    for (i = 1000; i < 1300; i++) gran[i] = 'z';
    preparar();
    fich_reiniciar();
    fich_poner(gran, n);
    COMPROBAR(ed_cargar("g.f") == 0);
    COMPROBAR(ed_guardar("g2.f") == 0);
    COMPROBAR(esc_len == n && memcmp(esc_datos, gran, n) == 0);

    /* sesiones de teclado aleatorias: guardar y volver a cargar deja el mismo texto */
    copia = (uint8_t *)malloc(ED_CAP + 64);
    for (k = 0; k < 30; k++) {
        int j;

        preparar();
        fich_reiniciar();
        for (j = 0; j < 800; j++) {
            uint32_t r = rnd(100);
            if (r < 70) ed_tecla(' ' + (int)rnd(95));
            else if (r < 80) ed_tecla(ED_K_ENTER);
            else if (r < 88) ed_tecla(ED_K_BORRAR);
            else if (r < 91) ed_tecla(ED_K_IZQ);
            else if (r < 94) ed_tecla(ED_K_DER);
            else if (r < 97) ed_tecla(ED_K_ARR);
            else ed_tecla(ED_K_ABA);
        }
        COMPROBAR(ed_guardar("r.f") == 0);
        COMPROBAR(texto_igual(esc_datos, esc_len));
        memcpy(copia, esc_datos, esc_len);
        n = esc_len;
        fich_poner(copia, n);
        fich_trozo = (uint16_t)rnd(90);
        COMPROBAR(ed_cargar("r.f") == 0);
        COMPROBAR(texto_igual(copia, n));
    }
    free(copia);
}

static void fila_msg(char *f)
{
    ed_refrescar_ok();
    fila_txt(ED_FILA_MSG, f);
}

static void test_menu_nuevo(void)
{
    char f[ED_ANCHO + 1];
    int i, ok;

    preparar();
    fich_reiniciar();
    ed_insertar_texto("abc");
    ed.modificado = 1;
    ed_tecla(ED_K_MENU); ed_tecla('N');
    COMPROBAR(ed.modo == 3);
    fila_msg(f);
    COMPROBAR(strstr(f, "Unsaved changes will be lost. Continue? (Y/N)") != 0);
    ok = 1;                                         /* la pregunta va en el estilo de error */
    for (i = 1; i < 45; i++) if (estilos[ED_FILA_MSG][i] != ED_E_ERROR) ok = 0;
    COMPROBAR(ok);
    fila_txt(ED_FILA_AYUDA, f);
    COMPROBAR(strstr(f, "any other key") != 0);
    fila_txt(0, f);
    COMPROBAR(strstr(f, "  FILE") != 0);
    ed_tecla('n');                                  /* cualquier otra tecla cancela */
    COMPROBAR(ed.modo == 0 && eb_longitud() == 3 && strcmp(ed.msg, "Cancelled") == 0 && ed.modificado == 1);
    fila_msg(f);
    COMPROBAR(strcmp(f, " Cancelled") == 0);

    ed_tecla(ED_K_MENU); ed_tecla('N'); ed_tecla('Y');
    COMPROBAR(ed.modo == 0 && eb_longitud() == 0 && ed.nombre == 0 && ed.modificado == 0);
    COMPROBAR(strcmp(ed.msg, "New file") == 0 && ed.msg_err == 0 && ed.pos == 0);
    fila_msg(f);
    COMPROBAR(strcmp(f, " New file") == 0);

    /* sin cambios no pregunta; y quita el nombre del fichero */
    fich_poner((const uint8_t *)"C HI\n", 5);
    COMPROBAR(ed_cargar("h.f") == 0 && ed.nombre != 0);
    ed_tecla(ED_K_MENU); ed_tecla('n');             /* minuscula tambien vale */
    COMPROBAR(ed.modo == 0 && eb_longitud() == 0 && ed.nombre == 0);
    ed_refrescar_ok();
    fila_txt(0, f);
    COMPROBAR(strncmp(f, " MYFORTRAN ", 11) == 0 && f[12] == ' ');
}

static void test_menu_cargar(void)
{
    char f[ED_ANCHO + 1];
    uint8_t b[8];
    int i;

    preparar();
    fich_reiniciar();
    fich_poner((const uint8_t *)"C HI\n", 5);
    fich_abrir_err = 0;
    ed_tecla(ED_K_MENU); ed_tecla('L');
    COMPROBAR(ed.modo == 2 && fich_abrir_n == 0);
    fila_msg(f);
    COMPROBAR(strcmp(f, " Load file:") == 0);
    COMPROBAR(cur_f == ED_FILA_MSG && cur_c == 12);
    fila_txt(ED_FILA_AYUDA, f);
    COMPROBAR(strstr(f, "ENTER accept") != 0 && strstr(f, "Esc cancel") != 0);
    teclas("ab.fx");
    ed_tecla(ED_K_BORRAR);
    ed_tecla(ED_K_BORRAR);
    ed_tecla(ED_K_BORRAR);
    ed_tecla('f');
    fila_msg(f);
    COMPROBAR(strcmp(f, " Load file: abf") == 0 && cur_c == 15);
    ed_tecla(ED_K_BORRAR);
    ed_tecla('.'); ed_tecla('f');
    ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.modo == 0 && strcmp(fich_nom_lect, "ab.f") == 0);
    COMPROBAR(ed.nombre != 0 && strcmp(ed.nombre, "ab.f") == 0);
    COMPROBAR(eb_longitud() == 5 && ed.modificado == 0);
    eb_leer(0, b, 5);
    COMPROBAR(memcmp(b, "C HI\n", 5) == 0);
    COMPROBAR(strcmp(ed.msg, "Loaded ab.f: 5 bytes") == 0);
    ed_refrescar_ok();

    /* Esc cancela sin tocar nada ni abrir nada */
    fich_abrir_n = 0;
    ed_tecla(ED_K_MENU); ed_tecla('L'); teclas("zz"); ed_tecla(27);
    COMPROBAR(ed.modo == 0 && fich_abrir_n == 0 && strcmp(ed.msg, "Cancelled") == 0);
    COMPROBAR(strcmp(ed.nombre, "ab.f") == 0 && eb_longitud() == 5);

    /* nombre vacio */
    ed_tecla(ED_K_MENU); ed_tecla('L'); ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.modo == 0 && fich_abrir_n == 0 && ed.msg_err == 1 && strcmp(ed.msg, "No file name") == 0);

    /* no se puede abrir: el texto y el nombre siguen */
    fich_abrir_err = 2;
    ed_tecla(ED_K_MENU); ed_tecla('L'); teclas("nada.f"); ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.modo == 0 && ed.msg_err == 1 && strcmp(ed.msg, "Cannot open nada.f (err 2)") == 0);
    COMPROBAR(eb_longitud() == 5 && strcmp(ed.nombre, "ab.f") == 0);

    /* el nombre se limita a 31 caracteres */
    ed_tecla(ED_K_MENU); ed_tecla('L');
    for (i = 0; i < 50; i++) ed_tecla('a' + i % 26);
    fila_msg(f);
    COMPROBAR(strlen(f) == 1 + 11 + 31 && cur_c == 1 + 11 + 31);
    ed_tecla(27);

    /* con cambios sin guardar pregunta antes de pedir el nombre */
    fich_abrir_err = 0;
    ed_tecla('q');
    ed_tecla(ED_K_MENU); ed_tecla('L');
    COMPROBAR(ed.modo == 3 && fich_abrir_n == 1 /* el intento de arriba */);
    ed_tecla(ED_K_ENTER);                           /* cancela */
    COMPROBAR(ed.modo == 0 && ed.modificado == 1);
    ed_tecla(ED_K_MENU); ed_tecla('L'); ed_tecla('y');
    COMPROBAR(ed.modo == 2 && eb_longitud() == 6);  /* aun no ha tirado nada */
    teclas("n.f"); ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.modo == 0 && eb_longitud() == 5 && strcmp(ed.nombre, "n.f") == 0 && ed.modificado == 0);
    ed_refrescar_ok();

    /* se puede recargar el fichero actual (el nombre apunta al propio buffer) */
    ed_tecla(ED_K_MENU); ed_tecla('L'); teclas("n.f"); ed_tecla(ED_K_ENTER);
    COMPROBAR(strcmp(ed.nombre, "n.f") == 0 && strcmp(ed.msg, "Loaded n.f: 5 bytes") == 0);
}

static void test_menu_guardar(void)
{
    char f[ED_ANCHO + 1];
    int n0;

    preparar();
    fich_reiniciar();
    teclas("hola");

    /* sin nombre: lo pide; el fichero no existe: guarda sin preguntar */
    ed_tecla(ED_K_MENU); ed_tecla('S');
    COMPROBAR(ed.modo == 2 && fich_crear_n == 0);
    fila_msg(f);
    COMPROBAR(strcmp(f, " Save as:") == 0 && cur_c == 10);
    ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.msg_err == 1 && strcmp(ed.msg, "No file name") == 0 && fich_crear_n == 0);
    ed_tecla(ED_K_MENU); ed_tecla('S'); teclas("o.f"); ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.modo == 0 && fich_crear_n == 1 && strcmp(fich_nom_esc, "o.f") == 0);
    COMPROBAR(esc_len == 4 && memcmp(esc_datos, "hola", 4) == 0 && ed.modificado == 0);
    COMPROBAR(strcmp(ed.nombre, "o.f") == 0 && strcmp(ed.msg, "Saved o.f: 4 bytes") == 0);

    /* con nombre: S guarda directamente */
    ed_tecla('x');
    ed_tecla(ED_K_MENU); ed_tecla('S');
    COMPROBAR(ed.modo == 0 && fich_crear_n == 2 && esc_len == 5 && ed.modificado == 0);
    COMPROBAR(strcmp(ed.msg, "Saved o.f: 5 bytes") == 0);

    /* A: pide el nombre con el actual escrito; el mismo nombre no pregunta aunque exista */
    fich_abrir_err = 0;                              /* "todo existe" */
    ed_tecla(ED_K_MENU); ed_tecla('A');
    COMPROBAR(ed.modo == 2);
    fila_msg(f);
    COMPROBAR(strcmp(f, " Save as: o.f") == 0 && cur_c == 13);
    ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.modo == 0 && fich_crear_n == 3);

    /* otro nombre que ya existe: pregunta */
    ed_tecla(ED_K_MENU); ed_tecla('A');
    ed_tecla(ED_K_BORRAR); ed_tecla(ED_K_BORRAR); ed_tecla(ED_K_BORRAR);
    teclas("p.f"); ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.modo == 3 && fich_crear_n == 3);
    fila_msg(f);
    COMPROBAR(strstr(f, "File exists. Overwrite? (Y/N)") != 0);
    COMPROBAR(estilos[ED_FILA_MSG][1] == ED_E_ERROR);
    ed_tecla('n');
    COMPROBAR(ed.modo == 0 && fich_crear_n == 3 && strcmp(ed.nombre, "o.f") == 0);
    ed_tecla(ED_K_MENU); ed_tecla('A');
    ed_tecla(ED_K_BORRAR); ed_tecla(ED_K_BORRAR); ed_tecla(ED_K_BORRAR);
    teclas("p.f"); ed_tecla(ED_K_ENTER); ed_tecla('Y');
    COMPROBAR(ed.modo == 0 && fich_crear_n == 4 && strcmp(fich_nom_esc, "p.f") == 0);
    COMPROBAR(strcmp(ed.nombre, "p.f") == 0 && ed.modificado == 0);

    /* otro nombre que NO existe: guarda sin preguntar */
    fich_abrir_err = 2;
    ed_tecla(ED_K_MENU); ed_tecla('A'); ed_tecla('2');
    n0 = fich_crear_n;
    ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.modo == 0 && fich_crear_n == n0 + 1 && strcmp(fich_nom_esc, "p.f2") == 0);

    /* error al crear con S: avisa y sigue modificado */
    ed_tecla('z');
    fich_crear_err = 9;
    ed_tecla(ED_K_MENU); ed_tecla('S');
    COMPROBAR(ed.modo == 0 && ed.msg_err == 1 && ed.modificado == 1);
    COMPROBAR(strcmp(ed.msg, "Cannot create p.f2 (err 9)") == 0);
    ed_refrescar_ok();
    fila_msg(f);
    COMPROBAR(estilos[ED_FILA_MSG][1] == ED_E_ERROR);
}

static void test_ayuda_ficheros(void)
{
    char f[ED_ANCHO + 1];

    preparar();
    ed_refrescar_ok();
    fila_txt(ED_FILA_AYUDA, f);
    COMPROBAR(strstr(f, "N/L/S/A file") != 0);
    ed_tecla(ED_K_MENU);
    ed_refrescar_ok();
    fila_txt(ED_FILA_AYUDA, f);
    COMPROBAR(strstr(f, "G goto") != 0 && strstr(f, "I tab") == 0 && strstr(f, "5-8") == 0);
    fila_txt(ED_FILA_MSG, f);
    COMPROBAR(strstr(f, "N new  L load  S save  A save as") != 0);
    COMPROBAR(violaciones == 0);
    fila_txt(ED_FILA_AYUDA, f);
    ed_tecla('H');
    ed_refrescar_ok();
    fila_txt(ED_FILA_AYUDA, f);
    COMPROBAR(strstr(f, "G goto") != 0 && strstr(f, "I tab") == 0);
    ed_tecla(ED_K_MENU); ed_tecla('I');             /* ya no es un comando */
    COMPROBAR(ed.msg != 0 && strcmp(ed.msg, "Unknown command") == 0);
}


/* ------------------------------------------------------------------
 * Ir a linea
 * ------------------------------------------------------------------ */

static void test_ir_a_linea(void)
{
    char f[ED_ANCHO + 1];
    int i;

    preparar();
    for (i = 1; i <= 100; i++) {
        char b[16];
        snprintf(b, sizeof(b), "L%d\n", i);
        ed_insertar_texto(b);
    }
    ed_tecla(ED_K_MENU); ed_tecla('t');
    ed_tecla(ED_K_DER); ed_tecla(ED_K_DER);          /* col_obj = 2: ir a linea debe ponerla a 0 */
    ed_tecla(ED_K_MENU); ed_tecla('G');
    COMPROBAR(ed.modo == 2);
    fila_msg(f);
    COMPROBAR(strcmp(f, " Go to line:") == 0 && cur_f == ED_FILA_MSG && cur_c == 13);
    teclas("4x2a");                                 /* solo se aceptan cifras */
    fila_msg(f);
    COMPROBAR(strcmp(f, " Go to line: 42") == 0);
    ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.modo == 0 && ed.linea == 41 && ed.col_obj == 0 && ed.msg == 0);
    ed_refrescar_ok();
    fila_txt(1 + (int)(ed.linea - ed.top_linea), f);
    COMPROBAR(strcmp(f, "L42") == 0);
    COMPROBAR(cur_f == 1 + (int)(ed.linea - ed.top_linea) && cur_c == 0);

    /* 1, 0 y mas alla del final */
    ed_tecla(ED_K_MENU); ed_tecla('G'); ed_tecla('1'); ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.linea == 0 && ed.pos == 0);
    ed_tecla(ED_K_MENU); ed_tecla('G'); ed_tecla('0'); ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.linea == 0 && ed.msg == 0);
    ed_tecla(ED_K_MENU); ed_tecla('G'); teclas("99999"); ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.linea == 100 && ed.pos == eb_longitud());       /* la linea 101 esta vacia */
    COMPROBAR(ed.msg != 0 && strcmp(ed.msg, "Past the end: moved to line 101") == 0);
    ed_refrescar_ok();
    ed_tecla(ED_K_MENU); ed_tecla('G'); teclas("100"); ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.linea == 99 && ed.msg == 0);
    ed_refrescar_ok();

    /* Esc cancela, Enter vacio no hace nada raro, Delete borra */
    ed_tecla(ED_K_MENU); ed_tecla('G'); teclas("7"); ed_tecla(27);
    COMPROBAR(ed.modo == 0 && ed.linea == 99);
    ed_tecla(ED_K_MENU); ed_tecla('G'); ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.modo == 0 && ed.msg_err == 1);
    ed_tecla(ED_K_MENU); ed_tecla('G'); teclas("12"); ed_tecla(ED_K_BORRAR); ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.linea == 0);

    /* el desbordamiento de 16 bits no da la vuelta: 65600 es "mas alla", no la linea 64 */
    ed_tecla(ED_K_MENU); ed_tecla('G'); teclas("65600"); ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.linea == 100);

    /* texto vacio */
    ed_nuevo();
    ed_tecla(ED_K_MENU); ed_tecla('G'); ed_tecla('5'); ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.linea == 0 && ed.pos == 0);
    ed_refrescar_ok();
}


/* El editor solo toca SUS paginas (ED_PAG_BASE .. ED_PAG_BASE+ED_PAGINAS-1): las del
 * compilador (0 .. ED_PAG_BASE-1) quedan intactas pase lo que pase. */
static void test_paginas_ajenas(void)
{
    static uint8_t mio[8192], antes[8192];
    uint8_t p;
    uint16_t i;
    int j, ok;

    COMPROBAR(ED_PAG_BASE == 6 && ED_PAGINAS == 4 && ED_CAP == 32768u);
    preparar();
    for (p = 0; p < ED_PAG_BASE; p++) {
        for (i = 0; i < 8192; i++) antes[i] = (uint8_t)(p * 37 + i * 11 + 5);
        banco_escribir(p, 0, antes, 8192);
    }
    for (j = 0; j < 3000; j++) {                   /* ediciones, cargas y guardados */
        uint32_t r = rnd(100);
        if (r < 80) ed_tecla(' ' + (int)rnd(95));
        else if (r < 90) ed_tecla(ED_K_ENTER);
        else ed_tecla(ED_K_BORRAR);
    }
    ed_guardar("p.f");
    ed_nuevo();
    ed_insertar_texto("hola\n");
    ok = 1;
    for (p = 0; p < ED_PAG_BASE; p++) {
        banco_leer(p, 0, mio, 8192);
        for (i = 0; i < 8192; i++) antes[i] = (uint8_t)(p * 37 + i * 11 + 5);
        if (memcmp(mio, antes, 8192) != 0) ok = 0;
    }
    COMPROBAR(ok);
    for (p = (uint8_t)(ED_PAG_BASE + ED_PAGINAS); p < BANCO_PAGINAS; p++) {
        banco_leer(p, 0, mio, 8192);                /* si sobra alguna pagina, tampoco */
        for (i = 0; i < 8192; i++) if (mio[i] != 0) ok = 0;
    }
    COMPROBAR(ok);
}


/* ------------------------------------------------------------------
 * Borrar linea (Esc+K) y guardado tras una carga incompleta
 * ------------------------------------------------------------------ */

static void texto_a_cadena(char *dst)
{
    uint16_t n = eb_longitud();

    if (n != 0) eb_leer(0, (uint8_t *)dst, n);
    dst[n] = 0;
}

/* Pone el cursor en el principio de la linea n (desde 0) con G */
static void cursor_a_linea(int n)
{
    char b[16];

    ed_tecla(ED_K_MENU); ed_tecla('G');
    snprintf(b, sizeof(b), "%d", n + 1);
    teclas(b);
    ed_tecla(ED_K_ENTER);
}

static void test_borrar_linea(void)
{
    char t[64];
    char f[ED_ANCHO + 1];

    /* en medio */
    preparar();
    ed_insertar_texto("uno\ndos\ntres\n");
    cursor_a_linea(1);
    ed_tecla(ED_K_DER);
    ed.modificado = 0;
    ed_tecla(ED_K_MENU); ed_tecla('K');
    texto_a_cadena(t);
    COMPROBAR(strcmp(t, "uno\ntres\n") == 0);
    COMPROBAR(ed.linea == 1 && ed.pos == 4 && ed.ini_lin == 4 && ed.col_obj == 0 && ed.modificado == 1);
    ed_refrescar_ok();
    fila_txt(2, f);
    COMPROBAR(strcmp(f, "tres") == 0);

    /* la primera */
    cursor_a_linea(0);
    ed_tecla(ED_K_MENU); ed_tecla('k');
    texto_a_cadena(t);
    COMPROBAR(strcmp(t, "tres\n") == 0 && ed.linea == 0 && ed.pos == 0);
    ed_refrescar_ok();

    /* la ultima linea con texto (la 3.a, vacia, existe tras el ultimo salto: se borra esa) */
    cursor_a_linea(1);
    COMPROBAR(ed.linea == 1 && ed.pos == 5);
    ed_tecla(ED_K_MENU); ed_tecla('K');
    texto_a_cadena(t);
    COMPROBAR(strcmp(t, "tres") == 0 && ed.linea == 0 && ed.pos == 0);
    ed_refrescar_ok();

    /* ultima sin salto final: desaparece y el cursor va a la anterior */
    ed_tecla(ED_K_MENU); ed_tecla('B');
    ed_tecla(ED_K_ENTER); teclas("cuatro");
    texto_a_cadena(t);
    COMPROBAR(strcmp(t, "tres\ncuatro") == 0 && ed.linea == 1);
    ed_tecla(ED_K_MENU); ed_tecla('K');
    texto_a_cadena(t);
    COMPROBAR(strcmp(t, "tres") == 0 && ed.linea == 0 && ed.pos == 0);

    /* una sola linea: se vacia; texto vacio: no hace nada */
    ed_tecla(ED_K_MENU); ed_tecla('K');
    COMPROBAR(eb_longitud() == 0 && ed.linea == 0 && ed.pos == 0);
    ed.modificado = 0;
    ed_tecla(ED_K_MENU); ed_tecla('K');
    COMPROBAR(eb_longitud() == 0 && ed.modificado == 0);
    ed_refrescar_ok();

    /* lineas vacias */
    preparar();
    ed_insertar_texto("a\n\n\nb");
    cursor_a_linea(1);
    ed_tecla(ED_K_MENU); ed_tecla('K');
    texto_a_cadena(t);
    COMPROBAR(strcmp(t, "a\n\nb") == 0 && ed.linea == 1);
    ed_refrescar_ok();

    /* lineas largas cargadas de un fichero, y con la vista desplazada */
    {
        static uint8_t fich[400];
        int i;
        for (i = 0; i < 300; i++) fich[i] = (uint8_t)('a' + i % 26);
        fich[300] = '\n';
        memcpy(fich + 301, "fin\n", 4);
        preparar();
        fich_reiniciar();
        fich_poner(fich, 305);
        COMPROBAR(ed_cargar("l.f") == 0);
        ed_tecla(ED_K_MENU); ed_tecla('E');
        ed_refrescar_ok();
        COMPROBAR(ed.izq > 0);
        ed_tecla(ED_K_MENU); ed_tecla('K');
        texto_a_cadena(t);
        COMPROBAR(strcmp(t, "fin\n") == 0);
        ed_refrescar_ok();
        COMPROBAR(ed.izq == 0);
    }

    /* borrar la linea de abajo de la pantalla con mucho texto: la vista se ajusta */
    {
        int i;
        preparar();
        for (i = 0; i < 80; i++) { char b[16]; snprintf(b, sizeof(b), "L%d\n", i); ed_insertar_texto(b); }
        cursor_a_linea(60);
        ed_refrescar_ok();
        for (i = 0; i < 20; i++) { ed_tecla(ED_K_MENU); ed_tecla('K'); ed_refrescar_ok(); }
        COMPROBAR(ed.linea == 60 && eb_contar_nl(eb_longitud()) == 60);    /* quedan L0..L59 y la vacia del final */
        ed_tecla(ED_K_MENU); ed_tecla('B');
        for (i = 0; i < 70; i++) { ed_tecla(ED_K_MENU); ed_tecla('K'); ed_refrescar_ok(); }
        COMPROBAR(eb_longitud() == 0);
    }

    /* la barra de ayuda lo menciona */
    preparar();
    ed_refrescar_ok();
    fila_txt(ED_FILA_AYUDA, f);
    COMPROBAR(strstr(f, "K del") != 0);
    ed_tecla(ED_K_MENU);
    ed_refrescar_ok();
    fila_txt(ED_FILA_AYUDA, f);
    COMPROBAR(strstr(f, "K delete line") != 0 && strlen(f) <= ED_ANCHO);
}

/* Contra una referencia ingenua: lineas aleatorias, Esc+K desde posiciones aleatorias */
static void test_borrar_linea_modelo(void)
{
    int k, j;

    for (k = 0; k < 20; k++) {
        static char modelo[4000];
        int ml = 0, nl;
        uint32_t r;

        semilla = 7000 + (uint32_t)k;
        preparar();
        nl = 1 + (int)rnd(40);
        for (j = 0; j < nl; j++) {
            int len = (int)rnd(20);
            int q;
            for (q = 0; q < len; q++) { char c = (char)('a' + rnd(26)); modelo[ml++] = c; ed_tecla(c); }
            if (j + 1 < nl || rnd(2)) { modelo[ml++] = '\n'; ed_tecla(ED_K_ENTER); }
        }
        modelo[ml] = 0;
        for (j = 0; j < 60 && ml > 0; j++) {
            int linea, i0, i1, total = 0, c;
            char *p;

            for (c = 0; c < ml; c++) if (modelo[c] == '\n') total++;
            linea = (int)rnd((uint32_t)total + 1);
            cursor_a_linea(linea);
            /* referencia: inicio y fin de la linea `linea` */
            i0 = 0;
            for (c = 0; c < linea; c++) { p = strchr(modelo + i0, '\n'); i0 = (int)(p - modelo) + 1; }
            p = strchr(modelo + i0, '\n');
            i1 = p ? (int)(p - modelo) : ml;
            if (p) i1++;                                        /* con su salto */
            else if (i0 > 0) i0--;                              /* ultima: se lleva el salto anterior */
            memmove(modelo + i0, modelo + i1, (size_t)(ml - i1) + 1);
            ml -= i1 - i0;
            ed_tecla(ED_K_MENU); ed_tecla('K');
            {
                char t[4000];
                texto_a_cadena(t);
                if (strcmp(t, modelo) != 0) { COMPROBAR(strcmp(t, modelo) == 0); break; }
            }
            ed_refrescar_ok();
            r = ed_verificar();
            if (r != 0) { COMPROBAR(r == 0); break; }
        }
        pruebas++;
    }
}

static void test_guardar_tras_carga_incompleta(void)
{
    static uint8_t gran[ED_CAP + 600];
    uint32_t i;
    int n0;

    for (i = 0; i < sizeof(gran); i++) gran[i] = (i % 40 == 39) ? '\n' : (uint8_t)('a' + i % 26);

    /* no cabe: S pregunta antes de pisar el fichero */
    preparar();
    fich_reiniciar();
    fich_poner(gran, sizeof(gran));
    COMPROBAR(ed_cargar("g.f") == 3);
    ed_tecla(ED_K_MENU); ed_tecla('S');
    COMPROBAR(ed.modo == 3 && fich_crear_n == 0);
    ed_refrescar_ok();
    {
        char f[ED_ANCHO + 1];
        fila_txt(ED_FILA_MSG, f);
        COMPROBAR(strstr(f, "partly loaded") != 0 && strstr(f, "LOSES") != 0);
        COMPROBAR(strlen(f) <= ED_ANCHO && estilos[ED_FILA_MSG][1] == ED_E_ERROR);
    }
    ed_tecla('n');
    COMPROBAR(ed.modo == 0 && fich_crear_n == 0 && eb_longitud() == ED_CAP);
    ed_tecla(ED_K_MENU); ed_tecla('S'); ed_tecla('y');
    COMPROBAR(ed.modo == 0 && fich_crear_n == 1 && strcmp(fich_nom_esc, "g.f") == 0 && esc_len == ED_CAP);
    ed_tecla('x');
    ed_tecla(ED_K_MENU); ed_tecla('S');                  /* ya guardado: no vuelve a preguntar */
    COMPROBAR(ed.modo == 0 && fich_crear_n == 2);

    /* A con el mismo nombre pregunta; con otro no (el original no se toca) */
    preparar();
    fich_reiniciar();
    fich_poner(gran, sizeof(gran));
    COMPROBAR(ed_cargar("g.f") == 3);
    fich_abrir_err = 2;                                  /* "g.f2" no existe: guardar no pregunta por sobrescribir */
    ed_tecla(ED_K_MENU); ed_tecla('A'); ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.modo == 3 && fich_crear_n == 0);
    ed_tecla(27);                                        /* Esc cancela */
    COMPROBAR(ed.modo == 0 && fich_crear_n == 0);
    ed_tecla(ED_K_MENU); ed_tecla('A'); ed_tecla('2'); ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.modo == 0 && fich_crear_n == 1 && strcmp(fich_nom_esc, "g.f2") == 0);
    COMPROBAR(strcmp(ed.nombre, "g.f2") == 0);
    ed_tecla(ED_K_MENU); ed_tecla('S');                  /* el nuevo nombre ya es completo */
    COMPROBAR(ed.modo == 0 && fich_crear_n == 2);

    /* fallo de lectura a mitad: tambien pregunta */
    preparar();
    fich_reiniciar();
    fich_poner(gran, 5000);
    fich_fallo_en = 2000;
    COMPROBAR(ed_cargar("r.f") == 2);
    ed_tecla(ED_K_MENU); ed_tecla('S');
    COMPROBAR(ed.modo == 3);
    ed_tecla('n');

    /* carga completa: no pregunta; nuevo y otra carga limpian la marca */
    preparar();
    fich_reiniciar();
    fich_poner(gran, 3000);
    COMPROBAR(ed_cargar("c.f") == 0);
    ed_tecla(ED_K_MENU); ed_tecla('S');
    COMPROBAR(ed.modo == 0 && fich_crear_n == 1);
    fich_poner(gran, sizeof(gran));
    COMPROBAR(ed_cargar("g.f") == 3);
    fich_poner(gran, 100);
    COMPROBAR(ed_cargar("p.f") == 0);
    n0 = fich_crear_n;
    ed_tecla(ED_K_MENU); ed_tecla('S');
    COMPROBAR(ed.modo == 0 && fich_crear_n == n0 + 1);
    fich_poner(gran, sizeof(gran));
    COMPROBAR(ed_cargar("g.f") == 3);
    ed_nuevo();
    fich_abrir_err = 2;
    ed_tecla('x'); ed_tecla(ED_K_MENU); ed_tecla('S'); teclas("z.f"); ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.modo == 0 && strcmp(fich_nom_esc, "z.f") == 0);
}


/* ------------------------------------------------------------------
 * Esc+C (compilar), Esc+X / Esc+Z (siguiente / anterior error)
 * ------------------------------------------------------------------ */

static void fk_errores(int n, const uint16_t *lineas, const uint8_t *cols, const uint8_t *cods, uint16_t total)
{
    int i;

    fk_aleatorio = 0;
    memset(fk_diag, 0, sizeof(fk_diag));
    for (i = 0; i < n; i++) {
        fk_diag[i].linea = lineas[i];
        fk_diag[i].columna = cols[i];
        fk_diag[i].cod = cods[i];
    }
    fk_n = total;
}

static void test_compilar(void)
{
    static const uint16_t L[3] = { 3, 5, 9 };
    static const uint8_t C[3] = { 7, 1, 255 }, K[3] = { 8, 21, 33 };
    char f[ED_ANCHO + 1];
    int i;

    /* sin errores */
    preparar();
    fich_reiniciar();
    ed_insertar_texto("      PROGRAM X\n      END\n");
    fk_errores(0, 0, 0, 0, 0);
    fk_llamadas = 0;
    ed_tecla(ED_K_MENU); ed_tecla('C');
    COMPROBAR(fk_llamadas == 1 && ed.msg != 0 && strcmp(ed.msg, "Checked OK: no errors") == 0 && ed.msg_err == 0);
    COMPROBAR(fk_lineas == 2);                               /* ed_linea: tras el ultimo salto no hay linea mas */
    COMPROBAR(strstr(fk_pantalla, "Checking") != 0);        /* se pinto antes de empezar */
    COMPROBAR(ed.modo == 0);
    ed_refrescar_ok();
    ed_tecla(ED_K_MENU); ed_tecla('X');                      /* nada que recorrer */
    COMPROBAR(strcmp(ed.msg, "No errors to show (Esc C checks)") == 0);

    /* tres errores: se va al primero y se recorre en circulo */
    preparar();
    fich_reiniciar();
    ed_insertar_texto("l1\nl2\nabcdefghij\nl4\nl5 largo\nl6\nl7\nl8\nl9\n");
    fk_errores(3, L, C, K, 3);
    ed_tecla(ED_K_MENU); ed_tecla('C');
    COMPROBAR(ed.linea == 2 && ed.pos - ed.ini_lin == 6 && ed.col_obj == 6);
    COMPROBAR(ed.msg_err == 1 && strcmp(ed.msg, "Error 1/3 at 3:7: wrong number of subscripts") == 0);
    ed_refrescar_ok();
    fila_txt(ED_FILA_MSG, f);
    COMPROBAR(strcmp(f, " Error 1/3 at 3:7: wrong number of subscripts") == 0 && estilos[ED_FILA_MSG][1] == ED_E_ERROR);
    COMPROBAR(cur_f == 1 + (int)(ed.linea - ed.top_linea) && cur_c == 6);
    ed_tecla(ED_K_MENU); ed_tecla('X');
    COMPROBAR(ed.linea == 4 && ed.pos == ed.ini_lin && strcmp(ed.msg, "Error 2/3 at 5:1: msg 21") == 0);
    ed_tecla(ED_K_MENU); ed_tecla('x');
    COMPROBAR(ed.linea == 8 && ed.pos == eb_fin_linea(ed.pos) && strcmp(ed.msg, "Error 3/3 at 9:255: msg 33") == 0);
    ed_refrescar_ok();                                       /* columna 255: al final de la linea */
    ed_tecla(ED_K_MENU); ed_tecla('X');
    COMPROBAR(ed.linea == 2 && strncmp(ed.msg, "Error 1/3", 9) == 0);        /* vuelve al primero */
    ed_tecla(ED_K_MENU); ed_tecla('Z');
    COMPROBAR(ed.linea == 8 && strncmp(ed.msg, "Error 3/3", 9) == 0);        /* y hacia atras da la vuelta */
    ed_tecla(ED_K_MENU); ed_tecla('Z');
    COMPROBAR(ed.linea == 4 && strncmp(ed.msg, "Error 2/3", 9) == 0);
    ed_refrescar_ok();

    /* mover el cursor no invalida; escribir si */
    ed_tecla(ED_K_ARR); ed_tecla(ED_K_DER);
    ed_tecla(ED_K_MENU); ed_tecla('X');
    COMPROBAR(ed.linea == 8 && strncmp(ed.msg, "Error 3/3", 9) == 0);
    ed_tecla('q');
    ed_tecla(ED_K_MENU); ed_tecla('X');
    COMPROBAR(ed.msg_err == 1 && strcmp(ed.msg, "Text changed: check again (Esc C)") == 0);
    ed_tecla(ED_K_MENU); ed_tecla('X');
    COMPROBAR(strcmp(ed.msg, "No errors to show (Esc C checks)") == 0);
    ed_tecla(ED_K_MENU); ed_tecla('C');
    COMPROBAR(strncmp(ed.msg, "Error 1/3", 9) == 0);
    ed_tecla(ED_K_BORRAR);                                   /* borrar tambien cambia el texto */
    ed_tecla(ED_K_MENU); ed_tecla('Z');
    COMPROBAR(strcmp(ed.msg, "Text changed: check again (Esc C)") == 0);

    /* sin compilar antes: X y Z avisan */
    preparar();
    ed_tecla(ED_K_MENU); ed_tecla('Z');
    COMPROBAR(strcmp(ed.msg, "No errors to show (Esc C checks)") == 0);

    /* mas errores de los que guarda el compilador: 21 en total, se recorren los 16 guardados */
    {
        uint16_t l16[16]; uint8_t c16[16], k16[16];
        for (i = 0; i < 16; i++) { l16[i] = (uint16_t)(i + 1); c16[i] = (uint8_t)(i + 1); k16[i] = (uint8_t)i; }
        preparar();
        for (i = 0; i < 20; i++) ed_insertar_texto("linea de texto\n");
        fk_errores(16, l16, c16, k16, 21);
        ed_tecla(ED_K_MENU); ed_tecla('C');
        COMPROBAR(strncmp(ed.msg, "Error 1/21 at 1:1", 17) == 0);
        for (i = 0; i < 15; i++) { ed_tecla(ED_K_MENU); ed_tecla('X'); ed_refrescar_ok(); }
        COMPROBAR(strncmp(ed.msg, "Error 16/21 at 16:16", 20) == 0 && ed.linea == 15);
        ed_tecla(ED_K_MENU); ed_tecla('X');
        COMPROBAR(strncmp(ed.msg, "Error 1/21", 10) == 0);
    }

    /* una linea de error mas alla del final: va a la ultima; salto lejano: se centra */
    {
        static const uint16_t l1[1] = { 500 }; static const uint8_t c1[1] = { 3 }, k1[1] = { 1 };
        preparar();
        for (i = 0; i < 100; i++) ed_insertar_texto("abc\n");
        ed_tecla(ED_K_MENU); ed_tecla('T');
        fk_errores(1, l1, c1, k1, 1);
        ed_tecla(ED_K_MENU); ed_tecla('C');
        COMPROBAR(ed.linea == 100 && ed.pos == eb_longitud());
        ed_refrescar_ok();
        {
            static const uint16_t l2[1] = { 50 };
            fk_errores(1, l2, c1, k1, 1);
            ed_tecla(ED_K_MENU); ed_tecla('T');
            ed_refrescar_ok();
            ed_tecla(ED_K_MENU); ed_tecla('C');
            ed_refrescar_ok();
            COMPROBAR(ed.linea == 49 && ed.linea >= ed.top_linea && ed.linea < ed.top_linea + ED_FILAS_TEXTO);
        }
    }

    /* columnas 1, 2 y 3 caen en los caracteres 0, 1 y 2 de la linea */
    {
        static const uint16_t l3[3] = { 1, 1, 1 }; static const uint8_t c3[3] = { 1, 2, 3 }, k3[3] = { 1, 2, 3 };
        preparar();
        ed_insertar_texto("abcd\n");
        fk_errores(3, l3, c3, k3, 3);
        ed_tecla(ED_K_MENU); ed_tecla('C');
        COMPROBAR(ed.pos == 0);
        ed_tecla(ED_K_MENU); ed_tecla('X');
        COMPROBAR(ed.pos == 1 && ed.col_obj == 1);
        ed_tecla(ED_K_MENU); ed_tecla('X');
        COMPROBAR(ed.pos == 2 && ed.col_obj == 2);
    }

    /* texto vacio */
    preparar();
    {
        static const uint16_t l1[1] = { 1 }; static const uint8_t c1[1] = { 7 }, k1[1] = { 32 };
        fk_errores(1, l1, c1, k1, 1);
        ed_tecla(ED_K_MENU); ed_tecla('C');
        COMPROBAR(ed.linea == 0 && ed.pos == 0 && strncmp(ed.msg, "Error 1/1", 9) == 0);
        ed_refrescar_ok();
    }

    /* la barra de ayuda lo menciona */
    preparar();
    ed_refrescar_ok();
    fila_txt(ED_FILA_AYUDA, f);
    COMPROBAR(strstr(f, "C check") != 0 && strstr(f, "O build") != 0 && strstr(f, "X/Z err") != 0 && strlen(f) <= ED_ANCHO);
    ed_tecla(ED_K_MENU);
    ed_refrescar_ok();
    fila_txt(ED_FILA_MSG, f);
    COMPROBAR(strstr(f, "C check") != 0 && strstr(f, "O build") != 0 && strstr(f, "X/Z next/prev error") != 0 && strlen(f) <= ED_ANCHO);
    fk_aleatorio = 0;
}


/* ------------------------------------------------------------------
 * Construir (Esc+O): MfSalida + nombre del .nex
 * ------------------------------------------------------------------ */

static const uint8_t src_prog[] = "      PROGRAM X\n      END\n";

/* el .nex falso: 264 trozos de 64 B con el numero de trozo repetido */
static int nex_correcto(void)
{
    int k;

    if (esc_len != 16896) return 0;
    for (k = 0; k < 264; k++)
        if (esc_datos[k * 64] != (k & 0xFF) || esc_datos[k * 64 + 63] != (k & 0xFF)) return 0;
    return 1;
}

static void cargar_prog(const char *nombre)
{
    fich_reiniciar();
    fich_poner(src_prog, sizeof(src_prog) - 1);
    ed_cargar(nombre);
    fich_abrir_err = 2;                                  /* ahora "nada existe" */
    fich_crear_n = 0;
    esc_len = 0;
    fk_errores(0, 0, 0, 0, 0);
    fk_abrir_falla = 0;
    fk_salida = 0;
    fk_reg_n = 0;
}

static void test_construir(void)
{
    static const uint16_t L[1] = { 2 };
    static const uint8_t C[1] = { 1 }, K[1] = { 8 };
    char f[ED_ANCHO + 1];
    uint16_t pos0, l0;
    int n0;

    /* comprobar (C) no abre nada ni registra salida */
    preparar();
    cargar_prog("prueba.f");
    fk_llamadas = 0;
    ed_tecla(ED_K_MENU); ed_tecla('C');
    COMPROBAR(fk_llamadas == 1 && fk_reg_en_llamada == 0 && fk_reg_n == 0 && fich_crear_n == 0);
    COMPROBAR(mf.gen == MF_GEN_NO && strcmp(ed.msg, "Checked OK: no errors") == 0);

    /* construir sin errores, con nombre: prueba.nex, 16896 B, cerrado, mensaje */
    ed.modificado = 0;
    pos0 = ed.pos; l0 = ed.linea;
    fk_llamadas = 0; fk_reg_n = 0;
    ed_tecla(ED_K_MENU); ed_tecla('O');
    COMPROBAR(fk_llamadas == 1 && fk_reg_en_llamada == 1 && fk_reg_n == 2 && fk_salida == 0);
    COMPROBAR(fich_crear_n == 1 && strcmp(fich_nom_esc, "prueba.nex") == 0 && esc_abiertos == 0);
    COMPROBAR(nex_correcto());
    COMPROBAR(mf.gen == MF_GEN_OK && ed.msg_err == 0 && strcmp(ed.msg, "Built prueba.nex") == 0);
    COMPROBAR(ed.modificado == 0 && ed.pos == pos0 && ed.linea == l0 && eb_longitud() == sizeof(src_prog) - 1);
    COMPROBAR(ed.modo == 0 && strcmp(ed.nombre, "prueba.f") == 0);
    ed_refrescar_ok();
    fila_msg(f);
    COMPROBAR(strcmp(f, " Built prueba.nex") == 0);

    /* mensaje mientras trabaja */
    COMPROBAR(strstr(fk_pantalla, "Building") != 0);

    /* un segundo O pisa el .nex sin preguntar */
    esc_len = 0;
    ed_tecla(ED_K_MENU); ed_tecla('O');
    COMPROBAR(fich_crear_n == 2 && nex_correcto() && esc_abiertos == 0);

    /* aunque el .nex exista (fich_abrir_err = 0): sin preguntar, tiene nombre de fuente */
    fich_abrir_err = 0;
    ed_tecla(ED_K_MENU); ed_tecla('O');
    COMPROBAR(ed.modo == 0 && fich_crear_n == 3);

    /* con errores: no abre nada, muestra el error, gen = NO */
    fich_abrir_err = 2;
    fk_errores(1, L, C, K, 1);
    n0 = fich_crear_n;
    ed_tecla(ED_K_MENU); ed_tecla('O');
    COMPROBAR(fich_crear_n == n0 && fk_reg_en_llamada == 1 && fk_salida == 0 && mf.gen == MF_GEN_NO);
    COMPROBAR(ed.msg_err == 1 && strncmp(ed.msg, "Error 1/1 at 2:1: ", 18) == 0 && ed.linea == 1);
    ed_tecla(ED_K_MENU); ed_tecla('X');                  /* y los errores se recorren igual */
    COMPROBAR(strncmp(ed.msg, "Error 1/1", 9) == 0);

    /* fallo al escribir (disco lleno): "Build failed", cerrado, texto intacto */
    fk_errores(0, 0, 0, 0, 0);
    fich_esc_limite = 1000;
    ed_tecla(ED_K_MENU); ed_tecla('O');
    COMPROBAR(mf.gen == MF_GEN_ERR && esc_abiertos == 0 && ed.msg_err == 1);
    COMPROBAR(strcmp(ed.msg, "Build failed: cannot write file") == 0);
    ed_refrescar_ok();
    COMPROBAR(estilos[ED_FILA_MSG][1] == ED_E_ERROR);
    COMPROBAR(eb_longitud() == sizeof(src_prog) - 1);

    /* fallo al crear el fichero */
    fich_esc_limite = 0;
    fich_crear_err = 5;
    ed_tecla(ED_K_MENU); ed_tecla('O');
    COMPROBAR(mf.gen == MF_GEN_ERR && esc_abiertos == 0 && ed.msg_err == 1);
    COMPROBAR(strcmp(ed.msg, "Build failed: cannot create file") == 0);
    fich_crear_err = 0;

    /* despues de un fallo se puede reintentar */
    ed_tecla(ED_K_MENU); ed_tecla('O');
    COMPROBAR(mf.gen == MF_GEN_OK && nex_correcto());

    /* sin nombre: lo pide */
    preparar();
    fich_reiniciar();
    fk_errores(0, 0, 0, 0, 0);
    fk_salida = 0;
    teclas("      END");
    ed_tecla(ED_K_MENU); ed_tecla('O');
    COMPROBAR(ed.modo == 2 && fich_crear_n == 0);
    fila_msg(f);
    COMPROBAR(strcmp(f, " Build as:") == 0 && cur_c == 11);
    ed_tecla(ED_K_ENTER);                                /* vacio */
    COMPROBAR(ed.modo == 0 && ed.msg_err == 1 && strcmp(ed.msg, "No file name") == 0 && fich_crear_n == 0);
    ed_tecla(ED_K_MENU); ed_tecla('O'); teclas("hola"); ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.modo == 0 && fich_crear_n == 1 && strcmp(fich_nom_esc, "hola.nex") == 0 && nex_correcto());
    COMPROBAR(strcmp(ed.msg, "Built hola.nex") == 0 && ed.nombre == 0);
    /* el segundo O reutiliza el nombre sin preguntar */
    ed_tecla(ED_K_MENU); ed_tecla('O');
    COMPROBAR(ed.modo == 0 && fich_crear_n == 2 && strcmp(fich_nom_esc, "hola.nex") == 0);
    /* Cancelar el nombre con Esc */
    ed_tecla(ED_K_MENU); ed_tecla('N');                  /* texto modificado: pregunta */
    ed_tecla('y');
    COMPROBAR(ed.nombre == 0 && ed.modo == 0);
    ed_tecla(ED_K_MENU); ed_tecla('O');                  /* nuevo: otra vez pide */
    COMPROBAR(ed.modo == 2);
    ed_tecla(ED_K_MENU);
    COMPROBAR(ed.modo == 0 && strcmp(ed.msg, "Cancelled") == 0 && fich_crear_n == 2);

    /* nombre que ya existe: confirmacion */
    fich_abrir_err = 0;
    ed_tecla(ED_K_MENU); ed_tecla('O'); teclas("hola.nex"); ed_tecla(ED_K_ENTER);
    COMPROBAR(ed.modo == 3 && fich_crear_n == 2);
    fila_msg(f);
    COMPROBAR(strcmp(f, " File exists. Overwrite? (Y/N)") == 0);
    ed_tecla('n');
    COMPROBAR(ed.modo == 0 && fich_crear_n == 2 && strcmp(ed.msg, "Cancelled") == 0);
    ed_tecla(ED_K_MENU); ed_tecla('O'); teclas("hola.nex"); ed_tecla(ED_K_ENTER);
    ed_tecla('Y');
    COMPROBAR(ed.modo == 0 && fich_crear_n == 3 && nex_correcto() && strcmp(ed.msg, "Built hola.nex") == 0);
    fich_abrir_err = 2;

    /* Nuevo olvida el nombre */
    ed_nuevo();
    ed_tecla(ED_K_MENU); ed_tecla('O');
    COMPROBAR(ed.modo == 2);
    ed_tecla(ED_K_MENU);

    /* extension tecleada: se sustituye por .nex */
    ed_tecla(ED_K_MENU); ed_tecla('O'); teclas("mi.prog.f"); ed_tecla(ED_K_ENTER);
    COMPROBAR(strcmp(fich_nom_esc, "mi.prog.nex") == 0);

    /* derivacion del nombre a partir del fuente */
    {
        static const char *const fu[] = { "a.f", "a.b.f", "prog", "dir/x.f", "dir.d/x", ".f", "d.d\\x", "x.", "a.nex", "ab.cd" };
        static const char *const nx[] = { "a.nex", "a.b.nex", "prog.nex", "dir/x.nex", "dir.d/x.nex", ".f.nex", "d.d\\x.nex", "x.nex", "", "ab.nex" };
        int i;

        for (i = 0; i < 10; i++) {
            preparar();
            cargar_prog(fu[i]);
            ed_tecla(ED_K_MENU); ed_tecla('O');
            if (nx[i][0] == 0) {                         /* "a.nex": construiria encima del fuente */
                COMPROBAR(fich_crear_n == 0 && ed.msg_err == 1 && strcmp(ed.msg, "Output name is the source file") == 0);
            } else {
                COMPROBAR(fich_crear_n == 1 && strcmp(fich_nom_esc, nx[i]) == 0);
            }
        }
    }

    /* el nombre del .nex sigue al del fuente si este cambia (solo el tecleado queda fijo) */
    preparar();
    cargar_prog("a.f");
    ed_tecla(ED_K_MENU); ed_tecla('O');
    COMPROBAR(strcmp(fich_nom_esc, "a.nex") == 0);
    ed_tecla(ED_K_MENU); ed_tecla('A'); ed_tecla(ED_K_BORRAR); ed_tecla(ED_K_BORRAR); ed_tecla(ED_K_BORRAR);
    teclas("b.f"); ed_tecla(ED_K_ENTER);
    ed_tecla(ED_K_MENU); ed_tecla('O');
    COMPROBAR(strcmp(fich_nom_esc, "b.nex") == 0);

    /* tras Nuevo, el nombre del fuente anterior ya no cuenta: se puede teclear igual que el .nex */
    preparar();
    cargar_prog("x.nex");
    ed_nuevo();
    ed_tecla(ED_K_MENU); ed_tecla('O'); teclas("x"); ed_tecla(ED_K_ENTER);
    COMPROBAR(fich_crear_n == 1 && strcmp(fich_nom_esc, "x.nex") == 0);

    /* nombre largo: se recorta y sigue acabando en .nex */
    preparar();
    cargar_prog("0123456789012345678901234567890.f");     /* 31 caracteres: el maximo del nombre */
    ed_tecla(ED_K_MENU); ed_tecla('O');
    COMPROBAR(fich_crear_n == 1 && strlen(fich_nom_esc) <= 36 && strcmp(fich_nom_esc + strlen(fich_nom_esc) - 4, ".nex") == 0);
    COMPROBAR(strncmp(fich_nom_esc, "0123456789012345678901234567890", 31) == 0);

    /* el nombre tecleado no puede ser el del fuente */
    preparar();
    cargar_prog("x.nex");
    ed_tecla(ED_K_MENU); ed_tecla('O');
    COMPROBAR(fich_crear_n == 0 && strcmp(ed.msg, "Output name is the source file") == 0);
    preparar();
    cargar_prog("x.f");
    ed_tecla(ED_K_MENU); ed_tecla('A'); ed_tecla(ED_K_BORRAR);
    teclas("nex"); ed_tecla(ED_K_ENTER);                  /* x.nex pasa a ser el fuente */
    n0 = fich_crear_n;
    fich_abrir_err = 2;
    ed_tecla(ED_K_MENU); ed_tecla('O');
    COMPROBAR(fich_crear_n == n0 && strcmp(ed.msg, "Output name is the source file") == 0);

    /* el compilador no genera aunque se le pida (gen = NO sin errores) */
    preparar();
    cargar_prog("h.f");
    fk_no_gen = 1;
    ed_tecla(ED_K_MENU); ed_tecla('O');
    COMPROBAR(mf.gen == MF_GEN_NO && ed.msg_err == 0 && strcmp(ed.msg, "Checked OK, nothing built") == 0 && fich_crear_n == 0);
    fk_no_gen = 0;

    /* el compilador falla al abrir */
    preparar();
    cargar_prog("g.f");
    fk_abrir_falla = 1;
    ed_tecla(ED_K_MENU); ed_tecla('O');
    COMPROBAR(mf.gen == MF_GEN_ERR && strcmp(ed.msg, "Build failed: cannot create file") == 0 && fich_crear_n == 0);
    fk_abrir_falla = 0;
}

/* Mezcla de teclas de menu con fallos de ficheros al azar: las invariantes se mantienen */
static void test_menus_aleatorio(void)
{
    static const uint8_t datos[] = "C HOLA\n      END\n";
    int k, j;

    for (k = 0; k < 6; k++) {
        semilla = 900 + (uint32_t)k;
        preparar();
        fich_reiniciar();
        fich_poner(datos, sizeof(datos) - 1);
        for (j = 0; j < 20000; j++) {
            uint32_t r = rnd(100);

            if (r < 30) ed_tecla(' ' + (int)rnd(95));
            else if (r < 40) ed_tecla(ED_K_MENU);
            else if (r < 45) ed_tecla(27);
            else if (r < 52) ed_tecla("NLSAYnlsay"[rnd(10)]);
            else if (r < 62) ed_tecla(ED_K_ENTER);
            else if (r < 72) ed_tecla(ED_K_BORRAR);
            else if (r < 80) ed_tecla(ED_K_IZQ + (int)rnd(4));
            else if (r < 85) { fich_abrir_err = (int)rnd(3); fich_poner(datos, sizeof(datos) - 1); fich_abrir_err = (int)rnd(3); }
            else if (r < 90) fich_crear_err = (rnd(3) == 0) ? 7 : 0;
            else if (r < 94) fich_esc_limite = (rnd(2) == 0) ? 0 : rnd(40);
            else ed_tecla(0);
            ed_refrescar();
            if (ed_verificar() != 0 || violaciones != 0) {
                COMPROBAR(ed_verificar() == 0);
                COMPROBAR(violaciones == 0);
                break;
            }
            if (fich_abiertos != 0 || esc_abiertos != 0) {
                COMPROBAR(fich_abiertos == 0 && esc_abiertos == 0);
                break;
            }
        }
        pruebas++;
    }
}

int main(void)
{
    int s;

    test_buffer_basico();
    test_buffer_lleno_total();
    test_buffer_linea_larga();
    test_buffer_aleatorio(1, 40, 30000);
    test_buffer_aleatorio(2, 400, 30000);
    test_buffer_aleatorio(3, 3000, 30000);
    test_buffer_aleatorio(4, 2, 30000);

    test_cargar_basico();
    test_cargar_vacio_y_sin_final();
    test_cargar_error_apertura();
    test_cargar_grande();
    test_cargar_no_cabe();
    test_cargar_fallo_lectura();
    test_tabuladores();

    test_escribir();
    test_cursor();
    test_comando_desconocido();
    test_limite_columna();
    test_scroll_horizontal();
    test_resaltado();
    test_scroll_vertical();
    test_caracteres_raros();
    test_estado();
    test_repintado_minimo();
    test_ed_linea();
    test_guardar_basico();
    test_guardar_errores();
    test_guardar_ida_y_vuelta();
    test_menu_nuevo();
    test_menu_cargar();
    test_menu_guardar();
    test_ayuda_ficheros();
    test_menus_aleatorio();
    test_ir_a_linea();
    test_paginas_ajenas();
    test_borrar_linea();
    test_compilar();
    test_construir();
    test_borrar_linea_modelo();
    test_guardar_tras_carga_incompleta();

    /* El pintor ingenuo es cuadratico en el tamano del texto: capacidades pequenas
     * (se llena a menudo) y pocas teclas con la capacidad completa. */
    for (s = 0; s < 6; s++) test_aleatorio(100 + s, 600, 15000);
    for (s = 0; s < 3; s++) test_aleatorio(200 + s, 5000, 15000);
    for (s = 0; s < 2; s++) test_aleatorio(300 + s, ED_CAP, 4000);
    for (s = 0; s < 4; s++) test_aleatorio(400 + s, 1, 6000);          /* parte de un fichero cargado */

    printf("test_ed01: %d pruebas, %d fallos\n", pruebas, fallos);
    return fallos != 0;
}
