/* back.h - Generador de codigo Z80 a partir del TAC (decision 0012).
 *
 * Entrada: el TAC que dejo gen.c en las paginas de datos (tac.h) y las tablas de
 * simbolos y etiquetas de sem.c. Salida: una imagen binaria lista para cargar en
 * `org` (codigo + rutinas de apoyo + constantes) y, si se pide, el listado .asm.
 *
 * El programa generado es una SUBRUTINA: se entra por `entry` con CALL y acaba con RET
 * (HALT y STOP del TAC). Imprime con RST 16 (la ROM del Spectrum / NextZXOS); en las
 * pruebas de PC el emulador intercepta esa llamada.
 *
 * Memoria del programa, desde org: [codigo][rutinas usadas][constantes][variables y
 * temporales]. Solo hasta el final de las constantes hay bytes en la imagen; lo
 * demas (fin_mem) es memoria sin inicializar que el cargador debe reservar.
 *
 * Alcance actual (etapa A): INTEGER y LOGICAL (escalares y arrays), aritmetica,
 * logica, relacionales, control de flujo, PRINT de enteros, logicos y constantes de
 * texto. CHARACTER (TMP, SCAT, SSUB, MOV.C...) devuelve BACK_NO_SOPORTADO.
 */
#ifndef MF_BACK_H
#define MF_BACK_H

#include <stdint.h>

#define BACK_PAG_TABLAS    10     /* pagina de datos con las tablas del backend (banco.h)            */
#define BACK_PAG_IMG       11     /* imagen en paginas 11..12 si img == NULL (16 KB como maximo)     */

#define BACK_OK            0
#define BACK_NO_SOPORTADO  (-1)   /* err_inst = instruccion TAC que no sabe traducir */
#define BACK_NO_CABE       (-2)   /* la imagen no cabe en el buffer                  */
#define BACK_INTERNO       (-3)   /* salto relativo fuera de rango, rutina ausente...*/
#define BACK_ETIQUETAS     (-4)   /* tabla de direcciones de etiquetas insuficiente  */
#define BACK_TEMPORALES    (-5)   /* mas de NTMP (128) temporales en todo el programa (se numeran seguidos entre unidades) */

typedef struct {
    int16_t  err;
    uint16_t err_inst;
    uint16_t org;
    uint16_t entry;
    uint16_t fin_img;     /* primera direccion tras los bytes de la imagen          */
    uint16_t fin_mem;     /* primera direccion tras variables y temporales          */
} BackRes;

/* Salida de texto: 0 = RST 16 (ROM; por defecto), 1 = rutina propia que dibuja en la pantalla
 * ($4000) con la fuente de la ROM ($3D00) sin depender de la ROM ni de NextZXOS. Con scroll:
 * al pasar de la fila 23 la pantalla sube una fila y se borra la ultima. Los atributos ($5800) los debe poner el cargador. */
void back_salida_pantalla(uint8_t si);

/* Solo en las pruebas de PC: 1 = READ toma las teclas de un texto en memoria ($F002, con el puntero en $F000) en vez del teclado. */
#ifndef MF_TARGET_NEXT
void back_entrada_prueba(uint8_t si);
#endif

/* img/cap: buffer donde se escribe la imagen (cap bytes). Con img == NULL la imagen se guarda en las
 * paginas de datos BACK_PAG_IMG.. (cap se limita a 16384): es lo que se usa en el Next, donde no hay
 * memoria libre para un buffer; z_imagen_leer() (z80.h) la devuelve. sal: listado .asm o NULL. */
void back_generar(uint8_t *img, uint16_t cap, uint16_t org,
                  void (*sal)(const char *linea), BackRes *r);

#endif
