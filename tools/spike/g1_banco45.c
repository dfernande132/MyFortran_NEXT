/* g1_banco45.c - Generador+backend: semantico (sim + sem) en el banco 45, como i1_banco45.c, pero los
 * ganchos del generador (gen_*) viven en el banco 44: pasan por trampolin. */
#pragma bank 45

#define tabla_texto           t_tabla_texto
#define lex_saltar_blancos    t_lex_saltar_blancos
#define posicion_fuente       t_posicion_fuente
#define stmt_elemento         t_stmt_elemento
#define stmt_dato             t_stmt_dato           /* DATA: un valor de la lista (stmt.c, banco 47) */

#define gen_iniciar           t_gen_iniciar
#define gen_etiqueta          t_gen_etiqueta
#define gen_sentencia         t_gen_sentencia
#define gen_print_item        t_gen_print_item
#define gen_param_item        t_gen_param_item
#define gen_read              t_gen_read
#define gen_goto_etq          t_gen_goto_etq
#define gen_fin_sentencia     t_gen_fin_sentencia
#define gen_unidad            t_gen_unidad
#define gen_llamada_inicio    t_gen_llamada_inicio
#define gen_llamada_arg       t_gen_llamada_arg

/* tabla de intrinsecas (intr.c, banco 47): el trampolin devuelve un puntero a una COPIA residente */
#define intr_buscar           t_intr_buscar

/* directorio de unidades (unid.c, banco 47) */
#define uni_iniciar           t_uni_iniciar
#define uni_buscar            t_uni_buscar
#define uni_alta              t_uni_alta
#define uni_leer              t_uni_leer
#define uni_escribir          t_uni_escribir
#define uni_llamada           t_uni_llamada
#define uni_comprobar         t_uni_comprobar

/* funciones de sentencia (sem_sf.c, banco 42): una sola puerta de entrada */
#define sf_entrada            t_sf_entrada

/* declaraciones de tipo (sem_decl.c, banco 42): una sola puerta de entrada */
#define decl_entrada          t_decl_entrada

#include "../../compiler/src/intr.h"
#include "../../compiler/src/sim.c"
#include "../../compiler/src/sem.c"
