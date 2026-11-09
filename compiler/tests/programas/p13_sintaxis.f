* ASTERISCO COMO COMENTARIO
C     SINTAXIS: blancos sin significado, continuaciones, GO TO, etiquetas,
C     mayusculas y varias sentencias parecidas a palabras clave
      PROGRAM SINTAX
      INTEGER DO10I, IF, I, J, K, REAL1
      INTEGER LARGA
      REAL END1
      DO10I = 5
      PRINT *, DO10I
      IF = 3
      PRINT *, IF + 1
      REAL1 = 7
      END1 = 2.5
      PRINT *, REAL1, END1
      DO 10 I = 1,3
   10 PRINT *, I
      DO20I=1,2
      DO 20 J = 1, 2
   20 CONTINUE
      PRINT *, J
      GO TO 30
      PRINT *, 'SALTADO'
   30 PRINT *, 'ETIQUETA'

      K = 1 +
     +    2 +
     +    3 +
     +    4
      PRINT *, K
      LARGA = 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13
     + + 14 + 15 + 16 + 17 + 18 + 19 + 20 + 21 + 22 + 23 + 24 + 25
      PRINT *, LARGA
      PRINT *, 'UNA CADENA CON '' COMILLA Y CON  DOS ESPACIOS'
      PRINT *, 'C', 'DO 10 I = 1, 3', 'IF (X) 1, 2, 3'
      IF (K .GT. 5) PRINT *, 'K', K
      I = 0
   40 I = I + 1
      IF (I .LT. 3) GOTO 40
      PRINT *, I
      PRINT *
      PRINT *, 'FIN'
      END
