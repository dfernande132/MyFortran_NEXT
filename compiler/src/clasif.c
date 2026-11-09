/* clasif.c - Ver clasif.h. */
#include "clasif.h"
#include "source.h"

#define NO_COINCIDE 0xFFFFu

static int es_letra(char c)
{
    return c >= 'A' && c <= 'Z';
}

static int es_digito(char c)
{
    return c >= '0' && c <= '9';
}

/* Todo el modulo recorre el texto con una posicion bruta p y esta funcion:
 * deja en *p la del siguiente caracter no blanco y devuelve ese caracter en
 * mayusculas ('\0' si ya no hay). Despues, "consumir" es p++. */
static char sig(uint16_t *p)
{
    *p = lex_saltar_blancos(*p);
    return lex_car(*p);
}

/* *p apunta a una comilla de apertura. Avanza hasta justo despues de la de
 * cierre y devuelve 1; si no hay cierre, devuelve 0.
 * Una comilla doble ('') cierra y reabre: se ve como dos cadenas pegadas, lo
 * que cubre exactamente lo mismo. Dentro de una cadena los blancos cuentan,
 * asi que aqui se lee en bruto. */
static int saltar_cadena(uint16_t *p)
{
    uint16_t q = (uint16_t)(*p + 1);

    while (q < lexer.longitud && sent_car(q) != '\'') {
        q++;
    }
    if (q >= lexer.longitud) {
        return 0;
    }
    *p = (uint16_t)(q + 1);
    return 1;
}

/* *p apunta a un '('. Avanza hasta justo despues de su ')' y devuelve 1; si
 * los parentesis no estan balanceados, devuelve 0. Ignora lo que haya en
 * cadenas (un ')' dentro de comillas no cuenta). */
static int saltar_grupo(uint16_t *p)
{
    uint16_t q = *p;
    int profundidad = 0;
    char c;

    for (;;) {
        c = sig(&q);
        if (c == '\0') {
            return 0;
        }
        if (c == '\'') {
            if (!saltar_cadena(&q)) {
                return 0;
            }
            continue;
        }
        if (c == '(') {
            profundidad++;
        } else if (c == ')') {
            profundidad--;
            if (profundidad == 0) {
                *p = (uint16_t)(q + 1);
                return 1;
            }
        }
        q++;
    }
}

/* Regla 1, primera parte: la sentencia que empieza en p0 es
 *     nombre { ( ... ) } =
 * Deja en *p_igual la posicion del '='. Los parentesis pueden repetirse
 * (A(1)(2:3)=..., subcadena de un elemento de array). */
static int forma_de_asignacion(uint16_t p0, uint16_t *p_igual)
{
    uint16_t p = p0;
    char c = sig(&p);

    if (!es_letra(c)) {
        return 0;
    }
    while (es_letra(c) || es_digito(c)) {
        p++;
        c = sig(&p);
    }
    while (c == '(') {
        if (!saltar_grupo(&p)) {
            return 0;
        }
        c = sig(&p);
    }
    if (c != '=') {
        return 0;
    }
    *p_igual = p;
    return 1;
}

/* Lo que hay antes del '=' tiene forma de "DO etiqueta variable": D, O, uno o
 * mas digitos, una letra y letras o digitos; sin parentesis. La variable puede
 * llevar digitos (DO 10 I1 = 1,5). */
static int prefijo_do(uint16_t p0, uint16_t p_igual)
{
    uint16_t p = p0;
    char c = sig(&p);

    if (c != 'D') {
        return 0;
    }
    p++;
    if (sig(&p) != 'O') {
        return 0;
    }
    p++;
    c = sig(&p);
    if (!es_digito(c)) {
        return 0;
    }
    while (es_digito(c)) {
        p++;
        c = sig(&p);
    }
    if (!es_letra(c)) {
        return 0;
    }
    while (es_letra(c) || es_digito(c)) {
        p++;
        c = sig(&p);
    }
    return p == p_igual;
}

/* La regla del DO: hay una coma FUERA de parentesis y de cadenas tras el '='
 * en p_igual? (DO10I=1,10: si. DO10I=1.10 y DO10I=F(1,2): no.) */
static int coma_fuera_de_parentesis(uint16_t p_igual)
{
    uint16_t p = (uint16_t)(p_igual + 1);
    int profundidad = 0;
    char c;

    for (;;) {
        c = sig(&p);
        if (c == '\0') {
            return 0;
        }
        if (c == '\'') {
            if (!saltar_cadena(&p)) {
                return 0;
            }
            continue;
        }
        if (c == '(') {
            profundidad++;
        } else if (c == ')') {
            profundidad--;
        } else if (c == ',' && profundidad == 0) {
            return 1;
        }
        p++;
    }
}

/* Si la palabra clave txt esta en p (saltando blancos entre sus letras),
 * devuelve la posicion justo despues de ella; si no, NO_COINCIDE. */
static uint16_t coincide_palabra(uint16_t p, const char *txt)
{
    while (*txt != '\0') {
        if (sig(&p) != *txt) {
            return NO_COINCIDE;
        }
        p++;
        txt++;
    }
    return p;
}

typedef struct {
    const char   *txt;
    TipoSentencia tipo;
} PalabraClave;

/* La tabla crece con los capitulos. El orden no importa: se busca la mas larga. */
static const PalabraClave PALABRAS[] = {
    { "PROGRAM",         SENT_PROGRAM },
    { "END",             SENT_END },
    { "SUBROUTINE",      SENT_SUBROUTINE },
    { "FUNCTION",        SENT_FUNCTION },
    { "CALL",            SENT_CALL },
    { "RETURN",          SENT_RETURN },
    { "INTEGER",         SENT_INTEGER },
    { "REAL",            SENT_REAL },
    { "DOUBLEPRECISION", SENT_DOUBLEPRECISION },
    { "LOGICAL",         SENT_LOGICAL },
    { "CHARACTER",       SENT_CHARACTER },
    { "PRINT",           SENT_PRINT },
    { "CONTINUE",        SENT_CONTINUE },
    { "GOTO",            SENT_GOTO },
    { "STOP",            SENT_STOP },
    { "DO",              SENT_DO },
    { "IF",              SENT_IF_LOGICO },    /* placeholder: clasificar_if decide */
    { "ELSEIF",          SENT_ELSEIF },
    { "ELSE",            SENT_ELSE },
    { "ENDIF",           SENT_ENDIF },
    { "DIMENSION",       SENT_DIMENSION },
    { "EXTERNAL",        SENT_EXTERNAL },
    { "INTRINSIC",       SENT_INTRINSIC },
    { "PARAMETER",       SENT_PARAMETER },
    { "DATA",            SENT_DATA },
    { "READ",            SENT_READ }
};
#define N_PALABRAS (sizeof(PALABRAS) / sizeof(PALABRAS[0]))

/* Lee la etiqueta de un DO (1 a 5 digitos) desde p y la coma opcional que le
 * sigue. Deja en c->etiqueta y c->resto el resultado; devuelve 0 si no hay
 * una etiqueta valida. */
static int leer_etiqueta_do(uint16_t p, Clasificacion *c)
{
    uint32_t valor = 0;
    uint16_t digitos = 0;
    char ch = sig(&p);

    while (es_digito(ch)) {
        if (digitos < 5) {
            valor = valor * 10 + (uint32_t)(ch - '0');
        }
        digitos++;
        p++;
        ch = sig(&p);
    }
    if (digitos == 0 || digitos > 5 || valor == 0) {
        return 0;
    }
    if (ch == ',') {            /* DO 10, I = 1, 10: la coma es opcional */
        p++;
    }
    c->etiqueta = valor;
    c->resto = p;
    return 1;
}

/* Clasifica un IF o un ELSEIF. fin = posicion justo despues de la palabra. */
static void clasificar_if(uint16_t fin, int es_elseif,
                          Clasificacion *c)
{
    uint16_t p = fin;
    uint16_t q;
    char ch = sig(&p);

    c->tipo = SENT_DESCONOCIDA;
    c->resto = fin;
    if (ch != '(' || !saltar_grupo(&p)) {
        return;
    }
    ch = sig(&p);                       /* lo que sigue al ')' */
    if (ch == '\0') {
        return;                             /* IF (cond) sin nada mas */
    }

    /* THEN y fin de sentencia: IF de bloque. Si tras THEN hay mas texto
     * (IF(L)THENY=1), es un IF logico cuya sentencia asigna a THENY. */
    q = coincide_palabra(p, "THEN");
    if (q != NO_COINCIDE && sig(&q) == '\0') {
        c->tipo = es_elseif ? SENT_ELSEIF : SENT_IF_BLOQUE;
        return;
    }
    if (es_elseif) {
        return;                             /* ELSEIF solo existe en forma de bloque */
    }
    c->tipo = es_digito(ch) ? SENT_IF_ARITMETICO : SENT_IF_LOGICO;
}

/* Digitos (con blancos entre ellos) desde p: su valor, o 0 si no hay o pasa de 65535. Deja p tras el ultimo. */
static uint32_t leer_digitos(uint16_t *p)
{
    uint32_t v = 0;
    uint16_t nd = 0;
    char ch = sig(p);

    while (es_digito(ch)) {
        if (v < 100000UL) {
            v = v * 10 + (uint32_t)(ch - '0');
        }
        nd++;
        (*p)++;
        ch = sig(p);
    }
    return (nd > 0 && v < 65536UL) ? v : 0;
}

/* Tras la palabra de un tipo (en p): [*n | *(n)] FUNCTION. Devuelve la posicion tras FUNCTION o NO_COINCIDE.
 * lenpos: el '*' de la longitud (0 = no hay); len: su valor si es una constante entera, 0 si es otra expresion. */
static uint16_t tras_tipo_funcion(uint16_t p, int es_char, uint16_t *lenpos, uint32_t *len)
{
    char ch = sig(&p);

    *lenpos = 0;
    *len = 0;
    if (es_char && ch == '*') {
        *lenpos = p;
        p++;
        ch = sig(&p);
        if (ch == '(') {
            uint16_t q = p;

            if (!saltar_grupo(&p)) {
                return NO_COINCIDE;
            }
            q++;
            *len = leer_digitos(&q);
            if (sig(&q) != ')') {
                *len = 0;                       /* (N+1) y similares: no es una constante simple */
            }
        } else {
            *len = leer_digitos(&p);
        }
    }
    return coincide_palabra(p, "FUNCTION");
}

/* Tras FUNCTION: un nombre y despues '('. */
static int nombre_y_parentesis(uint16_t p)
{
    char ch = sig(&p);

    if (!es_letra(ch)) {
        return 0;
    }
    while (es_letra(ch) || es_digito(ch)) {
        p++;
        ch = sig(&p);
    }
    return ch == '(';
}

void clasificar_sentencia(Clasificacion *c)
{
    uint16_t p0 = lex_saltar_blancos(lexer.pos);
    uint16_t p_igual = 0;
    uint16_t fin;
    uint16_t mejor_fin = NO_COINCIDE;
    uint16_t mejor_len = 0;
    uint16_t len;
    uint16_t i;
    const char *t;
    TipoSentencia mejor_tipo = SENT_DESCONOCIDA;

    c->tipo = SENT_DESCONOCIDA;
    c->resto = p0;
    c->etiqueta = 0;
    c->ftipo = 0;
    c->lenpos = 0;

    if (p0 >= lexer.longitud) {
        c->tipo = SENT_VACIA;
        return;
    }

    /* Regla 1: asignacion (o DO). */
    if (forma_de_asignacion(p0, &p_igual)) {
        if (prefijo_do(p0, p_igual) && coma_fuera_de_parentesis(p_igual)) {
            fin = coincide_palabra(p0, "DO");
            if (leer_etiqueta_do(fin, c)) {
                c->tipo = SENT_DO;
            }
            return;
        }
        c->tipo = SENT_ASIGNACION;          /* resto = p0: el escaner lee todo */
        return;
    }

    /* Regla 2: la palabra clave mas larga que coincida. */
    for (i = 0; i < N_PALABRAS; i++) {
        t = PALABRAS[i].txt;
        len = 0;
        while (t[len] != '\0') {
            len++;
        }
        fin = coincide_palabra(p0, t);
        if (fin != NO_COINCIDE && len > mejor_len) {
            mejor_len = len;
            mejor_fin = fin;
            mejor_tipo = PALABRAS[i].tipo;
        }
    }
    if (mejor_fin == NO_COINCIDE) {
        return;                             /* desconocida */
    }

    /* Regla 3 y casos que necesitan mirar mas alla de la palabra. */
    if (mejor_tipo == SENT_IF_LOGICO) {
        clasificar_if(mejor_fin, 0, c);
    } else if (mejor_tipo == SENT_ELSEIF) {
        clasificar_if(mejor_fin, 1, c);
    } else if (mejor_tipo == SENT_DO) {
        /* DO sin forma de asignacion: solo puede ser "DO 10, I=1,10" */
        if (leer_etiqueta_do(mejor_fin, c)) {
            c->tipo = SENT_DO;
        }
    } else if (mejor_tipo == SENT_FUNCTION) {
        if (nombre_y_parentesis(mejor_fin)) {
            c->tipo = SENT_FUNCTION;
            c->resto = mejor_fin;
        }
    } else {
        c->tipo = mejor_tipo;
        c->resto = mejor_fin;
        if (mejor_tipo == SENT_INTEGER || mejor_tipo == SENT_REAL || mejor_tipo == SENT_DOUBLEPRECISION
            || mejor_tipo == SENT_LOGICAL || mejor_tipo == SENT_CHARACTER) {
            uint16_t lp;
            uint32_t ln;
            uint16_t q = tras_tipo_funcion(mejor_fin, mejor_tipo == SENT_CHARACTER, &lp, &ln);

            if (q != NO_COINCIDE && nombre_y_parentesis(q)) {      /* INTEGER FUNCTION F(X) */
                c->ftipo = (uint8_t)mejor_tipo;
                c->tipo = SENT_FUNCTION;
                c->resto = q;
                c->lenpos = lp;
                c->etiqueta = ln;
            }
        }
    }
}
