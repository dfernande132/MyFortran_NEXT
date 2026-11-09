C     READ: cada READ consume una linea; enteros, reales, logicos y cadenas.
C     Cada READ va precedido de un PRINT que dice que se espera (en el Next la
C     pantalla estaria en blanco hasta teclear).
      PROGRAM ENTRA
      INTEGER N, I, S, MAY, MENOR, V(10)
      REAL X, Y, T
      LOGICAL B, C
      CHARACTER*8 NOM
      CHARACTER*3 K
      PRINT *, 'CUANTOS ENTEROS?'
      READ *, N
      S = 0
      MAY = -32767
      MENOR = 32767
      DO 10 I = 1, N
         PRINT *, 'VALOR', I
         READ *, V(I)
         S = S + V(I)
         IF (V(I) .GT. MAY) MAY = V(I)
         IF (V(I) .LT. MENOR) MENOR = V(I)
   10 CONTINUE
      PRINT *, 'N', N, 'SUMA', S, 'MAX', MAY, 'MIN', MENOR
      PRINT *, 'DOS REALES:'
      READ *, X, Y
      T = X * Y
      PRINT *, X, Y, T, X + Y
      PRINT *, 'DOS LOGICOS:'
      READ *, B, C
      PRINT *, B, C, B .AND. C, B .OR. C
      PRINT *, 'DOS CADENAS:'
      READ *, NOM, K
      PRINT *, NOM, K
      PRINT *, 'ENTERO REAL CADENA:'
      READ *, I, X, NOM
      PRINT *, I, X, NOM
      PRINT *, 'ULTIMO ENTERO:'
      READ *, N
      PRINT *, N, -N
      END
