# 0004 - Tabuladores en el fuente

Estado: aceptada (cierre del capitulo 1).

## Contexto
El estandar ANSI X3.9-1978 no contempla el tabulador. En la practica (VS Code, codigo DEC/g77) el
usuario pulsa Tab al inicio de linea. Sin tratarlo, `<Tab>X=1` pone la `X` en la columna 2 y da
error de etiqueta. La convencion DEC/g77 se cita de memoria; no esta verificada contra su documentacion.

## Decision
En `tomar_linea` (source.c), al leer cada linea fisica:
1. Tab en las columnas 1-6: se sustituye por blancos hasta la columna 6; el siguiente caracter
   cae en la columna 7. El numero de blancos es variable (1 a 6), no fijo.
2. Tab en el campo de etiqueta (cols 1-5) seguido de un digito 1-9: ese digito es el marcador de
   continuacion (columna 6) y el texto empieza en la 7. `<Tab>0` no es continuacion.
3. Tab desde la columna 7: se deja tal cual; el escaner lo trata como blanco de un caracter.
   Expandirlo a paradas de tabulacion desplazaria las columnas y podria empujar texto mas alla de la 72.

Una linea con solo tabuladores es una linea en blanco.

## Consecuencias
- Las columnas de los mensajes de error se refieren al texto ya expandido (la `X` de `<Tab>X=1`
  es la columna 7), no a la posicion fisica del byte.
- Es una extension: el modo estricto, si existe, deberia rechazarla.
- Tests: `test_tabuladores` (cap01).
