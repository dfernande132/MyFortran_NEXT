C     ESTRES DE PILA: 4 FUNCIONES-SENTENCIA ENCADENADAS
      PROGRAM Q04
      INTEGER I, R
      JA1(K) = K + 1
      JA2(K) = JA1(K) + 1
      JA3(K) = JA2(K) + 1
      JA4(K) = JA3(K) + 1
      I = 1
      R = JA4(I)
      PRINT *, R
      END
