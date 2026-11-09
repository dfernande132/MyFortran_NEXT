/* stmt.c - Ver stmt.h.
 *
 * Tecnica: descenso recursivo, una funcion p_xxx por clase de sentencia. Las
 * expresiones las analiza expr.c; aqui solo se comprueba la "armadura" de la
 * sentencia (palabras, comas, parentesis, etiquetas).
 *
 * El token actual es parser.tok (el MISMO que usa expr.c): asi, tras una
 * expresion, el parser de sentencias ve directamente el token que la detuvo
 * (una coma, un ')', el fin). Estado en globales (decision 0006).
 */
#include <string.h>
#include "stmt.h"

Stmt stm;

static Clasificacion clas;

/* Listas en FLUJO (declaradores, expresiones de PRINT, etiquetas del GOTO
 * calculado): no se acumulan en el AST; stmt_elemento() analiza un elemento
 * cada vez, y vuelve a empezar el AST en 'marca' (lo que hay por debajo es la
 * cabecera de la sentencia). Asi el pool solo guarda un elemento a la vez. */
#define LISTA_NO        0       /* no hay lista (o ya se acabo)               */
#define LISTA_PRIMERO   1       /* parser.tok es el primer elemento           */
#define LISTA_SIGUE     2       /* parser.tok es ',' o el final de la lista   */
static uint8_t  estado;
static uint16_t marca;

const char *stmt_mensaje(ErrorStmt e)
{
    switch (e) {
    case STM_OK:                return "no error";
    case STM_DESCONOCIDA:       return "unrecognized or unsupported statement";
    case STM_NOMBRE_ESPERADO:   return "name expected";
    case STM_IGUAL_ESPERADO:    return "'=' expected";
    case STM_COMA_ESPERADA:     return "',' expected";
    case STM_PAR_ABRE_ESPERADO: return "'(' expected";
    case STM_PAR_CIERRA_ESPERADO: return "')' expected";
    case STM_ETIQUETA_ESPERADA: return "statement label expected";
    case STM_ETIQUETA_RANGO:    return "label must be 1 to 99999";
    case STM_THEN_ESPERADO:     return "THEN expected";
    case STM_SOBRA:             return "unexpected text at end of statement";
    case STM_DECLARADOR:        return "invalid declarator";
    case STM_FORMATO:           return "format must be *, a label or a text constant";
    case STM_LONGITUD:          return "invalid length specifier";
    case STM_NO_EN_IF:          return "statement not allowed in logical IF";
    case STM_ARGUMENTO:         return "invalid STOP argument";
    case STM_POOL_LLENO:        return "statement too large";
    case STM_EXPR:              return "error in expression";
    case STM_LEXICO:            return "lexical error";
    case STM_BARRA_ESPERADA:    return "'/' expected";
    case STM_CONSTANTE_ESPERADA: return "constant expected";
    case STM_ENTRADA:           return "only list-directed READ from the keyboard is supported";
    }
    return "unknown error";
}

/* ------------------------------------------------------------------
 * Utilidades
 * ------------------------------------------------------------------ */

/* Anota el PRIMER error. */
static void fallo_en(ErrorStmt e, uint16_t pos)
{
    if (stm.err == STM_OK) {
        stm.err = (uint8_t)e;
        stm.err_pos = pos;
    }
}

static void stm_fallo(ErrorStmt e)
{
    fallo_en(e, parser.tok.pos);
}

/* Traslada un error de expr.c al registro. */
static void sincronizar(void)
{
    if (stm.err != STM_OK || parser.err == EXPR_OK) {
        return;
    }
    if (parser.err == EXPR_LEXICO) {
        stm.err = STM_LEXICO;
        stm.err_lex = (uint8_t)parser.err_lex;
    } else {
        stm.err = STM_EXPR;
        stm.err_expr = (uint8_t)parser.err;
    }
    stm.err_pos = parser.err_pos;
}

/* Siguiente token. */
static void sig(void)
{
    if (stm.err != STM_OK) {
        return;
    }
    lex_siguiente(&parser.tok);
    if (parser.tok.tipo == TOK_ERROR) {
        stm.err_lex = (uint8_t)parser.tok.valor;
        stm_fallo(STM_LEXICO);
    }
}

/* Exige el token t (y lo consume). */
static void esperar(TipoToken t, ErrorStmt e)
{
    if (stm.err != STM_OK) {
        return;
    }
    if (parser.tok.tipo == t) {
        sig();
    } else {
        stm_fallo(e);
    }
}

/* Exige el final de la sentencia. */
static void fin(void)
{
    if (stm.err == STM_OK && parser.tok.tipo != TOK_FIN) {
        stm_fallo(STM_SOBRA);
    }
}

/* Crea un nodo; 0 si hay error o el pool esta lleno. */
static uint16_t stm_crear(NodoTipo tipo, uint16_t a, uint16_t b, uint16_t pos)
{
    uint16_t n;

    if (stm.err != STM_OK) {
        return 0;
    }
    n = ast_nuevo(tipo, a, b, pos);
    if (n == 0) {
        stm_fallo(STM_POOL_LLENO);
    }
    return n;
}

static uint16_t nodo_entero(uint32_t v, uint16_t pos)
{
    return stm_crear(NODO_ENTERO, (uint16_t)v, (uint16_t)(v >> 16), pos);
}

/* Una expresion (parser.tok es su primer token). 0 si hay error. */
static uint16_t expr(void)
{
    uint16_t n;

    if (stm.err != STM_OK) {
        return 0;
    }
    n = expr_parsear();
    sincronizar();
    return stm.err == STM_OK ? n : 0;
}

/* Una etiqueta (1..99999) en *d. */
static void etiqueta(uint32_t *d)
{
    if (stm.err != STM_OK) {
        return;
    }
    if (parser.tok.tipo != TOK_ENTERO) {
        stm_fallo(STM_ETIQUETA_ESPERADA);
    } else if (parser.tok.valor < 1 || parser.tok.valor > 99999UL) {
        stm_fallo(STM_ETIQUETA_RANGO);
    } else {
        *d = parser.tok.valor;
        sig();
    }
}

/* ------------------------------------------------------------------
 * Una funcion por clase de sentencia. Al entrar, parser.tok es el primer
 * token tras la palabra clave.
 * ------------------------------------------------------------------ */

static void p_programa(void)
{
    if (parser.tok.tipo == TOK_NOMBRE) {
        stm.nombre = parser.tok.indice;
        sig();
    } else {
        stm_fallo(STM_NOMBRE_ESPERADO);
    }
    fin();
}

/* SUBROUTINE nombre [( formales )]  y  CALL nombre [( argumentos )]: tras el nombre, modo 1 si hay parentesis (la lista,
 * posiblemente vacia, va en flujo y su ')' y el fin los lee fin_de_lista). */
static void p_unidad_lista(void)
{
    if (parser.tok.tipo != TOK_NOMBRE) {
        stm_fallo(STM_NOMBRE_ESPERADO);
        return;
    }
    stm.nombre = parser.tok.indice;
    stm.resto = parser.tok.pos;         /* para los diagnosticos de la llamada (no es un IF logico) */
    sig();
    if (parser.tok.tipo == TOK_PAR_ABRE) {
        stm.modo = 1;
        sig();
        if (parser.tok.tipo == TOK_PAR_CIERRA) {
            sig();
            fin();
            return;
        }
        estado = LISTA_PRIMERO;
        return;
    }
    fin();
}

/* Longitud de CHARACTER*n o CHARACTER*(expr): devuelve el nodo. */
static uint16_t longitud(void)
{
    uint16_t n = 0;

    if (parser.tok.tipo == TOK_ENTERO) {
        n = nodo_entero(parser.tok.valor, parser.tok.pos);
        sig();
    } else if (parser.tok.tipo == TOK_PAR_ABRE) {
        sig();
        n = expr();
        esperar(TOK_PAR_CIERRA, STM_PAR_CIERRA_ESPERADO);
    } else {
        stm_fallo(STM_LONGITUD);
    }
    return n;
}

/* Un declarador: A, B(10), C(0:9,2) y, en CHARACTER, A*10 o A(5)*(N+1).
 * Se analiza como EXPRESION: B(10) ya es un REF, y A*10 sale como un MUL que
 * aqui se reinterpreta como "nombre con longitud". */
static uint16_t declarador(void)
{
    uint16_t e = expr();
    uint16_t len = 0;
    uint16_t r;

    if (stm.err != STM_OK) {
        return 0;
    }
    if (ast_tipo(e) == NODO_MUL && stm.tipo == SENT_CHARACTER) {
        r = ast_b(e);
        if (ast_tipo(r) == NODO_ENTERO) {
            len = r;
        } else if (ast_tipo(r) == NODO_PAREN) {
            len = ast_a(r);
        } else {
            fallo_en(STM_LONGITUD, ast_pos(r));
            return 0;
        }
        e = ast_a(e);
    }
    if (ast_tipo(e) != NODO_NOMBRE && ast_tipo(e) != NODO_REF) {
        fallo_en(STM_DECLARADOR, ast_pos(e));
        return 0;
    }
    return stm_crear(NODO_DECL, e, len, ast_pos(e));
}

static void p_declaracion(void)
{
    if (parser.tok.tipo == TOK_POR) {
        if (stm.tipo != SENT_CHARACTER) {
            stm_fallo(STM_LONGITUD);        /* REAL*8 y similares: extension no soportada */
            return;
        }
        lexer.solo_entero = 1;          /* CHARACTER*3 E: la longitud es "3", no el real "3E" */
        sig();
        lexer.solo_entero = 0;
        stm.c = longitud();
        if (parser.tok.tipo == TOK_COMA) {
            sig();
        }
    }
    estado = LISTA_PRIMERO;             /* los declaradores, con stmt_elemento() */
}

/* El clasificador garantiza "nombre [(...)]... =", asi que el destino ya es un
 * NOMBRE, un REF o un SUBCAD. */
static void p_asignacion(void)
{
    stm.a = expr();
    esperar(TOK_IGUAL, STM_IGUAL_ESPERADO);
    stm.b = expr();
    fin();
}

static void p_print(void)
{
    switch (parser.tok.tipo) {
    case TOK_POR:
        stm.modo = 0;
        sig();
        break;
    case TOK_ENTERO:
        stm.modo = 1;
        etiqueta(&stm.l1);
        break;
    case TOK_CADENA:
        stm.modo = 2;
        stm.a = stm_crear(NODO_CADENA, parser.tok.indice, 0, parser.tok.pos);
        sig();
        break;
    default:
        stm_fallo(STM_FORMATO);
        return;
    }
    if (stm.err == STM_OK && parser.tok.tipo != TOK_FIN) {
        esperar(TOK_COMA, STM_COMA_ESPERADA);
        estado = LISTA_PRIMERO;         /* las expresiones, con stmt_elemento() */
        return;
    }
    fin();
}

/* READ * , lista   |   READ ( unidad , * ) [lista]   con unidad = * o 5 (el teclado): solo entrada dirigida por lista. */
static void p_read(void)
{
    stm.modo = 0;
    if (parser.tok.tipo == TOK_POR) {
        sig();
        if (parser.tok.tipo == TOK_FIN) {
            return;                             /* READ *: salta un registro */
        }
        esperar(TOK_COMA, STM_COMA_ESPERADA);
        if (stm.err == STM_OK) {
            estado = LISTA_PRIMERO;
        }
        return;
    }
    if (parser.tok.tipo != TOK_PAR_ABRE) {
        stm_fallo(STM_ENTRADA);                 /* READ etiqueta, lista y similares */
        return;
    }
    sig();
    if (parser.tok.tipo == TOK_POR || (parser.tok.tipo == TOK_ENTERO && parser.tok.valor == 5)) {
        sig();
    } else {
        stm_fallo(STM_ENTRADA);
        return;
    }
    esperar(TOK_COMA, STM_COMA_ESPERADA);
    if (parser.tok.tipo != TOK_POR) {
        stm_fallo(STM_ENTRADA);
        return;
    }
    sig();
    esperar(TOK_PAR_CIERRA, STM_PAR_CIERRA_ESPERADO);
    if (stm.err == STM_OK && parser.tok.tipo == TOK_COMA) {
        sig();                                  /* READ (*,*), lista: la coma sobra pero se admite como en PRINT */
    }
    if (stm.err == STM_OK && parser.tok.tipo != TOK_FIN) {
        estado = LISTA_PRIMERO;
        return;
    }
    fin();
}

static void p_stop(void)
{
    switch (parser.tok.tipo) {
    case TOK_FIN:
        break;
    case TOK_ENTERO:
        stm.a = nodo_entero(parser.tok.valor, parser.tok.pos);
        sig();
        break;
    case TOK_CADENA:
        stm.a = stm_crear(NODO_CADENA, parser.tok.indice, 0, parser.tok.pos);
        sig();
        break;
    default:
        stm_fallo(STM_ARGUMENTO);
        return;
    }
    fin();
}

static void p_goto(void)
{
    if (parser.tok.tipo == TOK_ENTERO) {
        etiqueta(&stm.l1);              /* GOTO etiqueta */
    } else if (parser.tok.tipo == TOK_PAR_ABRE) {
        stm.modo = 1;                   /* GOTO (l1, l2, ...) [,] expr */
        sig();
        estado = LISTA_PRIMERO;         /* las etiquetas, con stmt_elemento(); al
                                           acabar la lista se lee ')' y el selector */
        return;
    } else {
        stm_fallo(STM_ETIQUETA_ESPERADA);   /* GOTO asignado: no soportado */
    }
    fin();
}

static void p_do(void)
{
    stm.l1 = clas.etiqueta;
    if (parser.tok.tipo == TOK_NOMBRE) {
        stm.nombre = parser.tok.indice;
        sig();
    } else {
        stm_fallo(STM_NOMBRE_ESPERADO);
    }
    esperar(TOK_IGUAL, STM_IGUAL_ESPERADO);
    stm.a = expr();
    esperar(TOK_COMA, STM_COMA_ESPERADA);
    stm.b = expr();
    if (stm.err == STM_OK && parser.tok.tipo == TOK_COMA) {
        sig();
        stm.c = expr();
    }
    fin();
}

/* THEN al final (IF de bloque y ELSE IF). */
static void then_y_fin(void)
{
    if (stm.err == STM_OK
        && !(parser.tok.tipo == TOK_NOMBRE
             && strcmp(tabla_texto(parser.tok.indice), "THEN") == 0)) {
        stm_fallo(STM_THEN_ESPERADO);
    }
    sig();
    fin();
}

/* IF, ELSEIF: "( condicion )" y luego, segun la clase, lo que siga. */
static void p_if(void)
{
    esperar(TOK_PAR_ABRE, STM_PAR_ABRE_ESPERADO);
    stm.a = expr();
    if (stm.err == STM_OK && parser.tok.tipo != TOK_PAR_CIERRA) {
        stm_fallo(STM_PAR_CIERRA_ESPERADO);
    }
    if (stm.err != STM_OK) {
        return;
    }
    if (stm.tipo == SENT_IF_LOGICO) {
        stm.resto = lexer.pos;          /* la sentencia interior empieza aqui */
        return;
    }
    sig();
    if (stm.tipo == SENT_IF_ARITMETICO) {
        etiqueta(&stm.l1);
        esperar(TOK_COMA, STM_COMA_ESPERADA);
        etiqueta(&stm.l2);
        esperar(TOK_COMA, STM_COMA_ESPERADA);
        etiqueta(&stm.l3);
        fin();
    } else {
        then_y_fin();
    }
}

/* ------------------------------------------------------------------
 * Listas en flujo
 * ------------------------------------------------------------------ */

/* Un elemento de la lista de la sentencia en curso; parser.tok es su primer token. */
static uint16_t elemento(void)
{
    uint32_t v = 0;
    uint16_t pos = parser.tok.pos;

    switch (stm.tipo) {
    case SENT_PRINT: case SENT_CALL: case SENT_READ:
        return expr();
    case SENT_SUBROUTINE: case SENT_FUNCTION:
        if (parser.tok.tipo != TOK_NOMBRE) {
            stm_fallo(STM_NOMBRE_ESPERADO);
            return 0;
        }
        v = parser.tok.indice;
        sig();
        return stm_crear(NODO_NOMBRE, (uint16_t)v, 0, pos);
    case SENT_GOTO:
        etiqueta(&v);
        return nodo_entero(v, pos);
    case SENT_PARAMETER: {              /* nombre = expresion */
        uint16_t nom, val;

        if (parser.tok.tipo != TOK_NOMBRE) {
            stm_fallo(STM_NOMBRE_ESPERADO);
            return 0;
        }
        nom = stm_crear(NODO_NOMBRE, (uint16_t)parser.tok.indice, 0, pos);
        sig();
        esperar(TOK_IGUAL, STM_IGUAL_ESPERADO);
        val = expr();
        if (stm.err != STM_OK) {
            return 0;
        }
        return stm_crear(NODO_DECL, nom, val, pos);
    }
    default:
        return declarador();
    }
}

/* Se acabo la lista: lo que cierra la sentencia. */
static void fin_de_lista(void)
{
    if (stm.tipo == SENT_SUBROUTINE || stm.tipo == SENT_CALL || stm.tipo == SENT_FUNCTION) {
        esperar(TOK_PAR_CIERRA, STM_PAR_CIERRA_ESPERADO);
    } else if (stm.tipo == SENT_PARAMETER) {
        esperar(TOK_PAR_CIERRA, STM_PAR_CIERRA_ESPERADO);
        fin();
        return;
    } else if (stm.tipo == SENT_GOTO) {
        esperar(TOK_PAR_CIERRA, STM_PAR_CIERRA_ESPERADO);
        if (parser.tok.tipo == TOK_COMA) {
            sig();
        }
        stm.b = expr();
    }
    fin();
}

/* DATA: fase del analisis (0 = va un nombre, 1 = tras un nombre, 2 = tras un valor, 3 = va un valor). */
static uint8_t dfase;

/* [signo] constante de DATA. */
static uint16_t constante_dato(void)
{
    uint16_t pos = parser.tok.pos;
    uint16_t n;
    uint8_t neg = 0;

    if (parser.tok.tipo == TOK_MAS || parser.tok.tipo == TOK_MENOS) {
        neg = parser.tok.tipo == TOK_MENOS;
        sig();
    }
    switch (parser.tok.tipo) {
    case TOK_ENTERO:
        n = nodo_entero(parser.tok.valor, parser.tok.pos);
        break;
    case TOK_REAL:
        n = stm_crear(NODO_REAL, (uint16_t)parser.tok.indice, 0, parser.tok.pos);
        break;
    case TOK_CADENA:
        n = stm_crear(NODO_CADENA, (uint16_t)parser.tok.indice, 0, parser.tok.pos);
        break;
    case TOK_LOGICO:
        n = stm_crear(NODO_LOGICO, (uint16_t)parser.tok.valor, 0, parser.tok.pos);
        break;
    case TOK_NOMBRE:
        n = stm_crear(NODO_NOMBRE, (uint16_t)parser.tok.indice, 0, parser.tok.pos);
        break;
    default:
        stm_fallo(STM_CONSTANTE_ESPERADA);
        return 0;
    }
    sig();
    if (neg && stm.err == STM_OK) {
        n = stm_crear(NODO_NEG, n, 0, pos);
    }
    return n;
}

uint8_t stmt_dato(void)
{
    if (stm.err != STM_OK || estado == LISTA_NO) {
        return 0;
    }
    ast_recortar(marca);
    if (dfase == 1 || dfase == 2) {
        if (parser.tok.tipo == TOK_COMA) {
            sig();
            dfase = dfase == 1 ? 0 : 3;
        } else if (parser.tok.tipo == TOK_DIV && dfase == 1) {
            sig();
            dfase = 3;
        } else if (parser.tok.tipo == TOK_DIV) {                /* fin de los valores del grupo */
            sig();
            if (parser.tok.tipo == TOK_COMA) {
                sig();
            }
            dfase = 0;
            if (parser.tok.tipo == TOK_FIN) {
                estado = LISTA_NO;
            }
            stm.modo = 3;
            stm.e = 0;
            return stm.err == STM_OK;
        } else {
            stm_fallo(STM_BARRA_ESPERADA);
            return 0;
        }
    }
    if (dfase == 0) {
        if (parser.tok.tipo != TOK_NOMBRE) {
            stm_fallo(STM_NOMBRE_ESPERADO);
            return 0;
        }
        stm.e = expr_referencia();
        sincronizar();
        stm.modo = 1;
        dfase = 1;
        return stm.err == STM_OK;
    }
    stm.c = 0;
    stm.e = constante_dato();
    if (stm.err == STM_OK && parser.tok.tipo == TOK_POR && (ast_tipo(stm.e) == NODO_ENTERO || ast_tipo(stm.e) == NODO_NOMBRE)) {
        stm.c = stm.e;                                          /* r*c */
        sig();
        stm.e = constante_dato();
    }
    stm.modo = 2;
    dfase = 2;
    return stm.err == STM_OK;
}

uint8_t stmt_elemento(void)
{
    if (stm.err != STM_OK || estado == LISTA_NO) {
        return 0;
    }
    if (estado == LISTA_SIGUE) {
        if (parser.tok.tipo != TOK_COMA) {
            estado = LISTA_NO;
            fin_de_lista();
            return 0;
        }
        sig();
    }
    estado = LISTA_SIGUE;
    ast_recortar(marca);             /* el elemento anterior ya no hace falta */
    stm.e = elemento();
    return stm.err == STM_OK;
}

/* ------------------------------------------------------------------
 * Punto de entrada comun
 * ------------------------------------------------------------------ */

static void analizar(uint8_t interna, uint32_t etiq, uint16_t desde)
{
    memset(&stm, 0, sizeof(stm));
    estado = LISTA_NO;
    stm.interna = interna;
    stm.etiqueta = etiq;
    lexer.pos = desde;
    clasificar_sentencia(&clas);
    stm.tipo = (uint8_t)clas.tipo;

    if (clas.tipo == SENT_DESCONOCIDA) {
        fallo_en(STM_DESCONOCIDA, lex_saltar_blancos(desde));
        return;
    }
    if (interna) {
        switch (clas.tipo) {
        case SENT_ASIGNACION: case SENT_CONTINUE: case SENT_GOTO:
        case SENT_STOP: case SENT_PRINT: case SENT_IF_ARITMETICO: case SENT_CALL: case SENT_RETURN: case SENT_READ:
            break;
        default:
            fallo_en(STM_NO_EN_IF, lex_saltar_blancos(desde));
            return;
        }
    }

    lexer.pos = clas.resto;
    parser_iniciar();
    sincronizar();

    switch (clas.tipo) {
    case SENT_PROGRAM:          p_programa(); break;
    case SENT_SUBROUTINE: case SENT_CALL: case SENT_FUNCTION:
                                stm.ftipo = clas.ftipo;
                                stm.flen = (uint16_t)clas.etiqueta;     /* CHARACTER*n FUNCTION: n (lo lee el clasificador) */
                                if (clas.lenpos != 0 && clas.etiqueta == 0) {
                                    fallo_en(STM_LONGITUD, clas.lenpos);   /* solo se admite una constante entera */
                                    break;
                                }
                                p_unidad_lista(); break;
    case SENT_INTEGER: case SENT_REAL: case SENT_DOUBLEPRECISION:
    case SENT_LOGICAL: case SENT_CHARACTER:
    case SENT_DIMENSION: case SENT_EXTERNAL: case SENT_INTRINSIC:
                                parser.estrella = 1;            /* A(*) y CHARACTER*(*) (S3c) */
                                p_declaracion(); break;
    case SENT_DATA:             dfase = 0;
                                estado = LISTA_PRIMERO;         /* con stmt_dato() */
                                break;
    case SENT_PARAMETER:        esperar(TOK_PAR_ABRE, STM_PAR_ABRE_ESPERADO);
                                estado = LISTA_PRIMERO;         /* nombre = expresion {, ...} ) */
                                break;
    case SENT_ASIGNACION:       p_asignacion(); break;
    case SENT_PRINT:            p_print(); break;
    case SENT_READ:             p_read(); break;
    case SENT_STOP:             p_stop(); break;
    case SENT_GOTO:             p_goto(); break;
    case SENT_DO:               p_do(); break;
    case SENT_IF_BLOQUE: case SENT_IF_ARITMETICO: case SENT_IF_LOGICO:
    case SENT_ELSEIF:           p_if(); break;
    case SENT_END: case SENT_CONTINUE: case SENT_ELSE: case SENT_ENDIF: case SENT_RETURN:
    case SENT_VACIA:            fin(); break;
    default:                    stm_fallo(STM_DESCONOCIDA); break;
    }
    marca = ast_usados();             /* por encima de aqui, un elemento cada vez */
}

void stmt_analizar(void)
{
    lex_iniciar();
    ast_iniciar();
    analizar(0, sentencia.etiqueta, 0);
}

void stmt_analizar_interna(void)
{
    analizar(1, 0, stm.resto);
}
