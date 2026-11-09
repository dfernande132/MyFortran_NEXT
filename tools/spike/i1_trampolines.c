/* i1_trampolines.c - Integracion: los trampolines de c5 mas los que necesita mf.c
 * (el driver RESIDENTE) para los mensajes copiados de cada banco. */
#include "c5_trampolines.c"
#include "mf_msg.h"

void t_msg_lex(uint8_t e, char *dst)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_LEC); msg_lex(e, dst); mapear(ant);
}

void t_msg_expr(uint8_t e, char *dst)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_EXP); msg_expr(e, dst); mapear(ant);
}

void t_msg_stmt(uint8_t e, char *dst)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_EXP); msg_stmt(e, dst); mapear(ant);
}

void t_msg_sem(uint8_t e, char *dst)
{
    uint8_t ant = banco_actual;
    mapear(BANCO_EXP); msg_sem(e, dst); mapear(ant);
}
