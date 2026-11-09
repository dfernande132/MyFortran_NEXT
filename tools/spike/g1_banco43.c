/* g1_banco43.c - Backend Z80 en el banco 43 = back + z80 + tac_rd (lectura del TAC); nex.c esta en el banco 42.
 * Sus tablas estan en la pagina de datos 10 y la imagen en las 11..12 (banco.h), no en el residente. */
#pragma bank 43

#define sim_leer              t_sim_leer
#define sim_dim_leer          t_sim_dim_leer
#define sim_cuenta            t_sim_cuenta
#define sem_etq_valor         t_sem_etq_valor
#define rt_real_leer          t_rt_real_leer
#define rt_ent_leer           t_rt_ent_leer         /* tablas del runtime de entrada (READ): banco 48 */
#define uni_cuenta            t_uni_cuenta
#define uni_leer              t_uni_leer
#define uni_dir_get           t_uni_dir_get
#define uni_dir_set           t_uni_dir_set

#include "../../compiler/src/tac_rd.c"
#include "../../compiler/src/z80.c"
#include "../../compiler/src/back.c"
