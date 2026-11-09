# Capítulo 6 - Código intermedio (TAC)

Estado: **cerrado para 0.1**. Generador, intérprete de referencia y pruebas en PC; el mismo código corre en el Next (banco 44).

Ficheros: `compiler/src/tac.{h,c}` (formato y almacén), `tac_lst.c` (listado), `gen.{h,c}`, `gen_ll.c`, `gen_int.h`
(generación), `platform/pc/tacvm.c` (intérprete), `compiler/tests/test_cap06.c`. Especificación: `docs/especificacion-tac.md`.
Decisiones: `docs/decisiones/0011-codigo-intermedio.md` (y 0014/0015 para subprogramas y datos).

## 1. Teoría en cinco ideas

1. **Qué es.** Hasta el capítulo 5 sabemos que el programa está bien escrito y tiene sentido. Ahora hay que *traducirlo*.
   Saltar directamente de Fortran a Z80 mezclaría dos problemas distintos (qué hace el programa / cómo lo hace este
   procesador). El **código de tres direcciones** (TAC) es el puente: un lenguaje diminuto, igual para todas las máquinas,
   donde cada instrucción hace como mucho una cosa: `ADD.I %0, X, Y` (`%0 := X + Y`). Tres direcciones = dos operandos
   y un destino.
2. **Por qué compensa.** El frontal (lexer, parser, semántico, generador) no sabe nada del Z80; el backend no sabe nada de
   Fortran. Para otro procesador se escribe otro backend. Y se puede *probar* cada mitad por separado: un intérprete de TAC
   (`tacvm`) ejecuta el programa sin Z80, y sirve de oráculo para el backend (capítulo 7).
3. **Aplanar.** Una expresión es un árbol; el TAC es una lista. `S + I*I` se aplana en `MUL %0, I, I` y luego
   `ADD S, S, %0`: se evalúa de dentro afuera y los resultados intermedios van a **temporales** (`%0`, `%1`…), que
   mueren al acabar la sentencia, así que se reutilizan. Los saltos (`JMP`, `JT`, `JF`) y las etiquetas (`LBL`) sustituyen
   a todas las estructuras: `IF`, `DO` y `GOTO` acaban siendo lo mismo.
4. **Tipos explícitos.** El semántico ya decidió el tipo de cada operación, así que el TAC lo lleva en la instrucción:
   `.I` entero, `.R` real, `.L` lógico, `.C` carácter. El backend nunca adivina; si `A + B` mezclaba entero y real, el
   generador ya insertó `ITOF`.
5. **Una pasada, sin árbol del programa.** El AST del capítulo 4 es de *una sentencia*, no del programa entero (no cabe en
   64 KB con bancos). Por eso el generador se llama desde el semántico, sentencia a sentencia: cuando una sentencia queda
   validada se traduce y su árbol se olvida. Si hay algún error, el TAC se descarta.

## 2. Decisiones de diseño

- **Registro de 8 bytes:** `op, tipo, a, b, c` (los operandos en 16 bits little-endian). Es lo que permite 1.024
  instrucciones por página de datos de 8 KB; capacidad actual 2.048 (páginas 3 y 4), más una página (la 5) para el *pool*
  de constantes grandes y cadenas.
- **Operando de 16 bits = clase (3 bits) + valor (13 bits).** Clases: `NADA`, `TEMP` (`%n`), `VAR` (índice en la tabla de
  símbolos), `INM` (inmediato pequeño), `CTE` (índice en el pool), `ETQ` (etiqueta) y `OCULTA` (`$n`, variables que el
  usuario no ve, como el contador de un `DO`). Etiquetas del usuario `@10`; las que inventa el generador, `@@1`.
- **Jerarquía de instrucciones.** Datos y aritmética (`MOV NEG ADD SUB MUL DIV POW`), relacionales (`EQ … GE`), lógicas
  (`AND OR EQV NEQV NOT`), control (`LBL JMP JT JF JEQ`), arrays (`LDE STE`), cadenas (`SCAT SSUB SSET SLEN`), conversión
  (`ITOF FTOI`), subprogramas (`ENTER PARAM CALL RET FUNR`), E/S (`PUT PUTNL RDLN RD`), datos (`DATA`) y `STOP`/`HALT`.
- **`DO` con cuenta de iteraciones calculada una vez.** F77 manda calcular `(final - inicial + paso) / paso` *antes* del
  primer paso y no mirar de nuevo los límites. El generador guarda esa cuenta en un oculto y la decrementa. Así no hay
  desbordamiento con enteros de 16 bits cerca de 32767 ni sorpresas si el cuerpo toca la variable de límite.
- **`GOTO` calculado = cadena de `JEQ`.** `GOTO (10, 20) N` se convierte en «si N vale 1 salta a 10; si vale 2, a 20; si no, sigue».
  Sin tabla de saltos: son pocas etiquetas y el código es más fácil de verificar.
- **Parámetros por copia de entrada / copia de salida**, no por referencia con punteros (decisión 0014). Es la única
  desviación del estándar con consecuencias observables (`CALL S(A, A)` con escritura); está documentada y probada.
- **Un intérprete como referencia.** `tacvm.c` ejecuta el TAC con la semántica de la especificación, en PC. Si el backend
  Z80 y `tacvm` discrepan, el fallo está en la traducción, no en el frontal.

## 3. Cómo se lee un listado

Los listados de abajo son salida real del compilador (herramienta `lst`, que compila un programa desde la entrada
estándar, vuelca el TAC y lo ejecuta en `tacvm`). Cada línea es `OP.tipo destino, fuente, fuente`.

### 3.1 Suma con `DO`

```fortran
      PROGRAM SUMA
      INTEGER I, S
      S = 0
      DO 10 I = 1, 5
         S = S + I*I
   10 CONTINUE
      PRINT *, S
      END
```

```
        MOV.I  S, 0
        MOV.I  $0, 1          paso (oculto)
        MOV.I  $1, 5          cuenta de iteraciones (oculto)
        MOV.I  I, 1
        LBL    @@1
        GT.I   %0, $1, 0
        JF     %0, @@2        si la cuenta se agotó, fuera
        MUL.I  %0, I, I       cuerpo: %0 := I*I
        ADD.I  S, S, %0
        LBL    @10            la etiqueta del CONTINUE sigue existiendo
        ADD.I  I, I, $0       I := I + paso
        SUB.I  $1, $1, 1      cuenta := cuenta - 1
        JMP    @@1
        LBL    @@2
        PUT.I  S
        PUTNL
        HALT
```

Salida: `55`. (Los comentarios a la derecha los hemos añadido aquí; el listado real no los lleva.) Observa que el
temporal `%0` se reutiliza: sirve para la condición del bucle y luego para `I*I`.

### 3.2 `CHARACTER` e `IF`

```fortran
      CHARACTER*5 A
      A = 'HOLA'
      IF (A .EQ. 'HOLA') PRINT *, A
```

```
        MOV.C  A, 'HOLA'
        EQ.C   %0, A, 'HOLA'
        JF     %0, @@1
        PUT.C  A
        PUTNL
        LBL    @@1
```

El `IF` lógico es siempre «calcula la condición, salta si es falsa». Fíjate en que la comparación de cadenas es del tipo
`.C`: en F77 comparar `'HOLA'` con `'HOLA '` es cierto (se rellena con blancos), y eso lo decide el backend, no el generador.

### 3.3 Arrays

```fortran
      INTEGER V(3)
      V(2) = 7
      PRINT *, V(2) + 1
```

```
        STE.I  V, 1, 7        V(2) := 7 (índice 1, base 0)
        LDE.I  %0, V, 1       %0 := V(2)
        ADD.I  %1, %0, 1
        PUT.I  %1
        PUTNL
```

El índice ya llega «normalizado» (base 0): el generador resta el límite inferior de la declaración. Si es constante, la
resta se hace en compilación.

### 3.4 Llamada a función

```fortran
      INTEGER K, DOBLE
      K = DOBLE(4)
...
      INTEGER FUNCTION DOBLE(N)
      DOBLE = 2*N
```

```
        PARAM.I 4, 0, 2       copia 4 al formal 0 de la unidad 2
        CALL   2
        FUNR.I %0, 2          %0 := resultado de la unidad 2
        MOV.I  K, %0
...
UNIT FUNCTION DOBLE
        MUL.I  DOBLE, 2, N    el resultado se asigna al nombre de la función
        RET
```

### 3.5 `GOTO` calculado

`GOTO (10, 20) N` genera `JEQ N, 1, @10` y `JEQ N, 2, @20`; si N no vale ni 1 ni 2, la ejecución cae a la siguiente instrucción,
como manda el estándar.

## 4. Qué se comprueba

- **Listados exactos** de la especificación (secciones 9.1-9.3 y las instrucciones nuevas de cada etapa), carácter a carácter.
- **Ejecución en `tacvm`:** el TAC de cada programa imprime lo que imprimiría Fortran.
- **Errores:** el compilador anota el error esperado con línea, columna y código.
- **Diferencial aleatorio:** expresiones enteras y lógicas generadas al azar se evalúan en C y en `tacvm`; deben coincidir.
- **Validador de TAC (`tacvm_validar`):** comprueba que el TAC generado es estructuralmente válido antes de ejecutarlo. Lo usan las pruebas
  de los capítulos 6 a 10.

## 5. Pruebas

```
compiler/tests/test_cap06.c    generador + tacvm (listados, salidas, errores, diferencial)
compiler/tests/test_programas.c  14 programas completos + ejemplos; TAC y Z80 deben dar la misma salida y la esperada
```

El generador vive en el banco 44 del Next (`gen.c`, accesible por los *trampolines* `t_gen_*`); el TAC y su pool, en páginas de datos (3-5). La memoria residente del compilador no guarda instrucciones: eso es lo que permite que un programa de 2.048 instrucciones quepa.

## 6. Límites conocidos

- 2.048 instrucciones por programa; al agotarse, el almacén se marca como lleno y la compilación falla.
- Tres dimensiones o más en un array: no soportado en 0.1.
- El TAC no tiene optimizaciones: `ADD.I I, I, $0` ocupa lo que ocupa. El backend Z80 recupera parte con casos especiales
  (`+1`, `-1`, constantes), pero un optimizador de TAC (propagación de constantes, eliminación de temporales) queda para después de 0.1.
