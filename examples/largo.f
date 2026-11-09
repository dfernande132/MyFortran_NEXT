C     PROGRAMA LARGO PARA PROBAR EL EDITOR Y EL COMPILADOR
C     SIN ERRORES: DEBE COMPILAR Y CONSTRUIR
      PROGRAM LARGO
      INTEGER F(20), P(30), S, I, J, K, N, M, T
      INTEGER A, B, C, D, MAXV, MINV, CUENTA
      CHARACTER*12 NOMBRE, SALUDO
      CHARACTER*30 FRASE
C
C     FIBONACCI
      F(1) = 1
      F(2) = 1
      DO 10 I = 3, 20
         F(I) = F(I - 1) + F(I - 2)
   10 CONTINUE
      PRINT *, 'FIBONACCI'
      DO 20 I = 1, 20
         PRINT *, I, F(I)
   20 CONTINUE
C
C     CRIBA DE ERATOSTENES HASTA 30
      DO 30 I = 1, 30
         P(I) = 1
   30 CONTINUE
      P(1) = 0
      DO 50 I = 2, 5
         IF (P(I) .EQ. 1) THEN
            DO 40 J = I * I, 30, I
               P(J) = 0
   40       CONTINUE
         END IF
   50 CONTINUE
      PRINT *, 'PRIMOS'
      CUENTA = 0
      DO 60 I = 1, 30
         IF (P(I) .EQ. 1) THEN
            PRINT *, I
            CUENTA = CUENTA + 1
         END IF
   60 CONTINUE
      PRINT *, 'CUANTOS', CUENTA
C
C     SUMAS Y EXTREMOS
      S = 0
      MAXV = F(1)
      MINV = F(1)
      DO 70 I = 1, 20
         S = S + F(I)
         IF (F(I) .GT. MAXV) MAXV = F(I)
         IF (F(I) .LT. MINV) MINV = F(I)
   70 CONTINUE
      PRINT *, 'SUMA', S
      PRINT *, 'MAXIMO', MAXV
      PRINT *, 'MINIMO', MINV
C
C     TABLA DE MULTIPLICAR
      DO 90 I = 1, 5
         DO 80 J = 1, 5
            K = I * J
            PRINT *, I, J, K
   80    CONTINUE
   90 CONTINUE
C
C     CLASIFICACION
      N = 7
      IF (N .LT. 0) THEN
         PRINT *, 'NEGATIVO'
      ELSE IF (N .EQ. 0) THEN
         PRINT *, 'CERO'
      ELSE IF (N .LT. 10) THEN
         PRINT *, 'UN DIGITO'
      ELSE
         PRINT *, 'GRANDE'
      END IF
C
C     COLLATZ DESDE 6
      M = 6
      T = 0
  100 IF (M .EQ. 1) GOTO 120
      IF (M - (M / 2) * 2 .EQ. 0) THEN
         M = M / 2
      ELSE
         M = 3 * M + 1
      END IF
      T = T + 1
      GOTO 100
  120 PRINT *, 'PASOS COLLATZ', T
C
C     CADENAS
      NOMBRE = 'MYFORTRAN'
      SALUDO = 'HOLA'
      FRASE = SALUDO // ' ' // NOMBRE
      PRINT *, FRASE
C
C     MAS ENTEROS
      A = 100
      B = 7
      C = A / B
      D = A - C * B
      PRINT *, A, B, C, D
      END
