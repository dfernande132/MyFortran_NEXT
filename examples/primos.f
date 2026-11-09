C     PRIMOS: criba de Eratostenes hasta el N que se teclee (1 a 200),
C     con descomposicion en factores primos del ultimo primo mas uno
      PROGRAM PRIMOS
      INTEGER MAXN
      PARAMETER (MAXN = 200)
      LOGICAL ES(MAXN)
      INTEGER N, I, J, CUENTA, M, F
      PRINT *, 'N (1 A 200)?'
      READ *, N
      IF (N .LT. 1 .OR. N .GT. MAXN) THEN
         PRINT *, 'FUERA DE RANGO'
         STOP
      END IF
      DO 10 I = 1, N
         ES(I) = I .GT. 1
   10 CONTINUE
      DO 30 I = 2, N
         IF (I * I .GT. N) GOTO 40
         IF (.NOT. ES(I)) GOTO 30
         DO 20 J = I * I, N, I
            ES(J) = .FALSE.
   20    CONTINUE
   30 CONTINUE
   40 CUENTA = 0
      M = 0
      DO 50 I = 1, N
         IF (ES(I)) THEN
            CUENTA = CUENTA + 1
            M = I
            PRINT *, I
         END IF
   50 CONTINUE
      PRINT *, 'HAY', CUENTA, 'PRIMOS HASTA', N
      IF (M .EQ. 0) STOP
      M = M + 1
      PRINT *, 'FACTORES DE', M
      F = 2
   60 IF (M .EQ. 1) GOTO 80
   70 IF (MOD(M, F) .NE. 0) THEN
         F = F + 1
         GOTO 60
      END IF
      PRINT *, F
      M = M / F
      GOTO 70
   80 END
