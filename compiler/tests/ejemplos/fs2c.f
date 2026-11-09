C Subprogramas, etapa S2c: funciones de sentencia F(X) = expresion.
      INTEGER FUNCTION TRIPLE(N)
      INTEGER N
      TRIPLE = 3 * N
      END
      SUBROUTINE S(A)
      INTEGER A, SQ, CUBO
      SQ(A) = A * A
      CUBO(K) = K * SQ(K)
      PRINT *, SQ(A), CUBO(2), CUBO(SQ(2))
      END
      PROGRAM PRUEBA
      INTEGER X, Y, F, G, H, V(3), I, TRIPLE, DOBLE
      REAL R, CUAD, MEDIA
      LOGICAL ESPAR
      F(X) = X + 100
      G(Y) = F(Y) * 2 + X
      H(I) = V(I) + TRIPLE(I) + ABS(-I)
      CUAD(R) = R * R
      MEDIA(R, T) = (R + T) / 2.0
      DOBLE(I) = I * 1.5
      ESPAR(I) = MOD(I, 2) .EQ. 0
      X = 7
      Y = 1
      V(1) = 10
      V(2) = 20
      V(3) = 30
      PRINT *, F(1), F(Y), F(X), G(5), G(X)
      PRINT *, H(1), H(2), H(3)
      DO 10 I = F(0) - 98, F(1) - 98
         PRINT *, I, V(F(I) - 100)
   10 CONTINUE
      CALL S(4)
      PRINT *, F(F(F(0)))
      PRINT *, CUAD(3.0), MEDIA(1.0, 4.0), DOBLE(3)
      IF (ESPAR(4)) PRINT *, 'PAR'
      IF (.NOT. ESPAR(X)) PRINT *, 'IMPAR'
      END
