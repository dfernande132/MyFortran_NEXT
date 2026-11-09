/* sem.h - Analisis semantico, sentencia a sentencia (capitulo 5, decision 0009).
 *
 * Uso (por cada sentencia ya analizada por stmt_analizar):
 *
 *     sem_iniciar();                    // una vez por unidad de programa
 *     ...
 *     stmt_analizar();
 *     if (stm.err == STM_OK) sem_sentencia();
 *     // sem_sentencia recorre las listas con stmt_elemento(): despues hay que
 *     // volver a mirar stm.err (error de sintaxis tardio, ver stmt.h)
 *     // IF logico: stmt_analizar_interna(); sem_sentencia();
 *
 * Los errores SE ACUMULAN: tras uno se sigue con la sentencia siguiente. Van a
 * la tabla comun `mf` (mf_api.h: linea y columna ya calculadas, porque el texto
 * de la sentencia se pierde al leer la siguiente).
 *
 * Rellena Nodo.tdato en cada nodo de expresion que tipa.
 */
#ifndef MF_SEM_H
#define MF_SEM_H

#include <stdint.h>
#include "sim.h"
#include "stmt.h"

#define SEM_CHAR_MAX    255      /* longitud maxima de CHARACTER */
#define SEM_BLOQUES_MAX 16       /* IF/DO anidados a la vez (pagina de datos 2: 16 x 12 B desde SEM_BLQ_OFF) */
#define SEM_BLQ_OFF     3072     /* pila de bloques del semantico: tras las etiquetas (256 x 12 B) y antes de la pila de bloques del generador (gen.h) */
#define SEM_DATA_PAGINA 10       /* DATA: nombres del grupo en curso. La pagina 10 es de las tablas del backend, que no corre hasta acabar el frontal */
#define SEM_DATA_MAX    128      /* nombres por grupo de DATA (6 B cada uno) */
#define SEM_ETIQUETAS_MAX 256    /* etiquetas por unidad (pagina de datos 2)       */
#define SIM_PAGINA_ETQ  2

typedef enum {
    SEM_OK,
    SEM_NUMERICO_ESPERADO,    /* "numeric operand expected"                  */
    SEM_LOGICO_ESPERADO,      /* "logical operand expected"                  */
    SEM_TEXTO_ESPERADO,       /* "character operand expected"                */
    SEM_ENTERO_ESPERADO,      /* "INTEGER expression expected"               */
    SEM_INCOMPATIBLES,        /* "incompatible types"                        */
    SEM_ARRAY_SIN_INDICES,    /* "array name needs subscripts"               */
    SEM_NO_ES_ARRAY,          /* "name is not an array"                      */
    SEM_NUM_INDICES,          /* "wrong number of subscripts"                */
    SEM_RANGO_NO_PERMITIDO,   /* "range not allowed here"                    */
    SEM_FUNCION,              /* "function references are not supported yet" */
    SEM_TABLA_LLENA,          /* "too many names"                            */
    SEM_YA_DEFINIDO,          /* "name already defined"                      */
    SEM_DIMENSION,            /* "array bounds must be integer constants, low <= high" */
    SEM_DIMENSIONES,          /* "only 1 or 2 dimensions are supported"      */
    SEM_LONGITUD,             /* "invalid CHARACTER length"                  */
    SEM_ENTERO_GRANDE,        /* "INTEGER constant out of range"             */
    SEM_SUBCADENA,            /* "substring needs a CHARACTER variable"      */
    SEM_VARIABLE_DO,          /* "DO variable must be a numeric scalar"      */
    SEM_MUCHAS_DIMS,          /* "too many arrays"                           */
    SEM_ETIQUETA_DUPLICADA,   /* "duplicate statement label"                 */
    SEM_ETIQUETA_INDEFINIDA,  /* "undefined statement label"                 */
    SEM_ETIQUETA_EN_DECL,     /* "label not allowed on this statement"       */
    SEM_MUCHAS_ETIQUETAS,     /* "too many labels"                           */
    SEM_BLOQUES_PROFUNDOS,    /* "blocks nested too deeply"                  */
    SEM_SIN_IF,               /* "ELSE, ELSE IF or END IF without IF"        */
    SEM_ELSE_REPETIDO,        /* "ELSE already seen in this IF block"        */
    SEM_IF_SIN_CERRAR,        /* "IF block is not closed"                    */
    SEM_DO_SIN_CERRAR,        /* "DO loop is not terminated"                 */
    SEM_DO_ETIQUETA_ATRAS,    /* "DO terminal label must come after the DO"  */
    SEM_DO_TERMINAL,          /* "invalid DO terminal statement"             */
    SEM_ANIDAMIENTO,          /* "DO and IF blocks are not properly nested"  */
    SEM_FALTA_END,            /* "END statement missing"                     */
    SEM_INTR_ARGS,            /* "wrong number of arguments"                 */
    SEM_INTR_TIPO,            /* "invalid argument type"                     */
    SEM_DECL_TARDE,           /* "DIMENSION/EXTERNAL/INTRINSIC must precede executable statements" */
    SEM_YA_DIMENSIONADO,      /* "array already dimensioned"                 */
    SEM_DIM_SIN_LIMITES,      /* "DIMENSION needs array bounds"              */
    SEM_NOMBRE_SOLO,          /* "a plain name is expected"                  */
    SEM_PROC_ARG,             /* "procedures as arguments are not supported yet" */
    SEM_NO_INTRINSECA,        /* "not an intrinsic function"                 */
    SEM_PARAM_CTE,            /* "PARAMETER value must be a constant expression" */
    SEM_PARAM_ASIGNA,         /* "cannot modify a PARAMETER constant"        */
    SEM_DATA_DEST,            /* "invalid DATA variable"                     */
    SEM_DATA_VALOR,           /* "DATA value must be a constant"             */
    SEM_DATA_TIPO,            /* "DATA value type does not match the variable" */
    SEM_DATA_CUENTA,          /* "number of DATA values does not match the variables" */
    SEM_DATA_REPET,           /* "invalid repeat count"                      */
    SEM_LECTURA_ITEM,         /* "READ item must be a variable or an array element" */
    SEM_GEN_NO_SOPORTADO,     /* "not supported yet (e.g. DOUBLE PRECISION)" */
    SEM_GEN_TAC_LLENO,        /* "program too large (intermediate code memory full)" */
    SEM_GEN_GOTO_CALC,        /* "too many labels in computed GOTO"          */
    SEM_DO_PASO_CERO,         /* "DO step must not be zero"                  */
    SEM_DESPUES_END,          /* "statements after END: only one program unit is supported" */
    SEM_REAL_DESBORDA         /* "real constant too large"                   */
} ErrorSem;

typedef struct {
    uint8_t     fin;          /* 1 = ya se vio el END de la unidad en curso (o de la ultima)       */
    uint8_t     abierta;      /* 1 = hay una unidad en curso (entre su primera sentencia y su END)  */
    uint8_t     unidad;       /* numero de la unidad en curso (unid.h; 0 = ninguna)                 */
    uint8_t     clase;        /* UK_PROGRAMA / UK_SUBRUTINA / UK_FUNCION de la unidad en curso       */
    uint16_t    nsent;        /* sentencias de la unidad en curso (la cabecera debe ser la primera) */
    uint8_t     ejec;         /* 1 = ya hubo una sentencia ejecutable (ya no caben funciones de sentencia) */
    uint8_t     sf_n;         /* funciones de sentencia definidas en la unidad (registros en la pagina GEN_PAGINA) */
    uint16_t    sf_uso;       /* bytes ocupados por sus registros desde GEN_SF_OFF                    */
    uint8_t     sf_activo;    /* 1 mientras se tipa el cuerpo de una definicion de funcion de sentencia */
} Sem;

extern Sem sem;

const char *sem_mensaje(ErrorSem e);

/* Nueva compilacion: tablas vacias y sin errores. Cada unidad de programa se abre sola con su primera sentencia. */
void sem_iniciar(void);

/* Fin del fichero: si la unidad no se cerro con END, lo anota (linea = la
 * ultima del fichero). */
void sem_fin_fichero(uint16_t linea);

/* Etiquetas de usuario para el generador de codigo (gen.c): el operando ETQ del
 * TAC es el INDICE (1..SEM_ETIQUETAS_MAX) de la etiqueta en la tabla de la unidad,
 * no su numero (que llega a 99999 y no cabe en 13 bits). */
uint16_t sem_etq_indice(uint32_t etiq);       /* la crea si no esta; 0 = tabla llena */
uint32_t sem_etq_valor(uint16_t indice);      /* numero de etiqueta de un indice     */

/* Analiza la sentencia actual (stm). No hace nada si stm.err != STM_OK. */
void sem_sentencia(void);

/* Sentencia con error de sintaxis: si es la cabecera de una unidad (SUBROUTINE, PROGRAM) abre la unidad igualmente (sin
 * nombre), para que el cuerpo no se tome por un programa principal aparte y llene de errores en cascada. */
void sem_sentencia_erronea(void);

#endif
