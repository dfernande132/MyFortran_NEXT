# Capítulo 2 - Expresiones y AST

Estado: **cerrado**. Código y 255 pruebas en PC (ASan/UBSan, 8 mutaciones muertas, 300.000 casos aleatorios
sin fallos de memoria). Tamaño medido en el Next; **pendiente la pila** (sección 7).

## 1. Qué hemos construido

- `ast.h/.c`: el árbol, en un pool de 128 nodos de 8 bytes con índices de 16 bits.
- `expr.h/.c`: el analizador de expresiones (precedence climbing) y su tabla de operadores.
- `tests/test_cap02.c`: 255 pruebas que comparan el árbol, escrito como expresión-S, con el esperado.
- `tools/medir/medir_core.c`: ahora también analiza expresiones y, solo en el Next, mide la pila.

## 2. Qué es un parser de expresiones

El lexer da una lista plana de tokens (`A + B * C`); el parser le da **estructura**: decide que
`B * C` se agrupa antes que la suma. La estructura se guarda como un árbol:

```
A+B*C     ->  (+ A (* B C))          -A**2   ->  (neg (** A 2))
```

## 3. Precedence climbing, paso a paso

Cada operador tiene una precedencia (número) y se sabe si asocia por la izquierda o la derecha:

| Prec. | Operadores | Notas |
|---|---|---|
| 1 | `.EQV.` `.NEQV.` | izquierda |
| 2 | `.OR.` | |
| 3 | `.AND.` | |
| 4 | `.NOT.` | unario |
| 5 | `.EQ.` `.NE.` `.LT.` `.LE.` `.GT.` `.GE.` | **no asociativos** |
| 6 | `//` | |
| 7 | `+` `-` | el signo unario va en este nivel |
| 8 | `*` `/` | |
| 9 | `**` | **derecha** |

La función `expresion(min)` hace esto: lee un operando; mientras el siguiente operador tenga
precedencia `>= min`, lo consume y analiza el operando derecho con `min = prec + 1` (asociativo por la
izquierda) o `min = prec` (por la derecha). Con `A-B-C`: tras `A-`, el derecho se pide con `min = 8`,
no puede absorber el segundo `-` (prec 7), así que vuelve y el bucle lo aplica sobre `(A-B)`.
Con `A**B**C`: el derecho se pide con `min = 9` y SÍ absorbe el segundo `**`.

## 4. Reglas del estándar que hemos tenido que respetar

- **Signo unario** solo al principio de una expresión o tras `(` o `,`. `A*-B` y `A**-B` son error.
  El signo abarca el término: `-A**2` es `-(A**2)`, y `-A*B` es `-(A*B)`.
- **`.NOT.`** liga más flojo que los relacionales: `.NOT. A .EQ. B` es `.NOT. (A .EQ. B)`.
  Solo cabe donde cabe un operando de `.AND.`: `A .EQ. .NOT. B` es error.
- **Relacionales no asociativos**: `A .LT. B .LT. C` es error.
- **`A(I)` es ambiguo** (elemento, función, función-sentencia). Siempre `NODO_REF`; el semántico decide.

## 5. El AST en un pool fijo

Un nodo = `{tipo, tdato, a, b, pos}` = 8 bytes. `a` y `b` significan cosas distintas según el tipo
(ver `ast.h`). Los argumentos de `F(X,Y)` forman una lista de celdas `NODO_LISTA`. El pool se reinicia
por sentencia. Si se llena: error "expression too large". Si algún día pesa, se emitirá código de 3
direcciones directamente (decisión 0005).

## 6. La salvaguarda de recursión

Cada llamada a `expresion` incrementa un contador; al llegar a `EXPR_PROF_MAX` (48) es un error. Un
paréntesis anidado cuesta 1 nivel más los de los operadores de dentro (hasta 3 con `+`, `*`, `**`).
Las cadenas largas a la izquierda (`A+A+A+...`) son un bucle y no consumen profundidad.

## 7. Medidas en el Next

Mismo método que en el capítulo 1 (sccz80 sin optimizar, `.map` analizado con `tools/analizar_map.py`).

| Medida | Cap. 1 (cierre) | Cap. 2 |
|---|---|---|
| Imagen binaria | 18.210 | **23.608** |
| **Margen** (32767 − imagen) | 14.557 | **9.159** |
| `expr.c` código / rodata | — | 2.677 / 334 (casi todo son los mensajes; la tabla `OPERADORES` son 48) |
| `ast.c` código | — | 238 |
| `ast` + `parser` (bss) | — | 1.026 + 21 |
| `medir_core.c` (código de prueba, no es del compilador) | 284 | 1.142 |

Del salto de +5.398 bytes, ~4,3 KB son del compilador (expr 2.677 + ast 238 + bss 1.047 + rodata 334) y
~1,1 KB es el programa de medición.

**La estimación previa estaba mal.** Se dijo "~0,6–0,9 KB de código" para la opción B; el real es
**2.915 bytes (expr + ast), unas 3,5 veces más**. Funciones mayores: `expresion` 456, `primario` 419,
`argumentos` 343, `operando` 305, `referencia` 277. No se ha investigado el motivo (hipótesis: el acceso
a `p->campo` y a `p->ast->nodo[i]` por IX en sccz80 sin optimizar). El resto de estimaciones de este
capítulo (pila) hay que tomarlas con la misma cautela.

**Pila** (medida con 4, 24 y 44 paréntesis anidados, pintando con `0xA5` la memoria bajo la pila):

| Paréntesis | Pila usada (bytes) |
|---|---|
| 4 | 340 |
| 24 | 1.160 |
| 44 (peor caso, `EXPR_PROF_MAX` = 48) | 1.980 |

Pendiente constante: (1.160 − 340) / 20 = (1.980 − 1.160) / 20 = **41 bytes por nivel**, lineal. Fijo: ~176 bytes
(parser, lexer y llamadas de la prueba). La estimación previa de ~30 era baja; la reconstrucción de ~47 se
acercó. El peor caso de 48 niveles se come **2 KB de los 9 KB de margen**. En MyLISP se midieron ~44 bytes por
nivel de `Eval`. Un programa F77 real rara vez anida más de ~8 paréntesis: 48 es muy generoso (decisión pendiente).

**Experimento 1: `Parser` global** (una variable por compilación). Línea base antes del cambio, con la misma
medida de pila: imagen 23.745, margen 9.022, `expr.c` 2.677 bytes de código.
Resultado: `expr.c` **1.841** bytes de código (−836, −31 %), imagen **22.900**, margen **9.867**
(+845). Pila con el parser global:

| Paréntesis | Antes | Después |
|---|---|---|
| 4 | 340 | 298 |
| 24 | 1.160 | 998 |
| 44 | 1.980 | 1.698 |

Pendiente: 41 → **35 bytes por nivel** (−15 %); fijo ~176 → ~158. Peor caso: −282 bytes.
Regla que sale de aquí: el estado compartido de cada fase va en una variable global.

**Experimento 2: `Lexer` global** (y `EXPR_PROF_MAX` 48 → 24, que no cambia el código). Línea base: el
resultado del experimento 1.

| | Antes | Después |
|---|---|---|
| `lexer.c` código | 3.920 | **3.330** (−590, −15 %) |
| `clasif.c` código | 2.525 | **2.263** (−262, −10 %) |
| `expr.c` código | 1.841 | 1.826 |
| Imagen | 22.900 | **21.956** (−944) |
| Margen | 9.867 | **10.811** |
| Pila (4 / 12 / 20 paréntesis) | — | 290 / 570 / 850 |

La pila no cambia de pendiente (**35 bytes por nivel**, igual que antes) y el coste fijo baja de ~158 a ~150:
la recursión del parser de expresiones no pasa por el lexer, así que era de esperar. Con `EXPR_PROF_MAX` = 24, el
peor caso son **850 bytes de pila** (antes 1.980 con 48). Acumulado de los dos experimentos: imagen 23.745 → 21.956,
margen 9.022 → 10.811. Decisión: `docs/decisiones/0006-estado-global.md`.

**Experimento 3: `Sentencia` global.** Línea base: el resultado del experimento 2.

| | Antes | Después |
|---|---|---|
| `source.c` código | 1.880 | **1.671** (−209, −11 %) |
| `lexer.c` código | 3.330 | 3.309 |
| Imagen | 21.956 | **21.699** (−257) |
| Margen | 10.811 | **11.068** |
| Pila | 290 / 570 / 850 | sin cambio |

Acumulado de los tres experimentos: imagen 23.745 → 21.699 (−2.046 bytes), margen 9.022 → 11.068. Cada experimento
da menos que el anterior (−836, −944, −257). Pendiente: `TablaCadenas` global, `Lexer.tmp` (1.321 bytes de datos),
`AST_NODOS`.

**Experimento 4: `TablaCadenas` global.** Línea base: el resultado del experimento 3.

| | Antes | Después |
|---|---|---|
| Imagen | 21.699 | **21.630** (−69) |
| Margen | 11.068 | **11.137** |
| Pila | 290 / 570 / 850 | sin cambio |

Balance de los cuatro experimentos: imagen 23.745 → 21.630 (**−2.115 bytes**, −8,9 %), margen 9.022 → 11.137.
Los rendimientos son decrecientes (−836, −944, −257, −69): el ahorro estaba en los módulos con más accesos por
puntero. Se cierra la serie.

## 8. Pendiente y límites

- El valor de `EXPR_PROF_MAX` y `AST_NODOS` es provisional.
- `A*-B` se rechaza aunque g77 lo acepta como extensión (decisión 0005).
- Sin recuperación de errores: el primer error de la sentencia gana.
- Las constantes no se convierten ni se pliegan; el semántico lo hará.

## 9. Ejercicios

1. Cambia `PREC_POW` para que `**` asocie por la izquierda y mira qué tests fallan.
2. Traza `A.OR.B.AND.C` a mano con la tabla de precedencias.
3. ¿Por qué `.NOT.` se analiza con `min = PREC_REL` y no con `PREC_NOT + 1`?
4. Añade un operador (por ejemplo `.XOR.`, una extensión habitual). ¿Cuántos ficheros tocas?

## Nota (2026-10-30, capítulo 5): profundidad de referencias

La medida de pila de este capítulo solo usaba paréntesis anidados. Un nivel de argumentos (`F(...)`, `A(...)`) recorre tres
funciones más y pesa el doble en la pila del Z80 (~74 B frente a ~38 B), de modo que 22 niveles desbordaron la pila libre en
el Next. Cada nivel de argumentos cuenta ahora doble en `parser.prof`: con `EXPR_PROF_MAX = 24` caben 11 niveles de
`F(F(...))`. Ver `docs/capitulos/cap05-semantico.md`, quinta ejecución.
