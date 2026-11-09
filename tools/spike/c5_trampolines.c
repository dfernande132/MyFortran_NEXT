/* c5_trampolines.c - Capitulo 5: los trampolines del capitulo 4 mas los del banco 45
 * (semantico) y los que el semantico necesita hacia los bancos 46 y 47.
 * Se incluye c4_trampolines.c tal cual para compartir el estado de mapear().
 */
#include "c4_trampolines.c"
#include "sim.h"
#include "sem.h"

#define BANCO_SEM 45

/* ---- hacia el banco 46 (lectura), desde el semantico ---- */
void t_posicion_fuente(uint16_t offset, uint16_t *linea, uint16_t *columna)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_LEC); posicion_fuente(offset, linea, columna); mapear(ant);
}

/* ---- banco 45 (semantico) ---- */
void t_sem_iniciar(void)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_SEM); sem_iniciar(); mapear(ant);
}

void t_sem_sentencia(void)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_SEM); sem_sentencia(); mapear(ant);
}

void t_sem_fin_fichero(uint16_t linea)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_SEM); sem_fin_fichero(linea); mapear(ant);
}

uint16_t t_sim_cuenta(void)
{
    uint8_t ant = banco_actual;
    uint16_t r;
    mapear(BANCO_SEM); r = sim_cuenta(); mapear(ant);
    return r;
}
