# ED-003 - Como pinta el editor: tilemap 80x32, colores y estilos

Fecha: 2026-11-01. Conversacion EDITOR. Documento de REFERENCIA para pasar a otra conversacion.
Etiquetas: MEDIDO (visto en ZEsarUX o en el Next real por Dani), DERIVADO (leido del codigo/fuentes),
SIN VERIFICAR (hipotesis sin comprobar). Los colores de las capturas de Dani (ZEsarUX y Next) coinciden con la paleta de abajo.

## 1. Ficheros que lo implementan (pasar estos para analizarlo)
| Fichero | Que hace |
|---|---|
| `editor/src/ed_plat.h` | CONTRATO pantalla/teclado/ficheros: tamano (80x32), filas, los 12 estilos `ED_E_*` y las funciones `plat_*`. Es lo que el editor sabe de la maquina. |
| `editor/plataforma/next/ed_next.c` | Implementacion para el Next: modo de pantalla, paleta, escritura directa en el mapa del tilemap, cursor, teclado. Aqui esta TODO lo especifico del hardware. |
| `editor/src/edit.c` | Genera las filas: `dibujar_linea`, `dibujar_estado`, `dibujar_ayuda_y_mensaje`, `emitir_fila`, y el clasificador de resaltado `clasificar` (con la lista `claves`). Independiente de la maquina. |
| `editor/plataforma/next/spike_tilemap.c` | Spike con el que se midieron los registros y el formato del atributo (historico). |
| `docs/editor/ED-001-editor-basico.md` | Secciones "Aspecto y modo de pantalla", "Resultado del spike del tilemap" y "Port a 80x32 con color". |
Los tests de PC (`editor/tests/test_ed01.c`) simulan la pantalla con una `plat_escribir` falsa y comprueban texto Y estilos fila a fila.

## 2. Contrato (`ed_plat.h`)
- Pantalla `ED_ANCHO x ED_FILAS` = 80 x 32. Fila 0 barra de estado; filas 1-29 texto (29 lineas); fila 30 barra de ayuda; fila 31 mensajes (en modo comando, segunda fila de ayuda).
- `plat_escribir(fila, col, txt, est, n)`: escribe `n` caracteres ASCII 32..126 desde la columna `col`; `est[i]` es el ESTILO del caracter i. El editor SIEMPRE repinta filas enteras de 80 celdas (rellena con blancos del estilo de la fila): la plataforma no tiene que borrar nada.
- `plat_cursor(fila, col)`: cursor de bloque visible. La plataforma lo dibuja como quiera.
- 12 estilos: 0 texto, 1 comentario, 2 etiqueta, 3 continuacion, 4 clave, 5 cadena, 6 numero, 7 fuera de columna 72, 8 estado, 9 ayuda, 10 cursor, 11 error.

## 3. Modo de pantalla del Next (MEDIDO en ZEsarUX con el spike; SIN VERIFICAR en hardware salvo que se indique)
- Se entra con la consola de z88dk: `console_ioctl(IOCTL_GENCON_SET_MODE, 66)` + `IOCTL_GENCON_SET_FONT32` con `font_8x8_bbc_system`, `clrscr()`. Despues la consola NO se usa: se escribe directamente en la memoria del mapa.
- Registros resultantes: `$6B = $C1` (tilemap activo, 80 columnas, CON byte de atributo por celda, paleta 1, bit 0 = tilemap sobre la ULA), `$6C = $80`, `$6E = $2C` (mapa en `$6C00`), `$6F = $0C` (definiciones de tile en `$4C00` si el registro es el MSB en el banco 5: SIN VERIFICAR).
- Mapa: 80 x 32 celdas x 2 bytes = 5.120 B en `$6C00-$7FFF` (ventana MMU3, fuera de la MMU2 `$4000` que usan las paginas de `banco.h`). Celda `i` = byte 2i caracter, 2i+1 atributo. `ed_next.c` lee `$6B` y `$6E` en `plat_iniciar` para no suponer la direccion; si `$6B` bit 5 indica celda de 1 byte, escribe solo caracteres (sin color).
- **Atributo = `estilo << 4`.** MEDIDO: el nibble ALTO es el desplazamiento de paleta; los bits bajos son espejo X/Y y rotacion y se dejan a 0 (con `estilo << 1` salian glifos espejados/girados). Por tanto cada estilo ocupa un bloque de 16 entradas de la paleta del tilemap: **fondo = paleta[16*estilo + 0], tinta = paleta[16*estilo + 1]**.
- Paleta del tilemap: `NextREG $43 = $30` (editar paleta del tilemap, primera paleta; `$70` si `$6B` bit 4 pide la segunda), `$40` = indice, `$41` = valor RRRGGGBB (8 bits) con autoincremento del indice. `paleta_cargar` escribe, para cada estilo s, en el indice `s<<4` los dos bytes {fondo, tinta}, y restaura `$43`.
- Se llama a `intrinsic_ei()` al arrancar: un `.nex` arranca con interrupciones apagadas y el contador `FRAMES` (`$5C78`) lo necesita el teclado.

## 4. Paleta (decision de Dani: aspecto tipo editor de Pascal65, fondo azul marino)
Valores RRRGGGBB (3-3-2 bits). La columna "RGB aprox." es una conversion lineal a 8 bits por canal solo para orientar; el Next extiende el valor a 9 bits y el tono exacto depende de ello.
| Estilo | Fondo | Tinta | RGB aprox. fondo / tinta | Uso |
|---|---|---|---|---|
| 0 texto | `$25` | `$DA` | #242455 / #DBDBAA | texto normal (azul marino / blanco roto) |
| 1 comentario | `$25` | `$6E` | / #6D6DAA | linea con C, c o * en la columna 1 |
| 2 etiqueta | `$25` | `$FC` | / #FFFF00 | digitos en columnas 1-5 |
| 3 continuacion | `$25` | `$E8` | / #FF4900 | caracter no blanco en la columna 6 |
| 4 clave | `$25` | `$F2` | / #FF92AA | PROGRAM, INTEGER, DO, IF, THEN, PRINT... (lista `claves` en `edit.c`) |
| 5 cadena | `$25` | `$7D` | / #6DFF55 | entre apostrofes |
| 6 numero | `$25` | `$F5` | / #FFB655 | digitos que no siguen a una letra |
| 7 fuera | `$25` | `$4A` | / #4949AA | columnas 73 en adelante (F77 las ignora) |
| 8 estado | `$DB` | `$25` | #DBDBFF / #242455 | fila 0 (gris claro con texto marino) |
| 9 ayuda | `$4F` | `$FF` | #496DFF / #FFFFFF | filas 30 y (modo comando) 31 |
| 10 cursor | `$FC` | `$25` | #FFFF00 / #242455 | cursor de bloque |
| 11 error | `$A0` | `$FF` | #B60000 / #FFFFFF | fila de mensajes con un error |
Para cambiar un color basta editar `paleta[][]` en `ed_next.c`; para anadir un estilo, `ED_E_*` y `ED_N_ESTILOS` en `ed_plat.h`, una entrada en `paleta` y quien lo asigne en `edit.c` (hay hasta 16 estilos posibles por el nibble).

## 5. Resaltado (`clasificar`, en `edit.c`, independiente de la maquina)
Cada linea se clasifica SOLA (F77 no tiene estado entre lineas), recibiendo el trozo visible `s[0..n-1]` y su columna absoluta `col0` (hay desplazamiento horizontal si la linea pasa de 80 columnas, solo posible en ficheros cargados; el editor no deja teclear mas alla de la columna 72). Orden de decision por caracter: linea de comentario -> `FUERA` (col >= 72) -> columnas 1-5 (etiqueta si es digito) -> columna 6 (continuacion si no es blanco) -> apostrofe/cadena -> palabra clave (comparacion sin distinguir mayusculas, la palabra entera) -> numero. Los caracteres fuera de 32..126 se pintan como `.`.

## 6. Cursor y repintado
- `plat_cursor` cambia el byte de ATRIBUTO de la celda al estilo `CURSOR` y recuerda (direccion, valor original, valor nuevo); al moverse restaura el original SOLO si el atributo sigue siendo el nuevo (si la fila se repinto entretanto, ya no es el cursor y no se toca).
- `ed_refrescar`: repinta todas las filas de texto si `ed.sucio_todo`, solo la del cursor si `ed.sucio_fila`; despues barra de estado, ayuda y mensaje, y coloca el cursor. Una fila = 160 bytes escritos (80 caracteres + 80 atributos); un ENTER repinta 29 filas. Velocidad a 3,5 MHz: ver "pendiente".
- Barra de estado: ` MYFORTRAN  <fichero>` a la izquierda; desde la columna 40 `L:<linea> C:<col>  Free <n>  Key <hex>` y `  CMD` / `  FILE` / `  *` (modificado).

## 7. Teclado (en `ed_next.c`, relacionado con la pantalla solo por compartir fichero)
`in_Inkey()` (codigos crudos, sin eco), repeticion con `FRAMES` ($5C78): `ED_REP_INICIO` 15 fotogramas hasta repetir, `ED_REP_PERIODO` 2. Esc = CAPS SHIFT + SPACE (llega como 0x20; se distingue leyendo el puerto `$FEFE` bit 0) con tratamiento de la asimetria de pulsar/soltar (H7b). Flechas del Next real: codigos 8/9/10/11 son HIPOTESIS (H3); un programa de sondeo de teclas esta pendiente.

## 8. Pendiente de medir / limites conocidos
- Velocidad de repintado a 3,5 MHz en el Next real (en ZEsarUX y en el Next de Dani resulta usable; sin cifras).
- `$6F`/ubicacion de las definiciones de tile: SIN VERIFICAR (el programa funciona porque la consola de z88dk ya las deja puestas).
- Sin scroll por hardware: el desplazamiento vertical es repintar filas.
- Solo ASCII 32..126 (la fuente `font_8x8_bbc_system` dibuja el apostrofe como comilla curva, visible en las capturas).
