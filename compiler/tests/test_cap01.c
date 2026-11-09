/* test_cap01.c - Pruebas del capitulo 1 (lector de fuente y lexer).
 * Se va ampliando paso a paso. Cada COMPROBAR dice que esperamos.
 */
#include <stdio.h>
#include <string.h>
#include "source.h"
#include "texto.h"
#include "mf_api.h"
#include "lexer.h"
#include "clasif.h"

static int pruebas = 0;
static int fallos = 0;

/* Si la condicion es falsa, cuenta un fallo e imprime la linea del test. */
#define COMPROBAR(cond)                                              \
    do {                                                             \
        pruebas++;                                                   \
        if (!(cond)) {                                               \
            fallos++;                                                \
            printf("FALLO (linea %d): %s\n", __LINE__, #cond);       \
        }                                                            \
    } while (0)

static void test_clasificar_linea(void)
{
    /* Ignoradas: vacias, en blanco y comentarios */
    COMPROBAR(clasificar_linea("") == LINEA_IGNORADA);
    COMPROBAR(clasificar_linea("      ") == LINEA_IGNORADA);
    COMPROBAR(clasificar_linea("C esto es un comentario") == LINEA_IGNORADA);
    COMPROBAR(clasificar_linea("c tambien en minuscula") == LINEA_IGNORADA);
    COMPROBAR(clasificar_linea("* y con asterisco") == LINEA_IGNORADA);

    /* Sentencias nuevas */
    COMPROBAR(clasificar_linea("      PROGRAM HOLA") == LINEA_INICIAL);
    COMPROBAR(clasificar_linea("   10 CONTINUE") == LINEA_INICIAL);
    COMPROBAR(clasificar_linea("     0X = 1") == LINEA_INICIAL);   /* col 6 = '0' */

    /* Continuaciones: col 6 distinta de blanco y de '0' */
    COMPROBAR(clasificar_linea("     +X = 1") == LINEA_CONTINUACION);
    COMPROBAR(clasificar_linea("     *X = 1") == LINEA_CONTINUACION);  /* '*' en col 6 NO es comentario */
    COMPROBAR(clasificar_linea("     1+2") == LINEA_CONTINUACION);

    /* Lineas cortas: no hay columna 6, no debe leerse fuera de la cadena */
    COMPROBAR(clasificar_linea("END") == LINEA_INICIAL);
    COMPROBAR(clasificar_linea("12345") == LINEA_INICIAL);          /* justo 5 caracteres */
}

static void test_iniciar_sentencia(void)
{
    char larga[81];

    /* Sin etiqueta */
    COMPROBAR(iniciar_sentencia("      PROGRAM HOLA", 7) == FUENTE_OK);
    COMPROBAR(sentencia.etiqueta == 0);
    COMPROBAR(sentencia.linea == 7);
    COMPROBAR(strcmp(sent_texto(), "PROGRAM HOLA") == 0);
    COMPROBAR(sentencia.longitud == 12);
    COMPROBAR(sentencia.segmentos == 1);
    COMPROBAR(sentencia.linea_seg[0] == 7);

    /* Con etiqueta */
    COMPROBAR(iniciar_sentencia("   10 CONTINUE", 1) == FUENTE_OK);
    COMPROBAR(sentencia.etiqueta == 10);
    COMPROBAR(strcmp(sent_texto(), "CONTINUE") == 0);

    /* Los blancos dentro de la etiqueta no cuentan: " 1 0 " vale 10 */
    COMPROBAR(iniciar_sentencia(" 1 0  X = 1", 1) == FUENTE_OK);
    COMPROBAR(sentencia.etiqueta == 10);
    COMPROBAR(strcmp(sent_texto(), "X = 1") == 0);

    /* Etiqueta maxima: no cabe en 16 bits */
    COMPROBAR(iniciar_sentencia("99999 X=1", 1) == FUENTE_OK);
    COMPROBAR(sentencia.etiqueta == 99999);

    /* Etiquetas invalidas */
    COMPROBAR(iniciar_sentencia("AB   X=1", 1) == FUENTE_ETIQUETA_INVALIDA);
    COMPROBAR(iniciar_sentencia("    0 X=1", 1) == FUENTE_ETIQUETA_INVALIDA);

    /* Lineas cortas: no hay texto y no se lee fuera de la cadena */
    /* "END" pegado a la columna 1 es un ERROR en formato fijo: la 'E' caeria
     * en la zona de la etiqueta. Una sentencia empieza en la columna 7. */
    COMPROBAR(iniciar_sentencia("END", 1) == FUENTE_ETIQUETA_INVALIDA);
    COMPROBAR(iniciar_sentencia("  10", 1) == FUENTE_OK);
    COMPROBAR(sentencia.etiqueta == 10);
    COMPROBAR(sentencia.longitud == 0);
    COMPROBAR(iniciar_sentencia("12", 1) == FUENTE_OK);
    COMPROBAR(sentencia.etiqueta == 12);
    COMPROBAR(sentencia.longitud == 0);

    /* Truncado en la columna 72: 6 blancos + 66 'A' + 8 'B' (que se pierden) */
    memset(larga, ' ', 6);
    memset(larga + 6, 'A', 66);
    memset(larga + 72, 'B', 8);
    larga[80] = '\0';
    COMPROBAR(iniciar_sentencia(larga, 1) == FUENTE_OK);
    COMPROBAR(sentencia.longitud == 66);
    COMPROBAR(sent_texto()[65] == 'A');
    COMPROBAR(sent_texto()[66] == '\0');
}

/* Ayuda: lee la siguiente sentencia y devuelve su resultado. */
static ErrorFuente leer(Fuente *f)
{
    return leer_sentencia(f);
}

/* Definida mas abajo: los tokens de una sentencia, como cadena. */
static void tokens_de(char *out);

/* Construye n lineas de fuente: la primera inicial y el resto continuaciones,
 * cada una con 66 'A' en las columnas 7-72. */
static void construir_lineas(char *dst, int n)
{
    int k;
    int pos = 0;
    int i;

    for (k = 0; k < n; k++) {
        memcpy(dst + pos, k == 0 ? "      " : "     +", 6);
        pos += 6;
        for (i = 0; i < 66; i++) {
            dst[pos++] = 'A';
        }
        dst[pos++] = '\n';
    }
    dst[pos] = '\0';
}

/* Tabuladores al inicio de linea (extension DEC/g77, decision 0004). */
static void test_tabuladores(void)
{
    Fuente f;

    /* Tab -> el texto empieza en la columna 7 */
    fuente_iniciar_texto(&f, "\tX = 1\n");
    COMPROBAR(leer(&f) == FUENTE_OK);
    COMPROBAR(sentencia.etiqueta == 0);
    COMPROBAR(strcmp(sent_texto(), "X = 1") == 0);

    /* Etiqueta + tab */
    fuente_iniciar_texto(&f, "10\tCONTINUE\n");
    COMPROBAR(leer(&f) == FUENTE_OK);
    COMPROBAR(sentencia.etiqueta == 10);
    COMPROBAR(strcmp(sent_texto(), "CONTINUE") == 0);

    /* Etiqueta de 5 digitos + tab en la columna 6 */
    fuente_iniciar_texto(&f, "12345\tEND\n");
    COMPROBAR(leer(&f) == FUENTE_OK);
    COMPROBAR(sentencia.etiqueta == 12345);
    COMPROBAR(strcmp(sent_texto(), "END") == 0);

    /* Tab + digito 1-9 = continuacion */
    fuente_iniciar_texto(&f, "\tX = 1 +\n\t1Y\n");
    COMPROBAR(leer(&f) == FUENTE_OK);
    COMPROBAR(sentencia.segmentos == 2);
    COMPROBAR(sent_texto()[SEGMENTO] == 'Y');
    COMPROBAR(sentencia.linea_seg[1] == 2);

    /* Tab + '0' NO es continuacion: es una sentencia que empieza por 0 */
    fuente_iniciar_texto(&f, "\t0\n");
    COMPROBAR(leer(&f) == FUENTE_OK);
    COMPROBAR(strcmp(sent_texto(), "0") == 0);

    /* Linea con solo un tab: en blanco */
    fuente_iniciar_texto(&f, "\t\n\tX=1\n");
    COMPROBAR(leer(&f) == FUENTE_OK);
    COMPROBAR(sentencia.linea == 2);

    /* Comentario con tab: sigue siendo comentario */
    fuente_iniciar_texto(&f, "C\tcomentario\n\tX=1\n");
    COMPROBAR(leer(&f) == FUENTE_OK);
    COMPROBAR(sentencia.linea == 2);

    /* Tab en el cuerpo (col >= 7): se conserva como un caracter */
    fuente_iniciar_texto(&f, "      A\tB\n");
    COMPROBAR(leer(&f) == FUENTE_OK);
    COMPROBAR(strcmp(sent_texto(), "A\tB") == 0);

    /* Posicion de error tras expandir: la X esta en la columna 7 */
    fuente_iniciar_texto(&f, "\tX=1\n");
    COMPROBAR(leer(&f) == FUENTE_OK);
    {
        uint16_t lin, col;
        posicion_fuente(0, &lin, &col);
        COMPROBAR(lin == 1 && col == 7);
    }
}

static void test_nivel1(void)
{
    Fuente f;
    char largo[1700];
    char tk[600];
    char esperado[200];
    uint16_t lin;
    uint16_t col;

    /* Continuacion simple. La 1a linea se rellena con blancos hasta 66. */
    fuente_iniciar_texto(&f, "      X = 1 +\n     +2\n");
    COMPROBAR(leer(&f) == FUENTE_OK);
    tokens_de(tk);
    COMPROBAR(strcmp(tk, "N:X = I:1 + I:2") == 0);
    COMPROBAR(sentencia.linea == 1);
    COMPROBAR(sentencia.segmentos == 2);
    COMPROBAR(sentencia.linea_seg[1] == 2);
    COMPROBAR(sentencia.longitud == 67);
    COMPROBAR(sent_texto()[7] == ' ');         /* relleno */
    COMPROBAR(sent_texto()[65] == ' ');
    COMPROBAR(sent_texto()[66] == '2');        /* empieza la 2a linea */
    COMPROBAR(leer(&f) == FUENTE_FIN);

    /* El texto se conserva TAL CUAL: con blancos y minusculas */
    fuente_iniciar_texto(&f, "      x = 'a  b'\n");
    COMPROBAR(leer(&f) == FUENTE_OK);
    COMPROBAR(strcmp(sent_texto(), "x = 'a  b'") == 0);

    /* ...y es el escaner quien salta blancos y pasa a mayusculas */
    fuente_iniciar_texto(&f, "      go to 10\n");
    COMPROBAR(leer(&f) == FUENTE_OK);
    tokens_de(tk);
    COMPROBAR(strcmp(tk, "N:GOTO10") == 0);
    fuente_iniciar_texto(&f, "      x = 'a  b', 'It''s'\n");
    COMPROBAR(leer(&f) == FUENTE_OK);
    tokens_de(tk);
    COMPROBAR(strcmp(tk, "N:X = S:a  b , S:It's") == 0);

    /* Una cadena partida entre dos lineas: la primera se rellena hasta la
     * columna 72, asi que la cadena lleva 59 blancos en medio (de "X = 'AB"
     * quedan 59 posiciones hasta la 66). */
    fuente_iniciar_texto(&f, "      X = 'AB\n     +CD'\n");
    COMPROBAR(leer(&f) == FUENTE_OK);
    tokens_de(tk);
    sprintf(esperado, "N:X = S:AB%59sCD", "");
    COMPROBAR(strcmp(tk, esperado) == 0);

    /* Varias sentencias, comentarios y blancos entre medias, CRLF, y el
     * ultimo sin salto de linea */
    fuente_iniciar_texto(&f,
        "C comentario\r\n"
        "      PROGRAM HOLA\r\n"
        "\r\n"
        "   10 X = 1 +\r\n"
        "C comentario entre continuaciones\r\n"
        "     + 2\r\n"
        "      END");
    COMPROBAR(leer(&f) == FUENTE_OK);
    COMPROBAR(strcmp(sent_texto(), "PROGRAM HOLA") == 0);
    COMPROBAR(sentencia.linea == 2);
    COMPROBAR(leer(&f) == FUENTE_OK);
    COMPROBAR(sentencia.etiqueta == 10);
    tokens_de(tk);
    COMPROBAR(strcmp(tk, "N:X = I:1 + I:2") == 0);
    COMPROBAR(sentencia.linea == 4);
    COMPROBAR(sentencia.segmentos == 2);
    COMPROBAR(sentencia.linea_seg[1] == 6);       /* la 5 era un comentario */
    /* posicion_fuente: de un offset del texto a linea y columna del fichero */
    posicion_fuente(0, &lin, &col);
    COMPROBAR(lin == 4 && col == 7);
    posicion_fuente(5, &lin, &col);
    COMPROBAR(lin == 4 && col == 12);
    posicion_fuente(65, &lin, &col);
    COMPROBAR(lin == 4 && col == 72);
    posicion_fuente(66, &lin, &col);
    COMPROBAR(lin == 6 && col == 7);
    posicion_fuente(67, &lin, &col);
    COMPROBAR(lin == 6 && col == 8);
    COMPROBAR(leer(&f) == FUENTE_OK);
    COMPROBAR(strcmp(sent_texto(), "END") == 0);
    COMPROBAR(sentencia.linea == 7);
    posicion_fuente(3, &lin, &col);   /* el final: justo tras el texto */
    COMPROBAR(lin == 7 && col == 10);
    COMPROBAR(leer(&f) == FUENTE_FIN);

    /* Fuente vacio */
    fuente_iniciar_texto(&f, "");
    COMPROBAR(leer(&f) == FUENTE_FIN);

    /* Continuacion huerfana: error en su linea; luego sigue todo bien */
    fuente_iniciar_texto(&f, "C hola\n     +X\n      END\n");
    COMPROBAR(leer(&f) == FUENTE_CONTINUACION_INVALIDA);
    COMPROBAR(f.linea_error == 2);
    COMPROBAR(leer(&f) == FUENTE_OK);
    COMPROBAR(strcmp(sent_texto(), "END") == 0);

    /* Etiqueta en una continuacion: error, y se recupera sin cascada */
    fuente_iniciar_texto(&f, "      X = 1\n  100+2\n     +3\n      END\n");
    COMPROBAR(leer(&f) == FUENTE_CONTINUACION_INVALIDA);
    COMPROBAR(f.linea_error == 2);
    COMPROBAR(leer(&f) == FUENTE_OK);
    COMPROBAR(strcmp(sent_texto(), "END") == 0);

    /* Etiqueta invalida en la linea inicial, con una continuacion detras */
    fuente_iniciar_texto(&f, "AB    X = 1\n     +2\n      END\n");   /* 4 blancos: col 6 libre */
    COMPROBAR(leer(&f) == FUENTE_ETIQUETA_INVALIDA);
    COMPROBAR(f.linea_error == 1);
    COMPROBAR(leer(&f) == FUENTE_OK);
    COMPROBAR(strcmp(sent_texto(), "END") == 0);

    /* MAX_SEGMENTOS lineas llenas caben justo (20 * 66 = 1320) */
    construir_lineas(largo, MAX_SEGMENTOS);
    fuente_iniciar_texto(&f, largo);
    COMPROBAR(leer(&f) == FUENTE_OK);
    COMPROBAR(sentencia.segmentos == MAX_SEGMENTOS);
    COMPROBAR(sentencia.longitud == MAX_SENTENCIA);
    COMPROBAR(leer(&f) == FUENTE_FIN);

    /* La siguiente no cabe: error en esa linea, y se consume el resto */
    construir_lineas(largo, MAX_SEGMENTOS + 2);
    fuente_iniciar_texto(&f, largo);
    COMPROBAR(leer(&f) == FUENTE_SENTENCIA_LARGA);
    COMPROBAR(f.linea_error == MAX_SEGMENTOS + 1);
    COMPROBAR(leer(&f) == FUENTE_FIN);
}

/* ==================================================================
 * Tests del escaner (lexer.c)
 * ================================================================== */

/* Texto corto de cada token, para escribir los casos en una linea.
 *   N:ALFA  nombre      I:12    entero      R:1.5E3 real      D:1D0 doble
 *   S:hola  cadena      .T. .F. logicos     !CODIGO error lexico
 */
static const char *simbolo(TipoToken tipo)
{
    switch (tipo) {
    case TOK_MAS:        return "+";
    case TOK_MENOS:      return "-";
    case TOK_POR:        return "*";
    case TOK_DIV:        return "/";
    case TOK_POTENCIA:   return "**";
    case TOK_CONCAT:     return "//";
    case TOK_IGUAL:      return "=";
    case TOK_PAR_ABRE:   return "(";
    case TOK_PAR_CIERRA: return ")";
    case TOK_COMA:       return ",";
    case TOK_DOS_PUNTOS: return ":";
    case TOK_EQ:         return ".EQ.";
    case TOK_NE:         return ".NE.";
    case TOK_LT:         return ".LT.";
    case TOK_LE:         return ".LE.";
    case TOK_GT:         return ".GT.";
    case TOK_GE:         return ".GE.";
    case TOK_AND:        return ".AND.";
    case TOK_OR:         return ".OR.";
    case TOK_NOT:        return ".NOT.";
    case TOK_EQV:        return ".EQV.";
    case TOK_NEQV:       return ".NEQV.";
    default:             return "?";
    }
}

static const char *codigo_error(ErrorLex e)
{
    switch (e) {
    case LEX_CARACTER_ILEGAL:      return "!ILEGAL";
    case LEX_CADENA_SIN_CERRAR:    return "!CADENA";
    case LEX_NOMBRE_LARGO:         return "!NOMBRE_LARGO";
    case LEX_ENTERO_GRANDE:        return "!ENTERO_GRANDE";
    case LEX_REAL_MAL_FORMADO:     return "!REAL";
    case LEX_OPERADOR_DESCONOCIDO: return "!OPERADOR";
    case LEX_TABLA_LLENA:          return "!TABLA_LLENA";
    }
    return "!?";
}

/* Convierte un token en su texto corto y lo anade a out. */
static void anadir_token(char *out, const Token *t)
{
    char tmp[300];

    switch (t->tipo) {
    case TOK_NOMBRE:
        sprintf(tmp, "N:%s", tabla_texto(t->indice));
        break;
    case TOK_ENTERO:
        sprintf(tmp, "I:%lu", (unsigned long)t->valor);
        break;
    case TOK_REAL:
        sprintf(tmp, "R:%s", tabla_texto(t->indice));
        break;
    case TOK_DOBLE:
        sprintf(tmp, "D:%s", tabla_texto(t->indice));
        break;
    case TOK_CADENA:
        sprintf(tmp, "S:%s", tabla_texto(t->indice));
        break;
    case TOK_LOGICO:
        sprintf(tmp, "%s", t->valor ? ".T." : ".F.");
        break;
    case TOK_ERROR:
        sprintf(tmp, "%s", codigo_error((ErrorLex)t->valor));
        break;
    default:
        sprintf(tmp, "%s", simbolo(t->tipo));
        break;
    }
    if (out[0] != '\0') {
        strcat(out, " ");
    }
    strcat(out, tmp);
}

/* Prepara una Sentencia con 'texto' (que ya debe estar normalizado). */
static void sentencia_de(const char *texto)
{
    sent_poner(texto);
    sentencia.linea = 1;
    sentencia.etiqueta = 0;
    sentencia.segmentos = 1;
    sentencia.linea_seg[0] = 1;
}

/* Escanea una sentencia hasta el final o hasta el primer error y deja el
 * resultado como cadena en out. */
static void escanear(char *out)
{
    Token t;

    out[0] = '\0';
    for (;;) {
        lex_siguiente(&t);
        if (t.tipo == TOK_FIN) {
            break;
        }
        anadir_token(out, &t);
        if (t.tipo == TOK_ERROR) {
            break;
        }
    }
}

static void lexar(const char *texto, char *out)
{

    tabla_iniciar();
    sentencia_de(texto);
    lex_iniciar();
    escanear(out);
}

/* Los tokens de una sentencia, como cadena (para comprobarlos de un vistazo). */
static void tokens_de(char *out)
{

    tabla_iniciar();
    lex_iniciar();
    escanear(out);
}

/* Comprueba que lexar(entrada) produce exactamente 'esperado'. */
#define LEXAR(entrada, esperado)                                     \
    do {                                                             \
        char res[600];                                               \
        lexar((entrada), res);                                       \
        pruebas++;                                                   \
        if (strcmp(res, (esperado)) != 0) {                          \
            fallos++;                                                \
            printf("FALLO (linea %d): lexar(\"%s\")\n"               \
                   "   esperado: %s\n   obtenido: %s\n",             \
                   __LINE__, (entrada), (esperado), res);            \
        }                                                            \
    } while (0)

static void test_lexer_basico(void)
{
    /* Nombres, enteros y simbolos */
    LEXAR("X=A+B*2", "N:X = N:A + N:B * I:2");
    LEXAR("ALFA(I,J)=0", "N:ALFA ( N:I , N:J ) = I:0");
    LEXAR("A1B2", "N:A1B2");
    LEXAR("A+-B", "N:A + - N:B");
    LEXAR("(A:B)", "( N:A : N:B )");
    LEXAR("", "");

    /* Operadores de uno y dos caracteres: se toma siempre el mas largo */
    LEXAR("A**2", "N:A ** I:2");
    LEXAR("A***B", "N:A ** * N:B");
    LEXAR("A//B", "N:A // N:B");
    LEXAR("A///B", "N:A // / N:B");
    LEXAR("A/B", "N:A / N:B");

    /* Nombres: el limite es NOMBRE_MAX = 8 */
    LEXAR("ABCDEFGH", "N:ABCDEFGH");
    LEXAR("ABCDEFGHI", "!NOMBRE_LARGO");

    /* Enteros: maximo 2147483647 */
    LEXAR("0", "I:0");
    LEXAR("2147483647", "I:2147483647");
    LEXAR("2147483648", "!ENTERO_GRANDE");
    LEXAR("99999999999", "!ENTERO_GRANDE");
}

static void test_lexer_reales(void)
{
    LEXAR("1.5", "R:1.5");
    LEXAR("1.", "R:1.");
    LEXAR(".5", "R:.5");
    LEXAR("1E3", "R:1E3");
    LEXAR("1.5E-3", "R:1.5E-3");
    LEXAR("1.E+2", "R:1.E+2");
    LEXAR(".5E2", "R:.5E2");
    LEXAR("1D0", "D:1D0");
    LEXAR("2.5D-10", "D:2.5D-10");

    /* Exponente sin digitos */
    LEXAR("1E", "!REAL");
    LEXAR("1E+", "!REAL");
    LEXAR("2.5D", "!REAL");

    /* Un real dentro de una expresion */
    LEXAR("X=1.5*Y", "N:X = R:1.5 * N:Y");
    LEXAR("X=1.+2", "N:X = R:1. + I:2");
}

static void test_lexer_punto(void)
{
    /* La ambiguedad clasica: 1.EQ.2 es entero, operador, entero */
    LEXAR("1.EQ.2", "I:1 .EQ. I:2");
    LEXAR("1.E3", "R:1.E3");                    /* en cambio esto es un real */
    LEXAR("1.5.AND.A", "R:1.5 .AND. N:A");
    LEXAR("X.GT..5", "N:X .GT. R:.5");
    LEXAR("X.EQ.1.", "N:X .EQ. R:1.");
    LEXAR("A.LT.1.E-3", "N:A .LT. R:1.E-3");

    /* Todos los operadores con punto */
    LEXAR("A.EQ.B", "N:A .EQ. N:B");
    LEXAR("A.NE.B", "N:A .NE. N:B");
    LEXAR("A.LT.B", "N:A .LT. N:B");
    LEXAR("A.LE.B", "N:A .LE. N:B");
    LEXAR("A.GT.B", "N:A .GT. N:B");
    LEXAR("A.GE.B", "N:A .GE. N:B");
    LEXAR("A.AND.B.OR..NOT.C", "N:A .AND. N:B .OR. .NOT. N:C");
    LEXAR("A.EQV.B.NEQV.C", "N:A .EQV. N:B .NEQV. N:C");

    /* Constantes logicas */
    LEXAR(".TRUE.", ".T.");
    LEXAR(".FALSE.", ".F.");
    LEXAR("L=.TRUE..AND.M", "N:L = .T. .AND. N:M");

    /* Errores con punto */
    LEXAR(".FOO.", "!OPERADOR");
    LEXAR(".", "!ILEGAL");
    LEXAR("A.B", "N:A !ILEGAL");
    LEXAR(".EQ", "!ILEGAL");
}

static void test_lexer_cadenas(void)
{
    LEXAR("'HOLA'", "S:HOLA");
    LEXAR("'A  B'", "S:A  B");                  /* los blancos se conservan */
    LEXAR("'IT''S'", "S:IT'S");                 /* '' dentro = una comilla  */
    LEXAR("''''", "S:'");
    LEXAR("''", "S:");                          /* vacia: el estandar no la admite;
                                                   la aceptamos (pendiente)  */
    LEXAR("PRINT*,'X'", "N:PRINT * , S:X");
    LEXAR("'abc", "!CADENA");
    LEXAR("'IT''", "!CADENA");
    LEXAR("'a+b'+1", "S:a+b + I:1");            /* lo de dentro NO se escanea */
}

static void test_lexer_errores_y_estado(void)
{
    Token t;
    Token a, b, c;
    char nom[16];
    int k;

    /* Caracteres ilegales */
    LEXAR("A#B", "N:A !ILEGAL");
    LEXAR("$", "!ILEGAL");

    /* Tras un error el escaner sigue: no se queda atascado */
    tabla_iniciar();
    sentencia_de("A#B");
    lex_iniciar();
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_NOMBRE);
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_ERROR);
    COMPROBAR((ErrorLex)t.valor == LEX_CARACTER_ILEGAL);
    COMPROBAR(t.pos == 1);
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_NOMBRE);
    COMPROBAR(strcmp(tabla_texto(t.indice), "B") == 0);
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_FIN);
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_FIN);               /* y se repite */

    /* Posiciones: offset en el texto de la sentencia */
    tabla_iniciar();
    sentencia_de("AB+12");
    lex_iniciar();
    lex_siguiente(&t);
    COMPROBAR(t.pos == 0);
    lex_siguiente(&t);
    COMPROBAR(t.pos == 2);
    lex_siguiente(&t);
    COMPROBAR(t.pos == 3);

    /* Valor y mensajes */
    tabla_iniciar();
    sentencia_de("123");
    lex_iniciar();
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_ENTERO && t.valor == 123);
    COMPROBAR(strcmp(lex_mensaje(LEX_ENTERO_GRANDE),
                     "integer constant is too large") == 0);

    /* Tabla de cadenas: los textos iguales comparten indice */
    tabla_iniciar();
    sentencia_de("X+X+Y");
    lex_iniciar();
    lex_siguiente(&a);
    lex_siguiente(&t);
    lex_siguiente(&b);
    lex_siguiente(&t);
    lex_siguiente(&c);
    COMPROBAR(a.indice == b.indice);
    COMPROBAR(a.indice != c.indice);
    COMPROBAR(a.indice != 0);

    /* "AB" no se confunde con "ABC" ni con "A" */
    tabla_iniciar();
    COMPROBAR(tabla_internar("ABC", 3) != tabla_internar("AB", 2));
    COMPROBAR(tabla_internar("AB", 2) == tabla_internar("AB", 2));
    COMPROBAR(tabla_internar("", 0) == 0);

    /* Tabla llena: se llena con nombres distintos y el escaner lo notifica */
    tabla_iniciar();
    for (k = 0; k < 5000; k++) {
        sprintf(nom, "N%07d", k);
        if (tabla_internar(nom, 8) == TABLA_LLENA) {
            break;
        }
    }
    COMPROBAR(k < 5000);
    sentencia_de("ZZZZZZZZ");
    lex_iniciar();
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_ERROR && (ErrorLex)t.valor == LEX_TABLA_LLENA);
}

/* Deja la tabla con exactamente 'libre' bytes sin usar (el primero que se
 * interna es N0000000, de 8 caracteres). */
static void llenar_tabla_hasta(uint16_t libre)
{
    char nom[16];
    char relleno[TABLA_TAM];
    uint16_t k = 0;
    uint16_t resto;

    tabla_iniciar();
    while ((uint16_t)(TABLA_TAM - cadenas.usado) > (uint16_t)(libre + 9)) {
        sprintf(nom, "N%07u", (unsigned)k++);
        tabla_internar(nom, 8);
    }
    resto = (uint16_t)(TABLA_TAM - cadenas.usado - libre);   /* 1..9 */
    if (resto >= 2) {
        memset(relleno, 'Q', resto - 1);
        tabla_internar(relleno, (uint16_t)(resto - 1));
    }
}

/* El buffer de trabajo del lexer es la cola libre de la tabla de cadenas. */
static void test_lexer_cola_tabla(void)
{
    Token t;
    Token a;
    static char largo[1400];
    uint16_t antes;
    uint16_t n;

    /* Tabla exactamente llena: un nombre YA presente se sigue encontrando; lo nuevo falla */
    llenar_tabla_hasta(0);
    COMPROBAR(cadenas.usado == TABLA_TAM);
    sentencia_de("N0000000 + 1.5 + 'AB' + ZZ");
    lex_iniciar();
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_NOMBRE && t.indice == tabla_internar("N0000000", 8));
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_MAS);
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_ERROR && (ErrorLex)t.valor == LEX_TABLA_LLENA);   /* 1.5 */
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_MAS);                                            /* sigue sincronizado */
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_ERROR && (ErrorLex)t.valor == LEX_TABLA_LLENA);   /* 'AB' */
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_MAS);
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_ERROR && (ErrorLex)t.valor == LEX_TABLA_LLENA);   /* ZZ nuevo */
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_FIN);

    /* Justo en el limite: con 4 bytes libres caben 3 caracteres + '\0', no 4 */
    llenar_tabla_hasta(4);
    COMPROBAR((uint16_t)(TABLA_TAM - cadenas.usado) == 4);
    sentencia_de("1.5");
    lex_iniciar();
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_REAL && strcmp(tabla_texto(t.indice), "1.5") == 0);
    COMPROBAR(cadenas.usado == TABLA_TAM);

    llenar_tabla_hasta(4);
    sentencia_de("'ABC'");
    lex_iniciar();
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_CADENA && strcmp(tabla_texto(t.indice), "ABC") == 0);

    llenar_tabla_hasta(4);
    sentencia_de("'ABCD'");
    lex_iniciar();
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_ERROR && (ErrorLex)t.valor == LEX_TABLA_LLENA);
    COMPROBAR(cadenas.usado == (uint16_t)(TABLA_TAM - 4));        /* no ensucia la tabla */

    /* Comillas dobladas en el limite: 'AB''' son 3 caracteres (AB'), 'ABC''' son 4 */
    llenar_tabla_hasta(4);
    sentencia_de("'AB'''");
    lex_iniciar();
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_CADENA && strcmp(tabla_texto(t.indice), "AB'") == 0);
    llenar_tabla_hasta(4);
    sentencia_de("'ABC'''");
    lex_iniciar();
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_ERROR && (ErrorLex)t.valor == LEX_TABLA_LLENA);

    llenar_tabla_hasta(4);
    sentencia_de("1.55");
    lex_iniciar();
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_ERROR && (ErrorLex)t.valor == LEX_TABLA_LLENA);

    /* Un texto repetido no crece la tabla (la cola se descarta) */
    tabla_iniciar();
    sentencia_de("'HOLA' + 'HOLA' + 2.5 + 2.5");
    lex_iniciar();
    lex_siguiente(&a);
    lex_siguiente(&t);                              /* + */
    antes = cadenas.usado;
    lex_siguiente(&t);
    COMPROBAR(a.tipo == TOK_CADENA && t.indice == a.indice && cadenas.usado == antes);
    lex_siguiente(&t);                              /* + */
    lex_siguiente(&a);
    lex_siguiente(&t);                              /* + */
    antes = cadenas.usado;
    lex_siguiente(&t);
    COMPROBAR(a.tipo == TOK_REAL && t.indice == a.indice && cadenas.usado == antes);

    /* Cadenas largas: una de 255 caracteres (el maximo de CHARACTER; el buffer de trabajo es de TABLA_BUF_TAM = 256) cabe;
     * una de 256 ya no (aunque la tabla tenga sitio); y una mayor que la tabla tampoco */
    tabla_iniciar();
    largo[0] = '\'';
    memset(largo + 1, 'x', 255);
    largo[256] = '\'';
    largo[257] = '\0';
    sentencia_de(largo);
    lex_iniciar();
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_CADENA && strlen(tabla_texto(t.indice)) == 255);
    COMPROBAR(tabla_texto(t.indice)[254] == 'x' && tabla_texto(t.indice)[255] == '\0');

    tabla_iniciar();
    memset(largo + 1, 'x', 256);
    largo[257] = '\'';
    largo[258] = '\0';
    sentencia_de(largo);
    lex_iniciar();
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_ERROR && (ErrorLex)t.valor == LEX_TABLA_LLENA);
    COMPROBAR(cadenas.usado == 1);

    tabla_iniciar();
    memset(largo + 1, 'x', 1300);
    largo[1301] = '\'';
    largo[1302] = '\0';
    sentencia_de(largo);
    lex_iniciar();
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_ERROR && (ErrorLex)t.valor == LEX_TABLA_LLENA);
    COMPROBAR(cadenas.usado == 1);
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_FIN);                 /* se consumio la cadena entera */

    /* Cadena larga con comillas dobladas y sin cerrar: gana el error de comilla */
    tabla_iniciar();
    largo[0] = '\'';
    for (n = 1; n < 1200; n++) {
        largo[n] = 'y';
    }
    largo[1200] = '\0';
    sentencia_de(largo);
    lex_iniciar();
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_ERROR && (ErrorLex)t.valor == LEX_CADENA_SIN_CERRAR);

    /* Operador con punto demasiado largo: error y sigue sincronizado */
    tabla_iniciar();
    sentencia_de(".ABCDEFGH. + .ABCDEFG. + .ABCDEFGHIJKL. + .EQ.");   /* 8, 7 y 12 letras */
    lex_iniciar();
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_ERROR && (ErrorLex)t.valor == LEX_OPERADOR_DESCONOCIDO);
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_MAS);
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_ERROR && (ErrorLex)t.valor == LEX_OPERADOR_DESCONOCIDO);
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_MAS);
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_ERROR && (ErrorLex)t.valor == LEX_OPERADOR_DESCONOCIDO);
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_MAS);
    lex_siguiente(&t);
    COMPROBAR(t.tipo == TOK_EQ);
}

/* De extremo a extremo: texto fuente -> sentencias -> tokens */
static void test_lexer_integracion(void)
{
    Fuente f;
    char out[600];

    tabla_iniciar();
    fuente_iniciar_texto(&f,
        "C calculo\n"
        "   10 x = 1.5 *\n"
        "     + y\n"
        "      z = .true. .and. 'ok'\n");

    COMPROBAR(leer_sentencia(&f) == FUENTE_OK);
    COMPROBAR(sentencia.etiqueta == 10);
    lex_iniciar();
    escanear(out);
    COMPROBAR(strcmp(out, "N:X = R:1.5 * N:Y") == 0);

    COMPROBAR(leer_sentencia(&f) == FUENTE_OK);
    lex_iniciar();
    escanear(out);
    COMPROBAR(strcmp(out, "N:Z = .T. .AND. S:ok") == 0);

    COMPROBAR(leer_sentencia(&f) == FUENTE_FIN);
}


/* Los blancos no significan nada fuera de las constantes de texto */
static void test_lexer_blancos(void)
{
    Fuente f;
    Token t;
    uint16_t lin;
    uint16_t col;

    LEXAR("A .EQ. B", "N:A .EQ. N:B");
    LEXAR("1 . EQ . 2", "I:1 .EQ. I:2");
    LEXAR(". true .", ".T.");
    LEXAR("1 0 0", "I:100");                    /* un blanco dentro del numero */
    LEXAR("x = a + b", "N:X = N:A + N:B");      /* minusculas */
    LEXAR("GO TO 10", "N:GOTO10");              /* palabras clave: las separara la 2.4 */
    LEXAR("A B", "N:AB");                       /* blanco dentro de un nombre */
    LEXAR("2 * * 3", "I:2 ** I:3");
    LEXAR("1 . 5", "R:1.5");                    /* el texto del real queda sin blancos */
    LEXAR("1 E 3", "R:1E3");
    LEXAR("1.5 e - 3", "R:1.5E-3");
    LEXAR("X = 'a b'", "N:X = S:a b");          /* dentro de una cadena, TAL CUAL */
    LEXAR("x = 'abc", "N:X = !CADENA");
    LEXAR("  ", "");
    LEXAR("A   ", "N:A");
    LEXAR("\tX", "N:X");                        /* el tabulador es un blanco */

    /* Posiciones brutas: dentro del texto, contando los blancos */
    tabla_iniciar();
    sentencia_de("  AB + 12");
    lex_iniciar();
    lex_siguiente(&t);
    COMPROBAR(t.pos == 2);
    lex_siguiente(&t);
    COMPROBAR(t.pos == 5);
    lex_siguiente(&t);
    COMPROBAR(t.pos == 7);

    /* De un token a su linea y columna reales en el fichero */
    tabla_iniciar();
    fuente_iniciar_texto(&f, "C x\n      X = 10 + Y\n");
    COMPROBAR(leer_sentencia(&f) == FUENTE_OK);
    lex_iniciar();
    lex_siguiente(&t);                     /* X  */
    lex_siguiente(&t);                     /* =  */
    lex_siguiente(&t);                     /* 10 */
    COMPROBAR(t.tipo == TOK_ENTERO);
    posicion_fuente(t.pos, &lin, &col);
    COMPROBAR(lin == 2 && col == 11);
}

/* ==================================================================
 * Tests de la clasificacion de sentencias (clasif.c)
 * ================================================================== */

static const char *nombre_sentencia(TipoSentencia t)
{
    switch (t) {
    case SENT_DESCONOCIDA:     return "DESCONOCIDA";
    case SENT_VACIA:           return "VACIA";
    case SENT_ASIGNACION:      return "ASIGNACION";
    case SENT_PROGRAM:         return "PROGRAM";
    case SENT_END:             return "END";
    case SENT_SUBROUTINE:      return "SUBROUTINE";
    case SENT_FUNCTION:        return "FUNCTION";
    case SENT_CALL:            return "CALL";
    case SENT_RETURN:          return "RETURN";
    case SENT_INTEGER:         return "INTEGER";
    case SENT_REAL:            return "REAL";
    case SENT_DOUBLEPRECISION: return "DOUBLEPRECISION";
    case SENT_LOGICAL:         return "LOGICAL";
    case SENT_CHARACTER:       return "CHARACTER";
    case SENT_PRINT:           return "PRINT";
    case SENT_CONTINUE:        return "CONTINUE";
    case SENT_GOTO:            return "GOTO";
    case SENT_STOP:            return "STOP";
    case SENT_DO:              return "DO";
    case SENT_IF_BLOQUE:       return "IF_BLOQUE";
    case SENT_IF_ARITMETICO:   return "IF_ARITMETICO";
    case SENT_IF_LOGICO:       return "IF_LOGICO";
    case SENT_ELSEIF:          return "ELSEIF";
    case SENT_ELSE:            return "ELSE";
    case SENT_ENDIF:           return "ENDIF";
    case SENT_DIMENSION:       return "DIMENSION";
    case SENT_EXTERNAL:        return "EXTERNAL";
    case SENT_INTRINSIC:       return "INTRINSIC";
    }
    return "?";
}

/* Clasifica 'texto' (sin normalizar: con blancos y minusculas si se quiere) y
 * deja en out "TIPO" o "TIPO(etiqueta)", seguido de " | " y los tokens que el
 * escaner lee a partir de c.resto. */
static void clasificar_texto(const char *texto, char *out)
{
    Clasificacion c;
    char tokens[500];
    char tmp[40];

    tabla_iniciar();
    sentencia_de(texto);
    lex_iniciar();
    clasificar_sentencia(&c);

    strcpy(out, nombre_sentencia(c.tipo));
    if (c.tipo == SENT_DO) {
        sprintf(tmp, "(%lu)", (unsigned long)c.etiqueta);
        strcat(out, tmp);
    }
    /* En una sentencia desconocida o vacia, 'resto' no significa nada: no se escanea */
    tokens[0] = '\0';
    if (c.tipo != SENT_DESCONOCIDA && c.tipo != SENT_VACIA) {
        lexer.pos = c.resto;
        escanear(tokens);
    }
    if (tokens[0] != '\0') {
        strcat(out, " | ");
        strcat(out, tokens);
    }
}

#define CLASIF(entrada, esperado)                                    \
    do {                                                             \
        char res[700];                                               \
        clasificar_texto((entrada), res);                            \
        pruebas++;                                                   \
        if (strcmp(res, (esperado)) != 0) {                          \
            fallos++;                                                \
            printf("FALLO (linea %d): clasificar(\"%s\")\n"          \
                   "   esperado: %s\n   obtenido: %s\n",             \
                   __LINE__, (entrada), (esperado), res);            \
        }                                                            \
    } while (0)

static void test_clasif_do(void)
{
    /* La regla de la coma */
    CLASIF("DO10I=1,10",    "DO(10) | N:I = I:1 , I:10");
    CLASIF("DO10I=1.10",    "ASIGNACION | N:DO10I = R:1.10");
    CLASIF("DO 10 I = 1,10",  "DO(10) | N:I = I:1 , I:10");
    CLASIF("DO 10 I = 1. 10", "ASIGNACION | N:DO10I = R:1.10");
    CLASIF("do 10 i = 1, 10", "DO(10) | N:I = I:1 , I:10");     /* minusculas */

    /* La coma debe estar FUERA de parentesis y de cadenas */
    CLASIF("DO10I=F(1,2)",  "ASIGNACION | N:DO10I = N:F ( I:1 , I:2 )");
    CLASIF("DO10I='A,B'",   "ASIGNACION | N:DO10I = S:A,B");
    CLASIF("DO10I=F(1,2),3", "DO(10) | N:I = N:F ( I:1 , I:2 ) , I:3");
    CLASIF("DO10I=1,N,2",   "DO(10) | N:I = I:1 , N:N , I:2");   /* con paso */

    /* La variable puede llevar digitos y empezar por E o D: la etiqueta la
     * lee el clasificador, si no el escaner leeria "10E" como un exponente */
    CLASIF("DO 10 I1 = 1,5",  "DO(10) | N:I1 = I:1 , I:5");
    CLASIF("DO10E=1,10",      "DO(10) | N:E = I:1 , I:10");
    CLASIF("DO 20 D = 1,3",   "DO(20) | N:D = I:1 , I:3");
    CLASIF("DO 20 EPS = 1,3", "DO(20) | N:EPS = I:1 , I:3");

    /* La coma opcional detras de la etiqueta */
    CLASIF("DO 10, I = 1,10", "DO(10) | N:I = I:1 , I:10");

    /* Lo que NO es un DO */
    CLASIF("DO10I(1)=2,3",  "ASIGNACION | N:DO10I ( I:1 ) = I:2 , I:3");
    CLASIF("DOI=1,3",       "ASIGNACION | N:DOI = I:1 , I:3");   /* sin etiqueta: no es F77 */
    CLASIF("DO10=1,3",      "ASIGNACION | N:DO10 = I:1 , I:3");  /* falta la variable */
    CLASIF("DO",            "DESCONOCIDA");
    CLASIF("DO 0 I = 1,2",  "DESCONOCIDA");                      /* etiqueta 0 no existe */
    CLASIF("DO 123456 I=1,2", "DESCONOCIDA");                    /* mas de 5 digitos */
}

static void test_clasif_if(void)
{
    CLASIF("IF(X.GT.0)THEN",       "IF_BLOQUE | ( N:X .GT. I:0 ) N:THEN");
    CLASIF("IF (X .GT. 0) THEN",   "IF_BLOQUE | ( N:X .GT. I:0 ) N:THEN");
    CLASIF("IF(X)10,20,30",        "IF_ARITMETICO | ( N:X ) I:10 , I:20 , I:30");
    CLASIF("IF(X.GT.0)Y=1",        "IF_LOGICO | ( N:X .GT. I:0 ) N:Y = I:1");
    CLASIF("IF(X.GT.0)GOTO10",     "IF_LOGICO | ( N:X .GT. I:0 ) N:GOTO10");

    /* Un IF cuya sentencia empieza por THEN no es de bloque */
    CLASIF("IF(L)THENY=1",         "IF_LOGICO | ( N:L ) N:THENY = I:1");

    /* Parentesis y cadenas dentro de la condicion */
    CLASIF("IF((A+B)*C.GT.0)THEN", "IF_BLOQUE | ( ( N:A + N:B ) * N:C .GT. I:0 ) N:THEN");
    CLASIF("IF(A.EQ.')')X=1",      "IF_LOGICO | ( N:A .EQ. S:) ) N:X = I:1");
    /* Estos dos SI distinguen si saltar_grupo ignora las cadenas: un ')' o un
     * '(' dentro de comillas no debe cerrar ni abrir nada */
    CLASIF("IF(A.EQ.')')THEN",     "IF_BLOQUE | ( N:A .EQ. S:) ) N:THEN");
    CLASIF("IF(A.EQ.'(')THEN",     "IF_BLOQUE | ( N:A .EQ. S:( ) N:THEN");
    CLASIF("X(')')=1",             "ASIGNACION | N:X ( S:) ) = I:1");

    /* Un IF puede ser un array: el '=' tras el parentesis lo delata */
    CLASIF("IF(3)=1",              "ASIGNACION | N:IF ( I:3 ) = I:1");
    CLASIF("IF(I,J)=5",            "ASIGNACION | N:IF ( N:I , N:J ) = I:5");
    CLASIF("IF=3",                 "ASIGNACION | N:IF = I:3");

    /* Mal formados */
    CLASIF("IF(X",                 "DESCONOCIDA");
    CLASIF("IF(X)",                "DESCONOCIDA");
    CLASIF("IFX",                  "DESCONOCIDA");

    /* ELSE, ELSEIF, ENDIF */
    CLASIF("ELSEIF(X.LT.0)THEN",   "ELSEIF | ( N:X .LT. I:0 ) N:THEN");
    CLASIF("ELSE IF (X) THEN",     "ELSEIF | ( N:X ) N:THEN");
    CLASIF("ELSEIF(X)Y=1",         "DESCONOCIDA");     /* ELSEIF solo existe de bloque */
    CLASIF("ELSE",                 "ELSE");
    CLASIF("ENDIF",                "ENDIF");
    CLASIF("END IF",               "ENDIF");
}

static void test_clasif_resto(void)
{
    /* Asignaciones: el escaner lee todo */
    CLASIF("X=1",                  "ASIGNACION | N:X = I:1");
    CLASIF("A(I,J)=0",             "ASIGNACION | N:A ( N:I , N:J ) = I:0");
    CLASIF("A(1)(2:3)='AB'",       "ASIGNACION | N:A ( I:1 ) ( I:2 : I:3 ) = S:AB");
    CLASIF("X(1:2)='AB'",          "ASIGNACION | N:X ( I:1 : I:2 ) = S:AB");

    /* Las palabras clave tambien pueden ser variables */
    CLASIF("END=5",                "ASIGNACION | N:END = I:5");
    CLASIF("PRINT=1",              "ASIGNACION | N:PRINT = I:1");
    CLASIF("REAL=2.5",             "ASIGNACION | N:REAL = R:2.5");
    CLASIF("STOP=0",               "ASIGNACION | N:STOP = I:0");

    /* Palabras clave: el escaner arranca justo despues */
    CLASIF("PROGRAM HOLA",         "PROGRAM | N:HOLA");
    CLASIF("PROGRAMHOLA",          "PROGRAM | N:HOLA");
    CLASIF("END",                  "END");
    CLASIF("INTEGER I, J",         "INTEGER | N:I , N:J");
    CLASIF("REAL X(10)",           "REAL | N:X ( I:10 )");
    CLASIF("DOUBLE PRECISION X",   "DOUBLEPRECISION | N:X");
    CLASIF("LOGICAL L",            "LOGICAL | N:L");
    CLASIF("CHARACTER*8 NAME",     "CHARACTER | * I:8 N:NAME");
    CLASIF("PRINT *, 'X'",         "PRINT | * , S:X");
    CLASIF("PRINT 100, A",         "PRINT | I:100 , N:A");
    CLASIF("CONTINUE",             "CONTINUE");
    CLASIF("STOP",                 "STOP");
    CLASIF("STOP 'FIN'",           "STOP | S:FIN");

    /* GO TO, con y sin blancos */
    CLASIF("GO TO 10",             "GOTO | I:10");
    CLASIF("GOTO10",               "GOTO | I:10");
    CLASIF("GOTO (10,20) I",       "GOTO | ( I:10 , I:20 ) N:I");

    /* La palabra mas larga gana */
    CLASIF("ENDIF",                "ENDIF");
    CLASIF("END",                  "END");
    CLASIF("DOUBLEPRECISIONX",     "DOUBLEPRECISION | N:X");

    /* Sin clasificar */
    CLASIF("",                     "VACIA");
    CLASIF("   ",                  "VACIA");
    CLASIF("FOO",                  "DESCONOCIDA");
    CLASIF("CALL FOO(A)",          "CALL | N:FOO ( N:A )");
    CLASIF("CALL FOO",             "CALL | N:FOO");
    CLASIF("CALLFOO(A,B)",         "CALL | N:FOO ( N:A , N:B )");
    CLASIF("CALL=3",               "ASIGNACION | N:CALL = I:3");          /* una variable llamada CALL */
    CLASIF("SUBROUTINE S(A)",      "SUBROUTINE | N:S ( N:A )");
    CLASIF("SUBROUTINE S",         "SUBROUTINE | N:S");
    CLASIF("RETURN",               "RETURN");
    CLASIF("RETURN=1",             "ASIGNACION | N:RETURN = I:1");
    CLASIF("WRITE(6,*)X",          "DESCONOCIDA");
    CLASIF("10",                   "DESCONOCIDA");
    CLASIF("X(1=2",                "DESCONOCIDA");
}

int main(void)
{
    test_clasificar_linea();
    test_iniciar_sentencia();
    test_nivel1();
    test_tabuladores();
    test_lexer_basico();
    test_lexer_reales();
    test_lexer_punto();
    test_lexer_cadenas();
    test_lexer_errores_y_estado();
    test_lexer_cola_tabla();
    test_lexer_integracion();
    test_lexer_blancos();
    test_clasif_do();
    test_clasif_if();
    test_clasif_resto();

    printf("%d pruebas, %d fallos\n", pruebas, fallos);
    return fallos != 0;
}
