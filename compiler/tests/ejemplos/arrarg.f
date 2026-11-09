C     S3B: ARRAYS COMO ARGUMENTO (TAMANO FIJO, AJUSTABLE Y SUPUESTO)
      PROGRAM ARRARG
      INTEGER V(6), W(4), DOT
      V(1) = 5
      V(2) = 3
      V(3) = 9
      V(4) = 1
      V(5) = 7
      V(6) = 2
      CALL SORT(V, 6)
      PRINT *, V(1), V(2), V(3), V(4), V(5), V(6)
      CALL REV(V, 6)
      PRINT *, V(1), V(2), V(3), V(4), V(5), V(6)
C     UN ELEMENTO: A UN FORMAL ESCALAR SE PASA EL VALOR,
C     A UN FORMAL ARRAY, EL TROZO QUE EMPIEZA AHI
      CALL DOBLA(V(2))
      CALL SORT(V(3), 4)
      PRINT *, V(1), V(2), V(3), V(4), V(5), V(6)
      DO 10 I = 1, 4
        W(I) = I
   10 CONTINUE
      PRINT *, DOT(W, W, 4), DOT(W, V(5), 2)
      END
      SUBROUTINE SORT(A, N)
      INTEGER A(N), T
      DO 20 I = 1, N - 1
        DO 10 J = 1, N - I
          IF (A(J) .GT. A(J + 1)) THEN
            T = A(J)
            A(J) = A(J + 1)
            A(J + 1) = T
          END IF
   10   CONTINUE
   20 CONTINUE
      END
      SUBROUTINE REV(A, N)
      INTEGER A(*)
      DO 10 I = 1, N / 2
        K = A(I)
        A(I) = A(N + 1 - I)
        A(N + 1 - I) = K
   10 CONTINUE
      END
      SUBROUTINE DOBLA(K)
      INTEGER K
      K = K * 2
      END
      INTEGER FUNCTION DOT(A, B, N)
      INTEGER A(N), B(N)
      DOT = 0
      DO 10 I = 1, N
        DOT = DOT + A(I) * B(I)
   10 CONTINUE
      END
