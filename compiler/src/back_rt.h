/* back_rt.h - Rutinas de apoyo (la "libreria de ejecucion") del programa generado, como TABLAS DE DATOS.
 *
 * Cada rutina es una lista de instrucciones Z80; back.c la recorre en rutina() y emite cada una con
 * z_ins() (bytes + linea de listado). Antes eran llamadas i1(...), i2(...) una por instruccion: unos 16 B
 * de codigo por instruccion Z80 de 1-3 bytes. Como datos cada una ocupa 2-4 B, y el banco 43 del Next
 * (back + z80 + nex) tenia solo ~0,6 KB libres. La salida (bytes y listado) es identica.
 *
 * Las macros I1, I2, I3... son las llamadas de antes en mayuscula, y se leen igual:
 *   I1(texto, opcode)                   instruccion de 1 byte
 *   I2(texto, b0, b1)                   de 2 bytes
 *   I3(texto, arg, b0, lo, hi)          de 3 bytes; la palabra va en little-endian (primero el byte bajo);
 *                                       arg sustituye al %s del texto en el listado
 *   IJR(texto, opcode, id)              JR cc a la etiqueta local id (desplazamiento calculado)
 *   IJPLOC(texto, opcode, id)           JP / CALL a la etiqueta local id (direccion absoluta)
 *   ILOC(id)                            define la etiqueta local id en el punto actual
 *   ICALLRT(r) / IJPRT(r)               CALL / JP a la rutina r (y la marca como necesaria)
 *   IRTOP(texto, opcode, r)             CALL cc / JP cc a la rutina r (sin marcarla)
 *   INECESITA(r)                        marca la rutina r como necesaria
 *   IREF(texto, arg, opcode)            instruccion de 3 bytes con la direccion del cursor rt_cur
 *   ICURDEF                             define rt_cur: dos bytes a cero (solo la salida a pantalla)
 *   IREFO(texto, arg, op, off)          instruccion de 3 bytes con la direccion RT_FPD + off (bloque de trabajo REAL)
 *   IREFE(texto, arg, pref, op, off)    lo mismo con prefijo (ED xx nn nn: 4 bytes)
 *   IDATA(n)                            n bytes a cero (el bloque de trabajo)
 *   IFIN                                fin de la tabla
 * El texto va dentro de T(): sin listado (-DMF_SIN_LISTADO, el Next) no existe y la tabla es un
 * flujo de bytes; con listado cada elemento es una estructura RtI con sus textos.
 */

#include "back_rt_mac.h"

/* RT_PUTC: A = caracter; conserva todos los registros. Salida por RST 16 */
static const RtI rt_c_putc_rst[] = {
        I1(T("PUSH AF"), 0xF5) I1(T("PUSH BC"), 0xC5) I1(T("PUSH DE"), 0xD5) I1(T("PUSH HL"), 0xE5)
            I1(T("RST 16"), 0xD7)
            I1(T("POP HL"), 0xE1) I1(T("POP DE"), 0xD1) I1(T("POP BC"), 0xC1) I1(T("POP AF"), 0xF1)
            I1(T("RET"), 0xC9)
        IFIN
};

/* RT_PUTC: lo mismo, con la rutina propia que dibuja en la pantalla (back_salida_pantalla) */
static const RtI rt_c_putc_pant[] = {
        I1(T("PUSH AF"), 0xF5) I1(T("PUSH BC"), 0xC5) I1(T("PUSH DE"), 0xD5) I1(T("PUSH HL"), 0xE5)
        I2(T("CP 13"), 0xFE, 0x0D)
        IJR(T("JR Z,%s"), 0x28, 20)
        I2(T("CP 8"), 0xFE, 0x08)               /* retroceso: el cursor una columna a la izquierda (si no esta en la primera) */
        IJR(T("JR NZ,%s"), 0x20, 29)
        IREF(T("LD HL,(%s)"), "rt_cur", 0x2A)
        I1(T("LD A,L"), 0x7D) I1(T("OR A"), 0xB7)
        IJPLOC(T("JP Z,%s"), 0xCA, 23)
        I1(T("DEC L"), 0x2D)
        IJPLOC(T("JP %s"), 0xC3, 22)
        ILOC(29)
        /* DE = fuente + 8*A (la ROM tiene los caracteres desde el 32 en $3D00 = $3C00 + 8*32) */
        I1(T("LD L,A"), 0x6F) I2(T("LD H,0"), 0x26, 0x00)
        I1(T("ADD HL,HL"), 0x29) I1(T("ADD HL,HL"), 0x29) I1(T("ADD HL,HL"), 0x29)
        I3(T("LD DE,%s"), "$3C00", 0x11, 0x00, 0x3C)
        I1(T("ADD HL,DE"), 0x19) I1(T("EX DE,HL"), 0xEB)
        /* BC = direccion de pantalla de (columna L, fila H): $40|(fila&$18) , ((fila&7)<<5)|columna */
        IREF(T("LD HL,(%s)"), "rt_cur", 0x2A)
        I1(T("LD A,H"), 0x7C) I2(T("AND $18"), 0xE6, 0x18) I2(T("OR $40"), 0xF6, 0x40) I1(T("LD B,A"), 0x47)
        I1(T("LD A,H"), 0x7C) I2(T("AND 7"), 0xE6, 0x07)
        I1(T("ADD A,A"), 0x87) I1(T("ADD A,A"), 0x87) I1(T("ADD A,A"), 0x87) I1(T("ADD A,A"), 0x87) I1(T("ADD A,A"), 0x87)
        I1(T("OR L"), 0xB5) I1(T("LD C,A"), 0x4F)
        I2(T("LD H,8"), 0x26, 0x08)
        ILOC(21)
        I1(T("LD A,(DE)"), 0x1A) I1(T("LD (BC),A"), 0x02)
        I1(T("INC DE"), 0x13) I1(T("INC B"), 0x04) I1(T("DEC H"), 0x25)
        IJR(T("JR NZ,%s"), 0x20, 21)
        IREF(T("LD HL,(%s)"), "rt_cur", 0x2A)
        I1(T("INC L"), 0x2C) I1(T("LD A,L"), 0x7D) I2(T("CP 32"), 0xFE, 0x20)
        IJR(T("JR C,%s"), 0x38, 22)
        ILOC(20)                            /* salto de linea */
        IREF(T("LD HL,(%s)"), "rt_cur", 0x2A)
        I2(T("LD L,0"), 0x2E, 0x00) I1(T("INC H"), 0x24) I1(T("LD A,H"), 0x7C) I2(T("CP 24"), 0xFE, 0x18)
        IJR(T("JR C,%s"), 0x38, 22)
        /* pasada la fila 23: el cursor se queda en ella, la pantalla sube una fila (copia fila R+1 -> R, R = 0..22, 8 lineas de 32 B;
         * la direccion de la fila siguiente suma $20 a E y, si hay acarreo, $08 a D: cambio de tercio) y se borra la ultima */
        I2(T("LD H,23"), 0x26, 0x17)
        IREF(T("LD (%s),HL"), "rt_cur", 0x22)
        I3(T("LD DE,%s"), "$4000", 0x11, 0x00, 0x40)
        I3(T("LD HL,%s"), "$4020", 0x21, 0x20, 0x40)
        I2(T("LD A,23"), 0x3E, 0x17)
        ILOC(24)
        I1(T("PUSH AF"), 0xF5) I1(T("PUSH HL"), 0xE5) I1(T("PUSH DE"), 0xD5)
        I2(T("LD B,8"), 0x06, 0x08)
        ILOC(25)
        I1(T("PUSH BC"), 0xC5) I1(T("PUSH HL"), 0xE5) I1(T("PUSH DE"), 0xD5)
        I3(T("LD BC,%s"), "32", 0x01, 0x20, 0x00)
        I2(T("LDIR"), 0xED, 0xB0)
        I1(T("POP DE"), 0xD1) I1(T("POP HL"), 0xE1) I1(T("INC H"), 0x24) I1(T("INC D"), 0x14) I1(T("POP BC"), 0xC1)
        IJR(T("DJNZ %s"), 0x10, 25)
        I1(T("POP DE"), 0xD1) I1(T("POP HL"), 0xE1)
        I1(T("LD A,E"), 0x7B) I2(T("ADD A,$20"), 0xC6, 0x20) I1(T("LD E,A"), 0x5F)
        IJR(T("JR NC,%s"), 0x30, 26)
        I1(T("LD A,D"), 0x7A) I2(T("ADD A,8"), 0xC6, 0x08) I1(T("LD D,A"), 0x57)
        ILOC(26)
        I1(T("LD A,L"), 0x7D) I2(T("ADD A,$20"), 0xC6, 0x20) I1(T("LD L,A"), 0x6F)
        IJR(T("JR NC,%s"), 0x30, 27)
        I1(T("LD A,H"), 0x7C) I2(T("ADD A,8"), 0xC6, 0x08) I1(T("LD H,A"), 0x67)
        ILOC(27)
        I1(T("POP AF"), 0xF1) I1(T("DEC A"), 0x3D)
        IJR(T("JR NZ,%s"), 0x20, 24)
        I3(T("LD HL,%s"), "$50E0", 0x21, 0xE0, 0x50)
        I2(T("LD B,8"), 0x06, 0x08)
        ILOC(28)
        I1(T("PUSH BC"), 0xC5) I1(T("PUSH HL"), 0xE5) I1(T("LD D,H"), 0x54) I1(T("LD E,L"), 0x5D) I1(T("INC DE"), 0x13)
        I2(T("LD (HL),0"), 0x36, 0x00)
        I3(T("LD BC,%s"), "31", 0x01, 0x1F, 0x00)
        I2(T("LDIR"), 0xED, 0xB0)
        I1(T("POP HL"), 0xE1) I1(T("INC H"), 0x24) I1(T("POP BC"), 0xC1)
        IJR(T("DJNZ %s"), 0x10, 28)
        IJR(T("JR %s"), 0x18, 23)
        ILOC(22)
        IREF(T("LD (%s),HL"), "rt_cur", 0x22)
        ILOC(23)
        I1(T("POP HL"), 0xE1) I1(T("POP DE"), 0xD1) I1(T("POP BC"), 0xC1) I1(T("POP AF"), 0xF1)
        I1(T("RET"), 0xC9)
        ICURDEF
        IFIN
};

/* RT_NEGHL */
static const RtI rt_c_neghl[] = {
        I1(T("LD A,L"), 0x7D) I1(T("CPL"), 0x2F) I1(T("LD L,A"), 0x6F)
        I1(T("LD A,H"), 0x7C) I1(T("CPL"), 0x2F) I1(T("LD H,A"), 0x67)
        I1(T("INC HL"), 0x23) I1(T("RET"), 0xC9)
        IFIN
};

/* RT_NEGDE */
static const RtI rt_c_negde[] = {
        I1(T("LD A,E"), 0x7B) I1(T("CPL"), 0x2F) I1(T("LD E,A"), 0x5F)
        I1(T("LD A,D"), 0x7A) I1(T("CPL"), 0x2F) I1(T("LD D,A"), 0x57)
        I1(T("INC DE"), 0x13) I1(T("RET"), 0xC9)
        IFIN
};

/* RT_MUL: HL = HL * DE (16 bits) */
static const RtI rt_c_mul[] = {
        I1(T("LD B,H"), 0x44) I1(T("LD C,L"), 0x4D)
        I3(T("LD HL,%s"), "0", 0x21, 0x00, 0x00)
        I2(T("LD A,16"), 0x3E, 0x10)
        ILOC(0)
        I1(T("ADD HL,HL"), 0x29)
        I2(T("SLA E"), 0xCB, 0x23)
        I2(T("RL D"), 0xCB, 0x12)
        IJR(T("JR NC,%s"), 0x30, 1)
        I1(T("ADD HL,BC"), 0x09)
        ILOC(1)
        I1(T("DEC A"), 0x3D)
        IJR(T("JR NZ,%s"), 0x20, 0)
        I1(T("RET"), 0xC9)
        IFIN
};

/* RT_DIV: HL = HL / DE, con signo, truncando hacia cero */
static const RtI rt_c_div[] = {
        /* divisor 0: error de ejecucion (A = 1 "Division by zero", 2 "0/0 is indeterminate") */
        I1(T("LD A,D"), 0x7A) I1(T("OR E"), 0xB3) IJR(T("JR NZ,%s"), 0x20, 42)
        I1(T("LD A,H"), 0x7C) I1(T("OR L"), 0xB5) I2(T("LD A,1"), 0x3E, 1) IJR(T("JR NZ,%s"), 0x20, 43)
        I1(T("INC A"), 0x3C)
        ILOC(43) IJPRT(RT_ERR)
        ILOC(42)
        I1(T("LD A,H"), 0x7C) I1(T("XOR D"), 0xAA) I1(T("PUSH AF"), 0xF5)
        I2(T("BIT 7,H"), 0xCB, 0x7C)
        INECESITA(RT_NEGHL)
        IRTOP(T("CALL NZ,%s"), 0xC4, RT_NEGHL)
        I2(T("BIT 7,D"), 0xCB, 0x7A)
        IRTOP(T("CALL NZ,%s"), 0xC4, RT_NEGDE)
        I1(T("LD B,H"), 0x44) I1(T("LD C,L"), 0x4D)
        I3(T("LD HL,%s"), "0", 0x21, 0x00, 0x00)
        I2(T("LD A,16"), 0x3E, 0x10)
        ILOC(2)
        I2(T("SLA C"), 0xCB, 0x21)
        I2(T("RL B"), 0xCB, 0x10)
        I2(T("ADC HL,HL"), 0xED, 0x6A)
        I1(T("OR A"), 0xB7)
        I2(T("SBC HL,DE"), 0xED, 0x52)
        IJR(T("JR NC,%s"), 0x30, 3)
        I1(T("ADD HL,DE"), 0x19)
        IJR(T("JR %s"), 0x18, 4)
        ILOC(3)
        I1(T("INC C"), 0x0C)
        ILOC(4)
        I1(T("DEC A"), 0x3D)
        IJR(T("JR NZ,%s"), 0x20, 2)
        I1(T("LD H,B"), 0x60) I1(T("LD L,C"), 0x69)
        I1(T("POP AF"), 0xF1)
        I1(T("RET P"), 0xF0)
        IJPRT(RT_NEGHL)
        IFIN
};

/* RT_POW: HL = HL ** DE */
static const RtI rt_c_pow[] = {
        I2(T("BIT 7,D"), 0xCB, 0x7A)
        IJR(T("JR NZ,%s"), 0x20, 8)
        I1(T("LD B,D"), 0x42) I1(T("LD C,E"), 0x4B)
        I1(T("LD D,H"), 0x54) I1(T("LD E,L"), 0x5D)
        I3(T("LD HL,%s"), "1", 0x21, 0x01, 0x00)
        ILOC(5)
        I1(T("LD A,B"), 0x78) I1(T("OR C"), 0xB1)
        I1(T("RET Z"), 0xC8)
        I1(T("PUSH BC"), 0xC5) I1(T("PUSH DE"), 0xD5)
        ICALLRT(RT_MUL)
        I1(T("POP DE"), 0xD1) I1(T("POP BC"), 0xC1)
        I1(T("DEC BC"), 0x0B)
        IJR(T("JR %s"), 0x18, 5)
        ILOC(8)             /* exponente negativo */
        I1(T("LD A,H"), 0x7C) I1(T("OR A"), 0xB7)
        IJR(T("JR NZ,%s"), 0x20, 9)
        I1(T("LD A,L"), 0x7D) I1(T("DEC A"), 0x3D)
        IJR(T("JR NZ,%s"), 0x20, 11)
        I3(T("LD HL,%s"), "1", 0x21, 0x01, 0x00)
        I1(T("RET"), 0xC9)
        ILOC(9)
        I1(T("LD A,H"), 0x7C) I1(T("INC A"), 0x3C)
        IJR(T("JR NZ,%s"), 0x20, 11)
        I1(T("LD A,L"), 0x7D) I1(T("INC A"), 0x3C)
        IJR(T("JR NZ,%s"), 0x20, 11)
        I1(T("LD A,E"), 0x7B) I2(T("AND 1"), 0xE6, 0x01)
        IJR(T("JR Z,%s"), 0x28, 10)
        I3(T("LD HL,%s"), "65535", 0x21, 0xFF, 0xFF)
        I1(T("RET"), 0xC9)
        ILOC(10)
        I3(T("LD HL,%s"), "1", 0x21, 0x01, 0x00)
        I1(T("RET"), 0xC9)
        ILOC(11)
        I3(T("LD HL,%s"), "0", 0x21, 0x00, 0x00)
        I1(T("RET"), 0xC9)
        IFIN
};

/* RT_LT: HL = (HL < DE) con signo, 0 o 1 */
static const RtI rt_c_lt[] = {
        I1(T("LD A,H"), 0x7C) I2(T("XOR 128"), 0xEE, 0x80) I1(T("LD H,A"), 0x67)
        I1(T("LD A,D"), 0x7A) I2(T("XOR 128"), 0xEE, 0x80) I1(T("LD D,A"), 0x57)
        I1(T("OR A"), 0xB7)
        I2(T("SBC HL,DE"), 0xED, 0x52)
        I3(T("LD HL,%s"), "0", 0x21, 0x00, 0x00)
        I1(T("RET NC"), 0xD0)
        I1(T("INC L"), 0x2C)
        I1(T("RET"), 0xC9)
        IFIN
};

/* RT_GE */
static const RtI rt_c_ge[] = {
        ICALLRT(RT_LT)
        I1(T("LD A,L"), 0x7D) I2(T("XOR 1"), 0xEE, 0x01) I1(T("LD L,A"), 0x6F)
        I1(T("RET"), 0xC9)
        IFIN
};

/* RT_GT */
static const RtI rt_c_gt[] = {
        I1(T("EX DE,HL"), 0xEB)
        IJPRT(RT_LT)
        IFIN
};

/* RT_LE */
static const RtI rt_c_le[] = {
        I1(T("EX DE,HL"), 0xEB)
        IJPRT(RT_GE)
        IFIN
};

/* RT_EQ */
static const RtI rt_c_eq[] = {
        I1(T("OR A"), 0xB7)
        I2(T("SBC HL,DE"), 0xED, 0x52)
        I3(T("LD HL,%s"), "0", 0x21, 0x00, 0x00)
        I1(T("RET NZ"), 0xC0)
        I1(T("INC L"), 0x2C)
        I1(T("RET"), 0xC9)
        IFIN
};

/* RT_NE */
static const RtI rt_c_ne[] = {
        ICALLRT(RT_EQ)
        I1(T("LD A,L"), 0x7D) I2(T("XOR 1"), 0xEE, 0x01) I1(T("LD L,A"), 0x6F)
        I1(T("RET"), 0xC9)
        IFIN
};

/* RT_PUTI: escribe ' ' y HL como entero con signo */
static const RtI rt_c_puti[] = {
        I2(T("LD A,32"), 0x3E, 0x20)
        ICALLRT(RT_PUTC)
        I2(T("BIT 7,H"), 0xCB, 0x7C)
        IJR(T("JR Z,%s"), 0x28, 12)
        I2(T("LD A,45"), 0x3E, 0x2D)
        ICALLRT(RT_PUTC)
        ICALLRT(RT_NEGHL)
        ILOC(12)
        I2(T("LD C,0"), 0x0E, 0x00)
        I3(T("LD DE,%s"), "10000", 0x11, 0x10, 0x27)
        IJPLOC(T("CALL %s"), 0xCD, 13)
        I3(T("LD DE,%s"), "1000", 0x11, 0xE8, 0x03)
        IJPLOC(T("CALL %s"), 0xCD, 13)
        I3(T("LD DE,%s"), "100", 0x11, 0x64, 0x00)
        IJPLOC(T("CALL %s"), 0xCD, 13)
        I3(T("LD DE,%s"), "10", 0x11, 0x0A, 0x00)
        IJPLOC(T("CALL %s"), 0xCD, 13)
        I1(T("LD A,L"), 0x7D) I2(T("ADD A,48"), 0xC6, 0x30)
        IJPRT(RT_PUTC)
        ILOC(13)            /* un digito: cuantas veces cabe DE en HL */
        I2(T("LD B,0"), 0x06, 0x00)
        ILOC(14)
        I1(T("OR A"), 0xB7)
        I2(T("SBC HL,DE"), 0xED, 0x52)
        IJR(T("JR C,%s"), 0x38, 15)
        I1(T("INC B"), 0x04)
        IJR(T("JR %s"), 0x18, 14)
        ILOC(15)
        I1(T("ADD HL,DE"), 0x19)
        I1(T("LD A,B"), 0x78) I1(T("OR C"), 0xB1)
        I1(T("RET Z"), 0xC8)
        I2(T("LD C,1"), 0x0E, 0x01)
        I1(T("LD A,B"), 0x78) I2(T("ADD A,48"), 0xC6, 0x30)
        IJPRT(RT_PUTC)
        IFIN
};

/* RT_PUTL: escribe ' ' y T o F */
static const RtI rt_c_putl[] = {
        I2(T("LD A,32"), 0x3E, 0x20)
        ICALLRT(RT_PUTC)
        I1(T("LD A,L"), 0x7D) I1(T("OR A"), 0xB7)
        I2(T("LD A,84"), 0x3E, 0x54)
        IJR(T("JR NZ,%s"), 0x20, 16)
        I2(T("LD A,70"), 0x3E, 0x46)
        ILOC(16)
        IJPRT(RT_PUTC)
        IFIN
};

/* RT_PUTS: HL = datos, B = longitud; escribe ' ' y la cadena */
static const RtI rt_c_puts[] = {
        I2(T("LD A,32"), 0x3E, 0x20)
        ICALLRT(RT_PUTC)
        I1(T("LD A,B"), 0x78) I1(T("OR A"), 0xB7)
        I1(T("RET Z"), 0xC8)
        ILOC(17)
        I1(T("LD A,(HL)"), 0x7E)
        ICALLRT(RT_PUTC)
        I1(T("INC HL"), 0x23)
        IJR(T("DJNZ %s"), 0x10, 17)
        I1(T("RET"), 0xC9)
        IFIN
};

/* RT_SMOV: HL = origen, B = su longitud; DE = destino, C = su longitud: copia y rellena con blancos */
static const RtI rt_c_smov[] = {
        I1(T("LD A,B"), 0x78) I1(T("CP C"), 0xB9)
        IJR(T("JR C,%s"), 0x38, 30)
        I1(T("LD B,C"), 0x41)
        ILOC(30)
        I1(T("LD A,C"), 0x79) I1(T("SUB B"), 0x90)
        I1(T("PUSH AF"), 0xF5)                    /* blancos que faltan */
        I1(T("LD A,B"), 0x78) I1(T("OR A"), 0xB7)
        IJR(T("JR Z,%s"), 0x28, 31)
        I1(T("LD C,B"), 0x48) I2(T("LD B,0"), 0x06, 0x00)
        I2(T("LDIR"), 0xED, 0xB0)
        ILOC(31)
        I1(T("POP AF"), 0xF1)
        I1(T("LD B,A"), 0x47) I1(T("OR A"), 0xB7)
        I1(T("RET Z"), 0xC8)
        I2(T("LD A,32"), 0x3E, 0x20)
        ILOC(32)
        I1(T("LD (DE),A"), 0x12) I1(T("INC DE"), 0x13)
        IJR(T("DJNZ %s"), 0x10, 32)
        I1(T("RET"), 0xC9)
        IFIN
};

/* RT_SADD: HL = origen, B = longitud; DE = destino, C = hueco libre: anade y avanza DE, C */
static const RtI rt_c_sadd[] = {
        I1(T("LD A,B"), 0x78) I1(T("CP C"), 0xB9)
        IJR(T("JR C,%s"), 0x38, 33)
        I1(T("LD B,C"), 0x41)
        ILOC(33)
        I1(T("LD A,B"), 0x78) I1(T("OR A"), 0xB7)
        I1(T("RET Z"), 0xC8)
        I1(T("LD A,C"), 0x79) I1(T("SUB B"), 0x90)
        I1(T("PUSH AF"), 0xF5)
        I1(T("LD C,B"), 0x48) I2(T("LD B,0"), 0x06, 0x00)
        I2(T("LDIR"), 0xED, 0xB0)
        I1(T("POP AF"), 0xF1)
        I1(T("LD C,A"), 0x4F)
        I1(T("RET"), 0xC9)
        IFIN
};

/* RT_SCPY: HL = origen, DE = destino, BC = cuantos (puede ser 0) */
static const RtI rt_c_scpy[] = {
        I1(T("LD A,B"), 0x78) I1(T("OR C"), 0xB1)
        I1(T("RET Z"), 0xC8)
        I2(T("LDIR"), 0xED, 0xB0)
        I1(T("RET"), 0xC9)
        IFIN
};

/* RT_CLAMP: HL = cuenta con signo, C = tope: A = cuenta limitada a 0..tope */
static const RtI rt_c_clamp[] = {
        I2(T("BIT 7,H"), 0xCB, 0x7C)
        IJR(T("JR NZ,%s"), 0x20, 34)
        I1(T("LD A,H"), 0x7C) I1(T("OR A"), 0xB7)
        IJR(T("JR NZ,%s"), 0x20, 35)
        I1(T("LD A,L"), 0x7D) I1(T("CP C"), 0xB9)
        I1(T("RET C"), 0xD8)
        ILOC(35)
        I1(T("LD A,C"), 0x79)
        I1(T("RET"), 0xC9)
        ILOC(34)
        I1(T("XOR A"), 0xAF)
        I1(T("RET"), 0xC9)
        IFIN
};

/* RT_SCMP: (HL,B) con (DE,C), el corto relleno con blancos: HL = -1, 0 o 1 */
static const RtI rt_c_scmp[] = {
        ILOC(37)
        I1(T("LD A,B"), 0x78) I1(T("OR C"), 0xB1)
        IJR(T("JR Z,%s"), 0x28, 41)
        I1(T("LD A,B"), 0x78) I1(T("OR A"), 0xB7)
        IJR(T("JR Z,%s"), 0x28, 38)
        I1(T("LD A,C"), 0x79) I1(T("OR A"), 0xB7)
        IJR(T("JR Z,%s"), 0x28, 39)
        I1(T("LD A,(DE)"), 0x1A) I1(T("CP (HL)"), 0xBE)
        IJR(T("JR NZ,%s"), 0x20, 40)
        I1(T("INC HL"), 0x23) I1(T("INC DE"), 0x13) I1(T("DEC B"), 0x05) I1(T("DEC C"), 0x0D)
        IJR(T("JR %s"), 0x18, 37)
        ILOC(38)                            /* la primera se acabo: se compara ' ' con (DE) */
        I1(T("LD A,(DE)"), 0x1A) I2(T("CP 32"), 0xFE, 0x20)
        IJR(T("JR NZ,%s"), 0x20, 40)
        I1(T("INC DE"), 0x13) I1(T("DEC C"), 0x0D)
        IJR(T("JR %s"), 0x18, 37)
        ILOC(39)                            /* la segunda se acabo */
        I2(T("LD A,32"), 0x3E, 0x20) I1(T("CP (HL)"), 0xBE)
        IJR(T("JR NZ,%s"), 0x20, 40)
        I1(T("INC HL"), 0x23) I1(T("DEC B"), 0x05)
        IJR(T("JR %s"), 0x18, 37)
        ILOC(40)                            /* distintas: acarreo = (DE) < (HL) -> la primera es mayor */
        I3(T("LD HL,%s"), "1", 0x21, 0x01, 0x00)
        I1(T("RET C"), 0xD8)
        I3(T("LD HL,%s"), "$FFFF", 0x21, 0xFF, 0xFF)
        I1(T("RET"), 0xC9)
        ILOC(41)
        I3(T("LD HL,%s"), "0", 0x21, 0x00, 0x00)
        I1(T("RET"), 0xC9)
        IFIN
};

#ifndef MF_SIN_LISTADO          /* en el Next las tablas REAL viven en OTRO banco (back_rt_real.c) y se leen por trampolin */
#include "back_rt_real.h"
#include "back_rt_entrada.h"
#endif

static const RtI *const rt_cod[RT_N] = {
    0,
    rt_c_neghl, rt_c_negde, rt_c_mul, rt_c_div, rt_c_pow, rt_c_lt,
    rt_c_ge, rt_c_gt, rt_c_le, rt_c_eq, rt_c_ne, rt_c_puti,
    rt_c_putl, rt_c_puts, rt_c_smov, rt_c_sadd, rt_c_scpy, rt_c_clamp, rt_c_scmp,
#ifndef MF_SIN_LISTADO
    rt_c_fpd, rt_c_bshl, rt_c_bshr, rt_c_badd, rt_c_bsub, rt_c_funp, rt_c_fnorm, rt_c_fadd,
    rt_c_fmul, rt_c_fdiv, rt_c_fcmp, rt_c_itof, rt_c_ftoi, rt_c_fneg, rt_c_fput,
    rt_c_fint, rt_c_fsqrt, rt_c_fmod, rt_c_fpow, rt_c_err,
    rt_c_rdd, rt_c_keysc, rt_c_keyh, rt_c_keyt, rt_c_rdline, rt_c_rdln, rt_c_rdtok, rt_c_rdi, rt_c_rdl, rt_c_rds, rt_c_rdf
#endif
};
