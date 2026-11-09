C     S3C: CHARACTER*(*) (LA LONGITUD ES LA DEL ARGUMENTO REAL)
      PROGRAM CHRAST
      CHARACTER*5 A
      CHARACTER*3 B
      CHARACTER*4 T(2)
      A = 'hello'
      B = 'abc'
      T(1) = 'wxyz'
      T(2) = 'mnop'
      CALL MUESTRA(A)
      CALL MUESTRA(B)
      CALL MUESTRA('xy')
      CALL MUESTRA(T(2))
      CALL MUESTRA(A // B)
      CALL MARCA(A)
      CALL MARCA(T(1))
      PRINT *, A, T(1)
      PRINT *, LARGO(A), LARGO('ab'), LARGO(A(2:4)), LARGO(T(1)(1:2))
      CALL PASA(B)
      PRINT *, B
      END
      SUBROUTINE MUESTRA(S)
      CHARACTER*(*) S
      PRINT *, '[', S, ']', LEN(S)
      END
      SUBROUTINE MARCA(S)
      CHARACTER*(*) S
      S(1:1) = '#'
      S(LEN(S):LEN(S)) = '$'
      END
      INTEGER FUNCTION LARGO(S)
      CHARACTER*(*) S
      LARGO = LEN(S) * 10
      END
      SUBROUTINE PASA(S)
      CHARACTER*(*) S
      IF (S .EQ. 'abc') PRINT *, 'igual'
      CALL FIJA(S)
      END
      SUBROUTINE FIJA(X)
      CHARACTER*2 X
      X(2:2) = '+'
      END
