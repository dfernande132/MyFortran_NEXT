/* g1_trampolines.c - Generador+backend: los trampolines de i1 mas los de los bancos 44 (generador de TAC) y
 * 43 (backend Z80), y los que ellos necesitan hacia el 45 (simbolos y etiquetas). Mismo mapear()/banco_actual. */
#include <string.h>
#include "i1_trampolines.c"
#include "gen.h"
#include "tac.h"
#include "back.h"
#include "z80.h"
#include "nex.h"
#include "mf_msg.h"
#include "unid.h"
#include "sem.h"
#include "intr.h"
#include "sem_sf.h"

#define BANCO_GEN   44
#define BANCO_BACK  43
#define BANCO_BACK_C 48
#define BANCO_NEX   42

/* ---- banco 44 (generador): ganchos que llama el semantico (banco 45) y mf.c ---- */
uint8_t stmt_dato(void);

/* ---- banco 45 (simbolos y etiquetas), desde los bancos 44 y 43 ---- */
/* ---- banco 48 (backend, parte C: plantillas CHARACTER), desde el banco 43 ---- */
void back_c_entrada(uint8_t cual, const TacInst *t, uint16_t x, uint16_t y);

/* ---- generador partido: gen.c (44) <-> gen_ll.c (42) ---- */
uint16_t gen_expr(uint16_t n, uint16_t dst);
uint16_t gen_desplazamiento(uint16_t n, const Simbolo *s, const Dim *dm);
uint16_t gen_subcadena(uint16_t base, uint16_t cap_base, uint16_t rango, uint8_t asum);
uint16_t gen_funcion(uint16_t n, uint8_t u);
void     gen_llamada_emitir(void);

/* ---- almacen TAC (tac.c, banco 42) llamado desde el generador (44) ---- */
/* ---- banco 43 (backend): punto de entrada ---- */
void t_back_generar(uint8_t *img, uint16_t cap, uint16_t org,
                    void (*sal)(const char *linea), BackRes *r)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_BACK); back_generar(img, cap, org, sal, r); mapear(ant);
}

/* ---- programas de prueba del driver (banco 47) ---- */
void g1_cargar(uint8_t n);

/* ---- banco 42 (nex.c): escribe el .nex con la salida del editor; llama al backend (43) por trampolin ---- */
/* funciones de sentencia: sem.c (45) -> sem_sf.c (42) y de vuelta */
uint8_t decl_entrada(uint8_t op, uint8_t tipo, uint16_t e);

/* ---- tablas de las rutinas REAL del runtime (banco 46): lectura por trozos desde el backend (banco 43) ---- */
uint8_t rt_real_leer(uint8_t r, uint16_t off, uint8_t *dst, uint8_t n);

/* ---- tablas del runtime de entrada (READ, banco 48) ---- */
uint8_t rt_ent_leer(uint8_t r, uint16_t off, uint8_t *dst, uint8_t n);

/* ---- real.c vive en el banco 46: lo llama el generador (banco 44) ---- */
uint8_t real_de_texto(const char *texto, uint32_t *bits);
uint32_t real_de_entero(int16_t v);

/* ---- tabla de intrinsecas (banco 47): se devuelve una copia en el residente (los punteros a RODATA del 47 no valen en el 45) ---- */
static Intrinseca intr_copia;

const Intrinseca *t_intr_buscar(const char *nombre)
{
    uint8_t ant = banco_actual;
    const Intrinseca *p;

    mapear(47);
    p = intr_buscar(nombre);
    if (p != 0) {
        memcpy(&intr_copia, p, sizeof(Intrinseca));
    }
    mapear(ant);
    return p != 0 ? &intr_copia : 0;
}

/* ---- decision 0014 (subprogramas) ---- */
/* unid.c vive en el banco 47 (BANCO_EXP): lo llaman sem.c (45), back.c (43) y mf.c (residente) */
