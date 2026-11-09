# 0014 - Subprogramas (SUBROUTINE, FUNCTION y todo lo que cuelga de ellos)

Fecha: 2026-11-02. Estado: S1 HECHA EN PC (ver "Estado de S1" al final); DECISIONES ACORDADAS con el autor (abajo, incluido que los desajustes de argumentos son ERROR); diseno detallado y etapas PROPUESTOS (se confirman al empezar cada etapa).

## Decisiones del autor
1. **Variables locales ESTATICAS**: cada unidad tiene sus variables en direcciones fijas (como ya hace el programa principal). Sin recursion.
   Conforme a la norma: las variables locales no guardadas quedan indefinidas al volver (15.? / 17.?); conservar el valor es una forma valida de "indefinido".
   `SAVE` no cuesta nada (todo es SAVE). La recursion (directa o indirecta) es ilegal en F77 y se detecta en compilacion (grafo de llamadas): `recursive call`.
2. **Paso por REFERENCIA** (la direccion del argumento real), que es la semantica de F77. Constantes y expresiones pasadas como argumento se copian a un
   temporal (no se puede corromper el pool de constantes si el subprograma asigna al argumento formal).
3. **Alcance: todo F77 en cuanto a subprogramas**, por etapas (S1..S6), cada una con su bateria de pruebas exhaustiva ANTES de pasar a la siguiente.
4. **Una sola fuente con varias unidades** (programa principal en cualquier posicion, subprogramas antes o despues) y un solo `.nex`.
   Compilacion separada / bibliotecas: version posterior (el editor preguntara al detectar subprogramas si se quieren compilar aparte).
   La norma no define bibliotecas ni enlazado: solo dice (2.4.2) que "un programa ejecutable es una coleccion de unidades de programa que consta de
   exactamente un programa principal y cualquier numero, incluso ninguno, de subprogramas y procedimientos externos"; como se empaquetan, compilan y
   enlazan es cosa de cada procesador. Las "bibliotecas" de F77 en la practica son del enlazador (objetos reubicables), no del lenguaje; las unicas
   rutinas "de biblioteca" que define la norma son las intrinsecas.

## Diseno propuesto (para confirmar)
### Llamada
- TAC (ops 39..43 reservadas): `ENTER u`, `PARAM x`, `CALL u`, `RET`, `FUNR d`. Cada formal tiene una **ranura estatica** (2 B) en la unidad llamada;
  el que llama escribe en ellas las direcciones de los reales (no hay pila de datos: la pila del Z80 solo guarda direcciones de retorno).
  Coste por argumento: `LD HL,addr / LD (slot),HL` = 6 B de codigo en el que llama; el acceso a un formal escalar: `LD HL,(slot)` y despues `(HL)`.
- Funcion: el resultado vive en una variable estatica con el nombre de la funcion; `FUNR` la lee tras el `CALL`. `RETURN` salta al epilogo de la unidad.
- Formales de tipo array: la ranura guarda la direccion base; los limites con dimensiones ajustables (`A(N,M)`) se evaluan **en la entrada** y se guardan en
  ranuras ocultas; `A(*)` (tamano supuesto) solo en la ultima dimension. CHARACTER formal: ranura de direccion + ranura oculta de longitud (`CHARACTER*(*)`).
- Argumento real: variable, elemento de array (direccion del elemento), array completo, subcadena, constante/expresion (temporal), nombre de procedimiento
  (direccion de su entrada; se invoca con `JP (HL)` desde una pequena rutina de salto).
- Como todas las unidades se compilan juntas, el compilador **comprueba** numero y tipo de argumentos entre llamada y definicion (la norma lo deja al
  programador; aqui es un diagnostico adicional, ERROR DE COMPILACION por decision del autor: `argument count mismatch`, `argument type mismatch`), salvo cuando el procedimiento es externo/ficticio.
- Posible optimizacion posterior (NO ahora): solapar las locales de unidades que nunca estan activas a la vez (el grafo de llamadas es un DAG).

### Etapas
- **S1** `SUBROUTINE`, `CALL`, `RETURN`, `END` de varias unidades, argumentos escalares (INTEGER, REAL, LOGICAL, CHARACTER), locales estaticas,
  deteccion de recursion, resolucion de nombres entre unidades. Sustituye "una unidad por compilacion".
- **S2** `FUNCTION` (INTEGER, REAL, LOGICAL, CHARACTER*n; tipo en la cabecera o por `IMPLICIT`/tipo implicito), referencias en expresiones
  (hoy `SEM_FUNCION`), funciones de sentencia (`F(X) = expr`), interaccion con intrinsecas (un nombre de usuario no puede ocultar una intrinseca salvo declaracion `EXTERNAL`).
- **S3** arrays como argumentos (dimension explicita, ajustable, `*`), elementos y subcadenas como argumentos, `CHARACTER*(*)`.
- **S4** `EXTERNAL`, `INTRINSIC`, procedimientos como argumentos (formales procedimiento), `ENTRY`, `RETURN e` y `CALL S(*10)` (retorno alternativo).
- **S5** `COMMON` (en blanco y con nombre), `EQUIVALENCE`, `SAVE`, `DATA`, `BLOCK DATA`, `PARAMETER` si aun no estan (a revisar: hoy no estan en el compilador).
- **S6** diagnosticos cruzados (recursion, tipos, numero de argumentos, tipo implicito de funciones), limites de tabla, reparto en bancos y medida en el Next.
  DOUBLE PRECISION sigue fuera (decision 0013): un `DOUBLE PRECISION FUNCTION` da `not supported`.

### Memoria (MEDIDO por Dani tras R4): huecos 43: 1310 B, 44: 2150 B, 45: 1483 B, 46: 7554 B, 47: 10227 B; fin de BSS $B61C
Los bancos 43 (backend) y 44 (generador) no dan para S1..S6 enteros: hace falta repartir (p. ej. la parte del backend de llamadas y COMMON en 46/47 con
trampolines, como se hizo con las tablas REAL). Se medira al cerrar cada etapa; no se promete nada antes.

## Bateria de pruebas (exhaustiva; condicion para cerrar cada etapa)
1. **Diagnosticos**: una prueba por cada regla violada y por cada mensaje nuevo (listado y texto exactos), incluida la posicion del error.
2. **Semantica a mano**: programas con salida esperada escrita a mano (paso por referencia, aliasing `CALL S(A, A)`, modificar el real, constantes protegidas,
   funciones y subrutinas anidadas, retornos multiples, tipos mixtos, arrays 1D/2D con dimensiones ajustables, subcadenas, procedimientos como argumentos...).
3. **Diferencial**: la misma fuente se ejecuta en `tacvm` (PC) y en el emulador Z80 (`z80emu`) con la salida comparada; si hay `gfortran -std=legacy`,
   tambien contra el.
4. **Generador aleatorio de programas** con grafos de llamadas, argumentos de todos los tipos y formas, en `test_cap08.c` (ya existe el de expresiones).
5. **Mutantes** del backend y del generador: cada mutante debe ser matado por alguna prueba (como se hizo en R3/R4).
6. **Limites**: profundidad de llamadas, numero de unidades/argumentos/ranuras, tablas llenas, programa grande en el Next.
7. **En el Next**: un `.f` por etapa con salida esperada (`examples/subprogramas_sN.f` + `.esperado.txt`), contrastada por Dani.

## Estado de S1 (2026-11-02, en PC; SIN construir con z88dk ni medir en el Next)
Hecho: `SUBROUTINE`, `CALL`, `RETURN`, varias unidades por fichero en cualquier orden (principal con o sin `PROGRAM`), argumentos INTEGER/REAL/LOGICAL,
locales estaticas, errores de compilacion (numero y tipo de argumentos, subrutina indefinida/repetida, recursion directa o indirecta por cierre transitivo,
varios principales, sin principal, `RETURN` en el principal, formal repetido, mas de 15 argumentos, tablas llenas). Pruebas: `test_cap09.c` (374: orden de
unidades, argumentos, flujo con cadena de 20 llamadas, diagnosticos con linea/columna/codigo, limites de tabla y un **fuzzer de 300 programas con oraculo
independiente** —modelo en C con referencia verdadera, enteros de 16 bits y REAL de 32; compara oraculo, TAC y Z80—; a mano 1200 programas sin fallos;
una mutacion deliberada del backend la detecta).
Refinamientos respecto al diseno de arriba (a confirmar por el autor):
1. **Escalares: copia de entrada/copia de salida**, no punteros en ranuras (mantiene el direccionamiento directo del backend). Equivale a la referencia
   mientras no haya alias entre un argumento escrito y otro argumento, que la norma prohibe; el fuzzer genera solo programas sin alias. **El alias
   `CALL S(A, A)` con escritura SI daria un resultado distinto al de una referencia verdadera**: se decide en S3 (cuando los arrays obliguen a la
   direccion verdadera) si los escalares pasan tambien a direccion.
2. **Numeracion global** de simbolos, etiquetas y temporales (limites del programa entero, no de cada unidad; ver seccion 8 de la especificacion TAC).
3. **`CHARACTER` escalar como argumento pasa a S3** (junto a `CHARACTER*(*)`); en S1 da `not supported`.

## Estado de S2a (2026-11-02, en PC; SIN construir con z88dk ni medir en el Next)
Hecho: `FUNCTION` con tipo en la cabecera (`INTEGER|REAL|LOGICAL FUNCTION F(...)`) o implicito, `RETURN`, referencias `F(args)` en cualquier expresion
(asignaciones, condiciones, limites de DO, subindices, argumentos de CALL y de otras funciones, `PRINT`, funciones intrinsecas anidadas), funciones sin
argumentos (`F()`: los parentesis son obligatorios, como en la norma). El llamador da su tipo al nombre (declaracion explicita o tipo implicito); la funcion
debe devolver ese tipo (`function result type mismatch` al final del fichero). Nuevos errores: `undefined function`, `name is not a function`,
`function result type mismatch` (`MF_COD_UNI` + 15..17). Un escalar con tipo declarado usado con parentesis (`I(2)`) es una referencia a funcion
(`undefined function` si no existe), ya no `not an array`. Pendiente: S2c funciones de sentencia
(`F(X) = expr`, hoy `not an array`).
Notas: las funciones NO deben hacer E/S si se las llama dentro de la lista de un `PRINT` (la salida se mezclaria); el compilador no lo comprueba. Una funcion
que modifica un argumento que aparece en otro punto de la misma sentencia da un resultado dependiente del orden de evaluacion (izquierda a derecha aqui).
Pruebas: `test_cap09.c` (419): 9 programas a mano con salida escrita, ~35 diagnosticos con linea/columna/codigo y el fuzzer ampliado con funciones puras en
expresiones y `V = F(...)` con funciones que escriben argumentos (2000 programas sin fallos; 7 mutantes del generador/backend: 6 muertos, el 7.o es equivalente).

## Estado de S2b (2026-11-02, en PC; SIN construir con z88dk ni medir en el Next)
Hecho: `CHARACTER*n FUNCTION F(...)`, `CHARACTER FUNCTION F()` (longitud 1), `CHARACTER*(n) FUNCTION F` y `FUNCTION F` + `CHARACTER F*n` dentro de la
funcion. Solo longitudes enteras constantes (`CHARACTER*0` da `invalid length specifier`). El llamador debe declarar el nombre con la **misma longitud**
(`function result length mismatch`, `MF_COD_UNI` + 18, al final del fichero) y tipo CHARACTER (`function result type mismatch` si no). El resultado se
rellena con blancos / se trunca como en una asignacion. `LEN(F(x))` es constante. TAC: `TMP.C d,len` seguido de `FUNR d,u` copia los bytes de la variable
resultado al temporal CHARACTER (`LDIR` en el Z80). `Unidad.flen` guarda la longitud. Los argumentos CHARACTER siguen en S3.
Limite heredado (no de S2b): la asignacion a subcadena `S(i:j) = ...` aun no esta soportada por el generador (tambien dentro de funciones).
Pruebas: `test_cap09.c` (428; 2000 programas del fuzzer sin fallos), ejemplo `examples/funciones_s2b.f` + `.esperado.txt`.

## Estado de S2c (2026-11-02, en PC; SIN construir con z88dk ni medir en el Next)
Hecho: funciones de sentencia `F(D1, ..., Dn) = expresion` (INTEGER, REAL, LOGICAL; hasta 8 ficticios; sin argumentos tambien). Solo valen ANTES de la
primera sentencia ejecutable de la unidad y con `F` sin usar (o declarada): despues es un error `name is not an array`, como antes. Cada unidad tiene las suyas.
Diseno: la definicion guarda el cuerpo ya tipado (nodos compactados, ficticios -> `NODO_ARGF`) en la pagina de datos 2 desde `GEN_SF_OFF` (2560 B);
cada referencia copia esos nodos en el arbol de SU sentencia (`NODO_SFUN`, indices reubicados) y el generador evalua los argumentos reales a operandos (por valor),
los ata a los ficticios y genera el cuerpo (`gsfun`); las expansiones anidadas guardan/restauran la atadura. No hay ops TAC nuevas ni trampolines nuevos.
El ficticio oculta una variable del mismo nombre; el cuerpo ve las demas variables de la unidad, intrinsecas, funciones del usuario y otras funciones de sentencia
definidas antes (sin recursion: `undefined function`). Los argumentos reales deben tener el tipo del ficticio (`argument type mismatch`) y su numero
(`argument count mismatch`). El valor del cuerpo se convierte al tipo de F como en una asignacion. Errores nuevos: `statement function already defined`,
`statement function too large` (el cuerpo copiado no cabe en el limite de 64 nodos de la sentencia, o la tabla de 2560 B esta llena) (`MF_COD_UNI` + 19, 20).
No soportado (S3): funciones de sentencia o ficticios CHARACTER (`not supported yet`), ficticio con el nombre de un array de la unidad.
Pruebas: `test_cap09.c` (451) y el fuzzer diferencial `tools/fuzz/fuzz_sf.py` (programas aleatorios con aritmetica de 16 bits calculada aparte: 1500 programas sin
fallos; 10 mutantes del generador y del semantico, todos muertos entre el fuzzer y los tests); ejemplo `examples/funciones_s2c.f` + `.esperado.txt`.

## Estado de S3a (2026-11-07, cerrada: probada en el Next con `argch.f`)
Hecho: argumentos CHARACTER escalares (variable, elemento de array, literal, concatenacion, referencia a funcion CHARACTER*n) para `SUBROUTINE` y `FUNCTION`,
con formales `CHARACTER*n` (o `CHARACTER` = longitud 1). Semantica: copia de entrada con la longitud del FORMAL (se trunca o se rellena con blancos como en una
asignacion) y copia de salida solo de los `n` caracteres del formal (el resto del real queda intacto; un literal o expresion no se escribe de vuelta). Elemento de array
CHARACTER: temporal de la longitud del elemento (`TMP.C` + `LDE`), copia de salida y `STE`. Tambien hecho: asignacion a subcadena `S(i:j) = e`, `A(k)(i:j) = e`
(op TAC `SSET`, con `EXT` para el limite superior; blancos o truncado; fuera de rango = error de ejecucion en tacvm).
Comprobaciones al final del fichero (`uni_comprobar`): `argument type mismatch` si el tipo no coincide y el NUEVO `argument shorter than dummy argument`
(`MF_COD_UNI` + 21) si la longitud conocida del real es menor que la del formal (norma F77 15.9.3.2: el real no puede ser mas corto). Un real mas largo es valido (el
formal ve los primeros `n` caracteres). El registro de llamadas anota, tras los tipos, un byte de longitud por cada argumento CHARACTER.
No soportado todavia: subcadena como argumento real (`S(i:j)`: su longitud no se conoce al compilar, `not supported yet`; S3d), arrays como argumento (S3b),
`CHARACTER*(*)` (S3c), funciones de sentencia con ficticios CHARACTER.
Pruebas: `test_cap09.c` (477: 17 programas con salida exacta, 9 diagnosticos con linea, columna y codigo, 5 de subcadena), fuzzer diferencial
`tools/fuzz/fuzz_char.py` (modelo propio en Python; 900 programas sin fallos; un mutante del `LDIR` de salida, muerto en 4 programas); ejemplo
`examples/args_char_s3a.f` + `.esperado.txt`.

## Estado de S3b + S3c + S3d (2026-11-08, cerradas: probadas en el Next con arrarg/arrmat/chrast/chrsub)
Hecho, las tres juntas:
- **S3b, arrays como argumento**: nombre de array completo, o elemento `A(i)` / `M(i,j)` como direccion de comienzo (el formal array ve el trozo que empieza ahi, como
  manda la norma). Formales con dimension fija, ajustable (`A(N)`, `A(N,N)`; la primera dimension variable se guarda en `Dim.svar` y es el paso de la 2D) y `*` en la ultima
  (`DIM_ASUMIDO`). Tipos INTEGER, REAL, LOGICAL y CHARACTER*n. El formal array es una celda de 2 bytes con el puntero (referencia verdadera: aqui NO hay copia). Un elemento
  pasado a un formal ESCALAR sigue siendo copia de entrada/salida (como en S1).
- **S3c, `CHARACTER*(*)`**: la longitud del formal es la del real. Disposicion `[long 2 B][datos]` como los temporales; `LEN(S)` genera `SLEN`; `S(i:j) = e`, comparaciones,
  concatenacion y paso a otra unidad funcionan con longitud supuesta. Capacidad de la copia de salida = la mayor longitud de real vista (la escribe `uni_comprobar` en el
  `extra` del simbolo formal; 255 si es desconocida o subcadena).
- **S3d, subcadenas como argumento real** `S(i:j)`, `T(k)(i:j)`: se copia a un temporal y al volver se escribe de vuelta (`SSET`), asi lo que el subprograma escribe llega a
  la variable. `GArg` pasa a 11 B empaquetados y `GEN_ARG_MAX` sube de 15x8 a **93** ranuras.
Comprobaciones nuevas (`comprobar_arg` en `unid.c`): tipo, array/escalar/elemento compatibles, longitud CHARACTER, y tamano del array real frente al formal de dimension
fija (`UNI_TAM_ARRAY`, `UNI_LONG_ARRAY`, codigos `MF_COD_UNI` 110 + el valor del enum). El registro de llamadas anota por argumento un byte de clase (tipo | `UA_ARRAY` 0x10 |
`UA_PROC` 0x20 | `UA_ELEM` 0x40) y un byte de informacion para CHARACTER y arrays (longitud o numero de elementos saturado a 255; 255 = desconocido).
Reparto del backend: `back.c` se compila dos veces (`BK_A` banco 43, `BK_C` nuevo **banco 48** via `back_c.c`, `g1_banco48.c`, trampolin `t_back_c`); el estado compartido es
`bk_*` y `Z80Est z_est`. `-DBACK_DIVIDIDO` activa en PC la misma particion para probarla. Pruebas del reparto: todas las suites en las tres configuraciones.
Error corregido por el camino: `gen_sentencia` ponia `g.ntemp = 0` tambien en `CALL`, de modo que `gllamada` reutilizaba el temporal `%0` tras la llamada (ahora no se reinicia en `CALL`).
No soportado: sentencia `DIMENSION`, `DOUBLE`, formal `CHARACTER*(*) A(n)` (array de cadenas de longitud supuesta), `PRINT *, A` de un array entero, `S(i:j)` de longitud 0
tratado como longitud 0 (no es error).
Pruebas: `test_cap09.c` 543 (bateria `tests_arrays.h`), fuzzer diferencial `tools/fuzz/fuzz_arr.py` (+ `runf.c`), mutaciones del generador, del semantico y del backend
(los dos huecos que dejaron vivos, `CHARACTER*(*)` de longitud 0 en copia de salida y `S(2:)` de un formal supuesto, tienen ya su test); ejemplos para el Next
`compiler/tests/ejemplos/arrarg.f`, `arrmat.f`, `chrast.f`, `chrsub.f` (+ `.esperado.txt`, revisados a mano). Pendiente: construir, medir bancos 41-48, bss y pila.
Siguiente: S4 (`EXTERNAL`, `INTRINSIC`, procedimientos como argumento, `ENTRY`, `RETURN` alternativo).
Medido en el Next (2026-11-08): huecos 41: 4584, 42: 11175, 43: 4176, 44: 275, 45: 1227, 46: 874, 47: 2701, 48: 8847 B; bss `$BAF1`; pila 425 de 1148 B (`chrsub.f`).
Reparto de memoria hecho en esta etapa: backend en dos bancos (43 y 48), `tac.c` al banco 42 (trampolines `t_tac_*`) y la tabla de cadenas del lexer a la
pagina de datos 1 (desde 6144; `tabla_texto` devuelve un puntero residente con 4 ranuras de nombres y un buffer de 256 B para textos largos; un literal CHARACTER
no puede pasar de 255). `stm_ast` (512 B de bss) se queda en el residente: 158 accesos por puntero.

## Diseno de S4 (2026-11-08, PROPUESTO: pendiente de confirmar las decisiones D1..D6 del final)
Alcance acordado con el autor: `DIMENSION` (entra aqui por ser basica en F77 y faltar), `EXTERNAL`, `INTRINSIC`, procedimientos como argumento, `ENTRY` y retorno
alternativo (`RETURN e`, `SUBROUTINE S(A, *, *)`, `CALL S(X, *10, *20)`). Estado de partida: el parser NO reconoce `DIMENSION`, `EXTERNAL`, `INTRINSIC` ni `ENTRY`
(`clasif.c` solo tiene `RETURN` sin expresion); el codigo `UA_PROC` (0x20) ya esta reservado en el registro de llamadas; la recursion se detecta con el grafo de llamadas (unid.c).

### Etapas (cada una con su bateria en PC antes de pasar a la siguiente, y un `.f` para el Next)
- **S4a `DIMENSION`, `EXTERNAL`, `INTRINSIC`** (solo declaraciones, sin cambiar la convencion de llamada).
- **S4b retorno alternativo** (`*` en la lista de formales, `RETURN e`, `*etq` como real).
- **S4c `ENTRY`**.
- **S4d procedimientos como argumento** (subrutinas y funciones del usuario).
- **S4e intrinsecas como argumento** (opcional; ver D2).

### S4a - declaraciones
- `DIMENSION A(10), B(3, 4)`: lista de nombres con dimensiones, mismas reglas y limites que en una declaracion de tipo (1 o 2 dimensiones, `*` en la ultima, formales
  ajustables). Si el nombre ya tiene tipo (`INTEGER A` antes), lo conserva y se hace array; si no, tipo implicito (o `CHARACTER*n` previo: la longitud del elemento). Es error
  declarar dos veces las dimensiones (`array already dimensioned`) y usar el nombre como escalar antes de `DIMENSION` (se exige que la declaracion preceda al uso; pendiente de verificar contra el texto de la norma, ver D4).
  Cambio en `declarar()` (sem.c): hoy exige el tipo; pasa a admitir "sin tipo" y reutiliza `dimensiones()`. Parser: sentencia nueva `SENT_DIMENSION` con la misma lista que una declaracion.
- `EXTERNAL P, Q`: marca los nombres como procedimientos del usuario (simbolo de clase `SIM_PROC`, nuevo): se pueden pasar como argumento (S4d) y, si coinciden con una
  intrinseca, ganan a la intrinseca (sigue siendo ilegal ocultar una intrinseca sin EXTERNAL, decision de S2). Un formal declarado `EXTERNAL` es un formal procedimiento.
- `INTRINSIC SIN, ABS`: solo comprueba que son intrinsecas conocidas (`not an intrinsic function`) y las marca; sin efecto en generacion salvo en S4e.
- Mensajes nuevos: `array already dimensioned`, `not an intrinsic function`, `name already declared as a variable`.

### S4b - retorno alternativo
- Formal `*` (solo en `SUBROUTINE`, nunca en `FUNCTION`): ocupa una posicion de la lista pero no tiene almacenamiento. Real `*10`: etiqueta de la unidad que llama. Registro de llamadas: clase de argumento
  nueva `UA_STAR` 0x80 y comprobacion: el formal en esa posicion debe ser `*` y viceversa (`argument type mismatch`).
- TAC: `RET` gana un operando opcional (`RET e`, `a` = expresion INTEGER; sin `a` es el `RET` de siempre). Una variable de 1 B en la imagen (`RTCODE`) recibe `e` si `1 <= e <= n` (n = numero de `*` del formal) y 0 en cualquier otro caso (retorno normal, que es lo que dice la norma
  para `e` fuera de rango). Solo las unidades con `*` y sus llamadores usan `RTCODE`; el resto no cambia ni crece.
- Tras el `CALL` de una llamada con `*`: nueva instruccion `JALT k, etq` por cada `*etq` (backend: `LD A,(RTCODE) / CP k / JP Z,etq`), y la sentencia sigue en la siguiente si `RTCODE` es 0.
  `RTCODE` se pone a 0 antes del `CALL`. Coste: ~8 B por `*` en el llamador y ~6 B por `RETURN e`.

### S4c - ENTRY
- `ENTRY E(a, b)` dentro de una `SUBROUTINE` o `FUNCTION`: otro punto de entrada a la MISMA unidad. Se modela como una **unidad alias**: registro propio en el directorio (`padre` = unidad
  que lo contiene, lista de formales propia, nombre propio) cuyo codigo empieza en la etiqueta del `ENTRY` dentro del cuerpo (la ejecucion continua desde la sentencia siguiente al `ENTRY`).
  `CALL E(...)` / `E(...)` generan `PARAM`/`CALL` contra el alias como contra cualquier unidad; los formales del alias se resuelven a simbolos de la unidad padre (un mismo nombre en varias listas es
  la misma variable, que es lo que dice la norma: la lista asocia posiciones, no variables).
- En una `FUNCTION`: cada nombre de entrada tiene su variable resultado. Los resultados de los nombres que tienen el MISMO tipo (y la misma longitud si es `CHARACTER`) comparten almacenamiento (la norma los asocia; SIN verificar el texto exacto); si el tipo difiere, cada uno tiene el suyo y `FUNR` lee el de la entrada usada. El `RETURN` es el mismo (epilogo comun).
- Diagnosticos: `ENTRY` fuera de subprograma o dentro de un bloque `IF`/`DO` (`ENTRY inside a block`) y nombre de entrada repetido (`program unit already defined`). NO se detecta que el cuerpo use un
  formal que no esta en la lista de la entrada realmente usada: queda indefinido (comportamiento no definido por el lenguaje, como en otros compiladores).

### S4d - procedimientos como argumento
Problema central: la unidad llamada solo se conoce en tiempo de ejecucion, pero hoy `PARAM x, k, u` escribe en los formales de la unidad `u`, fijada al compilar.
- **Convencion indirecta (propuesta D1)**: el que llama deja en un buzon global `ARGBUF` (2 B por argumento, hasta 15) la **direccion de cada real** (escalar, elemento, array, temporal, subcadena: todo se pasa por referencia verdadera) y
  hace `CALL (HL)` con la direccion del procedimiento. Cada unidad que se pasa como real a algun sitio (conjunto conocido en compilacion porque todo se compila junto; lo calcula el backend en su primera pasada) recibe un **resguardo**
  tras su cuerpo: copia de entrada desde las direcciones del buzon a sus formales (con el tipo/longitud del formal), guarda esas direcciones en 2 B ocultos por formal, llama al cuerpo por su camino normal y hace la copia de salida con las direcciones guardadas
  (las guarda ella misma porque el cuerpo puede usar el buzon). Las llamadas directas siguen exactamente como estan (sin coste); solo cuesta lo que se pasa como procedimiento.
- Formal procedimiento `P`: celda de 2 B con la direccion del resguardo. `CALL P(a, b)` o `P(a, b)` (funcion) emite `PARAMI x, k` (direccion al buzon) + `CALLI P` (TAC nuevos). Funcion: el resguardo copia el resultado a una variable global `FRET`
  (INTEGER/REAL/LOGICAL: 2 o 4 B) y el llamador lo lee con `FUNRI`.
- Real = nombre de procedimiento (`CALL APPLY(F, X)` con `EXTERNAL F`, o `F` definida en el fichero): `PARAM` con clase `UA_PROC`; el operando es la unidad y el backend carga la direccion de su resguardo.
- Comprobaciones: el real debe ser un procedimiento (`argument must be a procedure`); el tipo `SUBROUTINE`/`FUNCTION` debe coincidir con el uso del formal (`CALL P` exige subrutina, `P(..)` exige funcion de ese tipo). El numero y tipo de argumentos de la llamada indirecta se comprueban
  contra el procedimiento real en el sitio donde se pasa (las llamadas al formal dentro del llamado dejan una firma en el directorio; al pasar `F` se compara con los formales de `F`). Es lo mas caro de S4d (ver D5).
- **Recursion**: una llamada a traves de un formal no tiene destino conocido; se anade al grafo una arista desde la unidad que recibe el procedimiento hacia cada unidad que se le pasa como real. Asi se sigue detectando `recursive call`.
- Limite: funciones `CHARACTER` como argumento NO se admiten en la primera version (`not supported yet`) por el buffer de resultado (D6).

### S4e - intrinsecas como argumento (opcional)
`CALL APPLY(SIN, X)` / `INTRINSIC SIN`: hace falta una version llamable de cada intrinseca. Se sintetiza una unidad-resguardo oculta por intrinseca realmente usada (cuerpo `NOMBRE = INTR(arg)` generado con el
generador normal sobre un AST sintetico). Alcance propuesto: solo los nombres especificos de un argumento (`ABS`, `IABS`, `SQRT`, `SIN`, `COS`, `EXP`, `ALOG`, ...) y `MOD`/`MIN0`/`MAX0` de dos; el resto da `not supported yet`.

### Reparto de memoria (DERIVADO, SIN MEDIR; hoy 41: 4584, 42: 8302, 43: 4032, 44: 2989, 45: 1227, 46: 874, 47: 2701, 48: 8847 B; pila 458 B de margen)
- Parser (`stmt.c`/`clasif.c`, banco 47): ~0,6-0,9 KB (cuatro sentencias nuevas, `*` en listas, `RETURN e`). Cabe.
- Semantico: `sem.c` (banco 45) solo tiene 1227 B libres y S4 le anade ~2 KB (DIMENSION, EXTERNAL/INTRINSIC, ENTRY, RETURN e). **Antes de S4a hay que sacar las declaraciones de `sem.c` a otro fichero en el banco 42** (como se hizo con `sem_sf.c`).
- Generador: `gen_ll.c` (banco 42) absorbe llamadas indirectas y `JALT`; `gen.c` (44) tiene 2,9 KB.
- Backend: resguardos, `JALT`, `RET e`, `CALLI`: ~1,5 KB en `back_c.c` (banco 48, 8,8 KB libres) mas ~100 B de runtime en la imagen (buzon, `RTCODE`, `FRET`) solo si se usan.
- Pila: el margen actual es de ~458 B. Los trampolines nuevos y el sem partido no deben pasar de ~150 B de bss; si no, el recurso es `stm_ast` a pagina de datos (512 B).

### Bateria de S4 (ademas de la exigida arriba)
- Diagnosticos: uno por mensaje nuevo con linea, columna y codigo (`MF_COD_UNI`/`SEM_*`).
- Semantica a mano por etapa: `DIMENSION` con todas las combinaciones de tipo previo/implicito/CHARACTER/formal; retorno alternativo con 0, 1, 2 y mas retornos y `e` fuera de rango; `ENTRY` en subrutinas y funciones (mismo y distinto tipo,
  formales compartidos o no); procedimientos como argumento (subrutina, funcion INTEGER/REAL/LOGICAL, paso encadenado A -> B -> C, el mismo procedimiento pasado a dos formales, formal usado dos veces, real elemento/array/subcadena/expresion/constante, copia de salida a traves del resguardo).
- Diferencial `tacvm` / Z80, fuzzers nuevos (`fuzz_proc.py`: grafos de llamadas con procedimientos como argumento, ENTRY y retornos alternativos, con oraculo propio en Python), mutantes del backend (resguardo, `JALT`, buzon) y del generador.
- Limites: 15 argumentos por el buzon, cadenas de `PARAMI`, anidamiento de llamadas indirectas, recursion a traves de formales.
- Next: `examples/` con nombre corto por etapa (`dim.f`, `retalt.f`, `entry.f`, `procarg.f`).

### Decisiones abiertas para el autor
- **D1** Convencion de paso de procedimientos: resguardo + buzon de direcciones SOLO para las unidades que se pasan como real (propuesta; mas simple en el llamador, no cambia las llamadas directas) frente a especializar cada llamada indirecta por todos los destinos posibles (sin buzon, pero el codigo crece con el producto llamadas x destinos).
- **D2** Intrinsecas como argumento (S4e): ¿en esta etapa, solo la lista corta propuesta, o se deja para despues (`INTRINSIC` solo declara)? Recomiendo dejar S4e para el final y decidir con las medidas.
- **D3** Resultado de `ENTRY` en funciones: compartir almacenamiento si el tipo coincide (propuesta) o una variable por entrada siempre.
- **D4** `DIMENSION` despues del primer uso del nombre: error (propuesta; la regla exacta de la norma esta SIN verificar) o aceptarlo.
- **D5** Comprobar numero y tipo de argumentos de las llamadas indirectas al pasar el procedimiento (propuesta: si, es el coste mayor de S4d) o dejarlo como en F77 (sin comprobar, el programador responde) y documentarlo.
- **D6** Funciones `CHARACTER` como argumento: fuera en la primera version (propuesta).

## Hoja de ruta de versiones y decisiones de S4 (2026-11-08, ACORDADAS con el autor)
**MyFortran 0.1** (version publicable): hasta aqui S1-S3 mas: reparto previo de `sem.c` (hecho: `sem_decl.c`), S4a (`DIMENSION`, y `EXTERNAL`/`INTRINSIC` como declaraciones; usar un nombre
declarado asi como argumento real da un error explicito `not supported yet`), `READ` (entrada de datos, obligatoria), `PARAMETER` y `DATA`. Despues, pruebas exhaustivas en PC, todos los ejemplos en el Next y la preparacion de la version.
**0.2**: `COMMON`, `SAVE`, paso de procedimientos e intrinsecas como argumento (S4d/S4e) y soporte completo de `EXTERNAL`/`INTRINSIC`.
**0.3**: retornos alternativos (S4b), `ENTRY` (S4c), `EQUIVALENCE`, `BLOCK DATA` y ampliaciones segun las pruebas de los usuarios.
`ENTRY` no es un simple alias: cada entrada tiene su propia lista de argumentos y su propio punto de entrada dentro del cuerpo, compartiendo el contexto de la unidad.

Decisiones tomadas por el autor tras leer la norma (secciones citadas por el autor; el texto no se ha verificado aqui):
- **D3** (0.3): UN almacenamiento de resultado compartido por todas las entradas de una `FUNCTION`, con simbolos y tipos individuales (§15.7.3). Si el resultado es `CHARACTER`, todas las entradas deben ser `CHARACTER` con la misma longitud
  (salvo las reglas especiales de longitud supuesta); con resultados numericos se permiten tipos distintos. Esto sustituye a la propuesta anterior (compartir solo si el tipo coincide).
- **D4** (0.1): `DIMENSION` es valida tras una declaracion de tipo o tras la aparicion del nombre en una cabecera (como formal); es error despues de la primera sentencia ejecutable y al repetir el declarador de un array (§§3.5, 5.1, 8.1).
  Es una sentencia de especificacion: por tanto `DIMENSION`, `PARAMETER`, `EXTERNAL` e `INTRINSIC` tras una sentencia ejecutable dan error (`sem.ejec`). Las declaraciones de TIPO siguen aceptandose en cualquier punto, como hasta ahora.
- D1, D2, D5 y D6 (procedimientos como argumento) pasan a 0.2 y siguen abiertas.


**Estado (2026-11-08):** S4a, `PARAMETER`, `DATA` y `READ` estan implementados en PC; ver `0015-datos-y-entrada.md`. Quedan para 0.1: construir y medir en el Next, ejemplos en el Next, pruebas exhaustivas y preparacion de la version.
