C     FIBONACCI CON ENTEROS DE 16 BITS (EL MAYOR QUE CABE ES F(23)=28657)
      PROGRAM FIBO
      INTEGER F(23), I
      F(1) = 1
      F(2) = 1
      DO 10 I = 3, 23
         F(I) = F(I - 1) + F(I - 2)
   10 CONTINUE
      PRINT *, 'FIBONACCI'
      DO 20 I = 1, 23
         PRINT *, I, F(I)
   20 CONTINUE
      END
