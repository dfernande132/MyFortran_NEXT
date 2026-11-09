C     Pruebas de REAL (IEEE 754 simple por software)
      PROGRAM REALES
      REAL A, B, C, S, V(5)
      INTEGER I, N
      A = 1.5
      B = 0.1
      C = A * B + 2
      PRINT *, A, B, C
C     modo mixto y conversiones
      N = 7
      PRINT *, N / 2, N / 2.0, REAL(N) / 2
      I = -2.75
      PRINT *, I, INT(3.99), IFIX(-3.99)
C     serie armonica
      S = 0.0
      DO 10 I = 1, 20
         S = S + 1.0 / I
   10 CONTINUE
      PRINT *, 'H(20) =', S
C     array de reales
      DO 20 I = 1, 5
         V(I) = I * 0.25
   20 CONTINUE
      PRINT *, V(1), V(2), V(5)
C     comparaciones
      IF (A .GT. B) PRINT *, 'A > B'
      IF (B * 10 .EQ. 1.0) PRINT *, 'B*10 = 1'
C     rangos
      PRINT *, 1E10, 1.2E-5, 123456789.0, 3.4E38, 0.0
      PRINT *, -A, 2.0 / 3.0, 1.0 / 3.0
C     intrinsecas REAL
      PRINT *, ABS(-A), AINT(-2.5), ANINT(-2.5), NINT(2.5)
      PRINT *, SQRT(2.0), SQRT(16.0), AMOD(7.5, 2.0), AMOD(1.0, 0.1)
      PRINT *, MAX(A, B, 2.0), MIN(A, B), SIGN(3.0, -A), DIM(5.0, 3.0)
C     REAL ** INTEGER
      N = 3
      PRINT *, 2.0 ** 10, A ** N, 2.0 ** (-N), A ** 0
C     DO con variable REAL
      DO 30 X = 0.0, 1.0, 0.25
         PRINT *, X
   30 CONTINUE
C     errores de ejecucion: el programa imprime el mensaje y PARA
C     (division por cero, 0/0 y overflow REAL; ANSI X3.9-1978 6.6)
      C = 1E30
      PRINT *, 'overflow:'
      PRINT *, C * C
      PRINT *, 'no se imprime'
      END
