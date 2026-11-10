# Escala de pila (Next)

Programas para medir cuanta pila gasta el compilador al analizar expresiones anidadas, con el comando secreto `Esc` `M` del
editor (pila usada / pila pintada). Cada familia sube un nivel de anidamiento; se abren de menor a mayor **sin reiniciar el
editor** (la marca es un maximo acumulado) y se anota `Esc` `M` despues de `Esc` `C` y de `Esc` `O`.

| Familia | Anida |
|---|---|
| `pNN.f` | parentesis `((((I+N))))` |
| `fNN.f` | `ABS(ABS(...))` (cada nivel cuenta doble) |
| `mNN.f` | `ABS((...))` (cada nivel cuenta triple) |
| `bNN.f` | IF/DO anidados (no gastan pila del parser) |
| `sNN.f` | suma de NN operandos `I+N+I+...` (arbol profundo hacia la izquierda, con lineas de continuacion): `tipar` y `gen_expr` recursan NN-1 niveles aunque el parser no anide. `s31` es el maximo (63 nodos del AST); `s32` da `expression too large` |
| `qNN.f` | NN funciones-sentencia encadenadas (`JA2(K) = JA1(K) + 1`...): `gsfun`/`sf_copiar` anidados. `q10` es el maximo; `q11` da `statement function too large` |
| `maximo.f` | todo al maximo permitido a la vez |

Con `EXPR_PROF_MAX = 12` compilan sin error `p10`, `f05`, `m03`, `b16` y `maximo.f`; los mayores dan "expression too complex".
Si se cambia ese limite hay que regenerarlos. Medidas de la 0.1: ~35 B por unidad de presupuesto, base ~175 B, `maximo.f` 583/645
(ver `docs/COORDINACION.md`, 2026-11-08). La version de regresion automatica es `compiler/tests/programas/p14_limites.f`.

## Pendiente de medir en el Next (0.11)

`s08`, `s16`, `s24`, `s30`, `s31`, `q04`, `q08`, `q10`: abrir de menor a mayor **reiniciando el editor entre familias** (la marca de `Esc` `M` es un maximo acumulado) y anotar `Esc` `M` tras `Esc` `C` y tras `Esc` `O`. En PC los compila `compiler/tests/test_pila.c`
(con los limites: el maximo compila, el siguiente da el error controlado). Ese test tambien ejecuta en z80emu cadenas de 62 llamadas y mide la pila de los programas generados (SP inicial - SP minimo).
