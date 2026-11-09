/* unid.c - Ver unid.h. */
#include <string.h>
#include "unid.h"
#include "mf_api.h"
#include "banco.h"
#include "sim.h"

static uint8_t  nunid;
static uint16_t reg_fin;                /* primer byte libre de los registros                    */
static uint8_t  principales;

/* comprobaciones solo en PC: el plegado de constantes de sccz80 no admite estas expresiones */
#ifndef MF_TARGET_NEXT
typedef char unid_comprobar_tam[(sizeof(Unidad) == 40) ? 1 : -1];
typedef char unid_comprobar_dir1[(UNI_DIR_FIN <= UNI_ADIR_OFF) ? 1 : -1];
typedef char unid_comprobar_dir2[(UNI_ADIR_OFF + 2 * (UNI_MAX + 1) <= UNI_ADY_OFF) ? 1 : -1];
typedef char unid_comprobar_dir3[(UNI_ADY_OFF + 8 * (UNI_MAX + 1) <= UNI_REG_OFF) ? 1 : -1];
#endif

void uni_iniciar(void)
{
    uint8_t cero[64];
    uint16_t off;

    memset(cero, 0, sizeof(cero));
    for (off = UNI_ADY_OFF; off < UNI_REG_OFF; off = (uint16_t)(off + 64)) {      /* 512 B = 8 trozos de 64 */
        banco_escribir(UNI_PAGINA, off, cero, 64);
    }
    banco_escribir(UNI_PAGINA, UNI_REG_OFF, cero, 1);                              /* sin registros */
    nunid = 0;
    reg_fin = UNI_REG_OFF;
    principales = 0;
}

uint8_t uni_cuenta(void)
{
    return nunid;
}

void uni_leer(uint8_t u, Unidad *x)
{
    banco_leer(UNI_PAGINA, (uint16_t)((u - 1) * sizeof(Unidad)), x, sizeof(Unidad));
}

void uni_escribir(uint8_t u, const Unidad *x)
{
    banco_escribir(UNI_PAGINA, (uint16_t)((u - 1) * sizeof(Unidad)), x, sizeof(Unidad));
}

uint8_t uni_buscar(const char *nombre)
{
    Unidad x;
    uint8_t u;
    char n[NOMBRE_MAX];

    memset(n, 0, sizeof(n));
    strncpy(n, nombre, NOMBRE_MAX);
    if (n[0] == '\0') {
        return 0;
    }
    for (u = 1; u <= nunid; u++) {
        uni_leer(u, &x);
        if (memcmp(x.nombre, n, NOMBRE_MAX) == 0) {
            return u;
        }
    }
    return 0;
}

uint8_t uni_alta(const char *nombre, uint16_t linea, uint16_t col)
{
    Unidad x;

    if (nunid >= UNI_MAX) {
        return 0;
    }
    memset(&x, 0, sizeof(x));
    strncpy(x.nombre, nombre, NOMBRE_MAX);
    x.linea = linea;
    x.col = col;
    nunid++;
    uni_escribir(nunid, &x);
    return nunid;
}

uint16_t uni_dir_get(uint8_t u)
{
    uint16_t v;

    banco_leer(UNI_PAGINA, (uint16_t)(UNI_ADIR_OFF + 2 * u), &v, 2);
    return v;
}

void uni_dir_set(uint8_t u, uint16_t d)
{
    banco_escribir(UNI_PAGINA, (uint16_t)(UNI_ADIR_OFF + 2 * u), &d, 2);
}

uint8_t uni_llamada(uint8_t llamador, uint8_t llamado, uint16_t linea, uint8_t col, uint8_t n, const uint8_t *kind,
                    const uint8_t *lens, uint8_t ftipo, uint8_t flen)
{
    uint8_t cab[UNI_REG_CAB];
    uint8_t fila[8];
    uint8_t nc = 0, k, v;
    uint16_t need;

    for (k = 0; k < n; k++) {
        if ((kind[k] & 15) == TD_CHARACTER || (kind[k] & UA_ARRAY)) {
            nc++;
        }
    }
    need = (uint16_t)(UNI_REG_CAB + n + (ftipo == TD_CHARACTER ? 1 : 0) + nc);

    if ((uint32_t)reg_fin + need + UNI_REG_CAB > BANCO_PAGINA_TAM) {     /* deja sitio para el terminador */
        return 0;
    }
    cab[0] = llamador;
    cab[1] = llamado;
    cab[2] = n;
    cab[3] = (uint8_t)linea;
    cab[4] = (uint8_t)(linea >> 8);
    cab[5] = col;
    cab[6] = ftipo;
    banco_escribir(UNI_PAGINA, reg_fin, cab, UNI_REG_CAB);
    if (n != 0) {
        banco_escribir(UNI_PAGINA, (uint16_t)(reg_fin + UNI_REG_CAB), kind, n);
    }
    if (ftipo == TD_CHARACTER) {
        banco_escribir(UNI_PAGINA, (uint16_t)(reg_fin + UNI_REG_CAB + n), &flen, 1);
    }
    v = (uint8_t)(UNI_REG_CAB + n + (ftipo == TD_CHARACTER ? 1 : 0));
    for (k = 0; k < n; k++) {
        if ((kind[k] & 15) == TD_CHARACTER || (kind[k] & UA_ARRAY)) {
            banco_escribir(UNI_PAGINA, (uint16_t)(reg_fin + v), &lens[k], 1);
            v++;
        }
    }
    reg_fin = (uint16_t)(reg_fin + need);
    cab[0] = 0;                                  /* terminador */
    banco_escribir(UNI_PAGINA, reg_fin, cab, 1);
    banco_leer(UNI_PAGINA, (uint16_t)(UNI_ADY_OFF + 8 * llamador), fila, 8);
    fila[llamado >> 3] = (uint8_t)(fila[llamado >> 3] | (1u << (llamado & 7)));
    banco_escribir(UNI_PAGINA, (uint16_t)(UNI_ADY_OFF + 8 * llamador), fila, 8);
    return 1;
}

const char *uni_mensaje(ErrorUni e)
{
    switch (e) {
    case UNI_OK:                 return "no error";
    case UNI_SIN_PRINCIPAL:      return "no main program";
    case UNI_VARIOS_PRINCIPALES: return "more than one main program";
    case UNI_YA_DEFINIDA:        return "program unit already defined";
    case UNI_CABECERA:           return "PROGRAM, SUBROUTINE or FUNCTION must be the first statement";
    case UNI_TABLA_LLENA:        return "too many program units";
    case UNI_INDEFINIDA:         return "undefined subroutine";
    case UNI_NUM_ARGS:           return "argument count mismatch";
    case UNI_TIPO_ARG:           return "argument type mismatch";
    case UNI_RECURSIVA:          return "recursive call is not allowed in FORTRAN 77";
    case UNI_MUCHOS_ARGS:        return "too many arguments";
    case UNI_NO_SUBRUTINA:       return "name is not a subroutine";
    case UNI_RETURN_PRINCIPAL:   return "RETURN is not allowed in the main program";
    case UNI_ARG_DUPLICADO:      return "duplicate dummy argument";
    case UNI_MUCHAS_LLAMADAS:    return "too many calls";
    case UNI_NO_FUNCION:         return "name is not a function";
    case UNI_TIPO_FUNCION:       return "function result type mismatch";
    case UNI_FUNCION_INDEFINIDA: return "undefined function";
    case UNI_LONG_FUNCION:       return "function result length mismatch";
    case UNI_LONG_ARG:           return "argument shorter than dummy argument";
    case UNI_TAM_ARRAY:          return "array argument smaller than dummy array";
    case UNI_LONG_ARRAY:         return "array element length mismatch";
    case UNI_FSENT_DUPLICADA:    return "statement function already defined";
    case UNI_FSENT_GRANDE:       return "statement function too large";
    }
    return "unknown error";
}

/* Cierre transitivo de la matriz de llamadas (Warshall sobre filas de 64 bits): si u se alcanza a si misma, es recursiva. */
static void recursion(void)
{
    uint8_t fila_i[8], fila_k[8];
    uint8_t i, k, b;
    Unidad x;

    for (k = 1; k <= nunid; k++) {
        banco_leer(UNI_PAGINA, (uint16_t)(UNI_ADY_OFF + 8 * k), fila_k, 8);
        for (i = 1; i <= nunid; i++) {
            banco_leer(UNI_PAGINA, (uint16_t)(UNI_ADY_OFF + 8 * i), fila_i, 8);
            if (fila_i[k >> 3] & (1u << (k & 7))) {
                for (b = 0; b < 8; b++) {
                    fila_i[b] = (uint8_t)(fila_i[b] | fila_k[b]);
                }
                banco_escribir(UNI_PAGINA, (uint16_t)(UNI_ADY_OFF + 8 * i), fila_i, 8);
            }
        }
    }
    for (i = 1; i <= nunid; i++) {
        banco_leer(UNI_PAGINA, (uint16_t)(UNI_ADY_OFF + 8 * i), fila_i, 8);
        if (fila_i[i >> 3] & (1u << (i & 7))) {
            uni_leer(i, &x);
            mf_diag((uint8_t)(MF_COD_UNI + UNI_RECURSIVA), x.linea, x.col);
        }
    }
}

/* ¿Concuerda el argumento real (kind, info) con el formal (ftipo; simbolo fid)? UNI_OK o el error. */
static uint8_t comprobar_arg(uint8_t ak, uint8_t info, uint8_t fk, uint16_t fid)
{
    Simbolo sf;
    Dim dm;

    if ((ak & 15) != (fk & 15)) {
        return UNI_TIPO_ARG;
    }
    if (((ak & UA_ARRAY) && !(fk & UA_ARRAY)) || (!(ak & (UA_ARRAY | UA_ELEM)) && (fk & UA_ARRAY))) {
        return UNI_TIPO_ARG;                    /* array entero a escalar, o expresion a array */
    }
    sim_leer(fid, &sf);
    if ((ak & 15) == TD_CHARACTER) {
        if (fk & UA_ARRAY) {
            sim_dim_leer(sf.extra, &dm);
            return info != dm.len ? UNI_LONG_ARRAY : UNI_OK;
        }
        if (sf.flags & SIMF_LONG_ASUM) {        /* CHARACTER*(*): su buffer ha de caber el mayor argumento real (255 = no se sabe) */
            if (info > sf.extra) {
                sf.extra = info;
                sim_escribir(fid, &sf);
            }
            return UNI_OK;
        }
        return info < sf.extra ? UNI_LONG_ARG : UNI_OK;
    }
    if ((ak & UA_ARRAY) && (fk & UA_ARRAY)) {
        sim_dim_leer(sf.extra, &dm);
        if (dm.aj == 0 && info < 255) {
            uint32_t n = (uint32_t)(dm.sup[0] - dm.inf[0] + 1);

            if (SIM_NDIM(&sf) == 2) {
                n *= (uint32_t)(dm.sup[1] - dm.inf[1] + 1);
            }
            if (n < 255 && n > info) {
                return UNI_TAM_ARRAY;
            }
        }
    }
    return UNI_OK;
}

void uni_comprobar(void)
{
    uint16_t off = UNI_REG_OFF;
    uint8_t cab[UNI_REG_CAB];
    uint8_t kind[UNI_ARGS_MAX];
    uint8_t lens[UNI_ARGS_MAX];
    Unidad x;
    uint8_t u, k, flen, nc, v;

    principales = 0;
    for (u = 1; u <= nunid; u++) {
        uni_leer(u, &x);
        if (x.clase == UK_PROGRAMA) {
            principales++;
            if (principales > 1) {
                mf_diag((uint8_t)(MF_COD_UNI + UNI_VARIOS_PRINCIPALES), x.linea, x.col);
            }
        } else if (x.clase == UK_NADA) {
            mf_diag((uint8_t)(MF_COD_UNI + ((x.flags & UF_FUNREF) ? UNI_FUNCION_INDEFINIDA : UNI_INDEFINIDA)), x.linea, x.col);
        }
    }
    if (principales == 0 && mf.n == 0) {
        mf_diag((uint8_t)(MF_COD_UNI + UNI_SIN_PRINCIPAL), 1, 1);
    }
    for (;;) {
        banco_leer(UNI_PAGINA, off, cab, 1);
        if (cab[0] == 0) {
            break;
        }
        banco_leer(UNI_PAGINA, off, cab, UNI_REG_CAB);
        if (cab[2] != 0) {
            banco_leer(UNI_PAGINA, (uint16_t)(off + UNI_REG_CAB), kind, cab[2]);
        }
        flen = 0;
        if (cab[6] == TD_CHARACTER) {
            banco_leer(UNI_PAGINA, (uint16_t)(off + UNI_REG_CAB + cab[2]), &flen, 1);
        }
        v = (uint8_t)(UNI_REG_CAB + cab[2] + (cab[6] == TD_CHARACTER ? 1 : 0));
        nc = 0;
        for (k = 0; k < cab[2]; k++) {
            lens[k] = 0;
            if ((kind[k] & 15) == TD_CHARACTER || (kind[k] & UA_ARRAY)) {
                banco_leer(UNI_PAGINA, (uint16_t)(off + v + nc), &lens[k], 1);
                nc++;
            }
        }
        off = (uint16_t)(off + v + nc);
        uni_leer(cab[1], &x);
        if (x.clase == UK_NADA) {
            continue;                              /* ya informada como indefinida */
        }
        if (x.clase != (cab[6] != 0 ? UK_FUNCION : UK_SUBRUTINA)) {
            mf_diag((uint8_t)(MF_COD_UNI + (cab[6] != 0 ? UNI_NO_FUNCION : UNI_NO_SUBRUTINA)), (uint16_t)(cab[3] | (cab[4] << 8)), cab[5]);
            continue;
        }
        if (cab[6] != 0 && x.tipo != cab[6]) {
            mf_diag((uint8_t)(MF_COD_UNI + UNI_TIPO_FUNCION), (uint16_t)(cab[3] | (cab[4] << 8)), cab[5]);
            continue;
        }
        if (cab[6] == TD_CHARACTER && x.flen != flen) {
            mf_diag((uint8_t)(MF_COD_UNI + UNI_LONG_FUNCION), (uint16_t)(cab[3] | (cab[4] << 8)), cab[5]);
            continue;
        }
        if (cab[2] != x.nformales) {
            mf_diag((uint8_t)(MF_COD_UNI + UNI_NUM_ARGS), (uint16_t)(cab[3] | (cab[4] << 8)), cab[5]);
            continue;
        }
        for (k = 0; k < cab[2]; k++) {
            uint8_t e = comprobar_arg(kind[k], lens[k], x.ftipo[k], (uint16_t)(x.sim_ini + k));

            if (e != UNI_OK) {
                mf_diag((uint8_t)(MF_COD_UNI + e), (uint16_t)(cab[3] | (cab[4] << 8)), cab[5]);
                break;
            }
        }
    }
    recursion();
}
