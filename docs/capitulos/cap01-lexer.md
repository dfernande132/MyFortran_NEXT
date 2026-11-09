# Capítulo 1 — El lexer de Fortran 77

Estado: código completo, probado en PC (310 pruebas) y en el Next (mismo resultado), con medidas
(§8). Pendiente: contrastar los detalles con el texto del estándar (ver §9).

## 1. Qué hemos construido

Un programa que convierte **texto fuente** en **tokens**, en tres piezas:

| Pieza | Fichero | Entra | Sale |
|---|---|---|---|
| Lector de sentencias | `compiler/src/source.c` | texto del fichero | sentencias lógicas (etiqueta + texto) |
| Clasificador | `compiler/src/clasif.c` | una sentencia | su clase (`DO`, asignación, `IF`...) y desde dónde leer |
| Escáner | `compiler/src/lexer.c` | una sentencia | un token cada vez que se pide |

## 2. Qué es un lexer y dónde encaja

El compilador es una cadena de fases, cada una con una representación más cómoda que la anterior:

```
texto  →  [lexer]  →  tokens  →  [parser]  →  árbol  →  [semántico]  →  código de 3 direcciones  →  [generador]  →  Z80
```

El lexer agrupa caracteres en **tokens**: `X=A+2` son cinco tokens, `N:X  =  N:A  +  I:2`.
Descarta lo que no importa (blancos, comentarios) y detecta los errores más básicos
(un carácter que no pertenece al lenguaje, una cadena sin cerrar).

## 3. Lo que hace especial al lexer de Fortran 77

En C o en Pascal, el lexer es casi mecánico. En F77 no, por cuatro razones.

**3.1 Formato fijo.** El fuente es una tabla de columnas, herencia de las fichas perforadas:

| Columnas | Significado |
|---|---|
| 1 | `C`, `c` o `*`: toda la línea es un comentario |
| 1–5 | etiqueta de la sentencia |
| 6 | si no es blanco ni `0`: continúa la línea anterior |
| 7–72 | la sentencia |
| 73–80 | se ignoran |

**3.2 Los blancos no significan nada** fuera de las constantes de texto. `GO TO 10`, `GOTO10` y
`G O T O 1 0` son lo mismo, y `1 0 0` es el entero 100.

**3.3 Las palabras clave no están reservadas.** `IF = 3` y `END = 5` son asignaciones válidas.
Qué es una palabra clave y qué un nombre depende de la sentencia entera, no de la palabra.

**3.4 El punto es ambiguo.** `1.EQ.2` son tres tokens (entero, operador, entero), pero `1.E3`
es un real. Hay que mirar qué viene tras el punto antes de decidir.

La ambigüedad más famosa une las tres últimas:

```
DO 10 I = 1,10     →  un bucle DO
DO 10 I = 1.10     →  una asignación a una variable llamada DO10I
```

Sin blancos son `DO10I=1,10` y `DO10I=1.10`: lo único que los distingue es una **coma** a nivel
de paréntesis 0 tras el `=`.

## 4. Arquitectura

```
fuente ──▶ leer_sentencia ──▶ Sentencia ──▶ clasificar_sentencia ──▶ Clasificacion
              │                  │                                       │ (resto)
        clasificar_linea         └───────────────▶ lex_siguiente ◀───────┘
        iniciar_sentencia                              │
        anadir_continuacion                            ▼
                                                    Token ...
```

Decisiones de diseño que dan forma a esto (con su porqué en `docs/decisiones/`):

- **Interfaz "pull":** el escáner entrega un token cada vez que se le pide. No guardamos el array de
  tokens porque en el Next no cabría. (0001)
- **La sentencia conserva el texto original** y el escáner salta los blancos al leer con
  `mirar(k)` / `avanzar(n)`. Así cada carácter conserva su posición, que necesitan los errores y
  el resaltado de sintaxis del editor. Cada línea ocupa exactamente 66 caracteres (se rellena),
  de modo que la columna original sale de una cuenta, sin tabla. (0002)
- **Clasificación con reglas deterministas**, sin retroceso, antes de escanear. (0003)
- **Sin `malloc`**, tablas de tamaño fijo, `uint8_t`/`uint16_t`: es código que debe correr en un Z80.

## 5. Recorrido por el código

### 5.1 `source.c` — de líneas a sentencias

- `clasificar_linea`: ignorada (comentario o blanca), inicial o continuación. **Trampa:** la
  columna 6 es `linea[5]`, pero si la línea es corta, ese índice cae fuera de la cadena;
  `columna6()` recorre la línea comprobando el `'\0'`.
- `iniciar_sentencia`: lee la etiqueta (cols 1–5; los blancos no cuentan, `" 1 0 "` es 10) con
  `uint32_t` porque llega a 99999 y `uint16_t` solo a 65535, y copia las columnas 7–72.
- `anadir_continuacion`: rellena la línea anterior con blancos hasta 66 y añade la nueva.
- `posicion_fuente`: de un desplazamiento del texto a (línea, columna) del fichero:
  `linea_seg[p / 66]` y `p % 66 + 7`.
- `leer_sentencia`: salta comentarios, lee una sentencia inicial y le pega sus continuaciones.
  **Mira la línea siguiente antes de consumirla**: si resulta ser una sentencia nueva, no la toca.
  Tras un error sigue consumiendo las continuaciones para no encadenar errores falsos.

### 5.2 `lexer.c` — de caracteres a tokens

- **`TablaCadenas`:** guarda el texto de nombres, reales y cadenas; un token lleva solo un índice.
  Los textos iguales comparten posición, así que dos nombres son el mismo si y solo si tienen el
  mismo índice. Es una búsqueda lineal (simple; si pesa, un hash).
- **`mirar(k)` y `avanzar(n)`:** el k-ésimo carácter no blanco por delante (en mayúsculas), y
  consumir n de ellos. Todo lo demás se escribe encima de estas dos funciones.
- **Enteros:** el desbordamiento se comprueba *antes* de multiplicar por 10, comparando con
  `214748364` (= 2147483647 / 10) sin dividir, porque la división de 32 bits es cara en el Z80.
- **Reales:** no se convierten a número, se guarda su texto. Convertirlos depende de cómo
  representemos `REAL` (el `float` de z88dk son 6 bytes), decisión pendiente.
- **`abre_operador_punto`:** un `.`, una o más letras y otro `.` forman un operador; es la regla
  que separa `1.EQ.2` de `1.E3`.
- **Tras un error el escáner avanza**, así que un parser que pida tokens en bucle nunca se atasca.

### 5.3 `clasif.c` — qué es esta sentencia

Reglas, en este orden:

1. **Asignación:** `nombre {(...)}* =`. Cubre `IF(3)=1`, `END=5`, `A(1)(2:3)=...`.
2. **DO:** si lo anterior al `=` es `DO` + dígitos + nombre, y hay una coma fuera de paréntesis
   y de cadenas tras el `=`, es un bucle.
3. **Palabra clave** más larga de la tabla (distingue `END` de `ENDIF`, `DO` de `DOUBLEPRECISION`).
4. **IF:** tras el paréntesis de cierre, `THEN` y fin de sentencia → de bloque; un dígito →
   aritmético; otra cosa → lógico. (`IF(L)THENY=1` es lógico: asigna a la variable `THENY`.)

El clasificador devuelve `resto`, la posición desde la que debe seguir el escáner. En un `DO`
lee él mismo la etiqueta: si no, `DO10E=1,10` se leería como `10E` (un exponente incompleto).

## 6. Cómo probamos

`compiler/tests/test_cap01.c`, ejecutable con la tarea **"PC: tests (compilar y ejecutar)"**.
Cada caso se escribe en una línea y se lee de un vistazo:

```c
LEXAR("1.EQ.2",      "I:1 .EQ. I:2");
CLASIF("DO10I=1,10", "DO(10) | N:I = I:1 , I:10");
```

Tres capas de control, de menos a más exigente:

1. **Pruebas** (310 al cerrar el capítulo).
2. **Sanitizadores** (`-fsanitize=address,undefined`): detectan lecturas fuera de límites que
   un test normal no ve.
3. **Mutaciones:** se rompe el código a propósito y se comprueba que algún test falla. Si una
   mutación sobrevive, el test no protege esa línea. Hicimos tres rondas (11, 19 y 15
   mutaciones). En la última, una sobrevivió (`saltar_grupo` ignorando las cadenas): el test
   que existía daba el mismo resultado con o sin esa lógica. Se añadieron casos que distinguen.

## 7. Errores que cometimos y lo que enseñan

- **Tests que se equivocan, no el código:** `"END"` en la columna 1 no es una sentencia (la `E`
  cae en la zona de la etiqueta); una línea con solo tres espacios tras `AB` pone la `X` en la
  columna 6 y es una continuación. *Un test que falla no siempre señala un bug en el código.*
- **Error de uno en el borde:** con `i < 5` en lugar de `i < 6`, la línea de exactamente cinco
  caracteres se clasificaba como continuación. Solo falla ahí.
- **La etiqueta del DO** (`DO10E=1,10`): lo encontramos revisando el diseño, no con un test.

## 8. Medidas en el Next

Se miden con las tareas **"PC: medir nucleo"** y **"Next: medir nucleo"** y se analizan con
`python tools/analizar_map.py build/next/core.map`. Compilado con los flags base de MyLISP y
**sin ninguna optimización** (es la línea base). El programa de medición escribe lo mismo en PC
y en el Next: `sentencias=13 tokens=37 errores=1 desconocidas=0 tabla=47` (confirmado por el autor
en el Next).

Dos mediciones: la primera con `MAX_SEGMENTOS` = 4 y sin tabuladores (cierre inicial), la
segunda con `MAX_SEGMENTOS` = 20 y tabuladores (cierre definitivo). Mismo compilador y flags.

| Medida | 4 segmentos | 20 segmentos + tabs |
|---|---|---|
| Imagen binaria (`build/next/core`) | 15.896 | **18.210** |
| **Margen** (32767 − imagen; también es el hueco de pila) | 16.871 | **14.557** (MyLISP v1.0: ~848) |
| **Nuestro código** (`code_compiler`) | 8.581 | 8.751 |
| — `lexer.c` / `clasif.c` / `source.c` / `main` de prueba | 3.920 / 2.525 / 1.710 / 284 | 3.920 / 2.525 / 1.880 / 284 |
| **Nuestros datos estáticos** (`bss_compiler`) | 1.670 | 3.814 |
| — `tabla` / `sentencia` / `lexer` / `fuente` / `clasif` | 1.026 / 282 / 275 / 79 / 8 | 1.026 / 1.370 / 1.331 / 79 / 8 |
| Nuestro rodata (tablas y cadenas) | 821 | 821 (~330 son el fuente de prueba incrustado) |
| **Total nuestro** | 11.072 | **13.386** |
| Sistema (arranque, runtime sccz80, librería C con `printf`, fuente 4×8 de 768 B) | 4.754 | 4.754 |

Coste del cambio: +2.314 bytes de imagen = +2.144 de bss (`Sentencia` y `Lexer.tmp`, como se
estimó: ~+2,1 KB) + 170 de código (casi todo `tomar_linea` con los tabuladores, 275 B). La suma
de nuestras partes y el sistema deja ~70 bytes sin atribuir, igual que en la primera medición.

Cómo leerlo, y qué **no** está medido:

- El front-end (`source` + `lexer` + `clasif`) ocupa ~8,1 KB de código. Es más de la mitad de la
  imagen, y todavía no hay parser, semántico, código intermedio ni generador. *Hipótesis sin
  probar:* el compilador completo no cabrá en 32 KB residente y habrá que pasar a overlays desde
  la SD (etapa B). Sirve como motivo para seguir midiendo en cada capítulo.
- Los 8,6 KB de código salen de **sccz80 sin optimizar**. Ninguna función de nuestro código es
  recursiva, así que `static` en las variables locales sería seguro (docs/referencia, §6.3, lo
  da como una de las técnicas que más ahorran). **No se ha medido** cuánto; se deja como experimento
  de una sola variable por compilación cuando haga falta recortar.
- Tras subir a 20 segmentos, `Sentencia.texto` (1.321) y `Lexer.tmp` (1.321) son el 69 % de los datos
  estáticos, y la `TablaCadenas` (1.026) el 27 %. `Lexer.tmp` es el candidato claro a eliminar si
  el margen aprieta (copiar el lexema en sitio, o limitar su tamaño a una cadena, no a una sentencia).
  El margen de 14,5 KB es lo que queda para parser, semántico, código intermedio y pila.
- **El uso de pila no está medido.** La cadena de llamadas más profunda del front-end es corta
  (`clasificar_sentencia` → `saltar_grupo` → `sig` → `lex_saltar_blancos`), y con 14 KB de margen
  no hay riesgo hoy; sí lo habrá cuando el margen baje de unos cientos de bytes.
- Las cifras de rodata son un máximo: las cadenas literales no tienen símbolo propio y se suman a
  la tabla anterior (por eso `_OPERADORES_PUNTO` sale con 310 bytes, cuando la tabla pesa ~65).
- Una lección de método: la primera versión del análisis calculaba el tamaño de cada función hasta
  la siguiente *etiqueta interna* `i_NN` y dio 1.460 bytes de código en lugar de 8.439. Lo avisaba
  el documento de MyLISP (§2.4) y se nos pasó. El script `tools/analizar_map.py` ya lo descarta.

## 9. Pendiente y límites conocidos

- **Contraste con el estándar (ANSI X3.9-1978)**, hecho sobre `docs/referencia/ANSI_X3.9-1978_F77.txt`
  (el PDF mezcla páginas Full y Subset; ojo al leerlo):
  - Confirmado: coma opcional tras la etiqueta del DO (§11.10); `ELSE IF (e) THEN` solo existe de
    bloque; etiqueta de 1 a 5 dígitos con al menos uno distinto de cero; continuación en col. 6
    con cols. 1-5 en blanco; líneas de comentario permitidas entre inicial y continuaciones;
    nombres simbólicos de 1 a 6 caracteres; no hay palabras reservadas (§3.1.5: "no hay secuencia
    reservada en todos los contextos"); blancos sin efecto salvo en constantes de carácter y en
    H/apóstrofe de FORMAT (§3.3); la constante de carácter debe tener longitud > 0 (§4.8.1);
    constante real = básica, básica+exponente E, o entero+exponente E (`1E3` es real; `1.`
    y `.5` válidas, no ambas partes vacías); el exponente D es de doble precisión (Full).
  - Límites: Full admite 19 continuaciones / 1320 caracteres; Subset 9 / 660. Tras el análisis
    de Dani se subió `MAX_SEGMENTOS` de 4 a 20 (= Full). Coste de RAM estimado +2,1 KB
    (`Sentencia.texto` y `Lexer.tmp`); **pendiente de medir en el Next** (`Next: medir nucleo`).
  - Desviaciones a propósito: nombres de hasta 8 caracteres (el estándar, 6), minúsculas,
    `''` aceptada.
  - Sin determinar aún: relleno hasta la col. 72 de una cadena partida en continuación (no
    aparece en el texto OCR; mirar §3.2.3 en imagen), gramática exacta de función-sentencia
    frente a elemento de array, alcance de Hollerith (Apéndice C).
- `REAL FUNCTION F(X)` se clasifica mal hasta el capítulo de subprogramas (ver 0003).
- `//` y `/` se leen siempre como concatenación y división; en `COMMON //A` y `DATA /1,2/` no lo
  son. Lo resolverá el parser por contexto.
- `FORMAT(5HHOLA X)` (Hollerith): el escáner no lo trata todavía.
- La cadena vacía `''` se acepta aunque el estándar exige longitud > 0 (§4.8.1); decidir si se convierte en error.
- Tabuladores (decisión 0004): se expanden al leer solo en las columnas 1-6; en el cuerpo valen
  un blanco y un carácter para el cálculo de columnas.
- Pospuestos tras el análisis de Dani (con su capítulo destino): tabla hash para `TablaCadenas`
  (semántico, tras medir con ~500 identificadores); `REAL FUNCTION` con indicador "primera
  sentencia de la unidad" (subprogramas); `FORMAT`/Hollerith en modo crudo (E/S); `TABLA_TAM`
  se queda en 1 KB (error limpio con `TABLA_LLENA`); `uint32_t` de la etiqueta se mantiene.
- `mirar(k)` cuesta O(distancia); el clasificador es O(n). No medido en el Z80.

## 10. Ejercicios

1. En `lexer.c`, cambia `d > 7` por `d > 8` y mira qué tests fallan. ¿Por qué solo esos?
2. Traza a mano `clasificar_sentencia` con `DO10I=F(1,2),3` y con `DO10I=F(1,2)`.
3. Escribe el test de `IF(A.EQ.')')THEN` en papel y predice el resultado antes de ejecutar.
4. ¿Qué cambiaría si `mirar` no pasara a mayúsculas? ¿Y si no saltara los tabuladores?
5. Añade `WRITE` a la tabla de `clasif.c`. ¿Qué otros módulos habría que tocar?
