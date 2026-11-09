/* lexer.h - Escaner de tokens de Fortran 77 (nivel 2 del lexer).
 *
 * ENTRADA: una Sentencia tal como la deja source.c: texto original, con
 *          blancos y minusculas. El escaner SALTA los blancos (en F77 no
 *          significan nada fuera de las constantes de texto) y pasa a
 *          mayusculas al leer. Consecuencia: "GO TO 10" y "GOTO10" dan los
 *          mismos tokens, y "A B" es el nombre AB.
 * SALIDA:  un token cada vez que se le pide (interfaz "pull"): no se guarda
 *          el array de tokens, que en el Next no cabria.
 *
 * Toda la lectura pasa por dos primitivas, mirar() y avanzar() (lexer.c), que
 * son las que saltan los blancos. Token.pos es la posicion BRUTA en
 * Sentencia.texto; posicion_fuente() la convierte en linea y columna.
 *
 * Pendiente (2.4): reconocer palabras clave. Por ahora PROGRAM, DO, IF... son
 * simples NOMBRE. Consecuencia: "PROGRAM HOLA" se lee como UN nombre de 11
 * letras (error). La clasificacion de la sentencia lo resolvera antes de
 * llamar a este escaner.
 */
#ifndef MF_LEXER_H
#define MF_LEXER_H

#include <stdint.h>
#include "source.h"

/* ------------------------------------------------------------------
 * Tabla de cadenas: guarda el texto de nombres, constantes reales y
 * constantes de texto. El token lleva solo un indice (offset) a ella, como
 * symtab/strtab en MyLISP. Los textos IGUALES comparten posicion, asi que
 * dos nombres son el mismo si y solo si tienen el mismo indice.
 *
 * Es una busqueda lineal: simple y didactica. Si pesa, se cambia por un hash.
 * ------------------------------------------------------------------ */

#define TABLA_TAM    1024        /* bytes del pool; se mide en el Next            */
#define TABLA_LLENA  0xFFFFu     /* valor devuelto cuando no cabe                 */

/* El texto de la tabla NO esta en memoria residente: vive en la pagina de datos TABLA_PAGINA (banco.h), desde TABLA_OFF, y se lee por
 * copia (S3: ahorra ~700 B de bss en el Next). Solo queda el contador. pool[0] es la cadena vacia "". */
#define TABLA_PAGINA  1                 /* comparte pagina con las dimensiones (sim.h): estas llegan hasta TABLA_OFF */
#define TABLA_OFF     6144
#define TABLA_BUF_TAM 256               /* buffer de trabajo del lexer (y de los textos largos): un literal CHARACTER cabe, 255 como maximo */
typedef struct {
    uint16_t usado;              /* bytes ocupados; pool[0] es la cadena vacia ""  */
} TablaCadenas;

/* La tabla es UNICA y global (decision 0006). */
extern TablaCadenas cadenas;

void        tabla_iniciar(void);
/* Mete (o encuentra) los len primeros caracteres de s. Devuelve su offset,
 * 0 si len == 0, o TABLA_LLENA. */
uint16_t    tabla_internar(const char *s, uint16_t len);
/* Devuelve el texto en memoria RESIDENTE. Los de hasta 11 caracteres (nombres, palabras) van a una de 4 ranuras que se reutilizan en
 * rotacion: el puntero vale hasta 3 llamadas mas a tabla_texto. Los largos (literales, reales) usan UN buffer comun que se pisa en cada
 * llamada y en cada token del lexer: usarlo en el acto. */
const char *tabla_texto(uint16_t offset);

/* ------------------------------------------------------------------
 * Tokens
 * ------------------------------------------------------------------ */

#define NOMBRE_MAX 8    /* extension: el estandar dice 6 */

typedef enum {
    TOK_FIN,            /* se acabo la sentencia (se repite si se sigue pidiendo) */
    TOK_ERROR,          /* ver Token.valor: es un ErrorLex                        */

    /* Con contenido */
    TOK_NOMBRE,         /* indice -> tabla                                        */
    TOK_ENTERO,         /* valor                                                  */
    TOK_REAL,           /* indice -> tabla (texto; no se convierte a numero)      */
    TOK_DOBLE,          /* idem, con exponente D                                  */
    TOK_CADENA,         /* indice -> tabla (sin comillas; '' ya colapsado a ')    */
    TOK_LOGICO,         /* valor: 1 = .TRUE., 0 = .FALSE.                         */

    /* Aritmeticos y de texto */
    TOK_MAS, TOK_MENOS, TOK_POR, TOK_DIV, TOK_POTENCIA, TOK_CONCAT,

    /* Puntuacion */
    TOK_IGUAL, TOK_PAR_ABRE, TOK_PAR_CIERRA, TOK_COMA, TOK_DOS_PUNTOS,

    /* Con puntos */
    TOK_EQ, TOK_NE, TOK_LT, TOK_LE, TOK_GT, TOK_GE,
    TOK_AND, TOK_OR, TOK_NOT, TOK_EQV, TOK_NEQV
} TipoToken;

typedef struct {
    TipoToken tipo;
    uint16_t  pos;       /* donde empieza, como offset en Sentencia.texto           */
    uint32_t  valor;     /* ENTERO / LOGICO; en un ERROR, el ErrorLex               */
    uint16_t  indice;    /* NOMBRE / REAL / DOBLE / CADENA: offset en la tabla      */
} Token;

/* ------------------------------------------------------------------
 * Errores lexicos
 * ------------------------------------------------------------------ */

typedef enum {
    LEX_CARACTER_ILEGAL,         /* un caracter que no pertenece al lenguaje      */
    LEX_CADENA_SIN_CERRAR,
    LEX_NOMBRE_LARGO,            /* mas de NOMBRE_MAX caracteres                  */
    LEX_ENTERO_GRANDE,           /* no cabe en 32 bits con signo (> 2147483647)   */
    LEX_REAL_MAL_FORMADO,        /* exponente sin digitos: 1E, 2.5E+                */
    LEX_OPERADOR_DESCONOCIDO,    /* .FOO.                                         */
    LEX_TABLA_LLENA
} ErrorLex;

/* Mensaje para el usuario, en ingles (convencion del proyecto). */
const char *lex_mensaje(ErrorLex e);

/* ------------------------------------------------------------------
 * El escaner
 * ------------------------------------------------------------------ */

typedef struct {
    uint16_t      longitud;
    uint16_t      pos;          /* posicion BRUTA del proximo caracter por leer;
                                   se puede mover desde fuera (2.4 lo usara tras
                                   reconocer la palabra clave)                    */
    uint8_t       solo_entero;  /* 1 = el parser espera un ENTERO (la longitud de CHARACTER*3): no se
                                   lee parte decimal ni exponente, porque "3 E" (con el blanco ya
                                   ignorado) es "3" y el nombre E, no una constante real mal formada */
} Lexer;

/* El escaner es UNICO y global: el compilador procesa una sentencia a la vez.
 * Una variable global se direcciona con 3 bytes de codigo en el Z80; un puntero
 * pasado por parametro cuesta ~10 por cada acceso (ver cap02 §7). */
extern Lexer lexer;

/* Lectura por POSICION BRUTA (indice en Sentencia.texto), sin mover lexer.pos.
 * Las usa tambien la clasificacion de sentencias (clasif.c), que necesita
 * recorrer el texto saltando blancos pero sin consumir tokens.
 *   lex_saltar_blancos: primera posicion >= p que no es blanco (o longitud).
 *   lex_car:            el caracter en p en MAYUSCULAS, o '\0' si p >= longitud. */
uint16_t lex_saltar_blancos(uint16_t p);
char     lex_car(uint16_t p);

/* Prepara el escaner sobre la sentencia global (source.h) ya leida. */
void lex_iniciar(void);

/* Rellena t con el siguiente token. Tras un TOK_ERROR el escaner ya ha
 * avanzado, asi que se puede seguir pidiendo tokens (no se queda atascado). */
void lex_siguiente(Token *t);

#endif
