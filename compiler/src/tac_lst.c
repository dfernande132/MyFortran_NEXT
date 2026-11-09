/* tac_lst.c - Listado en texto del TAC (formato de docs/especificacion-tac.md, seccion 4).
 *
 * Solo lo usan las pruebas de PC y, mas adelante, el volcado a disco. Cada linea
 * se entrega por la funcion sal(); el generador no guarda el texto.
 */
#include <stdio.h>
#include <string.h>
#include "gen.h"
#include "tac.h"
#include "sim.h"
#include "sem.h"
#include "lexer.h"
#include "unid.h"

static const char *const nombre_op[TOP_TOTAL] = {
    "NOP", "LBL", "MOV", "NEG", "ADD", "SUB", "MUL", "DIV", "POW",
    "EQ", "NE", "LT", "LE", "GT", "GE", "AND", "OR", "EQV", "NEQV", "NOT",
    "JMP", "JT", "JF", "JEQ", "LDE", "STE", "SCAT", "SSUB", "SLEN",
    "PUT", "PUTNL", "STOP", "HALT", "TMP", "EXT", "ITOF", "FTOI", "RFUN", "RMOD",
    "ENTER", "PARAM", "CALL", "RET", "FUNR", "SSET", "DATA", "RDLN", "RD"
};

static char sufijo_tipo(uint8_t t)
{
    switch (t) {
    case TD_INTEGER:   return 'I';
    case TD_REAL:      return 'R';
    case TD_DOUBLE:    return 'D';
    case TD_LOGICAL:   return 'L';
    case TD_CHARACTER: return 'C';
    default:           return 0;
    }
}

/* Escribe el operando en dst (dst: 300 B). Devuelve cuanto escribio. */
static void texto_operando(uint16_t op, char *dst)
{
    uint16_t v = TAC_VALOR(op);
    Simbolo s;
    char tmp[256];
    uint8_t len, i;
    char *p;

    switch (TAC_CLASE(op)) {
    case TAC_TEMP:
        sprintf(dst, "%%%u", (unsigned)v);
        break;
    case TAC_VAR:
        sim_leer(v, &s);
        sprintf(dst, "%.*s", NOMBRE_MAX, s.nombre);
        break;
    case TAC_INM:
        sprintf(dst, "%d", (int)tac_inm_valor(op));
        break;
    case TAC_CTE:
        switch (tac_cte_tipo(op)) {
        case TAC_C_ENTERO:
            sprintf(dst, "%d", (int)tac_cte_entero(op));
            break;
        case TAC_C_LOGICO:
            strcpy(dst, tac_cte_entero(op) ? ".T." : ".F.");
            break;
        case TAC_C_REAL: {
            uint32_t bits = tac_cte_real(op);
            float f;

            memcpy(&f, &bits, 4);
            sprintf(dst, "%.9g", (double)f);
            if (strpbrk(dst, ".en") == NULL) {          /* que no parezca un entero */
                strcat(dst, ".0");
            }
            break;
        }
        default:
            len = tac_cte_cadena(op, tmp);
            p = dst;
            *p++ = '\'';
            for (i = 0; i < len; i++) {
                if (tmp[i] == '\'') {
                    *p++ = '\'';
                }
                *p++ = tmp[i];
            }
            *p++ = '\'';
            *p = '\0';
            break;
        }
        break;
    case TAC_ETQ:
        if (v >= TAC_ETQ_INTERNA) {
            sprintf(dst, "@@%u", (unsigned)(v - TAC_ETQ_INTERNA));
        } else {
            sprintf(dst, "@%lu", (unsigned long)sem_etq_valor(v));
        }
        break;
    case TAC_OCULTA:
        sprintf(dst, "$%u", (unsigned)v);
        break;
    default:
        dst[0] = '\0';
        break;
    }
}

static void linea_var(uint16_t id, void (*sal)(const char *))
{
    Simbolo s;
    Dim dm;
    char l[96];
    char ms[24];
    int n;

    sim_leer(id, &s);
    n = sprintf(l, "VAR %.*s %s", NOMBRE_MAX, s.nombre,
                SIM_TIPO(&s) == TD_INTEGER ? "INTEGER" :
                SIM_TIPO(&s) == TD_REAL ? "REAL" :
                SIM_TIPO(&s) == TD_DOUBLE ? "DOUBLE" :
                SIM_TIPO(&s) == TD_LOGICAL ? "LOGICAL" : "CHARACTER");
    if (SIM_CLASE(&s) == SIM_ARRAY) {
        sim_dim_leer(s.extra, &dm);
        if (SIM_TIPO(&s) == TD_CHARACTER) {
            n += sprintf(l + n, "*%u", (unsigned)dm.len);
        }
        n += sprintf(l + n, " (%d:%d", (int)dm.inf[0], (int)dm.sup[0]);
        if (SIM_NDIM(&s) == 2) {
            n += sprintf(l + n, ",%d:%d", (int)dm.inf[1], (int)dm.sup[1]);
        }
        strcpy(l + n, ")");
    } else if (SIM_TIPO(&s) == TD_CHARACTER) {
        sprintf(ms, "*%u", (unsigned)s.extra);
        strcat(l, ms);
    }
    sal(l);
}

void tac_listar(void (*sal)(const char *linea))
{
    char l[1024];
    char o[3][300];
    uint16_t id, i, n = tac_cuenta();
    TacInst t;
    uint16_t cuenta_sim = sim_cuenta();
    char suf[3];
    uint8_t k, nops;

    (void)cuenta_sim;
    for (i = 0; i < n; i++) {
        uint16_t ops[3];

        tac_leer(i, &t);
        if (t.op == TOP_ENTER) {                         /* cabecera de la unidad y sus variables */
            Unidad u;

            uni_leer((uint8_t)tac_inm_valor(t.a), &u);
            sprintf(l, "UNIT %s %.*s",
                    u.clase == UK_SUBRUTINA ? "SUBROUTINE" : u.clase == UK_FUNCION ? "FUNCTION" : "PROGRAM",
                    u.nombre[0] != '\0' ? NOMBRE_MAX : 4, u.nombre[0] != '\0' ? u.nombre : "MAIN");
            sal(l);
            for (id = u.sim_ini; id != 0 && id <= u.sim_fin; id++) {
                linea_var(id, sal);
            }
            continue;
        }
        ops[0] = t.a;
        ops[1] = t.b;
        ops[2] = t.c;
        suf[0] = '\0';
        if (sufijo_tipo(t.tipo) != 0) {
            suf[0] = '.';
            suf[1] = sufijo_tipo(t.tipo);
            suf[2] = '\0';
        }
        nops = 0;
        for (k = 0; k < 3; k++) {
            if (TAC_CLASE(ops[k]) != TAC_NADA) {
                texto_operando(ops[k], o[nops]);
                nops++;
            }
        }
        /* STE: el listado es STE.T A, off, valor */
        {
            char m[16];
            int n2;

            sprintf(m, "%s%s", t.op < TOP_TOTAL ? nombre_op[t.op] : "???", suf);
            n2 = sprintf(l, "        %-6s", m);
            for (k = 0; k < nops; k++) {
                n2 += sprintf(l + n2, "%s%s", k == 0 ? " " : ", ", o[k]);
            }
            while (n2 > 0 && l[n2 - 1] == ' ') {
                l[--n2] = '\0';
            }
        }
        sal(l);
    }
}
