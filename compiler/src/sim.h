/* sim.h - Tabla de simbolos de UNA unidad de programa (capitulo 5).
 *
 * Vive en una pagina de datos (platform/banco.h): 512 registros de 16 B son
 * exactamente los 8 KB de una pagina, asi que ninguna busqueda cruza paginas
 * (decision 0009). En el residente solo quedan las cabeceras del hash
 * (SIM_CUBETAS x 2 B) y el contador.
 *
 * Un simbolo se identifica por su "id": posicion + 1 (0 = no existe). El
 * registro se LEE y se ESCRIBE por copia: nunca se guarda un puntero a la
 * pagina (banco.h).
 */
#ifndef MF_SIM_H
#define MF_SIM_H

#include <stdint.h>
#include "lexer.h"      /* NOMBRE_MAX */

#define SIM_MAX      512    /* simbolos del programa (todas las unidades): 512 x 16 B = 1 pagina */
#define SIM_CUBETAS  64     /* cabeceras del hash (potencia de 2)         */
#define SIM_PAGINA   0      /* pagina logica de banco.h                   */
#define SIM_PAGINA_DIM 1    /* dimensiones de los arrays                  */

/* Tipo de dato (tambien es Nodo.tdato). 0 = sin tipo / error. */
typedef enum {
    TD_NINGUNO = 0, TD_INTEGER, TD_REAL, TD_DOUBLE, TD_LOGICAL, TD_CHARACTER
} TipoDato;

/* Clase de simbolo. */
typedef enum {
    SIM_VARIABLE = 0, SIM_ARRAY, SIM_FUNCION
} ClaseSim;

/* Banderas. */
#define SIMF_TIPO_EXPLICITO  0x01   /* el tipo se declaro (no es implicito)   */
#define SIMF_USADO           0x02   /* aparece en alguna expresion            */
#define SIMF_ASIGNADO        0x04   /* aparece como destino de una asignacion */
#define SIMF_FORMAL          0x20   /* argumento formal de la unidad (decision 0014)             */
#define SIMF_PROC            0x80   /* declarado EXTERNAL o INTRINSIC (no es una variable)    */
#define SIMF_LONG_ASUM       0x40   /* formal CHARACTER*(*): longitud supuesta (la del argumento real)        */
#define SIMF_NDIM_SHIFT      3      /* bits 3-4: numero de dimensiones (0-2)  */
#define SIMF_NDIM_MASK       0x18
#define SIM_NDIM(s)          ((uint8_t)(((s)->flags & SIMF_NDIM_MASK) >> SIMF_NDIM_SHIFT))

typedef struct {
    char     nombre[NOMBRE_MAX];  /* en mayusculas, relleno con '\0'          */
    uint8_t  tc;                  /* tipo (bits 0-3) | clase (bits 4-7)       */
    uint8_t  flags;               /* SIMF_*                                   */
    uint16_t sig;                 /* siguiente de la cadena del hash (id)     */
    uint16_t extra;               /* CHARACTER: longitud; array: dimensiones  */
    uint16_t dir;                 /* direccion en el programa (lo asigna el generador) */
} Simbolo;

#define SIM_TIPO(s)   ((uint8_t)((s)->tc & 0x0F))
#define SIM_CLASE(s)  ((uint8_t)((s)->tc >> 4))
#define SIM_TC(t, c)  ((uint8_t)((t) | ((c) << 4)))
/* PARAMETER: variable escalar de solo lectura, marcada con los dos bits de dimensiones a 1 (un escalar no los usa). Si es INTEGER,
 * su valor queda en extra para usarlo en constantes (limites de DIMENSION, longitudes). */
#define SIM_ES_PARAM(s)  (SIM_CLASE(s) == SIM_VARIABLE && ((s)->flags & SIMF_NDIM_MASK) == SIMF_NDIM_MASK)

/* Dimensiones de un array (hasta 2). Se guardan en SIM_PAGINA_DIM y el
 * Simbolo.extra del array es su indice. len: longitud de CHARACTER (1 si no). */
typedef struct {
    int16_t  inf[2];      /* limites inferiores                                */
    int16_t  sup[2];      /* limites superiores                                */
    uint16_t len;         /* longitud de cada elemento CHARACTER               */
    uint16_t svar;        /* formal ajustable: simbolo (INTEGER formal) cuyo valor es el limite superior de la 1.a dimension; 0 = constante */
    uint8_t  aj;          /* DIM_ASUMIDO: la ultima dimension es "*" (tamano supuesto) */
    uint8_t  pad;
} Dim;
#define DIM_ASUMIDO   0x01   /* ultima dimension "*" */
#define DIM_VARIABLE  0x02   /* algun limite superior es un INTEGER formal */

/* Guarda las dimensiones y devuelve su indice (o 0xFFFF si no caben). */
uint16_t sim_dim_nueva(const Dim *d);
void     sim_dim_leer(uint16_t indice, Dim *d);

/* Vacia la tabla (inicio de cada compilacion). */
void     sim_iniciar(void);

/* Inicio de cada unidad de programa: los nombres anteriores dejan de ser visibles, pero la tabla es UNA para todo el
 * programa (los id son globales: VAR de un operando TAC = id), con SIM_MAX simbolos en total. */
void     sim_unidad(void);

/* Numero de simbolos dados de alta. */
uint16_t sim_cuenta(void);

/* id del simbolo con ese nombre, o 0. */
uint16_t sim_buscar(const char *nombre);

/* Da de alta un simbolo NUEVO. Devuelve su id, o 0 si la tabla esta llena o el
 * nombre ya existe o es mas largo que NOMBRE_MAX. */
uint16_t sim_alta(const char *nombre, uint8_t tipo, uint8_t clase);

void     sim_leer(uint16_t id, Simbolo *s);
void     sim_escribir(uint16_t id, const Simbolo *s);

#endif
