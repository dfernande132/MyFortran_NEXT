/* sem_decl.c - Sentencias de declaracion de tipo (INTEGER, REAL, ..., con dimensiones y CHARACTER*n): declarar() y sus ayudas.
 *
 * Sale de sem.c para dejar sitio al semantico en el banco 45 (en el Next vive en el banco 42, como sem_sf.c). Una sola puerta de
 * entrada (decl_entrada, de sem.c hacia aqui) y la de vuelta es sem_cb (sem_sf.h). Su estado esta en el residente (sem, stm, stm_ast). */
#include <string.h>
#include "sem.h"
#include "sem_sf.h"
#include "sem_decl.h"
#include "mf_api.h"
#include "source.h"
#include "banco.h"
#include "unid.h"
#include "intr.h"
#include "tac.h"
#include "real.h"

static void decl_error(ErrorSem cod, uint16_t pos)
{
    sem_cb(SEMCB_ERROR, cod, pos);
}

/* Valor de una expresion entera constante: literales, PARAMETER INTEGER y + - * / ** con parentesis. 0 si no lo es o se sale de 16 bits. */
static uint8_t decl_valor(uint16_t n, int32_t *v)
{
    uint16_t nd = n;
    int32_t x, y;
    Simbolo s;
    uint16_t id;

    switch (ast_tipo(nd)) {
    case NODO_ENTERO:
        x = (int32_t)ast_valor(nd);
        if (x > 32768L) {
            return 0;
        }
        *v = x;
        return 1;
    case NODO_NOMBRE:
        id = sim_buscar(tabla_texto(ast_a(nd)));
        if (id == 0) {
            return 0;
        }
        sim_leer(id, &s);
        if (!SIM_ES_PARAM(&s) || SIM_TIPO(&s) != TD_INTEGER) {
            return 0;
        }
        *v = (int16_t)s.extra;
        return 1;
    case NODO_PAREN: case NODO_POS:
        return decl_valor(ast_a(nd), v);
    case NODO_NEG:
        if (!decl_valor(ast_a(nd), &x)) {
            return 0;
        }
        *v = -x;
        return *v >= -32768L && *v <= 32767L;
    case NODO_ADD: case NODO_SUB: case NODO_MUL: case NODO_DIV: case NODO_POW:
        if (!decl_valor(ast_a(nd), &x) || !decl_valor(ast_b(nd), &y)) {
            return 0;
        }
        break;
    default:
        return 0;
    }
    switch (ast_tipo(nd)) {
    case NODO_ADD: x += y; break;
    case NODO_SUB: x -= y; break;
    case NODO_MUL: x *= y; break;
    case NODO_DIV:
        if (y == 0) {
            return 0;
        }
        x /= y;
        break;
    default: {                                  /* x**y con y >= 0 */
        int32_t r = 1;

        if (y < 0) {
            return 0;
        }
        while (y-- > 0) {
            r *= x;
            if (r > 32768L || r < -32768L) {
                return 0;
            }
        }
        x = r;
        break;
    }
    }
    *v = x;
    return x >= -32768L && x <= 32767L;
}

/* Valor de una constante entera de 16 bits (con signo, PARAMETER y operaciones); 0 si no lo es. */
static uint8_t decl_constante(uint16_t n, int16_t *v)
{
    int32_t x;

    if (!decl_valor(n, &x) || x > 32767L) {
        return 0;
    }
    *v = (int16_t)x;
    return 1;
}

static uint16_t longitud_const(uint16_t n)
{
    int16_t v;

    if (!decl_constante(n, &v) || v < 1 || v > SEM_CHAR_MAX) {
        return 0;
    }
    return (uint16_t)v;
}

/* Limite superior variable de un formal ajustable (A(N)): el nombre de un INTEGER formal escalar de la unidad. Devuelve su id o 0. */
static uint16_t limite_variable(uint16_t en)
{
    uint16_t id;
    Simbolo s;

    if (ast_tipo(en) != NODO_NOMBRE) {
        return 0;
    }
    id = sim_buscar(tabla_texto(ast_a(en)));
    if (id == 0) {
        return 0;
    }
    sim_leer(id, &s);
    if (!(s.flags & SIMF_FORMAL) || SIM_CLASE(&s) != SIM_VARIABLE || SIM_TIPO(&s) != TD_INTEGER) {
        return 0;
    }
    return id;
}

/* Dimensiones de una lista de argumentos de un NODO_REF. formal = 1 si se declara un argumento formal: entonces el limite
 * superior puede ser un INTEGER formal (ajustable) y el de la ultima dimension "*" (tamano supuesto). */
static uint8_t dimensiones(uint16_t ref, Dim *d, uint8_t formal)
{
    uint16_t c;
    uint8_t k = 0;

    for (c = ast_b(ref); c != 0; c = ast_b(c)) {
        uint16_t e = ast_a(c);
        uint16_t en = e;
        uint16_t hn = en;
        int16_t lo = 1, hi = 1;
        uint16_t sv = 0;
        uint8_t estrella = 0;

        if (k == 2) {
            decl_error(SEM_DIMENSIONES, ast_pos(en));
            return 0;
        }
        if (ast_tipo(en) == NODO_RANGO) {
            if (ast_a(en) == 0 || ast_b(en) == 0 || !decl_constante(ast_a(en), &lo)) {
                decl_error(SEM_DIMENSION, ast_pos(en));
                return 0;
            }
            hn = ast_b(en);
        }
        if (ast_tipo(hn) == NODO_ESTRELLA) {
            estrella = 1;
            hi = lo;
        } else if (formal && !decl_constante(hn, &hi) && (sv = limite_variable(hn)) != 0) {
            hi = lo;                                    /* el valor se conoce al llamar: el limite no se usa al compilar */
        } else if (!estrella && sv == 0 && (!decl_constante(hn, &hi) || hi < lo)) {
            decl_error(SEM_DIMENSION, ast_pos(en));
            return 0;
        }
        if (estrella && !formal) {
            decl_error(SEM_DIMENSION, ast_pos(en));
            return 0;
        }
        if (estrella) {
            d->aj = DIM_ASUMIDO;
        } else if (d->aj & DIM_ASUMIDO) {
            decl_error(SEM_DIMENSION, ast_pos(en));              /* "*" solo en la ultima dimension */
            return 0;
        }
        if (sv != 0) {
            d->aj |= DIM_VARIABLE;
            if (k == 0) {
                d->svar = sv;
            }
        }
        d->inf[k] = lo;
        d->sup[k] = hi;
        k++;
    }
    return k;
}

/* El simbolo es la variable resultado de la FUNCTION en curso. */
static uint8_t es_resultado(uint16_t id)
{
    Unidad x;

    if (sem.unidad == 0) {
        return 0;
    }
    uni_leer(sem.unidad, &x);
    return x.sim_res == id;
}

static void declarar(uint8_t tipo, uint16_t e)
{
    uint16_t dec = e;
    uint16_t nom = ast_a(dec);
    const char *nombre = tabla_texto(ast_a(nom));
    uint8_t clase = ast_tipo(nom) == NODO_REF ? SIM_ARRAY : SIM_VARIABLE;
    Dim d;
    uint16_t len = 1;
    uint8_t ndim = 0;
    uint8_t asumida = 0;
    uint16_t id;
    Simbolo s;

    memset(&d, 0, sizeof(d));
    id = sim_buscar(nombre);
    if (id != 0) {
        sim_leer(id, &s);                               /* s.flags: ¿es un formal sin declarar? */
    }
    if (tipo == TD_CHARACTER) {
        uint16_t nl = ast_b(dec) != 0 ? ast_b(dec) : stm.c;

        if (nl != 0 && ast_tipo(nl) == NODO_ESTRELLA) {
            asumida = 1;                                /* CHARACTER*(*) */
            len = 0;
            if (id == 0 || !(s.flags & SIMF_FORMAL) || clase != SIM_VARIABLE || es_resultado(id)) {
                decl_error(SEM_LONGITUD, ast_pos(nl));      /* solo un formal escalar */
                return;
            }
        } else if (nl != 0) {
            len = longitud_const(nl);
            if (len == 0) {
                decl_error(SEM_LONGITUD, ast_pos(nl));
                return;
            }
        }
    }
    if (clase == SIM_ARRAY) {
        ndim = dimensiones(nom, &d, (uint8_t)(id != 0 && (s.flags & SIMF_FORMAL) && !(s.flags & SIMF_TIPO_EXPLICITO)));
        if (ndim == 0) {
            return;
        }
    }
    if (id != 0) {
        if ((s.flags & SIMF_PROC) && SIM_CLASE(&s) == SIM_VARIABLE && !(s.flags & SIMF_TIPO_EXPLICITO) && clase == SIM_VARIABLE
            && tipo != TD_DOUBLE && !asumida) {         /* EXTERNAL F antes de INTEGER F: da el tipo de la funcion */
            s.tc = SIM_TC(tipo, SIM_VARIABLE);
            s.flags |= SIMF_TIPO_EXPLICITO;
            s.extra = tipo == TD_CHARACTER ? len : 0;
            sim_escribir(id, &s);
            return;
        }
        if (SIM_CLASE(&s) == SIM_ARRAY && !(s.flags & SIMF_TIPO_EXPLICITO)) {   /* DIMENSION A(..) antes de INTEGER A */
            if (clase == SIM_ARRAY) {
                decl_error(SEM_YA_DIMENSIONADO, ast_pos(nom));
            } else if (tipo == TD_DOUBLE || asumida) {
                decl_error(SEM_GEN_NO_SOPORTADO, ast_pos(nom));
            } else {
                sim_dim_leer(s.extra, &d);
                d.len = len;
                s.tc = SIM_TC(tipo, SIM_ARRAY);
                s.flags |= SIMF_TIPO_EXPLICITO;
                s.extra = sim_dim_nueva(&d);
                if (s.extra == 0xFFFF) {
                    decl_error(SEM_MUCHAS_DIMS, ast_pos(nom));
                    return;
                }
                sim_escribir(id, &s);
            }
            return;
        }
        if ((s.flags & SIMF_FORMAL) && !(s.flags & SIMF_TIPO_EXPLICITO)) {      /* el tipo de un formal se declara una vez */
            if (tipo == TD_CHARACTER && clase == SIM_VARIABLE && es_resultado(id)) {   /* CHARACTER F*n dentro de FUNCTION F */
                s.tc = SIM_TC(tipo, clase);
                s.extra = len;
                s.flags |= SIMF_TIPO_EXPLICITO;
                sim_escribir(id, &s);
                return;
            }
            if (tipo == TD_DOUBLE) {
                decl_error(SEM_GEN_NO_SOPORTADO, ast_pos(nom));
                return;
            }
            s.tc = SIM_TC(tipo, clase);
            s.flags |= SIMF_TIPO_EXPLICITO;
            if (clase == SIM_ARRAY) {                   /* argumento formal array (S3b/S3c) */
                d.len = len;
                s.extra = sim_dim_nueva(&d);
                if (s.extra == 0xFFFF) {
                    decl_error(SEM_MUCHAS_DIMS, ast_pos(nom));
                    return;
                }
                s.flags |= (uint8_t)(ndim << SIMF_NDIM_SHIFT);
            } else if (tipo == TD_CHARACTER) {
                s.extra = len;
                if (asumida) {
                    s.flags |= SIMF_LONG_ASUM;
                }
            }
            sim_escribir(id, &s);
            return;
        }
        decl_error(SEM_YA_DEFINIDO, ast_pos(nom));
        return;
    }
    nombre = tabla_texto(ast_a(nom));                      /* ranura de nombres reutilizada por dimensiones() */
    id = sim_alta(nombre, tipo, clase);
    if (id == 0) {
        decl_error(SEM_TABLA_LLENA, ast_pos(nom));
        return;
    }
    sim_leer(id, &s);
    s.flags |= SIMF_TIPO_EXPLICITO;
    if (clase == SIM_ARRAY) {
        d.len = len;
        s.extra = sim_dim_nueva(&d);
        if (s.extra == 0xFFFF) {
            decl_error(SEM_MUCHAS_DIMS, ast_pos(nom));
        }
        s.flags |= (uint8_t)(ndim << SIMF_NDIM_SHIFT);
    } else if (tipo == TD_CHARACTER) {
        s.extra = len;
    }
    sim_escribir(id, &s);
}

/* Tipo implicito F77: I-N entero, el resto real. */
static uint8_t decl_implicito(const char *nombre)
{
    return (nombre[0] >= 'I' && nombre[0] <= 'N') ? TD_INTEGER : TD_REAL;
}

/* DIMENSION nombre(limites): convierte en array una variable ya declarada (conserva su tipo) o crea un array de tipo implicito. */
static void dimension(uint16_t e)
{
    uint16_t nom = ast_a(e);
    Dim d;
    Simbolo s;
    uint16_t id;
    uint16_t len = 1;
    uint8_t ndim;

    if (sem.ejec) {
        decl_error(SEM_DECL_TARDE, ast_pos(nom));
        return;
    }
    if (ast_tipo(nom) != NODO_REF) {
        decl_error(SEM_DIM_SIN_LIMITES, ast_pos(nom));
        return;
    }
    memset(&d, 0, sizeof(d));
    id = sim_buscar(tabla_texto(ast_a(nom)));
    if (id == 0) {
        ndim = dimensiones(nom, &d, 0);
        if (ndim == 0) {
            return;
        }
        d.len = 1;
        id = sim_alta(tabla_texto(ast_a(nom)), decl_implicito(tabla_texto(ast_a(nom))), SIM_ARRAY);
        if (id == 0) {
            decl_error(SEM_TABLA_LLENA, ast_pos(nom));
            return;
        }
        sim_leer(id, &s);
    } else {
        sim_leer(id, &s);
        if (SIM_CLASE(&s) == SIM_ARRAY) {
            decl_error(SEM_YA_DIMENSIONADO, ast_pos(nom));
            return;
        }
        if (SIM_CLASE(&s) != SIM_VARIABLE || (s.flags & SIMF_PROC) || es_resultado(id)) {
            decl_error(SEM_YA_DEFINIDO, ast_pos(nom));
            return;
        }
        if (s.flags & SIMF_LONG_ASUM) {
            decl_error(SEM_GEN_NO_SOPORTADO, ast_pos(nom));
            return;
        }
        if (SIM_TIPO(&s) == TD_CHARACTER) {
            len = s.extra;
        }
        ndim = dimensiones(nom, &d, (uint8_t)((s.flags & SIMF_FORMAL) != 0));
        if (ndim == 0) {
            return;
        }
        d.len = len;
        s.tc = SIM_TC(SIM_TIPO(&s), SIM_ARRAY);
    }
    s.extra = sim_dim_nueva(&d);
    if (s.extra == 0xFFFF) {
        decl_error(SEM_MUCHAS_DIMS, ast_pos(nom));
        s.extra = 0;
        return;
    }
    s.flags |= (uint8_t)(ndim << SIMF_NDIM_SHIFT);
    sim_escribir(id, &s);
}

/* EXTERNAL / INTRINSIC nombre: el nombre queda marcado como procedimiento; usarlo como valor o argumento se rechaza (0.1). */
static void procedimiento(uint8_t intrinseca, uint16_t e)
{
    uint16_t nom = ast_a(e);
    Simbolo s;
    uint16_t id;

    if (sem.ejec) {
        decl_error(SEM_DECL_TARDE, ast_pos(nom));
        return;
    }
    if (ast_tipo(nom) != NODO_NOMBRE || ast_b(e) != 0) {
        decl_error(SEM_NOMBRE_SOLO, ast_pos(nom));
        return;
    }
    if (intrinseca && intr_buscar(tabla_texto(ast_a(nom))) == 0) {
        decl_error(SEM_NO_INTRINSECA, ast_pos(nom));
        return;
    }
    id = sim_buscar(tabla_texto(ast_a(nom)));
    if (id == 0) {
        id = sim_alta(tabla_texto(ast_a(nom)), decl_implicito(tabla_texto(ast_a(nom))), intrinseca ? SIM_FUNCION : SIM_VARIABLE);
        if (id == 0) {
            decl_error(SEM_TABLA_LLENA, ast_pos(nom));
            return;
        }
        sim_leer(id, &s);
    } else {
        sim_leer(id, &s);
        if (s.flags & SIMF_FORMAL) {
            decl_error(SEM_PROC_ARG, ast_pos(nom));        /* argumento formal que es un procedimiento: 0.2 */
            return;
        }
        if (SIM_CLASE(&s) == SIM_ARRAY || intrinseca || es_resultado(id)) {
            decl_error(SEM_YA_DEFINIDO, ast_pos(nom));
            return;
        }
    }
    s.flags |= SIMF_PROC;
    sim_escribir(id, &s);
}

/* La expresion es constante: literales, nombres de PARAMETER y operadores. */
static uint8_t es_constante(uint16_t n)
{
    uint16_t nd = n;
    Simbolo s;
    uint16_t id;

    switch (ast_tipo(nd)) {
    case NODO_ENTERO: case NODO_REAL: case NODO_CADENA: case NODO_LOGICO:
        return 1;
    case NODO_NOMBRE:
        id = sim_buscar(tabla_texto(ast_a(nd)));
        if (id == 0) {
            return 0;
        }
        sim_leer(id, &s);
        return SIM_ES_PARAM(&s);
    case NODO_NEG: case NODO_POS: case NODO_NOT: case NODO_PAREN:
        return es_constante(ast_a(nd));
    case NODO_ADD: case NODO_SUB: case NODO_MUL: case NODO_DIV: case NODO_POW: case NODO_CONCAT:
    case NODO_EQ: case NODO_NE: case NODO_LT: case NODO_LE: case NODO_GT: case NODO_GE:
    case NODO_AND: case NODO_OR: case NODO_EQV: case NODO_NEQV:
        return es_constante(ast_a(nd)) && es_constante(ast_b(nd));
    default:
        return 0;
    }
}

/* PARAMETER (nombre = valor): comprobaciones antes de asignar. 1 = seguir. */
static uint8_t parametro_previo(uint16_t e)
{
    uint16_t dec = e;
    uint16_t nom = ast_a(dec);
    uint16_t id;
    Simbolo s;

    if (sem.ejec) {
        decl_error(SEM_DECL_TARDE, ast_pos(nom));
        return 0;
    }
    id = sim_buscar(tabla_texto(ast_a(nom)));
    if (id != 0) {
        sim_leer(id, &s);
        if (SIM_CLASE(&s) != SIM_VARIABLE || SIM_ES_PARAM(&s) || (s.flags & (SIMF_PROC | SIMF_FORMAL)) || es_resultado(id)) {
            decl_error(SEM_YA_DEFINIDO, ast_pos(nom));
            return 0;
        }
    }
    if (!es_constante(ast_b(dec))) {
        decl_error(SEM_PARAM_CTE, ast_pos(ast_b(dec)));
        return 0;
    }
    return 1;
}

/* PARAMETER: ya asignado sin errores: lo marca como constante (y guarda el valor de un INTEGER). */
static void parametro_fin(uint16_t e)
{
    uint16_t dec = e;
    uint16_t id = sim_buscar(tabla_texto(ast_a(ast_a(dec))));
    Simbolo s;
    int16_t v = 0;

    if (id == 0) {
        return;
    }
    sim_leer(id, &s);
    if (SIM_TIPO(&s) == TD_INTEGER && !decl_constante(ast_b(dec), &v)) {
        decl_error(SEM_PARAM_CTE, ast_pos(ast_b(dec)));        /* p. ej. un REAL en un PARAMETER INTEGER */
        return;
    }
    if (SIM_TIPO(&s) == TD_INTEGER) {
        s.extra = (uint16_t)v;
    }
    s.flags |= SIMF_NDIM_MASK | SIMF_TIPO_EXPLICITO;
    sim_escribir(id, &s);
}

/* ------------------------------------------------------------------
 * DATA. Los nombres de un grupo se guardan (id, desplazamiento, elementos) en la pagina SEM_DATA_PAGINA; cada valor se reparte entre
 * ellos en orden y deja instrucciones DATA (+ EXT con el desplazamiento) en el TAC: el backend las convierte en la inicializacion
 * que corre al arrancar el programa.
 * ------------------------------------------------------------------ */
typedef struct {
    uint16_t id, off, cnt;
} DatoDest;

static uint8_t  dt_err;                 /* ya se informo de un error en esta sentencia: no se encadenan mas */
static uint8_t  dt_n, dt_i, dt_ini;     /* nombres, el que se rellena, 1 = ya se esta repartiendo un valor */
static uint16_t dt_pos, dt_resta;       /* elemento del nombre en curso por el que va, y cuantos le quedan */

static void dato_error(ErrorSem cod, uint16_t pos)
{
    if (!dt_err) {
        decl_error(cod, pos);
    }
    dt_err = 1;
}

static void dt_cargar(void)
{
    DatoDest d;

    if (dt_i < dt_n) {
        banco_leer(SEM_DATA_PAGINA, (uint16_t)(dt_i * sizeof(d)), &d, sizeof(d));
        dt_pos = d.off;
        dt_resta = d.cnt;
    } else {
        dt_resta = 0;
    }
}

static void dato_destino(uint16_t e)
{
    uint16_t nom = e;
    Simbolo s;
    Dim dm;
    DatoDest d;
    uint16_t id;
    uint16_t n0;
    int16_t i1, i2;

    if (ast_tipo(nom) != NODO_NOMBRE && ast_tipo(nom) != NODO_REF) {
        dato_error(SEM_GEN_NO_SOPORTADO, ast_pos(nom));             /* subcadenas, DO implicitos */
        return;
    }
    id = sim_buscar(tabla_texto(ast_a(nom)));
    if (id == 0) {
        if (ast_tipo(nom) == NODO_REF) {
            dato_error(SEM_NO_ES_ARRAY, ast_pos(nom));
            return;
        }
        id = sim_alta(tabla_texto(ast_a(nom)), decl_implicito(tabla_texto(ast_a(nom))), SIM_VARIABLE);
        if (id == 0) {
            dato_error(SEM_TABLA_LLENA, ast_pos(nom));
            return;
        }
    }
    sim_leer(id, &s);
    if ((s.flags & (SIMF_FORMAL | SIMF_PROC)) || SIM_ES_PARAM(&s) || SIM_CLASE(&s) == SIM_FUNCION || es_resultado(id)) {
        dato_error(SEM_DATA_DEST, ast_pos(nom));
        return;
    }
    d.id = id;
    d.off = 0;
    d.cnt = 1;
    if (SIM_CLASE(&s) == SIM_ARRAY) {
        sim_dim_leer(s.extra, &dm);
        n0 = (uint16_t)(dm.sup[0] - dm.inf[0] + 1);
        if (ast_tipo(nom) == NODO_NOMBRE) {                         /* el array entero */
            d.cnt = n0;
            if (SIM_NDIM(&s) == 2) {
                d.cnt = (uint16_t)(n0 * (uint16_t)(dm.sup[1] - dm.inf[1] + 1));
            }
        } else {                                                /* un elemento, con indices constantes */
            uint16_t c = ast_b(nom);
            uint8_t k = 0;

            for (; c != 0; c = ast_b(c)) {
                int16_t *dst = k == 0 ? &i1 : &i2;

                if (k == 2 || !decl_constante(ast_a(c), dst)) {
                    dato_error(SEM_DATA_DEST, ast_pos(nom));
                    return;
                }
                k++;
            }
            if (k != SIM_NDIM(&s) || i1 < dm.inf[0] || i1 > dm.sup[0] || (k == 2 && (i2 < dm.inf[1] || i2 > dm.sup[1]))) {
                dato_error(SEM_NUM_INDICES, ast_pos(nom));
                return;
            }
            d.off = (uint16_t)(i1 - dm.inf[0]);
            if (k == 2) {
                d.off = (uint16_t)(d.off + (uint16_t)(i2 - dm.inf[1]) * n0);
            }
        }
    } else if (ast_tipo(nom) == NODO_REF) {
        dato_error(SEM_GEN_NO_SOPORTADO, ast_pos(nom));             /* S(i:j) */
        return;
    }
    if (dt_n >= SEM_DATA_MAX) {
        dato_error(SEM_TABLA_LLENA, ast_pos(nom));
        return;
    }
    banco_escribir(SEM_DATA_PAGINA, (uint16_t)(dt_n * sizeof(d)), &d, sizeof(d));
    dt_n++;
}

static void dato_valor(uint16_t e)
{
    uint16_t v = e;
    uint16_t c = v;                  /* la constante sin el signo */
    uint8_t neg = 0;
    uint8_t vt;                         /* tipo del valor */
    int16_t iv = 0;
    uint32_t bits = 0;
    uint16_t rep = 1;
    int16_t r16;
    uint16_t pos = ast_pos(v);

    if (ast_tipo(c) == NODO_NEG) {
        neg = 1;
        c = ast_a(c);
    }
    if (stm.c != 0) {
        if (!decl_constante(stm.c, &r16) || r16 < 1) {
            dato_error(SEM_DATA_REPET, ast_pos(stm.c));
            return;
        }
        rep = (uint16_t)r16;
    }
    if (ast_tipo(c) == NODO_REAL) {
        vt = TD_REAL;
        if (real_de_texto(tabla_texto(ast_a(c)), &bits) != REAL_OK) {
            dato_error(SEM_REAL_DESBORDA, pos);
            return;
        }
        if (neg) {
            bits ^= 0x80000000UL;
        }
    } else if (ast_tipo(c) == NODO_CADENA) {
        vt = TD_CHARACTER;
        if (neg) {
            dato_error(SEM_DATA_VALOR, pos);
            return;
        }
    } else if (ast_tipo(c) == NODO_LOGICO) {
        vt = TD_LOGICAL;
        if (neg) {
            dato_error(SEM_DATA_VALOR, pos);
            return;
        }
        iv = (int16_t)ast_a(c);
    } else {
        int32_t x;

        vt = TD_INTEGER;
        if (!decl_valor(e, &x) || x > 32767L) {
            dato_error(SEM_DATA_VALOR, pos);
            return;
        }
        iv = (int16_t)x;
    }
    dt_ini = 1;
    while (rep > 0) {
        Simbolo s;
        Dim dm;
        DatoDest d;
        uint16_t seg, op, el;
        uint8_t tt;

        if (dt_resta == 0) {
            dt_cargar();                /* siguiente nombre (la primera vez, el primero) */
            if (dt_resta == 0) {
                dato_error(SEM_DATA_CUENTA, pos);
                return;
            }
        }
        banco_leer(SEM_DATA_PAGINA, (uint16_t)(dt_i * sizeof(d)), &d, sizeof(d));
        sim_leer(d.id, &s);
        tt = SIM_TIPO(&s);
        el = 1;
        if (SIM_CLASE(&s) == SIM_ARRAY) {
            sim_dim_leer(s.extra, &dm);
            el = dm.len;
        } else {
            el = s.extra;
        }
        if (!(tt == vt || (tt == TD_REAL && vt == TD_INTEGER))) {
            dato_error(SEM_DATA_TIPO, pos);
            return;
        }
        seg = rep < dt_resta ? rep : dt_resta;
        if (tt == TD_CHARACTER) {
            const char *txt = tabla_texto(ast_a(c));
            uint16_t ln = (uint16_t)strlen(txt);

            if (ln > el) {
                ln = el;
            }
            op = ln == 0 ? tac_cadena(" ", 1) : tac_cadena(txt, (uint8_t)ln);
        } else if (tt == TD_REAL) {
            op = tac_real(vt == TD_INTEGER ? real_de_entero(iv) : bits);
        } else if (tt == TD_LOGICAL) {
            op = tac_logico((uint8_t)iv);
        } else {
            op = tac_entero(iv);
        }
        if (mf.n != 0) {
            return;
        }
        if (tac_lleno()) {
            dato_error(SEM_GEN_TAC_LLENO, pos);
            return;
        }
        tac_emitir(TOP_DATA, tt, TAC_OPER(TAC_VAR, d.id), op, tac_entero((int16_t)seg));
        tac_emitir(TOP_EXT, 0, tac_entero((int16_t)dt_pos), 0, 0);
        dt_pos = (uint16_t)(dt_pos + seg);
        dt_resta = (uint16_t)(dt_resta - seg);
        rep = (uint16_t)(rep - seg);
        if (dt_resta == 0) {
            dt_i++;
            dt_cargar();
        }
    }
}

/* Fin de un grupo (o principio de la sentencia): comprueba que no sobran nombres y deja la lista vacia. */
static void dato_fin(void)
{
    if (dt_err) {
        dt_n = dt_i = dt_ini = 0;
        return;
    }
    if (dt_ini && (dt_resta != 0 || dt_i < dt_n)) {
        dato_error(SEM_DATA_CUENTA, 0);
    } else if (!dt_ini && dt_n != 0) {
        dato_error(SEM_DATA_CUENTA, 0);                        /* nombres sin valores */
    }
    dt_n = dt_i = dt_ini = 0;
    dt_pos = dt_resta = 0;
}

uint8_t decl_entrada(uint8_t op, uint8_t tipo, uint16_t e)
{
    if (op == DECL_OP_DATA_DEST) {
        dato_destino(e);
        return 0;
    }
    if (op == DECL_OP_DATA_VAL) {
        dato_valor(e);
        return 0;
    }
    if (op == DECL_OP_DATA_FIN) {
        if (tipo != 0) {
            dt_n = dt_i = dt_ini = dt_err = 0;              /* principio de la sentencia */
            dt_pos = dt_resta = 0;
        } else {
            dato_fin();
        }
        return 0;
    }
    if (op == DECL_OP_PARAM) {
        return parametro_previo(e);
    }
    if (op == DECL_OP_PARAM_FIN) {
        parametro_fin(e);
        return 0;
    }
    if (op == DECL_OP_TIPO) {
        declarar(tipo, e);
    } else if (op == DECL_OP_DIM) {
        dimension(e);
    } else {
        procedimiento((uint8_t)(op == DECL_OP_INT), e);
    }
    return 0;
}
