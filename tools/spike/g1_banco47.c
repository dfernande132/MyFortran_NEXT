/* g1_banco47.c - Generador+backend: fase "sintaxis" en el banco 47 (igual que i1), mas los dos programas de
 * prueba del driver: viven aqui (rodata del banco, hay sitio) y no en el residente, donde ocupaban ~700 B.
 * El driver los carga en el buffer del editor con g1_cargar (via trampolin t_g1_cargar). */
#include "i1_banco47.c"
#include "../../compiler/src/intr.c"      /* tabla de intrinsecas (912 B de datos): sale del 45, que se quedo corto; sem.c la consulta por t_intr_buscar */
#include "sim.h"
void t_sim_leer(uint16_t id, Simbolo *s);
void t_sim_dim_leer(uint16_t indice, Dim *d);          /* uni_comprobar lee los formales (S3a): la tabla de simbolos vive en el banco 45 */
void t_sim_escribir(uint16_t id, const Simbolo *s);    /* la capacidad de un CHARACTER*(*) (S3c) se anota en su simbolo */
#define sim_leer              t_sim_leer
#define sim_dim_leer          t_sim_dim_leer
#define sim_escribir          t_sim_escribir
#include "../../compiler/src/unid.c"      /* directorio de unidades y registro de llamadas (decision 0014); lo llaman sem (45) y back (43) por trampolin */
#undef sim_leer
#undef sim_dim_leer
#undef sim_escribir
#include <string.h>
#include "../../editor/src/edbuf.h"
#include "g1_programas.h"

void g1_cargar(uint8_t n)
{
    const char *t = n == 0 ? P_FIBONACCI : P_CADENAS;

    eb_iniciar();
    eb_agregar((const uint8_t *)t, (uint16_t)strlen(t));
}
