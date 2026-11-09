C     CRIBA DE ERATOSTENES con un vector LOGICAL y PARAMETER
      PROGRAM PRIMOS
      INTEGER N
      PARAMETER (N = 200)
      LOGICAL ES(N)
      INTEGER I, J, CUENTA, SUMA, ULT
      DO 10 I = 1, N
         ES(I) = .TRUE.
   10 CONTINUE
      ES(1) = .FALSE.
      DO 30 I = 2, N
         IF (I * I .GT. N) GOTO 40
         IF (.NOT. ES(I)) GOTO 30
         DO 20 J = I * I, N, I
            ES(J) = .FALSE.
   20    CONTINUE
   30 CONTINUE
   40 CUENTA = 0
      SUMA = 0
      ULT = 0
      DO 50 I = 1, N
         IF (ES(I)) THEN
            CUENTA = CUENTA + 1
            SUMA = SUMA + I
            ULT = I
            IF (CUENTA .LE. 10) PRINT *, CUENTA, I
         END IF
   50 CONTINUE
      PRINT *, 'PRIMOS HASTA', N, CUENTA
      PRINT *, 'SUMA', SUMA, 'ULTIMO', ULT
      END
