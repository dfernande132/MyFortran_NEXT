C     PARAMETER, DATA, DIMENSION, CHARACTER: valores iniciales y tamanos con constantes
      PROGRAM DATOS
      INTEGER N, M
      PARAMETER (N = 5, M = N * 2 + 1)
      REAL PI, DOS
      PARAMETER (PI = 3.140625, DOS = 2.0)
      INTEGER V(N), W(M), Z(3), I, S
      DIMENSION T(4)
      CHARACTER*4 NOM(3)
      LOGICAL F(2)
      DATA V / 1, 2, 3, 4, 5 /
      DATA Z / 3 * 7 /
      DATA W / 11 * 0 /
      DATA T / 0.5, 1.5, 2.5, 3.5 /
      DATA NOM / 'UNO', 'DOS', 'TRES' /
      DATA F / .TRUE., .FALSE. /
      DATA I, S / 100, -1 /
      PRINT *, N, M, PI, DOS, PI * DOS
      S = 0
      DO 10 I = 1, N
         S = S + V(I)
   10 CONTINUE
      PRINT *, 'SUMA V', S, Z(1), Z(2), Z(3)
      DO 20 I = 1, M
         W(I) = W(I) + I * I
   20 CONTINUE
      PRINT *, W(1), W(5), W(M), M
      PRINT *, T(1) + T(4), T(2) * T(3)
      DO 30 I = 1, 3
         PRINT *, I, NOM(I)
   30 CONTINUE
      PRINT *, F(1), F(2)
      DATA Z(2) / 99 /
      PRINT *, Z(1), Z(2), I
      END
