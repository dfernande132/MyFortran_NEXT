# 0012 - Backend Z80: del TAC al codigo de maquina y al .asm

Fecha: 2026-10-30. Estado: ACORDADO con el autor ("adelante con la opcion 3"); IMPLEMENTADA la etapa A en PC. SIN MEDIR en el Next.

## Decision
Una sola capa de emision con dos salidas a la vez: **bytes en memoria** y **texto .asm** (`z80.c`). El traductor TAC -> Z80
(`back.c`) llama a `z_ins(texto, arg, n, bytes...)`; no hay ensamblador externo en el Next ni en el flujo normal. El `.asm`
es para ver y aprender (fin didactico) y, en PC, para contrastar con `z80asm` de z88dk (comprobacion independiente, opcional).

## Diseno (etapa A: INTEGER y LOGICAL)
- Dos pasadas sobre el TAC: la 1 solo mide (direcciones de etiquetas, rutinas usadas, datos); la 2 emite.
- Acumulador HL, segundo operando DE. Codigo deliberadamente ingenuo (sin optimizar): cada TAC se traduce solo.
- Variables en memoria fija (2 B por INTEGER/LOGICAL; arrays 2 B/elemento, columna-mayor, indices base 0 ya calculados por el TAC).
  Temporales `T<n>` y ocultas `H<n>` de 2 B. LOGICAL = 16 bit 0/1.
- Programa = subrutina: se entra con CALL en `entry` y acaba con RET (HALT y STOP -> RET).
- Salida con `RST 16` (A = caracter, 13 = salto de linea); `rt_putc` conserva todos los registros. En PC el emulador intercepta.
- Rutinas de apoyo propias, emitidas solo si se usan (cierre de dependencias): `rt_putc, rt_neghl, rt_negde, rt_mul, rt_div`
  (con signo, trunca a cero), `rt_pow, rt_lt/ge/gt/le/eq/ne, rt_puti, rt_putl, rt_puts`.
- Disposicion: `[codigo][rutinas][pool de constantes][variables/temporales (sin inicializar)]`. Solo hasta el final de las
  constantes hay bytes en la imagen; `fin_mem` es lo que el cargador debe reservar.
- Simbolos del listado: `V_<nombre>`, `T<n>`, `H<n>`, `L_<etiqueta usuario>`, `I_<n>`, `K_<offset>`, `rt_*`; bloque EQU al final.

## Etapa B: CHARACTER (implementada en PC)
- Una cadena se pasa a las rutinas como **puntero a los datos + longitud de 1 byte** (1..255): la primera en `HL,B`, la segunda en
  `DE,C`. Variable: `V_x` con longitud fija (relleno con blancos); constante: `K_n+2` (los datos del pool); temporal: `Tn` =
  [longitud (2 B, solo se usa el byte bajo)][datos], con capacidad fijada por el ultimo `TMP` (el hueco de `Tn` es el maximo de
  `2` o `2+cap` de todos sus usos, medido en la pasada 1).
- Rutinas nuevas (solo si se usan): `rt_smov` (copia con relleno de blancos o truncado), `rt_sadd` (anade respetando el hueco libre;
  SCAT son dos llamadas), `rt_scpy`, `rt_clamp` (longitud de una subcadena limitada a 0..cap), `rt_scmp` (orden ASCII con la mas
  corta rellena con blancos: HL = -1/0/1, y luego se reutilizan `rt_eq/ne/lt/le/gt/ge` contra 0). `rt_puts` pasa a `HL,B`.
- Arrays CHARACTER: direccion = base + desplazamiento x longitud del elemento (`rt_mul`); LDE copia al temporal, STE usa `rt_smov`.
- NO se traduce `SLEN` (el generador aun no lo emite: no existe LEN); sin probar, da `BACK_NO_SOPORTADO`.
- Una variable sin inicializar contiene lo que haya en memoria (el interprete de TAC la deja a blancos): los programas de prueba no
  leen variables sin asignar.

## Fuera de alcance (error `BACK_NO_SOPORTADO`)
REAL/DOUBLE (ya los rechaza el generador), `SLEN`, mensaje de STOP.

## Verificacion
`platform/pc/z80emu.c` es un emulador Z80 minimo (solo las instrucciones que emite `back.c`; un opcode desconocido aborta).
`compiler/tests/test_cap07.c` ejecuta cada programa en tacvm y en el emulador y compara la salida (35 programas + expresiones
aleatorias). **Salvedad:** emulador y emisor son del mismo autor; un error de interpretacion de un opcode podria estar en ambos.
La comprobacion independiente es ensamblar el `.asm` con `z80asm` de z88dk y comparar con el `.bin` (objetivo `z80-pc`).

## Bancos y memoria en el Next (2026-10-31; PREPARADO, SIN MEDIR)
- **Banco 44**: `tac.c` (escritura del TAC) + `gen.c`. **Banco 43**: `back.c` + `z80.c` + `tac_rd.c` (lectura del TAC). Sem sigue en el 45
  y llama a los ganchos `gen_*` por trampolin (`t_gen_*`); gen y back llaman a sim/etiquetas (45) por `t_sim_leer`, `t_sim_buscar`,
  `t_sim_dim_leer`, `t_sem_etq_indice`, `t_sem_etq_valor`. El estado del TAC (`tac_n`, `tac_pool_n`, `tac_lle`) es GLOBAL (residente) y
  `tac_cuenta()`, `tac_pool_usado()`, `tac_lleno()`, `tac_inm_valor()` son macros: ningun banco necesita llamar a otro para leerlo.
- **Tablas del backend fuera del residente**: las que eran arrays estaticos de PC (`etq_dir` 4.800 B, `var_dir` 1.026 B, direcciones/tamanos/
  capacidades de temporales ~640 B) estan en la **pagina de datos 10** (`BACK_PAG_TABLAS`), ~6,5 KB de 8 KB.
- **Imagen del programa generado** en las paginas **11..12** (`BACK_PAG_IMG`) con `back_generar(NULL, ...)`: un buffer de 64 B en el
  residente y volcado por `banco_escribir`; se lee con `z_imagen_leer()`. Con `img != NULL` se escribe en un buffer como antes (PC).
  `BANCO_PAGINAS` pasa de 10 a **13** (fisicas 50..62); el editor sigue en 6..9.
- `back.c`/`z80.c` ya no usan formatos de printf poco habituales (`%.8s`, `%lu`, `%04X`, `snprintf`): el printf de z88dk no los garantiza.
- Estimacion SIN MEDIR (cuenta mia sobre el tamano de x86-64 `-Os` calibrado con el banco 45 medido): `gen+tac` ~7-8 KB, `back+z80` ~13-15 KB
  (mucho texto de listado). Caben en un banco de 16 KB cada uno solo si la estimacion no se pasa; si el 43 no cabe, habra que partir las rutinas
  de apoyo (`rutina()`) a otro banco o quitar el texto del listado en el Next. Residente nuevo estimado: ~0,4 KB de bss + ~0,7 KB de trampolines.
- MEDIDO (2026-10-31, Dani): primer intento, el banco 43 (back+z80+tac_rd) se pasaba de 16 KB por **1.302 B** (el 44 no dio error). Arreglo: `-DMF_SIN_LISTADO`
  (spike `g1_integrar`) compila fuera los literales del listado .asm (~2,8 KB de texto, estimado) y el bloque de EQU/etiquetas; el listado `.asm` se
  sigue generando en PC. Verificado en PC: misma imagen con y sin la macro (sumas 32D0 y C71A). Tamano resultante SIN MEDIR.
- Spike `g1_integrar` (`tools/spike/g1_*.c`): i1 + bancos 44 y 43; imprime la suma de comprobacion de la imagen para compararla con la de
  `tools/medir/z80.c` en PC (fibonacci: 473 B, suma 32D0; cadenas: 633 B, suma C71A).

## Pendiente
Medir residente, pila y los dos bancos en el Next; cargador/enlazado (donde y como se ejecuta el programa generado: no puede ser en $8000, que es
el residente); mensaje de STOP; REAL.

## Prueba en el Next / emulador (herramienta de PC, sin z88dk)
Primer intento (RST 16 a la ROM con CHAN-OPEN desde un .nex): pantalla negra en ZEsarUX, sin texto. Causa probable (NO confirmada):
atributos a 0 y sysvars sin inicializar. Cambio: `back_salida_pantalla(1)` hace que `rt_putc` dibuje en $4000 con la fuente de
la ROM ($3C00+8*c), cursor en 2 B (`rt_cur`) dentro de la imagen, sin scroll (tras la fila 24 vuelve a la 0). Probado en el emulador
de PC con una fuente falsa (`test_cap07`: `test_pantalla`). `z80-pc` genera `.asm/.bin/.nex` con esta salida y `.esperado.txt` con la
de RST 16. El `.nex` (NEX V1.2 a mano, banco 2) lleva el arranque en $BE00: borra pixeles, atributos = 7, CALL programa, JR $.
SIN VERIFICAR: que el formato NEX cargue y que la fuente de la ROM este en $3D00 en ese entorno.
Si `C:\z88dk\bin\z88dk-z80asm.exe` existe, `z80-pc` ensambla el `.asm` y compara con el `.bin` (comprobacion independiente).

## Fichero .nex y guardado (2026-10-31)
- `compiler/src/nex.c/.h`, portable y sin ficheros: construye por trozos de 64 B el `.nex` (NEX V1.2, un banco de 16 KB = `$8000..$BFFF`, SP=`$BFFE`, PC=`$BE00`; en `$BE00` un arranque de 32 B que borra la pantalla, pone atributos 7 y hace CALL al programa) y lo entrega a una salida que pone el editor. Requiere `org >= $8000` y `fin_mem <= $BE00`.
- `nex_generar(MfSalida*)` (mismo fichero, banco 43) hace back_generar con salida propia a pantalla y escribe por `abrir/escribir/cerrar`. `mf_compilar` lo llama solo si hay una salida registrada con `mf_salida` y no hubo errores: asi el editor decide entre "comprobar" y "construir". Errores de generacion: `mf.gen == MF_GEN_ERR` y `mf.gen_cod` (101..107, `mf_mensaje`).
- Por que el editor pone el esxDOS: el reparto de la seccion 1 de COORDINACION ya le da el acceso a ficheros; el compilador no lleva codigo esxDOS.

## Scroll de la pantalla del ejecutable (2026-11-08, en PC; SIN probar en el Next)
La salida a pantalla (`back_salida_pantalla(1)`) ya no vuelve a la fila 0: al pasar de la fila 23 el cursor se queda en ella, la pantalla sube una fila (copia fila R+1 -> R
para R = 0..22, ocho lineas de 32 B por fila; la direccion de la fila siguiente suma $20 al byte bajo y, si hay acarreo, $08 al alto: cambio de tercio) y se borra la ultima.
El scroll es eager (ocurre al saltar de linea, no al escribir el siguiente caracter). Los atributos no se tocan. Cuesta ~85 B mas en la imagen de todo programa que imprima
(DERIVADO) y 8 x 23 LDIR de 32 B por salto de linea (~125.000 T-estados a 3,5 MHz, ~35 ms; ~4 ms con el Next a 28 MHz, DERIVADO). Pruebas: `test_cap07` (`pantalla_esperada`
modela el scroll; 24, 23+linea partida, 200 lineas) y tres mutantes del Z80 (direccion inicial, acarreo de tercio, fila borrada), todos muertos.
