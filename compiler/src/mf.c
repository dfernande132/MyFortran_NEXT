/* mf.c - Punto de entrada del compilador para el editor (mf_api.h).
 *
 * Une las fases en el orden del programa y recoge TODOS los errores (fuente,
 * sintaxis, semantica) en una unica tabla, en el orden en que aparecen.
 *
 * NOTA para la integracion en bancos: este fichero es el DRIVER y debe ser
 * RESIDENTE; desde aqui las fases de los bancos se llaman por trampolin (t_*).
 */
#include <string.h>
#include "mf_api.h"
#include "mf_msg.h"
#include "source.h"
#include "lexer.h"
#include "clasif.h"
#include "ast.h"
#include "expr.h"
#include "stmt.h"
#include "sim.h"
#include "sem.h"
#ifndef MF_SIN_GEN
#include "nex.h"
#endif
#ifndef MF_SIN_GEN
#include "gen.h"
#endif

static Fuente fuente;

static void copiar(char *dst, const char *src)
{
    uint8_t i = 0;

    while (src[i] != '\0' && i < MF_MENSAJE_MAX - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static const MfSalida *salida;          /* NULL = solo comprobar */

void mf_salida(const MfSalida *s)
{
    salida = s;
}

void mf_mensaje(uint8_t cod, char *dst)
{
    if (cod >= MF_COD_UNI) {
        msg_uni((uint8_t)(cod - MF_COD_UNI), dst);
    } else
#ifndef MF_SIN_GEN
    if (cod >= MF_COD_GEN) {
        msg_gen((uint8_t)(cod - MF_COD_GEN), dst);
    } else
#endif
    if (cod >= MF_COD_LEX) {
        msg_lex((uint8_t)(cod - MF_COD_LEX), dst);
    } else if (cod >= MF_COD_EXPR) {
        msg_expr((uint8_t)(cod - MF_COD_EXPR), dst);
    } else if (cod >= MF_COD_STMT) {
        msg_stmt((uint8_t)(cod - MF_COD_STMT), dst);
    } else if (cod >= MF_COD_FUENTE) {
        switch (cod - MF_COD_FUENTE) {
        case FUENTE_ETIQUETA_INVALIDA:
            copiar(dst, "invalid statement label field");
            break;
        case FUENTE_CONTINUACION_INVALIDA:
            copiar(dst, "invalid continuation line");
            break;
        case FUENTE_SENTENCIA_LARGA:
            copiar(dst, "statement has too many continuation lines");
            break;
        default:
            copiar(dst, "unknown error");
            break;
        }
    } else {
        msg_sem(cod, dst);
    }
}

/* Registra el error de la sentencia en curso (stm.err != STM_OK). */
static void registrar_stm(void)
{
    uint8_t cod;

    if (stm.err == STM_LEXICO) {
        cod = (uint8_t)(MF_COD_LEX + stm.err_lex);
    } else if (stm.err == STM_EXPR) {
        cod = (stm.err_expr == EXPR_LEXICO)
              ? (uint8_t)(MF_COD_LEX + stm.err_lex)
              : (uint8_t)(MF_COD_EXPR + stm.err_expr);
    } else {
        cod = (uint8_t)(MF_COD_STMT + stm.err);
    }
    mf_diag_pos(cod, stm.err_pos);
}

/* Semantica de la sentencia ya analizada + registro de errores (tambien los
 * tardios de las listas, que solo salen al recorrerlas dentro de sem). */
static void procesar(void)
{
    if (stm.err == STM_OK) {
        sem_sentencia();
    }
    if (stm.err != STM_OK) {
        sem_sentencia_erronea();
        registrar_stm();
    }
}

uint16_t mf_compilar(void)
{
    ErrorFuente e;

    tabla_iniciar();
    sem_iniciar();                      /* tambien deja mf vacia */
#ifndef MF_SIN_GEN
    gen_activar(1);
#endif
    fuente_iniciar(&fuente);
    for (;;) {
        e = leer_sentencia(&fuente);
        if (e == FUENTE_FIN) {
            break;
        }
        if (e != FUENTE_OK) {
            mf_diag((uint8_t)(MF_COD_FUENTE + e), fuente.linea_error,
                    e == FUENTE_ETIQUETA_INVALIDA ? 1
                    : e == FUENTE_CONTINUACION_INVALIDA ? 6 : 7);
            continue;
        }
        stmt_analizar();
        procesar();
        if (stm.err == STM_OK && stm.tipo == SENT_IF_LOGICO) {
            stmt_analizar_interna();
            procesar();
        }
    }
    sem_fin_fichero(fuente.num_linea);
#ifndef MF_SIN_GEN
    if (mf.n == 0 && salida != NULL) {
        mf.gen_cod = nex_generar(salida);
        mf.gen = mf.gen_cod == 0 ? MF_GEN_OK : MF_GEN_ERR;
    }
#endif
    return mf.n;
}
