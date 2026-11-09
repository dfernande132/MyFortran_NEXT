/* unid.h - Directorio de UNIDADES de programa y registro de llamadas (decision 0014).
 *
 * Un programa es una coleccion de unidades (programa principal, SUBROUTINE, FUNCTION...) en un solo fichero.
 * Mientras se compila cada unidad se anota aqui lo que las demas necesitan saber: nombre, clase, formales y
 * donde estan; cada CALL queda registrado (quien llama, a quien, con que argumentos y donde) y se comprueba al
 * final del fichero, cuando ya se conocen todas (se puede llamar a una unidad que se define mas abajo).
 *
 * Vive en la pagina de datos UNI_PAGINA (banco.h):
 *     [0, UNI_DIR_FIN)      Unidad x UNI_MAX
 *     [UNI_ADIR_OFF, ...)   direccion de cada unidad en el Z80 (la rellena el backend)
 *     [UNI_ADY_OFF, ...)    matriz de llamadas (UNI_MAX filas de 8 bytes: bit v de la fila u = "u llama a v")
 *     [UNI_REG_OFF, 8192)   registros de llamadas
 * Los numeros de unidad van de 1 a UNI_MAX (0 = ninguna). Por copia, como la tabla de simbolos.
 */
#ifndef MF_UNID_H
#define MF_UNID_H

#include <stdint.h>
#include "lexer.h"      /* NOMBRE_MAX */

#define UNI_PAGINA     13
#define UNI_MAX        63
#define UNI_ARGS_MAX   15

#define UK_NADA        0        /* solo se ha llamado: aun no se ha visto su definicion */
#define UK_PROGRAMA    1
#define UK_SUBRUTINA   2
#define UK_FUNCION     3

#define UF_LLAMADA     0x01     /* alguien la llama (o referencia)                            */
#define UF_FUNREF      0x02     /* alguien la referencia como funcion (para el mensaje de "indefinida") */

typedef struct {
    char     nombre[NOMBRE_MAX];  /* '\0' = sin nombre (programa principal sin PROGRAM)                */
    uint8_t  clase;               /* UK_*                                                              */
    uint8_t  flags;
    uint8_t  nformales;
    uint8_t  tipo;                /* FUNCTION: tipo del resultado (TipoDato)                           */
    uint16_t sim_ini;             /* id del primer simbolo de la unidad: los formales son los primeros (sim_ini..)  */
    uint16_t sim_fin;             /* id del ultimo (se fija al cerrar la unidad)                        */
    uint16_t sim_res;             /* FUNCTION: id de la variable resultado (0 = no es funcion)         */
    uint16_t linea, col;          /* cabecera de la definicion, o primera llamada si aun no esta definida */
    uint8_t  ftipo[UNI_ARGS_MAX]; /* tipo de cada formal (se rellena al cerrar la unidad)              */
    uint8_t  flen;                /* FUNCTION CHARACTER: longitud del resultado (se rellena al cerrar la unidad) */
    uint8_t  pad[2];
} Unidad;                         /* 40 bytes */

#define UNI_DIR_FIN    (UNI_MAX * 40)           /* 2520 */
#define UNI_ADIR_OFF   2560
#define UNI_ADY_OFF    2688                      /* 63 x 8 = 504 -> 3192 */
#define UNI_REG_OFF    3200

/* Registro de una llamada (UNI_REG_CAB bytes + un tipo por argumento). */
#define UNI_REG_CAB    7
/* kind de un argumento: bits 0-3 TipoDato, bit 4 = nombre de array (entero), bit 5 = procedimiento (S4),
 * bit 6 = elemento de array (se pasa por valor a un formal escalar y por direccion a un formal array) */
#define UA_ARRAY       0x10
#define UA_PROC        0x20
#define UA_ELEM        0x40

void    uni_iniciar(void);
uint8_t uni_cuenta(void);
uint8_t uni_buscar(const char *nombre);         /* 0 = no existe */
uint8_t uni_alta(const char *nombre, uint16_t linea, uint16_t col);   /* 0 = tabla llena (o nombre repetido) */
void    uni_leer(uint8_t u, Unidad *x);
void    uni_escribir(uint8_t u, const Unidad *x);

/* Direccion de la unidad en el programa Z80 (backend). */
uint16_t uni_dir_get(uint8_t u);
void     uni_dir_set(uint8_t u, uint16_t d);

/* Anota una llamada: llamador -> llamado con n argumentos de esos tipos (kind). ftipo = 0 para CALL; para una referencia
 * a funcion, el tipo (TipoDato) que el llamador espera del resultado y, si es CHARACTER, su longitud (flen), que se anota tras
 * los tipos de los argumentos. lens[k] = dato extra del argumento k si es CHARACTER (su longitud; en un array CHARACTER, la de cada
 * elemento) o un array entero (sus elementos, saturado a 255 = no se sabe): se anotan compactados tras flen. 0 = no cabe. */
uint8_t uni_llamada(uint8_t llamador, uint8_t llamado, uint16_t linea, uint8_t col, uint8_t n, const uint8_t *kind,
                    const uint8_t *lens, uint8_t ftipo, uint8_t flen);

/* Codigos de error (mf_api.h: MF_COD_UNI + e). */
typedef enum {
    UNI_OK,
    UNI_SIN_PRINCIPAL,        /* "no main program"                                         */
    UNI_VARIOS_PRINCIPALES,   /* "more than one main program"                              */
    UNI_YA_DEFINIDA,          /* "program unit already defined"                            */
    UNI_CABECERA,             /* "PROGRAM, SUBROUTINE or FUNCTION must be the first statement" */
    UNI_TABLA_LLENA,          /* "too many program units"                                  */
    UNI_INDEFINIDA,           /* "undefined subroutine"                                    */
    UNI_NUM_ARGS,             /* "argument count mismatch"                                 */
    UNI_TIPO_ARG,             /* "argument type mismatch"                                  */
    UNI_RECURSIVA,            /* "recursive call is not allowed in FORTRAN 77"             */
    UNI_MUCHOS_ARGS,          /* "too many arguments"                                      */
    UNI_NO_SUBRUTINA,         /* "name is not a subroutine"                                */
    UNI_RETURN_PRINCIPAL,     /* "RETURN is not allowed in the main program"               */
    UNI_ARG_DUPLICADO,        /* "duplicate dummy argument"                                */
    UNI_MUCHAS_LLAMADAS,      /* "too many calls"                                          */
    UNI_NO_FUNCION,           /* "name is not a function"                                  */
    UNI_TIPO_FUNCION,         /* "function result type mismatch"                           */
    UNI_FUNCION_INDEFINIDA,   /* "undefined function"                                      */
    UNI_LONG_FUNCION,         /* "function result length mismatch" (CHARACTER)             */
    UNI_FSENT_DUPLICADA,      /* "statement function already defined"                      */
    UNI_FSENT_GRANDE,         /* "statement function too large"                            */
    UNI_LONG_ARG,             /* "argument shorter than dummy argument" (CHARACTER)        */
    UNI_TAM_ARRAY,            /* "array argument smaller than dummy array"                 */
    UNI_LONG_ARRAY            /* "array element length mismatch" (CHARACTER)               */
} ErrorUni;

const char *uni_mensaje(ErrorUni e);

/* Fin del fichero: comprueba todas las llamadas y que haya UN programa principal; anota los errores. */
void uni_comprobar(void);

#endif
