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
| `maximo.f` | todo al maximo permitido a la vez |

Con `EXPR_PROF_MAX = 12` compilan sin error `p10`, `f05`, `m03`, `b16` y `maximo.f`; los mayores dan "expression too complex".
Si se cambia ese limite hay que regenerarlos. Medidas de la 0.1: ~35 B por unidad de presupuesto, base ~175 B, `maximo.f` 583/645
(ver `docs/COORDINACION.md`, 2026-11-08). La version de regresion automatica es `compiler/tests/programas/p14_limites.f`.
