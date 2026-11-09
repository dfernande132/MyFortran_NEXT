/* gen_ll.c - Generador de codigo: llamadas CALL y referencias a funciones del usuario (ver gen.h, gen_int.h). */
#include "gen_int.h"

/* ------------------------------------------------------------------
 * CALL (decision 0014): copia de entrada de cada argumento al formal, CALL, copia de salida de los que son variables
 * o elementos de array. Los formales de la unidad llamada no se conocen aun (puede definirse mas abajo): PARAM los
 * nombra por posicion y el backend los resuelve.
 * ------------------------------------------------------------------ */

void gen_llamada_inicio(uint8_t uid)
{
    g.llamado = uid;
    g.base = 0;
    g.top = 0;
    g.ntemp = 0;
}

/* Recoge el argumento real n en la ranura `g.top` del buffer: variable (por referencia: copia de entrada y de salida),
 * elemento de array (idem y STE) o valor (temporal). Las referencias a funcion dentro del argumento usan ranuras posteriores. */
static void garg(uint16_t n)
{
    uint16_t nd = n;
    GArg a;
    Simbolo s;
    Dim dm;
    uint16_t id;
    uint8_t k = g.top;
    uint8_t td = ast_tdato(nd);
    uint8_t modo = GA_ENTRADA;

    if (!seguir()) {
        return;
    }
    if (k >= GEN_ARG_MAX) {
        no_soportado(ast_pos(nd));
        return;
    }
    memset(&a, 0, sizeof(a));
    id = ast_tipo(nd) == NODO_NOMBRE || ast_tipo(nd) == NODO_REF ? sim_buscar(tabla_texto(ast_a(nd))) : 0;
    if (ast_tipo(nd) == NODO_NOMBRE && id != 0 && (sim_leer(id, &s), SIM_CLASE(&s) == SIM_ARRAY)) {
        a.op = TAC_OPER(TAC_VAR, id);
        modo = GA_ARRAY;
    } else if (ast_tipo(nd) == NODO_NOMBRE && id != 0) {
        a.op = TAC_OPER(TAC_VAR, id);
        modo = GA_VAR;
    } else if (ast_tipo(nd) == NODO_REF && id != 0 && (sim_leer(id, &s), SIM_CLASE(&s) == SIM_ARRAY)) {
        uint16_t off, d;

        sim_dim_leer(s.extra, &dm);
        off = gen_desplazamiento(n, &s, &dm);
        d = nuevo_temp();
        if (td == TD_CHARACTER) {
            emitir(TOP_TMP, TD_CHARACTER, d, tac_entero((int16_t)dm.len), 0);
        }
        emitir(TOP_LDE, td, d, TAC_OPER(TAC_VAR, id), off);
        a.op = d;
        a.arr = TAC_OPER(TAC_VAR, id);
        a.off = off;
        modo = GA_ELEM;
    } else if (ast_tipo(nd) == NODO_REF && id != 0 && SIM_CLASE(&s) == SIM_VARIABLE && SIM_TIPO(&s) == TD_CHARACTER
               && ast_tipo(ast_a(ast_b(nd))) == NODO_RANGO) {           /* S(i:j): por referencia */
        a.op = gen_expr(n, 0);
        if (a.op == 0) {
            return;
        }
        a.arr = TAC_OPER(TAC_VAR, id);
        a.lo = g.slo;
        a.hi = g.shi;
        modo = GA_SUB;
    } else if (ast_tipo(nd) == NODO_SUBCAD) {                                          /* A(k)(i:j): por referencia */
        uint16_t ref = ast_a(nd);
        uint16_t off, d;

        id = sim_buscar(tabla_texto(ast_a(ref)));
        sim_leer(id, &s);
        sim_dim_leer(s.extra, &dm);
        off = gen_desplazamiento(ast_a(nd), &s, &dm);
        d = nuevo_temp();
        emitir(TOP_TMP, TD_CHARACTER, d, tac_entero((int16_t)dm.len), 0);
        emitir(TOP_LDE, TD_CHARACTER, d, TAC_OPER(TAC_VAR, id), off);
        a.op = gen_subcadena(d, dm.len, ast_a(ast_b(nd)), 0);
        a.arr = TAC_OPER(TAC_VAR, id);
        a.off = off;
        a.lo = g.slo;
        a.hi = g.shi;
        modo = GA_SUBE;
    } else {
        a.op = gen_expr(n, 0);
        if (a.op == 0) {
            return;
        }
    }
    GA_PON(&a, td, modo);
    banco_escribir(GEN_PAGINA, (uint16_t)(GEN_ARG_OFF + k * sizeof(GArg)), &a, sizeof(GArg));
    g.top = (uint8_t)(k + 1);
}

void gen_llamada_arg(uint8_t k)
{
    (void)k;
    garg(stm.e);
}

/* PARAM de entrada, CALL y PARAM de salida de la llamada en curso (ranuras base..top-1). */
void gen_llamada_emitir(void)
{
    GArg a;
    uint8_t k, td, modo;

    for (k = g.base; k < g.top; k++) {
        banco_leer(GEN_PAGINA, (uint16_t)(GEN_ARG_OFF + k * sizeof(GArg)), &a, sizeof(GArg));
        td = GA_TIPO(&a);
        modo = GA_MODO(&a);
        if (modo == GA_ARRAY) {
            emitir(TOP_PARAM, td, a.op, tac_entero((int16_t)(512 + k - g.base)), tac_entero(g.llamado));
        } else if (modo == GA_ELEM) {            /* valor o direccion segun el formal (el backend lo decide): EXT arr, off */
            emitir(TOP_PARAM, td, a.op, tac_entero((int16_t)(1024 + k - g.base)), tac_entero(g.llamado));
            emitir(TOP_EXT, 0, a.arr, a.off, 0);
        } else {
            emitir(TOP_PARAM, td, a.op, tac_entero((int16_t)(k - g.base)), tac_entero(g.llamado));
        }
    }
    emitir(TOP_CALL, 0, tac_entero(g.llamado), 0, 0);
    for (k = g.base; k < g.top; k++) {
        banco_leer(GEN_PAGINA, (uint16_t)(GEN_ARG_OFF + k * sizeof(GArg)), &a, sizeof(GArg));
        td = GA_TIPO(&a);
        modo = GA_MODO(&a);
        if (modo == GA_ENTRADA || modo == GA_ARRAY) {
            continue;
        }
        if (modo == GA_ELEM) {
            emitir(TOP_PARAM, td, a.op, tac_entero((int16_t)(1280 + k - g.base)), tac_entero(g.llamado));
            emitir(TOP_EXT, 0, a.arr, a.off, 0);
            emitir(TOP_STE, td, a.arr, a.off, a.op);
            continue;
        }
        emitir(TOP_PARAM, td, a.op, tac_entero((int16_t)(256 + k - g.base)), tac_entero(g.llamado));
        if (modo == GA_SUB) {                    /* el trozo vuelve a la variable: SSET var, lo, temporal + EXT hi */
            emitir(TOP_SSET, TD_CHARACTER, a.arr, a.lo, a.op);
            emitir(TOP_EXT, 0, a.hi, 0, 0);
        } else if (modo == GA_SUBE) {            /* ... al elemento: se recarga, se modifica el trozo y se guarda con STE */
            uint16_t b2 = nuevo_temp();
            Simbolo sa;
            Dim da;

            sim_leer(TAC_VALOR(a.arr), &sa);
            sim_dim_leer(sa.extra, &da);
            emitir(TOP_TMP, TD_CHARACTER, b2, tac_entero((int16_t)da.len), 0);
            emitir(TOP_LDE, TD_CHARACTER, b2, a.arr, a.off);
            emitir(TOP_SSET, TD_CHARACTER, b2, a.lo, a.op);
            emitir(TOP_EXT, 0, a.hi, 0, 0);
            emitir(TOP_STE, TD_CHARACTER, a.arr, a.off, b2);
        }
    }
}

/* F(argumentos) con F una funcion del usuario: argumentos, llamada y lectura del resultado en un temporal. */
uint16_t gen_funcion(uint16_t n, uint8_t u)
{
    uint16_t nd = n;
    uint8_t ll = g.llamado, ba = g.base, tp = g.top;
    uint16_t c, d = 0;

    g.llamado = u;
    g.base = g.top;
    for (c = ast_b(nd); c != 0 && seguir(); c = ast_b(c)) {
        garg(ast_a(c));
    }
    if (seguir()) {
        gen_llamada_emitir();
        d = nuevo_temp();
        if (ast_tdato(nd) == TD_CHARACTER) {                     /* el resultado CHARACTER*n: un temporal de n caracteres */
            Simbolo s;

            sim_leer(sim_buscar(tabla_texto(ast_a(nd))), &s);
            emitir(TOP_TMP, TD_CHARACTER, d, tac_entero((int16_t)s.extra), 0);
            g.cap = s.extra;
        }
        emitir(TOP_FUNR, ast_tdato(nd), d, tac_entero(u), 0);
    }
    g.llamado = ll;
    g.base = ba;
    g.top = tp;
    return d;
}
