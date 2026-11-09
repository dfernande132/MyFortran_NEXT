# 0009 - Analisis semantico (capitulo 5)

Fecha: 2026-10-30. Decidido con el autor.

## Alcance
Tabla de simbolos + tipado de expresiones y declaraciones + emparejado de bloques (IF/DO) + etiquetas.
Sin plegado de constantes. Tipos: INTEGER (16 bit), LOGICAL, REAL, CHARACTER, arrays de 1 y 2 dimensiones.

## Decisiones
1. **Semantico sentencia a sentencia** (consecuencia de las listas en flujo, 0008-B): no existe un AST del programa.
2. **Tabla de simbolos por UNIDAD de programa**, en paginas de datos (`platform/banco.h`, una pagina de 8 KB).
   `SIM_MAX` = 512 (constante). Es el maximo que cabe en UNA ventana MMU2 con registros de 16 B; mas simbolos
   obligarian a cambiar de pagina dentro de una busqueda. Los nombres globales (subprogramas, COMMON) iran en otra
   tabla en el cap. de subprogramas.
3. **Registro de 16 B** (sin padding): nombre[8] inline (NOMBRE_MAX), tipo|clase empaquetados (1), banderas (1),
   enlace de hash (2), extra (2: longitud CHARACTER o indice de dimensiones), direccion (2).
4. **Busqueda con hash** (cabeceras en el residente, 2 B por cubeta) para no hacer una busqueda lineal de 512 entradas.
5. **Errores acumulados**: tras un error semantico se sigue con la sentencia siguiente (tope de errores por pasada).
6. **Compilador sin float**: las constantes REAL se llevan como texto de la tabla de cadenas.
7. **Zona de trabajo (0008-D)**: el semantico y el generador reutilizan `sentencia.texto` cuando acabe `stmt_elemento()`.

## Estimaciones (SIN medir)
- Nombres por unidad en un programa didactico: 50-150. Coste de pagina: 16 B x 512 = 8.192 B.

## Estado (2026-10-30)
Implementado en `sim.c` y `sem.c` con 1.259 pruebas en PC. Detalle y limitaciones en `docs/capitulos/cap05-semantico.md`.
Pendiente de verificar contra el estandar: lista de sentencias que no pueden terminar un DO.
Medido en el Next (spike c5_medir): banco 45 = 8.954 B, peor pila 913 B, hueco de pila 1.432 B con el medidor. Ver cap05 (sexta ejecucion).
Consecuencia fuera del semantico: en `expr.c` cada nivel de argumentos F(...) cuenta doble en la profundidad (5 niveles maximo con EXPR_PROF_MAX = 12; era 24 antes de medir la pila del Next, ver COORDINACION 2026-11-08).
