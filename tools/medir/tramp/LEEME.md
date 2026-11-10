# Arnes del trampolin generico (0.11)

Comprueba `tools/spike/e1_tramp.asm` (`__tramp` + resguardos `t_*`) sin hardware: ensambla la parte generica con pasmo y la ejecuta
en libz80ex con `_mapear` enganchado. Cubre argumentos intactos, IX/IY preservados, banco dentro/fuera, anidamiento hasta 39 niveles,
retorno de 32 bits (DEHL) y pila restaurada (177 pruebas).

Requiere Linux con `pasmo`, `gcc` y `libz80ex`. Ejecutar `./regen.sh` tras tocar `e1_tramp.asm`. NO es parte de `PC: tests`.
Limite: la pila de niveles del trampolin son 40 niveles x 3 B sin guarda; no hay comprobacion de desbordamiento en el Next.
