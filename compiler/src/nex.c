/* nex.c - Ver nex.h. Los 16896 bytes se componen por trozos: cada trozo se rellena a ceros y se le
 * superponen las tres piezas (cabecera, imagen del programa y arranque) que caen en el. */
#include <string.h>
#include "nex.h"
#include "z80.h"
#include "mf_api.h"
#include "mf_msg.h"

#define CAB_TAM   512u
#define ARR_TAM   32u
#define CAB_UTIL  24u                 /* los 24 primeros bytes de la cabecera no son todos cero    */

/* Copia en el trozo [base, base+NEX_TROZO) la parte de la pieza (posicion pos, n bytes, en src) que cae en el. */
static void poner(uint8_t *ch, uint16_t base, uint16_t pos, const uint8_t *src, uint16_t n)
{
    uint16_t i, p;

    for (i = 0; i < n; i++) {
        p = (uint16_t)(pos + i);
        if (p >= base && (uint16_t)(p - base) < NEX_TROZO) {
            ch[p - base] = src[i];
        }
    }
}

int16_t nex_comprobar(const BackRes *r)
{
    if (r->err != BACK_OK || r->fin_img < r->org) {
        return NEX_RESULTADO;
    }
    if (r->org < 0x8000u || r->fin_mem > NEX_ARRANQUE || r->fin_mem < r->fin_img) {
        return NEX_NO_CABE;
    }
    return NEX_OK;
}

int16_t nex_escribir(NexSalida sal, const BackRes *r)
{
    uint8_t ch[NEX_TROZO];
    uint8_t cab[CAB_UTIL];
    uint8_t arr[ARR_TAM];
    uint16_t base, k, lo, hi, n_img;
    int16_t e = nex_comprobar(r);

    if (e != NEX_OK) {
        return e;
    }
    memset(cab, 0, sizeof(cab));
    memcpy(cab, "NextV1.2", 8);
    cab[9] = 1;                           /* un banco de 16 KB                                   */
    cab[12] = 0xFE; cab[13] = 0xBF;       /* SP = $BFFE                                          */
    cab[14] = (uint8_t)(NEX_ARRANQUE & 0xFF);
    cab[15] = (uint8_t)(NEX_ARRANQUE >> 8);
    cab[18 + 2] = 1;                      /* banco 2 presente                                    */

    arr[0] = 0xF3;                                        /* DI                                  */
    arr[1] = 0x21; arr[2] = 0x00; arr[3] = 0x40;          /* LD HL,$4000                         */
    arr[4] = 0x11; arr[5] = 0x01; arr[6] = 0x40;          /* LD DE,$4001                         */
    arr[7] = 0x01; arr[8] = 0xFF; arr[9] = 0x17;          /* LD BC,$17FF                         */
    arr[10] = 0x36; arr[11] = 0x00;                       /* LD (HL),0                           */
    arr[12] = 0xED; arr[13] = 0xB0;                       /* LDIR  (pixeles a 0)                 */
    arr[14] = 0x21; arr[15] = 0x00; arr[16] = 0x58;       /* LD HL,$5800                         */
    arr[17] = 0x11; arr[18] = 0x01; arr[19] = 0x58;       /* LD DE,$5801                         */
    arr[20] = 0x01; arr[21] = 0xFF; arr[22] = 0x02;       /* LD BC,$02FF                         */
    arr[23] = 0x36; arr[24] = 0x07;                       /* LD (HL),7                           */
    arr[25] = 0xED; arr[26] = 0xB0;                       /* LDIR  (atributos)                   */
    arr[27] = 0xCD;                                       /* CALL entry                          */
    arr[28] = (uint8_t)(r->entry & 0xFF);
    arr[29] = (uint8_t)(r->entry >> 8);
    arr[30] = 0x18; arr[31] = 0xFE;                       /* JR $                                */

    n_img = (uint16_t)(r->fin_img - r->org);
    for (base = 0; base < NEX_TAM; base = (uint16_t)(base + NEX_TROZO)) {
        memset(ch, 0, sizeof(ch));
        poner(ch, base, 0, cab, CAB_UTIL);
        /* programa: posicion en el fichero = 512 + (direccion - $8000) */
        lo = (uint16_t)(CAB_TAM + (r->org - 0x8000u));
        if (lo < (uint16_t)(base + NEX_TROZO) && (uint16_t)(lo + n_img) > base) {
            hi = lo > base ? lo : base;               /* primera posicion del fichero dentro del trozo */
            k = (uint16_t)(lo + n_img);
            if (k > (uint16_t)(base + NEX_TROZO)) {
                k = (uint16_t)(base + NEX_TROZO);
            }
            z_imagen_leer((uint16_t)(hi - lo), ch + (hi - base), (uint16_t)(k - hi));
        }
        /* arranque en $BE00 */
        lo = (uint16_t)(CAB_TAM + (NEX_ARRANQUE - 0x8000u));
        poner(ch, base, lo, arr, ARR_TAM);
        if (!sal(ch, NEX_TROZO)) {
            return NEX_ESCRITURA;
        }
    }
    return NEX_OK;
}

/* Genera el Z80 del TAC actual (salida propia a pantalla, sin ROM) y lo escribe como .nex por la salida del editor.
 * Devuelve 0 si todo fue bien o un codigo MF_COD_GEN + k (mf_mensaje): 1 construccion no soportada,
 * 2 no cabe la imagen, 3 error interno, 4 demasiadas etiquetas, 5 no cabe en el .nex, 6 no se pudo abrir
 * el fichero, 7 fallo al escribir o cerrar. */
uint8_t nex_generar(const MfSalida *s)
{
    BackRes r;
    int16_t e;
    uint8_t ok;

    back_salida_pantalla(1);
    back_generar(NULL, 16384, 0x8000, NULL, &r);
    back_salida_pantalla(0);                         /* el defecto: no deja la salida a pantalla puesta */
    if (r.err != BACK_OK) {
        return (uint8_t)(MF_COD_GEN - r.err);        /* err -1..-4 -> 1..4 */
    }
    if (nex_comprobar(&r) != NEX_OK) {
        return MF_COD_GEN + 5;
    }
    if (!s->abrir()) {
        return MF_COD_GEN + 6;
    }
    e = nex_escribir(s->escribir, &r);
    ok = s->cerrar((uint8_t)(e == NEX_OK));
    return (uint8_t)(e == NEX_OK && ok ? 0 : MF_COD_GEN + 7);
}
