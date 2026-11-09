      CHARACTER*5 A
      A = 'abcde'
      CALL S(A)
      PRINT *, A
      END
      SUBROUTINE S(X)
      CHARACTER*5 X
      X(2:3) = 'ZZ'
      END
