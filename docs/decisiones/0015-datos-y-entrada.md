# 0015 - DIMENSION, EXTERNAL/INTRINSIC, PARAMETER, DATA y READ (MyFortran 0.1)

Fecha: 2026-11-08. Estado: IMPLEMENTADO en PC (compilador, TAC, backend Z80, emulador); SIN CONSTRUIR con z88dk ni probado en el Next.

## Decisiones
1. **DIMENSION** (S4a): valida tras una declaracion de tipo o antes de ella (`DIMENSION M(3)` y despues `INTEGER M`), tambien sobre formales. Error `declaration must precede executable statements`
   despues de la primera sentencia ejecutable y `array already dimensioned` al repetir el declarador (D4 de la hoja de ruta); `DIMENSION needs array bounds` si falta el declarador. Los limites pueden ser constantes enteras o `PARAMETER` enteros.
2. **EXTERNAL / INTRINSIC** (S4a): solo declaran. Un nombre declarado asi NO puede pasarse como argumento real (`procedures as arguments are not supported yet`; 0.2). `INTRINSIC` exige una intrinseca conocida
   (`not an intrinsic function`); `EXTERNAL` fija que el nombre es un procedimiento (el tipo de una funcion externa se declara aparte, como en la norma). Llamar a una intrinseca declarada funciona igual.
3. **PARAMETER**: una variable escalar INICIALIZADA y de solo lectura (se anota con los dos bits de dimensiones del simbolo). Si es INTEGER su valor se pliega en compilacion y sirve para dimensionar y para longitudes
   `CHARACTER*N`; las REAL, LOGICAL y CHARACTER son variables con valor inicial (como un `DATA` implicito) y no se pueden usar como constantes de `DATA`. Asignarles es error (`cannot modify a PARAMETER constant`); un valor no constante da `PARAMETER value must be a constant expression`.
   Los valores admiten expresiones constantes enteras (`M = N * 2 + 1`). El tipo debe declararse ANTES del `PARAMETER` (si no, rige el implicito, como en la norma).
4. **DATA**: inicializacion estatica, una vez, al arrancar el programa (todas las unidades; las variables de los subprogramas son estaticas y conservan el valor entre llamadas). Admite listas
   `A, B(2), C /1, 2, 3/`, repeticion `n*valor`, varios grupos por sentencia y arrays completos. Puede ir en cualquier punto de la unidad (tambien tras sentencias ejecutables). No admite DO implicito ni subcadenas como destino. Comprobaciones: tipo del valor (`DATA value type does not match the variable`),
   numero de valores contra el de elementos (`number of DATA values does not match the variables`), repeticion invalida, y que el destino no sea un formal ni un `PARAMETER` (`invalid DATA variable`). El TAC lleva `DATA v,k,n` + `EXT off` (especificacion-tac.md 8b); el backend las emite juntas al principio como cargas inmediatas
   (o `LDIR` desde el pool si es texto) y replica con un `LDIR` solapado cuando hay varios elementos iguales.
5. **READ** (entrada dirigida por lista): `READ *, lista`, `READ (*,*) lista` y `READ (5,*) lista`, solo del teclado. Cada `READ` lee un registro (linea) nuevo; elementos: variables escalares
   y elementos de array (no arrays completos, subcadenas ni DO implicito). Separadores: blancos y comas; `,,` = valor nulo; `/` termina; cadenas con comillas simples o dobles (comilla repetida dentro);
   sin repeticiones `r*c` ni formatos. Dato mal formado o fuera de rango: `Invalid input` y para; sin mas entrada: `End of input` (solo en pruebas). La linea tecleada se repite (eco) con salto de linea.
   `READ` puede ser el cuerpo de un `IF` logico.

## Runtime Z80 (tools/rtgen/rt_entrada.asm)
Bloque `RT_RDD` de 109 B (linea de 79 caracteres + NUL, puntero, flags, registros REAL), `RT_RDLINE` (lee con eco y borrado: `8` mueve el cursor a la izquierda; la salida a pantalla ya lo entiende),
`RT_RDTOK` (siguiente valor, desdobla comillas en su sitio), `RT_RDI/RDL/RDS/RDF` (convierten y guardan) y el teclado: `RT_KEYSC` (primera tecla pulsada, sin contar los shifts), `RT_KEYH` (espera una tecla suelta y la siguiente;
CAPS SHIFT = mayusculas, SYMBOL SHIFT = simbolos de la tabla del Spectrum, CAPS SHIFT+0 = borrar, ENTER = fin de linea) y `RT_KEYT` (solo pruebas de PC: texto en memoria).
Tamano total ~1,1 KB de codigo+datos en la imagen, solo en los programas que usan `READ` (los REAL arrastran ademas las rutinas de `FMUL/FADD/FDIV/FPOW`).
Las tablas de estas rutinas viven en el **banco 48** (`back_rt_entrada.c`) y el backend las lee por el trampolin `t_rt_ent_leer`.
La conversion REAL acumula la mantisa como REAL (`acc*10 + digito`) y escala con `10^|e|` por `FPOW` y `FMUL`/`FDIV`: exacta con <= 7 cifras significativas y |exponente decimal| <= 10; mas alla puede diferir
del PC en el ultimo bit (la lectura correctamente redondeada de cualquier decimal queda para despues). Exponentes por encima de 38 dan `REAL overflow`.

## Pruebas (PC)
`compiler/tests/test_cap10.c` (99 + 3000 aleatorias con `MF_AZAR_N`/`MF_AZAR_SEMILLA`): sintaxis, semantica y errores de cada sentencia nueva; para `READ`, TAC y Z80 deben dar la misma salida (30 casos
dirigidos + 400 aleatorios por defecto, con datos buenos y malos). `tools/fuzz/runf.c` acepta `MF_ENTRADA=fichero` para programas con `READ`. Ejemplos: `compiler/tests/ejemplos/dimen.f`, `param.f`, `datos.f`, `leer.f` (con `leer.entrada.txt`).

## Efectos en memoria (DERIVADO, SIN MEDIR)
- Banco 43 (backend A): ~+0,5 KB de `back.c` (traduccion de `RD/RDLN`, dependencias de rutinas, `DATA`); la tabla de usadas del backend pasa de 2 a 1 byte por rutina (pagina 10) para que quepan las 11 rutinas nuevas.
- Banco 48: ~+1,4 KB de tablas de entrada, `RD.C` (backend C).
- Banco 42 (`sem_decl.c`): DIMENSION/PARAMETER/DATA ~+2 KB; banco 45 (`sem.c`): cuerpo de las sentencias nuevas; banco 47 (`stmt.c`, `clasif.c`): analisis de las sentencias.
- Residente: trampolines nuevos `t_gen_param_item`, `t_gen_read`, `t_stmt_dato`, `t_rt_ent_leer` (~+120 B, sin bss nuevo). Los datos de DATA durante la compilacion usan la pagina 10 (`SEM_DATA_PAGINA`).

## Limites que quedan (para el README de 0.1)
No hay DOUBLE PRECISION, COMMON, EQUIVALENCE, SAVE, BLOCK DATA, ENTRY ni RETURN alternativo; ni procedimientos o intrinsecas como argumento; `DATA` sin DO implicito ni subcadenas; `READ` sin formatos, repeticiones, arrays completos, subcadenas ni DO implicito; `PRINT` no escribe arrays completos; los formales `CHARACTER*(*) A(n)` no estan soportados.
