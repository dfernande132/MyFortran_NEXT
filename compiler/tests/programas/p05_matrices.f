C     VECTORES Y MATRICES: producto 4x4, traza, traspuesta, limites inferiores
C     distintos de 1
      PROGRAM MATRIZ
      INTEGER A(4,4), B(4,4), C(4,4), I, J, K, T, TRAZA
      INTEGER L(-2:2)
      DO 10 I = 1, 4
         DO 10 J = 1, 4
            A(I,J) = I + J
            B(I,J) = I - J
   10 CONTINUE
      DO 30 I = 1, 4
         DO 30 J = 1, 4
            T = 0
            DO 20 K = 1, 4
               T = T + A(I,K) * B(K,J)
   20       CONTINUE
            C(I,J) = T
   30 CONTINUE
      DO 40 I = 1, 4
         PRINT *, C(I,1), C(I,2), C(I,3), C(I,4)
   40 CONTINUE
      TRAZA = 0
      DO 50 I = 1, 4
         TRAZA = TRAZA + C(I,I)
   50 CONTINUE
      PRINT *, 'TRAZA', TRAZA
      DO 60 I = 1, 4
         DO 60 J = 1, 4
            B(J,I) = A(I,J) * 10 + I
   60 CONTINUE
      PRINT *, B(1,1), B(1,4), B(4,1), B(2,3)
      DO 70 I = -2, 2
         L(I) = I * I
   70 CONTINUE
      PRINT *, L(-2), L(-1), L(0), L(1), L(2)
      END
