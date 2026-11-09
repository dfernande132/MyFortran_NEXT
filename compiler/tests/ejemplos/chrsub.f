C     S3D: SUBCADENAS COMO ARGUMENTO REAL (SE PASAN POR REFERENCIA:
C     LO QUE EL SUBPROGRAMA ESCRIBE VUELVE A LA VARIABLE)
      PROGRAM CHRSUB
      CHARACTER*8 A
      CHARACTER*4 T(2)
      A = 'abcdefgh'
      CALL FIJA(A(2:3))
      PRINT *, A
      I = 5
      J = 8
      CALL ABRE(A(I:J))
      PRINT *, A
      T(1) = 'wxyz'
      T(2) = 'mnop'
      K = 2
      CALL FIJA(T(K)(1:2))
      CALL FIJA(T(1)(3:4))
      PRINT *, T(1), T(2)
      DO 10 I = 1, 3
        CALL TODO(A(I:I+2))
   10 CONTINUE
      PRINT *, A
      CALL SW(A(1:2), A(7:8))
      PRINT *, A
      END
      SUBROUTINE FIJA(X)
      CHARACTER*2 X
      X = '**'
      END
      SUBROUTINE ABRE(X)
      CHARACTER*4 X
      X(1:1) = '<'
      X(4:4) = '>'
      END
      SUBROUTINE TODO(X)
      CHARACTER*(*) X
      PRINT *, LEN(X), X
      X(1:1) = '#'
      END
      SUBROUTINE SW(P, Q)
      CHARACTER*2 P, Q, T
      T = P
      P = Q
      Q = T
      END
