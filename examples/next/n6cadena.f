C     CADENAS: concatenacion, subcadenas con indices variables, LEN, comparacion
C     (ICHAR, CHAR e INDEX aun no estan en el generador: se hace a mano)
      PROGRAM CADENA
      CHARACTER*20 S, R, U
      CHARACTER*1 C
      CHARACTER*6 A, B
      CHARACTER*26 MIN, MAY
      INTEGER I, N, V, K, POS
      LOGICAL PAL
      S = 'ANITA LAVA LA TINA'
      PRINT *, S, LEN(S)
      N = 20
   10 IF (S(N:N) .NE. ' ') GOTO 20
      N = N - 1
      GOTO 10
   20 PRINT *, 'LONGITUD UTIL', N
      R = ' '
      DO 30 I = 1, N
         R(I:I) = S(N - I + 1:N - I + 1)
   30 CONTINUE
      PRINT *, R
      PAL = .TRUE.
      I = 1
      K = N
   40 IF (I .GE. K) GOTO 60
      IF (S(I:I) .EQ. ' ') THEN
         I = I + 1
         GOTO 40
      END IF
      IF (S(K:K) .EQ. ' ') THEN
         K = K - 1
         GOTO 40
      END IF
      IF (S(I:I) .NE. S(K:K)) PAL = .FALSE.
      I = I + 1
      K = K - 1
      GOTO 40
   60 PRINT *, 'PALINDROMO', PAL
      MIN = 'abcdefghijklmnopqrstuvwxyz'
      MAY = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
      U = 'hola, Mundo 123'
      DO 70 I = 1, 20
         C = U(I:I)
         DO 65 K = 1, 26
            IF (C .EQ. MIN(K:K)) U(I:I) = MAY(K:K)
   65    CONTINUE
   70 CONTINUE
      PRINT *, U
      V = 0
      DO 80 I = 1, 20
         C = U(I:I)
         IF (C .EQ. 'A' .OR. C .EQ. 'E' .OR. C .EQ. 'I') V = V + 1
         IF (C .EQ. 'O' .OR. C .EQ. 'U') V = V + 1
   80 CONTINUE
      PRINT *, 'VOCALES', V
      A = 'AB'
      B = 'CD'
      PRINT *, A // B, A // '-' // B
      POS = 0
      DO 90 I = 1, N - 1
         IF (POS .EQ. 0 .AND. S(I:I + 1) .EQ. 'LA') POS = I
   90 CONTINUE
      PRINT *, 'LA EN', POS
      PRINT *, 'ABC' .LT. 'ABD', 'ABC' .EQ. 'ABC ', 'B' .GT. 'AZ'
      PRINT *, S(7:10), S(1:1), S(N - 3:N)
      END
