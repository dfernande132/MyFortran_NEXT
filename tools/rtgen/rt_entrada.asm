; rt_entrada.asm - Entrada dirigida por lista (READ *, lista). Debe hacer lo mismo que platform/pc/tacvm.c (in_linea_nueva, in_token y TOP_RD).
; Bloque de trabajo RDD (se nombra $simbolo): la linea tecleada, el puntero de lectura y los registros del conversor REAL.
; Una linea son hasta 79 caracteres terminados en 0. Cada READ lee una linea nueva; los elementos se separan por blancos o comas.

.sym ln=0           ; 80 B: la linea
.sym lq=80          ; 2 B: direccion del siguiente caracter por leer
.sym nd=82          ; 1 B: 1 = hace falta una linea nueva
.sym sl=83          ; 1 B: se vio una '/' (el resto de la lista queda sin tocar)
.sym tp=84          ; 2 B: inicio del valor leido
.sym tl=86          ; 1 B: longitud del valor
.sym tq=87          ; 1 B: iba entre comillas
.sym dst=88         ; 2 B: destino del valor
.sym ra=90          ; 4 B: REAL acumulador
.sym rb=94          ; 4 B
.sym rc=98          ; 4 B
.sym ng=102         ; 1 B: signo negativo
.sym nz=103         ; 1 B: cifras vistas
.sym fp=104         ; 1 B: punto decimal visto
.sym dg=105         ; 1 B: cifra en curso
.sym ed=106         ; 1 B: 1 = el exponente decimal es negativo
.sym e10=107        ; 2 B: exponente decimal

.rut RDD
.data 109

; ---- teclado ----------------------------------------------------------------------------------------------------------------------
; Primera tecla (sin contar las mayusculas ni el simbolo) pulsada: A = 0 ninguna, o 1 + 5 * fila + bit
.rut KEYSC
LD B,0xFE
LD D,0
r1:
LD C,0xFE
IN A,(C)
CPL
AND 0x1F
JR Z,nx
LD E,A
LD A,B
CP 0xFE
JR NZ,m1
RES 0,E
m1:
CP 0x7F
JR NZ,m2
RES 1,E
m2:
LD A,E
OR A
JR NZ,fd
nx:
LD A,D
ADD A,5
LD D,A
RLC B
LD A,B
CP 0xFE
JR NZ,r1
XOR A
RET
fd:
LD A,D
f1:
SRL E
JR C,f2
INC A
JR f1
f2:
INC A
RET

; Espera una tecla y devuelve su caracter en A: letras en minuscula (mayusculas con CAPS SHIFT), simbolos con SYMBOL SHIFT,
; ENTER = 13, CAPS SHIFT + 0 = 8 (borrar)
.rut KEYH
w1:
CALL RT_KEYSC
OR A
JR NZ,w1
w2:
CALL RT_KEYSC
OR A
JR Z,w2
DEC A
LD E,A
LD D,0
LD BC,0xFEFE
IN A,(C)
AND 1
LD H,A
LD BC,0x7FFE
IN A,(C)
AND 2
LD B,A
LD C,H
LD HL,kt
ADD HL,DE
LD A,B
OR A
JR NZ,ns
PUSH HL
LD DE,40
ADD HL,DE
LD A,(HL)
POP HL
OR A
RET NZ
ns:
LD A,C
OR A
LD A,(HL)
RET NZ
CP 48
JR NZ,u1
LD A,8
RET
u1:
CP 97
RET C
CP 123
RET NC
SUB 32
RET
kt:
.db 0,122,120,99,118
.db 97,115,100,102,103
.db 113,119,101,114,116
.db 49,50,51,52,53
.db 48,57,56,55,54
.db 112,111,105,117,121
.db 13,108,107,106,104
.db 32,0,109,110,98
.db 0,58,96,63,47
.db 126,124,92,123,125
.db 0,0,0,60,62
.db 33,64,35,36,37
.db 95,41,40,39,38
.db 34,59,0,93,91
.db 0,61,43,45,94
.db 0,0,46,44,42

; Pruebas (PC): la "tecla" sale del texto que hay en $F002 (el puntero en $F000); 0 = no queda entrada
.rut KEYT
LD HL,(=0xF000)
LD A,(HL)
OR A
RET Z
INC HL
LD (=0xF000),HL
CP 10
RET NZ
LD A,13
RET

; ---- una linea ------------------------------------------------------------------------------------------------------------------
; Lee una linea con eco (8 = borrar) hasta ENTER. Sin entrada (solo en pruebas): error 7 si la linea esta vacia
.rut RDLINE
LD HL,$ln
LD B,0
rd:
PUSH HL
PUSH BC
CALL RT_KEY
POP BC
POP HL
OR A
JR Z,eof
CP 13
JR Z,fin
CP 8
JR Z,bs
CP 32
JR C,rd
CP 127
JR NC,rd
LD C,A
LD A,B
CP 79
JR NC,rd
LD A,C
LD (HL),A
INC HL
INC B
CALL RT_PUTC
JR rd
bs:
LD A,B
OR A
JR Z,rd
DEC HL
DEC B
LD A,8
CALL RT_PUTC
LD A,32
CALL RT_PUTC
LD A,8
CALL RT_PUTC
JR rd
eof:
LD A,B
OR A
LD A,7
JP Z,RT_ERR
fin:
LD A,13
CALL RT_PUTC
LD (HL),0
LD HL,$ln
LD ($lq),HL
XOR A
LD ($nd),A
RET

; Empieza una sentencia READ: linea nueva, aunque no tenga lista
.rut RDLN
XOR A
LD ($sl),A
JP RT_RDLINE

; ---- siguiente valor ------------------------------------------------------------------------------------------------------------
; Devuelve A = 0 y ($tp), ($tl), ($tq) con el valor, o A = 1 si es nulo (",," o tras una '/'). Los valores entre comillas se
; desdoblan en su sitio ('' -> '). Error 6: comilla sin cerrar; 7: no hay mas lineas
.rut RDTOK
LD A,($sl)
OR A
JR NZ,nul
sk:
LD A,($nd)
OR A
JR Z,hv
CALL RT_RDLINE
hv:
LD HL,($lq)
b1:
LD A,(HL)
CP 32
JR NZ,t1
INC HL
JR b1
t1:
OR A
JR NZ,t2
LD A,1
LD ($nd),A
JR sk
t2:
LD C,A
CP 44
JR NZ,t3
INC HL
LD ($lq),HL
nul:
LD A,1
RET
t3:
CP 47
JR NZ,t4
LD ($sl),A
LD A,1
RET
t4:
CP 39
JR Z,qt
CP 34
JR Z,qt
LD ($tp),HL
LD B,0
u1:
LD A,(HL)
OR A
JR Z,ue
CP 32
JR Z,ue
CP 44
JR Z,ue
CP 47
JR Z,ue
INC HL
INC B
JR u1
ue:
XOR A
LD ($tq),A
JR po
qt:
INC HL
LD ($tp),HL
LD D,H
LD E,L
LD B,0
q1:
LD A,(HL)
OR A
JR Z,bad
INC HL
CP C
JR NZ,q2
LD A,(HL)
CP C
JR NZ,qe
INC HL
LD A,C
q2:
LD (DE),A
INC DE
INC B
JR q1
qe:
LD A,1
LD ($tq),A
po:
LD A,B
LD ($tl),A
p1:
LD A,(HL)
CP 32
JR NZ,p2
INC HL
JR p1
p2:
CP 44
JR NZ,p3
INC HL
p3:
LD ($lq),HL
XOR A
RET
bad:
LD A,6
JP RT_ERR

; ---- INTEGER: HL = destino ----------------------------------------------------------------------------------------------------
.rut RDI
LD ($dst),HL
CALL RT_RDTOK
DEC A
RET Z
LD A,($tq)
OR A
JR NZ,bad
LD A,($tl)
LD B,A
OR A
JR Z,bad
LD HL,($tp)
LD C,0
LD A,(HL)
CP 43
JR Z,sg
CP 45
JR NZ,dg
INC C
sg:
INC HL
DEC B
JR Z,bad
dg:
LD DE,0
d1:
LD A,(HL)
SUB 48
CP 10
JR NC,bad
PUSH HL
LD H,D
LD L,E
ADD HL,HL
JR C,bad
LD D,H
LD E,L
ADD HL,HL
JR C,bad
ADD HL,HL
JR C,bad
ADD HL,DE
JR C,bad
LD E,A
LD D,0
ADD HL,DE
JR C,bad
LD A,H
CP 128
JR C,ok
JR NZ,bad
LD A,L
OR A
JR NZ,bad
ok:
EX DE,HL
POP HL
INC HL
DEC B
JR NZ,d1
LD A,C
OR A
JR Z,ps
LD A,E
CPL
LD E,A
LD A,D
CPL
LD D,A
INC DE
JR st
ps:
BIT 7,D
JR NZ,bad
st:
LD HL,($dst)
LD (HL),E
INC HL
LD (HL),D
RET
bad:
LD A,6
JP RT_ERR

; ---- LOGICAL: HL = destino (T o F, con o sin puntos; el resto del valor no cuenta) ---------------------------------------------
.rut RDL
LD ($dst),HL
CALL RT_RDTOK
DEC A
RET Z
LD A,($tq)
OR A
JR NZ,bad
LD A,($tl)
LD B,A
OR A
JR Z,bad
LD HL,($tp)
LD A,(HL)
CP 46
JR NZ,l1
INC HL
DEC B
JR Z,bad
LD A,(HL)
l1:
OR 32
LD DE,0
CP 116
JR Z,l2
CP 102
JR NZ,bad
JR l3
l2:
INC E
l3:
LD HL,($dst)
LD (HL),E
INC HL
LD (HL),D
RET
bad:
LD A,6
JP RT_ERR

; ---- CHARACTER: DE = donde van los caracteres, C = capacidad. Rellena con blancos ---------------------------------------------
.rut RDS
PUSH DE
PUSH BC
CALL RT_RDTOK
POP BC
POP DE
DEC A
RET Z
LD HL,($tp)
LD A,($tl)
LD B,A
s1:
LD A,C
OR A
RET Z
DEC C
LD A,B
OR A
JR Z,sb
DEC B
LD A,(HL)
INC HL
JR sw
sb:
LD A,32
sw:
LD (DE),A
INC DE
JR s1

; ---- REAL: HL = destino (4 bytes). Mantisa decimal acumulada como REAL (exacta hasta 7 cifras), despues * o / 10^exponente ---------
.rut RDF
JR go
bad:
LD A,6
JP RT_ERR
go:
LD ($dst),HL
CALL RT_RDTOK
DEC A
RET Z
LD A,($tq)
OR A
JP NZ,bad
LD A,($tl)
LD B,A
OR A
JP Z,bad
XOR A
LD ($ng),A
LD ($nz),A
LD ($fp),A
LD ($e10),A
LD ($e10+1),A
LD ($ra),A
LD ($ra+1),A
LD ($ra+2),A
LD ($ra+3),A
LD HL,($tp)
LD A,(HL)
CP 43
JR Z,p1
CP 45
JR NZ,lp
LD A,1
LD ($ng),A
p1:
INC HL
DEC B
JP Z,bad
lp:
LD A,(HL)
CP 46
JR NZ,ld
LD A,($fp)
OR A
JP NZ,bad
INC A
LD ($fp),A
JR nx
ld:
SUB 48
CP 10
JR NC,ex
PUSH HL
PUSH BC
LD ($dg),A
LD HL,10
LD BC,$rb
CALL RT_ITOF
LD HL,$ra
LD DE,$rb
LD BC,$ra
CALL RT_FMUL
LD A,($dg)
LD L,A
LD H,0
LD BC,$rc
CALL RT_ITOF
LD HL,$ra
LD DE,$rc
LD BC,$ra
XOR A
CALL RT_FADD
POP BC
POP HL
LD A,($nz)
INC A
LD ($nz),A
LD A,($fp)
OR A
JR Z,nx
PUSH HL
LD HL,($e10)
DEC HL
LD ($e10),HL
POP HL
nx:
INC HL
DEC B
JR NZ,lp
JR fn
ex:
LD A,(HL)
OR 32
CP 101
JR Z,e1
CP 100
JP NZ,bad
e1:
INC HL
DEC B
JP Z,bad
LD C,0
LD A,(HL)
CP 43
JR Z,e2
CP 45
JR NZ,e3
INC C
e2:
INC HL
DEC B
JP Z,bad
e3:
LD DE,0
e4:
LD A,(HL)
SUB 48
CP 10
JP NC,bad
PUSH HL
LD H,D
LD L,E
ADD HL,HL
LD D,H
LD E,L
ADD HL,HL
ADD HL,HL
ADD HL,DE
LD E,A
LD D,0
ADD HL,DE
EX DE,HL
POP HL
LD A,D
CP 16
JR C,e5
LD DE,4000
e5:
INC HL
DEC B
JR NZ,e4
LD HL,($e10)
LD A,C
OR A
JR Z,e6
SBC HL,DE
JR e7
e6:
ADD HL,DE
e7:
LD ($e10),HL
fn:
LD A,($nz)
OR A
JP Z,bad
LD HL,($e10)
LD A,H
OR L
JR Z,sn
LD A,($ra+3)
AND 127
JR NZ,sc
LD A,($ra+2)
AND 128
JR Z,sn
sc:
XOR A
LD ($ed),A
BIT 7,H
JR Z,s2
INC A
LD ($ed),A
LD A,L
CPL
LD L,A
LD A,H
CPL
LD H,A
INC HL
s2:
LD ($e10),HL
LD HL,10
LD BC,$rc
CALL RT_ITOF
LD HL,$rc
LD DE,($e10)
LD BC,$rb
CALL RT_FPOW
LD HL,$ra
LD DE,$rb
LD BC,$ra
LD A,($ed)
OR A
JR NZ,dv
CALL RT_FMUL
JR sn
dv:
CALL RT_FDIV
sn:
LD A,($ng)
OR A
LD HL,$ra
LD DE,($dst)
JR Z,cp
LD B,D
LD C,E
JP RT_FNEG
cp:
LD BC,4
LDIR
RET
