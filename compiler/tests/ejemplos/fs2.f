C Subprogramas, etapa S2a: FUNCTION INTEGER / REAL / LOGICAL.
      INTEGER FUNCTION FACT(N)
      INTEGER N, I
      FACT = 1
      DO 10 I = 2, N
         FACT = FACT * I
   10 CONTINUE
      END
      REAL FUNCTION MEDIA(A, B)
      REAL A, B
      MEDIA = (A + B) * 0.5
      END
      LOGICAL FUNCTION PRIMO(N)
      INTEGER N, D
      PRIMO = N .GT. 1
      D = 2
   20 IF (D * D .GT. N) RETURN
      IF (MOD(N, D) .EQ. 0) THEN
         PRIMO = .FALSE.
         RETURN
      END IF
      D = D + 1
      GO TO 20
      END
      INTEGER FUNCTION SIG(N)
      INTEGER N
      N = N + 1
      SIG = N
      END
      PROGRAM PRUEBA
      INTEGER FACT, SIG, I, J, K
      REAL MEDIA
      LOGICAL PRIMO
      PRINT *, FACT(5), FACT(FACT(3))
      PRINT *, MEDIA(1.0, 2.0), MEDIA(MEDIA(1.0, 3.0), 5.0)
      K = 0
      DO 30 I = 1, 20
         IF (PRIMO(I)) K = K + 1
   30 CONTINUE
      PRINT *, 'PRIMOS HASTA 20', K
      I = 10
      J = SIG(I)
      K = SIG(I)
      PRINT *, I, J, K
      END
