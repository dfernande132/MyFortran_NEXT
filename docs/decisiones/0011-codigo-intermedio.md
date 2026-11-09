# 0011 - Codigo intermedio de tres direcciones (TAC)

Fecha: 2026-10-30. Estado: PROPUESTA, decidido con el autor solo lo marcado ACORDADO. Nada implementado.

> **Estado (2026-10-30): IMPLEMENTADO en PC** (`tac.c`, `gen.c`, `tac_lst.c`; ver `docs/especificacion-tac.md` v0.2). Cambios respecto a
> esta propuesta: (E) no hace falta el campo `lon` en `Nodo`: el generador calcula la capacidad de los temporales CHARACTER al
> vuelo; las etiquetas de usuario van en el TAC como indice de la tabla de etiquetas (resuelve el limite de 13 bits); la pila de
> bloques del generador y el buffer del GOTO calculado viven en la pagina de datos 2 (no en el residente). SIN MEDIR en el Next:
> residente, pila y banco del generador (propuesta: banco 44 con trampolines `t_gen_*`, o en el 45).

## Objetivo
Cerrar el ciclo completo (fuente -> TAC -> ejecucion) pronto, aun sin implementar todo F77, para probar de extremo a extremo.
El TAC es el contrato entre el front-end (cap. 1-5) y el backend (Z80, aun sin disenar). Un intérprete de TAC en las pruebas de
PC valida la semantica sin escribir todavia el generador de Z80.

## ACORDADO con el autor
1. **Almacen**: paginas de datos de `platform/banco.h`. El compilador pide 2 paginas mas para el TAC.
   Reparto propuesto: compilador 0..5 (0 simbolos, 1 dimensiones, 2 etiquetas, 3..4 TAC, 5 libre: pool de constantes/cadenas),
   editor 6..9, `BANCO_PAGINAS = 10` (fisicas 50..59). **Requiere el acuerdo del EDITOR** (`ED_PAG_BASE = 6`): `banco.h`
   NO se toca hasta que lo confirme en el buzon (con 10 paginas y `ED_PAG_BASE = 4` el editor pisaria el TAC).
   Tope: error `program too large` al llenar las paginas del TAC.
2. **Alcance inicial**: INTEGER, LOGICAL y CHARACTER; asignacion, aritmetica entera, relacionales, logicas, IF/ELSE IF/ELSE/END IF,
   IF aritmetico, DO, GOTO (simple y calculado), CONTINUE, STOP, END, arrays 1-2D, PRINT (enteros, logicos, cadenas), concatenacion,
   subcadenas y comparacion de CHARACTER. Sin REAL/DOUBLE ni intrinsecas en el TAC: el semantico las acepta, el generador da
   un error claro ("not supported by the code generator yet"). Las operaciones llevan etiqueta de tipo para anadir REAL despues
   sin cambiar el formato.

3. **(2026-10-30) Aprobados por el autor los pasos propuestos**: una sola pasada con el generador llamado desde el semantico, instruccion
   de 8 B con operandos de 16 bits, subprogramas reservados en el formato, DO con cuenta de iteraciones calculada al entrar,
   intérprete de TAC en las pruebas de PC.
4. **El TAC tiene especificacion propia**: `docs/especificacion-tac.md` (normativa, independiente de la maquina, pensada para reutilizar
   el formato en otro compilador o backend). Esta decision conserva el razonamiento; en caso de diferencia manda la especificacion.
   Cambios respecto al borrador de esta decision: los relacionales y `MOV`/`LDE`/`STE` se distinguen por el campo `tipo` (no hay
   `SCMP`/`SMOV`); `PUT.T` sustituye a `PUTI/PUTL/PUTS`; el GOTO calculado usa `JEQ`; el DO usa **dos** ocultas, no cuatro;
   nuevo `TMP.C` para declarar la capacidad de un temporal CHARACTER y `EXT` para el cuarto operando de `SSUB`.

## Propuesta de diseno (a decidir)
### A. Cuando se emite
Una sola pasada: el semantico llama al generador **justo despues de tipar y validar** cada expresion/sentencia, mientras el AST de
la sentencia sigue vivo (las listas en flujo, `stmt_elemento`, invalidan el elemento anterior: PRINT y GOTO calculado se emiten
elemento a elemento, dentro del recorrido de `sem.c`). No existe un AST del programa. Si `mf.n != 0`, el TAC se descarta (no se
sigue emitiendo tras el primer error de la unidad; los errores de las fases anteriores siguen acumulandose).
El generador (`gen.c`) vive en un banco propio (propuesta: 44, reservado) y `sem.c` lo llama por trampolin (resident `t_gen_*`).

### B. Formato de instruccion: 8 bytes
```
uint8_t op; uint8_t tipo; uint16_t a, b, c;   /* tipo: TD_INTEGER/LOGICAL/CHARACTER/... */
```
Operando de 16 bits: 3 bits de clase + 13 de valor (0..8191):
`NADA`, `TEMP` (temporal de la sentencia), `VAR` (id de simbolo, <= 512), `INM` (entero pequeno con signo, -4096..4095),
`CTE` (indice del pool: enteros grandes y cadenas), `ETQ` (etiqueta: de usuario o interna), `OCULTA` (variable oculta del
generador, p. ej. el contador de un DO).
Capacidad: 8 KB / 8 B = 1.024 instrucciones por pagina; con 2 paginas ~2.048 (estimacion, SIN MEDIR).

### C. Conjunto de operaciones (borrador)
- Datos: `MOV d, s`; `LDE d, A, idx` / `STE A, idx, s` (elemento de array; el indice lineal lo calcula el generador con el
  desplazamiento y la dimension); `NEG`, `ADD`, `SUB`, `MUL`, `DIV`, `POW` (INTEGER/LOGICAL: `POW` solo con exponente entero).
- Logicas/relacionales: `EQ NE LT LE GT GE` (resultado LOGICAL), `AND OR EQV NEQV NOT`.
- Control: `ETQ n`, `JMP n`, `JT c, n`, `JF c, n`, `JTAB sel, k, n1..` (GOTO calculado: se emite como `k` saltos condicionales
  encadenados, sin tabla en el TAC), `STOP`, `HALT`.
- E/S: `PUTI s`, `PUTL s`, `PUTS cte|var`, `PUTNL`.
- CHARACTER: `SMOV d, s` (copia con relleno de blancos/truncado a la longitud de d), `SCAT d, a, b`, `SSUB d, s, i, j`,
  `SCMP` y relacionales de cadenas (comparan con relleno de blancos segun el estandar), `SLEN`.
- Unidades (reservado, sin implementar): `ENTER u`, `PARAM x` (por referencia), `CALL u`, `RET`, `FUNR d` (valor de funcion).
  En F77 no hay recursion: todo estatico, un unico marco por unidad, argumentos por referencia (direccion).

### D. DO
Se baja a: `i = inicio; fin_o = fin; paso_o = paso; cnt_o = (fin_o - i + paso_o) / paso_o` (la cuenta de iteraciones de F77 se
calcula UNA vez, al entrar); el bucle comprueba `cnt_o > 0`. Cuatro variables ocultas por bucle (`OCULTA`), 2 B cada una en el
programa generado.

### E. CHARACTER: lo que el semantico todavia NO guarda
Hoy `tdato` solo lleva el TIPO; la longitud de `CHARACTER*n` esta en `Simbolo.extra`. Una expresion de cadena necesita
longitud conocida en compilacion para reservar su temporal. Opciones:
1. (propuesta) Nuevo campo `uint8_t lon` en `Nodo` (maximo de la expresion: constante = su longitud; `A//B` = suma; `S(i:j)` =
   `j-i+1` si son constantes, si no la longitud declarada de `S`). Coste: `sizeof(Nodo)` pasa de 8 a 9 B (AST_NODOS = 64:
   +64 B de bss residente, estimacion).
2. Calcularlo en el generador al recorrer el arbol (sin tocar `Nodo`): cuesta recursion extra y pila.
Se decide al implementar con una medida de pila/residente; la 1 es mas simple.
Un temporal CHARACTER lleva en el programa generado un byte de longitud real + `lon` bytes de datos.

### F. Direcciones y variables
`Simbolo.dir` (ya existe, lo asigna el generador) se rellena cuando el backend asigne la memoria de datos del programa. El TAC
solo habla de `VAR id`. Los temporales de una sentencia se numeran desde 0 y se reutilizan sentencia a sentencia (a lo sumo 64:
un nodo del AST por temporal).

### G. Pruebas
- `compiler/tests/test_cap06.c`: por cada construccion, el TAC esperado (se compara como texto desensamblado) y comprobaciones
  de limites (paginas llenas, 13 bits de operando).
- **Interprete de TAC en PC** (`tools/medir/` o `compiler/tests/`, no entra en el Next): ejecuta el TAC y compara la salida con la
  esperada de programas Fortran (suma, factorial, criba, bucles anidados, cadenas). Mutaciones y fuzz como en los capitulos previos.

## Que NO se decide aqui
Representacion de REAL, formato del ejecutable (.nex / codigo en memoria), ABI del backend, convenio de E/S del programa
generado, unidades de programa. Es el capitulo siguiente (backend) y la decision de subprogramas.

## Riesgos (SIN medir)
- Residente: el generador anade estado global (contadores de temporales, etiquetas internas, posicion de escritura en el TAC).
  Se medira como cada fase anterior.
- Pila: el generador se llama desde el recorrido de `sem.c`, a nivel de sentencia: no deberia crecer la recursion de expresiones
  (a comprobar en `i1_integrar`).
- Una pasada con descarte al primer error impide inspeccionar TAC parcial; aceptable.
