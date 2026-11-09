C     REALES: aritmetica IEEE de 32 bits, conversiones, SQRT exacta, acumulacion,
C     DO con variable REAL y comparaciones
      PROGRAM REALES
      REAL X, Y, S, Z
      INTEGER I, N
      X = 1.5
      Y = 0.25
      PRINT *, X + Y, X - Y, X * Y, X / Y
      PRINT *, 1.0 / 3.0, 2.0 / 3.0, 10.0 / 4.0, 100.0 / 7.0
      S = 0.0
      DO 10 I = 1, 10
         S = S + 0.1
   10 CONTINUE
      PRINT *, 'SUMA 0.1', S
      S = 0.0
      DO 20 I = 1, 10
         S = S + 0.5
   20 CONTINUE
      PRINT *, 'SUMA 0.5', S
      PRINT *, I + 0.5, REAL(7) / 2.0, FLOAT(I) * 1.5
      PRINT *, INT(3.99), INT(-3.99), NINT(2.5), NINT(-2.5), NINT(2.4)
      PRINT *, INT(0.5), NINT(0.5), NINT(-0.4), INT(100.7)
      PRINT *, AINT(2.7), AINT(-2.7), ANINT(2.5), ANINT(-2.5)
      PRINT *, SQRT(16.0), SQRT(2.25), SQRT(0.25), SQRT(100.0)
      PRINT *, ABS(-2.5), MAX(1.5, 2.5), MIN(1.5, 2.5), AMOD(7.5, 2.0)
      PRINT *, SIGN(3.0, -1.0), DIM(5.5, 2.0), DIM(2.0, 5.5)
      PRINT *, X ** 2, 2.0 ** 3, 0.5 ** 4, Y ** 0
      Z = 3.0
      PRINT *, -Z, -Z * Z, (-Z) ** 2, - Z + 5.0
      N = 0
      DO 30 X = 0.0, 1.0, 0.25
         N = N + 1
         PRINT *, N, X
   30 CONTINUE
      DO 40 X = 2.0, 1.0, -0.5
         PRINT *, X
   40 CONTINUE
      PRINT *, X .GT. 0.9, X .LT. 0.9, 0.1 + 0.2 .EQ. 0.3, 1.5 .NE. 1.5
      PRINT *, 123.456, 0.000, 99999.5, 1234.5678
      END
