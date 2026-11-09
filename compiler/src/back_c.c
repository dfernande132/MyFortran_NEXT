/* back_c.c - Parte C del backend dividido (BACK_DIVIDIDO): las plantillas CHARACTER de back.c, para otro banco del Next.
 *
 * El banco 43 (back + z80 + tac_rd) se quedaba corto: back.c se compila DOS veces. La parte A (banco 43) trae todo menos
 * cadena_info/ld_str/array_char/direccion_elemento/traducir_c/stop_mensaje/param_c; esas funciones viven aqui (banco 48), con copias
 * de lo pequeno que necesitan (z80.c, tac_rd.c y los ayudantes de back.c) bajo otros nombres para que no choquen al enlazar.
 * El estado compartido (bk_pasada, bk_err, ..., z_est) lo define la parte A y aqui se declara extern. La parte A llama a esta por
 * t_back_c (en el Next, un trampolin residente que mapea el banco 48; en PC, la llamada directa de abajo).
 *
 * Solo se compila con -DBACK_DIVIDIDO (el PC lo usa para probar la division con las mismas suites).
 */
#ifdef BACK_DIVIDIDO
#define BACK_PARTE_C
#define z_iniciar        zc_iniciar
#define z_pc             zc_pc
#define z_desbordado     zc_desbordado
#define z_fin            zc_fin
#define z_imagen_leer    zc_imagen_leer
#define z_ins            zc_ins
#define z_datos          zc_datos
#define z_linea          zc_linea
#define tac_leer         tcc_leer
#define tac_cte_tipo     tcc_cte_tipo
#define tac_cte_entero   tcc_cte_entero
#define tac_cte_cadena   tcc_cte_cadena
#define tac_cte_real     tcc_cte_real
#include "tac_rd.c"
#include "z80.c"
#include "back.c"

#ifndef MF_TARGET_NEXT
void t_back_c(uint8_t cual, const TacInst *t, uint16_t x, uint16_t y)
{
    back_c_entrada(cual, t, x, y);
}
#endif
#else
typedef int back_c_vacio;                /* sin BACK_DIVIDIDO este fichero no aporta nada */
#endif
