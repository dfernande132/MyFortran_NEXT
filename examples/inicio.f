C     +---------------------------------------------------------+
C     |  #   # #   # #####  ###  ####  ##### ####   ###  #   #  |
C     |  ## ##  # #  #     #   # #   #   #   #   # #   # ##  #  |
C     |  # # #   #   ####  #   # ####    #   ####  ##### # # #  |
C     |  #   #   #   #     #   # #  #    #   #  #  #   # #  ##  |
C     |  #   #   #   #      ###  #   #   #   #   # #   # #   #  |
C     +---------------------------------------------------------+
C
C     MYFORTRAN 0.1 - FORTRAN 77 FOR THE ZX SPECTRUM NEXT
C     (C) 2026 DFSANTOS
C
C     ARROWS MOVE THE CURSOR      DELETE ERASES TO THE LEFT
C     ESC, THEN ONE KEY:
C       H / E    START / END OF LINE
C       T / B    START / END OF TEXT
C       U / D    PAGE UP / PAGE DOWN
C       G        GO TO LINE            K   DELETE LINE
C       C        CHECK THE PROGRAM AND SHOW THE FIRST ERROR
C       X / Z    NEXT / PREVIOUS ERROR
C       O        BUILD THE .NEX FILE (NAME OF THE SOURCE + .NEX)
C       N L S A  NEW / LOAD / SAVE / SAVE AS
C
C     FIXED FORMAT: LABELS IN COLUMNS 1-5, CONTINUATION IN 6,
C     CODE IN COLUMNS 7-72. THE PROGRAM BELOW IS READY: ESC, THEN O.
      PROGRAM INICIO
      PRINT *, 'HELLO FROM MYFORTRAN'
      END
