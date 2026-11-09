/* g1_mf.c - Generador+backend: el DRIVER mf.c y mf_diag.c, RESIDENTES (como i1_mf.c) y con el generador de
 * TAC en el banco 44: gen_activar pasa por trampolin. */
#define tabla_iniciar          t_tabla_iniciar
#define fuente_iniciar         t_fuente_iniciar
#define leer_sentencia         t_leer_sentencia
#define stmt_analizar          t_stmt_analizar
#define stmt_analizar_interna  t_stmt_analizar_interna
#define sem_iniciar            t_sem_iniciar
#define sem_sentencia          t_sem_sentencia
#define sem_fin_fichero        t_sem_fin_fichero
#define posicion_fuente        t_posicion_fuente
#define msg_lex                t_msg_lex
#define msg_expr               t_msg_expr
#define msg_stmt               t_msg_stmt
#define msg_sem                t_msg_sem
#define msg_uni                t_msg_uni
#define sem_sentencia_erronea  t_sem_sentencia_erronea
#define gen_activar            t_gen_activar
#define nex_generar            t_nex_generar
#define msg_gen                t_msg_gen

#include "../../compiler/src/mf_diag.c"
#include "../../compiler/src/mf.c"
