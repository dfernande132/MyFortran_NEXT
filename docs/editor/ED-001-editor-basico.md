# ED-001 - Editor basico: buffer de hueco, pantalla de 64 columnas, teclado bloqueante

Fecha: 2026-10-30. Conversacion EDITOR. Estado: codigo y 277 pruebas en PC (ver abajo);
NADA medido ni probado todavia en el Next.

## Alcance de este paso
Un `.nex` AUTONOMO (no usa el compilador) que arranca con un texto de ejemplo y deja
escribir, borrar y moverse. Carga `prueba.f` al arrancar (paso 2, lectura con esxdos); todavia NO guarda (paso siguiente).
Sirve para validar en el Next las hipotesis H1-H3 y medir tamano y velocidad.

## Decisiones
1. **Buffer de hueco sobre `banco.h`** (`editor/src/edbuf.c`). El texto es un vector lineal de
   `ED_PAGINAS x 8 KB` con un hueco en el cursor. Teclear = un acceso a pagina; mover el cursor
   lejos mueve el hueco por bloques de 64 B (el coste por acceso es fijo, 0007 punto 5).
   Hoy `ED_PAG_BASE = 0` y usa las 4 paginas de `banco.h` (32 KB); el reparto con el compilador esta
   pendiente (COORDINACION 2.3).
2. **Lineas separadas por `\n`**, sin tabla de lineas. Se calcula inicio/fin de linea escaneando por
   bloques. El numero de linea del cursor se lleva incrementalmente y las pruebas lo contrastan
   con un recuento completo.
3. **Una sola variable global `ed`** (decision 0006) y un buffer `tmp` de 64 B compartido en edbuf.c.
4. **Limite de 72 columnas al teclear** (formato fijo F77). Un fichero cargado con lineas mas largas
   se muestra entero (desplazamiento horizontal); no se recorta nada.
5. **Pantalla 64 x 24**: fila 0 estado, 1-22 texto, 23 mensajes. La ULTIMA celda de la fila 23 no se
   escribe nunca (en una consola real provocaria scroll). Repintado minimo: al teclear un caracter
   solo se repinta su fila; al partir/unir lineas o desplazar, todas.
6. **Teclado por sondeo** (`in_Inkey` en bucle, repeticion propia con `FRAMES`): sin rutina de
   interrupcion propia, asi que no choca con el contrato de 0007 punto 4 ni con el `DI/EI` de
   `banco_next.c`. (`getchar` se descarto: ver H3.)
7. **Codigo de la maquina aparte**: `editor/plataforma/next/ed_next.c`, NO `platform/next/`, porque
   `build.ps1 spike-next` compila con z88dk todo `platform/next/*.c` y rompería los spikes.
   Script propio `tools/build_editor.ps1` (build.ps1 es del compilador).

## Resultados del test en ZEsarUX (2026-10-30)
Comprobado por Dani: teclado, repeticion, limite de 72 columnas, Esc (CAPS+SPACE) y carga de `prueba.f`
(`Loaded prueba.f: 498 bytes`, igual que el fichero) funcionan en ZEsarUX con carga rapida del `.nex`
(esxdos disponible ahi). Observacion: con la fuente de 4 px de 64 columnas, la N se ve como "П" y la M se
parece a la N, y `*` se ve como `x`: es la fuente, no el editor. Alternativa documentada en
docs/referencia (seccion 8): modo tilemap 80x32 con fuente 8x8; decision pendiente. Spike: `editor/plataforma/next/spike_tilemap.c` (tarea "Editor Next: spike tilemap"), ver seccion "Aspecto y modo de pantalla".

## Tabuladores (decision de Dani, 2026-10-30)
Tabuladores simples: paradas cada 3 columnas (0, 3, 6, 9...). Dos Tab en una linea en blanco dejan el
codigo en la columna 7 de F77 y los siguientes sangran de 3 en 3. Se aplica al teclear y al CARGAR un
fichero (se guardan siempre blancos, nunca `\t`). Un solo Tab da 3 blancos: la linea sigue siendo
inicial (columnas 1-5 libres de etiqueta). Se probo antes con paradas 5, 6, 9... (un Tab = columna de
continuacion) y se descarto por complicado.
Ejemplo: `\t\tPRINT` -> 6 blancos; `\t\t\tPRINT` -> 9.
La tecla Tab del Next NO produce ningun codigo en ZEsarUX (Dani, 2026-10-30; no se sabe si en hardware
real): `ED_K_TAB` queda sin definir y el tabulador se teclea con Esc/EDIT y luego `I`. Se aplaza a una
prueba en un Next real.

## Hipotesis a verificar en el Next (primer test)
| Id | Hipotesis | Como se ve |
|---|---|---|
| H1 | `gotoxy()` de `<conio.h>` enlaza y posiciona en 64 columnas | el texto de ejemplo sale en 7 lineas, la barra de estado arriba |
| H2 | pantalla ULA en `$4000`, texto de 64 col con el caracter izquierdo en el nibble alto: el cursor (subrayado) aparece bajo el caracter correcto | subrayado bajo el caracter, no bajo su vecino |
| H3 | **FALSA para `getchar()`** (primer test, 2026-10-30): cursor-abajo insertaba una linea y ENTER no la partia. Desensamblado de `fgetc_cons_inkey`: espera `CLIB_FGETC_CONS_DELAY` ms al entrar en cada llamada e INTERCAMBIA 13 y 10. Ahora: `in_Inkey()` (codigos crudos de la ROM del 48K: abajo 10, ENTER 13) | la barra de estado muestra `Key XX` (hex) de la ultima tecla |
| H4 | velocidad del repintado completo aceptable (22 filas x 64 caracteres por `putchar`) | sensacion al hacer ENTER o al desplazar; medir si molesta |
| H5 | repeticion propia (`ED_REP_INICIO=15`, `ED_REP_PERIODO=2` fotogramas) razonable | mantener pulsada una letra |
| H6 | `FRAMES` (`$5C78`) avanza tras `EI` (el spike S3 del compilador lo midio en CSpect) | la repeticion funciona; si no, solo se pierde la repeticion |
| H7 | Esc llega como CAPS SHIFT + SPACE y `in_Inkey` lo da como 0x20 (= espacio). Primer test: `Key 20` y no abria el modo comando. Fix: leer CAPS SHIFT por el puerto `$FEFE` y traducir a 27 | Esc abre el modo comando y NO inserta un espacio; Space solo sigue insertando un espacio |
| H8 | `esxdos_f_open/read/close` leen `prueba.f` del directorio actual al arrancar (el script lo copia a `build/next/` junto a `editor.nex`) | la ultima fila dice `Loaded prueba.f: NNN bytes` y el texto es el del fichero; si no, `Cannot open ... (err N)` y sale el texto de ejemplo. Si la maquina se cuelga con `Loading prueba.f ...` en pantalla: esxdos no existe en ese emulador, usar la tarea "SIN leer fichero" |
| H9 | `esxdos_f_read` devuelve <= 0 al final; un error de lectura real no se distingue de un fin de fichero | un fichero cortado sin aviso |

Si algun codigo de tecla es otro: se corrige en `editor/src/ed_teclas.h` (o `-DED_K_xxx=n`).
Plan B sin teclas de cursor: EDIT/Esc y luego `5 6 7 8` (izq, abajo, arriba, der).

## Pruebas (PC, `Editor PC: tests`)
`editor/tests/test_ed01.c`, 277 pruebas: buffer contra modelo ingenuo (30.000 operaciones aleatorias,
cruzando paginas y bloques, lleno exacto), escenarios del editor, y teclas aleatorias contra un
pintor ingenuo (invariantes + pantalla completa + cursor despues de cada tecla).
Revisadas en Linux con sanitizadores (ASan/UBSan) y con 14 mutaciones del codigo: 13 las detectan las
pruebas; 1 es equivalente (otro margen del desplazamiento horizontal, no un fallo). Fuzz ampliado
(40 semillas mas, sin sanitizadores): sin fallos.

## Pendiente
- Medir el `.nex` (tarea `Editor Next: compilar`): tamano, margen, composicion del `.map`.
- Guardar con `esxdos_f_write` y `ESXDOS_MODE_CREAT_TRUNC | ESXDOS_MODE_W` (existen en las cabeceras; sin probar en el Next). Menus de cargar/guardar con nombre de fichero.
- Tecla Tab: sin codigo en ZEsarUX; repetir en un Next real (`ED_K_TAB`).
- Avisar al compilador (COORDINACION): los ficheros del editor nunca llevan `\t`; el expandido de tabuladores del lexer (0004) solo afecta a ficheros ajenos.
- Mover el codigo del editor a banco(s) con trampolines y reducir el residente (hueco actual del compilador: 1.912 B).
- `mf_api.h` y compilar desde el editor (COORDINACION 3).

## Aspecto y modo de pantalla (2026-10-30, spike pendiente de ejecutar)
Referencia estetica elegida por Dani: el editor de Pascal65 (fondo azul marino, resaltado pastel,
barra de estado en gris claro invertido, linea de ayuda abajo). Con la ULA (64 columnas, fuente de 4 px)
el color es por celda de 8x8 = 2 caracteres: solo se puede colorear lineas enteras. Para color por
caracter hace falta el tilemap 80x32 (modo 66). `spike_tilemap.c` mide, en este orden, lo que NO esta
confirmado: registros $6B/$6C/$6E/$6F, direccion del mapa y bytes por celda, efecto del byte de atributo
(rejilla de los 256 valores), escritura de la paleta del tilemap ($43=$30, $40/$41) y dos hipotesis de
formato del atributo (A: estilo<<1, modo texto 1 bit; B: estilo<<4). Teclas 1 diag, 2 solo textcolor
(ya confirmado), 3 paleta hip. A, 4 paleta hip. B. Paleta propuesta (RRRGGGBB): fondo $25; texto $DA;
comentario $6E; etiqueta $FC; continuacion $E8; clave $F2; cadena $7D; numero $F5; fuera de col. 72 $4A;
estado $DB/$25; ayuda $4F/$FF; cursor $FC/$25; error $A0/$FF. La clasificacion de lineas F77 en estilos
se comprobo en PC (no hay z88dk en la nube); el resto (compilar, registros, paleta) NO se ha ejecutado.

### Resultado del spike del tilemap (ZEsarUX, 2026-10-30, capturas de Dani)
MEDIDO: `$6B=C1` (tilemap ON, 80 columnas, CON byte de atributo en el mapa, paleta 1, modo normal, bit 0 = tilemap sobre la ULA), `$6C=80`, `$6E=2C` (mapa en `$6C00`, 2 bytes por celda, `mapa[0]` = 'S' correcto), `$6F=0C` (definiciones de tile en `$4C00` si el registro es el MSB en el banco 5: SIN VERIFICAR), `$43=30`. Atributo que escribe la consola para texto blanco: `$70`.
MEDIDO: la hipotesis B es la buena: atributo = estilo<<4 (nibble alto = desplazamiento de paleta, 16 estilos), fondo = paleta[16*estilo+0], tinta = paleta[16*estilo+1]. La hipotesis A (estilo<<1) sale con glifos espejados/girados: los bits bajos del atributo son espejo X/Y y rotacion; hay que dejarlos a 0. La escritura de la paleta del tilemap (`$43=$30`, `$40`/`$41`) funciona. Decision de Dani: la maqueta B ("la 4") es el aspecto del editor. Pendiente: la tecla F2/F3... no existe en el teclado del Next (la maqueta es solo estetica); mapa de memoria: el mapa ocupa `$6C00-$7FFF` (80*32*2 = 5.120 B), comprobar que no choca con nada del compilador/banco.h.

## Port a 80x32 con color (2026-10-30)
Hecho en PC, SIN compilar para el Next ni probar alli: la nube no tiene z88dk.
- `ed_plat.h`: pantalla 80x32. Fila 0 barra de estado; filas 1-29 texto (29 lineas); fila 30 ayuda; fila 31 mensajes. `plat_escribir(fila, col, txt, est, n)` recibe un ESTILO por caracter (`ED_E_*`, 12 estilos). Ya no existe la regla "no escribir la ultima celda" (se escribe en memoria, no por consola).
- `ed_next.c`: modo 66 + fuente de z88dk, luego escritura DIRECTA en el mapa del tilemap (`$6C00`, caracter + atributo `estilo<<4`), paleta propia en `NextREG $43/$40/$41`, cursor de bloque = cambiar el estilo de la celda (restaura el atributo anterior solo si nadie la repinto). Teclado y ficheros, igual que antes.
- `edit.c`: clasificador de lineas F77 (`clasificar`): comentario (C, c, `*` en columna 1), etiqueta (cols 1-5), continuacion (col 6), clave, cadena, numero, y gris tenue desde la columna 73. Cada linea se clasifica sola. Con desplazamiento horizontal (linea de >80 columnas, solo llegan de ficheros) el trozo visible se clasifica de forma aproximada en el borde izquierdo.
- Barra de estado: ` MYFORTRAN  <fichero>` a la izquierda y, desde la columna 40, `L:<linea> C:<col>  Free <n>  Key <hex>  [CMD|*]`. Barra de ayuda con los comandos de Esc (de verdad existentes; Save/Load/Run llegaran con las funciones). Error en rojo (toda la fila de mensajes).
- Tests: 344 (PC, ASan/UBSan). Nuevos: estado y barras con estilos, desplazamiento horizontal con linea larga, resaltado con casos escritos a mano, y el fuzz compara tambien los ESTILOS contra un clasificador de referencia reescrito aparte. 10 mutaciones del resaltado y las barras, todas detectadas.
- Tabulador: sin cambios (3 columnas).
Pendiente de medir en el Next: que compile (`font_8x8_bbc_system`, `arch/zxn.h` + `arch/zxn/esxdos.h` juntos), velocidad de repintado a 3,5 MHz (una fila = 160 bytes escritos; un ENTER repinta 29 filas), tamano del `.nex` y margen.

### Esc intermitente (Dani, ZEsarUX, 2026-10-30) - H7b
Sintoma: un toque de Esc daba "Unknown command" (Key 20); manteniendolo alternaba CMD y `*`; a la 3.a o 4.a entraba. Causa probable (INFERENCIA, no medida): CAPS y SPACE no cambian a la vez; al soltar, CAPS sube antes y `in_Inkey` devuelve 0x20 = blanco, que en modo comando es "Unknown command"; y mantenida, Esc se repetia (abrir/cerrar). Cambios: (1) `ed_next.c`: tecla nueva 0x20 espera 2 fotogramas y mira CAPS; 0x20 mientras Esc sigue pulsado se ignora; Esc no se repite; (2) `edit.c`: Esc/EDIT dentro del modo comando cancela sin error. Tests 346. SIN VERIFICAR en el Next/ZEsarUX.

## ed_linea residente (2026-10-30, en PC)
`editor/src/ed_linea.{c,h}`: implementa `int ed_linea(uint16_t n, char *dst)` de `compiler/include/mf_api.h` sobre el buffer (`eb_*`). Linea n (la 1.a es la 1), sin `\n`, <= 72 caracteres + `\0`, 1 = existe, 0 = n pasa del final; una linea vacia existe; detras del ultimo `\n` no hay linea. Cache (numero y posicion de la ultima linea pedida) que se descarta si se pide una anterior o si cambia el texto: `BufTexto.ver` (campo nuevo de `edbuf`) sube en cada insertar/borrar/agregar/iniciar. Residente por diseno: solo usa `eb_*`/`banco_*`.
Tests: 369 (nuevos: casos del contrato, cambio del texto entre compilaciones, linea >72, fuzz de ediciones y consultas contra una referencia aparte). Mutaciones: 8 de 9 detectadas; la que sobrevive (quitar `ver++` en `eb_agregar`) es equivalente: anadir al final nunca mueve el inicio de una linea ya conocida. SIN compilar con z88dk.


## Nuevo, cargar y guardar (2026-10-30, en PC)
Teclas: Esc y luego `N` (nuevo), `L` (cargar), `S` (guardar), `A` (guardar como).
- `L` y `A` piden el nombre en la ultima fila (maximo 31 caracteres, `ED_NOMBRE_MAX`); Enter acepta, Delete borra, Esc cancela; el cursor se pone al final de lo tecleado. `A` empieza con el nombre actual escrito.
- `S` guarda directamente si el fichero ya tiene nombre; si no, pide uno.
- `N` y `L` preguntan `Continue? (Y/N)` si hay cambios sin guardar (`ed.modificado`); `A`/`S` con un nombre DISTINTO del actual que ya existe preguntan `Overwrite? (Y/N)`. Cualquier tecla que no sea Y cancela.
- `ed_guardar(nombre)` devuelve 0 (guardado), 1 (no se pudo crear; nada cambia) o 2 (escritura corta: el fichero queda incompleto, sigue marcado como modificado y el nombre no cambia). Escribe el texto TAL CUAL esta en el buffer, incluidas lineas de mas de 72 caracteres cargadas de un fichero; los saltos de linea son `\n`.
- Plataforma nueva: `plat_fich_abrir_escritura` / `plat_fich_escribir` (`ed_plat.h`). En el Next: `esxdos_f_open(CREAT_TRUNC|W)` y `esxdos_f_write` (hipotesis H10, SIN VERIFICAR en el Next; compilar con z88dk tampoco se ha hecho aun).
- Pruebas (PC, ASan/UBSan): 614 pruebas, 0 fallos. Incluyen guardar y volver a cargar tras 30 sesiones aleatorias de teclado, un fichero que casi llena el buffer con una linea de 300 caracteres, errores al crear y escritura corta, todos los caminos de los menus, y un fuzz de menus con fallos de fichero al azar. 16 mutaciones: 14 detectadas; 2 equivalentes.
- Coste residente (DERIVADO de los fuentes, SIN MEDIR): +74 B de bss (dos buffers de nombre de 32 B, `msgbuf` de 48 a 56 B, 2 variables). El codigo nuevo es de `edit.c`/`ed_next.c` (banco 41 cuando se parta).
- Falta: hacer una prueba real en el Next (crear, sobrescribir, disco lleno, tarjeta protegida).

## Ir a linea y ayuda (2026-10-30, en PC)
- Esc y luego `G`: pide un numero en la ultima fila (solo cifras, hasta 5) y lleva el cursor al principio de esa linea (la 1.a es la 1). Pasada del final va a la ultima y lo dice (`Past the end: moved to line N`); un numero mayor que 65535 cuenta como "mas alla", no da la vuelta.
- Decision de Dani: se quita el comando `I` (tabulador) del modo comando y de las barras; los comandos `5-8` tambien salen de la barra (siguen funcionando). La tecla Tab propiamente dicha se mantiene en el codigo (`ED_K_TAB`, hoy 0 = sin definir) por si el Next fisico la entrega: pendiente de un programa que muestre que codigo da cada pulsacion del teclado del Next (flechas incluidas) para ajustar `ed_teclas.h`.
- El sangrado automatico a la columna 7 se probo en PC y se RETIRO (no aporta en una primera version). No esta en el codigo.
- Pruebas: 647, 0 fallos (ASan/UBSan). Las pruebas se compilan con `-DED_K_TAB=250` para ejercitar la ruta de la tecla Tab.

## Borrar linea y guardado tras carga incompleta (2026-10-30, en PC)
- Esc y luego `K`: borra la linea del cursor entera. El cursor queda al principio de la linea que ocupa su lugar; si era la ultima, al principio de la anterior (y la linea desaparece); con una sola linea solo se vacia. Barras: `K del`.
- **Riesgo cubierto:** si un fichero no cabe en el buffer (`File too big`) o falla la lectura a mitad, el editor lo carga incompleto. Guardarlo encima del MISMO fichero perderia el resto sin avisar. Ahora `S`/`A` sobre ese nombre piden `File was only partly loaded: saving LOSES the rest. Save anyway? (Y/N)`. Guardar con OTRO nombre no pregunta (el original no se toca) y, una vez guardado, la marca se quita. `N`, otra carga completa y guardar la limpian.
- Pruebas (PC, ASan/UBSan, con `BANCO_PAGINAS = 10`): 1.757 comprobaciones, 0 fallos; incluyen una referencia ingenua para Esc+K (20 semillas, 60 borrados cada una) y `K` en el fuzz de teclas. 20 mutaciones del codigo nuevo, todas detectadas.

## Compilar desde el editor (2026-10-30, en PC contra un compilador SIMULADO)
- Esc y luego `C`: muestra `Compiling ...` y llama a `mf_compilar()` (contrato `compiler/include/mf_api.h`). Sin errores: `Compiled OK: no errors`. Con errores: salta el cursor al primero (linea y columna del diagnostico; una columna pasada del final de la linea, o la saturada 255, cae al final) y escribe en rojo `Error 1/3 at 3:7: <mensaje de mf_mensaje>`. Si el compilador cuenta mas de `MF_DIAG_MAX` (16) errores, el total sale en el mensaje (`1/21`) y se recorren los 16 guardados.
- `X` / `Z`: error siguiente / anterior, dando la vuelta. Mover el cursor no invalida la lista; cambiar el texto (`eb.ver`) si: `Text changed: compile again (Esc C)`. Sin compilacion previa: `No errors to show (Esc C compiles)`.
- Se activa con `-DED_CON_COMPILADOR` (+ `-Icompiler/include`). Sin esa macro, el `.nex` suelto del editor responde `No compiler in this build` (el editor suelto no enlaza el compilador). Las letras `C`, `X`, `Z` son una eleccion mia, faciles de cambiar en `comando()`.
- Coste residente (DERIVADO, SIN MEDIR): `msgbuf` de 56 a 80 B (+24) y 4 B de estado (`comp_*`); `mostrar_error` usa 64 B de pila para el mensaje (`MF_MENSAJE_MAX`). El codigo y los textos nuevos serian del banco 41.
- La interfaz usa solo: `mf_compilar`, `mf_mensaje`, `mf.n` y `mf.diag[]`, y `ed_linea` (via el compilador). Comprobado en PC que el compilador falso lee TODAS las lineas por `ed_linea` (2 para un texto de 2 lineas con `\n` final) y que ve en pantalla `Compiling ...` al empezar.
- Pruebas: 1.838 comprobaciones, 0 fallos (ASan/UBSan); 19 mutaciones del codigo nuevo, todas detectadas. `C`, `X` y `Z` entran tambien en el fuzz de teclas (con diagnosticos inventados a partir del texto).
- Barras: normal ` Esc, then: H/E line T/B text U/D page G goto K del C comp X/Z err N/L/S/A file`; modo comando ` CMD: H/E T/B U/D G goto K del C comp X/Z err  N new  L load  S save  A save as`.
- Pendiente: enlazarlo con el compilador REAL (editor en el banco 41 y `t_ed_*`), y comprobar que al volver de `mf_compilar` el banco del editor queda mapeado.

## Comprobar y construir (2026-11-01, en PC contra un compilador SIMULADO)
Esc+C **comprueba** (`mf_salida(0)`; antes "compilar") y Esc+O **construye** (registra `ed_salida`, llama a `mf_compilar`, desregistra): sin errores el compilador genera y escribe el `.nex` por `abrir/escribir/cerrar`. Resultados: `Built x.nex`, `Build failed: <mf_mensaje(gen_cod)>` (error), `Checked OK, nothing built`, o el primer error.
Nombre del `.nex`: el del fuente cambiando la extension (`a.f`->`a.nex`, `prog`->`prog.nex`, `d.d/x`->`d.d/x.nex`); sin nombre de fuente pide `Build as:` (aviso Y/N si existe) y lo recuerda hasta Nuevo/Cargar; nunca construye encima del propio fuente. `ed_salida.c` y `ed_fich.c` (esxdos) son RESIDENTES: los llama el compilador con el banco 41 desmapeado. En modo comando la fila de mensajes hace de segunda fila de ayuda. `ed_bucle.c` aloja `ed_ejecutar()` (punto de entrada del banco 41, ED-002); `ed_main.c` es solo el `main` plano.
Verificado en PC (1.905 pruebas, mutacion: ver COORDINACION 2026-11-01). SIN MEDIR en el Next: residente, pila, esxdos con el 41 mapeado.
