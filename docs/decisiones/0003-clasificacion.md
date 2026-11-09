# 0003 - Clasificación de sentencias con reglas deterministas

Fecha: 2026-10-29. Decidido con el autor.

## Decisiones

1. **Reglas deterministas, sin retroceso.** Una pasada de solo lectura (`clasif.c`)
   decide qué clase de sentencia es y desde dónde debe leer el escáner. Orden:
   asignación (`nombre {(...)}* =`), regla del DO (coma fuera de paréntesis tras el
   `=`), palabra clave más larga de la tabla, y la forma del IF.
2. **La tabla de palabras clave crece por capítulo.** Una sentencia válida aún no
   soportada sale como `SENT_DESCONOCIDA`.
3. **F77 estricto** a nivel de sentencia: sin `ENDDO`, `DO WHILE`, `IMPLICIT NONE` ni
   `INTEGER*4`. (Sí aceptamos minúsculas y nombres de hasta 8 caracteres.)
4. **La implementación la escribe Claude y el autor la revisa** (reparto elegido).

## Alternativa descartada: lexer guiado por el parser, con retroceso

Se estudió un análisis externo que proponía que el parser dictase cómo agrupar los
caracteres, retrocediendo cuando una regla falla (por ejemplo, `IF(I,J)=5`) y consultando
la tabla de símbolos desde el léxico. Se descartó porque:

- `IF(I,J)=5` se decide sin parsear: tras saltar el paréntesis balanceado aparece un `=`,
  y eso solo puede ser una asignación.
- La única ambigüedad que necesita la tabla de símbolos es `F(X)=X*X` (asignación a un
  elemento de array o sentencia-función), pero los tokens son idénticos en ambos casos:
  lo decide el semántico, no el léxico. (Dicho de memoria del estándar; por contrastar.)
- El retroceso y el acoplamiento cuestan memoria y complejidad en el Z80.

## Correcciones que salieron de la revisión de ese análisis

- La variable de un `DO` puede llevar dígitos (`DO 10 I1 = 1,5`): alfanumérica, no solo letras.
- La coma de la regla del DO se busca ignorando las de dentro de cadenas y de paréntesis.
- Existe una coma opcional tras la etiqueta (`DO 10, I = 1,10`).
- **Fallo propio descubierto al revisarlo:** en `DO10E=1,10` el escáner leería `10E` como
  inicio de exponente y daría error. Por eso el clasificador lee la etiqueta del DO y
  entrega al escáner el resto desde el nombre de la variable.

## Limitación conocida

`REAL FUNCTION F(X)` se clasifica como `REAL` con resto `FUNCTIONF(X)`. Se resolverá en el
capítulo de subprogramas con una heurística (es función si tras `FUNCTION` hay un nombre
y un `(`).
