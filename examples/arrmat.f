C     S3B/S3C: MATRICES COMO ARGUMENTO (PRIMERA DIMENSION AJUSTABLE),
C     COLUMNAS Y ARRAYS DE REALES, LOGICOS Y CADENAS
      PROGRAM ARRMAT
      INTEGER A(2, 2), B(2, 2), C(2, 2), M(3, 3)
      REAL R(3)
      LOGICAL L(3)
      CHARACTER*3 T(3)
      A(1, 1) = 1
      A(2, 1) = 2
      A(1, 2) = 3
      A(2, 2) = 4
      B(1, 1) = 5
      B(2, 1) = 6
      B(1, 2) = 7
      B(2, 2) = 8
      CALL MUL(A, B, C, 2)
      PRINT *, C(1, 1), C(1, 2), C(2, 1), C(2, 2)
      DO 10 J = 1, 3
        DO 10 I = 1, 3
          M(I, J) = 10 * I + J
   10 CONTINUE
      CALL TRAZA(M, 3)
      J = 2
      CALL CERO(M(1, J), 3)
      PRINT *, M(1, 1), M(2, 1), M(1, 2), M(2, 2), M(3, 2), M(1, 3)
      R(1) = 1.5
      R(2) = 2.5
      R(3) = 4.0
      PRINT *, SUMA(R, 3)
      L(1) = .TRUE.
      L(2) = .FALSE.
      L(3) = .TRUE.
      CALL CUENTA(L, 3, N)
      PRINT *, N
      T(1) = 'uno'
      T(2) = 'dos'
      T(3) = 'tre'
      CALL CAMBIA(T, 3)
      PRINT *, T(1), T(2), T(3)
      END
      SUBROUTINE MUL(A, B, C, N)
      INTEGER N, A(N, N), B(N, N), C(N, N)
      DO 10 I = 1, N
        DO 10 J = 1, N
          C(I, J) = 0
          DO 10 K = 1, N
            C(I, J) = C(I, J) + A(I, K) * B(K, J)
   10 CONTINUE
      END
      SUBROUTINE TRAZA(A, N)
      INTEGER N, A(N, *), S
      S = 0
      DO 10 I = 1, N
        S = S + A(I, I)
   10 CONTINUE
      PRINT *, S
      END
      SUBROUTINE CERO(A, N)
      INTEGER N, A(N)
      DO 10 I = 1, N
        A(I) = 0
   10 CONTINUE
      END
      REAL FUNCTION SUMA(A, N)
      INTEGER N
      REAL A(N)
      SUMA = 0.0
      DO 10 I = 1, N
        SUMA = SUMA + A(I)
   10 CONTINUE
      END
      SUBROUTINE CUENTA(B, N, K)
      INTEGER N, K
      LOGICAL B(N)
      K = 0
      DO 10 I = 1, N
        IF (B(I)) K = K + 1
   10 CONTINUE
      END
      SUBROUTINE CAMBIA(A, N)
      INTEGER N
      CHARACTER*3 A(N), W
      W = A(1)
      A(1) = A(N)
      A(N) = W
      A(2)(1:1) = '_'
      END
