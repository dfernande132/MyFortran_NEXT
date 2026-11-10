#!/bin/bash
cd "$(dirname "$0")"
python3 - <<'PY'
import re
s=open('../spike/e1_tramp.asm').read()
gen=s.split('; ---- resguardos')[0]
open('tramp_gen.asm','w').write('\n'.join(l for l in gen.split('\n') if not re.match(r'\s*(PUBLIC|EXTERN|SECTION)\b',l)))
PY
pasmo --bin prueba.asm prueba.bin prueba.sym && gcc -O1 -Wall -o arnes arnes.c -lz80ex && ./arnes
