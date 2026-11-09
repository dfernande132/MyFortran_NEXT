C     S4A: DIMENSION (ANTES Y DESPUES DEL TIPO), EXTERNAL E INTRINSIC
      PROGRAM DIMEN
      INTEGER A
      REAL B
      DIMENSION A(5), B(2,3), C(4)
      DIMENSION M(3)
      INTEGER M
      CHARACTER*3 S
      DIMENSION S(2)
      INTEGER TRIPLE
      EXTERNAL TRIPLE
      INTRINSIC SQRT
      DO 10 I = 1, 5
         A(I) = I * I
   10 CONTINUE
      DO 20 I = 1, 2
         DO 20 J = 1, 3
            B(I, J) = I + J / 10.0
   20 CONTINUE
      DO 30 I = 1, 4
         C(I) = SQRT(I * 1.0)
   30 CONTINUE
      M(1) = A(2)
      M(2) = A(5)
      M(3) = TRIPLE(M(2))
      S(1) = 'ab'
      S(2) = 'cde'
      PRINT *, A(1), A(3), A(5)
      PRINT *, B(1, 1), B(2, 3)
      PRINT *, C(1), C(4)
      PRINT *, M(1), M(2), M(3)
      PRINT *, S(1), S(2)
      END

      INTEGER FUNCTION TRIPLE(K)
      TRIPLE = 3 * K
      END
