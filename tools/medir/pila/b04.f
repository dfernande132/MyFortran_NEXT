C     ESCALA DE PILA (ver estres.f)
      PROGRAM ESC
      INTEGER I, N, R, K
      I = 1
      N = 2
      IF (I .GT. 0) THEN
      DO 101 K = 1, 1
      IF (I .GT. 0) THEN
      DO 103 K = 1, 1
      R = 1
  103 CONTINUE
      END IF
  101 CONTINUE
      END IF
      PRINT *, R
      END
