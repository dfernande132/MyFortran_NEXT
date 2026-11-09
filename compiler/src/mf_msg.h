/* mf_msg.h - Mensajes de cada fase COPIADOS a un buffer del que llama.
 * Existen porque un banco no puede devolver punteros a su RODATA (0007): cada
 * fase copia su mensaje en `dst` (residente) dentro de su propio banco. */
#ifndef MF_MSG_H
#define MF_MSG_H

#include <stdint.h>

void msg_lex(uint8_t e, char *dst);    /* ErrorLex  (fase lexer)  */
void msg_expr(uint8_t e, char *dst);   /* ErrorExpr (fase expr)   */
void msg_stmt(uint8_t e, char *dst);   /* ErrorStmt (fase stmt)   */
void msg_sem(uint8_t e, char *dst);    /* ErrorSem  (fase sem)    */
void msg_uni(uint8_t e, char *dst);    /* MF_COD_UNI + e (unid.c) */
void msg_gen(uint8_t e, char *dst);    /* MF_COD_GEN + e (mf_msg.c) */

#endif
