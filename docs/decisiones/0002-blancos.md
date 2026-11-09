# 0002 - Conservar los blancos y saltarlos al leer

Fecha: 2026-10-29. Propuesto por el autor; adoptado tras discutirlo.

## Decision

La sentencia lleva el texto ORIGINAL (columnas 7-72, con blancos y minusculas).
El escaner y el clasificador leen a traves de dos primitivas, `mirar(k)` y
`avanzar(n)`, que saltan los blancos y pasan a mayusculas. Las constantes de
texto se leen en bruto. Se elimina `normalizar_sentencia`.

Cada linea ocupa exactamente `SEGMENTO` (66) caracteres: las que van seguidas de
una continuacion se rellenan con blancos hasta la columna 72. Asi la posicion
en el fichero de `texto[p]` es linea `linea_seg[p / 66]`, columna `p % 66 + 7`
(`posicion_fuente`), sin tabla de columnas.

## Por que

- Los mensajes de error y, sobre todo, el resaltado de sintaxis del editor
  necesitan la columna original de cada token. Con la normalizacion previa habia
  que guardar un mapa de columnas (255 bytes de RAM en el Next).
- `FORMAT(5HHOLA X)` conserva sus blancos.
- Mas fiel al estandar: `1 . EQ . 2`, `1 0 0` (= 100) y el relleno hasta la
  columna 72 en cadenas partidas (este ultimo punto, segun el estandar, de memoria).

## Lo que NO resuelve

La ambiguedad `DO10I=1.10` / `DO 10 I=1,10` no depende de los blancos (el
estandar permite escribir el DO sin ellos): la decide la coma a nivel de
parentesis 0. Sigue siendo trabajo de la clasificacion de sentencias (2.4).

## Costes

- `mirar(k)` pasa de O(1) a O(distancia). No medido en el Z80.
- Tabuladores: ver 0004 (se expanden en las columnas 1-6; en el cuerpo, un blanco de un caracter).
- Una sentencia ocupa como maximo 20 lineas (`MAX_SEGMENTOS`), 1320 caracteres (antes 4/264; ver cap01 §9).
