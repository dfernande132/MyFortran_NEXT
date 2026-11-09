# Ejemplos

Programas para ensenar y para probar a mano en el Next. Nombres 8.3 sin guion bajo (el emulador no los lanza si no).

| Programa | Que ensena |
|---|---|
| `fibo.f` | el programa mas sencillo: vector, DO, PRINT |
| `leer.f` | READ: enteros, reales, logicos y cadenas (`leer.entrada.txt`: lo que hay que teclear) |
| `arrmat.f` | vectores y matrices, tambien como argumentos de subprogramas |
| `primos.f` | READ + criba de Eratostenes + factorizacion (`primos.entrada.txt`) |
| `reales.f` | aritmetica REAL de 32 bits |
| `inicio.f`, `prueba.f`, `largo.f`, `errores.f` | los usa el editor (pantalla de inicio, pruebas, Esc X / Esc Z) |

`examples/next/` tiene las seis pruebas para comprobar el compilador en el Next (con `LEEME.md`).
Cada `X.esperado.txt` es lo que debe escribir el programa; `compiler/tests/test_programas.c` lo comprueba en PC.
Los programas que se usaron como prueba en cada etapa del compilador estan en `compiler/tests/ejemplos/`.
