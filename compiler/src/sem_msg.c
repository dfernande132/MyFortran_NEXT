/* sem_msg.c - Texto de los errores del semantico (ErrorSem). Aparte de sem.c para poder ponerlo en otro banco en el Next
 * (banco 47, junto a los mensajes de expr/stmt/unid): sem.c llena el banco 45 y el texto son ~3 KB. */
#include "sem.h"

const char *sem_mensaje(ErrorSem e)
{
    switch (e) {
    case SEM_OK:                return "no error";
    case SEM_NUMERICO_ESPERADO: return "numeric operand expected";
    case SEM_LOGICO_ESPERADO:   return "logical operand expected";
    case SEM_TEXTO_ESPERADO:    return "character operand expected";
    case SEM_ENTERO_ESPERADO:   return "INTEGER expression expected";
    case SEM_INCOMPATIBLES:     return "incompatible types";
    case SEM_ARRAY_SIN_INDICES: return "array name needs subscripts";
    case SEM_NO_ES_ARRAY:       return "name is not an array";
    case SEM_NUM_INDICES:       return "wrong number of subscripts";
    case SEM_RANGO_NO_PERMITIDO:return "range not allowed here";
    case SEM_FUNCION:           return "function references are not supported yet";
    case SEM_TABLA_LLENA:       return "too many names";
    case SEM_YA_DEFINIDO:       return "name already defined";
    case SEM_DIMENSION:         return "array bounds must be integer constants, low <= high";
    case SEM_DIMENSIONES:       return "only 1 or 2 dimensions are supported";
    case SEM_LONGITUD:          return "invalid CHARACTER length";
    case SEM_ENTERO_GRANDE:     return "INTEGER constant out of range";
    case SEM_SUBCADENA:         return "substring needs a CHARACTER variable";
    case SEM_VARIABLE_DO:       return "DO variable must be a numeric scalar";
    case SEM_MUCHAS_DIMS:       return "too many arrays";
    case SEM_ETIQUETA_DUPLICADA:return "duplicate statement label";
    case SEM_ETIQUETA_INDEFINIDA:return "undefined statement label";
    case SEM_ETIQUETA_EN_DECL:  return "label not allowed on this statement";
    case SEM_MUCHAS_ETIQUETAS:  return "too many labels";
    case SEM_BLOQUES_PROFUNDOS: return "blocks nested too deeply";
    case SEM_SIN_IF:            return "ELSE, ELSE IF or END IF without IF";
    case SEM_ELSE_REPETIDO:     return "ELSE already seen in this IF block";
    case SEM_IF_SIN_CERRAR:     return "IF block is not closed";
    case SEM_DO_SIN_CERRAR:     return "DO loop is not terminated";
    case SEM_DO_ETIQUETA_ATRAS: return "DO terminal label must come after the DO";
    case SEM_DO_TERMINAL:       return "invalid DO terminal statement";
    case SEM_ANIDAMIENTO:       return "DO and IF blocks are not properly nested";
    case SEM_FALTA_END:         return "END statement missing";
    case SEM_INTR_ARGS:         return "wrong number of arguments";
    case SEM_INTR_TIPO:         return "invalid argument type";
    case SEM_DECL_TARDE:        return "declaration must precede executable statements";
    case SEM_YA_DIMENSIONADO:   return "array already dimensioned";
    case SEM_DIM_SIN_LIMITES:   return "DIMENSION needs array bounds";
    case SEM_NOMBRE_SOLO:       return "a plain name is expected";
    case SEM_PROC_ARG:          return "procedures as arguments are not supported yet";
    case SEM_NO_INTRINSECA:     return "not an intrinsic function";
    case SEM_PARAM_CTE:         return "PARAMETER value must be a constant expression";
    case SEM_PARAM_ASIGNA:      return "cannot modify a PARAMETER constant";
    case SEM_DATA_DEST:         return "invalid DATA variable";
    case SEM_DATA_VALOR:        return "DATA value must be a constant";
    case SEM_DATA_TIPO:         return "DATA value type does not match the variable";
    case SEM_DATA_CUENTA:       return "number of DATA values does not match the variables";
    case SEM_DATA_REPET:        return "invalid repeat count";
    case SEM_LECTURA_ITEM:      return "READ item must be a variable or an array element";
    case SEM_GEN_NO_SOPORTADO:  return "not supported yet (e.g. DOUBLE PRECISION)";
    case SEM_GEN_TAC_LLENO:     return "program too large (code memory full)";
    case SEM_GEN_GOTO_CALC:     return "too many labels in computed GOTO";
    case SEM_DO_PASO_CERO:      return "DO step must not be zero";
    case SEM_DESPUES_END:       return "statement after END (only one program unit is supported)";
    case SEM_REAL_DESBORDA:     return "real constant too large";
    }
    return "unknown error";
}
