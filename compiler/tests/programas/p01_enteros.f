C     ENTEROS: operadores, division y MOD con signos, potencias, precedencia
      PROGRAM ENTERO
      INTEGER A, B, I
      A = 17
      B = 5
      PRINT *, A + B, A - B, A * B, A / B, MOD(A, B)
      A = -17
      PRINT *, A / B, MOD(A, B), -A / B, MOD(-A, B)
      B = -5
      PRINT *, 17 / B, MOD(17, B), A / B, MOD(A, B)
      PRINT *, 2 ** 0, 2 ** 1, 3 ** 4, (-2) ** 3, (-2) ** 4
      PRINT *, 2 + 3 * 4, (2 + 3) * 4, 2 * 3 ** 2, 100 / 10 / 5
      PRINT *, -2 ** 2, 10 - 4 - 3, 2 ** 3 ** 2
C     digitos de 1234 con un bucle de GOTO
      A = 1234
      B = 0
      I = 0
   10 IF (A .EQ. 0) GOTO 20
      B = B + MOD(A, 10)
      A = A / 10
      I = I + 1
      GOTO 10
   20 PRINT *, 'DIGITOS', I, 'SUMA', B
      PRINT *, ABS(-7), ABS(7), IABS(-3), MAX(3, 9, 4), MIN(3, 9, 4, -1)
      PRINT *, SIGN(5, -1), SIGN(-5, 1), ISIGN(4, 3)
      PRINT *, DIM(7, 3), DIM(3, 7), IDIM(2, 9)
      PRINT *, MAX(-5, -2), MIN(10, 20), MOD(7, 7), MOD(0, 5)
      PRINT *, 32767, -32767, 0
      END
