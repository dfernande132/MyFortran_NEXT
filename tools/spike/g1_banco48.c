/* g1_banco48.c - Backend dividido (BACK_DIVIDIDO): la parte C, las plantillas CHARACTER, en el banco 48. La parte A (back.c sin ellas)
 * sigue en el banco 43 y las llama por t_back_c (g1_trampolines.c). back_c.c lleva copias renombradas de z80.c y tac_rd.c y comparte con
 * el 43 el estado (bk_*, z_est), que vive en el residente. Lo que necesita de otros bancos pasa por trampolin igual que en el 43. */
#pragma bank 48
/* renombres de sem_sf.c (antes de cualquier #include, para que tambien cambien los prototipos) */
#define tabla_texto           t_tabla_texto
#define sim_buscar            t_sim_buscar
#define intr_buscar           t_intr_buscar
#define sem_cb                t_sem_cb

#define sim_leer              t_sim_leer
#define sim_dim_leer          t_sim_dim_leer
#define sim_cuenta            t_sim_cuenta
#define sem_etq_valor         t_sem_etq_valor
#define rt_real_leer          t_rt_real_leer
#define uni_cuenta            t_uni_cuenta
#define uni_leer              t_uni_leer
#define uni_dir_get           t_uni_dir_get
#define uni_dir_set           t_uni_dir_set

#include "../../compiler/src/lexer.h"
#include "../../compiler/src/sim.h"
#include "../../compiler/src/ast.h"
#include "../../compiler/src/unid.h"
#include "../../compiler/src/back_c.c"
#include "../../compiler/src/back_rt_entrada.c"
/* Funciones de sentencia (sem_sf.c): salio del 42, que se quedo corto con S4a/DATA/READ (+507 B). Una sola puerta: sf_entrada. */
#include "../../compiler/src/sem_sf.c"
