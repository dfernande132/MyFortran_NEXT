; rt_real.asm - Aritmetica REAL (IEEE 754 simple, sin denormales/infinitos/NaN; ver docs/decisiones/0013-real.md).
; Un REAL son 4 bytes little-endian: b0 b1 = mantisa baja, b2 = (exponente bit 0)<<7 | mantisa alta (7 bits), b3 = signo<<7 | exponente>>1.
;
; Registro DESEMPAQUETADO (8 B, "a" y "b" en el bloque de trabajo): m0 m1 m2 m3 (mantisa de 32 bits, bit 31 = el 1 implicito, m0 = bits de
; guarda), e (palabra con signo, exponente sesgado), s (0/1).
;
; Convenio de llamada de las operaciones: HL = direccion de a, DE = direccion de b, BC = direccion del resultado (A = 1 para restar en FADD).
; Todas pueden destruir cualquier registro.

.sym a=0
.sym a_m0=0
.sym a_m1=1
.sym a_m2=2
.sym a_m3=3
.sym a_e=4
.sym a_s=6
.sym b=8
.sym b_m0=8
.sym b_m1=9
.sym b_m2=10
.sym b_m3=11
.sym b_e=12
.sym b_s=14
.sym dst=16
.sym fop=18
.sym cnt=19
.sym w=20
.sym bn=48
.sym tmp=83
.sym dg=103
.sym dgs=167
.sym fpp=169
.sym nd=171
.sym ni=172
.sym zc=173
.sym e10=174
.sym rest=175

.rut FPD
.data 176

; ---- cadenas de bytes (little-endian) --------------------------------------------------------------------------------------------
; HL = puntero al byte menos significativo, B = n; el acarreo entra y sale por C
.rut BSHL
l:
RL (HL)
INC HL
DJNZ l
RET

; HL = puntero al byte MAS significativo, B = n
.rut BSHR
l:
RR (HL)
DEC HL
DJNZ l
RET

; (DE) += (HL), n bytes, con acarreo de entrada
.rut BADD
l:
LD A,(DE)
ADC A,(HL)
LD (DE),A
INC HL
INC DE
DJNZ l
RET

.rut BSUB
l:
LD A,(DE)
SBC A,(HL)
LD (DE),A
INC HL
INC DE
DJNZ l
RET

; ---- desempaquetar: HL = REAL, DE = registro --------------------------------------------------------------------------------------
.rut FUNP
XOR A
LD (DE),A
INC DE
LDI
LDI
LD A,(HL)
INC HL
LD C,A
LD A,(HL)
ADD A,A
LD B,0
RL B
BIT 7,C
JR Z,e0
INC A
e0:
OR A
JR Z,zero
LD H,A
LD A,C
OR 0x80
LD (DE),A
INC DE
LD A,H
LD (DE),A
INC DE
XOR A
LD (DE),A
INC DE
LD A,B
LD (DE),A
RET
zero:
LD (DE),A
DEC DE
LD (DE),A
DEC DE
LD (DE),A
INC DE
INC DE
INC DE
LD (DE),A
INC DE
LD (DE),A
INC DE
LD A,B
LD (DE),A
RET

; ---- normalizar, redondear y empaquetar el registro "a" en (dst) ------------------------------------------------------------------
.rut FNORM
LD HL,@a_m0
LD A,(HL)
INC HL
OR (HL)
INC HL
OR (HL)
INC HL
OR (HL)
JR NZ,nl
wr0:
LD HL,(@dst)
XOR A
LD (HL),A
INC HL
LD (HL),A
INC HL
LD (HL),A
INC HL
LD A,(@a_s)
RRCA
LD (HL),A
RET
nl:
LD A,(@a_m3)
BIT 7,A
JR NZ,rd
LD HL,@a_m0
LD B,4
OR A
CALL RT_BSHL
LD HL,(@a_e)
DEC HL
LD (@a_e),HL
JR nl
rd:
LD A,(@a_m0)
CP 0x80
JR C,ex
JR NZ,up
LD A,(@a_m1)
AND 1
JR Z,ex
up:
LD HL,@a_m1
INC (HL)
JR NZ,ex
INC HL
INC (HL)
JR NZ,ex
INC HL
INC (HL)
JR NZ,ex
LD (HL),0x80
LD HL,(@a_e)
INC HL
LD (@a_e),HL
ex:
LD HL,(@a_e)
BIT 7,H
JR NZ,wr0
LD A,H
OR A
JR NZ,big
OR L
JR Z,wr0
INC A
JR Z,big
LD HL,(@dst)
LD A,(@a_m1)
LD (HL),A
INC HL
LD A,(@a_m2)
LD (HL),A
INC HL
LD A,(@a_e)
RRCA
LD C,A
AND 0x80
LD B,A
LD A,(@a_m3)
AND 0x7F
OR B
LD (HL),A
INC HL
LD A,C
AND 0x7F
LD C,A
LD A,(@a_s)
RRCA
OR C
LD (HL),A
RET
big:
LD A,3
JP RT_ERR

; ---- suma / resta: A = 0 suma, 1 resta -----------------------------------------------------------------------------------------
.rut FADD
LD (@fop),A
LD (@dst),BC
PUSH DE
LD DE,@a
CALL RT_FUNP
POP HL
LD DE,@b
CALL RT_FUNP
LD A,(@fop)
OR A
JR Z,ns
LD A,(@b_s)
XOR 1
LD (@b_s),A
ns:
LD A,(@a_e)
OR A
JR Z,az
LD A,(@b_e)
OR A
JR NZ,bn
go:
JP RT_FNORM
az:
LD A,(@b_e)
OR A
JR NZ,bc
LD A,(@a_s)
LD B,A
LD A,(@b_s)
AND B
LD (@a_s),A
JR go
bc:
LD HL,@b
LD DE,@a
LD BC,8
LDIR
JR go
bn:
LD B,A
LD A,(@a_e)
CP B
JR NC,ok
LD HL,@a
LD DE,@b
LD B,8
sw:
LD A,(DE)
LD C,(HL)
LD (HL),A
LD A,C
LD (DE),A
INC HL
INC DE
DJNZ sw
ok:
LD A,(@b_e)
LD B,A
LD A,(@a_e)
SUB B
JR Z,al
LD B,A
sh:
LD HL,@b_m3
SRL (HL)
DEC HL
RR (HL)
DEC HL
RR (HL)
DEC HL
RR (HL)
JR NC,sn
SET 0,(HL)
sn:
DJNZ sh
al:
LD A,(@a_s)
LD B,A
LD A,(@b_s)
XOR B
JR NZ,sb
LD HL,@b_m0
LD DE,@a_m0
LD B,4
OR A
CALL RT_BADD
JR NC,gz
LD HL,@a_m3
LD B,4
SCF
CALL RT_BSHR
JR NC,ee
LD HL,@a_m0
SET 0,(HL)
ee:
LD HL,(@a_e)
INC HL
LD (@a_e),HL
gz:
JP RT_FNORM
sb:
LD HL,@b_m0
LD DE,@a_m0
LD B,4
OR A
CALL RT_BSUB
JR NC,sr
LD HL,@a_m0
LD B,4
OR A
ng:
LD A,0
SBC A,(HL)
LD (HL),A
INC HL
DJNZ ng
LD A,(@b_s)
LD (@a_s),A
sr:
LD HL,@a_m0
LD A,(HL)
INC HL
OR (HL)
INC HL
OR (HL)
INC HL
OR (HL)
JR NZ,gz
LD (@a_s),A
JR gz

; ---- multiplicacion ---------------------------------------------------------------------------------------------------------------
.rut FMUL
LD (@dst),BC
PUSH DE
LD DE,@a
CALL RT_FUNP
POP HL
LD DE,@b
CALL RT_FUNP
LD A,(@a_s)
LD B,A
LD A,(@b_s)
XOR B
LD (@a_s),A
LD A,(@a_e)
OR A
JR Z,zr
LD A,(@b_e)
OR A
JR NZ,mm
zr:
LD HL,@a_m0
LD B,4
z4:
LD (HL),0
INC HL
DJNZ z4
JP RT_FNORM
mm:
LD HL,@b_m1
LD DE,@w
LD BC,3
LDIR
XOR A
LD (@w+3),A
LD (@w+4),A
LD (@w+5),A
LD A,24
LD (@cnt),A
ml:
OR A
LD HL,@w
BIT 0,(HL)
JR Z,sr
LD HL,@a_m1
LD DE,@w+3
LD B,3
CALL RT_BADD
sr:
LD HL,@w+5
LD B,6
CALL RT_BSHR
LD HL,@cnt
DEC (HL)
JR NZ,ml
LD HL,@w+2
LD DE,@a_m0
LD BC,4
LDIR
LD HL,@w
LD A,(HL)
INC HL
OR (HL)
JR Z,ex
LD HL,@a_m0
SET 0,(HL)
ex:
LD HL,(@a_e)
LD DE,(@b_e)
ADD HL,DE
LD DE,-126
ADD HL,DE
LD (@a_e),HL
JP RT_FNORM

; ---- division -----------------------------------------------------------------------------------------------------------------------
.rut FDIV
LD (@dst),BC
PUSH DE
LD DE,@a
CALL RT_FUNP
POP HL
LD DE,@b
CALL RT_FUNP
LD A,(@a_s)
LD B,A
LD A,(@b_s)
XOR B
LD (@a_s),A
LD A,(@b_e)
OR A
JR Z,dz
LD A,(@a_e)
OR A
JR NZ,dv
zr:
LD HL,@a_m0
LD B,4
z4:
LD (HL),0
INC HL
DJNZ z4
JP RT_FNORM
dz:
LD A,(@a_e)
OR A
LD A,1
JR NZ,ez
INC A
ez:
JP RT_ERR
dv:
LD HL,@a_m1
LD DE,@w
LD BC,3
LDIR
XOR A
LD (@w+3),A
LD HL,@b_m1
LD DE,@w+4
LD BC,3
LDIR
XOR A
LD (@w+7),A
LD HL,@a_m0
LD B,4
q0:
LD (HL),A
INC HL
DJNZ q0
LD A,26
LD (@cnt),A
dl:
LD HL,@w+4
LD DE,@w
LD B,4
OR A
CALL RT_BSUB
JR NC,q1
LD HL,@w+4
LD DE,@w
LD B,4
OR A
CALL RT_BADD
OR A
JR sq
q1:
SCF
sq:
LD HL,@a_m0
LD B,4
CALL RT_BSHL
LD HL,@w
LD B,4
OR A
CALL RT_BSHL
LD HL,@cnt
DEC (HL)
JR NZ,dl
LD C,6
s6:
LD HL,@a_m0
LD B,4
OR A
CALL RT_BSHL
DEC C
JR NZ,s6
LD HL,@w
LD A,(HL)
INC HL
OR (HL)
INC HL
OR (HL)
INC HL
OR (HL)
JR Z,ex
LD HL,@a_m0
SET 0,(HL)
ex:
LD HL,(@a_e)
LD DE,(@b_e)
OR A
SBC HL,DE
LD DE,127
ADD HL,DE
LD (@a_e),HL
JP RT_FNORM

; ---- comparacion: HL = -1, 0 o 1 (a<b, a=b, a>b) -----------------------------------------------------------------------------------
.rut FCMP
PUSH DE
LD DE,@a
CALL RT_FUNP
POP HL
LD DE,@b
CALL RT_FUNP
LD A,(@a_e)
OR A
JR NZ,x1
LD (@a_s),A
x1:
LD A,(@b_e)
OR A
JR NZ,x2
LD (@b_s),A
x2:
LD A,(@a_s)
LD B,A
LD A,(@b_s)
SUB B
JR Z,sm
LD L,A
ADD A,A
SBC A,A
LD H,A
RET
sm:
LD HL,@a_e
LD DE,@b_e
LD B,4
l:
LD A,(DE)
CP (HL)
JR NZ,df
DEC HL
DEC DE
DJNZ l
LD HL,0
RET
df:
SBC A,A
AND 1
LD B,A
LD A,(@a_s)
XOR B
LD HL,0xFFFF
RET Z
LD HL,1
RET

; ---- INTEGER -> REAL: HL = entero, BC = destino --------------------------------------------------------------------------------------
.rut ITOF
LD (@dst),BC
XOR A
LD (@a_m0),A
LD (@a_m1),A
LD (@a_s),A
BIT 7,H
JR Z,pp
LD A,L
CPL
LD L,A
LD A,H
CPL
LD H,A
INC HL
LD A,1
LD (@a_s),A
pp:
LD A,H
LD (@a_m3),A
LD A,L
LD (@a_m2),A
LD HL,142
LD (@a_e),HL
JP RT_FNORM

; ---- REAL -> INTEGER (trunca, satura): HL = REAL, devuelve HL = entero ----------------------------------------------------------------
.rut FTOI
LD DE,@a
CALL RT_FUNP
LD A,(@a_e)
CP 142
JR NC,sat
LD C,A
LD A,142
SUB C
LD B,A
LD HL,(@a_m2)
sh:
SRL H
RR L
DJNZ sh
LD A,(@a_s)
OR A
RET Z
LD A,L
CPL
LD L,A
LD A,H
CPL
LD H,A
INC HL
RET
sat:
LD A,(@a_s)
OR A
LD HL,0x7FFF
RET Z
LD HL,0x8000
RET

; ---- cambio de signo: HL = origen, BC = destino ------------------------------------------------------------------------------------------
.rut FNEG
LD D,B
LD E,C
LD BC,4
LDIR
DEC DE
LD A,(DE)
XOR 0x80
LD (DE),A
RET

; ---- escribir un REAL (PRINT *): HL = direccion ------------------------------------------------------------------------------------------
.rut FPUT
PUSH HL
LD A,32
CALL RT_PUTC
POP HL
LD DE,@a
CALL RT_FUNP
LD A,(@a_s)
OR A
JR Z,ps
LD A,45
CALL RT_PUTC
ps:
LD A,(@a_e)
OR A
JR NZ,nz
LD A,48
CALL RT_PUTC
LD A,46
CALL RT_PUTC
LD B,8
zl:
LD A,48
CALL RT_PUTC
DJNZ zl
RET
nz:
LD HL,@bn
LD (HL),0
LD DE,@bn+1
LD BC,118
LDIR
LD A,(@a_e)
ADD A,2
LD C,A
RRA
SRL A
SRL A
LD E,A
LD D,0
LD HL,@bn
ADD HL,DE
EX DE,HL
LD HL,@a_m1
PUSH BC
LD BC,3
LDIR
POP BC
LD A,C
AND 7
JR Z,nsh
LD C,A
sl:
LD HL,@bn
LD B,35
OR A
CALL RT_BSHL
DEC C
JR NZ,sl
nsh:
LD HL,@dg+48
LD (@fpp),HL
LD (@dgs),HL
XOR A
LD (@nd),A
LD (@ni),A
LD (@zc),A
LD HL,@bn+19
LD B,16
zt:
OR (HL)
INC HL
DJNZ zt
JR Z,fr
il:
LD HL,@bn+34
LD D,16
XOR A
dl:
LD C,(HL)
LD B,8
d8:
SLA C
RLA
CP 10
JR C,nd8
SUB 10
INC C
nd8:
DJNZ d8
LD (HL),C
DEC HL
DEC D
JR NZ,dl
LD HL,(@dgs)
DEC HL
LD (HL),A
LD (@dgs),HL
LD HL,@ni
INC (HL)
LD HL,@bn+19
LD B,16
XOR A
z2:
OR (HL)
INC HL
DJNZ z2
JR NZ,il
fr:
LD A,(@ni)
LD (@nd),A
fl:
LD A,(@nd)
CP 10
JR NC,fd
LD HL,@bn
LD B,19
XOR A
z3:
OR (HL)
INC HL
DJNZ z3
JR Z,fd
LD HL,@bn
LD DE,@tmp
LD BC,19
LDIR
XOR A
LD (@tmp+19),A
LD HL,@tmp
LD B,20
OR A
CALL RT_BSHL
LD HL,@tmp
LD B,20
OR A
CALL RT_BSHL
LD HL,@bn
LD DE,@tmp
LD B,20
OR A
CALL RT_BADD
LD HL,@tmp
LD B,20
OR A
CALL RT_BSHL
LD HL,@tmp
LD DE,@bn
LD BC,19
LDIR
LD A,(@tmp+19)
LD B,A
LD A,(@nd)
OR B
JR NZ,st
LD HL,@zc
INC (HL)
JR fl
st:
LD A,B
LD HL,(@fpp)
LD (HL),A
INC HL
LD (@fpp),HL
LD HL,@nd
INC (HL)
JR fl
fd:
LD A,(@ni)
OR A
JR Z,ez
DEC A
LD (@e10),A
JR ec
ez:
LD A,(@zc)
CPL
LD (@e10),A
ec:
LD HL,(@dgs)
LD DE,10
ADD HL,DE
EX DE,HL
LD HL,@dg+64
OR A
SBC HL,DE
LD B,L
EX DE,HL
XOR A
rl:
OR (HL)
INC HL
DJNZ rl
LD C,A
LD HL,@bn
LD B,19
XOR A
fz:
OR (HL)
INC HL
DJNZ fz
OR C
LD (@rest),A
LD HL,(@dgs)
LD DE,9
ADD HL,DE
LD A,(HL)
DEC HL
CP 5
JR C,rdn
JR NZ,rup
LD A,(@rest)
OR A
JR NZ,rup
LD A,(HL)
AND 1
JR Z,rdn
rup:
LD B,9
ic:
INC (HL)
LD A,(HL)
CP 10
JR NZ,rdn
LD (HL),0
DEC HL
DJNZ ic
INC HL
LD (HL),1
LD HL,@e10
INC (HL)
rdn:
LD A,(@e10)
INC A
CP 10
JR NC,ef
LD B,A
LD HL,(@dgs)
OR A
JR NZ,fi
LD A,48
CALL RT_PUTC
JR fp
fi:
LD A,(HL)
ADD A,48
CALL RT_PUTC
INC HL
DJNZ fi
fp:
LD A,46
CALL RT_PUTC
LD A,(@e10)
INC A
LD C,A
LD A,9
SUB C
RET Z
LD B,A
f2:
LD A,(HL)
ADD A,48
CALL RT_PUTC
INC HL
DJNZ f2
RET
ef:
LD HL,(@dgs)
LD A,(HL)
ADD A,48
CALL RT_PUTC
INC HL
LD A,46
CALL RT_PUTC
LD B,8
el:
LD A,(HL)
ADD A,48
CALL RT_PUTC
INC HL
DJNZ el
LD A,69
CALL RT_PUTC
LD A,(@e10)
LD B,43
BIT 7,A
JR Z,ep
NEG
LD B,45
ep:
LD C,A
LD A,B
CALL RT_PUTC
LD A,C
LD B,48
tl:
CP 10
JR C,td
SUB 10
INC B
JR tl
td:
PUSH AF
LD A,B
CALL RT_PUTC
POP AF
ADD A,48
CALL RT_PUTC
RET

; ---- AINT / ANINT: HL = origen, BC = destino, A = 0 trunca, 1 al mas cercano (mitades lejos del cero) ---------------------------------------
.rut FINT
LD (@fop),A
LD (@dst),BC
LD DE,@a
CALL RT_FUNP
LD A,(@a_e)
OR A
JR Z,go
CP 150
JR NC,go
CP 126
JR C,zr
LD C,A
LD A,150
SUB C
LD B,A
sh:
LD HL,@a_m3
SRL (HL)
DEC HL
RR (HL)
DEC HL
RR (HL)
DJNZ sh
LD A,(@fop)
BIT 0,A
JR Z,fx
JR NC,fx
LD HL,@a_m1
INC (HL)
JR NZ,fx
INC HL
INC (HL)
JR NZ,fx
INC HL
INC (HL)
fx:
LD HL,150
LD (@a_e),HL
go:
JP RT_FNORM
zr:
LD HL,@a_m0
LD B,4
z4:
LD (HL),0
INC HL
DJNZ z4
JP RT_FNORM

; ---- raiz cuadrada (redondeada al par, como IEEE): HL = origen, BC = destino. Argumento negativo: error 4 -------------------------------------
.rut FSQRT
LD (@dst),BC
LD DE,@a
CALL RT_FUNP
LD A,(@a_e)
OR A
JR NZ,nzr
JP RT_FNORM
nzr:
LD A,(@a_s)
OR A
JR Z,ps
LD A,4
JP RT_ERR
ps:
LD HL,@a_m0
LD DE,@w
LD BC,4
LDIR
LD A,(@a_e)
LD B,1
RRA
JR C,od
INC B
od:
sr:
PUSH BC
LD HL,@w+3
LD B,4
OR A
CALL RT_BSHR
POP BC
DJNZ sr
LD HL,@w+4
LD B,8
XOR A
z8:
LD (HL),A
INC HL
DJNZ z8
LD A,27
LD (@cnt),A
it:
LD C,2
bt:
LD HL,@w
LD B,4
OR A
CALL RT_BSHL
LD HL,@w+4
LD B,4
CALL RT_BSHL
DEC C
JR NZ,bt
LD HL,@w+8
LD DE,@w+12
LD BC,4
LDIR
LD C,2
tr:
LD HL,@w+12
LD B,4
OR A
CALL RT_BSHL
DEC C
JR NZ,tr
LD HL,@w+12
SET 0,(HL)
LD HL,@w+8
LD B,4
OR A
CALL RT_BSHL
LD HL,@w+12
LD DE,@w+4
LD B,4
OR A
CALL RT_BSUB
JR C,rs
LD HL,@w+8
SET 0,(HL)
JR nx
rs:
LD HL,@w+12
LD DE,@w+4
LD B,4
OR A
CALL RT_BADD
nx:
LD A,(@cnt)
DEC A
LD (@cnt),A
JR NZ,it
LD HL,@w+4
LD A,(HL)
INC HL
OR (HL)
INC HL
OR (HL)
INC HL
OR (HL)
LD (@w+16),A
LD HL,@w+8
LD DE,@a_m0
LD BC,4
LDIR
LD C,5
sl:
LD HL,@a_m0
LD B,4
OR A
CALL RT_BSHL
DEC C
JR NZ,sl
LD A,(@w+16)
OR A
JR Z,ns
LD HL,@a_m0
SET 0,(HL)
ns:
LD A,(@a_e)
SRL A
ADD A,64
LD L,A
LD H,0
LD (@a_e),HL
go:
JP RT_FNORM

; ---- AMOD: HL = x, DE = p, BC = destino. x - INT(x/p)*p exacto (resto binario por restas y desplazamientos) ----------------------------------
.rut FMOD
LD (@dst),BC
PUSH DE
LD DE,@a
CALL RT_FUNP
POP HL
LD DE,@b
CALL RT_FUNP
LD A,(@b_e)
OR A
JR NZ,mp
LD A,(@a_e)
OR A
LD A,1
JR NZ,e1
INC A
e1:
JP RT_ERR
mp:
LD A,(@a_e)
OR A
JR Z,go
LD B,A
LD A,(@b_e)
LD C,A
LD A,B
SUB C
JR C,go
INC A
LD (@cnt),A
XOR A
LD (@w+16),A
lp:
LD HL,@b_m0
LD DE,@a_m0
LD B,4
OR A
CALL RT_BSUB
JR NC,ok
LD A,(@w+16)
OR A
JR NZ,ok
LD HL,@b_m0
LD DE,@a_m0
LD B,4
OR A
CALL RT_BADD
ok:
LD A,(@cnt)
DEC A
LD (@cnt),A
JR Z,fin
LD HL,@a_m0
LD B,4
OR A
CALL RT_BSHL
SBC A,A
AND 1
LD (@w+16),A
JR lp
fin:
LD HL,(@b_e)
LD (@a_e),HL
go:
JP RT_FNORM

; ---- REAL ** INTEGER: HL = base, DE = exponente (entero), BC = destino. Como __powisf2 de libgcc; 0**0 y 0**negativo: error 5 ---------------
.rut FPOW
LD (@bn+11),BC
LD A,D
AND 0x80
LD (@bn+10),A
BIT 7,D
JR Z,ab
LD A,E
CPL
LD E,A
LD A,D
CPL
LD D,A
INC DE
ab:
LD (@bn+8),DE
LD DE,@bn
LD BC,4
LDIR
LD A,(@bn+3)
AND 0x7F
JR NZ,nz
LD A,(@bn+2)
AND 0x80
JR NZ,nz
LD A,(@bn+10)
OR A
JR NZ,e5
LD HL,(@bn+8)
LD A,H
OR L
JR NZ,nz
e5:
LD A,5
JP RT_ERR
nz:
LD HL,(@bn+8)
BIT 0,L
JR Z,on
LD HL,@bn
LD DE,@bn+4
LD BC,4
LDIR
JR lp
on:
LD HL,@bn+4
LD (HL),0
INC HL
LD (HL),0
INC HL
LD (HL),0x80
INC HL
LD (HL),0x3F
lp:
LD HL,(@bn+8)
SRL H
RR L
LD (@bn+8),HL
LD A,H
OR L
JR Z,fin
LD HL,@bn
LD DE,@bn
LD BC,@bn
CALL RT_FMUL
LD HL,(@bn+8)
BIT 0,L
JR Z,lp
LD HL,@bn+4
LD DE,@bn
LD BC,@bn+4
CALL RT_FMUL
JR lp
fin:
LD A,(@bn+10)
OR A
JR Z,mv
LD HL,@bn+13
LD (HL),0
INC HL
LD (HL),0
INC HL
LD (HL),0x80
INC HL
LD (HL),0x3F
LD HL,@bn+13
LD DE,@bn+4
LD BC,@bn+4
CALL RT_FDIV
mv:
LD HL,@bn+4
LD DE,(@bn+11)
LD BC,4
LDIR
RET

; ---- error de ejecucion: A = 1 division por cero, 2 0/0, 3 overflow REAL, 4 SQRT de un negativo, 5 0 ** (<= 0), 6 dato de entrada no valido (READ), 7 fin de la entrada. Imprime el mensaje y un salto de linea y PARA ----------------
; (HALT con las interrupciones desactivadas; el arranque del .nex hace DI). Los textos van detras del HALT.
.rut ERR
LD HL,m1
DEC A
JR Z,pr
LD HL,m2
DEC A
JR Z,pr
LD HL,m3
DEC A
JR Z,pr
LD HL,m4
DEC A
JR Z,pr
LD HL,m5
DEC A
JR Z,pr
LD HL,m6
DEC A
JR Z,pr
LD HL,m7
pr:
LD A,(HL)
INC HL
OR A
JR Z,nl
PUSH HL
CALL RT_PUTC
POP HL
JR pr
nl:
LD A,13
CALL RT_PUTC
HALT
m1:
.db "Division by zero",0
m2:
.db "0/0 is indeterminate",0
m3:
.db "REAL overflow",0
m4:
.db "SQRT of a negative number",0
m5:
.db "Zero to a non-positive power",0
m6:
.db "Invalid input",0
m7:
.db "End of input",0
