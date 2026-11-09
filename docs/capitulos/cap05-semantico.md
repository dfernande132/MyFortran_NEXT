# Capítulo 5 - Análisis semántico

Estado: **cerrado**. Código y 1.259 pruebas en PC (ASan/UBSan, mutación, fuzz); medido en el Next con tres bancos (spike `c5_medir`).

Ficheros: `compiler/src/sim.{h,c}` (tabla de símbolos), `compiler/src/sem.{h,c}` (tipado, declaraciones, bloques,
etiquetas), `compiler/tests/test_cap05.c`. Decisión: `docs/decisiones/0009-semantico.md`.

## 1. Teoría en cinco ideas

1. **Qué es.** El parser comprueba que la sentencia está bien *escrita*; el semántico, que *tiene sentido*: `X = L + 1`
   es sintácticamente perfecta, pero sumar un LOGICAL no significa nada. Sus tareas: símbolos, tipos, bloques, etiquetas.
2. **Tabla de símbolos.** Para cada nombre: tipo, clase (variable, array), dimensiones, banderas. En F77 el tipo se
   declara o es **implícito**: inicial I–N entero, el resto real. Un nombre que aparece sin declarar se crea al vuelo.
3. **Tipar expresiones = recorrer el árbol de abajo arriba.** Las hojas tienen tipo propio (constante, variable); cada
   operador combina los de sus hijos con una regla. Aritmética: gana el tipo mayor (INTEGER < REAL < DOUBLE). Relacionales:
   dos numéricos o dos CHARACTER, resultado LOGICAL. Lógicos: solo LOGICAL. `//`: solo CHARACTER. El resultado queda en
   `Nodo.tdato`; el generador (cap. 6) decidirá con él las conversiones.
4. **A(I) es ambiguo hasta aquí.** El parser no sabe si `A(I)` es un elemento de array, una función o una subcadena. El
   semántico lo decide mirando la tabla: es array si fue declarado así; `S(2:3)` es subcadena si S es CHARACTER escalar.
5. **Bloques y etiquetas con una pila y una tabla.** `IF/ELSE/ENDIF` y `DO … etiqueta` se emparejan con una **pila** de
   bloques abiertos; las etiquetas, con una **tabla** de definidas y referenciadas que se revisa al `END` (hay saltos
   hacia delante). Como F77 permite que varios DO terminen en la misma etiqueta, una sentencia etiquetada cierra *todos*
   los DO de la cima que acaban en ella.

## 2. Decisiones de diseño

- **Sentencia a sentencia.** `sem_sentencia()` recibe el registro actual (`stm`) y recorre las listas con
  `stmt_elemento()`: nunca existe el AST del programa (consecuencia de 0008-B). Tras llamarla hay que volver a mirar
  `stm.err` (error de sintaxis tardío en una lista).
- **Tabla de símbolos por unidad de programa, 512 símbolos** (`SIM_MAX`) en la página de datos 0 de `banco.h`.
  Registro de 16 B (nombre de 8 inline, tipo|clase, banderas, enlace, extra, dirección) = exactamente una página de 8 KB.
  Búsqueda por hash de 64 cubetas (128 B residentes). Las dimensiones de los arrays van en la página 1 (`Dim`, 10 B);
  `Simbolo.extra` es su índice y `flags` guarda el número de dimensiones (0–2). `dir` la rellenará el generador.
- **Etiquetas** en la página 2: hasta 256 por unidad, registro de 10 B (12 en PC por alineacion), búsqueda lineal (estimación: sobra para
  programas didácticos; sin medir). **Pila de bloques** residente: 16 niveles × 10 B (12 en PC) (sin medir).
- **Errores acumulados:** se guardan los primeros 16 (con línea y columna ya convertidas, porque el texto de la sentencia
  se pierde al leer la siguiente) y `sem.n_err` cuenta todos. Los errores de un subárbol no se repiten en cascada.
- **Constantes REAL como texto**: el compilador no usa float (regla del núcleo).

## 3. Qué se comprueba

Tipos de operandos y asignación (numérico←numérico con conversión; LOGICAL←LOGICAL; CHARACTER←CHARACTER); constantes
INTEGER de 16 bit con signo (`-32768` vale); arrays (nombre sin índices, nº de índices, índice no entero, rango fuera de
sitio); subcadenas (`S(i:j)`, `A(k)(i:j)`); declaraciones (redeclarado, límites constantes con `lo ≤ hi`, 1 o 2
dimensiones, longitud CHARACTER 1..255); condiciones de IF; variable DO escalar numérica; selector del GOTO calculado;
IF/ELSEIF/ELSE/ENDIF emparejados (ELSE repetido, huérfanos, bloque sin cerrar); DO con etiqueta posterior, sentencia
terminal válida, cierre múltiple y anidamiento DO/IF; etiquetas duplicadas, indefinidas, y etiqueta en declaración;
falta de `END` (`sem_fin_fichero`).

## 4. Pruebas

`test_cap05.c`: 1.259 pruebas en PC con ASan/UBSan (tabla de símbolos, tipado implícito y explícito, `tdato` de los
nodos, banderas, arrays, subcadenas, control, bloques, etiquetas, límites 16/256/512, acumulación). Mutación de `sim.c` (8 mutantes): el único real (comparar 7 de 8 letras del nombre) se mató con nombres de la misma cubeta
que solo difieren en la 8.ª letra; quedan 2 equivalentes (cambios de la función hash que no alteran el resultado). Mutación
de `sem.c` (unas 60 mutaciones en tres rondas): los supervivientes reales eran la falta de pruebas de `tdato`, de banderas
USADO/ASIGNADO, de `lo >= hi`, de las referencias 1.ª y 3.ª del IF aritmético y de la primera referencia de una etiqueta;
se añadieron pruebas y mueren. Quedan equivalentes: `x > 32767` frente a `> 32768` en `constante` y `v < 0` frente a
`v < 1` en `longitud_const` (el desbordamiento a `int16_t` produce el mismo error), exigir `REFERENCIADA` al revisar
etiquetas indefinidas, y `if (stm.err == STM_OK)` antes de `indice(stm.b)` en el GOTO calculado. Fuzz de 200.000 programas de 6 sentencias mutadas y otros de
12 sentencias con bloques y etiquetas: sin fallos de sanitizadores. El fuzz encontró un fallo real: `T(1)(1,2)`
(segundo paréntesis que no es un rango) hacía recursión infinita; ahora es `SEM_SUBCADENA`.

## 5. Limitaciones conocidas

- **Funciones**: las INTRÍNSECAS se reconocen desde 0010 (`docs/decisiones/0010-intrinsecas.md`). Cualquier otro `F(X)` que no
  sea un array declarado da `SEM_FUNCION` (no hay subprogramas de usuario hasta el capítulo siguiente). Tampoco hay
  función-sentencia.
- Un nombre usado y luego declarado (`X = 1` / `REAL X`) es `SEM_YA_DEFINIDO`. No se exige que las declaraciones precedan
  a las ejecutables.
- Dimensiones y longitudes solo con constantes enteras (sin `PARAMETER`, que aún no existe). Máximo 2 dimensiones.
- Las etiquetas de referencia de GOTO/IF/DO se atribuyen a la **columna 7** de la sentencia (el parser no guarda la
  posición del token etiqueta); el GOTO calculado sí da la posición exacta. El `PRINT n` con formato no se comprueba
  (no hay `FORMAT`).
- **Sentencia terminal de un DO** (lista confirmada por Dani, 2026-10-30): no pueden serlo GOTO incondicional, GOTO
  asignado, IF aritmético, IF de bloque, ELSE IF, ELSE, END IF, RETURN, STOP, END ni otro DO. Aquí se comprueban las que ya
  existen en el compilador; GOTO asignado y RETURN se añadirán cuando se soporten.
- `IF` lógico terminal: la sentencia interior no puede ser DO, IF de bloque, ELSE IF, ELSE, END IF, END ni otro IF lógico;
  el clasificador ya rechaza todas ellas dentro de un IF lógico, así que basta admitirlo siempre.
- No se mide aún el coste en el Next (código, residente, pila): `sem.c`/`sim.c` están fuera de `core-pc`/`core-next`.

## 6. Medida en el Next (spike c5)

`tools/spike/c5_*.c`: tres bancos (46 lectura, 47 sintaxis, 45 semántico = `sim.c`+`sem.c`) con trampolines; la tabla de
símbolos, dimensiones y etiquetas van a las páginas de datos 0-2 (`platform/next/banco_next.c`, ventana MMU2 con DI).
`medir_core.c` con `-DMF_CON_SEM` añade la fase `sem:` (programa de 21 sentencias con 4 errores a propósito) y mide la pila
del peor caso (31 sumas = 63 nodos). En PC (`tools/build.ps1 -Objetivo sem-pc`) la salida esperada es
`sem: sentencias=21 sintaxis=0 errores=4 simbolos=8` y errores `5@17:7, 2@18:13, 6@19:7, 21@20:7` (código@línea:columna).
Debe coincidir en el Next. A medir: tamaño del residente y de cada banco, hueco hasta la pila, pila `sem`, nº de mapeos.

### Primera ejecución (2026-10-30)

- Salida de la fase `sem:` **idéntica en PC y Next** (21 sentencias, 4 errores `5@17:7, 2@18:13, 6@19:7, 21@20:7`, 8 símbolos).
  Es la primera vez que la tabla de símbolos (páginas de datos 0 y 1) y las etiquetas (página 2) corren en el Next.
- Tamaños: residente **14.238 B** (c4: 12.196; +2.042), fin del bss `$B79E`, **hueco hasta la pila (`$BFF0`) 2.130 B**
  (c4: 4.172). Bancos: 45 = 8.930 B (`sim`+`sem`), 46 = 8.061 B, 47 = 5.211 B (los dos últimos, iguales que en c4).
- Reparto del +2.042 B residente según `analizar_map.py`: `code_compiler` +1.003 (trampolines nuevos y la fase `sem_programa`
  del medidor), `rodata_compiler` +537 (sobre todo `PROGRAMA3`), `bss_compiler` +474 (estimado: `sem` ~100 B, pila de bloques
  ~160, cubetas del hash 128, `buf_sem` 96). **Solo el bss y los trampolines son del producto**; el resto es del medidor.
- Fallo: tras imprimir el error 4 el emulador se **reseteaba** al medir la pila. Causa más probable (no demostrada): con
  `PILA_PINTADA` = 2048 y solo 2.130 B libres, el relleno `0xA5` de la medida llegaba al final del bss y pisaba variables
  de los trampolines (`banco_actual`, `orig6/7`), de modo que `mapear(ant)` restauraba un banco erróneo. Mismo patrón que el
  cuelgue de c4. Arreglo: `medir_core.c` no pinta nunca por debajo de `__BSS_END_tail + 64` y el informe de pila dice el
  rango realmente pintado.

### Segunda ejecución, con la medida de pila arreglada (2026-10-30)

- Ya no se resetea. Salida de PC y Next idéntica; `FIN PROCESO` alcanzado.
- Residente **14.303 B**, fin del bss `$B7DF`, **hueco hasta la pila 2.065 B** (+65 B por el código nuevo del medidor).
  Bancos sin cambios (45 = 8.930, 46 = 8.061, 47 = 5.211 B). 2.190 mapeos de banco (~120 ms estimados a 55 µs/mapeo).
- Pila del parser (como en c4): n=4 → 299, n=12 → 579, n=20 → 859 B.
- **Pila del semántico, 31 sumas encadenadas (63 de 64 nodos): 1.142 B** (rango pintado 1.914; no saturó). Es mayor que la del
  parser porque el tipado recorre el árbol por recursión (una cadena de sumas por la izquierda da un árbol profundo, ~62
  marcos `tipar`/`tipar_binario`). Estimación sin comprobar: ~17 B por marco.
- Con 2.065 B libres sobran ~900 B sobre ese peor caso medido. No es el peor caso posible: paréntesis anidados + sumas
  puede costar algo más por marco. El medidor ya mide también `X=((…(I+…+I)…))` con 22 paréntesis y 20 sumas (también 63
  nodos); pendiente de ejecutar.

### Tercera ejecución: peor caso mixto y recorte de marcos (2026-10-30)

- Medido en el Next (antes de recortar): 31 sumas **1.142 B**; **22 paréntesis + 20 sumas 1.373 B** (rango pintado 1.627, sin
  saturar). Residente 14.582 B (+279 B, solo código del medidor), fin del bss `$B8F6`, **hueco hasta la pila 1.786 B**:
  el peor caso medido dejaba solo ~410 B de margen, demasiado justo.
- Causa de que los paréntesis cuesten más por nivel (hipótesis razonada, no comprobada en el .map): `tipar` y `tipar_ref`
  llevaban un `Simbolo` local de 16 B, y sccz80 reserva todos los locales al entrar, así que cada marco recursivo lo pagaba
  aunque solo lo usara la hoja NOMBRE.
- Cambio (sin cambio de comportamiento; 1.259 pruebas y fuzz iguales): la hoja NOMBRE pasa a `tipar_nombre()` y `tipar_ref`
  lee el símbolo con `datos_simbolo()` en tres `uint8_t` pequeños. El medidor mide ahora tres formas: 31 sumas, 22 paréntesis +
  20 sumas y 22 `V(` anidados (`X=V(V(…V(1)…))`, que pasa por `tipar_ref` en cada nivel, el caso que más pesaba).
- **Pendiente de medir en el Next** con el cambio aplicado.

### Cuarta ejecución: el recorte funciona; la tercera forma cuelga (2026-10-30)

- Con el recorte de marcos (`tipar_nombre`, `datos_simbolo`): 31 sumas **604 B** (antes 1.142, −47 %) y 22 paréntesis + 20 sumas
  **907 B** (antes 1.373, −34 %). Confirma que el `Simbolo` local pesaba en cada marco recursivo.
- Residente 14.836 B (+254 B del medidor), fin del bss `$B9F4`, **hueco hasta la pila 1.532 B**; banco 45 = 8.954 B.
- La tercera forma (`X=V(V(…V(1)…))` con 22 niveles) **se colgó** y no llegó a imprimir. Hipótesis (sin comprobar): el
  consumo que desborda no es el del semántico sino el del **parser** (cada nivel de argumentos recorre más marcos que un
  paréntesis: `primario → referencia → argumentos → expresion`), y la medida de pila de los capítulos 2/4 solo cubría
  paréntesis (859 B a n=20). Si es así, `EXPR_PROF_MAX = 24` no acota la pila en el peor caso real.
- Siguiente medida: se mide de menos a más (4, 10, 16, 22 niveles), por separado solo el parser y parser + semántico.

### Quinta ejecución: el parser era el que desbordaba (2026-10-30)

- Pila de `X=V(V(…V(1)…))` (parser; con semántico sale igual, así que el semántico no es el máximo): 4 niveles **588 B**,
  10 niveles **790 B**, 16 niveles **1.192 B** (rango pintado 1.269). Con 22 niveles se extrapola ~1.600 B > 1.532 B libres:
  desbordó (el cuelgue anterior). Hipótesis confirmada en lo esencial: la medida de pila de los capítulos 2/4 solo cubría
  paréntesis (~35 B/nivel); un nivel de argumentos pasa por 3 funciones más (`referencia`, `argumentos`, `argumento`) y
  pesa ~67-74 B (el tramo 10→16 sale a 67 B/nivel; el 4→10 a 34, **no lo explico**).
- **Cambio en `expr.c`:** cada nivel de argumentos `F(...)` cuenta DOBLE en `parser.prof` (presupuesto
  `1 + paréntesis + 2·niveles ≤ EXPR_PROF_MAX`). Con `EXPR_PROF_MAX = 24`: hasta 23 paréntesis, **11 niveles de `F(F(…))`** o
  mezclas dentro del presupuesto. Es un límite nuevo del lenguaje (documentado en `expr.h`). Verificado en PC para todas las
  combinaciones (p, k) con p, k < 14 y con pruebas nuevas en `test_cap02.c` (196).
- Peor caso esperado del parser tras el cambio: ~11 niveles × 74 B ≈ 0,8 KB + base. **Sin medir en el Next.**

### Sexta ejecución: cerrado (2026-10-30)

- Ya no se cuelga; `FIN PROCESO`. Salida de la fase `sem:` igual que en PC.
- Pila (parser; con semántico sale igual, salvo las dos primeras filas): 31 sumas **604 B**, 22 paréntesis + 20 sumas **913 B**,
  `V(` anidados: 4 niveles **388 B**, 8 → **656 B**, 11 (máximo) → **857 B**, 12 (da error de profundidad, `sint=1`) → **885 B**.
  Pendiente constante de **67 B por nivel** (4→8 y 8→11), la prevista. **Peor caso medido: 913 B.** (La lectura anterior de 790 B
  con 10 niveles no encaja con esta recta; no la explico, y el cambio de `expr.c` es posterior.)
- Residente 14.936 B (el medidor creció con sus tres formas), fin del bss `$BA58`, **hueco hasta la pila 1.432 B**: margen
  de ~520 B sobre el peor caso medido. Bancos: 45 = 8.954 B, 46 = 8.061 B, 47 = 5.257 B (+46 B por el contador doble en
  `argumentos`). 3.216 mapeos de banco (~176 ms estimados).
- Conclusión: el capítulo 5 cabe en el diseño de memoria (0007) con tres bancos de código y tres páginas de datos. El hueco
  de pila es el recurso escaso: ~1,4 KB con el medidor residente. El producto no llevará el medidor ni sus textos, pero el
  driver real y el editor ocuparán ese espacio; seguimos sin saberlo. Las opciones A, C y F de 0008 siguen disponibles.
