# Capítulo 4 - Analizador de sentencias

Estado: **medido** (código y 203 pruebas en PC, medido en el Next residente y en bancos); pendiente de cierre.

Ficheros: `compiler/src/stmt.{h,c}`, `compiler/tests/test_cap04.c`, nodo nuevo `NODO_DECL` en `ast.h`,
fase nueva en `tools/medir/medir_core.c`, spike de medida `tools/spike/c4_*.c`.

## 1. Teoría en cinco ideas

1. **Qué es.** Los capítulos anteriores resuelven *trocear* (lexer), *decidir la clase* (clasificador) y *entender una
   expresión* (expr). El analizador de sentencias los junta: dada la clase, comprueba que lo que sigue a la
   palabra clave tiene la forma que la gramática exige y lo deja en un **registro** (`Stmt`). Es sintaxis: no
   mira si un nombre está declarado ni qué tipo tiene (capítulo 5).
2. **Descenso recursivo sin dudas.** El clasificador ya dijo *qué* sentencia es (las palabras clave de F77 no
   están reservadas, ese era su trabajo). Con la clase conocida, cada gramática es corta y se resuelve mirando
   solo el token actual: una función `p_xxx` por clase, con un *lookahead* de uno.
3. **Parser plano.** La unidad de F77 es la sentencia, no el bloque: `IF-THEN … ELSE … ENDIF` y `DO … CONTINUE`
   llegan como registros sueltos y la fase siguiente los empareja con una pila. Por eso basta un registro por
   sentencia: leer → clasificar → analizar → entregar → olvidar.
4. **IF lógico = dos registros en secuencia.** `IF (c) sentencia` lleva una sentencia dentro. `stmt_analizar` deja
   la condición; `stmt_analizar_interna` deja después la sentencia interior (que sustituye al registro del IF).
   El consumidor ve «condición, salto, cuerpo» en orden natural, sin segundo registro ni recursión.
5. **Errores y recuperación.** Se anota el primer error (código + posición bruta, que `posicion_fuente` convierte en
   línea y columna). No hace falta recuperación: la siguiente sentencia empieza en la línea siguiente.

## 2. Decisiones de diseño

- **Registro único y global** `Stmt stm` + `Ast stm_ast` (decisión 0006). El token actual es `parser.tok`, el mismo
  que usa `expr.c`: tras una expresión, el analizador ve directamente el token que la detuvo (coma, `)`, fin).
- **Declaraciones reutilizan el parser de expresiones.** `B(10)` y `C(0:9,2)` ya salen como `NODO_REF` con lista de
  argumentos/rangos. `CHARACTER A*10` aprovecha que `A*10` es un `NODO_MUL` y se reinterpreta como «nombre con
  longitud». Cada declarador se envuelve en `NODO_DECL` (a = nombre o REF, b = longitud o 0). Casi nada de código nuevo.
- **Listas en flujo** (decisión 0008-B): declaradores, expresiones de PRINT y etiquetas del GOTO calculado no se guardan
  en el AST. `stmt_analizar()` solo lee la cabecera; `while (stmt_elemento())` entrega cada elemento en `stm.e` (raíz de un
  árbol que vale hasta la siguiente llamada). Al acabar la lista se lee el cierre (`)` y selector del GOTO calculado) y
  `stm.b` ya es válido. `AST_NODOS` baja a 64: el tope es por *elemento*, no por sentencia. Idea general: un compilador
  de una pasada no necesita ver la sentencia entera, solo el trozo que está procesando.
- **GOTO calculado**: etiquetas como nodos `NODO_ENTERO` (uno cada vez) + expresión selectora. El GOTO asignado no se soporta.
- **PRINT**: formato `*`, etiqueta o constante de texto; lista de expresiones (sin DO implícito).
- **Etiquetas operando** (GOTO, DO, IF aritmético, PRINT): 1..99999, error `STM_ETIQUETA_RANGO` si no.
- **Nombres `static` únicos por banco** (consecuencia de 0007): `expr.c` y `stmt.c` iban a compartir banco y ambos
  tenían `fallo` y `crear`; se renombraron los de stmt.c a `stm_fallo` y `stm_crear`.

## 3. Lo que el clasificador hace imposible (y por eso no hay error)

| Texto | Qué ve el clasificador | Consecuencia |
|---|---|---|
| `DO 10 I = 1` | asignación a `DO10I` (sin coma) | no hay error «falta la coma» en el DO |
| `INTEGER A=1` | asignación a `INTEGERA` | sin palabras reservadas, es F77 legal |
| `IF (A)` sin nada detrás | `SENT_DESCONOCIDA` | no hay error «falta la sentencia» tras IF |
| asignación con destino no válido | no llega: exige `nombre [(...)]… =` | no hay error «destino inválido» |

Se eliminaron `STM_DESTINO` y `STM_SENTENCIA_ESPERADA`: eran código que ninguna prueba podía alcanzar.

## 4. Pruebas

`test_cap04.c`: 203 pruebas en PC, cada sentencia escrita como expresión-S (`X = 1` → `(asig X 1)`), errores por
código y posición bruta. Con sanitizadores (ASan/UBSan): 0 fallos. Fuzz de 300.000 sentencias aleatorias (mezcla
de palabras clave y signos): sin fallos de memoria. Pruebas de mutación: 23 mutantes de `stmt.c` (límites de etiqueta,
modos de PRINT/GOTO, `stm.resto`, longitud de CHARACTER, reinicio del AST, mapeo de errores léxicos, etc.); el único
que sobrevivió la primera vez (`stm.resto + 1`) destapó un hueco (`IF(X)Y=1` sin blanco tras `)`) y se añadió la
prueba. Ahora los detectan todos.

Salida de la fase nueva de `medir_core` en PC (para contrastar con el Next):
`stmt: sentencias=35 ok=33 err=2 nodos=83` (35 = 13 de `FUENTE` + 22 de `PROGRAMA2`; los 2 errores son
`Y = 3 # 4` y `READ *, K`, aún no soportada).

## 5. Medidas en el Next (CSpect, sccz80 sin optimizar)

| Medida | Antes (cap. 2) | Todo residente (`core`) | Repartido en bancos (`c4_medir`) |
|---|---|---|---|
| Imagen residente | 21.630 B | 26.387 B (margen 6.380) | 14.456 B (acaba en `$B878`) |
| Código en bancos | - | - | banco 46: 7.677 B; banco 47: 5.278 B |
| Pila n=4 / 12 / 20 | 290 / 570 / 850 | 290 / 570 / 850 | 300 / 580 / 860 |
| Mapeos de banco | - | - | 1.338 (~73 ms a 55 µs, estimado) |
| `stmt:` | - | `sentencias=35 ok=33 err=2 nodos=83` | idéntico |

- `stmt.c` aporta 2.497 B de código y 1.072 B de bss (el AST de 1 KB y el registro de 34 B). Con la fase
  de `medir_core` y su segundo programa, el todo-residente crece 4.757 B.
- La salida de `stmt:` coincide con la de PC en el Next, tanto residente como en bancos.
- **Hueco de pila en la versión por bancos: 1.912 B** (residente hasta `$B878`, pila desde `$BFF0`). El peor caso de
  expresión usa 860 B, pero el hueco ya es corto: cada fase nueva con estado global lo estrecha.
- **Incidencia de medida (no del compilador)**: la primera ejecución de `c4_medir` se colgó tras la fase de sentencias.
  Causa: `medir_pila` pinta `PILA_PINTADA` = 3.072 B bajo la pila y, con el residente acabando en `$B878`, pisaba el bss
  (`ast`, `clasif`, `clas`). Se hizo configurable (`#ifndef PILA_PINTADA`) y `c4_medir` usa 1.536 B. Lección: toda medida
  que escriba memoria por debajo de la pila debe comprobar antes el hueco real.

### ¿Qué puede pasar a páginas de datos?

Del bss residente de la versión por bancos (~6 KB): `sentencia` 1.370 B (texto de hasta 20 líneas), `lexer` 1.327 B
(de ellos 1.321 de `tmp`), `cadenas` 1.026 B, `stm_ast` 1.026 B, `ast` del driver de medida 1.026 B (solo existe en
`medir_core`; el compilador real no lo tiene), `stm` 34 B, `parser` 19 B. **Ninguna de las estructuras vivas debe
paginarse**: el lexer lee `sentencia.texto` carácter a carácter y el parser accede a los nodos del AST constantemente;
con un coste fijo por acceso paginado (~50-70 µs estimados por mapeo) sería inviable. Lo que sí ayuda es:
(1) las estructuras NUEVAS y grandes y de larga vida (tabla de símbolos, código intermedio, código objeto, texto fuente
del editor) nacen ya en páginas, accedidas en bloques; (2) reducir lo existente: `Lexer.tmp` (1.321 B) se puede acotar
(los nombres miden ≤ 8, los reales y cadenas se pueden internar sin copia previa), con el riesgo de tocar el lexer;
(3) sacar de la libreria C lo que no usemos (printf completo). Esas dos medidas son experimentos por hacer, no
decisiones tomadas.

## 6. Limitaciones conocidas

- Sin DO implícito en listas de entrada/salida, sin `READ`/`WRITE`/`FORMAT`, sin `CHARACTER*(*)`, sin `A(*)`,
  sin GOTO asignado, sin `REAL*8`, sin `FUNCTION`/`SUBROUTINE`/`CALL`/`COMMON`/`DIMENSION`/`PARAMETER`/`DATA`/`EXTERNAL`
  (capítulos siguientes). Salen como `STM_DESCONOCIDA`.
