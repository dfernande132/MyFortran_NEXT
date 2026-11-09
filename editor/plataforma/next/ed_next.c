/* ed_next.c - Pantalla y teclado del editor para el Next (z88dk, +zxn).
 *
 * Esta carpeta (editor/plataforma/next/) NO es platform/next/: build.ps1 compila
 * con z88dk TODO platform/next/*.c en cada spike, y este fichero necesita
 * ed_plat.h. Aqui no estorba.
 *
 * PANTALLA: tilemap 80x32 (modo 66 de la consola de z88dk + font_8x8_bbc_system,
 * docs/referencia seccion 8) y, despues, se escribe DIRECTAMENTE en la memoria del
 * mapa. MEDIDO en ZEsarUX con el spike (editor/plataforma/next/spike_tilemap.c,
 * ED-001 "Resultado del spike"):
 *   - $6B=C1: tilemap activo, 80 columnas, byte de atributo por celda, paleta 1
 *   - mapa en $6C00 (registro $6E=2C), 2 bytes por celda: caracter, atributo
 *   - atributo = estilo << 4 (nibble alto = desplazamiento de paleta; el bajo, a 0:
 *     son espejo/rotacion). Fondo = paleta[16*estilo], tinta = paleta[16*estilo+1]
 *   - la paleta del tilemap se escribe con NextREG $43 = $30 (+$40 si $6B bit 4),
 *     $40 = indice, $41 = valor RRRGGGBB (autoincremento)
 * SIN VERIFICAR: que el programa real (con esxdos, banco.h, etc.) se comporte igual
 * que el spike; que la velocidad de repintado sea comoda a 3,5 MHz (el repintado de
 * una fila escribe 160 bytes).
 *
 * TECLADO:
 *   H3  (CORREGIDA tras el primer test) getchar() NO sirve: segun el desensamblado
 *       de fgetc_cons_inkey, (a) espera CLIB_FGETC_CONS_DELAY ms al ENTRAR en cada
 *       llamada y (b) intercambia ENTER (13) y cursor-abajo (10). Se lee con
 *       in_Inkey() (codigos crudos, sin eco) y la repeticion se mide con FRAMES.
 *   H7  Esc (CAPS SHIFT + SPACE) llega como 0x20 igual que un espacio; se distingue
 *       leyendo CAPS SHIFT (puerto $FEFE, bit 0, activo a 0) y se traduce a 27.
 *       (Comprobado en ZEsarUX.)
 *   H6  El contador FRAMES ($5C78) avanza si hay interrupciones (EI): plat_iniciar()
 *       las activa; un .nex arranca con ellas apagadas.
 * FICHEROS:
 *   H8  esxdos_f_open/read/close como en MyLISP; nombre relativo al directorio actual
 *       de esxdos. En un emulador SIN esxdos puede colgar: -DED_SIN_ARRANQUE_FICHERO.
 *   H9  Al final del fichero esxdos_f_read devuelve <= 0; aqui cualquier valor <= 0
 *       se entrega como 0 = fin (un error de lectura se veria como fichero cortado).
 *   H10 (SIN VERIFICAR EN EL NEXT) Guardar: esxdos_f_open con
 *       ESXDOS_MODE_CREAT_TRUNC | ESXDOS_MODE_W y esxdos_f_write(handle, src, n), que
 *       devuelve los bytes escritos. Un fallo se detecta por errno != 0 o por una
 *       escritura corta; en ambos casos el editor dice "Write error".
 * La ventana MMU2 ($4000) debe estar en reposo (pagina 10): banco_next.c la
 * restaura siempre. El mapa esta en $6C00-$7FFF, fuera de esa ventana.
 */
#include <stdint.h>
#include <errno.h>
#include <conio.h>
#include <sys/ioctl.h>
#include <input.h>
#include <arch/z80.h>
#include <arch/zxn.h>
#include <arch/zxn/esxdos.h>
#include <intrinsic.h>
#include "ed_plat.h"

extern unsigned char font_8x8_bbc_system[];

/* Registros del Next */
#define R_PAL_IDX   0x40
#define R_PAL_VAL   0x41
#define R_PAL_CTL   0x43
#define R_TM_CTL    0x6B
#define R_TM_MAP    0x6E

/* {fondo, tinta} en RRRGGGBB, en el orden de los estilos ED_E_* (ed_plat.h) */
static const uint8_t paleta[ED_N_ESTILOS][2] = {
    { 0x25, 0xDA },  /* texto: azul marino / blanco roto */
    { 0x25, 0x6E },  /* comentario: gris azulado */
    { 0x25, 0xFC },  /* etiqueta: amarillo */
    { 0x25, 0xE8 },  /* continuacion: rojo anaranjado */
    { 0x25, 0xF2 },  /* palabra clave: rosa */
    { 0x25, 0x7D },  /* cadena: verde */
    { 0x25, 0xF5 },  /* numero: naranja */
    { 0x25, 0x4A },  /* fuera de la columna 72: gris tenue */
    { 0xDB, 0x25 },  /* barra de estado: gris claro / marino */
    { 0x4F, 0xFF },  /* ayuda: azul medio / blanco */
    { 0xFC, 0x25 },  /* cursor: amarillo / marino */
    { 0xA0, 0xFF }   /* error: rojo oscuro / blanco */
};

static uint8_t *mapa;       /* direccion CPU del mapa del tilemap */
static uint8_t celda;       /* bytes por celda (2 = caracter + atributo) */

/* Cursor: byte de atributo modificado y su valor antes/despues, para
 * deshacerlo solo si nadie ha repintado esa celda mientras tanto. */
static uint8_t *cur_dir;
static uint8_t cur_orig;
static uint8_t cur_nuevo;

static void cursor_quitar(void)
{
    if (cur_dir != 0 && *cur_dir == cur_nuevo) *cur_dir = cur_orig;
    cur_dir = 0;
}

static void paleta_cargar(uint8_t r6b)
{
    uint8_t s, ant;

    ant = ZXN_READ_REG(R_PAL_CTL);
    ZXN_WRITE_REG(R_PAL_CTL, (uint8_t)(0x30 | ((r6b & 0x10) ? 0x40 : 0)));
    for (s = 0; s < ED_N_ESTILOS; ++s) {
        ZXN_WRITE_REG(R_PAL_IDX, (uint8_t)(s << 4));
        ZXN_WRITE_REG(R_PAL_VAL, paleta[s][0]);
        ZXN_WRITE_REG(R_PAL_VAL, paleta[s][1]);
    }
    ZXN_WRITE_REG(R_PAL_CTL, ant);
}

void plat_iniciar(void)
{
    int modo;
    void *fuente;
    uint8_t r6b, f;
    uint8_t blancos[ED_ANCHO];
    uint8_t est[ED_ANCHO];

    modo = 66;
    fuente = font_8x8_bbc_system;
    console_ioctl(IOCTL_GENCON_SET_MODE, &modo);
    console_ioctl(IOCTL_GENCON_SET_FONT32, &fuente);
    clrscr();

    r6b = ZXN_READ_REG(R_TM_CTL);
    celda = (r6b & 0x20) ? 1 : 2;
    mapa = (uint8_t *)(0x4000 + ((uint16_t)(ZXN_READ_REG(R_TM_MAP) & 0x3F) << 8));
    paleta_cargar(r6b);

    for (f = 0; f < ED_ANCHO; ++f) { blancos[f] = ' '; est[f] = ED_E_TEXTO; }
    for (f = 0; f < ED_FILAS; ++f) plat_escribir(f, 0, blancos, est, ED_ANCHO);

    cur_dir = 0;
    intrinsic_ei();       /* un .nex arranca con las interrupciones apagadas; FRAMES las necesita */
}

void plat_escribir(uint8_t fila, uint8_t col, const uint8_t *txt, const uint8_t *e, uint8_t n)
{
    uint8_t *p;

    p = mapa + ((uint16_t)fila * ED_ANCHO + col) * celda;
    if (celda == 2) {
        while (n != 0) {
            *p++ = *txt++;
            *p++ = (uint8_t)(*e++ << 4);
            n--;
        }
    } else {
        while (n != 0) {
            *p++ = *txt++;
            n--;
        }
    }
}

void plat_cursor(uint8_t fila, uint8_t col)
{
    uint8_t *p;

    cursor_quitar();
    if (celda != 2) return;
    p = mapa + ((uint16_t)fila * ED_ANCHO + col) * 2 + 1;       /* byte de atributo */
    cur_orig = *p;
    cur_nuevo = (uint8_t)(ED_E_CURSOR << 4);
    *p = cur_nuevo;
    cur_dir = p;
}

/* Repeticion de teclas, en fotogramas de 50 Hz. Ajustables con -D. */
#ifndef ED_REP_INICIO
#define ED_REP_INICIO   15      /* hasta la primera repeticion (0,3 s) */
#endif
#ifndef ED_REP_PERIODO
#define ED_REP_PERIODO  2       /* entre repeticiones (25 por segundo) */
#endif

#define FRAMES_BAJO  (*(uint8_t *)0x5C78)    /* byte bajo del contador de fotogramas */

/* Bloquea hasta que hay una tecla NUEVA o toca repetir la que sigue pulsada.
 * Devuelve el codigo crudo de in_Inkey (ENTER 13, abajo 10, ...). Las
 * diferencias de FRAMES se hacen en 8 bits: valen mientras no pasen 256
 * fotogramas (5 s) entre dos lecturas; por eso, una vez repitiendo, solo
 * cuenta el tiempo desde la ultima repeticion. */
int plat_tecla(void)
{
    static uint16_t ultima;
    static uint8_t t0, t_rep, repitiendo;
    uint16_t k;
    uint8_t ahora, t;
    uint16_t espera;

    for (;;) {
        k = in_Inkey();
        if (k == 0) {
            ultima = 0;
            continue;
        }
        if (k == 0x20) {
            /* H7b (medido por Dani en ZEsarUX: un toque de Esc daba "Unknown command"):
             * CAPS y SPACE no cambian a la vez. Al PULSAR puede verse SPACE solo un
             * instante (se escribiria un blanco), y al SOLTAR sube CAPS antes que SPACE
             * (llegaria un blanco justo despues del Esc). Se arregla asi:
             *  - tecla nueva 0x20: esperar 2 fotogramas a que llegue CAPS y mirar entonces;
             *  - 0x20 mientras Esc sigue pulsado (ultima == 27): ignorarlo. */
            if (ultima == 27) continue;
            if (ultima != 0x20) {
                t = FRAMES_BAJO;
                for (espera = 0; espera < 30000u && (uint8_t)(FRAMES_BAJO - t) < 2; espera++) { }
            }
            if ((z80_inp(0xFEFE) & 1) == 0) k = 27;            /* H7: Esc = CAPS + SPACE */
        }
        ahora = FRAMES_BAJO;
        if (k != ultima) {
            ultima = k;
            t0 = ahora;
            repitiendo = 0;
            return (int)k;
        }
        if (k == 27) continue;                  /* Esc no se repite (alternaria CMD y "Unknown command") */
        if (repitiendo == 0) {
            if ((uint8_t)(ahora - t0) >= ED_REP_INICIO) {
                repitiendo = 1;
                t_rep = ahora;
                return (int)k;
            }
        } else if ((uint8_t)(ahora - t_rep) >= ED_REP_PERIODO) {
            t_rep = ahora;
            return (int)k;
        }
    }
}
