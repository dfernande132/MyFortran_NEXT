C     ESTRES DE PILA: 8 FUNCIONES-SENTENCIA ENCADENADAS
      PROGRAM Q08
      INTEGER I, R
      JA1(K) = K + 1
      JA2(K) = JA1(K) + 1
      JA3(K) = JA2(K) + 1
      JA4(K) = JA3(K) + 1
      JA5(K) = JA4(K) + 1
      JA6(K) = JA5(K) + 1
      JA7(K) = JA6(K) + 1
      JA8(K) = JA7(K) + 1
      I = 1
      R = JA8(I)
      PRINT *, R
      END
