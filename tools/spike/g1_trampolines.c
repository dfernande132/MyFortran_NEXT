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
void t_gen_activar(uint8_t si)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_GEN); gen_activar(si); mapear(ant);
}

void t_gen_iniciar(void)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_GEN); gen_iniciar(); mapear(ant);
}

void t_gen_etiqueta(uint32_t etiq)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_GEN); gen_etiqueta(etiq); mapear(ant);
}

void t_gen_sentencia(void)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_GEN); gen_sentencia(); mapear(ant);
}

void t_gen_print_item(void)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_GEN); gen_print_item(); mapear(ant);
}

uint8_t stmt_dato(void);

uint8_t t_stmt_dato(void)
{
    uint8_t ant = banco_actual;
    uint8_t r;
    mapear(BANCO_EXP); r = stmt_dato(); mapear(ant);
    return r;
}

void t_gen_param_item(void)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_GEN); gen_param_item(); mapear(ant);
}

void t_gen_read(uint8_t item)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_GEN); gen_read(item); mapear(ant);
}

void t_gen_goto_etq(uint32_t etiq, uint16_t pos)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_GEN); gen_goto_etq(etiq, pos); mapear(ant);
}

void t_gen_fin_sentencia(void)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_GEN); gen_fin_sentencia(); mapear(ant);
}

/* ---- banco 45 (simbolos y etiquetas), desde los bancos 44 y 43 ---- */
uint16_t t_sim_buscar(const char *nombre)
{
    uint8_t ant = banco_actual;
    uint16_t r;
    mapear(BANCO_SEM); r = sim_buscar(nombre); mapear(ant);
    return r;
}

void t_sim_leer(uint16_t id, Simbolo *s)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_SEM); sim_leer(id, s); mapear(ant);
}

void t_sim_escribir(uint16_t id, const Simbolo *s)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_SEM); sim_escribir(id, s); mapear(ant);
}

void t_sim_dim_leer(uint16_t indice, Dim *d)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_SEM); sim_dim_leer(indice, d); mapear(ant);
}

uint16_t t_sem_etq_indice(uint32_t etiq)
{
    uint8_t ant = banco_actual;
    uint16_t r;
    mapear(BANCO_SEM); r = sem_etq_indice(etiq); mapear(ant);
    return r;
}

uint32_t t_sem_etq_valor(uint16_t indice)
{
    uint8_t ant = banco_actual;
    uint32_t r;
    mapear(BANCO_SEM); r = sem_etq_valor(indice); mapear(ant);
    return r;
}

/* ---- banco 48 (backend, parte C: plantillas CHARACTER), desde el banco 43 ---- */
void back_c_entrada(uint8_t cual, const TacInst *t, uint16_t x, uint16_t y);

void t_back_c(uint8_t cual, const TacInst *t, uint16_t x, uint16_t y)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_BACK_C); back_c_entrada(cual, t, x, y); mapear(ant);
}

/* ---- generador partido: gen.c (44) <-> gen_ll.c (42) ---- */
uint16_t gen_expr(uint16_t n, uint16_t dst);
uint16_t gen_desplazamiento(uint16_t n, const Simbolo *s, const Dim *dm);
uint16_t gen_subcadena(uint16_t base, uint16_t cap_base, uint16_t rango, uint8_t asum);
uint16_t gen_funcion(uint16_t n, uint8_t u);
void     gen_llamada_emitir(void);

uint16_t t_gen_expr(uint16_t n, uint16_t dst)
{
    uint8_t ant = banco_actual;
    uint16_t r;
    mapear(BANCO_GEN); r = gen_expr(n, dst); mapear(ant);
    return r;
}

uint16_t t_gen_desplazamiento(uint16_t n, const Simbolo *s, const Dim *dm)
{
    uint8_t ant = banco_actual;
    uint16_t r;
    mapear(BANCO_GEN); r = gen_desplazamiento(n, s, dm); mapear(ant);
    return r;
}

uint16_t t_gen_subcadena(uint16_t base, uint16_t cap_base, uint16_t rango, uint8_t asum)
{
    uint8_t ant = banco_actual;
    uint16_t r;
    mapear(BANCO_GEN); r = gen_subcadena(base, cap_base, rango, asum); mapear(ant);
    return r;
}

uint16_t t_gen_funcion(uint16_t n, uint8_t u)
{
    uint8_t ant = banco_actual;
    uint16_t r;
    mapear(BANCO_NEX); r = gen_funcion(n, u); mapear(ant);
    return r;
}

void t_gen_llamada_emitir(void)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_NEX); gen_llamada_emitir(); mapear(ant);
}

/* ---- almacen TAC (tac.c, banco 42) llamado desde el generador (44) ---- */
void t_tac_iniciar(void)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_NEX); tac_iniciar(); mapear(ant);
}

void t_tac_emitir(uint8_t op, uint8_t tipo, uint16_t a, uint16_t b, uint16_t c)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_NEX); tac_emitir(op, tipo, a, b, c); mapear(ant);
}

uint16_t t_tac_entero(int16_t v)
{
    uint8_t ant = banco_actual;
    uint16_t r;
    mapear(BANCO_NEX); r = tac_entero(v); mapear(ant);
    return r;
}

uint16_t t_tac_logico(uint8_t v)
{
    uint8_t ant = banco_actual;
    uint16_t r;
    mapear(BANCO_NEX); r = tac_logico(v); mapear(ant);
    return r;
}

uint16_t t_tac_cadena(const char *s, uint8_t len)
{
    uint8_t ant = banco_actual;
    uint16_t r;
    mapear(BANCO_NEX); r = tac_cadena(s, len); mapear(ant);
    return r;
}

uint16_t t_tac_real(uint32_t bits)
{
    uint8_t ant = banco_actual;
    uint16_t r;
    mapear(BANCO_NEX); r = tac_real(bits); mapear(ant);
    return r;
}

/* ---- banco 43 (backend): punto de entrada ---- */
void t_back_generar(uint8_t *img, uint16_t cap, uint16_t org,
                    void (*sal)(const char *linea), BackRes *r)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_BACK); back_generar(img, cap, org, sal, r); mapear(ant);
}

/* ---- programas de prueba del driver (banco 47) ---- */
void g1_cargar(uint8_t n);

void t_g1_cargar(uint8_t n)
{
    uint8_t ant = banco_actual;
    mapear(47); g1_cargar(n); mapear(ant);
}

/* ---- banco 42 (nex.c): escribe el .nex con la salida del editor; llama al backend (43) por trampolin ---- */
void t_back_salida_pantalla(uint8_t si)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_BACK); back_salida_pantalla(si); mapear(ant);
}

void t_z_imagen_leer(uint16_t desp, uint8_t *dst, uint16_t n)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_BACK); z_imagen_leer(desp, dst, n); mapear(ant);
}

/* funciones de sentencia: sem.c (45) -> sem_sf.c (42) y de vuelta */
uint8_t decl_entrada(uint8_t op, uint8_t tipo, uint16_t e);

uint8_t t_decl_entrada(uint8_t op, uint8_t tipo, uint16_t e)
{
    uint8_t ant = banco_actual, r;
    mapear(BANCO_NEX); r = decl_entrada(op, tipo, e); mapear(ant);
    return r;
}

uint16_t t_sim_alta(const char *nombre, uint8_t tipo, uint8_t clase)
{
    uint8_t ant = banco_actual;
    uint16_t r;
    mapear(BANCO_SEM); r = sim_alta(nombre, tipo, clase); mapear(ant);
    return r;
}

uint16_t t_sim_dim_nueva(const Dim *d)
{
    uint8_t ant = banco_actual;
    uint16_t r;
    mapear(BANCO_SEM); r = sim_dim_nueva(d); mapear(ant);
    return r;
}

uint8_t t_sf_entrada(uint8_t op, uint16_t n, uint8_t destino)
{
    uint8_t ant = banco_actual, r;
    mapear(48); r = sf_entrada(op, n, destino); mapear(ant);
    return r;
}

uint16_t t_sem_cb(uint8_t op, uint16_t a, uint16_t b)
{
    uint8_t ant = banco_actual;
    uint16_t r;
    mapear(BANCO_SEM); r = sem_cb(op, a, b); mapear(ant);
    return r;
}

uint8_t t_nex_generar(const MfSalida *s)
{
    uint8_t ant = banco_actual, r;
    mapear(BANCO_NEX); r = nex_generar(s); mapear(ant);
    return r;
}

void t_msg_gen(uint8_t e, char *dst)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_EXP); msg_gen(e, dst); mapear(ant);
}

/* ---- tablas de las rutinas REAL del runtime (banco 46): lectura por trozos desde el backend (banco 43) ---- */
uint8_t rt_real_leer(uint8_t r, uint16_t off, uint8_t *dst, uint8_t n);

uint8_t t_rt_real_leer(uint8_t r, uint16_t off, uint8_t *dst, uint8_t n)
{
    uint8_t ant = banco_actual, k;
    mapear(46); k = rt_real_leer(r, off, dst, n); mapear(ant);
    return k;
}

/* ---- tablas del runtime de entrada (READ, banco 48) ---- */
uint8_t rt_ent_leer(uint8_t r, uint16_t off, uint8_t *dst, uint8_t n);

uint8_t t_rt_ent_leer(uint8_t r, uint16_t off, uint8_t *dst, uint8_t n)
{
    uint8_t ant = banco_actual, k;
    mapear(48); k = rt_ent_leer(r, off, dst, n); mapear(ant);
    return k;
}

/* ---- real.c vive en el banco 46: lo llama el generador (banco 44) ---- */
uint8_t real_de_texto(const char *texto, uint32_t *bits);
uint32_t real_de_entero(int16_t v);

uint8_t t_real_de_texto(const char *texto, uint32_t *bits)
{
    uint8_t ant = banco_actual, r;
    mapear(46); r = real_de_texto(texto, bits); mapear(ant);
    return r;
}

uint32_t t_real_de_entero(int16_t v)
{
    uint8_t ant = banco_actual;
    uint32_t r;
    mapear(46); r = real_de_entero(v); mapear(ant);
    return r;
}

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
void t_gen_unidad(uint8_t clase, uint8_t uid)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_GEN); gen_unidad(clase, uid); mapear(ant);
}

void t_gen_llamada_inicio(uint8_t uid)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_NEX); gen_llamada_inicio(uid); mapear(ant);
}

void t_gen_llamada_arg(uint8_t k)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_NEX); gen_llamada_arg(k); mapear(ant);
}

void t_sem_sentencia_erronea(void)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_SEM); sem_sentencia_erronea(); mapear(ant);
}

/* unid.c vive en el banco 47 (BANCO_EXP): lo llaman sem.c (45), back.c (43) y mf.c (residente) */
void t_msg_uni(uint8_t e, char *dst)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_EXP); msg_uni(e, dst); mapear(ant);
}

void t_uni_iniciar(void)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_EXP); uni_iniciar(); mapear(ant);
}

uint8_t t_uni_cuenta(void)
{
    uint8_t ant = banco_actual, r;
    mapear(BANCO_EXP); r = uni_cuenta(); mapear(ant);
    return r;
}

uint8_t t_uni_buscar(const char *nombre)
{
    uint8_t ant = banco_actual, r;
    mapear(BANCO_EXP); r = uni_buscar(nombre); mapear(ant);
    return r;
}

uint8_t t_uni_alta(const char *nombre, uint16_t linea, uint16_t col)
{
    uint8_t ant = banco_actual, r;
    mapear(BANCO_EXP); r = uni_alta(nombre, linea, col); mapear(ant);
    return r;
}

void t_uni_leer(uint8_t u, Unidad *x)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_EXP); uni_leer(u, x); mapear(ant);
}

void t_uni_escribir(uint8_t u, const Unidad *x)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_EXP); uni_escribir(u, x); mapear(ant);
}

uint16_t t_uni_dir_get(uint8_t u)
{
    uint8_t ant = banco_actual;
    uint16_t r;
    mapear(BANCO_EXP); r = uni_dir_get(u); mapear(ant);
    return r;
}

void t_uni_dir_set(uint8_t u, uint16_t d)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_EXP); uni_dir_set(u, d); mapear(ant);
}

uint8_t t_uni_llamada(uint8_t llamador, uint8_t llamado, uint16_t linea, uint8_t col, uint8_t n, const uint8_t *kind, const uint8_t *lens, uint8_t ftipo, uint8_t flen)
{
    uint8_t ant = banco_actual, r;
    mapear(BANCO_EXP); r = uni_llamada(llamador, llamado, linea, col, n, kind, lens, ftipo, flen); mapear(ant);
    return r;
}

void t_uni_comprobar(void)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_EXP); uni_comprobar(); mapear(ant);
}
