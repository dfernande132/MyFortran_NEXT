C     READ: ENTRADA DIRIGIDA POR LISTA DESDE EL TECLADO
C     Teclea, por ejemplo:   3   4.5   si   'Hola mundo'   y despues dos enteros, uno por linea
      PROGRAM LEER
      INTEGER I, J, K
      REAL X
      LOGICAL P
      CHARACTER*12 T
      PRINT *, 'Entero, real, logico, texto:'
      READ *, I, X, P, T
      PRINT *, I, X, P, T
      PRINT *, 'Dos enteros (uno por linea):'
      READ *, J
      READ *, K
      PRINT *, J + K, J * K
      END
