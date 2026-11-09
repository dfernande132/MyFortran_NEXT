C     ESTRES DE PILA: PARENTESIS, IF/DO ANIDADOS, F(F(...)) Y READ
      PROGRAM ESTRES
      INTEGER I, J, K, L, M, N, R, ID
      INTEGER A(3)
      I = 1
      N = 2
      R = (((((((I+N)))))))
      R = ((((((((I+N)*(I-N))))))))
      R = ABS(ABS(ABS(ABS(ABS(I+N)))))
      R = ABS((ABS((ABS((I+N))))))
      PRINT *, R
      IF (I .GT. 0) THEN
      DO 101 K = 1, 1
      IF (I .GT. 0) THEN
      DO 103 K = 1, 1
      IF (I .GT. 0) THEN
      DO 105 K = 1, 1
      IF (I .GT. 0) THEN
      DO 107 K = 1, 1
      IF (I .GT. 0) THEN
      DO 109 K = 1, 1
      IF (I .GT. 0) THEN
      DO 111 K = 1, 1
      IF (I .GT. 0) THEN
      DO 113 K = 1, 1
      IF (I .GT. 0) THEN
      DO 115 K = 1, 1
      R = R + 1
  115 CONTINUE
      END IF
  113 CONTINUE
      END IF
  111 CONTINUE
      END IF
  109 CONTINUE
      END IF
  107 CONTINUE
      END IF
  105 CONTINUE
      END IF
  103 CONTINUE
      END IF
  101 CONTINUE
      END IF
      PRINT *, R
      READ *, J
      PRINT *, J + R
      END
