C     S3a: ARGUMENTOS CHARACTER ESCALARES (COPIA DE ENTRADA Y DE SALIDA
C     CON LA LONGITUD DEL FORMAL)
      PROGRAM ARGCH
      CHARACTER*5 A
      CHARACTER*4 T(3)
      CHARACTER*8 R, JOIN
      CHARACTER*3 UP
      A = 'abcde'
      CALL PONZZ(A)
      PRINT *, A
      CALL CORTO(A)
      PRINT *, A
      T(1) = 'aaaa'
      T(2) = 'bbbb'
      T(3) = 'cccc'
      K = 2
      CALL ADMIRA(T(K))
      CALL ADMIRA(T(K+1))
      PRINT *, T(1), T(2), T(3)
      CALL MUESTRA('lit')
      CALL MUESTRA(A // 'XY')
      R = JOIN('ab', 'cd')
      PRINT *, R
      PRINT *, UP(UP('abc'))
      CALL EXT(T(1))
      PRINT *, T(1)
      END
      SUBROUTINE PONZZ(X)
      CHARACTER*5 X
      X(2:3) = 'ZZ'
      END
      SUBROUTINE CORTO(X)
      CHARACTER*2 X
      X = 'q'
      END
      SUBROUTINE ADMIRA(S)
      CHARACTER*2 S
      S(2:2) = '!'
      END
      SUBROUTINE MUESTRA(S)
      CHARACTER*3 S
      PRINT *, '[', S, ']'
      END
      CHARACTER*8 FUNCTION JOIN(A, B)
      CHARACTER*2 A, B
      JOIN = A // '-' // B
      END
      CHARACTER*3 FUNCTION UP(S)
      CHARACTER*3 S
      UP = S
      UP(1:1) = '#'
      END
      SUBROUTINE EXT(S)
      CHARACTER*4 S
      CALL INT1(S)
      S(1:1) = '<'
      END
      SUBROUTINE INT1(T)
      CHARACTER*4 T
      T(4:4) = '>'
      END
