C     DATA: VALORES INICIALES (UNA VEZ, AL ARRANCAR)
      PROGRAM DATOS
      INTEGER A(4), K, L
      REAL X, Y(2)
      CHARACTER*5 NOM, T(2)
      LOGICAL P
      DATA A /1, 2, 3, 4/
      DATA K, L /10, -20/
      DATA X /2.5/, Y /0.5, 1.5/
      DATA NOM /'Ana'/, T /'uno', 'dos'/
      DATA P /.TRUE./
      PRINT *, A(1), A(2), A(3), A(4)
      PRINT *, K, L
      PRINT *, X, Y(1), Y(2)
      PRINT *, NOM, T(1), T(2)
      PRINT *, P
      CALL CUENTA
      CALL CUENTA
      CALL CUENTA
      END

      SUBROUTINE CUENTA
      INTEGER N
      DATA N /100/
      N = N + 1
      PRINT *, N
      END
