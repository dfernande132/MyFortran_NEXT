# Capítulo 3 - Arquitectura de memoria (spikes)

Estado: **cerrado** (S1-S4 medidos en CSpect; decisión 0007). Se resuelve ANTES del parser de sentencias (que pasa a ser el capítulo 4): en MyLISP la
arquitectura de memoria se decidió tarde y se pagó en cada compilación.

## 1. Qué hay que decidir

El compilador completo no cabrá residente en los 32 KB de `$8000-$FFFF` (front-end ya ~9,4 KB de código; faltan
semántico, código intermedio, generador y librería). Hay que fijar:

1. **Qué datos van a páginas** (fuera de los 32 KB) y con qué interfaz. Candidatos: tabla de símbolos del
   programa, código intermedio, código objeto, texto fuente del editor. El AST de UNA sentencia (1 KB) y la tabla
   de cadenas (1 KB) no lo necesitan.
2. **Si el código se parte en overlays** y cómo se cargan (SD, o bancos empaquetados en el `.nex`) y cómo llaman
   al código residente.
3. **Dónde está la ventana de datos y dónde la de código**, y qué hace el IDE con la pantalla mientras tanto.

## 2. Lo que ya sabemos (docs/referencia)

- Datos paginados con `MMU2` (`$4000-$5FFF`) por accesores: **probado** en MyLISP (heap ≈ 2,3 veces más lento).
- Código en bancos: **nunca probado en el Next**. `__banked` y `__far` no funcionan en sccz80. Ideas sin probar:
  trampolín por `MMU3` (`$6000-$7FFF`), overlays desde la SD con `esxdos_f_read`, secciones `CRT_ORG_BANK_n`.
- Un `.nex` no vuelve al Browser; lanzar otro programa desde un `.nex` no está comprobado.
- El overlay de MEGA65 (otro compilador) no es evidencia para el Next.

## 3. Escalera de experimentos (una incógnita nueva por paso)

| Spike | Pregunta | Resultado |
|---|---|---|
| **S1** `s1_banco` | ¿Funcionan los accesores de datos y el cronómetro por raster? ¿Cuánto cuesta paginar? | sí; x1.2-1.3 |
| **S2** `s2_codigo` | ¿Se puede compilar código en un banco, empaquetarlo en el `.nex` y llamarlo desde el residente? | sí (`#pragma bank N`, pila bajo $C000) |
| **S3** `s3_codigo` | ¿Varios bancos y llamadas banco→banco por trampolín? ¿Se pierden interrupciones? | sí; con `DI` se pierden, sin `DI` no |
| **S4** `s4_medir` | ¿Se parte el front-end real en bancos sin tocar sus fuentes? ¿Cuánto residente queda? | sí; 21.630 → 12.473 B, salida idéntica a PC |
| S5 (no hecho) | Cargar código desde SD o lanzar otro programa desde un `.nex` | descartado por ahora: no hace falta mientras cada fase quepa en 16 KB |

La API de datos es `platform/banco.h`: PC con un array (los tests de PC la ejercitan), Next con paginación.

## 4. Resultados

### S1 — Datos en páginas (medido en Next, sccz80 sin optimizar)

| Medida | Resultado |
|---|---|
| Ida y vuelta por la ventana MMU2 (P1) | OK |
| Temporizador por línea de raster | 219/220/220, estable (válido) |
| Acceso 8 B: plano / banco | 2400 / 1872 (ratio x1.2) |
| Acceso 256 B: plano / banco | 464 / 384 (ratio x1.2) |
| Imagen / margen | 15132 / 17635 bytes |
| Código residente del módulo `banco` | ~93 B |

Lectura: el coste extremo a extremo de usar datos paginados es x1.2-1.3; los ratios están diluidos por la sobrecarga del propio banco de pruebas, así que el coste fijo por acceso (DI, mapeo, copia, restauración, EI) es aproximadamente constante y conviene agrupar accesos (copiar bloques). No es un obstáculo: los datos pueden vivir en páginas.


### S2 — Código en un banco (medido en CSpect, sccz80 sin optimizar)

Ficheros: `tools/spike/s2_codigo.c` (residente), `s2_banco47.c` (`#pragma bank 47` → sección `CODE_47`, ORG $C000, banco 16K nº 47 = páginas 94/95), `s2_codigo.flags` (`REGISTER_SP=0xBFF0`).

| Pregunta | Resultado |
|---|---|
| P0 MMU6/MMU7 al arrancar el .nex | 0 y 1 (restauradas igual tras las llamadas) |
| P0 pila / dirección del banco | pila ~ 49068 ($BFAC, bajo $C000); `b47_suma` en 49152 ($C000) |
| P1 compila, enlaza y empaqueta | OK: `s2_codigo_BANK_47.bin` 131 B dentro del .nex (33280 B); imagen residente 7924 B |
| P2 llamada DI→mapear MMU6/7→llamar→restaurar→EI | OK (suma, checksum sobre buffer residente) |
| P3 banco→residente y RODATA del banco | OK (141; cadena constante correcta) |
| P4 llamadas en 10 fotogramas | residente 4672, paginada 2128 → x2.1 |

Lectura: con 10 fotogramas = 0,2 s (supuesto 50 Hz, CSpect) el coste por llamada es ~43 µs residente y ~94 µs paginada, es decir ~51 µs de sobrecarga fija por entrada al banco (estimado). La función de prueba es trivial, así que x2.1 es el peor caso: una fase que trabaje ms o segundos por llamada no lo nota. Conclusión: el código en bancos funciona de extremo a extremo.

Restricciones que impone: (1) residente + pila debe caber bajo $C000 (desde $8000: ≤16 KB); (2) solo hay un banco de 16 KB visible a la vez en $C000; (3) una llamada de banco a banco necesita un trampolín residente que mapee el destino y restaure el origen.

Sin medir: llamada banco→banco, interrupciones activas durante la llamada, hardware real, carga del código desde SD (S3).

### S3 — Varios bancos y llamadas banco→banco (medido en CSpect)

Ficheros: `tools/spike/s3_codigo.c` (residente con trampolines `entrar_bNN_*`), `s3_banco47.c`, `s3_banco46.c`, `s3_banco45.c`, `s3_codigo.flags`.

Diseño: un banco nunca llama a otro directamente; llama a un trampolín residente que guarda el banco actual, mapea el destino en MMU6/7, llama y restaura el anterior. Cadena de prueba 47→46→45 (esperado 54); cada banco lee su propio RODATA al volver.

| Pregunta | Resultado |
|---|---|
| P1 cadena 47→46→45 | 54 OK; 47→46 hoja 12 OK; 45 directo 17 OK; banco actual al volver 255 OK; MMU6/7 = 0/1 restauradas |
| P2 llamadas en 10 fotogramas | residente 3664 (~54 µs); 1 banco 1904 (~105 µs); 2 niveles 1136 (~176 µs); 3 niveles 720 (~277 µs) |
| P3 FRAMES con interrupciones activas | 5 → 14, delta 9 (esperado ~10); ver S3b |

Lectura: el mecanismo banco→banco funciona y cada banco conserva su identidad al volver (lo prueba la lectura de RODATA propio tras el retorno). Cada mapeo de ida y vuelta cuesta del orden de 50-70 µs (estimado a 50 Hz; el nivel 3 incluye además más trabajo en las funciones de prueba, así que no es un coste puro por nivel). No son comparables en valor absoluto con S2 (otra función de referencia y otra envoltura de medida). FRAMES avanza con los bancos cambiando, es decir, la rutina de interrupción del ROM sigue ejecutándose y nada se corrompió. El delta 9 frente a ~10 esperado es una sola medida: no permite decidir si se pierde alguna interrupción; pendiente de una tanda más larga con control sin mapeo.

#### S3b — tanda larga: ¿se pierden interrupciones? (100 fotogramas de raster, CSpect)

Se lee FRAMES (ROM, IM1) justo tras detectar el cambio de fotograma, al principio y al final; sin pérdidas debería dar 100. Se mira el raster tras cada operación.

| Caso | FRAMES (de 100) |
|---|---|
| con DI: nada (bucle vacío) | 86 |
| con DI: residente (sin mapeo) | 96 |
| con DI: 1 banco | 61 (1ª ejecución: 75) |
| con DI: 3 niveles | 66 (1ª ejecución: 57) |
| SIN DI: 1 banco | 93 |
| SIN DI: 3 niveles | 97 |

Lectura: con `DI`/`EI` alrededor de cada mapeo se pierden 25-40 interrupciones de 100; sin `DI` el resultado cae en el rango de los controles (93-97) y no hubo corrupción (P1 OK con ambas variantes). Sin `DI` las dos escrituras MMU6/MMU7 no son atómicas, lo cual es seguro mientras la rutina de interrupción no ejecute código ni acceda a datos en $C000+ y su pila esté bajo $C000 (así es con el ROM/IM1 de este spike).

Hipótesis (no demostrada): la interrupción de cada fotograma es un pulso corto del Z80 y se pierde si cae dentro de la ventana con `DI`.

Ruido sin explicar: los controles (sin ningún `DI`) dan 86 y 96, y las medidas con `DI` varían mucho entre ejecuciones (1 banco: 75 y 61; operaciones de la tanda 12706 y 7897). El orden de magnitud del efecto del `DI` (−30 puntos) está muy por encima del ruido (±10), pero los números exactos de tiempo en CSpect son solo orientativos; hace falta hardware real. P2 repetido: residente 3296 (~60 µs), 1 banco 1744 (~114 µs), 2 niveles 1024 (~195 µs), 3 niveles 656 (~304 µs), ~10 % por encima de la primera ejecución.

Decisión provisional: el trampolín mapea SIN `DI` (contrato: ninguna rutina de interrupción usa $C000+). Se revisará en hardware real y cuando haya IDE/teclado.

Restricciones heredadas de S2 (siguen en pie): residente + pila bajo $C000; un único banco de 16 KB visible a la vez; llamada banco→banco solo vía trampolín residente.

Sin medir: pérdida de interrupciones en tandas largas, hardware real, carga del código desde SD.

### S4 — Front-end real repartido en bancos (medido en CSpect, sccz80 sin optimizar)

Ficheros: `tools/spike/s4_banco46.c` (source + lexer + clasif, unity build), `s4_banco47.c` (ast + expr; `lex_siguiente` redirigido al trampolín), `s4_trampolines.c` (residente, 9 trampolines), `s4_medir.c` (el `medir_core.c` de siempre con `#define` a los trampolines), `s4_medir.flags` (`REGISTER_SP=0xBFF0`). El código del compilador no se modifica; estado global (sentencia, lexer, cadenas, parser, ast) en el residente.

| Medida | `core` (todo residente) | S4 (2 bancos) |
|---|---|---|
| Imagen residente (incluye bss) | 21630 B | 12473 B (termina en $B0B9) |
| Código en bancos | – | banco 46: 7677 B; banco 47: 2398 B |
| Hueco libre en cada banco | – | 8707 B y 13986 B |
| Pila n=4 / 12 / 20 | 290 / 570 / 850 | 300 / 580 / 860 |
| Mapeos de banco en la ejecución | – | 740 (~40 ms estimados a 55 µs) |

Salida del programa: `sentencias=13 tokens=37 errores=1 desconocidas=0 tabla=47`, `expresiones=9 ok=6 nodos=53 profunda=1`. Contrastada con la salida de PC: coincide en todos los valores.

Lectura: el front-end real funciona partido en bancos con trampolines, sin tocar sus fuentes. El residente baja 9157 B. La pila sube 10 B por punto (los marcos de los trampolines). El residente (12473 B) más la pila (hueco hasta $BFF0: ~3,9 KB) sigue cabiendo bajo $C000. Composición aproximada del residente: bss del compilador 4926 B (estado global), código de driver/trampolines 1882 B, rodata del driver 630 B, el resto es librería C del z88dk (printf, fuente 4x8…).

Hallazgos: (1) `lexer.c` y `clasif.c` duplican `es_letra`/`es_digito` (chocan al juntar módulos); (2) un banco no puede devolver ni guardar punteros a su RODATA (`expr_mensaje`, `lex_mensaje`); (3) cada banco ocupa 16 KB en el `.nex` aunque su contenido sea menor.

## 5. Conclusiones

1. La arquitectura es viable en el Next: residente pequeño, estado global residente, código por fases en
   bancos de 16 KB, datos grandes en páginas. Decisión completa y reglas en `docs/decisiones/0007-arquitectura-memoria.md`.
2. El front-end real (lexer, clasificador, expresiones) pasa de 21.630 B residentes a 12.473 B sin modificar
   sus fuentes y con salida idéntica a PC; el coste de paginación de la ejecución de prueba es del orden de 40 ms.
3. El coste fijo de entrar en un banco (~50-70 µs estimados) obliga a diseñar las fases con pocas entradas
   y mucho trabajo por entrada; la API de datos, a acceder en bloques.
4. Los trampolines no usan `DI`: con `DI` se pierden interrupciones. El IDE debe respetar que ninguna
   interrupción toque `$C000+`.
5. Límites de lo medido: CSpect (tiempos orientativos, ruido de ±10 % entre ejecuciones), sin hardware real,
   sin carga desde SD. Los números de coste son estimaciones a 50 Hz.
6. Reglas de código que impone: RODATA no cruza bancos; nombres `static` únicos por banco; wrappers `unity build`
   en lugar de tocar los módulos.

Siguiente: capítulo 4 (parser de sentencias), escrito ya contra esta arquitectura (una fase = un banco,
estado global residente).
