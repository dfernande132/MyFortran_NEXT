C Subprogramas, etapa S2b: CHARACTER*n FUNCTION.
      CHARACTER*4 FUNCTION NOMBRE(N)
      INTEGER N
      IF (N .EQ. 1) THEN
         NOMBRE = 'UNO'
      ELSE IF (N .EQ. 2) THEN
         NOMBRE = 'DOS'
      ELSE
         NOMBRE = 'MUCHOS'
      END IF
      END
      CHARACTER*6 FUNCTION GUARDA(N)
      INTEGER N
      CHARACTER*6 P, A
      P = '......'
      A = '######'
      GUARDA = A(1:N) // P(1:6-N)
      END
      CHARACTER*3 FUNCTION CORTA(N)
      INTEGER N
      CHARACTER*6 GUARDA
      CORTA = GUARDA(N)
      END
      FUNCTION C5(K)
      CHARACTER C5*5
      C5 = 'PQ'
      END
      PROGRAM PRUEBA
      CHARACTER*4 NOMBRE
      CHARACTER*6 GUARDA
      CHARACTER*3 CORTA
      CHARACTER C5*5
      CHARACTER*12 S
      INTEGER I
      DO 10 I = 1, 3
         PRINT *, NOMBRE(I)
   10 CONTINUE
      S = NOMBRE(1) // '+' // NOMBRE(2)
      PRINT *, S
      IF (NOMBRE(2) .EQ. 'DOS') PRINT *, 'IGUAL'
      IF (NOMBRE(1) .GT. NOMBRE(2)) PRINT *, 'MAYOR'
      PRINT *, NOMBRE(3) // '|'
      PRINT *, LEN(NOMBRE(1))
      PRINT *, GUARDA(1), GUARDA(3), GUARDA(5)
      PRINT *, CORTA(4), CORTA(1)
      PRINT *, C5(1), '|'
      END
