C     SUBPROGRAMAS: funciones, subrutinas, vectores como argumentos,
C     funcion-sentencia, EXTERNAL y argumentos por referencia
      PROGRAM SUBPRG
      INTEGER MCD, FACT, FIB, CUAD, I, N, V(5), MAYOR
      EXTERNAL MCD
      CUAD(N) = N * N
      PRINT *, 'MCD', MCD(48, 18), MCD(17, 5), MCD(100, 75), MCD(7, 7)
      DO 10 I = 0, 7
         PRINT *, I, FACT(I), FIB(I + 10), CUAD(I)
   10 CONTINUE
      DO 20 I = 1, 5
         V(I) = CUAD(I) - 7
   20 CONTINUE
      CALL SUMAR(V, 5, N)
      PRINT *, 'SUMA', N, 'MAYOR', MAYOR(V, 5)
      CALL ESCALAR(V, 5, 3)
      PRINT *, V(1), V(2), V(3), V(4), V(5)
      I = 4
      N = 9
      CALL CAMBIA(I, N)
      PRINT *, I, N
      CALL CAMBIA(I + 0, N)
      PRINT *, I, N
      END
      INTEGER FUNCTION MCD(A, B)
      INTEGER A, B, X, Y, T
      X = A
      Y = B
   10 IF (Y .EQ. 0) GOTO 20
      T = MOD(X, Y)
      X = Y
      Y = T
      GOTO 10
   20 MCD = X
      RETURN
      END
      INTEGER FUNCTION FACT(N)
      INTEGER N, I
      FACT = 1
      DO 10 I = 2, N
         FACT = FACT * I
   10 CONTINUE
      END
      INTEGER FUNCTION FIB(N)
      INTEGER N, A, B, T, I
      A = 0
      B = 1
      DO 10 I = 1, N
         T = A + B
         A = B
         B = T
   10 CONTINUE
      FIB = A
      END
      INTEGER FUNCTION MAYOR(V, N)
      INTEGER N, V(N), I
      MAYOR = V(1)
      DO 10 I = 2, N
         IF (V(I) .GT. MAYOR) MAYOR = V(I)
   10 CONTINUE
      END
      SUBROUTINE SUMAR(V, N, S)
      INTEGER N, V(N), S, I
      S = 0
      DO 10 I = 1, N
         S = S + V(I)
   10 CONTINUE
      END
      SUBROUTINE ESCALAR(V, N, K)
      INTEGER N, V(N), K, I
      DO 10 I = 1, N
         V(I) = V(I) * K
   10 CONTINUE
      END
      SUBROUTINE CAMBIA(A, B)
      INTEGER A, B, T
      T = A
      A = B
      B = T
      END
