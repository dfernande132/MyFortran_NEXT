# Pruebas en el Next

Cada programa `nNxxxxxx.f` es copia de uno de `compiler/tests/programas/` (ya probado en PC, con TAC y Z80 iguales entre si y
iguales a la salida calculada aparte por `oraculo.py`). Aqui se comprueba lo mismo en el Next: **el .nex generado debe
escribir exactamente `nNxxxxxx.esperado.txt`.**

Orden recomendado (de menos a mas exigente con la memoria):

| Programa | Que prueba | Entrada |
|---|---|---|
| `n1orden.f` | vectores, DO, IF de bloque, LOGICAL, MOD | no |
| `n2subpro.f` | 6 subprogramas, vectores como argumentos, funcion-sentencia | no |
| `n3entra.f` | READ de enteros, reales, logicos y cadenas | si: `n3entra.entrada.txt` |
| `n4reales.f` | aritmetica REAL de 32 bits, DO con variable REAL, formato de salida | no |
| `n5limite.f` | parentesis, F(F(..)) y IF/DO anidados al maximo (pila) | no |
| `n6cadena.f` | CHARACTER, subcadenas, comparaciones | no |

## Procedimiento por programa

1. Cargar el `.f` en el editor, `Esc` `C` (comprobar): debe decir que no hay errores.
2. `Esc` `O` (construir) y ejecutar el `.nex` resultante.
3. Comparar la pantalla con `nNxxxxxx.esperado.txt`, linea a linea. (Salen mas de 24 lineas en `n4`: el texto sube solo.)
4. Con `n3` (el programa pide cada dato con un texto; si la pantalla se queda en negro sin ningun texto, es un fallo real), teclear las lineas de `n3entra.entrada.txt` en ese orden, cada una seguida de ENTER (`'` es la comilla).
5. Anotar el valor de `Esc` `M` (pila usada) despues de comprobar y construir `n5limite.f`: la 0.1 se probo con 583/645.

Si un programa no compila o se queda sin memoria, anotar el mensaje y la linea: ese caso entra como prueba nueva en PC.
