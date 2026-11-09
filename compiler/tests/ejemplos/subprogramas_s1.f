C Subprogramas, etapa S1: SUBROUTINE, CALL, RETURN, argumentos por referencia.
C El programa principal esta en medio; los subprogramas, antes y despues.
      SUBROUTINE INTERC(A, B)
      INTEGER A, B, T
      T = A
      A = B
      B = T
      END
      PROGRAM PRUEBA
      INTEGER X, Y, N, S
      REAL R, Q
      LOGICAL F
      X = 3
      Y = 8
      CALL INTERC(X, Y)
      PRINT *, 'INTERC', X, Y
C una constante se pasa por valor: el subprograma puede cambiarla
      CALL INC(7, N)
      PRINT *, 'INC', N
C una expresion tambien
      CALL INC(X * 2 + 1, N)
      PRINT *, 'INC', N
      R = 1.5
      Q = 0.25
      CALL MEDIA(R, Q)
      PRINT *, 'MEDIA', R, Q
      F = .FALSE.
      CALL NEGAR(F)
      PRINT *, 'NEGAR', F
      S = 0
      DO 10 N = 1, 5
         CALL ACUM(S, N)
   10 CONTINUE
      PRINT *, 'ACUM', S
      CALL SALIR(S)
      PRINT *, 'NO SE IMPRIME'
      END
      SUBROUTINE INC(K, R)
      INTEGER K, R
      K = K + 1
      R = K
      END
      SUBROUTINE MEDIA(A, B)
      REAL A, B
      A = (A + B) * 0.5
      B = A + 1.0
      END
      SUBROUTINE NEGAR(L)
      LOGICAL L
      L = .NOT. L
      END
      SUBROUTINE ACUM(S, I)
      INTEGER S, I
      IF (MOD(I, 2) .EQ. 0) RETURN
      S = S + I * 10
      END
      SUBROUTINE SALIR(S)
      INTEGER S
      IF (S .GT. 50) THEN
         PRINT *, 'SALIR', S
         STOP
      END IF
      PRINT *, 'SEGUIR'
      END
