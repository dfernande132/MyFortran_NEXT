# 0005 - Expresiones: precedence climbing y AST en pool fijo

Fecha: 2026-10-30. Decidido con el autor.

## Decisiones

1. **Descenso recursivo con precedence climbing** (opción B de las tres estudiadas), con una
   tabla de operadores (`OPERADORES[]` en `expr.c`). Anadir un operador es anadir una fila.
2. **AST en un pool fijo de 128 nodos de 8 bytes (1 KB), con indices de 16 bits**; el 0 es
   "sin nodo". Se reinicia por sentencia (`ast_iniciar`). Sin `malloc`.
3. **Si el AST diera problemas de espacio mas adelante**, se emitiria directamente codigo de
   3 direcciones desde el parser y se analizaria la mejora entonces (decision del autor). El
   campo `tdato` y la separacion parser/semantico se pierden en ese caso.
4. **Recursion acotada**: contador de profundidad (`EXPR_PROF_MAX` = 48). Superarlo es un
   error "expression too complex", no un desbordamiento de pila. El valor es provisional: se
   ajusta con la medida de pila en el Next.
5. **Reglas del estandar estrictas** (cap. 6): signo unario solo al principio de una expresion o
   argumento (`A*-B` y `A**-B` son error, "use parentheses"); `.NOT.` solo donde cabe un
   operando de `.AND.`; relacionales no asociativos. Se rechazan aunque muchos compiladores
   (g77) los admiten como extension: es un punto facil de relajar mas adelante.
6. **`A(I)` es siempre `NODO_REF`.** Elemento de array, funcion o funcion-sentencia lo decide el
   semantico con la tabla de simbolos. `F()` (sin argumentos) es valido; `C(2:5)` y `C(I)(2:3)`
   (subcadenas) se parsean aqui (`NODO_RANGO`, `NODO_SUBCAD`).
7. **Los parentesis se conservan** como `NODO_PAREN`: `F((X))` pasa un valor, `F(X)` una variable.

## Alternativas estudiadas

- **A. Una funcion por nivel de precedencia**: lo mas legible, pero ~10 marcos de pila por cada
  parentesis anidado y ~10 funciones casi iguales.
- **C. Patio de maniobras (pilas explicitas)**: pila del sistema constante, pero las llamadas
  anidadas, el signo unario y los errores se complican, y no ahorra codigo respecto a B.
