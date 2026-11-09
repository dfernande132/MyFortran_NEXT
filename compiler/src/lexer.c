/* lexer.c - Ver lexer.h. */
#include "lexer.h"
#include "banco.h"

Lexer lexer;    /* el escaner unico; ver lexer.h */
TablaCadenas cadenas;   /* la tabla de cadenas unica; ver lexer.h */

/* ==================================================================
 * Tabla de cadenas
 * ================================================================== */

/* Buffer comun de textos largos y de trabajo del lexer; ranuras de nombres; ventana de lectura de la tabla. */
static char     tabla_buf[TABLA_BUF_TAM];
#define TABLA_CORTO    12
#define TABLA_RANURAS  4
static char     corto[TABLA_RANURAS][TABLA_CORTO];
static uint8_t  ranura;
static uint8_t  ventana[32];
static uint16_t vent_ini;
static uint8_t  vent_ok;

void tabla_iniciar(void)
{
    uint8_t cero = 0;

    banco_escribir(TABLA_PAGINA, TABLA_OFF, &cero, 1);    /* offset 0 = cadena vacia: "no hay texto" */
    cadenas.usado = 1;
    vent_ok = 0;
    ranura = 0;
}

/* Byte `off` de la tabla, con una ventana de 32 bytes para no copiar de la pagina uno a uno. */
static uint8_t tabla_byte(uint16_t off)
{
    if (!vent_ok || (uint16_t)(off - vent_ini) >= sizeof(ventana)) {
        banco_leer(TABLA_PAGINA, (uint16_t)(TABLA_OFF + off), ventana, sizeof(ventana));
        vent_ini = off;
        vent_ok = 1;
    }
    return ventana[off - vent_ini];
}

uint16_t tabla_internar(const char *s, uint16_t len)
{
    uint16_t off;
    uint16_t i;
    uint8_t cero = 0;

    if (len == 0) {
        return 0;
    }

    /* 1. Buscar si ya esta. Cada entrada termina en '\0'. */
    off = 1;
    while (off < cadenas.usado) {
        i = 0;
        while (i < len && tabla_byte((uint16_t)(off + i)) == (uint8_t)s[i]) {
            i++;
        }
        /* Coincide si hemos comparado len caracteres Y la entrada acaba ahi
         * (si no, la entrada es mas larga: "AB" no es "ABC"). */
        if (i == len && tabla_byte((uint16_t)(off + i)) == 0) {
            return off;
        }
        while (tabla_byte(off) != 0) {           /* saltar al final de esta entrada */
            off++;
        }
        off++;                               /* y pasar al principio de la siguiente */
    }

    /* 2. No esta: anadirla, si cabe (len + 1 por el '\0'). */
    if (cadenas.usado + len + 1 > TABLA_TAM) {
        return TABLA_LLENA;
    }
    off = cadenas.usado;
    banco_escribir(TABLA_PAGINA, (uint16_t)(TABLA_OFF + off), s, len);     /* s puede ser tabla_buf */
    banco_escribir(TABLA_PAGINA, (uint16_t)(TABLA_OFF + off + len), &cero, 1);
    vent_ok = 0;
    cadenas.usado = (uint16_t)(cadenas.usado + len + 1);
    return off;
}

const char *tabla_texto(uint16_t offset)
{
    char *d = corto[ranura];
    uint8_t i;

    banco_leer(TABLA_PAGINA, (uint16_t)(TABLA_OFF + offset), d, TABLA_CORTO);
    for (i = 0; i < TABLA_CORTO; i++) {
        if (d[i] == '\0') {
            ranura = (uint8_t)((ranura + 1) & (TABLA_RANURAS - 1));
            return d;
        }
    }
    banco_leer(TABLA_PAGINA, (uint16_t)(TABLA_OFF + offset), tabla_buf, TABLA_BUF_TAM);
    tabla_buf[TABLA_BUF_TAM - 1] = '\0';
    return tabla_buf;
}

/* ==================================================================
 * Mensajes de error
 * ================================================================== */

const char *lex_mensaje(ErrorLex e)
{
    switch (e) {
    case LEX_CARACTER_ILEGAL:      return "illegal character";
    case LEX_CADENA_SIN_CERRAR:    return "unterminated character constant";
    case LEX_NOMBRE_LARGO:         return "name is longer than 8 characters";
    case LEX_ENTERO_GRANDE:        return "integer constant is too large";
    case LEX_REAL_MAL_FORMADO:     return "malformed real constant";
    case LEX_OPERADOR_DESCONOCIDO: return "unknown dotted operator";
    case LEX_TABLA_LLENA:          return "string table is full";
    }
    return "lexical error";
}

/* ==================================================================
 * Lectura que salta blancos: mirar() y avanzar()
 *
 * El texto de la sentencia conserva sus blancos. En F77 no significan nada
 * (salvo dentro de constantes de texto), asi que TODO lo que no sea leer una
 * cadena pasa por estas dos funciones, que los ignoran:
 *
 *   mirar(k)    el k-esimo caracter NO BLANCO desde pos (k = 0 es el
 *                   siguiente), en mayusculas, o '\0' si ya no hay.
 *   avanzar(n)  consume n caracteres no blancos (y los blancos entre ellos).
 *
 * Cuesta O(distancia) por llamada en vez de O(1), pero las sentencias son
 * cortas. (No medido en el Z80.)
 * ================================================================== */

static int es_letra(char c)
{
    return c >= 'A' && c <= 'Z';
}

static int es_digito(char c)
{
    return c >= '0' && c <= '9';
}

static int es_blanco(char c)
{
    return c == ' ' || c == '\t';
}

uint16_t lex_saltar_blancos(uint16_t p)
{
    while (p < lexer.longitud && es_blanco(sent_car(p))) {
        p++;
    }
    return (p < lexer.longitud) ? p : lexer.longitud;
}

char lex_car(uint16_t p)
{
    char c;

    if (p >= lexer.longitud) {
        return '\0';
    }
    c = sent_car(p);
    if (c >= 'a' && c <= 'z') {
        c = (char)(c - 'a' + 'A');
    }
    return c;
}

/* Posicion bruta del k-esimo caracter no blanco desde pos, o longitud si no hay. */
static uint16_t bruto(uint16_t k)
{
    uint16_t p = lexer.pos;

    for (;;) {
        while (p < lexer.longitud && es_blanco(sent_car(p))) {
            p++;
        }
        if (p >= lexer.longitud) {
            return lexer.longitud;
        }
        if (k == 0) {
            return p;
        }
        k--;
        p++;
    }
}

static char mirar(uint16_t k)
{
    return lex_car(bruto(k));
}

static void avanzar(uint16_t n)
{
    uint16_t p;

    if (n == 0) {
        return;
    }
    p = bruto((uint16_t)(n - 1));
    lexer.pos = (p < lexer.longitud) ? (uint16_t)(p + 1) : lexer.longitud;
}

/* BUFFER DE TRABAJO = la COLA LIBRE de la tabla de cadenas. Antes habia un
 * array propio de 1.321 bytes (Lexer.tmp); pero ningun texto puede ser mayor
 * que lo que queda libre en la tabla (acabaria en TABLA_LLENA), asi que se
 * escribe directamente donde iria el texto y tabla_internar lo confirma
 * (usado += len + 1) o lo descarta (si ya estaba, no se toca usado).
 * Los nombres (<= 8 caracteres) y los operadores con punto usan un buffer
 * local en la pila, para que un nombre repetido se encuentre aunque la tabla
 * este casi llena. */
#define COLA()        (tabla_buf)
#define COLA_LIBRE()  ((uint16_t)((TABLA_TAM - cadenas.usado) < TABLA_BUF_TAM ? (TABLA_TAM - cadenas.usado) : TABLA_BUF_TAM))

/* Copia en dst los n primeros caracteres no blancos desde pos + desde (ya en
 * mayusculas y sin blancos) y deja el '\0'. Hay que llamarla ANTES de avanzar. */
static void copiar_lexema(char *dst, uint16_t desde, uint16_t n)
{
    uint16_t i;

    for (i = 0; i < n; i++) {
        dst[i] = mirar((uint16_t)(desde + i));
    }
    dst[n] = '\0';
}

static void fallar(Token *t, ErrorLex e)
{
    t->tipo = TOK_ERROR;
    t->valor = (uint32_t)e;
}

/* Guarda un texto en la tabla y rellena el token, o falla si no cabe. */
static void guardar_texto(Token *t, TipoToken tipo,
                          const char *txt, uint16_t len)
{
    t->indice = tabla_internar(txt, len);
    if (t->indice == TABLA_LLENA) {
        fallar(t, LEX_TABLA_LLENA);
    } else {
        t->tipo = tipo;
    }
}

/* ==================================================================
 * Nombres
 * ================================================================== */

static void leer_nombre(Token *t)
{
    char nombre[NOMBRE_MAX + 1];
    uint16_t n = 0;

    /* Letra seguida de letras o digitos (el '_' no es de F77). Los blancos
     * intermedios no cuentan: "A B" es el nombre AB. */
    while (es_letra(mirar(n)) || es_digito(mirar(n))) {
        n++;
    }

    if (n > NOMBRE_MAX) {
        avanzar(n);
        fallar(t, LEX_NOMBRE_LARGO);
        return;
    }
    copiar_lexema(nombre, 0, n);
    avanzar(n);
    guardar_texto(t, TOK_NOMBRE, nombre, n);
}

/* ==================================================================
 * Numeros y operadores con punto
 * ================================================================== */

/* En la posicion n (no blancos) hay un '.'. Devuelve 1 si ese punto abre un
 * operador con punto (.EQ., .AND., .TRUE....): un punto, UNA O MAS LETRAS y
 * otro punto.
 *
 * Es la regla de "1.EQ.2": hay que decidir si "1." es un real o si el punto
 * pertenece a ".EQ.". Y "1.E3" sigue siendo un real porque tras la E hay un
 * digito y no un punto. */
static int abre_operador_punto(uint16_t n)
{
    uint16_t k = (uint16_t)(n + 1);

    while (es_letra(mirar(k))) {
        k++;
    }
    return k > (uint16_t)(n + 1) && mirar(k) == '.';
}

/* Constante numerica: entera o real. Se entra con un digito, o con un '.'
 * seguido de digito (".5"). */
static void leer_numero(Token *t)
{
    uint16_t n = 0;             /* caracteres no blancos consumidos hasta ahora */
    uint16_t k;
    uint32_t valor = 0;
    uint32_t d;
    int desborda = 0;
    int es_real = 0;
    int es_doble = 0;
    char c;

    /* a) Parte entera. El valor se acumula en 32 bits; el maximo es
     *    2147483647 = 214748364 * 10 + 7. Se comprueba ANTES de multiplicar,
     *    sin dividir (la division de 32 bits es cara en el Z80). */
    while (es_digito(mirar(n))) {
        d = (uint32_t)(mirar(n) - '0');
        if (valor > 214748364UL || (valor == 214748364UL && d > 7)) {
            desborda = 1;
        } else {
            valor = valor * 10 + d;
        }
        n++;
    }

    /* b) Parte decimal: un '.' que NO abre un operador con punto. */
    if (!lexer.solo_entero && mirar(n) == '.' && !abre_operador_punto(n)) {
        es_real = 1;
        n++;
        while (es_digito(mirar(n))) {
            n++;
        }
    }

    /* c) Exponente: E o D, signo opcional, y al menos un digito. */
    c = mirar(n);
    if (!lexer.solo_entero && (c == 'E' || c == 'D')) {
        k = (uint16_t)(n + 1);
        if (mirar(k) == '+' || mirar(k) == '-') {
            k++;
        }
        if (!es_digito(mirar(k))) {
            avanzar((uint16_t)(n + 1));           /* consumir hasta la E */
            fallar(t, LEX_REAL_MAL_FORMADO);
            return;
        }
        while (es_digito(mirar(k))) {
            k++;
        }
        n = k;
        es_real = 1;
        es_doble = (c == 'D');
    }

    if (!es_real) {
        avanzar(n);
        if (desborda) {
            fallar(t, LEX_ENTERO_GRANDE);
        } else {
            t->tipo = TOK_ENTERO;
            t->valor = valor;
        }
        return;
    }
    /* Los reales NO se convierten a numero: se guarda su texto (sin blancos). */
    if ((uint16_t)(n + 1) > COLA_LIBRE()) {      /* no cabe en la tabla */
        avanzar(n);
        fallar(t, LEX_TABLA_LLENA);
        return;
    }
    copiar_lexema(COLA(), 0, n);
    avanzar(n);
    guardar_texto(t, es_doble ? TOK_DOBLE : TOK_REAL, COLA(), n);
}

typedef struct {
    const char *txt;
    TipoToken   tipo;
    uint8_t     valor;      /* solo para .TRUE. (1) y .FALSE. (0) */
} OperadorPunto;

static const OperadorPunto OPERADORES_PUNTO[] = {
    { "EQ",    TOK_EQ,     0 }, { "NE",  TOK_NE,  0 }, { "LT",  TOK_LT,  0 },
    { "LE",    TOK_LE,     0 }, { "GT",  TOK_GT,  0 }, { "GE",  TOK_GE,  0 },
    { "AND",   TOK_AND,    0 }, { "OR",  TOK_OR,  0 }, { "NOT", TOK_NOT, 0 },
    { "EQV",   TOK_EQV,    0 }, { "NEQV", TOK_NEQV, 0 },
    { "TRUE",  TOK_LOGICO, 1 }, { "FALSE", TOK_LOGICO, 0 }
};
#define N_OPERADORES_PUNTO (sizeof(OPERADORES_PUNTO) / sizeof(OPERADORES_PUNTO[0]))

/* a (terminada en '\0') es igual a los len caracteres de b? */
static int coincide(const char *a, const char *b, uint16_t len)
{
    uint16_t i;

    for (i = 0; i < len; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return a[len] == '\0';
}

/* Se entra con un '.' que NO va seguido de un digito. */
static void leer_operador_punto(Token *t)
{
    char txt[8];                    /* el operador mas largo, FALSE, tiene 5 */
    uint16_t n = 1;
    uint16_t len;
    uint16_t i;

    while (es_letra(mirar(n))) {
        n++;
    }
    if (n == 1 || mirar(n) != '.') {
        avanzar(1);                 /* un '.' suelto: ilegal */
        fallar(t, LEX_CARACTER_ILEGAL);
        return;
    }
    len = (uint16_t)(n - 1);            /* las letras entre los dos puntos */
    if (len < sizeof(txt)) {
        copiar_lexema(txt, 1, len);
    }
    avanzar((uint16_t)(n + 1));     /* los dos puntos incluidos */
    if (len >= sizeof(txt)) {       /* demasiado largo para ser un operador */
        fallar(t, LEX_OPERADOR_DESCONOCIDO);
        return;
    }

    for (i = 0; i < N_OPERADORES_PUNTO; i++) {
        if (coincide(OPERADORES_PUNTO[i].txt, txt, len)) {
            t->tipo = OPERADORES_PUNTO[i].tipo;
            t->valor = OPERADORES_PUNTO[i].valor;
            return;
        }
    }
    fallar(t, LEX_OPERADOR_DESCONOCIDO);
}

/* ==================================================================
 * Constantes de texto
 *
 * Aqui NO se saltan blancos ni se pasa a mayusculas: dentro de una cadena
 * cuentan tal cual. Se lee en bruto desde pos, que apunta a la comilla.
 * ================================================================== */

static void leer_cadena(Token *t)
{
    uint16_t n = 1;             /* se salta la comilla de apertura */
    uint16_t salida = 0;
    uint16_t cabe = COLA_LIBRE();   /* bytes libres en la tabla (con el '\0') */
    int cerrada = 0;
    int lleno = 0;
    char c;

    while ((uint16_t)(lexer.pos + n) < lexer.longitud) {
        c = sent_car((uint16_t)(lexer.pos + n));
        if (c == '\'') {
            if ((uint16_t)(lexer.pos + n + 1) < lexer.longitud &&
                sent_car((uint16_t)(lexer.pos + n + 1)) == '\'') {
                c = '\'';                      /* '' dentro de la cadena = una comilla */
                if ((uint16_t)(salida + 1) < cabe) {
                    COLA()[salida++] = c;
                } else {
                    lleno = 1;
                }
                n = (uint16_t)(n + 2);
                continue;
            }
            n++;                                 /* comilla de cierre */
            cerrada = 1;
            break;
        }
        if ((uint16_t)(salida + 1) < cabe) {
            COLA()[salida++] = c;
        } else {
            lleno = 1;                          /* se sigue leyendo para consumirla entera */
        }
        n++;
    }
    lexer.pos = (uint16_t)(lexer.pos + n);

    if (!cerrada) {
        fallar(t, LEX_CADENA_SIN_CERRAR);
        return;
    }
    if (lleno) {
        fallar(t, LEX_TABLA_LLENA);
        return;
    }
    COLA()[salida] = '\0';
    guardar_texto(t, TOK_CADENA, COLA(), salida);
}

/* ==================================================================
 * Simbolos y punto de entrada
 * ================================================================== */

static void leer_simbolo(Token *t)
{
    char c = mirar(0);
    uint16_t avance = 1;

    switch (c) {
    case '+': t->tipo = TOK_MAS;        break;
    case '-': t->tipo = TOK_MENOS;      break;
    case '(': t->tipo = TOK_PAR_ABRE;   break;
    case ')': t->tipo = TOK_PAR_CIERRA; break;
    case ',': t->tipo = TOK_COMA;       break;
    case '=': t->tipo = TOK_IGUAL;      break;
    case ':': t->tipo = TOK_DOS_PUNTOS; break;
    case '*':
        if (mirar(1) == '*') {
            t->tipo = TOK_POTENCIA;
            avance = 2;
        } else {
            t->tipo = TOK_POR;
        }
        break;
    case '/':
        if (mirar(1) == '/') {
            t->tipo = TOK_CONCAT;
            avance = 2;
        } else {
            t->tipo = TOK_DIV;
        }
        break;
    default:
        fallar(t, LEX_CARACTER_ILEGAL);
        break;
    }
    avanzar(avance);
}

void lex_iniciar(void)
{
    lexer.longitud = sentencia.longitud;
    lexer.pos = 0;
    lexer.solo_entero = 0;
}

void lex_siguiente(Token *t)
{
    char c;

    t->valor = 0;
    t->indice = 0;

    /* Saltar los blancos hasta el primer caracter del token. A partir de aqui
     * lexer.pos apunta a el, y Token.pos es su posicion bruta en el texto. */
    lexer.pos = bruto(0);
    t->pos = lexer.pos;

    if (lexer.pos >= lexer.longitud) {
        t->tipo = TOK_FIN;
        return;
    }

    c = mirar(0);
    if (es_letra(c)) {
        leer_nombre(t);
    } else if (es_digito(c)) {
        leer_numero(t);
    } else if (c == '.') {
        if (es_digito(mirar(1))) {
            leer_numero(t);          /* ".5" */
        } else {
            leer_operador_punto(t);  /* ".EQ.", ".TRUE.", o un '.' suelto */
        }
    } else if (c == '\'') {
        leer_cadena(t);
    } else {
        leer_simbolo(t);
    }
}
