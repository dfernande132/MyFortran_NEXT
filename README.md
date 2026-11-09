# MyFortran

Compilador de **Fortran 77** para el **ZX Spectrum Next**, con su propio editor (estilo Turbo Pascal 3.0). Todo corre en el
Next: se escribe el programa, se compila y se obtiene un `.nex` que se ejecuta solo. Es un proyecto **didactico**: el
codigo esta escrito para entenderse, cada decision tiene su documento y nada se da por probado si no se ha medido.

Version **0.1** (primera publica). Licencia MIT (ver `LICENSE`).

| Pantalla de inicio y ayuda | Editando `primos.f` (criba de Eratostenes) |
|---|---|
| ![Editor de MyFortran: pantalla de inicio con la ayuda de teclas](screenshots/Inicio.png) | ![Editor de MyFortran con primos.f cargado, con resaltado de sintaxis](screenshots/primos.png) |

## Que hay en la 0.1

Un unico `MyFortran.nex` (editor + compilador, repartidos en ocho bancos de 16 KB) que genera programas `.nex` de Z80.

**Tipos:** `INTEGER` (16 bits), `REAL` (IEEE de 32 bits por software), `LOGICAL`, `CHARACTER*n` con subcadenas `S(i:j)`.
`DOUBLE PRECISION` se reconoce pero **no** esta soportado.

**Sentencias:** `PROGRAM`, `END`, declaraciones de tipo, `DIMENSION`, `PARAMETER`, `DATA`, `EXTERNAL`, `INTRINSIC`, asignacion,
`IF` logico, de bloque (`ELSE IF`, `ELSE`) y aritmetico, `DO` (con paso, variable REAL y etiqueta compartida), `GO TO` simple y
calculado, `CONTINUE`, `STOP`, `PRINT *`, `READ *` (del teclado), `SUBROUTINE`, `FUNCTION`, `CALL`, `RETURN` y funciones-sentencia.
Vectores y matrices (hasta 2 dimensiones, limites inferiores distintos de 1), tambien como argumentos con dimension ajustable.

**Funciones intrinsecas:** `ABS`, `MOD`, `SIGN`, `DIM`, `MAX`, `MIN` (y sus variantes `IABS`, `ISIGN`, `IDIM`, `MAX0`, `AMAX1`...),
`AINT`, `ANINT`, `NINT`, `SQRT`, `LEN`, `INT`, `REAL`, `FLOAT`, `IFIX`.

### Limitaciones conocidas de la 0.1

El compilador rechaza con un mensaje claro (normalmente `not supported yet`) lo siguiente; esta previsto para 0.2 y 0.3:

- `DOUBLE PRECISION`; `COMMON`, `EQUIVALENCE`, `SAVE`, `BLOCK DATA`, `ENTRY`, `RETURN` alternativo.
- Procedimientos e intrinsecas como argumentos de otros procedimientos.
- Entrada/salida: solo `PRINT *` y `READ *` (por lista, sin `FORMAT`, sin `WRITE`, sin ficheros). `READ` no admite arrays completos,
  subcadenas, `DO` implicito ni repeticiones `r*c`. `DATA` sin `DO` implicito ni subcadenas. No se puede imprimir un array entero.
- Arrays de mas de 2 dimensiones.
- Intrinsecas `ICHAR`, `CHAR`, `INDEX`, `LGE/LGT/LLE/LLT` y las funciones matematicas `EXP`, `LOG`, `SIN`, `COS`, `TAN`, `ATAN`...
  (`SQRT` si). Potencia con exponente `REAL` (`2.0**0.5`); con exponente entero funciona.
- `INTEGER` de 16 bits: un desbordamiento envuelve sin avisar; una constante fuera de rango es un error de compilacion.
- Lectura de `REAL` exacta solo hasta ~7 cifras significativas.
- Limites por la pila del Next (ver "Memoria"): hasta 10 parentesis anidados, 5 niveles de `F(F(...))` y 16 bloques `IF`/`DO` anidados. Una sentencia admite 20 lineas (1 inicial y 19 de continuacion) y unos 64 nodos de expresion.

## Usar MyFortran en el Next

Copia `MyFortran.nex` a la tarjeta SD (o a la carpeta del emulador) y ejecutalo. Los nombres de los programas deben ser **8.3
sin guion bajo** (`fibo.f`, `n1orden.f`) para poder lanzarlos desde el Browser.

El editor tiene dos modos. Se escribe directamente; para dar una orden se pulsa **Esc** (CAPS SHIFT + SPACE) y despues una letra:

| Tecla tras Esc | Orden |
|---|---|
| `H` / `E` | inicio / fin de linea |
| `T` / `B` | inicio / fin del texto |
| `U` / `D` | pagina arriba / abajo |
| `G` | ir a una linea |
| `K` | borrar la linea |
| `C` | **comprobar** (compila sin generar nada; muestra los errores) |
| `O` | **construir**: genera el `.nex` del programa (con el nombre del fuente) |
| `X` / `Z` | error siguiente / anterior |
| `N` `L` `S` `A` | nuevo, cargar, guardar, guardar como |
| `5` `6` `7` `8` | cursor izquierda, abajo, arriba, derecha |

Un programa de ejemplo: `examples/leer.f` (lee datos del teclado), `examples/fibo.f`, `examples/primos.f`...
`READ` espera una linea y termina con ENTER; `PRINT` escribe en pantalla y, al llegar abajo, la pantalla sube sola.

## Compilar MyFortran desde el codigo

Windows con VS Code (`Ctrl+Shift+P` -> `Tasks: Run Task`). Requisitos: `gcc` de MSYS2 UCRT64 (`C:\msys64\ucrt64\bin`) para las pruebas
en PC y [z88dk](https://z88dk.org) en `C:\z88dk` para el Next. `tools/build.ps1` ajusta el `PATH` solo durante la compilacion.

| Tarea | Que hace |
|---|---|
| PC: tests (compilar y ejecutar) | Compila el nucleo con gcc y ejecuta todas las suites (`compiler/tests/test_*.c`) |
| Next: MyFortran.nex (compilar editor + compilador) | Genera `build/next/MyFortran.nex` e informa del espacio libre de cada banco |
| PC: TAC / Z80 del fichero abierto | Compila el `.f` abierto, muestra su codigo intermedio o su ensamblador y lo ejecuta en un emulador |
| Editor PC: tests, Editor Next: ... | Lo mismo para el editor suelto (`tools/build_editor.ps1`) |
| PC/Next: smoke | Comprueban que las herramientas funcionan |

Se puede limpiar con `tools\limpiar.bat` (borra `build\`, temporales y restos; pide confirmacion).

## Como esta hecho

Un nucleo en C portable (sin `malloc`, tablas de tamano fijo, sin `float`) con las fases: lector de sentencias y lexer
-> parser de expresiones y sentencias (AST en una pagina de datos) -> semantico (tabla de simbolos, tipos, bloques) -> codigo de
tres direcciones (TAC) -> generador de Z80 con una pequena biblioteca de ejecucion (REAL, PRINT, READ) -> `.nex`. El mismo codigo
corre en PC (con un interprete del TAC y un emulador de Z80 para probarlo) y en el Next.

En el Next la memoria es el problema: 48 KB visibles. El codigo esta en bancos de 16 KB con **trampolines** residentes y las
tablas viven en paginas de datos de 8 KB (`banco.h`).

### Memoria (medidas de la 0.1, Next)

Libre al final de cada banco, tras compilar `MyFortran.nex`: 41 (editor) 4.584 B, 42 2.451 B, 43 2.215 B, 44 2.133 B, 45 1.906 B,
46 748 B, 47 959 B, 48 3.087 B. El fin del `bss` residente esta en `$BCE8` y la pila llega a `$BFAD`. La pila usada compilando y
construyendo el programa de maximos (`examples/` y `tools/medir/pila/maximo.f`) es **583 de 645 bytes** (se mide con `Esc` `M`, un comando
oculto de desarrollo).

## Pruebas

`PC: tests` ejecuta unas 36.000 pruebas repartidas por capitulos (lexer, parser, semantico, TAC, generador Z80, subprogramas,
DATA/READ), mas de 465.000 de aritmetica REAL contra un oraculo, fuzzers de programas aleatorios y `test_programas.c`: **312 programas completos** (`compiler/tests/programas/`, `examples/`
y `compiler/tests/ejemplos/`) que se ejecutan como TAC y como Z80 y se comparan con la salida esperada, calculada aparte en
Python (`oraculo.py`). En el Next hay seis programas de comprobacion en `examples/next/` (con su `LEEME.md`).

## Estructura

| Carpeta | Contenido |
|---|---|
| `compiler/src`, `compiler/include` | El compilador en C portable; `mf_api.h` es el contrato con el editor |
| `compiler/tests` | Pruebas por capitulo y programas completos |
| `editor/` | El editor (buffer de hueco, pantalla de 80 columnas, esxdos) y sus pruebas |
| `platform/pc`, `platform/next` | Lo que depende de la maquina: paginas de datos, ficheros, emulador de prueba |
| `tools/` | Scripts de compilacion (`build.ps1`), generador de rutinas Z80 (`rtgen/`), fuzzers, medidas y el pegamento de bancos del Next (`spike/e1_*`) |
| `examples/` | Programas de ejemplo (y las pruebas del Next en `examples/next/`) |
| `docs/` | `capitulos/` (cap. 1-6, uno por fase: teoria, decisiones y pruebas), `decisiones/` (por que se decidio cada cosa), `editor/` y `especificacion-tac.md` |

Las decisiones de diseno estan numeradas en `docs/decisiones/` (0001 etapas ... 0015 datos y entrada).

## Hoja de ruta

- **0.1** (esta): lo anterior.
- **0.2**: `COMMON`, `SAVE`, procedimientos e intrinsecas como argumentos.
- **0.3**: `RETURN` alternativo, `ENTRY`, `EQUIVALENCE`, `BLOCK DATA`.

## Incidencias de entorno (Windows)

- **gcc muere en silencio (codigo 1, sin mensaje).** `C:\z88dk\bin` trae DLLs propias
  (`libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`, `zlib1.dll`...) con el mismo nombre
  que las de MSYS2, y `cc1.exe` carga las equivocadas si z88dk va antes en el `PATH`.
  Solucion: `tools/build.ps1` antepone al `PATH` el directorio del compilador que toca
  (MSYS2 `C:\msys64\ucrt64\bin` para PC, z88dk para Next), solo durante la compilacion.
  Consecuencia: compilar siempre mediante las tareas, no con `gcc` a mano en la terminal. (Causa
  probable, confirmada porque compilar con el PATH limpio funciono.)
