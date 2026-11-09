C     ORDENACION por burbuja de 20 valores de un generador congruencial
      PROGRAM ORDEN
      INTEGER V(20), I, T, X, SUMA
      LOGICAL CAMBIO, OK
      X = 7
      DO 10 I = 1, 20
         X = MOD(X * 25 + 13, 251)
         V(I) = X
   10 CONTINUE
      PRINT *, 'ANTES'
      DO 15 I = 1, 20, 5
         PRINT *, V(I), V(I + 1), V(I + 2), V(I + 3), V(I + 4)
   15 CONTINUE
   20 CAMBIO = .FALSE.
      DO 30 I = 1, 19
         IF (V(I) .GT. V(I + 1)) THEN
            T = V(I)
            V(I) = V(I + 1)
            V(I + 1) = T
            CAMBIO = .TRUE.
         END IF
   30 CONTINUE
      IF (CAMBIO) GOTO 20
      PRINT *, 'DESPUES'
      DO 35 I = 1, 20, 5
         PRINT *, V(I), V(I + 1), V(I + 2), V(I + 3), V(I + 4)
   35 CONTINUE
      OK = .TRUE.
      SUMA = 0
      DO 40 I = 1, 19
         IF (V(I) .GT. V(I + 1)) OK = .FALSE.
         SUMA = MOD(SUMA + I * V(I), 1000)
   40 CONTINUE
      PRINT *, 'ORDENADO', OK, 'CONTROL', SUMA
      END
