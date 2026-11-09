C     LOGICOS: tabla de verdad, precedencia y relacionales
      PROGRAM LOGICA
      LOGICAL P, Q, R
      INTEGER I, J, K, N
      DO 10 I = 0, 1
         DO 10 J = 0, 1
            P = I .EQ. 1
            Q = J .EQ. 1
            PRINT *, P, Q, P .AND. Q, P .OR. Q, .NOT. P
            PRINT *, P .EQV. Q, P .NEQV. Q, .NOT. P .AND. Q
   10 CONTINUE
      P = .TRUE.
      Q = .FALSE.
      R = .TRUE.
      PRINT *, P .OR. Q .AND. R, (P .OR. Q) .AND. .NOT. R
      PRINT *, .NOT. Q .OR. Q, .NOT. (Q .OR. P)
      PRINT *, P .EQV. Q .OR. R, P .NEQV. Q .AND. R
      N = 0
      DO 20 I = 1, 3
         DO 20 J = 1, 3
            DO 20 K = 1, 3
               IF (I .LT. J .AND. J .LT. K) N = N + 100
               IF (I .EQ. J .OR. J .EQ. K .OR. I .EQ. K) N = N + 1
   20 CONTINUE
      PRINT *, 'N', N
      PRINT *, 3 .LT. 4, 3 .LE. 3, 3 .GT. 4
      PRINT *, 3 .GE. 4, 3 .EQ. 3, 3 .NE. 3
      PRINT *, -3 .LT. 2, 2 * 3 .EQ. 6
      PRINT *, 7 / 2 .EQ. 3, MOD(7, 2) .EQ. 1
      END
