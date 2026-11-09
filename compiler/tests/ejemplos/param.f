C     PARAMETER: CONSTANTES CON NOMBRE (ENTERAS PARA DIMENSIONAR)
      PROGRAM PARAM
      CHARACTER*4 SALUDO
      LOGICAL SI
      PARAMETER (N = 4, M = N * 2 + 1)
      PARAMETER (PI = 3.14159, SALUDO = 'hola', SI = .TRUE.)
      INTEGER V(M)
      DO 10 I = 1, M
         V(I) = I * N
   10 CONTINUE
      PRINT *, N, M
      PRINT *, V(1), V(M)
      PRINT *, PI * 2.0
      PRINT *, SALUDO
      IF (SI) PRINT *, 'cierto'
      END
