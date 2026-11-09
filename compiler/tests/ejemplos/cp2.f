      CHARACTER*4 T(3)
      T(1) = 'aaaa'
      T(2) = 'bbbb'
      T(3) = 'cccc'
      K = 2
      CALL F(T(K))
      CALL F(T(K+1))
      PRINT *, T(1), T(2), T(3)
      END
      SUBROUTINE F(S)
      CHARACTER*2 S
      S(2:2) = '!'
      END
