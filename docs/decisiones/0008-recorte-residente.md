# 0008 - Recorte del residente

Fecha: 2026-10-30. Pedido por Dani: dejar espacio al editor en `$8000-$BFFF`.

## Donde estaban los 14.456 B del residente (c4_medir.map, medido)
| Parte | Bytes | Es del compilador? |
|---|---|---|
| Driver de medida (`c4_medir.c` + `medir_core.c`): codigo 1.407, bss 1.177 (incluye un `ast` propio de 1.026), rodata 1.111 | 3.695 | no: solo existe para medir |
| Estado global del compilador (bss): `sentencia` 1.370, `lexer` 1.327, `cadenas` 1.026, `stm_ast` 1.026, `stm` 34, `parser` 19, otros 12 | 4.814 | si |
| Trampolines (14 stubs) 637 + `banco_next.c` 93 | 730 | si |
| Libreria C y runtime de z88dk: consola 64 col, fuente 4x8 (768), printf, teclado `in_Inkey`, aritmetica, crt... | ~5.217 | no: depende de lo que enlace el programa final |

Conclusion: el compilador real son ~5,5 KB residentes. El resto es el driver de pruebas y la libreria C.

## Hecho: se elimina `Lexer.tmp` (-1.321 B de bss)
Era el buffer de trabajo del lexer (nombres, reales, cadenas, operadores). Ningun texto puede ser mayor que lo que
queda libre en la tabla de cadenas (si no, `LEX_TABLA_LLENA`), asi que el lexer escribe directamente en la COLA LIBRE de
la tabla y `tabla_internar` lo confirma (`usado += len+1`) o lo descarta si ya estaba. Nombres (<= 8) y operadores con
punto usan un buffer local de pila (<= 9 B), para que un nombre ya presente se encuentre aunque la tabla este llena.
- Cambio de comportamiento en el limite (documentado, probado): un REAL o una constante de texto cuyo texto no quepa en
  la cola libre falla con `LEX_TABLA_LLENA` aunque ya existiera igual en la tabla (antes se encontraba).
- Pruebas nuevas en `test_cap01.c` (`test_lexer_cola_tabla`, 32 pruebas): tabla exactamente llena, limite de 4 bytes
  libres, comillas dobladas en el limite, cadena de 1000 y de 1300 caracteres, cadena larga sin cerrar, operadores con
  punto de 7, 8 y 12 letras, repeticion sin crecer la tabla. Mutacion: 10 mutantes, 8 detectados; los 2 supervivientes
  son equivalentes (copiar un texto sobre si mismo; leer un buffer sin inicializar que no puede coincidir con ningun operador).
- PENDIENTE de medir en el Next: residente esperado ~13.135 B (14.456 - 1.321).

## Hecho: B, `AST_NODOS` 128 -> 64 con listas en flujo (-512 B de bss)
- Las listas (declaradores, expresiones de PRINT, etiquetas de GOTO calculado) ya no se acumulan en el AST: `stmt_analizar()`
  deja el cursor en el primer elemento y cada `stmt_elemento()` analiza UNO (en `stm.e`) y libera los nodos del anterior
  (`stm_ast.usados = marca`). La cola (`)` y selector del GOTO calculado, fin de sentencia) se procesa cuando se acaba la lista,
  asi que **los errores de un elemento o del cierre solo aparecen al avanzar**: hay que llamar a `stmt_elemento()` hasta que
  devuelva 0 y despues mirar `stm.err`. Contrato para el generador (cap. 5+): `stmt_analizar(); while (stmt_elemento()) {...}`.
- Limite nuevo: un UNICO elemento (una expresion, un declarador) cabe en 63 nodos (p. ej. ~31 sumas encadenadas);
  una sentencia puede tener cualquier numero de elementos (probado: 60 declaradores, 200 expresiones de PRINT, 100 etiquetas).
- Medido en PC: maximo vivo en las pruebas = 9 nodos; en el programa de `medir_core` = 7. Pruebas: cap02 191, cap04 211, cap01 342, cap03 69,
  todas con ASan/UBSan; fuzz 300.000 sentencias mutadas sin fallo; mutacion de la logica de flujo: detectados 4 de 5 (el superviviente,
  `stm.err != STM_OK` en `stmt_elemento`, es equivalente porque el parser es pegajoso tras un error).
- MEDIDO en el Next (c4_medir, tmp + B): residente 12.196 B (era 14.456; -2.260 B, mas de los -1.833 B esperados). Bancos: 46 = 8.061 B (+384), 47 = 5.211 B (-67). Margen frente a 32.767: 20.571 B. Hueco hasta la pila: ~4.170 B (derivado de 0xBFF0 - (0x8000+12.196), sin leer el .map).

## Decidido: D, zona de trabajo compartida (se aplica desde el capitulo 5)
- `sentencia.texto` (1.321 B) es lo unico que el lexer lee. Su vida util acaba cuando `stmt_elemento()` devuelve 0 (las listas
  se analizan en flujo, asi que el texto sigue haciendo falta hasta ahi). Despues solo quedan posiciones BRUTAS (`err_pos`), que
  son numeros y no necesitan el texto.
- Regla: tras terminar una sentencia, `texto` es memoria de TRABAJO de la fase siguiente (tabla de simbolos temporal,
  generador de codigo). Se accede por un unico nombre (`zona_trabajo()` / `ZONA_TRABAJO_MAX`, a anadir en cap. 5) y NO por
  `sentencia.texto`. Una marca centinela al principio (`ZONA_MARCA`) se comprueba en pruebas PC para pillar lecturas del texto tras ceder la zona.
- No se anade codigo aun: sin consumidor no hay nada que medir. El ahorro real es "no reservar 1.3 KB mas" en el cap. 5 y siguientes.

## Opciones pendientes (cada una cambia un limite del lenguaje o el diseno; decide Dani)
| Opcion | Ahorro | Coste |
|---|---|---|
| A. `MAX_SEGMENTOS` 20 -> 10 (el estandar F77 Subset admite 9 continuaciones; Full, 19: ANSI X3.9-1978 §3.2.3) | ~680 B | rechaza sentencias de 11 a 20 lineas (legales en F77 Full) |
| ~~B. `AST_NODOS` 128 -> 64~~ HECHO | 512 B | limita sentencias grandes (~20 declaradores, expresiones de ~60 nodos); se mitiga procesando las listas de declaradores/PRINT en flujo (un elemento cada vez) en vez de acumularlas |
| C. Tabla de cadenas 1.024 -> 512 y reinicio POR SENTENCIA (los nombres se copian a la tabla de simbolos paginada en el cap. 5) | 512 B | ata el diseno del cap. 5; las constantes de texto > ~500 caracteres fallan |
| D. Area de trabajo compartida entre fases: `sentencia.texto` (1.321 B) solo hace falta hasta terminar `stmt_analizar` (luego basta la posicion para errores); reutilizarla como memoria de trabajo de la fase siguiente (generador, simbolos) | hasta 1.321 B de bss de OTRA fase que no hay que anadir | disciplina: ninguna fase posterior puede leer el texto; se prueba con una marca |
| E. Trampolin generico en ensamblador (tabla banco+direccion) en vez de 14 stubs de C | ~300-400 B (estimado, sin probar) | ensamblador Z80 y convencion de llamada de sccz80 |
| F. El editor NO enlaza `printf`/consola de z88dk y usa rutinas propias | hasta ~5 KB | lado del editor; el compilador no usa stdio |
