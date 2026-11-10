/* c5_trampolines.c - Capitulo 5: los trampolines del capitulo 4 mas los del banco 45
 * (semantico) y los que el semantico necesita hacia los bancos 46 y 47.
 * Se incluye c4_trampolines.c tal cual para compartir el estado de mapear().
 */
#include "c4_trampolines.c"
#include "sim.h"
#include "sem.h"

#define BANCO_SEM 45

/* ---- hacia el banco 46 (lectura), desde el semantico ---- */
/* ---- banco 45 (semantico) ---- */
