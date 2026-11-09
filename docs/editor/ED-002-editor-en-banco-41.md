# ED-002 - El editor en el banco 41: puntos de entrada `t_ed_*` (PROPUESTA del EDITOR)

Estado: **PROPUESTA** para que la revise el compilador (dueno de `mapear()` y de los trampolines) y la
apruebe Dani. Nada de esto esta programado ni medido; las cifras de tamano son DERIVADAS de `.map`
anteriores del editor plano. Contexto: `COORDINACION.md` secciones 2-3, decision 0007 (bancos y
trampolines) y `ED-001`.

## 0. Respuesta a la pregunta del buzon: ¿los trampolines restauran el banco?
**Si** (DERIVADO de leer `tools/spike/c4_trampolines.c` y `c5_trampolines.c`, no medido en esta
combinacion): cada `t_*` hace `ant = banco_actual; mapear(destino); llamada; mapear(ant);`. Condicion
que impone al editor: **el trampolin de entrada del editor tiene que usar ESE MISMO `mapear()` y
`banco_actual`** (hoy `static` en `c4_trampolines.c`; por eso `c5_trampolines.c` hace `#include` de
`c4_trampolines.c`). Si el editor se mapeara a mano, `banco_actual` seria `NINGUNO` y al volver de
`mf_compilar` el trampolin restauraria "lo que habia al arrancar" (`orig6/orig7`): el codigo del
editor desapareceria. Con el mismo `mapear()`, `mf_compilar` -> `t_*` -> vuelta deja mapeado el 41.
`mf_mensaje` igual (`t_msg_*` en `i1_trampolines.c` restauran `ant`).

## 1. Que va a cada sitio
| Unidad | Destino | Motivo |
|---|---|---|
| `edbuf.c`, `ed_linea.c` | **residente** | las llama el compilador desde sus bancos (`ed_linea`) y el editor (`eb_*`); solo hay un banco visible |
| `edit.c`, `ed_next.c`, bucle del editor | **banco 41** (codigo y rodata) | no los llama nadie de fuera; el codigo de un banco SI puede llamar a funciones residentes (`eb_*`, `banco_*`, `mf_compilar`, `mf_mensaje`, esxdos, `in_Inkey`) |
| bss/data de `edit.c`, `ed_next.c` (`ed`, `lin`, `est`, `msgbuf`, `fichero`, `entrada`...) | **residente** (MEDIDO por el compilador en `c5_medir.map`: el bss de un fichero de banco queda residente) | ~350 B (DERIVADO de los fuentes) |
| `main()` y el trampolin | residente | minimos |

Tamano (DERIVADO, SIN MEDIR): banco 41 ~10 KB de 16 KB (7.152 B medidos en el editor del 80x32 de las 17:00
mas ~1,9 KB de Nuevo/Cargar/Guardar medidos como crecimiento de la imagen, mas ~1 KB estimado de Ir a linea /
Borrar linea / Compilar). Editor residente ~2,3 KB (edbuf 1.722 + ed_linea 267 + bss ~350 + trampolin/main ~50).

## 2. Propuesta de puntos de entrada
### Opcion A (RECOMENDADA): UNA sola entrada que devuelve una accion
```c
/* residente (trampolin, lo escribe el compilador junto a los suyos, porque necesita mapear()) */
uint8_t t_ed_ejecutar(void);        /* mapear(BANCO_ED = 41); r = ed_ejecutar(); mapear(ant); return r; */

/* en el banco 41 (lo escribe el editor, editor/src/ed_bucle.c) */
uint8_t ed_ejecutar(void);          /* si es la primera vez: plat_iniciar + ed_iniciar + carga de prueba.f;
                                       despues: for(;;){ ed_tecla(plat_tecla()); ed_refrescar(); }
                                       Vuelve solo cuando el editor necesita algo del residente: */
#define ED_ACCION_NADA       0      /* (no se usa: de aqui no se vuelve sin motivo) */
#define ED_ACCION_EJECUTAR   1      /* el usuario pidio ejecutar (hoy no existe; para el futuro) */
#define ED_ACCION_SALIR      2
```
```c
/* residente */
int main(void)
{
    banco_iniciar();
    for (;;) {
        switch (t_ed_ejecutar()) {
        case ED_ACCION_EJECUTAR: /* futuro: ejecutar el programa compilado y volver */ break;
        default: return 0;
        }
    }
}
```
Por que una sola entrada y no una por tecla:
- **Un mapeo por sesion**, no dos por pulsacion (`t_ed_tecla` + `t_ed_refrescar`). El trampolin de c5 cuesta ~55 us por mapeo (estimacion del compilador, no medida aislada): irrelevante a una tecla por frame, pero asi no hay que medirlo.
- **No cruzan punteros ni estructuras** entre el residente y el banco: `plat_tecla`, la pantalla y `ed` viven del lado del banco.
- Menos codigo residente: un trampolin (~40 B, dato del compilador: 20 `t_*` = 796 B) en vez de 5 (~200 B).
- Si hace falta recuperar el control (ejecutar un programa, salir), `ed_ejecutar` **vuelve** con un codigo y el estado (`ed`, el texto) queda residente, asi que se puede volver a entrar sin perder nada.

### Opcion B (alternativa, solo si el residente necesita el control entre teclas)
`t_ed_iniciar()`, `t_ed_tecla(int c)`, `t_ed_refrescar()`, `t_ed_cargar(const char *nombre)` (el `nombre` debe ser residente
o literal del residente: **no un puntero al rodata del banco**), `t_ed_cursor()` si el residente pinta algo. Cada una:
`ant = banco_actual; mapear(41); r = ed_x(...); mapear(ant); return r;`. Mas pasos por el trampolin y mas codigo
residente; no veo hoy un motivo.

## 3. Reglas que cumple el editor al estar en un banco (de decision 0007 / S4)
1. Ningun `t_ed_*` devuelve un puntero a rodata del banco. Dentro del banco SI se usa `ed.msg = msg_xxx` (apunta al
   rodata del 41), porque solo lo lee el codigo del propio banco con el 41 mapeado (`ed_refrescar`). **Cuidado**: si
   alguien del residente leyera `ed.msg` con otro banco mapeado, veria basura. Hoy nadie lo hace.
2. El codigo del banco 41 llama a `mf_compilar`/`mf_mensaje` (residentes). Dentro, los `t_*` del compilador mapean 45/46/47
   y **restauran el 41** (seccion 0). Los textos de los mensajes los copia `mf_mensaje` en un buffer del editor
   (la pila: residente), nunca se devuelve un puntero al rodata de un banco del compilador.
3. `esxdos_f_*` y `in_Inkey` son codigo de la libreria (residente). El editor les pasa SOLO buffers residentes (`lin`, `entrada`,
   `fichero`, `msgbuf`) o literales; con el 41 mapeado en MMU6/7 las llamadas a esxdos no lo tocan **(SIN VERIFICAR en el Next)**.
4. La pila: sigue siendo la residente (`REGISTER_SP = 0xBFF0`, decision 0007). El editor anade a la pila del compilador
   los marcos de `ed_tecla` -> `comando` -> `compilar` -> `mf_compilar` (+10 B del trampolin de entrada una sola vez).
   **SIN MEDIR**: la pila maxima del compilador con `ed_linea` anidado fue 961 B; el editor suma marcos propios del
   orden de 100-200 B (estimacion, hay que medirla con el driver de integracion).
5. Ni interrupciones ni ventana de datos cambian: `banco.h` (MMU2) y DI/EI como en 2.4; el cambio de codigo (MMU6/7) no toca eso.

## 4. Quien hace que (propuesta)
- **Compilador**: define `BANCO_ED = 41`, escribe `t_ed_ejecutar` junto a sus trampolines (usa su `mapear()` / `banco_actual`)
  y un wrapper de banco `#pragma bank 41` que incluya los `.c` del editor sin modificarlos (patron de `tools/spike/i1_banco45.c`).
- **Editor**: `editor/src/ed_bucle.c` con `ed_ejecutar()`, partir `ed_main.c` (`main` residente minimo), y mantener los `.c` aptos
  para el modo plano (el `.nex` suelto sigue existiendo y se compila sin bancos).
- **Los dos**: `.nex` de integracion nuevo (`i2_*`) y medida: residente real, pila real con el editor, tiempo de
  `Esc+C` con `examples/prueba.f`, y una prueba de esxdos (cargar y guardar) con el 41 mapeado.

## 5. Riesgos abiertos
- Esxdos con el banco mapeado (punto 3.3): SIN VERIFICAR.
- Presupuesto del residente: el compilador estima ~6 KB de pila libres sin consola (DERIVADO); con el editor integrado hay que remedirlo.
  Palancas ya identificadas: partir `edbuf` (~-0,9 KB residentes), quitar la consola/fuente 4x8 de la libreria (~-3 KB, sin medir).
- Rendimiento del repintado 80x32 a 3,5 MHz: sigue sin medir.
