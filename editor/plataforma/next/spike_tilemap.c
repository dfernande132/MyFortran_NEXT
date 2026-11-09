/* spike_tilemap.c - SPIKE: pantalla 80x32 en modo tilemap con color por caracter.
 *
 * Para decidir si el editor pasa de 64 columnas (ULA: color por celda de 8x8 = 2
 * caracteres) a 80 columnas (tilemap: color por caracter). No es parte del editor.
 *
 * Que se sabe (docs/referencia, seccion 8, medido por Dani en MyLISP):
 *   - modo 66 + font_8x8_bbc_system cargada a mano da 80x32 con letra de 8x8
 *   - textcolor() (primer plano) funciona; textbackground() no
 *   - la memoria de la tilemap empieza hacia $6C00; formato del atributo SIN CONFIRMAR
 * Que NO se sabe y este spike mide (todo hipotesis hasta verlo en pantalla):
 *   H-T1  los registros $6B/$6E/$6F dicen donde esta el mapa y si hay byte de atributo
 *   H-T2  el byte que sigue a cada caracter es el atributo y elige la paleta
 *   H-T3  (hip. A) modo texto: atributo = estilo<<1, y el pixel de la fuente (1 bit)
 *         completa el indice: fondo = paleta[2*estilo], tinta = paleta[2*estilo+1]
 *   H-T4  (hip. B) atributo = estilo<<4, fondo = paleta[16*estilo], tinta = +1
 *   H-T5  NextREG $43 = $30 (+$40 si $6B bit 4) selecciona la paleta del tilemap
 *         y $40/$41 con autoincremento la escriben (formato RRRGGGBB)
 *
 * Pantallas (teclas):
 *   1  diagnostico: registros, autocomprobacion del mapa y rejilla de los 256 atributos
 *   2  maqueta con SOLO textcolor() (lo ya confirmado: sin fondo por celda)
 *   3  maqueta con paleta propia, hipotesis A
 *   4  maqueta con paleta propia, hipotesis B
 * La paleta del tilemap queda modificada al pasar por 3 o 4; reiniciar para volver.
 *
 * NO se ha compilado ni ejecutado cuando se escribio (la nube no tiene z88dk).
 */
#include <stdint.h>
#include <conio.h>
#include <sys/ioctl.h>
#include <input.h>
#include <arch/zxn.h>

extern unsigned char font_8x8_bbc_system[];

#define R_PAL_IDX   0x40
#define R_PAL_VAL   0x41
#define R_PAL_CTL   0x43
#define R_TM_CTL    0x6B
#define R_TM_ATTR   0x6C
#define R_TM_MAP    0x6E
#define R_TM_DEF    0x6F

/* Estilos (indices en paleta[]) */
#define E_TEXTO   0
#define E_COMENT  1
#define E_ETIQ    2
#define E_CONT    3
#define E_CLAVE   4
#define E_CADENA  5
#define E_NUM     6
#define E_FUERA   7     /* columnas 73+: F77 las ignora */
#define E_ESTADO  8
#define E_AYUDA   9
#define E_CURSOR  10
#define E_ERROR   11
#define N_ESTILOS 12

/* {fondo, tinta} en RRRGGGBB */
static const uint8_t paleta[N_ESTILOS][2] = {
    { 0x25, 0xDA },  /* texto: azul marino / blanco roto */
    { 0x25, 0x6E },  /* comentario: gris azulado */
    { 0x25, 0xFC },  /* etiqueta: amarillo */
    { 0x25, 0xE8 },  /* continuacion: rojo anaranjado */
    { 0x25, 0xF2 },  /* palabra clave: rosa */
    { 0x25, 0x7D },  /* cadena: verde */
    { 0x25, 0xF5 },  /* numero: naranja */
    { 0x25, 0x4A },  /* fuera de columna 72: gris tenue */
    { 0xDB, 0x25 },  /* barra de estado: gris claro / marino */
    { 0x4F, 0xFF },  /* ayuda: azul medio / blanco */
    { 0xFC, 0x25 },  /* cursor: amarillo / marino */
    { 0xA0, 0xFF }   /* error: rojo oscuro / blanco */
};

/* Estado del spike (una sola variable global, como el resto del proyecto) */
static struct {
    uint8_t *mapa;      /* direccion CPU del mapa */
    uint8_t  celda;     /* bytes por celda: 2 (caracter+atributo) o 1 */
    uint8_t  cols;      /* 80 o 40 */
    uint8_t  ok;        /* 0 si el mapa no es accesible directamente */
    uint8_t  at_base;   /* atributo que escribe la consola para texto normal */
    uint8_t  hip;       /* 0 = hipotesis A (estilo<<1), 1 = B (estilo<<4) */
    uint8_t  r6b, r6c, r6e, r6f, r43;
} sp;

/* ---- escritura directa en el mapa ---- */

static void poner(uint8_t f, uint8_t c, uint8_t ch, uint8_t at)
{
    uint8_t *p;
    if (!sp.ok || c >= sp.cols || f >= 32) return;
    p = sp.mapa + ((uint16_t)f * sp.cols + c) * sp.celda;
    p[0] = ch;
    if (sp.celda == 2) p[1] = at;
}

static void cadena(uint8_t f, uint8_t c, const char *s, uint8_t at)
{
    while (*s) { poner(f, c, (uint8_t)*s, at); ++c; ++s; }
}

static void rellenar(uint8_t f, uint8_t c0, uint8_t c1, uint8_t at)
{
    uint8_t c;
    for (c = c0; c < c1; ++c) poner(f, c, ' ', at);
}

static void hex2(uint8_t f, uint8_t c, uint8_t v, uint8_t at)
{
    static const char d[] = "0123456789ABCDEF";
    poner(f, c, (uint8_t)d[v >> 4], at);
    poner(f, (uint8_t)(c + 1), (uint8_t)d[v & 15], at);
}

static uint8_t atr(uint8_t estilo)
{
    return sp.hip ? (uint8_t)(estilo << 4) : (uint8_t)(estilo << 1);
}

static void limpiar(uint8_t at)
{
    uint8_t f;
    for (f = 0; f < 32; ++f) rellenar(f, 0, sp.cols, at);
}

/* ---- paleta ---- */

static void paleta_cargar(void)
{
    uint8_t s, sel;
    sel = (uint8_t)(0x30 | ((sp.r6b & 0x10) ? 0x40 : 0));   /* H-T5 */
    ZXN_WRITE_REG(R_PAL_CTL, sel);
    for (s = 0; s < N_ESTILOS; ++s) {
        ZXN_WRITE_REG(R_PAL_IDX, sp.hip ? (uint8_t)(s << 4) : (uint8_t)(s << 1));
        ZXN_WRITE_REG(R_PAL_VAL, paleta[s][0]);
        ZXN_WRITE_REG(R_PAL_VAL, paleta[s][1]);
    }
    ZXN_WRITE_REG(R_PAL_CTL, sp.r43);
}

/* ---- clasificacion de una linea de F77 en estilos ---- */

static const char *claves[] = {
    "PROGRAM", "INTEGER", "REAL", "DO", "CONTINUE", "IF", "THEN", "ELSE",
    "END", "PRINT", "GOTO", "STOP", 0
};

static uint8_t es_letra(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
static uint8_t es_digito(char c) { return c >= '0' && c <= '9'; }

/* Rellena est[i] con el estilo de cada caracter de s. */
static void clasificar(const char *s, uint8_t *est)
{
    uint8_t n, i, j, k, enstr, restante;
    const char *w;

    for (n = 0; s[n]; ++n) { }
    if (s[0] == 'C' || s[0] == 'c' || s[0] == '*') {
        for (i = 0; i < n; ++i) est[i] = E_COMENT;
        return;
    }
    enstr = 0;
    restante = 0;
    for (i = 0; i < n; ++i) {
        if (i >= 72) { est[i] = E_FUERA; continue; }
        if (i < 5) { est[i] = es_digito(s[i]) ? E_ETIQ : E_TEXTO; continue; }
        if (i == 5) { est[i] = (s[i] != ' ') ? E_CONT : E_TEXTO; continue; }
        if (s[i] == '\'') {
            est[i] = E_CADENA;
            enstr = !enstr;
            continue;
        }
        if (enstr) { est[i] = E_CADENA; continue; }
        if (restante) { est[i] = E_CLAVE; --restante; continue; }
        est[i] = E_TEXTO;
        if (es_letra(s[i]) && (i == 6 || !es_letra(s[i - 1]))) {
            for (j = i; es_letra(s[j]); ++j) { }
            for (k = 0; claves[k]; ++k) {
                w = claves[k];
                for (restante = 0; w[restante] && s[i + restante] == w[restante]; ++restante) { }
                if (w[restante] == 0 && (uint8_t)(i + restante) == j) {
                    est[i] = E_CLAVE;
                    restante = (uint8_t)(restante - 1);
                    break;
                }
                restante = 0;
            }
        } else if (es_digito(s[i]) && !es_letra(s[i - 1])) {
            est[i] = E_NUM;
        }
    }
}

static const char *prueba[] = {
    "C     PRUEBA DEL EDITOR DE MYFORTRAN",
    "C     CALCULA LOS PRIMEROS TERMINOS DE FIBONACCI",
    "      PROGRAM FIBO",
    "      INTEGER N, I, A, B, T",
    "      N = 10",
    "      A = 0",
    "      B = 1",
    "      DO 10 I = 1, N",
    "         PRINT *, I, A",
    "         T = A + B",
    "         A = B",
    "         B = T",
    "   10 CONTINUE",
    "      IF (N .GT. 5) THEN",
    "         PRINT *, 'N ES MAYOR QUE 5'",
    "      ELSE",
    "         PRINT *, 'N ES PEQUENO'",
    "      END IF",
    "      X = 1.0 + 2.0 * 3.0 - 4.0 / 5.0 + 6.0 ** 2 + 7.0 + 8.0 + 9.0 +",
    "     +    10.0 + 11.0",
    "      END",
    0
};

/* ---- pantallas ---- */

static void pantalla_diag(void)
{
    uint8_t f, c, a, t;

    limpiar(sp.at_base);
    t = sp.at_base;
    cadena(0, 0, "SPIKE TILEMAP - 1 diag  2 textcolor  3 paleta A  4 paleta B", t);
    cadena(2, 0, "6B=", t);  hex2(2, 3, sp.r6b, t);
    cadena(2, 7, "6C=", t);  hex2(2, 10, sp.r6c, t);
    cadena(2, 14, "6E=", t); hex2(2, 17, sp.r6e, t);
    cadena(2, 21, "6F=", t); hex2(2, 24, sp.r6f, t);
    cadena(2, 28, "43=", t); hex2(2, 31, sp.r43, t);
    cadena(3, 0, (sp.r6b & 0x80) ? "tilemap ON " : "tilemap OFF ", t);
    cadena(3, 12, (sp.r6b & 0x40) ? "80 col " : "40 col ", t);
    cadena(3, 19, (sp.r6b & 0x20) ? "SIN atributo en mapa " : "atributo en mapa ", t);
    cadena(3, 40, (sp.r6b & 0x08) ? "modo texto " : "modo normal ", t);
    cadena(3, 52, (sp.r6b & 0x10) ? "paleta 2" : "paleta 1", t);
    cadena(4, 0, "mapa en $", t);
    hex2(4, 9, (uint8_t)((uint16_t)sp.mapa >> 8), t);
    hex2(4, 11, (uint8_t)((uint16_t)sp.mapa & 0xFF), t);
    cadena(4, 16, "bytes/celda ", t);
    poner(4, 28, (uint8_t)('0' + sp.celda), t);
    cadena(4, 32, "mapa[0] (esperado 53='S') = ", t);
    hex2(4, 60, sp.ok ? sp.mapa[0] : 0, t);
    cadena(5, 0, "atributo que escribe la consola para texto blanco = ", t);
    hex2(5, 52, sp.at_base, t);
    if (!sp.ok) cadena(5, 56, "MAPA NO ACCESIBLE DIRECTAMENTE (bit 6 de $6E): este spike no sirve tal cual", t);

    cadena(6, 0, "Rejilla: cada bloque EEE usa como atributo fila*16+columna (hex en los bordes)", t);
    for (c = 0; c < 16; ++c) hex2(7, (uint8_t)(6 + c * 4), c, t);
    for (f = 0; f < 16; ++f) {
        hex2((uint8_t)(8 + f), 2, (uint8_t)(f << 4), t);
        for (c = 0; c < 16; ++c) {
            a = (uint8_t)((f << 4) | c);
            cadena((uint8_t)(8 + f), (uint8_t)(6 + c * 4), "EEE", a);
        }
    }
    cadena(25, 0, "Si el atributo elige colores se ven bloques distintos; captura la pantalla.", t);
}

static void pantalla_textcolor(void)
{
    uint8_t f, i, n, ant;
    uint8_t est[90];
    /* estilo -> textcolor() de conio: 0 negro 1 azul 2 verde 3 cian 4 rojo 5 magenta 6 amarillo 7 blanco */
    static const uint8_t col[N_ESTILOS] = { 7, 3, 6, 4, 5, 2, 6, 1, 7, 7, 7, 4 };

    textbackground(1);
    textcolor(7);
    clrscr();
    gotoxy(0, 0);
    ant = 255;
    for (f = 0; prueba[f]; ++f) {
        clasificar(prueba[f], est);
        gotoxy(0, (uint8_t)(f + 2));
        for (n = 0; prueba[f][n]; ++n) { }
        for (i = 0; i < n; ++i) {
            if (est[i] != ant) { textcolor(col[est[i]]); ant = est[i]; }
            putchar(prueba[f][i]);
        }
    }
    textcolor(7);
    gotoxy(0, 0);
    cputs("SOLO textcolor(): sin fondo por celda (barra de estado imposible)  1 diag 3/4 paleta");
}

static void pantalla_maqueta(void)
{
    uint8_t f, i, n;
    uint8_t est[90];

    paleta_cargar();
    limpiar(atr(E_TEXTO));
    /* barra de estado */
    rellenar(0, 0, sp.cols, atr(E_ESTADO));
    cadena(0, 1, "MYFORTRAN   prueba.f", atr(E_ESTADO));
    cadena(0, 56, "L:9  C:13   INS   *", atr(E_ESTADO));
    cadena(0, 76, sp.hip ? "[B]" : "[A]", atr(E_ESTADO));
    /* texto */
    for (f = 0; prueba[f]; ++f) {
        clasificar(prueba[f], est);
        for (n = 0; prueba[f][n]; ++n) { }
        for (i = 0; i < n; ++i) poner((uint8_t)(f + 1), i, (uint8_t)prueba[f][i], atr(est[i]));
    }
    /* cursor de bloque sobre el 1.er caracter de PRINT (fila 9, col 9) */
    poner(9, 9, 'P', atr(E_CURSOR));
    /* regla de columnas: 1-5 etiqueta, 6 continuacion, 7-72 codigo, 73+ ignorado */
    cadena(24, 0, "12345", atr(E_ETIQ));
    cadena(24, 5, "6", atr(E_CONT));
    cadena(24, 6, "789012345678901234567890123456789012345678901234567890123456789012", atr(E_TEXTO));
    cadena(24, 72, "345678", atr(E_FUERA));
    cadena(26, 0, "Ejemplo de error de compilacion:", atr(E_TEXTO));
    rellenar(27, 0, sp.cols, atr(E_ERROR));
    cadena(27, 1, "prueba.f:19: error: parentesis sin cerrar", atr(E_ERROR));
    /* ayuda y mensaje */
    rellenar(30, 0, sp.cols, atr(E_AYUDA));
    cadena(30, 1, "F2 Save  F3 Load  F5 Run  F7 Compile  Esc Cmd", atr(E_AYUDA));
    cadena(31, 0, "Loaded prueba.f: 498 bytes", atr(E_TEXTO));
}

/* ---- arranque ---- */

static void iniciar(void)
{
    int modo;
    void *fuente;

    modo = 66;
    fuente = font_8x8_bbc_system;
    console_ioctl(IOCTL_GENCON_SET_MODE, &modo);
    console_ioctl(IOCTL_GENCON_SET_FONT32, &fuente);
    textbackground(1);
    textcolor(7);
    clrscr();
    gotoxy(0, 0);
    putchar('S');           /* para la autocomprobacion mapa[0] */

    sp.r6b = ZXN_READ_REG(R_TM_CTL);
    sp.r6c = ZXN_READ_REG(R_TM_ATTR);
    sp.r6e = ZXN_READ_REG(R_TM_MAP);
    sp.r6f = ZXN_READ_REG(R_TM_DEF);
    sp.r43 = ZXN_READ_REG(R_PAL_CTL);
    sp.cols = (sp.r6b & 0x40) ? 80 : 40;
    sp.celda = (sp.r6b & 0x20) ? 1 : 2;
    sp.ok = (sp.r6e & 0x40) ? 0 : 1;
    sp.mapa = (uint8_t *)(0x4000 + ((uint16_t)(sp.r6e & 0x3F) << 8));
    sp.at_base = (sp.ok && sp.celda == 2) ? sp.mapa[1] : sp.r6c;
    sp.hip = 0;
}

int main(void)
{
    int k, ultima;

    iniciar();
    pantalla_diag();
    ultima = 0;
    for (;;) {
        k = in_Inkey();
        if (k != ultima) {
            ultima = k;
            if (k == '1') { pantalla_diag(); }
            else if (k == '2') { pantalla_textcolor(); }
            else if (k == '3') { sp.hip = 0; pantalla_maqueta(); }
            else if (k == '4') { sp.hip = 1; pantalla_maqueta(); }
        }
    }
    return 0;
}
