C     ALGORITMOS CLASICOS: Collatz, numeros perfectos, binario, capicua, bisiestos
      PROGRAM ALGO
      INTEGER I, J, N, S, PASOS, MEJOR, QUIEN, R, D, K
      LOGICAL BIS
      MEJOR = 0
      QUIEN = 0
      DO 20 I = 1, 30
         N = I
         PASOS = 0
   10    IF (N .EQ. 1) GOTO 15
         IF (MOD(N, 2) .EQ. 0) THEN
            N = N / 2
         ELSE
            N = 3 * N + 1
         END IF
         PASOS = PASOS + 1
         GOTO 10
   15    IF (PASOS .GT. MEJOR) THEN
            MEJOR = PASOS
            QUIEN = I
         END IF
   20 CONTINUE
      PRINT *, 'COLLATZ', QUIEN, MEJOR
      DO 40 I = 2, 500
         S = 1
         DO 30 J = 2, I / 2
            IF (MOD(I, J) .EQ. 0) S = S + J
   30    CONTINUE
         IF (S .EQ. I) PRINT *, 'PERFECTO', I
   40 CONTINUE
      DO 60 I = 1, 4
         N = I * 7
         R = 0
         K = 1
   50    IF (N .EQ. 0) GOTO 55
         R = R + MOD(N, 2) * K
         K = K * 10
         N = N / 2
         GOTO 50
   55    PRINT *, I * 7, R
   60 CONTINUE
      N = 12321
      R = 0
      K = N
   70 IF (K .EQ. 0) GOTO 75
      R = R * 10 + MOD(K, 10)
      K = K / 10
      GOTO 70
   75 PRINT *, N, R, R .EQ. N
      DO 80 I = 1896, 2100, 52
         BIS = MOD(I, 4) .EQ. 0 .AND.
     +         (MOD(I, 100) .NE. 0 .OR. MOD(I, 400) .EQ. 0)
         PRINT *, I, BIS
   80 CONTINUE
      END
