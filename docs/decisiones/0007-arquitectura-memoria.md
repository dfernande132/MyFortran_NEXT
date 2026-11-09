# 0007 - Arquitectura de memoria del compilador nativo

Fecha: 2026-10-30. Decidido con el autor tras los spikes S1-S4 del capitulo 3
(`docs/capitulos/cap03-arquitectura.md`, todo medido en CSpect; hardware real sin probar).

## Decision
1. **Residente** (`$8000-$BFFF`, 16 KB): libreria C, estado global de TODAS las fases (decision 0006),
   trampolines de banco, servicios comunes (E/S, pantalla, paginacion de datos) y la pila.
   Residente + pila deben acabar por debajo de `$C000`.
2. **Codigo en bancos de 16 KB**, uno por FASE (lectura, expresiones, ...), cada uno con
   `#pragma bank N` (seccion `CODE_N`, ORG `$C000`; appmake lo empaqueta en el `.nex`).
   Solo UN banco esta visible a la vez, en MMU6/MMU7.
3. **Entrar en un banco o pasar de uno a otro = trampolin residente** `t_*`: guarda el banco actual,
   mapea el destino, llama, restaura. Un banco nunca llama a otro directamente.
4. **Los trampolines mapean SIN `DI`/`EI`.** Con `DI` se pierden interrupciones (S3b: FRAMES 61-66 de
   100; sin `DI` 93-97). Contrato: ninguna rutina de interrupcion ejecuta codigo ni lee datos en `$C000+`,
   y su pila esta bajo `$C000`.
5. **Datos grandes en paginas** por la API `platform/banco.h` (ventana MMU2 `$4000-$5FFF`, paginas desde 50),
   con acceso en BLOQUES (coste fijo por acceso; S1: x1.2-1.3 extremo a extremo). Candidatos: tabla de
   simbolos, codigo intermedio, codigo objeto, texto fuente. El AST de una sentencia y la tabla de cadenas
   se quedan residentes.
6. **Bancos usados por el compilador**: 47, 46, 45... hacia abajo (paginas 94-95, 92-93, ...). Evitar paginas
   0-27 y 40-47 (problemas no explicados en MyLISP).

## Reglas que salen de los spikes
- **RODATA**: un banco no puede devolver ni guardar punteros a su propio RODATA (dejaria de estar
  mapeado). Los textos que cruzan fronteras de banco viven en el residente o se copian.
- **Estado global y punteros a datos residentes** cruzan bancos sin problema (estan siempre mapeados).
- **Fuentes del compilador intactas**: el reparto se hace con wrappers `unity build` y `#define` a los
  trampolines (S4). Los nombres `static` de modulos que comparten banco no pueden repetirse
  (hoy: `es_letra`/`es_digito` en lexer.c y clasif.c; unificar).
- Cada banco ocupa 16 KB en el `.nex` aunque su contenido sea menor.
- Pila: S4 mide 300/580/860 B (n=4/12/20) frente a 290/570/850 sin bancos (+10 B por los marcos de los
  trampolines).

## Medido (referencia)
| | Todo residente | S4 (2 bancos) |
|---|---|---|
| Imagen residente | 21.630 B | 12.473 B |
| Codigo en bancos | - | 7.677 + 2.398 B (caben 8.707 y 13.986 B mas) |
| Mapeos en la ejecucion de prueba | - | 740 (~40 ms estimados) |
| Coste de mapeo (ida y vuelta) | - | ~50-70 us estimados (S2/S3, 50 Hz supuesto) |

## No resuelto
- Hardware real (todo medido en CSpect; los tiempos exactos son orientativos).
- Cargar codigo desde SD (`esxdos_f_read`) o lanzar otro programa desde un `.nex`: no probado y no hace
  falta mientras cada fase quepa en 16 KB.
- Cuando el IDE comparta el residente con el compilador: reparto de los 16 KB (capitulo del IDE).
- Interrupciones propias del IDE (teclado): deben respetar el contrato del punto 4.
