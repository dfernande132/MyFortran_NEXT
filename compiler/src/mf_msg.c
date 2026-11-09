/* mf_msg.c - ver mf_msg.h. En PC se compila entero; en el Next cada banco define
 * MF_MSG_BANCO (46 lexer, 47 expr+stmt+sem+unid+gen) antes de incluirlo y solo
 * obtiene la funcion de su fase. */
#include "mf_api.h"
#include "mf_msg.h"
#include "lexer.h"
#include "ast.h"
#include "expr.h"
#include "stmt.h"
#include "sem.h"
#include "unid.h"

static void copiar_msg(char *dst, const char *src)
{
    uint8_t i = 0;

    while (src[i] != '\0' && i < MF_MENSAJE_MAX - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

#if !defined(MF_MSG_BANCO) || MF_MSG_BANCO == 46
void msg_lex(uint8_t e, char *dst)
{
    copiar_msg(dst, lex_mensaje((ErrorLex)e));
}
#endif

#if !defined(MF_MSG_BANCO) || MF_MSG_BANCO == 47
void msg_expr(uint8_t e, char *dst)
{
    copiar_msg(dst, expr_mensaje((ErrorExpr)e));
}

void msg_stmt(uint8_t e, char *dst)
{
    copiar_msg(dst, stmt_mensaje((ErrorStmt)e));
}

void msg_uni(uint8_t e, char *dst)             /* unid.c vive en el banco 47 */
{
    copiar_msg(dst, uni_mensaje((ErrorUni)e));
}

void msg_sem(uint8_t e, char *dst)             /* sem_msg.c tambien (el banco 45 esta lleno) */
{
    copiar_msg(dst, sem_mensaje((ErrorSem)e));
}

/* Errores de la generacion del .nex (nex.c, codigo MF_COD_GEN + e); antes en nex.c (banco 43, que se quedo corto) */
void msg_gen(uint8_t e, char *dst)
{
    static const char *const t[8] = {
        "unknown error", "construct not supported by the Z80 backend yet", "program image too large",
        "internal error in the Z80 backend", "too many labels for the Z80 backend",
        "program does not fit below $BE00", "cannot create the output file",
        "error writing the output file"
    };

    copiar_msg(dst, t[e < 8 ? e : 0]);
}
#endif
