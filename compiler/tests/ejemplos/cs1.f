      CHARACTER*8 S
      CHARACTER*4 A(3)
      INTEGER I
      S = 'ABCDEFGH'
      S(2:4) = 'xy'
      PRINT *, S
      S(7:8) = 'LONGER'
      PRINT *, S
      S(:3) = '123'
      S(6:) = 'zz'
      PRINT *, S
      I = 3
      S(I:I+1) = '++'
      PRINT *, S
      A(2) = 'WXYZ'
      A(2)(2:3) = '--'
      I = 2
      A(I)(4:) = '!'
      PRINT *, A(2)
      END
