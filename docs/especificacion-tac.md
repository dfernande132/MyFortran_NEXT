# Especificacion del codigo de tres direcciones (TAC) de MyFortran

Version **0.2** - 2026-10-30. Documento normativo del codigo intermedio. Implementado en `compiler/src/tac.c`, `gen.c` y
`tac_lst.c`, y comprobado con el interprete `platform/pc/tacvm.c` (`compiler/tests/test_cap06.c`); los ejemplos de la seccion 9
son salida real del generador. Es independiente de la maquina: otro compilador que produzca este TAC, o otro backend (otro ordenador retro) que lo consuma, solo necesita este documento.
El razonamiento y las alternativas descartadas estan en `docs/decisiones/0011-codigo-intermedio.md`.
Lo que NO esta decidido figura en la seccion 10.

Convencion de este documento: "DEBE" = obligatorio para conformidad; "PUEDE" = opcional.

## 1. Modelo de maquina abstracta
- **Memoria estatica.** Cada unidad de programa tiene todas sus variables en posiciones fijas: no hay pila de datos ni recursion.
  Los argumentos de subprogramas se pasan POR REFERENCIA (reservado, seccion 8).
- **Tipos** (campo `tipo` de la instruccion):

| Tipo | Codigo | Representacion | Estado |
|---|---|---|---|
| `INTEGER` | 1 | 16 bits en complemento a dos (-32768..32767); el desbordamiento da la vuelta (modulo 65536) | v0.1 |
| `REAL` | 2 | IEEE 754 simple por software, 4 bytes (decision 0013); sin denormales, infinitos ni NaN | v0.2 |
| `DOUBLE` | 3 | reservada | reservado |
| `LOGICAL` | 4 | 0 = falso, 1 = verdadero. Toda operacion que produce un LOGICAL produce 0 o 1 | v0.1 |
| `CHARACTER` | 5 | cadena de longitud fija, rellena con blancos (ASCII); los temporales llevan ademas su longitud real | v0.1 |

  Los codigos coinciden con `TipoDato` (`compiler/src/sim.h`). `0` = sin tipo (instrucciones de control).
- **Valores con nombre.**
  - *Variables*: simbolos del programa fuente (escalares y arrays).
  - *Temporales* `%n`: valores intermedios de UNA sentencia; el generador los numera desde 0 en cada sentencia. Su tipo lo
    da la instruccion que los define. Un temporal CHARACTER DEBE declararse con `TMP` antes de usarse.
  - *Ocultas* `$n`: variables del generador que viven mientras dure una construccion (contadores de DO). 2 B (INTEGER).
  - *Etiquetas*: `@n` (etiqueta de usuario, n = 1..99999 tal como en el fuente) y `@@n` (internas del generador).
- **Orden de evaluacion**: el TAC esta ya secuenciado; el orden de las instrucciones ES el orden de evaluacion.
- **Errores en ejecucion**: la division (`DIV`, `DIV.R`) por cero, 0/0 y el desbordamiento REAL (`ADD/SUB/MUL/DIV` con REAL) PARAN el programa
  con un mensaje (`Division by zero`, `0/0 is indeterminate`, `REAL overflow`; decision 0013). Indices fuera de rango: comportamiento NO
  definido en v0.1; el backend PUEDE comprobarlo y detener el programa (seccion 10).

## 2. Unidad TAC
Una unidad = cabecera + declaraciones + instrucciones, en este orden. En v0.1 hay UNA unidad por compilacion (`PROGRAM`).

```
UNIT PROGRAM <nombre>
VAR <nombre> <tipo>[*<longitud>] [ (<lo>:<hi>[,<lo>:<hi>]) ]
...
<instrucciones>
```
- `UNIT` abre la unidad (`PROGRAM`; `SUBROUTINE`/`FUNCTION` reservados).
- `VAR` declara una variable. `INTEGER`, `LOGICAL`, `CHARACTER*n` (n = 1..255). Con dimensiones es un array de 1 o 2 dimensiones,
  limites enteros constantes con `lo <= hi`. Un array 2-D se guarda **por columnas** (el primer indice varia mas deprisa).
  Las variables NO declaradas por el usuario (tipado implicito I-N) tambien aparecen como `VAR`.
- La unidad termina con la ultima instruccion; todo camino de ejecucion DEBE acabar en `HALT` o `STOP`.

## 3. Operandos
| Clase | Texto | Significado |
|---|---|---|
| NADA | (vacio) | sin operando |
| TEMP | `%0` `%1` ... | temporal |
| VAR | nombre (`S`, `A`) | variable (en binario: indice de simbolo) |
| INM | `-5` `100` | entero con signo de 13 bits (-4096..4095) |
| CTE | `70000`, `'texto'`, `.T.`, `.F.` | constante que no cabe como INM: entero grande, cadena, logico |
| ETQ | `@10`, `@@3` | etiqueta (en binario, indice de la tabla de etiquetas de la unidad; ver seccion 5) |
| OCULTA | `$0` `$1` | variable oculta |

En el texto, el codificador elige INM o CTE segun el valor; el programador del backend ve siempre "un entero". Las constantes
de cadena se escriben entre apostrofos con `''` para el apostrofo. Un LOGICAL constante es `.T.`/`.F.` (siempre CTE).

## 4. Formato textual (listado)
Una instruccion por linea; los comentarios empiezan por `;`. Forma:

```
<MNEMONICO>[.<T>] <operando>[, <operando>[, <operando>]]
```
donde `.T` es el tipo: `.I` INTEGER, `.R` REAL, `.D` DOUBLE, `.L` LOGICAL, `.C` CHARACTER. Las operaciones aritmeticas, `MOV`,
`PUT`, los relacionales, `LDE`/`STE` exigen tipo; el resto no lo lleva. El primer operando es el DESTINO cuando la operacion
produce un valor (como en ensamblador de tres direcciones: `ADD.I d, a, b` = `d := a + b`).

## 5. Codificacion binaria (v0.1)
Registro de **8 bytes, little endian**:
```
 byte 0   byte 1   bytes 2-3   bytes 4-5   bytes 6-7
   op      tipo        a           b           c
```
Cada operando (a, b, c) es de 16 bits: bits 15-13 = clase, bits 12-0 = valor (0..8191).
Clase: 0 NADA, 1 TEMP, 2 VAR, 3 INM (valor en complemento a dos de 13 bits), 4 CTE (indice del pool de constantes),
5 ETQ, 6 OCULTA, 7 reservada.

- **VAR**: indice del simbolo en la tabla de simbolos de la unidad (1..512).
- **ETQ**: valores 1..4095 = indice (1..256) de la etiqueta de usuario en la **tabla de etiquetas** de la unidad (que guarda el
  numero real, 1..99999: asi cualquier etiqueta cabe en 13 bits); valores >= 4096 = etiqueta interna del generador `@@(v-4096)`.
  Quien consuma el binario necesita esa tabla solo para volver a mostrar el numero original; los saltos funcionan con los indices.
- **CTE**: desplazamiento / 2 dentro del pool de constantes. Cada entrada: `tipo` (1 B: 1 entero, 2 cadena, 3 logico),
  `longitud` (1 B), datos (entero: 2 B little endian; logico: 1 B 0/1; cadena: `longitud` bytes sin terminador), relleno a
  longitud par. No se eliminan duplicados.

**REAL.** Una constante REAL del pool (tipo 4) ocupa 4 bytes: el float IEEE 754 simple, little endian (decision 0013). `ITOF d,x`
(`d` REAL, `x` INTEGER) y `FTOI d,x` (`d` INTEGER, `x` REAL; trunca hacia cero y satura a -32768..32767) llevan en `tipo` el tipo del
DESTINO. En una operacion de modo mixto el generador inserta `ITOF` en el operando entero; en una asignacion convierte con `ITOF` o
`FTOI` al tipo del destino. Los relacionales llevan en `tipo` el tipo de los operandos ya convertidos (REAL si alguno lo era).
En el listado una constante real se escribe con 9 cifras significativas (`1.5`, `0.100000001`).

Disposicion de operandos por instruccion (`op` / `a` / `b` / `c`): `LBL L`: a. `MOV d,s`: a, b. Aritmeticas y relacionales
`op d,x,y`: a, b, c. `NEG/NOT d,x`: a, b. `JMP L`: a. `JT/JF x,L`: a, b. `JEQ x,y,L`: a, b, c. `LDE d,A,off`: a, b, c.
`STE A,off,v`: a, b, c. `TMP t,len`: a, b. `SCAT d,x,y` / `SSUB d,x,i`: a, b, c (`EXT j` lleva j en a). `PUT x`, `STOP [x]`: a.

| Op | Mnemonico | Op | Mnemonico | Op | Mnemonico |
|---|---|---|---|---|---|
| 0 | `NOP` | 12 | `LE` | 24 | `LDE` |
| 1 | `LBL` | 13 | `GT` | 25 | `STE` |
| 2 | `MOV` | 14 | `GE` | 26 | `SCAT` |
| 3 | `NEG` | 15 | `AND` | 27 | `SSUB` |
| 4 | `ADD` | 16 | `OR` | 28 | `SLEN` |
| 5 | `SUB` | 17 | `EQV` | 29 | `PUT` |
| 6 | `MUL` | 18 | `NEQV` | 30 | `PUTNL` |
| 7 | `DIV` | 19 | `NOT` | 31 | `STOP` |
| 8 | `POW` | 20 | `JMP` | 32 | `HALT` |
| 9 | `EQ` | 21 | `JT` | 33 | `TMP` |
| 10 | `NE` | 22 | `JF` | 34 | `EXT` |
| 11 | `LT` | 23 | `JEQ` | 35 | `ITOF` |
| | | | | 36 | `FTOI` |
| | | | | 37 | `RFUN` |
| | | | | 38 | `RMOD` |
| | | | | 39-43 | reservados: `ENTER PARAM CALL RET FUNR` |
| 40 | `ITOF` | 41 | `FTOI` | | |

40-255 reservados. La numeracion es **provisional**: no fija la ABI hasta cerrar la v1.0 (seccion 10).

## 6. Instrucciones
`d` destino, `a b c` fuentes, `L` etiqueta, `T` tipo de la instruccion. Salvo indicacion, `a`/`b` pueden ser variable, temporal,
oculta o constante.

### Datos y aritmetica
| Instruccion | Semantica |
|---|---|
| `MOV.T d, a` | `d := a`. `.C`: copia con relleno de blancos o truncado a la longitud de `d` (asignacion F77). |
| `NEG.I d, a` | `d := -a` |
| `ADD.I d, a, b` / `SUB.I` / `MUL.I` | suma, resta, producto de 16 bits (modulo 65536) |
| `DIV.I d, a, b` | cociente entero, truncado hacia cero (`-7/2 = -3`) |
| `POW.I d, a, b` | `a**b` con exponente entero. `b >= 0`: producto repetido (`a**0 = 1`). `b < 0`: `1/(a**-b)` truncado: `1` si `a = 1`; `1` o `-1` (segun la paridad de `b`) si `a = -1`; `0` en otro caso (`a = 0` no definido) |

### Logica y relacionales (resultado LOGICAL)
| Instruccion | Semantica |
|---|---|
| `EQ.T d, a, b` `NE` `LT` `LE` `GT` `GE` | `d := (a op b)`. `.I`: enteros con signo. `.C`: orden ASCII con la cadena mas corta rellena con blancos |
| `AND.L d, a, b` `OR` `EQV` `NEQV` | operaciones logicas sobre 0/1 |
| `NOT.L d, a` | `d := (a = 0)` |

### Control de flujo
| Instruccion | Semantica |
|---|---|
| `LBL L` | define la etiqueta `L` en este punto (no ejecuta nada) |
| `JMP L` | salto incondicional |
| `JT a, L` / `JF a, L` | salta si `a` (LOGICAL) es 1 / 0 |
| `JEQ a, b, L` | salta si `a = b` (INTEGER) |
| `STOP [a]` | detiene el programa; `a` (opcional, INM o cadena CTE) es el argumento del `STOP` de F77 y se imprime `STOP <a>` |
| `HALT` | fin normal del programa (`END` alcanzado) |

### Arrays
El generador calcula el desplazamiento: `desp = (i1 - lo1) + (i2 - lo2) * (hi1 - lo1 + 1)` para 2-D, `desp = i1 - lo1` para 1-D
(0-based, en ELEMENTOS). Esas cuentas se emiten como `SUB.I`/`MUL.I`/`ADD.I` normales. Luego:

| Instruccion | Semantica |
|---|---|
| `LDE.T d, A, a` | `d := A[desp = a]` (el tipo es el del array; `.C` copia `n` bytes) |
| `STE.T A, a, b` | `A[desp = a] := b` |

### CHARACTER
| Instruccion | Semantica |
|---|---|
| `TMP.C %n, len` | declara el temporal `%n` CHARACTER con capacidad maxima `len` (1..255) |
| `SCAT.C d, a, b` | `d := a // b` (longitud real = suma, truncada a la capacidad de `d`) |
| `SSUB.C d, a, i` + `EXT j` | `d := a(i:j)`, 1-based; si `j < i` el resultado tiene longitud 0. `EXT` es un registro continuacion con el cuarto operando en `a` |
| `SLEN.I d, a` | `d :=` longitud real de `a` (para un temporal) o la declarada (para una variable) |

`EXT x`: no ejecuta; aporta el operando extra de la instruccion anterior (DEBE ir inmediatamente despues).

### Entrada/salida (PRINT de lista, unidad por defecto)
| Instruccion | Semantica |
|---|---|
| `PUT.T a` | escribe un elemento de la lista de salida (`.I` entero, `.L` logico, `.C` cadena) |
| `PUTNL` | fin de registro (salto de linea) |

Formato (convencion de la implementacion de referencia; PUEDE variar entre backends): un blanco antes de cada elemento,
enteros en decimal con `-` si es negativo, logicos como `T`/`F`, cadenas tal cual (sin comillas). Una lista vacia imprime solo
el fin de registro.

## 7. Reglas de generacion (normativas para el front-end)
1. **Expresiones**: se evaluan en orden con temporales; un nodo cuyo resultado va directamente a una variable PUEDE escribir en
   ella (`S = S + I` -> `ADD.I S, S, I`). Los parentesis no generan codigo.
2. **Asignacion**: `V = e` -> calcula `e` y `MOV.T V, <resultado>` (o escritura directa). `A(i) = e` -> desplazamiento + `STE`.
3. **IF logico** `IF (c) S` -> `<c>; JF c, @@k; <S>; LBL @@k`.
4. **IF de bloque / ELSE IF / ELSE / END IF**: cada rama tiene su `JF` a la siguiente; cada rama, salvo la ultima, acaba en `JMP`
   al `END IF`.
5. **IF aritmetico** `IF (e) l1,l2,l3` -> `LT.I %0, e, 0; JT %0, @l1; EQ.I %0, e, 0; JT %0, @l2; JMP @l3`.
6. **GOTO** `GOTO n` -> `JMP @n`. **GOTO calculado** `GOTO (l1,...,lk), e` -> por cada i: `JEQ e, i, @li`; si `e` no esta en 1..k el
   programa continua con la sentencia siguiente.
7. **DO** `DO n V = i, f, p` (`p` = 1 si falta):
```
        MOV.I  $s, <p>
        SUB.I  %0, <f>, <i>
        ADD.I  %0, %0, <p>
        DIV.I  $c, %0, <p>     ; iteraciones: max(0, (f - i + p) / p), calculado UNA vez
        MOV.I  V, <i>
        LBL    @@t
        GT.I   %0, $c, 0
        JF     %0, @@e
        ...cuerpo...
        LBL    @n              ; la sentencia terminal (con su etiqueta de usuario)
        ADD.I  V, V, $s
        SUB.I  $c, $c, 1
        JMP    @@t
        LBL    @@e
```
   Los tres valores `i`, `f`, `p` se evaluan UNA vez, en ese orden, y el conteo se calcula ANTES de asignar la variable (asi
   `DO I = 1, I + 2` usa el valor anterior de `I`). Con operandos constantes el generador pliega el conteo (ver el ejemplo 9.1);
   si `i` y `p` son el mismo entero, `f - i + p = f`. El paso cero constante es un error de compilacion
   (`DO step must not be zero`); en ejecucion no esta definido. Las variables ocultas `$s`/`$c` se numeran por bucle (dos por
   bucle abierto; se reutilizan al cerrarse). El conteo se calcula en aritmetica de 16 bits: un bucle cuyo rango (`f - i + p`)
   supere 32767 da resultados incorrectos (limitacion de INTEGER de 16 bits, seccion 10).
8. **STOP** -> `STOP [arg]`; **END** del programa -> `HALT`; **CONTINUE** -> nada (o `LBL` si lleva etiqueta).
9. **PRINT lista**: un `PUT.T` por elemento, en orden; luego `PUTNL`. **PRINT 'texto'** -> `PUT.C 'texto'; PUTNL`. `PRINT` con
   etiqueta de FORMAT, o `PRINT 'texto', lista`, aun no se genera (error de compilacion).
10. **Etiquetas de usuario**: toda sentencia con etiqueta genera `LBL @n` antes de su codigo, aunque nadie salte a ella. La
    sentencia terminal de un DO (`10 CONTINUE`, `10 S = S + 1`, `10 IF (...) ...`) lleva su `LBL`, su codigo, y DESPUES el cierre
    del bucle; varios DO que terminan en la misma etiqueta se cierran del mas interno al mas externo.
11. **Plegado de constantes** (PUEDE): el generador pliega operaciones entre enteros INM en el calculo de desplazamientos de
    arrays y en el conteo del DO. Fuera de eso no optimiza.
12. **Temporales**: se numeran desde 0 en cada sentencia (y en cada elemento de un PRINT); `TMP.C` declara los CHARACTER, con la
    capacidad calculada por el generador (constante: su longitud; variable: la declarada; concatenacion: suma, tope 255;
    subcadena de limites constantes: `j - i + 1`; si no, la de la cadena base).
13. **Varias unidades por fichero** (S1): lo que sigue a un `END` abre una unidad nueva (ver seccion 8).

## 8b. Datos iniciales y entrada (decision 0015, MyFortran 0.1)
Ops 45..47 (tras `SSET`): `DATA`, `RDLN`, `RD`.
- `DATA.t v, k, n` seguido de `EXT off`: inicializa `n` elementos de la variable `v` (escalar o array), desde el elemento `off`, con el valor `k` (INM o CTE del tipo de `v`;
  para CHARACTER una CTE cadena ya ajustada a la longitud de `v`). Se ejecuta **una vez, al arrancar el programa, antes de la primera unidad** (todas las unidades; las variables
  son estaticas). Por eso no es una instruccion del flujo: el backend las emite todas juntas al principio (`datos_iniciales`) y `traducir` las ignora. `tacvm` las aplica antes
  de ejecutar y valida que cada `DATA` lleve su `EXT`.
- `RDLN`: empieza una sentencia `READ`. **Cada `READ` consume un registro nuevo** (una linea tecleada), tenga o no lista; lo que sobraba del anterior se descarta. Pone a cero la marca
  de `/` (fin de lista). Sin mas entrada: error de ejecucion `End of input`.
- `RD.t d` (INTEGER, LOGICAL, REAL o CHARACTER): lee el siguiente valor de la lista en `d` (variable, o temporal si el elemento es de un array; en ese caso el generador carga antes el
  elemento con `LDE` y lo guarda despues con `STE`, de modo que un valor nulo lo deja como estaba). Reglas de la entrada dirigida por lista que se implementan:
  * los valores se separan por blancos y/o una coma; una coma sin valor (`,,`) es un valor **nulo** (la variable no cambia); una `/` termina la lista (el resto queda sin tocar);
  * si se acaba la linea antes de llenar la lista se sigue en la linea siguiente;
  * CHARACTER: sin comillas es una palabra (hasta blanco, coma o `/`); con comillas simples o dobles puede llevar blancos y comas, y la comilla repetida (`''` dentro de comillas simples) es una comilla.
    El valor se trunca o se rellena con blancos a la longitud de `d`;
  * LOGICAL: `T`, `F`, `.TRUE.`, `.FALSE.` (en cualquier combinacion de mayusculas; solo cuenta la primera letra tras el punto opcional);
  * INTEGER: `[+-]digitos`, de -32768 a 32767; REAL: `[+-]digitos[.digitos][E|D[+-]digitos]` (o `.5`, `5.`);
  * no hay repeticiones `r*c`, ni lectura de arrays completos, subcadenas o DO implicito.
  * Dato mal formado o fuera de rango: error de ejecucion `Invalid input`. Cada linea leida se repite en pantalla (eco).
- Errores de ejecucion nuevos de `RT_ERR`: 6 `Invalid input`, 7 `End of input` (se suman a los de la decision 0013).
- Z80: `RT_RDLN`, `RT_RDI`, `RT_RDL`, `RT_RDS`, `RT_RDF` (`tools/rtgen/rt_entrada.asm`) con un bloque de trabajo `RT_RDD` de 109 B (la linea de 79 caracteres, el puntero, los registros del conversor REAL).
  El teclado se lee por puertos (`RT_KEYH`: espera tecla, mayusculas con CAPS SHIFT, simbolos con SYMBOL SHIFT, CAPS SHIFT+0 = borrar, ENTER termina la linea); en las pruebas de PC
  `RT_KEYT` toma las teclas de un texto en memoria (`back_entrada_prueba`). El REAL se convierte acumulando la mantisa como REAL y multiplicando o dividiendo por 10^n: exacto hasta
  7 cifras significativas y |exponente| <= 38; con mas cifras puede diferir en el ultimo bit de `tacvm`.

## 8. Subprogramas (decision 0014, etapa S1: SUBROUTINE / CALL / RETURN)
Ops 39..43 (tras `RMOD`): `ENTER`, `PARAM`, `CALL`, `RET`, `FUNR` (esta ultima reservada para S2); op 44: `SSET` (S3a).
- `ENTER u`: SIEMPRE es la primera instruccion de un programa TAC (a = INM numero de unidad, 0 = la primera del fichero). El listado
  imprime en su lugar `UNIT <clase> <nombre>` y las lineas `VAR` de la unidad, y no escribe la instruccion. Las unidades van una tras otra;
  cada una acaba en `RET` (subrutina) o `HALT` (principal).
- `PARAM x, k, u`: a = operando real, b = INM `k` (entrada: formal k := actual) o `256 + k` (salida: actual := formal k), c = INM unidad
  llamada, `tipo` = tipo del argumento. **Semantica implementada: copia de entrada / copia de salida** para escalares (equivale a la referencia
  mientras no haya alias, que la norma prohibe cuando uno de los dos se escribe). El generador evalua TODOS los reales, emite los `PARAM` de
  entrada, el `CALL` y despues los `PARAM` de salida (un elemento de array se escribe de vuelta con `STE`). Los arrays (S3b) se pasan por
  direccion verdadera (ver `PARAM` con arrays mas abajo).
- `CALL u`: llama a la unidad `u`. `RET`: vuelve (tambien `STOP`/`HALT` en una subrutina: con mas de una unidad el prologo guarda SP en
  `SPSAVE` y `STOP` hace `LD SP,(SPSAVE) / RET`).
- `FUNR d, u` (S2): `d` (temporal) := variable resultado de la funcion `u` (`Unidad.sim_res`); va justo despues de los `PARAM` de salida de
  su `CALL`. Una referencia a funcion dentro de una expresion genera: argumentos reales (los temporales siguen vivos: son del programa),
  `PARAM` de entrada, `CALL`, `PARAM` de salida, `FUNR`. Los argumentos se recogen en un buffer de 128 ranuras que se usa como pila (`F(G(X))`).
  Si la funcion es `CHARACTER*n`, `FUNR` va precedida de `TMP.C d,n` y copia los `n` bytes de la variable resultado al temporal (el generador comprueba que
  la longitud del llamador coincide con la de la funcion).
  La unidad `FUNCTION` es como la `SUBROUTINE` (`ENTER` + `RET`) y tiene ademas la variable resultado (nombre de la funcion, tras los formales).
- `PARAM.C` (S3a, tipo CHARACTER): de entrada es un `MOV.C` formal := real (longitud del formal); de salida (`k >= 256`) copia SOLO los `n` caracteres del formal sobre el
  real (variable o temporal de elemento), `LDIR` en el Z80. El generador garantiza `len(real) >= len(formal)` (error `argument shorter than dummy argument`).
- `SSET.C base, i, e` seguido de `EXT j` (S3a): `base(i:j) := e` (CHARACTER; `base` variable o temporal de un elemento): copia `min(len(e), j-i+1)` caracteres
  y rellena con blancos hasta `j`. Los limites fuera de rango (`i < 1`, `j > len(base)`) los da el programa: tacvm FALLA con error de ejecucion; el Z80 solo limita la longitud con `RT_CLAMP` (SIN comprobar `i`), igual que `SSUB`.
  Para un elemento de array el generador lo carga con `LDE` antes y lo escribe con `STE` despues.
- `PARAM` con arrays y subcadenas (S3b/c/d). Banderas de `b` (sobre `k`): `256` salida (copia de vuelta), `512` direccion de array completo (`a` = VAR array; se pasa el
  puntero al comienzo y el formal lo guarda en su celda de 2 B, sin copia), `1024` elemento de array seguido de un `EXT arr, off`: si el formal es array, el puntero es la direccion
  del elemento; si es escalar se usa el camino escalar con el temporal de `LDE`. `1280` = 1024+256 (elemento con copia de vuelta). Un formal array o `CHARACTER*(*)` se direcciona
  por su puntero/celda (en el Z80 `ld_de_base`/`ld_hl_base`). `SSET`/`SSUB` llevan `EXT` con el limite superior. `SLEN d, x` (op 28) se emite para `LEN` de una cadena de longitud
  supuesta. `CHARACTER*(*)` formal: `[long 2 B][datos]`, capacidad 255 para el generador.
- Numeracion global: simbolos (`SIM_MAX` 512 en total), etiquetas (256), temporales (128), palabras ocultas de DO y etiquetas internas son
  **del programa**, no de la unidad, para que un subprograma no pise los del que llama. Los formales son los primeros simbolos de la unidad.
- Nombres en el listado Z80: `V<u>_X`, `L<u>_n`, `U_<nombre>` (con una sola unidad se conservan los de antes).
- Limites (S6 los revisara): 63 unidades, 15 argumentos, ~700 llamadas, y los globales de arriba. Errores `MF_COD_UNI` = 110 + `ErrorUni` (`unid.h`).

## 9. Ejemplos
### 9.1 Suma
```
      INTEGER I, S
      S = 0
      DO 10 I = 1, 10
         S = S + I
   10 CONTINUE
      PRINT *, S
      END
```
```
UNIT PROGRAM MAIN
VAR I INTEGER
VAR S INTEGER
        MOV.I  S, 0
        MOV.I  $0, 1
        MOV.I  $1, 10              ; iteraciones = (10 - 1 + 1) / 1, plegado
        MOV.I  I, 1
        LBL    @@1
        GT.I   %0, $1, 0
        JF     %0, @@2
        ADD.I  S, S, I
        LBL    @10
        ADD.I  I, I, $0
        SUB.I  $1, $1, 1
        JMP    @@1
        LBL    @@2
        PUT.I  S
        PUTNL
        HALT
```
### 9.2 CHARACTER y IF
```
      CHARACTER*8 A
      A = 'HI' // 'X'
      IF (A .EQ. 'HIX') PRINT *, 'OK'
      END
```
```
UNIT PROGRAM MAIN
VAR A CHARACTER*8
        TMP.C  %0, 3
        SCAT.C %0, 'HI', 'X'
        MOV.C  A, %0
        EQ.C   %1, A, 'HIX'
        JF     %1, @@1
        PUT.C  'OK'
        PUTNL
        LBL    @@1
        HALT
```
### 9.3 Array
```
      INTEGER V(10)
      V(3) = 7
      I = V(3) + 1
      END
```
```
UNIT PROGRAM MAIN
VAR V INTEGER (1:10)
VAR I INTEGER
        STE.I  V, 2, 7             ; desplazamiento = 3 - 1 = 2, plegado por el generador
        LDE.I  %0, V, 2
        ADD.I  I, %0, 1
        HALT
```

## 10. Puntos abiertos (no normativos todavia)
- **REAL** (hecho, decision 0013): IEEE 754 simple por software, `PRINT *` al estilo gfortran con 9 cifras. **DOUBLE**: error
  `not supported yet`. `REAL ** x` y `x ** REAL`, `DO` con variable REAL y las intrinsecas con argumentos REAL (salvo `INT`, `IFIX`,
  `REAL`, `FLOAT`): sin generar todavia (R4).
- **Numeracion definitiva de operaciones** (seccion 5): provisional; se fija al implementar la v0.1 y se cierra la v1.0.
- **Errores en ejecucion** (division por cero, indice fuera de rango, subcadena fuera de rango): politica del backend.
- **E/S**: solo `PRINT` de lista a la unidad por defecto; `READ`, `WRITE`, `FORMAT` y ficheros estan fuera de v0.1.
- **Subprogramas**: seccion 8.
- **Intrinsecas**: el semantico las tipa (decision 0010). El generador expande las ENTERAS en TAC, sin operaciones nuevas ni rutinas
  de ejecucion: `ABS/IABS`, `MOD`, `SIGN/ISIGN`, `DIM/IDIM`, `MAX/MAX0`, `MIN/MIN0` (MOV/comparacion/`JF`/`NEG`) e `INT(x)` / `IFIX(x)` (con `FTOI` si x es REAL), `REAL(i)` / `FLOAT(i)` (con `ITOF`);
  `LEN(c)` es una constante al compilar (variable, elemento, texto o concatenacion de ellos; no una subcadena). Las demas (REAL,
  `ICHAR`, `INDEX`, `CHAR`, `LGE`...) siguen sin generar.
- **Longitud de temporales CHARACTER**: la calcula el generador al vuelo (`gen.c`, variable `g.cap`); no hizo falta el campo `lon`
  en el AST (0011, E queda descartada).
- **Aritmetica de 16 bits**: el conteo de un DO y las operaciones envuelven modulo 65536. No hay deteccion de desbordamiento.
- **Aun sin generar** (error de compilacion `not supported yet`): DOUBLE PRECISION, funciones intrinsecas con argumentos REAL
  (`ABS`, `SQRT`...), `**` con REAL, asignacion a subcadena (`S(2:3) = ...`), `PRINT` con FORMAT, variable de DO no INTEGER.
- **Capacidad** (implementacion de referencia): 2048 instrucciones; el pool de constantes ocupa una pagina de 8 KB (p. ej. 157
  cadenas de 50 caracteres); 64 etiquetas en un GOTO calculado; 16 IF/DO anidados. Al agotarse se anota
  `program too large (code memory full)` una sola vez.
