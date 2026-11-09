C     PRUEBA DEL EDITOR DE MYFORTRAN
C     CALCULA LOS PRIMEROS TERMINOS DE FIBONACCI
      PROGRAM FIBO
      INTEGER N, I, A, B, T
      N = 10
      A = 0
      B = 1
      DO 10 I = 1, N
         PRINT *, I, A
         T = A + B
         A = B
         B = T
   10 CONTINUE
      IF (N .GT. 5) THEN
         PRINT *, 'N ES MAYOR QUE 5'
      ELSE
         PRINT *, 'N ES PEQUENO'
      END IF
      X = 1.0 + 2.0 * 3.0 - 4.0 / 5.0 + 6.0 ** 2 + 7.0 + 8.0 + 9.0 +
     +    10.0 + 11.0
		PRINT *, 'DOS TAB'
			PRINT *, 'TRES TAB'
      END
