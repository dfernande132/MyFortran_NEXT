/* clasif.h - Clasificacion de sentencias Fortran 77 (2.4).
 *
 * Decide QUE CLASE de sentencia es antes de escanear sus tokens, y dice desde
 * donde debe empezar a leer el escaner. Hace falta porque las palabras clave
 * de F77 no estan reservadas y los blancos no cuentan:
 *
 *      DO10I=1,10    es un bucle DO          DO10I=1.10   es una asignacion
 *      IF(3)=1       asigna a un array IF    IF(X)10,20,30  IF aritmetico
 *
 * Es una pasada de SOLO LECTURA sobre el texto (no consume tokens, no
 * retrocede). Se aplican, en este orden, reglas deterministas:
 *
 *   1. ASIGNACION: un nombre, cero o mas grupos de parentesis balanceados, y
 *      un '='.  (Si ademas parece "DO etiqueta variable =" y hay una coma fuera
 *      de parentesis tras el '=', es un DO, no una asignacion.)
 *   2. PALABRA CLAVE: la mas larga de la tabla que coincida con el principio
 *      (para distinguir END de ENDIF, o DO de DOUBLEPRECISION).
 *   3. IF: segun lo que siga al parentesis de cierre: THEN y fin -> de bloque;
 *      un digito -> aritmetico; cualquier otra cosa -> logico (la sentencia
 *      que sigue se clasifica con otra llamada).
 *
 * La tabla de palabras clave crece con los capitulos: una sentencia valida
 * que aun no soportamos sale como SENT_DESCONOCIDA.
 *
 * "REAL FUNCTION F(X)" es SENT_FUNCTION con ftipo = SENT_REAL (decision 0014); solo lo es si tras FUNCTION
 * hay un nombre y un '(' ("REAL FUNCTIONX" sigue siendo la declaracion de una variable).
 */
#ifndef MF_CLASIF_H
#define MF_CLASIF_H

#include <stdint.h>
#include "lexer.h"

typedef enum {
    SENT_DESCONOCIDA,        /* no se reconoce (o aun no esta soportada)       */
    SENT_VACIA,              /* no hay nada (solo etiqueta, o blancos)         */
    SENT_ASIGNACION,         /* X=..., A(I)=..., F(X)=... (esta ultima puede ser
                                una sentencia-funcion: lo decide el semantico) */
    SENT_PROGRAM, SENT_END,
    SENT_SUBROUTINE,         /* SUBROUTINE nombre [( formales )]                */
    SENT_CALL,               /* CALL nombre [( argumentos )]                    */
    SENT_RETURN,             /* RETURN                                          */
    SENT_INTEGER, SENT_REAL, SENT_DOUBLEPRECISION, SENT_LOGICAL, SENT_CHARACTER,
    SENT_PRINT, SENT_CONTINUE, SENT_GOTO, SENT_STOP,
    SENT_DO,                 /* etiqueta en Clasificacion.etiqueta              */
    SENT_IF_BLOQUE,          /* IF (cond) THEN                                  */
    SENT_IF_ARITMETICO,      /* IF (expr) l1, l2, l3                            */
    SENT_IF_LOGICO,          /* IF (cond) sentencia                             */
    SENT_ELSEIF, SENT_ELSE, SENT_ENDIF,
    SENT_DIMENSION, SENT_EXTERNAL, SENT_INTRINSIC, SENT_PARAMETER, SENT_DATA, SENT_READ,
    SENT_FUNCTION            /* [tipo] FUNCTION nombre ( formales ); el tipo en Clasificacion.ftipo */
} TipoSentencia;

typedef struct {
    TipoSentencia tipo;
    uint16_t      resto;      /* posicion BRUTA desde la que debe seguir el escaner:
                                 tras la palabra clave, o el principio si es una
                                 asignacion; en un DO, tras la etiqueta (y su
                                 coma opcional), es decir, en la variable;
                                 en un IF, en el '(' */
    uint32_t      etiqueta;   /* SENT_DO: la etiqueta de fin; SENT_FUNCTION CHARACTER: la longitud */
    uint8_t       ftipo;      /* SENT_FUNCTION: SENT_INTEGER/REAL/DOUBLEPRECISION/LOGICAL/CHARACTER, o 0 (tipo implicito) */
    uint16_t      lenpos;     /* SENT_FUNCTION con CHARACTER: posicion del '*' de la longitud (0 = no hay) */
} Clasificacion;

/* Clasifica la sentencia que el escaner tiene por delante (desde lexer.pos).
 * No modifica el escaner. Para el IF logico, tras parsear la condicion se vuelve a
 * llamar con lexer.pos al principio de la sentencia interior. */
void clasificar_sentencia(Clasificacion *c);

#endif
