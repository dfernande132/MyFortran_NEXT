/* tests_arrays.h - Pruebas de S3b/S3c/S3d: arrays y cadenas como argumentos (se incluye en test_cap09.c, que trae
 * los ayudantes SALIDA / ERROR / U). Cada SALIDA compara el TAC, el Z80 y el texto escrito a mano. */

static void test_arrays_basicos(void)
{
    /* array entero, dimensiones constantes: el subprograma lo lee y lo escribe */
    SALIDA("      INTEGER V(3)\n"
           "      V(1) = 10\n"
           "      V(2) = 20\n"
           "      V(3) = 30\n"
           "      CALL INC(V)\n"
           "      PRINT *, V(1), V(2), V(3)\n"
           "      END\n"
           "      SUBROUTINE INC(A)\n"
           "      INTEGER A(3)\n"
           "      DO 10 I = 1, 3\n"
           "        A(I) = A(I) + 1\n"
           "   10 CONTINUE\n"
           "      END\n",
           " 11 21 31\n");
    /* el formal no ve el limite inferior del real: A(1) es el primer elemento aunque el real sea V(0:2) */
    SALIDA("      INTEGER V(0:2)\n"
           "      V(0) = 7\n"
           "      V(1) = 8\n"
           "      V(2) = 9\n"
           "      CALL P(V)\n"
           "      PRINT *, V(0), V(1), V(2)\n"
           "      END\n"
           "      SUBROUTINE P(A)\n"
           "      INTEGER A(3)\n"
           "      PRINT *, A(1), A(3)\n"
           "      A(1) = 0\n"
           "      END\n",
           " 7 9\n 0 8 9\n");
    /* un formal con limite inferior propio */
    SALIDA("      INTEGER V(3)\n"
           "      V(1) = 1\n"
           "      V(2) = 2\n"
           "      V(3) = 3\n"
           "      CALL P(V)\n"
           "      PRINT *, V(1), V(2), V(3)\n"
           "      END\n"
           "      SUBROUTINE P(A)\n"
           "      INTEGER A(-1:1)\n"
           "      A(-1) = A(-1) * 10\n"
           "      A(0) = A(0) * 10\n"
           "      A(1) = A(1) * 10\n"
           "      END\n",
           " 10 20 30\n");
    /* REAL y LOGICAL */
    SALIDA("      REAL R(2)\n"
           "      LOGICAL L(3)\n"
           "      R(1) = 1.5\n"
           "      R(2) = 2.5\n"
           "      L(1) = .TRUE.\n"
           "      L(2) = .FALSE.\n"
           "      L(3) = .TRUE.\n"
           "      CALL P(R, L)\n"
           "      PRINT *, R(1), R(2)\n"
           "      PRINT *, L(1), L(2), L(3)\n"
           "      END\n"
           "      SUBROUTINE P(X, B)\n"
           "      REAL X(2)\n"
           "      LOGICAL B(3)\n"
           "      T = X(1)\n"
           "      X(1) = X(2)\n"
           "      X(2) = T\n"
           "      DO 10 I = 1, 3\n"
           "        B(I) = .NOT. B(I)\n"
           "   10 CONTINUE\n"
           "      END\n",
           " 2.50000000 1.50000000\n F T F\n");
    /* matriz 2x3: el orden de los elementos es por columnas */
    SALIDA("      INTEGER M(2, 3)\n"
           "      DO 10 J = 1, 3\n"
           "        DO 10 I = 1, 2\n"
           "          M(I, J) = 10 * I + J\n"
           "   10 CONTINUE\n"
           "      CALL P(M)\n"
           "      PRINT *, M(1, 1), M(2, 1), M(1, 2), M(2, 2), M(1, 3), M(2, 3)\n"
           "      END\n"
           "      SUBROUTINE P(A)\n"
           "      INTEGER A(2, 3)\n"
           "      A(2, 3) = A(1, 3) + A(2, 1)\n"
           "      A(1, 1) = 0\n"
           "      END\n",
           " 0 21 12 22 13 34\n");
    /* el real es 2x3 y el formal lo ve como un vector de 6 (asociacion de almacenamiento) */
    SALIDA("      INTEGER M(2, 3)\n"
           "      DO 10 J = 1, 3\n"
           "        DO 10 I = 1, 2\n"
           "          M(I, J) = 10 * I + J\n"
           "   10 CONTINUE\n"
           "      CALL P(M)\n"
           "      END\n"
           "      SUBROUTINE P(A)\n"
           "      INTEGER A(6)\n"
           "      PRINT *, A(1), A(2), A(3), A(4), A(5), A(6)\n"
           "      END\n",
           " 11 21 12 22 13 23\n");
    /* y al reves: vector real, formal matriz */
    SALIDA("      INTEGER V(6)\n"
           "      DO 10 I = 1, 6\n"
           "        V(I) = I\n"
           "   10 CONTINUE\n"
           "      CALL P(V)\n"
           "      END\n"
           "      SUBROUTINE P(A)\n"
           "      INTEGER A(3, 2)\n"
           "      PRINT *, A(1, 1), A(3, 1), A(1, 2), A(3, 2)\n"
           "      END\n",
           " 1 3 4 6\n");
    /* formal con limites inferiores en las dos dimensiones */
    SALIDA("      INTEGER V(4)\n"
           "      DO 10 I = 1, 4\n"
           "        V(I) = I\n"
           "   10 CONTINUE\n"
           "      CALL P(V)\n"
           "      END\n"
           "      SUBROUTINE P(A)\n"
           "      INTEGER A(0:1, 5:6)\n"
           "      PRINT *, A(0, 5), A(1, 5), A(0, 6), A(1, 6)\n"
           "      END\n",
           " 1 2 3 4\n");
}

static void test_arrays_elemento(void)
{
    /* un elemento a un formal escalar: por valor con copia de salida (como siempre) */
    SALIDA("      INTEGER V(3)\n"
           "      V(1) = 1\n"
           "      V(2) = 2\n"
           "      V(3) = 3\n"
           "      K = 2\n"
           "      CALL DBL(V(K))\n"
           "      PRINT *, V(1), V(2), V(3)\n"
           "      END\n"
           "      SUBROUTINE DBL(X)\n"
           "      INTEGER X\n"
           "      X = X * 2\n"
           "      END\n",
           " 1 4 3\n");
    /* un elemento a un formal array: el formal es el trozo que empieza en el elemento */
    SALIDA("      INTEGER V(5)\n"
           "      DO 10 I = 1, 5\n"
           "        V(I) = I\n"
           "   10 CONTINUE\n"
           "      CALL P(V(3))\n"
           "      PRINT *, V(1), V(2), V(3), V(4), V(5)\n"
           "      END\n"
           "      SUBROUTINE P(A)\n"
           "      INTEGER A(3)\n"
           "      PRINT *, A(1), A(2), A(3)\n"
           "      A(1) = 0\n"
           "      A(3) = 9\n"
           "      END\n",
           " 3 4 5\n 1 2 0 4 9\n");
    /* una columna de una matriz */
    SALIDA("      INTEGER M(3, 2)\n"
           "      DO 10 J = 1, 2\n"
           "        DO 10 I = 1, 3\n"
           "          M(I, J) = 10 * J + I\n"
           "   10 CONTINUE\n"
           "      J = 2\n"
           "      CALL ZERO(M(1, J), 3)\n"
           "      PRINT *, M(1, 1), M(2, 1), M(3, 1), M(1, 2), M(2, 2), M(3, 2)\n"
           "      END\n"
           "      SUBROUTINE ZERO(A, N)\n"
           "      INTEGER A(N)\n"
           "      DO 10 I = 1, N\n"
           "        A(I) = 0\n"
           "   10 CONTINUE\n"
           "      END\n",
           " 11 12 13 0 0 0\n");
    /* el mismo subprograma con un elemento (formal escalar) y con un array (formal array) en la misma ejecucion */
    SALIDA("      INTEGER V(3)\n"
           "      V(1) = 5\n"
           "      V(2) = 6\n"
           "      V(3) = 7\n"
           "      CALL A1(V(2))\n"
           "      CALL A2(V(2))\n"
           "      PRINT *, V(1), V(2), V(3)\n"
           "      END\n"
           "      SUBROUTINE A1(X)\n"
           "      INTEGER X\n"
           "      X = X + 100\n"
           "      END\n"
           "      SUBROUTINE A2(Y)\n"
           "      INTEGER Y(2)\n"
           "      Y(2) = Y(2) + 1000\n"
           "      END\n",
           " 5 106 1007\n");
    /* elemento de un array CHARACTER a un formal array CHARACTER */
    SALIDA("      CHARACTER*2 T(4)\n"
           "      T(1) = 'aa'\n"
           "      T(2) = 'bb'\n"
           "      T(3) = 'cc'\n"
           "      T(4) = 'dd'\n"
           "      CALL P(T(2))\n"
           "      PRINT *, T(1), T(2), T(3), T(4)\n"
           "      END\n"
           "      SUBROUTINE P(S)\n"
           "      CHARACTER*2 S(2)\n"
           "      S(1) = 'XX'\n"
           "      S(2) = S(1)\n"
           "      END\n",
           " aa XX XX dd\n");
}

static void test_arrays_variables(void)
{
    /* limite ajustable: A(N) */
    SALIDA("      INTEGER V(4)\n"
           "      DO 10 I = 1, 4\n"
           "        V(I) = I * I\n"
           "   10 CONTINUE\n"
           "      PRINT *, ISUM(V, 4), ISUM(V, 2)\n"
           "      END\n"
           "      INTEGER FUNCTION ISUM(A, N)\n"
           "      INTEGER A(N)\n"
           "      ISUM = 0\n"
           "      DO 10 I = 1, N\n"
           "        ISUM = ISUM + A(I)\n"
           "   10 CONTINUE\n"
           "      END\n",
           " 30 5\n");
    /* tamano supuesto: A(*) */
    SALIDA("      INTEGER V(5)\n"
           "      DO 10 I = 1, 5\n"
           "        V(I) = 2 * I\n"
           "   10 CONTINUE\n"
           "      CALL REV(V, 5)\n"
           "      PRINT *, V(1), V(2), V(3), V(4), V(5)\n"
           "      END\n"
           "      SUBROUTINE REV(A, N)\n"
           "      INTEGER A(*)\n"
           "      DO 10 I = 1, N / 2\n"
           "        K = A(I)\n"
           "        A(I) = A(N + 1 - I)\n"
           "        A(N + 1 - I) = K\n"
           "   10 CONTINUE\n"
           "      END\n",
           " 10 8 6 4 2\n");
    /* matriz ajustable A(N, M): el primer indice marca el paso entre columnas */
    SALIDA("      INTEGER M(3, 2)\n"
           "      DO 10 J = 1, 2\n"
           "        DO 10 I = 1, 3\n"
           "          M(I, J) = 10 * I + J\n"
           "   10 CONTINUE\n"
           "      CALL P(M, 3, 2)\n"
           "      END\n"
           "      SUBROUTINE P(A, N, K)\n"
           "      INTEGER A(N, K)\n"
           "      PRINT *, A(1, 1), A(N, 1), A(1, K), A(N, K)\n"
           "      END\n",
           " 11 31 12 32\n");
    /* el mismo subprograma con matrices de formas distintas */
    SALIDA("      INTEGER M(2, 2), W(3, 3)\n"
           "      DO 10 J = 1, 2\n"
           "        DO 10 I = 1, 2\n"
           "          M(I, J) = 10 * I + J\n"
           "   10 CONTINUE\n"
           "      DO 20 J = 1, 3\n"
           "        DO 20 I = 1, 3\n"
           "          W(I, J) = 100 * I + J\n"
           "   20 CONTINUE\n"
           "      CALL P(M, 2)\n"
           "      CALL P(W, 3)\n"
           "      END\n"
           "      SUBROUTINE P(A, N)\n"
           "      INTEGER A(N, *)\n"
           "      PRINT *, A(N, 2), A(1, 2)\n"
           "      END\n",
           " 22 12\n 302 102\n");
    /* limite inferior distinto de 1 en la primera dimension variable */
    SALIDA("      INTEGER M(6)\n"
           "      DO 10 I = 1, 6\n"
           "        M(I) = I\n"
           "   10 CONTINUE\n"
           "      CALL P(M, 2)\n"
           "      END\n"
           "      SUBROUTINE P(A, N)\n"
           "      INTEGER A(0:N, *)\n"
           "      PRINT *, A(0, 1), A(2, 1), A(0, 2), A(1, 2)\n"
           "      END\n",
           " 1 3 4 5\n");
}

static void test_cadena_asumida(void)
{
    /* LEN de un CHARACTER*(*) es la del argumento real; cada llamada puede traer una distinta */
    SALIDA("      CHARACTER*5 A\n"
           "      CHARACTER*3 B\n"
           "      A = 'hello'\n"
           "      B = 'abc'\n"
           "      CALL P(A)\n"
           "      CALL P(B)\n"
           "      CALL P('xy')\n"
           "      END\n"
           "      SUBROUTINE P(S)\n"
           "      CHARACTER*(*) S\n"
           "      PRINT *, LEN(S), S\n"
           "      END\n",
           " 5 hello\n 3 abc\n 2 xy\n");
    /* asignacion a un CHARACTER*(*): rellena con blancos hasta su longitud y se devuelve al real */
    SALIDA("      CHARACTER*6 A\n"
           "      A = 'abcdef'\n"
           "      CALL P(A)\n"
           "      PRINT *, A\n"
           "      END\n"
           "      SUBROUTINE P(S)\n"
           "      CHARACTER*(*) S\n"
           "      S = 'q'\n"
           "      END\n",
           " q     \n");
    /* subcadenas de un CHARACTER*(*), con el limite omitido */
    SALIDA("      CHARACTER*6 A\n"
           "      A = 'abcdef'\n"
           "      CALL P(A)\n"
           "      PRINT *, A\n"
           "      END\n"
           "      SUBROUTINE P(S)\n"
           "      CHARACTER*(*) S\n"
           "      PRINT *, S(2:3), S(4:), S(:2)\n"
           "      S(2:3) = 'ZZ'\n"
           "      S(5:) = '!'\n"
           "      END\n",
           " bc def ab\n aZZd! \n");
    /* concatenacion y comparacion */
    SALIDA("      CHARACTER*3 A\n"
           "      A = 'abc'\n"
           "      CALL P(A)\n"
           "      CALL P('abd')\n"
           "      END\n"
           "      SUBROUTINE P(S)\n"
           "      CHARACTER*(*) S\n"
           "      PRINT *, S // '-' // S\n"
           "      IF (S .EQ. 'abc') PRINT *, 'igual'\n"
           "      IF (S .GT. 'abc') PRINT *, 'mayor'\n"
           "      END\n",
           " abc-abc\n igual\n abd-abd\n mayor\n");
    /* un CHARACTER*(*) se pasa a otro subprograma, con CHARACTER*(*) y con longitud fija */
    SALIDA("      CHARACTER*4 A\n"
           "      A = 'wxyz'\n"
           "      CALL P(A)\n"
           "      PRINT *, A\n"
           "      END\n"
           "      SUBROUTINE P(S)\n"
           "      CHARACTER*(*) S\n"
           "      CALL Q(S)\n"
           "      CALL R(S)\n"
           "      END\n"
           "      SUBROUTINE Q(T)\n"
           "      CHARACTER*(*) T\n"
           "      T(1:1) = '1'\n"
           "      END\n"
           "      SUBROUTINE R(U)\n"
           "      CHARACTER*2 U\n"
           "      U(2:2) = '2'\n"
           "      END\n",
           " 12yz\n");
    /* elemento de un array CHARACTER a un CHARACTER*(*) */
    SALIDA("      CHARACTER*3 T(2)\n"
           "      T(1) = 'abc'\n"
           "      T(2) = 'def'\n"
           "      CALL P(T(2))\n"
           "      PRINT *, T(1), T(2)\n"
           "      END\n"
           "      SUBROUTINE P(S)\n"
           "      CHARACTER*(*) S\n"
           "      S(1:1) = 'D'\n"
           "      END\n",
           " abc Def\n");
    /* una funcion con un argumento CHARACTER*(*) */
    SALIDA("      CHARACTER*7 A\n"
           "      A = 'abcdefg'\n"
           "      K = NB('ab')\n"
           "      PRINT *, K, NB(A), NB(A(2:5))\n"
           "      END\n"
           "      INTEGER FUNCTION NB(S)\n"
           "      CHARACTER*(*) S\n"
           "      NB = LEN(S) * 10\n"
           "      END\n",
           " 20 70 40\n");
}

static void test_subcadenas_argumento(void)
{
    /* S(i:j) como argumento: el trozo se pasa por referencia (copia de entrada y de salida) */
    SALIDA("      CHARACTER*6 A\n"
           "      A = 'abcdef'\n"
           "      CALL P(A(2:3))\n"
           "      PRINT *, A\n"
           "      END\n"
           "      SUBROUTINE P(X)\n"
           "      CHARACTER*2 X\n"
           "      PRINT *, X\n"
           "      X = 'ZZ'\n"
           "      END\n",
           " bc\n aZZdef\n");
    /* limites variables */
    SALIDA("      CHARACTER*6 A\n"
           "      A = 'abcdef'\n"
           "      I = 3\n"
           "      J = 5\n"
           "      CALL P(A(I:J))\n"
           "      PRINT *, A\n"
           "      END\n"
           "      SUBROUTINE P(X)\n"
           "      CHARACTER*3 X\n"
           "      X(1:1) = '<'\n"
           "      X(3:3) = '>'\n"
           "      END\n",
           " ab<d>f\n");
    /* limites variables hacia un CHARACTER*(*) */
    SALIDA("      CHARACTER*8 A\n"
           "      A = 'abcdefgh'\n"
           "      DO 10 I = 1, 3\n"
           "        CALL P(A(I:I+2))\n"
           "   10 CONTINUE\n"
           "      PRINT *, A\n"
           "      END\n"
           "      SUBROUTINE P(X)\n"
           "      CHARACTER*(*) X\n"
           "      PRINT *, LEN(X), X\n"
           "      X(1:1) = '#'\n"
           "      END\n",
           " 3 abc\n 3 bcd\n 3 cde\n ###defgh\n");
    /* subcadena de un elemento de array */
    SALIDA("      CHARACTER*4 T(2)\n"
           "      T(1) = 'abcd'\n"
           "      T(2) = 'efgh'\n"
           "      K = 2\n"
           "      CALL P(T(K)(2:3))\n"
           "      CALL P(T(1)(3:4))\n"
           "      PRINT *, T(1), T(2)\n"
           "      END\n"
           "      SUBROUTINE P(X)\n"
           "      CHARACTER*2 X\n"
           "      X = '**'\n"
           "      END\n",
           " ab** e**h\n");
    /* la subcadena y la variable entera en la misma llamada no se solapan: dos trozos del mismo CHARACTER */
    SALIDA("      CHARACTER*6 A\n"
           "      A = 'abcdef'\n"
           "      CALL SW(A(1:2), A(5:6))\n"
           "      PRINT *, A\n"
           "      END\n"
           "      SUBROUTINE SW(P, Q)\n"
           "      CHARACTER*2 P, Q, T\n"
           "      T = P\n"
           "      P = Q\n"
           "      Q = T\n"
           "      END\n",
           " efcdab\n");
    /* en una expresion de funcion: solo la copia de entrada importa y el valor sigue en la variable */
    SALIDA("      CHARACTER*5 A\n"
           "      A = 'hello'\n"
           "      K = NV(A(2:4))\n"
           "      PRINT *, K, A\n"
           "      END\n"
           "      INTEGER FUNCTION NV(S)\n"
           "      CHARACTER*3 S\n"
           "      NV = 0\n"
           "      IF (S .EQ. 'ell') NV = 1\n"
           "      S = 'xxx'\n"
           "      END\n",
           " 1 hxxxo\n");
}

static void test_arrays_programas(void)
{
    /* ordenacion por burbuja: un subprograma de uso corriente */
    SALIDA("      INTEGER V(6)\n"
           "      V(1) = 5\n"
           "      V(2) = 3\n"
           "      V(3) = 9\n"
           "      V(4) = 1\n"
           "      V(5) = 7\n"
           "      V(6) = 2\n"
           "      CALL SORT(V, 6)\n"
           "      PRINT *, V(1), V(2), V(3), V(4), V(5), V(6)\n"
           "      END\n"
           "      SUBROUTINE SORT(A, N)\n"
           "      INTEGER A(N), T\n"
           "      DO 20 I = 1, N - 1\n"
           "        DO 10 J = 1, N - I\n"
           "          IF (A(J) .GT. A(J + 1)) THEN\n"
           "            T = A(J)\n"
           "            A(J) = A(J + 1)\n"
           "            A(J + 1) = T\n"
           "          END IF\n"
           "   10   CONTINUE\n"
           "   20 CONTINUE\n"
           "      END\n",
           " 1 2 3 5 7 9\n");
    /* producto de matrices 2x2 por 2x2, con tamano ajustable */
    SALIDA("      INTEGER A(2, 2), B(2, 2), C(2, 2)\n"
           "      A(1, 1) = 1\n"
           "      A(2, 1) = 2\n"
           "      A(1, 2) = 3\n"
           "      A(2, 2) = 4\n"
           "      B(1, 1) = 5\n"
           "      B(2, 1) = 6\n"
           "      B(1, 2) = 7\n"
           "      B(2, 2) = 8\n"
           "      CALL MUL(A, B, C, 2)\n"
           "      PRINT *, C(1, 1), C(1, 2), C(2, 1), C(2, 2)\n"
           "      END\n"
           "      SUBROUTINE MUL(A, B, C, N)\n"
           "      INTEGER N, A(N, N), B(N, N), C(N, N)\n"
           "      DO 10 I = 1, N\n"
           "        DO 10 J = 1, N\n"
           "          C(I, J) = 0\n"
           "          DO 10 K = 1, N\n"
           "            C(I, J) = C(I, J) + A(I, K) * B(K, J)\n"
           "   10 CONTINUE\n"
           "      END\n",
           " 23 31 34 46\n");
    /* un array formal pasa a otro subprograma, a dos niveles */
    SALIDA("      INTEGER V(4)\n"
           "      DO 10 I = 1, 4\n"
           "        V(I) = I\n"
           "   10 CONTINUE\n"
           "      CALL A(V)\n"
           "      PRINT *, V(1), V(2), V(3), V(4)\n"
           "      END\n"
           "      SUBROUTINE A(X)\n"
           "      INTEGER X(4)\n"
           "      CALL B(X, 4)\n"
           "      CALL B(X(3), 2)\n"
           "      END\n"
           "      SUBROUTINE B(Y, N)\n"
           "      INTEGER Y(N)\n"
           "      DO 10 I = 1, N\n"
           "        Y(I) = Y(I) * 10\n"
           "   10 CONTINUE\n"
           "      END\n",
           " 10 20 300 400\n");
    /* el mismo array dos veces como argumentos de solo lectura, y funciones con arrays en expresiones */
    SALIDA("      INTEGER V(3), DOT\n"
           "      V(1) = 1\n"
           "      V(2) = 2\n"
           "      V(3) = 3\n"
           "      K = DOT(V, V, 3) + DOT(V, V, 1)\n"
           "      PRINT *, DOT(V, V, 3), DOT(V, V(2), 2), K\n"
           "      END\n"
           "      INTEGER FUNCTION DOT(A, B, N)\n"
           "      INTEGER A(N), B(N)\n"
           "      DOT = 0\n"
           "      DO 10 I = 1, N\n"
           "        DOT = DOT + A(I) * B(I)\n"
           "   10 CONTINUE\n"
           "      END\n",
           " 14 8 15\n");
    /* una funcion REAL con un array REAL y elementos en la misma llamada */
    SALIDA("      REAL R(3)\n"
           "      R(1) = 1.5\n"
           "      R(2) = 2.5\n"
           "      R(3) = 4.0\n"
           "      S = TOTAL(R, 3)\n"
           "      PRINT *, S\n"
           "      CALL PUT(R(2), S)\n"
           "      PRINT *, R(2), S\n"
           "      END\n"
           "      REAL FUNCTION TOTAL(A, N)\n"
           "      REAL A(N)\n"
           "      TOTAL = 0.0\n"
           "      DO 10 I = 1, N\n"
           "        TOTAL = TOTAL + A(I)\n"
           "   10 CONTINUE\n"
           "      END\n"
           "      SUBROUTINE PUT(X, Y)\n"
           "      X = Y\n"
           "      Y = 0.5\n"
           "      END\n",
           " 8.00000000\n 8.00000000 0.500000000\n");
    /* arrays y escalares mezclados, en cualquier orden, con elementos de arrays distintos */
    SALIDA("      INTEGER V(3), W(3)\n"
           "      DO 10 I = 1, 3\n"
           "        V(I) = I\n"
           "        W(I) = 10 * I\n"
           "   10 CONTINUE\n"
           "      K = 5\n"
           "      CALL MIX(K, V, W(2), W, V(3))\n"
           "      PRINT *, K, V(1), V(2), V(3), W(1), W(2), W(3)\n"
           "      END\n"
           "      SUBROUTINE MIX(N, A, E, B, F)\n"
           "      INTEGER N, A(3), E, B(3), F\n"
           "      N = N + A(1) + E + B(3) + F\n"
           "      A(2) = 99\n"
           "      E = -1\n"
           "      F = -2\n"
           "      B(1) = 0\n"
           "      END\n",
           " 59 1 99 -2 0 -1 30\n");
    /* array LOGICAL y elemento de LOGICAL */
    SALIDA("      LOGICAL L(2)\n"
           "      L(1) = .FALSE.\n"
           "      L(2) = .FALSE.\n"
           "      CALL SET(L(2))\n"
           "      CALL CNT(L, N)\n"
           "      PRINT *, L(1), L(2), N\n"
           "      END\n"
           "      SUBROUTINE SET(B)\n"
           "      LOGICAL B\n"
           "      B = .TRUE.\n"
           "      END\n"
           "      SUBROUTINE CNT(B, N)\n"
           "      LOGICAL B(2)\n"
           "      N = 0\n"
           "      DO 10 I = 1, 2\n"
           "        IF (B(I)) N = N + 1\n"
           "   10 CONTINUE\n"
           "      END\n",
           " F T 1\n");
    /* array CHARACTER como argumento */
    SALIDA("      CHARACTER*3 T(3)\n"
           "      T(1) = 'uno'\n"
           "      T(2) = 'dos'\n"
           "      T(3) = 'tre'\n"
           "      CALL REV(T, 3)\n"
           "      PRINT *, T(1), T(2), T(3)\n"
           "      END\n"
           "      SUBROUTINE REV(A, N)\n"
           "      CHARACTER*3 A(N), W\n"
           "      W = A(1)\n"
           "      A(1) = A(N)\n"
           "      A(N) = W\n"
           "      A(2)(1:1) = '_'\n"
           "      END\n",
           " tre _os uno\n");
    /* subprogramas que no se llaman en el orden en que se definen: el formal array se resuelve igual */
    SALIDA("      INTEGER V(2)\n"
           "      V(1) = 4\n"
           "      V(2) = 5\n"
           "      CALL P(V)\n"
           "      END\n"
           "      SUBROUTINE P(A)\n"
           "      INTEGER A(2)\n"
           "      CALL Q(A(2), A)\n"
           "      PRINT *, A(1), A(2)\n"
           "      END\n"
           "      SUBROUTINE Q(X, B)\n"
           "      INTEGER X, B(2)\n"
           "      X = X + B(1)\n"
           "      END\n",
           " 4 9\n");
}

static void test_arrays_errores(void)
{
    /* tipo: array a escalar, escalar o expresion a array, tipos distintos */
    ERROR("      INTEGER V(3)\n      CALL S(V)\n      END\n      SUBROUTINE S(I)\n      INTEGER I\n      END\n",
          1, 2, 12, U(UNI_TIPO_ARG));
    ERROR("      CALL S(5)\n      END\n      SUBROUTINE S(A)\n      INTEGER A(3)\n      END\n", 1, 1, 12, U(UNI_TIPO_ARG));
    ERROR("      INTEGER X\n      CALL S(X)\n      END\n      SUBROUTINE S(A)\n      INTEGER A(3)\n      END\n",
          1, 2, 12, U(UNI_TIPO_ARG));
    ERROR("      INTEGER V(3)\n      CALL S(V)\n      END\n      SUBROUTINE S(A)\n      REAL A(3)\n      END\n",
          1, 2, 12, U(UNI_TIPO_ARG));
    ERROR("      INTEGER V(3)\n      CALL S(V(1))\n      END\n      SUBROUTINE S(A)\n      REAL A(3)\n      END\n",
          1, 2, 12, U(UNI_TIPO_ARG));
    ERROR("      INTEGER V(3)\n      CALL S(V(1) + 1)\n      END\n      SUBROUTINE S(A)\n      INTEGER A(3)\n      END\n",
          1, 2, 12, U(UNI_TIPO_ARG));
    ERROR("      INTEGER V(3)\n      CALL S(V)\n      END\n      SUBROUTINE S(A)\n      CHARACTER*(*) A\n      END\n",
          1, 2, 12, U(UNI_TIPO_ARG));
    /* tamano: el array real es menor que el formal */
    ERROR("      INTEGER V(3)\n      CALL S(V)\n      END\n      SUBROUTINE S(A)\n      INTEGER A(5)\n      END\n",
          1, 2, 12, U(UNI_TAM_ARRAY));
    ERROR("      INTEGER V(2, 2)\n      CALL S(V)\n      END\n      SUBROUTINE S(A)\n      INTEGER A(5)\n      END\n",
          1, 2, 12, U(UNI_TAM_ARRAY));
    ERROR("      CHARACTER*3 T(2)\n      CALL S(T)\n      END\n      SUBROUTINE S(A)\n      CHARACTER*4 A(2)\n      END\n",
          1, 2, 12, U(UNI_LONG_ARRAY));
    ERROR("      CHARACTER*4 A\n      CALL S(A(1:2))\n      END\n      SUBROUTINE S(X)\n      CHARACTER*3 X\n      END\n",
          1, 2, 12, U(UNI_LONG_ARG));
    /* declaraciones de formales */
    ERROR("      END\n      SUBROUTINE S(A)\n      INTEGER A(*, 3)\n      END\n", 1, 3, 20, SEM_DIMENSION);
    ERROR("      INTEGER V(*)\n      END\n", 1, 1, 17, SEM_DIMENSION);
    ERROR("      CHARACTER*(*) X\n      END\n", 1, 1, 18, SEM_LONGITUD);
    ERROR("      END\n      SUBROUTINE S(A)\n      INTEGER A(N)\n      END\n", 1, 3, 17, SEM_DIMENSION);
    ERROR("      END\n      SUBROUTINE S(A, X)\n      REAL X\n      INTEGER A(X)\n      END\n", 1, 4, 17, SEM_DIMENSION);
    ERROR("      END\n      SUBROUTINE S(A, N)\n      INTEGER N\n      INTEGER A(N+1)\n      END\n", 1, 4, 18, SEM_DIMENSION);
    ERROR("      END\n      FUNCTION F(A)\n      CHARACTER*(*) F\n      END\n", 1, 3, 18, SEM_LONGITUD);
    ERROR("      END\n      SUBROUTINE S(A)\n      CHARACTER*(*) A(3)\n      END\n", 1, 3, 18, SEM_LONGITUD);
    /* el nombre de un array sigue necesitando subindices en una expresion */
    ERROR("      INTEGER V(3)\n      K = V\n      END\n", 1, 2, 11, SEM_ARRAY_SIN_INDICES);
    /* lo que si cabe: el real mayor que el formal, ajustable, supuesto y sin ordenar */
    SALIDA("      INTEGER V(5)\n      V(5) = 7\n      CALL S(V)\n      CALL T(V, 5)\n      CALL U(V)\n      END\n"
           "      SUBROUTINE S(A)\n      INTEGER A(2)\n      A(2) = 1\n      END\n"
           "      SUBROUTINE T(A, N)\n      INTEGER A(N)\n      PRINT *, A(N)\n      END\n"
           "      SUBROUTINE U(A)\n      INTEGER A(*)\n      PRINT *, A(2), A(5)\n      END\n",
           " 7\n 1 7\n");
}

static void test_arrays_casos_raros(void)
{
    /* subcadena de longitud cero hacia un CHARACTER*(*): la copia de salida no copia nada (LDIR con BC = 0 copiaria 64 KB) */
    SALIDA("      CHARACTER*6 A\n"
           "      A = 'abcdef'\n"
           "      I = 3\n"
           "      J = 2\n"
           "      CALL P(A(I:J))\n"
           "      CALL P(A(4:3))\n"
           "      PRINT *, A\n"
           "      END\n"
           "      SUBROUTINE P(X)\n"
           "      CHARACTER*(*) X\n"
           "      PRINT *, LEN(X)\n"
           "      END\n",
           " 0\n 0\n abcdef\n");
    /* S(2:) de un CHARACTER*(*) pasado a un CHARACTER*2: su longitud no se conoce al compilar y no es un error */
    SALIDA("      CHARACTER*6 A\n"
           "      A = 'abcdef'\n"
           "      CALL P(A)\n"
           "      PRINT *, A\n"
           "      END\n"
           "      SUBROUTINE P(S)\n"
           "      CHARACTER*(*) S\n"
           "      CALL Q(S(2:))\n"
           "      CALL R(S)\n"
           "      END\n"
           "      SUBROUTINE Q(X)\n"
           "      CHARACTER*2 X\n"
           "      X = '**'\n"
           "      END\n"
           "      SUBROUTINE R(X)\n"
           "      CHARACTER*2 X\n"
           "      X(1:1) = '+'\n"
           "      END\n",
           " +**def\n");
    /* el subindice del elemento pasado es una llamada a funcion (llamada anidada con argumentos pendientes) */
    SALIDA("      INTEGER V(4), IDX\n"
           "      DO 10 I = 1, 4\n"
           "        V(I) = I\n"
           "   10 CONTINUE\n"
           "      CALL P(V(IDX(2)), V)\n"
           "      PRINT *, V(1), V(2), V(3), V(4)\n"
           "      END\n"
           "      INTEGER FUNCTION IDX(K)\n"
           "      IDX = K + 1\n"
           "      END\n"
           "      SUBROUTINE P(E, A)\n"
           "      INTEGER E, A(4)\n"
           "      E = E * 10\n"
           "      A(1) = A(1) + E\n"
           "      END\n",
           " 31 2 30 4\n");
    /* elementos de una matriz con subindices en expresion, hacia un escalar y hacia un formal array */
    SALIDA("      INTEGER M(3, 3)\n"
           "      DO 10 J = 1, 3\n"
           "        DO 10 I = 1, 3\n"
           "          M(I, J) = 10 * I + J\n"
           "   10 CONTINUE\n"
           "      I = 1\n"
           "      J = 2\n"
           "      CALL INC(M(I + 1, J))\n"
           "      CALL COL(M(I + 1, J + 1), 2)\n"
           "      PRINT *, M(2, 2), M(3, 2), M(1, 3), M(2, 3), M(3, 3)\n"
           "      END\n"
           "      SUBROUTINE INC(K)\n"
           "      INTEGER K\n"
           "      K = K + 100\n"
           "      END\n"
           "      SUBROUTINE COL(A, N)\n"
           "      INTEGER N, A(N)\n"
           "      DO 10 I = 1, N\n"
           "        A(I) = -A(I)\n"
           "   10 CONTINUE\n"
           "      END\n",
           " 122 32 13 -23 -33\n");
    /* los 15 argumentos permitidos, con arrays, elementos, cadenas y subcadenas (cada uno distinto: la norma no deja repetir) */
    SALIDA("      INTEGER V(2), W(2), X(2), Y(2), Z(2), K\n"
           "      CHARACTER*4 S, U, R, T(2), Q(2)\n"
           "      V(1) = 1\n"
           "      V(2) = 2\n"
           "      W(1) = 3\n"
           "      W(2) = 4\n"
           "      X(1) = 5\n"
           "      X(2) = 6\n"
           "      Y(1) = 7\n"
           "      Y(2) = 8\n"
           "      Z(1) = 9\n"
           "      Z(2) = 10\n"
           "      S = 'abcd'\n"
           "      U = 'efgh'\n"
           "      R = 'mnop'\n"
           "      T(1) = 'qrst'\n"
           "      T(2) = 'uvwx'\n"
           "      Q(1) = 'ijkl'\n"
           "      Q(2) = 'ABCD'\n"
           "      K = 5\n"
           "      CALL M15(V, W, X(2), Y(1), K, S, U(2:3), T, Q(2),\n"
           "     +         Q(1)(1:2), 7, 'xy', Z, 8, R)\n"
           "      PRINT *, V(1), V(2), W(1), W(2), K\n"
           "      PRINT *, X(2), Y(1)\n"
           "      PRINT *, S, U, R\n"
           "      PRINT *, T(1), T(2), Q(1), Q(2)\n"
           "      END\n"
           "      SUBROUTINE M15(A1, A2, E1, E2, K, S1, S2, T1, T2,\n"
           "     +               T3, N1, L1, A3, N2, S3)\n"
           "      INTEGER A1(2), A2(2), E1, E2, K, N1, A3(2), N2\n"
           "      CHARACTER*4 S1, T1(2), T2, S3\n"
           "      CHARACTER*2 S2, T3, L1\n"
           "      A1(1) = N1\n"
           "      A2(2) = N2\n"
           "      E1 = E1 + A3(1)\n"
           "      E2 = E2 * 10\n"
           "      K = K + 1\n"
           "      S2 = L1\n"
           "      T3 = 'ZZ'\n"
           "      T2(4:4) = '!'\n"
           "      S3(1:1) = '#'\n"
           "      S1(2:2) = '@'\n"
           "      T1(2)(1:1) = '%'\n"
           "      END\n",
           " 7 2 3 8 6\n 15 70\n a@cd exyh #nop\n qrst %vwx ZZkl ABC!\n");
}

static void test_arrays_anidadas(void)
{
    /* llamadas a funcion anidadas con argumentos de todas las formas: las ranuras del buffer de argumentos se apilan */
    SALIDA("      INTEGER V(3), F\n"
           "      V(1) = 1\n"
           "      V(2) = 2\n"
           "      V(3) = 3\n"
           "      K = F(V, F(V(2), F(V(3), 0)))\n"
           "      PRINT *, K\n"
           "      END\n"
           "      INTEGER FUNCTION F(A, N)\n"
           "      INTEGER A(1), N\n"
           "      F = A(1) + N\n"
           "      END\n",
           " 6\n");
}

/* K = G(1, ..., 1, G(1, ..., 1, G(... 0 ...))) con `n` niveles de G de 15 argumentos (los 14 primeros constantes): cada nivel deja 14
 * ranuras del buffer de argumentos ocupadas mientras se evalua el siguiente. Partido en lineas de 72 columnas con continuaciones. */
static void anidadas(char *dst, int n)
{
    char expr[1024], ln[96];
    int i, k, len = 0, col;

    for (i = 0; i < n; i++) {
        expr[len++] = 'G';
        expr[len++] = '(';
        for (k = 0; k < 14; k++) {
            expr[len++] = '1';
            expr[len++] = ',';
        }
    }
    expr[len++] = '0';
    for (i = 0; i < n; i++) {
        expr[len++] = ')';
    }
    expr[len] = '\0';
    strcpy(dst, "      INTEGER G\n");
    strcpy(ln, "      K = ");
    col = (int)strlen(ln);
    for (i = 0; i < len; i++) {
        ln[col++] = expr[i];
        ln[col] = '\0';
        if (col == 72 && i + 1 < len) {
            strcat(dst, ln);
            strcat(dst, "\n");
            strcpy(ln, "     +");
            col = 6;
        }
    }
    strcat(dst, ln);
    strcat(dst, "\n      PRINT *, K\n      END\n"
                "      INTEGER FUNCTION G(A, B, C, D, E, F, H, I, J, L, M, N, O, P, Q)\n"
                "      INTEGER A, B, C, D, E, F, H, I, J, L, M, N, O, P, Q\n"
                "      G = A + B + C + D + E + F + H + I + J + L + M + N + O + P + Q\n"
                "      END\n");
}

static void test_arrays_profundas(void)
{
    static char prog[8192], esp[16];

    /* Dos niveles (30 ranuras en uso a la vez). Mas no caben: el arbol de una sentencia tiene 64 nodos (AST_NODOS) y cada argumento
     * gasta dos, asi que las 93 ranuras de GEN_ARG_MAX no se agotan nunca. */
    anidadas(prog, 2);
    sprintf(esp, " %d\n", 2 * 14);
    salida(__LINE__, prog, esp);
}

static void test_arrays(void)
{
    test_arrays_profundas();
    test_arrays_casos_raros();
    test_arrays_anidadas();
    test_arrays_programas();
    test_arrays_errores();
    test_subcadenas_argumento();
    test_cadena_asumida();
    test_arrays_basicos();
    test_arrays_elemento();
    test_arrays_variables();
}
