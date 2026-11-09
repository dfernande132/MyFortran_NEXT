C     CONTROL: IF de bloque, IF aritmetico, DO con paso negativo y cero vueltas,
C     DO anidados con etiqueta compartida, GOTO calculado, salida de un DO, STOP
      PROGRAM CONTROL
      INTEGER I, J, K, N, S
      DO 10 I = -2, 12, 3
         IF (I .LT. 0) THEN
            PRINT *, I, 'NEGATIVO'
         ELSE IF (I .EQ. 0) THEN
            PRINT *, I, 'CERO'
         ELSE IF (I .LT. 7) THEN
            PRINT *, I, 'PEQUENO'
         ELSE
            PRINT *, I, 'GRANDE'
         END IF
   10 CONTINUE
      S = 0
      DO 20 I = 10, 1, -3
         S = S + I
   20 CONTINUE
      PRINT *, 'PASO NEGATIVO', S, I
      DO 30 I = 5, 1
         S = 999
   30 CONTINUE
      PRINT *, 'CERO VUELTAS', S, I
      N = 0
      DO 40 I = 1, 4
         DO 40 J = 1, I
            N = N + I * J
   40 CONTINUE
      PRINT *, 'ANIDADOS', N
      DO 60 K = -1, 1
         IF (K) 51, 52, 53
   51    PRINT *, K, 'MENOR'
         GOTO 60
   52    PRINT *, K, 'IGUAL'
         GOTO 60
   53    PRINT *, K, 'MAYOR'
   60 CONTINUE
      DO 80 K = 1, 4
         GOTO (71, 72, 73), K
         PRINT *, K, 'FUERA DE RANGO'
         GOTO 80
   71    PRINT *, K, 'UNO'
         GOTO 80
   72    PRINT *, K, 'DOS'
         GOTO 80
   73    PRINT *, K, 'TRES'
   80 CONTINUE
      N = 0
      DO 90 I = 1, 100
         N = N + I
         IF (N .GT. 50) GOTO 95
   90 CONTINUE
   95 PRINT *, 'SALIDA', I, N
      IF (N .GT. 0) THEN
         IF (N .GT. 60) THEN
            PRINT *, 'ALTO'
         ELSE
            IF (MOD(N, 2) .EQ. 0) PRINT *, 'PAR'
            IF (MOD(N, 2) .NE. 0) PRINT *, 'IMPAR'
         END IF
      END IF
      STOP
      PRINT *, 'NO SE IMPRIME'
      END
