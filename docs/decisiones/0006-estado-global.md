# 0006 - Estado compartido de cada fase en variables globales

Fecha: 2026-10-30. Decidido con el autor.

## Decision
El estado de cada fase del compilador (parser, lexer; despues sentencia y las demas) es una
**variable global unica**, no una estructura que se pasa por puntero a cada funcion.
`extern Parser parser;` en `expr.h`, `extern Lexer lexer;` en `lexer.h`.

## Por que (medido en el Next, sccz80 sin optimizar)
- Parser global: `expr.c` 2.677 -> 1.841 bytes de codigo (-31 %); pila por nivel 41 -> 35 bytes.
- Lexer global: `lexer.c` -15 %, `clasif.c` -10 %; imagen 22.900 -> 21.956.
- Sentencia global: `source.c` 1.880 -> 1.671 (-11 %); imagen 21.956 -> 21.699.
- Tabla de cadenas: ver arriba.
- Mecanismo (hipotesis, no verificada en el ensamblador): una global se direcciona con una
  instruccion de 3 bytes; `p->campo` con `p` en la pila exige cargar el puntero, sumar el
  desplazamiento y empujar `p` en cada llamada.
- Encaja con F77: el compilador procesa UNA sentencia a la vez; no hace falta mas de un lexer o parser.

## Consecuencias
- No se pueden tener dos lexers/parsers vivos a la vez (por ejemplo, para analizar con lookahead
  en una segunda instancia). Si algun dia hace falta, se guarda y restaura el estado.
- Los tests comparten el estado: cada prueba debe llamar a `lex_iniciar`/`parser_iniciar` primero.
- Esto NO es lo mismo que `static` local: las funciones recursivas (`expresion`...) no pueden usar
  locales `static`.
- `TablaCadenas` global: imagen 21.699 -> 21.630 (-69). Rendimientos decrecientes: se aplica a las demas
  fases desde el principio, sin experimento previo.
