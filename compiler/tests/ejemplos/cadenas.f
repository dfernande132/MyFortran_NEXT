C     CADENAS: CHARACTER, concatenacion, subcadenas y comparacion
      PROGRAM CADENA
      CHARACTER*12 SAL
      CHARACTER*5 NOM(3)
      INTEGER I
      NOM(1) = 'ANA'
      NOM(2) = 'LUIS'
      NOM(3) = 'MARIA'
      DO 10 I = 1, 3
         SAL = 'HOLA ' // NOM(I)
         PRINT *, SAL
         IF (NOM(I) .GT. 'B') PRINT *, 'DESPUES DE B', NOM(I)(1:2)
   10 CONTINUE
      END
