/* ed_linea.c - ed_linea() de compiler/include/mf_api.h sobre el buffer del editor.
 *
 * Es RESIDENTE (COORDINACION 2.2/3): el compilador la llama mientras su banco esta
 * mapeado en MMU6/7, asi que no puede vivir en el banco del editor. Solo usa eb_*
 * (edbuf.c, tambien residente) y no llama a ningun banco.
 *
 * Contrato: linea fisica n (la 1.a es la 1), sin '\n', como mucho 72 caracteres +
 * '\0' (dst de 73 B, residente). Devuelve 1 si existe y 0 si n pasa del final.
 * Una linea vacia EXISTE; detras del ultimo '\n' no hay otra. El compilador pide
 * n, n+1, ... y solo vuelve atras al empezar otra compilacion.
 *
 * Cache: numero y posicion de la ultima linea pedida; si la siguiente es n o
 * posterior solo se escanea desde ahi (O(1) amortizado). Se descarta si pide una
 * anterior o si el texto cambio (eb.ver).
 */
#include <stdint.h>
#include "edbuf.h"

#define ED_LINEA_MAX  72

static uint16_t c_n;        /* numero de la linea de la cache (0 = vacia) */
static uint16_t c_pos;      /* posicion donde empieza esa linea */
static uint16_t c_ver;      /* eb.ver cuando se lleno la cache */

int ed_linea(uint16_t n, char *dst)
{
    uint16_t len, f, k;

    if (n == 0) return 0;
    if (c_n == 0 || n < c_n || c_ver != eb.ver) {
        c_n = 1;
        c_pos = 0;
        c_ver = eb.ver;
    }
    len = eb_longitud();
    while (c_n < n) {
        f = eb_fin_linea(c_pos);
        if (f >= len) return 0;             /* era la ultima linea: n pasa del final */
        c_pos = f + 1;
        c_n++;
    }
    if (c_pos >= len) return 0;             /* detras del ultimo '\n' no hay linea */
    k = eb_fin_linea(c_pos) - c_pos;
    if (k > ED_LINEA_MAX) k = ED_LINEA_MAX;
    if (k != 0) eb_leer(c_pos, (uint8_t *)dst, k);
    dst[k] = '\0';
    return 1;
}
