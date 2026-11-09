      CHARACTER*8 R
      CHARACTER*8 JOIN
      R = JOIN('ab', 'cd')
      PRINT *, R
      END
      CHARACTER*8 FUNCTION JOIN(A, B)
      CHARACTER*2 A, B
      JOIN = A // '-' // B
      END
