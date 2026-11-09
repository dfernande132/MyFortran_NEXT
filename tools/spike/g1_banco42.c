/* g1_banco42.c - Escritura del .nex (nex.c) en el banco 42: sale del 43 (backend), que se quedo corto para S3.
 * Llama al backend por trampolin (back_generar, back_salida_pantalla) y lee la imagen con z_imagen_leer, tambien por trampolin. */
#pragma bank 42
/* puertas por trampolin que usa sem_decl.c (antes las traia el bloque de sem_sf.c); antes de cualquier #include para renombrar tambien los prototipos */
#define tabla_texto           t_tabla_texto
#define sim_buscar            t_sim_buscar
#define sim_leer              t_sim_leer
#define intr_buscar           t_intr_buscar
#define sem_cb                t_sem_cb
#include "../../compiler/src/lexer.h"
#include "../../compiler/src/sim.h"
#include "../../compiler/src/ast.h"
#include "../../compiler/src/unid.h"

#define back_generar          t_back_generar
#define back_salida_pantalla  t_back_salida_pantalla
#define z_imagen_leer         t_z_imagen_leer

#include "../../compiler/src/nex.c"

/* almacen del TAC (antes en el banco 44); sus variables (tac_n, tac_pool_n, tac_lle) viven en el residente */
#include "../../compiler/src/tac.c"

/* Declaraciones de tipo (sem_decl.c): sale del banco 45 como sem_sf.c; mismas puertas (sem_cb) mas la tabla de simbolos y el directorio por trampolin */
#define sim_escribir          t_sim_escribir
#define sim_alta              t_sim_alta
#define sim_dim_nueva         t_sim_dim_nueva
#define uni_leer              t_uni_leer
#define sim_dim_leer          t_sim_dim_leer       /* DIMENSION / DATA / PARAMETER leen las dimensiones del banco 45 */
#define real_de_texto         t_real_de_texto      /* PARAMETER / DATA con valores REAL: real.c vive en el 46 */
#define real_de_entero        t_real_de_entero
void     t_sim_dim_leer(uint16_t indice, Dim *d);
uint8_t  t_real_de_texto(const char *texto, uint32_t *bits);
uint32_t t_real_de_entero(int16_t v);
void     t_uni_leer(uint8_t u, Unidad *x);
void     t_sim_escribir(uint16_t id, const Simbolo *s);
uint16_t t_sim_alta(const char *nombre, uint8_t tipo, uint8_t clase);
uint16_t t_sim_dim_nueva(const Dim *d);
#include "../../compiler/src/sem_decl.c"

/* generador: llamadas (antes en el banco 44, que se quedo corto con S3); lo que necesita de gen.c vuelve por trampolin */
#define gen_expr              t_gen_expr
#define gen_desplazamiento    t_gen_desplazamiento
#define gen_subcadena         t_gen_subcadena
#define sim_dim_leer          t_sim_dim_leer
void t_sim_dim_leer(uint16_t indice, Dim *d);
#include "../../compiler/src/gen_ll.c"
