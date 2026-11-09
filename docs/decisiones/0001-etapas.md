# 0001 - Desarrollo en dos etapas, PC primero

Fecha: 2026-10-29. Decidido con el autor.

## Decisiones

1. **Etapa A: el nucleo se escribe en C portable y se desarrolla/prueba en PC con gcc.**
   Salida inicial: ensamblador Z80 en texto, ensamblado con z88dk y ejecutado en CSpect.
2. **Etapa B: llevar compilador y editor al Next.** Se decide con medidas, no antes.
   **Aclaracion del autor (2026-10-30): el objetivo final es un compilador NATIVO dentro del
   Next, estilo Turbo C, con IDE propio; el compilador cruzado en PC es solo el medio de
   desarrollo, no un producto alternativo.** Las medidas deciden COMO cabe (reduccion, datos
   paginados, overlays), no SI se lleva al Next.
3. **Un repositorio, dos componentes** (`compiler/`, `editor/`) unidos por un contrato minimo
   (`compiler/include/mf_api.h`) y una capa `platform/` compartida. Permite dos conversaciones
   en paralelo sin pisarse.
4. **Espacio de trabajo de VS Code:** la carpeta `MyFortran`, con la documentacion previa en `docs/referencia/`.

## Por que

- Un compilador nativo con fases separadas y representacion intermedia (lo que se quiere
  ensenar) puede no caber en los 32 KB del Next, compartidos con la pila. Turbo Pascal 3.0
  era, segun se recuerda y sin verificar, de una sola pasada y sin codigo intermedio.
- Iterar en PC es mucho mas rapido y permite tests de terminal.

## Riesgos abiertos

- No se sabe si el pipeline completo cabe en el Next. Se mide desde el capitulo 1 con `zcc +zxn` y el `.map`.
- Desde un `.nex` no esta comprobado lanzar otro programa (afecta a "compilar y ejecutar" en el editor).
- Representacion de `REAL`/`DOUBLE PRECISION` (`float` de z88dk = 6 bytes): pendiente.
- Editor: se evalua una base existente (p. ej. Kilo, por verificar) o uno propio; su modelo de
  memoria con `realloc` no encaja con "sin malloc" en el Next.
