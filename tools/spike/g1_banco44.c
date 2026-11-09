/* g1_banco44.c - Generador de TAC en el banco 44 = tac (escritura) + gen (real.c esta en el 46).
 * Lo que gen.c llama y vive en otros bancos pasa por trampolin (t_*, g1_trampolines.c). Su estado (gen, tac_n...)
 * queda en el residente (bss) y las paginas de datos de banco.h. */
#pragma bank 44

#define sim_buscar            t_sim_buscar
#define sim_leer              t_sim_leer
#define sim_dim_leer          t_sim_dim_leer
#define sem_etq_indice        t_sem_etq_indice
#define tabla_texto           t_tabla_texto
#define uni_buscar            t_uni_buscar
/* las llamadas (gen_ll.c) viven en el banco 42: el generador las alcanza por trampolin */
#define gen_funcion           t_gen_funcion
#define gen_llamada_emitir    t_gen_llamada_emitir
#define real_de_texto         t_real_de_texto      /* real.c vive en el banco 46 (el 44 se quedo corto con S2) */
#define real_de_entero        t_real_de_entero

/* tac.c vive en el banco 42 (el 44 se paso 365 B con S3b/c/d): el generador lo llama por trampolin */
#define tac_iniciar           t_tac_iniciar
#define tac_emitir            t_tac_emitir
#define tac_entero            t_tac_entero
#define tac_logico            t_tac_logico
#define tac_cadena            t_tac_cadena
#define tac_real              t_tac_real
#include "../../compiler/src/gen.c"
