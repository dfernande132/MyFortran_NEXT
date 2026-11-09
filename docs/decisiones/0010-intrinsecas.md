# 0010 - Funciones intrinsecas (ampliacion del capitulo 5)

Fecha: 2026-10-30. Decidido con el autor (opcion "Funciones intrinsecas"; sin diseno de codegen todavia).

## Alcance
`F(...)` donde F no es una variable ni un array declarado y esta en la tabla de intrinsecas deja de dar
`SEM_FUNCION`: el semantico comprueba numero y tipo de los argumentos y deduce el tipo del resultado
(`Nodo.tdato`). Las funciones del usuario y las de sentencia siguen dando `SEM_FUNCION` (cap. de subprogramas).

## Decisiones
1. **Tabla fija en `compiler/src/intr.{h,c}`**: 76 entradas `{nombre[8], nmin, nmax, atipo, rtipo}` (12 B cada una,
   RODATA del banco del semantico; el puntero que devuelve `intr_buscar` no sale de ese banco). Genericas (ABS, MOD,
   SIGN, DIM, MAX, MIN, AINT, ANINT, SQRT, EXP, LOG, LOG10, SIN..TANH, ATAN2), conversiones (INT, REAL, DBLE, NINT,
   FLOAT, IFIX, SNGL, IDINT, IDNINT), especificas INTEGER/REAL/DOUBLE (IABS, MAX0, AMAX1, ALOG, DSQRT... DPROD) y
   CHARACTER (LEN, ICHAR, INDEX, CHAR, LGE/LGT/LLE/LLT). Sin COMPLEX (el compilador no tiene ese tipo).
2. **Reglas del estandar, estrictas**: las genericas numericas exigen argumentos del MISMO tipo (`MOD(1, 2.0)` es
   error); las matematicas (SQRT, SIN...) exigen REAL o DOUBLE (`SQRT(I)` es error); las especificas fijan el tipo.
   `MAX/MIN` admiten 2..20 argumentos.
3. **Resolucion de nombres**: se busca primero en la tabla de simbolos; solo si el nombre NO esta se mira en la tabla de
   intrinsecas. Asi `MAX = 3` seguido de `MAX(1,2)` da `SEM_NO_ES_ARRAY` (el nombre ya es una variable), y el tipo
   implicito del nombre de la funcion NO cuenta (`MOD` empieza por M pero puede devolver REAL).
4. **No se crea simbolo** para una intrinseca, y `SQRT(X) = 1` da `SEM_NO_ES_ARRAY`.
5. **Errores nuevos**: `SEM_INTR_ARGS` "wrong number of arguments" (en el nombre de la funcion) y `SEM_INTR_TIPO`
   "invalid argument type" (en el argumento). Un argumento ya erroneo no encadena un segundo error.
6. **Pila**: la comprobacion es una funcion hoja (`intr_comprobar`) y el recorrido de los argumentos va en
   `tipar_funcion`, que sustituye a `indice` en la cadena `tipar -> tipar_ref -> .. -> tipar` (mismo numero de marcos
   por nivel). MEDIDO en el Next (`i1_integrar`, CSpect, 2026-10-30): 11 niveles `ABS(` = 928 B de pila, IGUAL que
   11 niveles `V(` (928 B); el peor caso global medido sigue siendo 961 B (22 parentesis); programa con intrinsecas y un error
   (`I = SQRT(I)` -> `2:16 [34] invalid argument type`) 275 B. Banco 45: 8.962 -> 11.011 B (+2.049 B; quedan 5.373 B libres;
   `intr_comprobar` 639 B, `tipar_funcion` 230 B y la tabla ~912 B, cifras del `.map` y derivadas). El residente del compilador
   NO crece: el `.nex` pasa de 14.598 a 15.043 B solo por el driver de medida (`i1_main.c`, +283 B de codigo medido).

## Limitaciones conocidas
- No hay sentencia `INTRINSIC` ni `EXTERNAL`. Una declaracion de tipo sobre el nombre (`REAL SQRT`) lo vuelve variable.
- Las especificas con nombre propio de REAL solo estan para los casos con diferencias de nombre (ALOG, AMOD, AMAX1...);
  SQRT/EXP/SIN... son genericas y sirven tambien para REAL.
- Longitud del resultado de CHAR: el tipo es CHARACTER; la longitud (1) no se lleva en `tdato`.
- Sin plegado de constantes (`SQRT(4.0)` no se evalua).

## Pruebas (PC, ASan/UBSan)
`test_cap05.c` (`test_intrinsecas`, `test_tabla_intrinsecas`): usos correctos, tipo del resultado, cada error, y por cada
entrada de la tabla: nombre unico/mayusculas/min<=max, llamada correcta con su tipo de resultado, uno de menos y uno de mas.
Mutaciones: 9 cambios en `intr_comprobar`/`tipar_ref`/`tipar_funcion` detectados (uno sobrevivio al principio:
la regla `NUMX`; se anadieron `INT('A')` y `DBLE(.TRUE.)`).
