/* back_rt_real.h - GENERADO por tools/rtgen/rtasm.py desde tools/rtgen/rt_real.asm. NO EDITAR A MANO.
 * Rutinas de apoyo del programa generado: 2596 B de codigo y datos en total. */

/* FPD: 176 B */
#define rt_n_fpd 3
static const RtI rt_c_fpd[] = {
        IDATA(176)
        IFIN
};

/* BSHL: 6 B */
#define rt_n_bshl 11
static const RtI rt_c_bshl[] = {
        I2(T("RL (HL)"), 0xCB, 0x16) I1(T("INC HL"), 0x23) I2(T("DJNZ $-3"), 0x10, 0xFB) I1(T("RET"), 0xC9)
        IFIN
};

/* BSHR: 6 B */
#define rt_n_bshr 11
static const RtI rt_c_bshr[] = {
        I2(T("RR (HL)"), 0xCB, 0x1E) I1(T("DEC HL"), 0x2B) I2(T("DJNZ $-3"), 0x10, 0xFB) I1(T("RET"), 0xC9)
        IFIN
};

/* BADD: 8 B */
#define rt_n_badd 16
static const RtI rt_c_badd[] = {
        I1(T("LD A,(DE)"), 0x1A) I1(T("ADC A,(HL)"), 0x8E) I1(T("LD (DE),A"), 0x12) I1(T("INC HL"), 0x23)
        I1(T("INC DE"), 0x13) I2(T("DJNZ $-5"), 0x10, 0xF9) I1(T("RET"), 0xC9)
        IFIN
};

/* BSUB: 8 B */
#define rt_n_bsub 16
static const RtI rt_c_bsub[] = {
        I1(T("LD A,(DE)"), 0x1A) I1(T("SBC A,(HL)"), 0x9E) I1(T("LD (DE),A"), 0x12) I1(T("INC HL"), 0x23)
        I1(T("INC DE"), 0x13) I2(T("DJNZ $-5"), 0x10, 0xF9) I1(T("RET"), 0xC9)
        IFIN
};

/* FUNP: 54 B */
#define rt_n_funp 101
static const RtI rt_c_funp[] = {
        I1(T("XOR A"), 0xAF) I1(T("LD (DE),A"), 0x12) I1(T("INC DE"), 0x13) I2(T("LDI"), 0xED, 0xA0)
        I2(T("LDI"), 0xED, 0xA0) I1(T("LD A,(HL)"), 0x7E) I1(T("INC HL"), 0x23) I1(T("LD C,A"), 0x4F)
        I1(T("LD A,(HL)"), 0x7E) I1(T("ADD A,A"), 0x87) I2(T("LD B,0"), 0x06, 0x00) I2(T("RL B"), 0xCB, 0x10)
        I2(T("BIT 7,C"), 0xCB, 0x79) I2(T("JR Z,$+3"), 0x28, 0x01) I1(T("INC A"), 0x3C) I1(T("OR A"), 0xB7)
        I2(T("JR Z,$+17"), 0x28, 0x0F) I1(T("LD H,A"), 0x67) I1(T("LD A,C"), 0x79) I2(T("OR 0x80"), 0xF6, 0x80)
        I1(T("LD (DE),A"), 0x12) I1(T("INC DE"), 0x13) I1(T("LD A,H"), 0x7C) I1(T("LD (DE),A"), 0x12)
        I1(T("INC DE"), 0x13) I1(T("XOR A"), 0xAF) I1(T("LD (DE),A"), 0x12) I1(T("INC DE"), 0x13)
        I1(T("LD A,B"), 0x78) I1(T("LD (DE),A"), 0x12) I1(T("RET"), 0xC9) I1(T("LD (DE),A"), 0x12)
        I1(T("DEC DE"), 0x1B) I1(T("LD (DE),A"), 0x12) I1(T("DEC DE"), 0x1B) I1(T("LD (DE),A"), 0x12)
        I1(T("INC DE"), 0x13) I1(T("INC DE"), 0x13) I1(T("INC DE"), 0x13) I1(T("LD (DE),A"), 0x12)
        I1(T("INC DE"), 0x13) I1(T("LD (DE),A"), 0x12) I1(T("INC DE"), 0x13) I1(T("LD A,B"), 0x78)
        I1(T("LD (DE),A"), 0x12) I1(T("RET"), 0xC9)
        IFIN
};

/* FNORM: 154 B */
#define rt_n_fnorm 221
static const RtI rt_c_fnorm[] = {
        IREFO(T("LD HL,%s"), "rt_fs", 0x21, 0) I1(T("LD A,(HL)"), 0x7E) I1(T("INC HL"), 0x23) I1(T("OR (HL)"), 0xB6)
        I1(T("INC HL"), 0x23) I1(T("OR (HL)"), 0xB6) I1(T("INC HL"), 0x23) I1(T("OR (HL)"), 0xB6)
        I2(T("JR NZ,$+18"), 0x20, 0x10) IREFO(T("LD HL,(%s)"), "rt_fs+16", 0x2A, 16) I1(T("XOR A"), 0xAF)
        I1(T("LD (HL),A"), 0x77) I1(T("INC HL"), 0x23) I1(T("LD (HL),A"), 0x77) I1(T("INC HL"), 0x23)
        I1(T("LD (HL),A"), 0x77) I1(T("INC HL"), 0x23) IREFO(T("LD A,(%s)"), "rt_fs+6", 0x3A, 6) I1(T("RRCA"), 0x0F)
        I1(T("LD (HL),A"), 0x77) I1(T("RET"), 0xC9) IREFO(T("LD A,(%s)"), "rt_fs+3", 0x3A, 3)
        I2(T("BIT 7,A"), 0xCB, 0x7F) I2(T("JR NZ,$+20"), 0x20, 0x12) IREFO(T("LD HL,%s"), "rt_fs", 0x21, 0)
        I2(T("LD B,4"), 0x06, 0x04) I1(T("OR A"), 0xB7) ICALLRT(RT_BSHL) IREFO(T("LD HL,(%s)"), "rt_fs+4", 0x2A, 4)
        I1(T("DEC HL"), 0x2B) IREFO(T("LD (%s),HL"), "rt_fs+4", 0x22, 4) I2(T("JR $-23"), 0x18, 0xE7)
        IREFO(T("LD A,(%s)"), "rt_fs", 0x3A, 0) I2(T("CP 0x80"), 0xFE, 0x80) I2(T("JR C,$+34"), 0x38, 0x20)
        I2(T("JR NZ,$+9"), 0x20, 0x07) IREFO(T("LD A,(%s)"), "rt_fs+1", 0x3A, 1) I2(T("AND 1"), 0xE6, 0x01)
        I2(T("JR Z,$+25"), 0x28, 0x17) IREFO(T("LD HL,%s"), "rt_fs+1", 0x21, 1) I1(T("INC (HL)"), 0x34)
        I2(T("JR NZ,$+19"), 0x20, 0x11) I1(T("INC HL"), 0x23) I1(T("INC (HL)"), 0x34) I2(T("JR NZ,$+15"), 0x20, 0x0D)
        I1(T("INC HL"), 0x23) I1(T("INC (HL)"), 0x34) I2(T("JR NZ,$+11"), 0x20, 0x09)
        I2(T("LD (HL),0x80"), 0x36, 0x80) IREFO(T("LD HL,(%s)"), "rt_fs+4", 0x2A, 4) I1(T("INC HL"), 0x23)
        IREFO(T("LD (%s),HL"), "rt_fs+4", 0x22, 4) IREFO(T("LD HL,(%s)"), "rt_fs+4", 0x2A, 4)
        I2(T("BIT 7,H"), 0xCB, 0x7C) I2(T("JR NZ,$-85"), 0x20, 0xA9) I1(T("LD A,H"), 0x7C) I1(T("OR A"), 0xB7)
        I2(T("JR NZ,$+48"), 0x20, 0x2E) I1(T("OR L"), 0xB5) I2(T("JR Z,$-92"), 0x28, 0xA2) I1(T("INC A"), 0x3C)
        I2(T("JR Z,$+42"), 0x28, 0x28) IREFO(T("LD HL,(%s)"), "rt_fs+16", 0x2A, 16)
        IREFO(T("LD A,(%s)"), "rt_fs+1", 0x3A, 1) I1(T("LD (HL),A"), 0x77) I1(T("INC HL"), 0x23)
        IREFO(T("LD A,(%s)"), "rt_fs+2", 0x3A, 2) I1(T("LD (HL),A"), 0x77) I1(T("INC HL"), 0x23)
        IREFO(T("LD A,(%s)"), "rt_fs+4", 0x3A, 4) I1(T("RRCA"), 0x0F) I1(T("LD C,A"), 0x4F)
        I2(T("AND 0x80"), 0xE6, 0x80) I1(T("LD B,A"), 0x47) IREFO(T("LD A,(%s)"), "rt_fs+3", 0x3A, 3)
        I2(T("AND 0x7F"), 0xE6, 0x7F) I1(T("OR B"), 0xB0) I1(T("LD (HL),A"), 0x77) I1(T("INC HL"), 0x23)
        I1(T("LD A,C"), 0x79) I2(T("AND 0x7F"), 0xE6, 0x7F) I1(T("LD C,A"), 0x4F)
        IREFO(T("LD A,(%s)"), "rt_fs+6", 0x3A, 6) I1(T("RRCA"), 0x0F) I1(T("OR C"), 0xB1) I1(T("LD (HL),A"), 0x77)
        I1(T("RET"), 0xC9) I2(T("LD A,3"), 0x3E, 0x03) IJPRT(RT_ERR)
        IFIN
};

/* FADD: 237 B */
#define rt_n_fadd 304
static const RtI rt_c_fadd[] = {
        IREFO(T("LD (%s),A"), "rt_fs+18", 0x32, 18) IREFE(T("LD (%s),BC"), "rt_fs+16", 0xED, 0x43, 16)
        I1(T("PUSH DE"), 0xD5) IREFO(T("LD DE,%s"), "rt_fs", 0x11, 0) ICALLRT(RT_FUNP) I1(T("POP HL"), 0xE1)
        IREFO(T("LD DE,%s"), "rt_fs+8", 0x11, 8) ICALLRT(RT_FUNP) IREFO(T("LD A,(%s)"), "rt_fs+18", 0x3A, 18)
        I1(T("OR A"), 0xB7) I2(T("JR Z,$+10"), 0x28, 0x08) IREFO(T("LD A,(%s)"), "rt_fs+14", 0x3A, 14)
        I2(T("XOR 1"), 0xEE, 0x01) IREFO(T("LD (%s),A"), "rt_fs+14", 0x32, 14)
        IREFO(T("LD A,(%s)"), "rt_fs+4", 0x3A, 4) I1(T("OR A"), 0xB7) I2(T("JR Z,$+11"), 0x28, 0x09)
        IREFO(T("LD A,(%s)"), "rt_fs+12", 0x3A, 12) I1(T("OR A"), 0xB7) I2(T("JR NZ,$+37"), 0x20, 0x23)
        IJPRT(RT_FNORM) IREFO(T("LD A,(%s)"), "rt_fs+12", 0x3A, 12) I1(T("OR A"), 0xB7)
        I2(T("JR NZ,$+15"), 0x20, 0x0D) IREFO(T("LD A,(%s)"), "rt_fs+6", 0x3A, 6) I1(T("LD B,A"), 0x47)
        IREFO(T("LD A,(%s)"), "rt_fs+14", 0x3A, 14) I1(T("AND B"), 0xA0) IREFO(T("LD (%s),A"), "rt_fs+6", 0x32, 6)
        I2(T("JR $-20"), 0x18, 0xEA) IREFO(T("LD HL,%s"), "rt_fs+8", 0x21, 8) IREFO(T("LD DE,%s"), "rt_fs", 0x11, 0)
        I3(T("LD BC,%s"), "8", 0x01, 0x08, 0x00) I2(T("LDIR"), 0xED, 0xB0) I2(T("JR $-33"), 0x18, 0xDD)
        I1(T("LD B,A"), 0x47) IREFO(T("LD A,(%s)"), "rt_fs+4", 0x3A, 4) I1(T("CP B"), 0xB8)
        I2(T("JR NC,$+19"), 0x30, 0x11) IREFO(T("LD HL,%s"), "rt_fs", 0x21, 0)
        IREFO(T("LD DE,%s"), "rt_fs+8", 0x11, 8) I2(T("LD B,8"), 0x06, 0x08) I1(T("LD A,(DE)"), 0x1A)
        I1(T("LD C,(HL)"), 0x4E) I1(T("LD (HL),A"), 0x77) I1(T("LD A,C"), 0x79) I1(T("LD (DE),A"), 0x12)
        I1(T("INC HL"), 0x23) I1(T("INC DE"), 0x13) I2(T("DJNZ $-7"), 0x10, 0xF7)
        IREFO(T("LD A,(%s)"), "rt_fs+12", 0x3A, 12) I1(T("LD B,A"), 0x47) IREFO(T("LD A,(%s)"), "rt_fs+4", 0x3A, 4)
        I1(T("SUB B"), 0x90) I2(T("JR Z,$+23"), 0x28, 0x15) I1(T("LD B,A"), 0x47)
        IREFO(T("LD HL,%s"), "rt_fs+11", 0x21, 11) I2(T("SRL (HL)"), 0xCB, 0x3E) I1(T("DEC HL"), 0x2B)
        I2(T("RR (HL)"), 0xCB, 0x1E) I1(T("DEC HL"), 0x2B) I2(T("RR (HL)"), 0xCB, 0x1E) I1(T("DEC HL"), 0x2B)
        I2(T("RR (HL)"), 0xCB, 0x1E) I2(T("JR NC,$+4"), 0x30, 0x02) I2(T("SET 0,(HL)"), 0xCB, 0xC6)
        I2(T("DJNZ $-18"), 0x10, 0xEC) IREFO(T("LD A,(%s)"), "rt_fs+6", 0x3A, 6) I1(T("LD B,A"), 0x47)
        IREFO(T("LD A,(%s)"), "rt_fs+14", 0x3A, 14) I1(T("XOR B"), 0xA8) I2(T("JR NZ,$+42"), 0x20, 0x28)
        IREFO(T("LD HL,%s"), "rt_fs+8", 0x21, 8) IREFO(T("LD DE,%s"), "rt_fs", 0x11, 0) I2(T("LD B,4"), 0x06, 0x04)
        I1(T("OR A"), 0xB7) ICALLRT(RT_BADD) I2(T("JR NC,$+25"), 0x30, 0x17) IREFO(T("LD HL,%s"), "rt_fs+3", 0x21, 3)
        I2(T("LD B,4"), 0x06, 0x04) I1(T("SCF"), 0x37) ICALLRT(RT_BSHR) I2(T("JR NC,$+7"), 0x30, 0x05)
        IREFO(T("LD HL,%s"), "rt_fs", 0x21, 0) I2(T("SET 0,(HL)"), 0xCB, 0xC6)
        IREFO(T("LD HL,(%s)"), "rt_fs+4", 0x2A, 4) I1(T("INC HL"), 0x23) IREFO(T("LD (%s),HL"), "rt_fs+4", 0x22, 4)
        IJPRT(RT_FNORM) IREFO(T("LD HL,%s"), "rt_fs+8", 0x21, 8) IREFO(T("LD DE,%s"), "rt_fs", 0x11, 0)
        I2(T("LD B,4"), 0x06, 0x04) I1(T("OR A"), 0xB7) ICALLRT(RT_BSUB) I2(T("JR NC,$+21"), 0x30, 0x13)
        IREFO(T("LD HL,%s"), "rt_fs", 0x21, 0) I2(T("LD B,4"), 0x06, 0x04) I1(T("OR A"), 0xB7)
        I2(T("LD A,0"), 0x3E, 0x00) I1(T("SBC A,(HL)"), 0x9E) I1(T("LD (HL),A"), 0x77) I1(T("INC HL"), 0x23)
        I2(T("DJNZ $-5"), 0x10, 0xF9) IREFO(T("LD A,(%s)"), "rt_fs+14", 0x3A, 14)
        IREFO(T("LD (%s),A"), "rt_fs+6", 0x32, 6) IREFO(T("LD HL,%s"), "rt_fs", 0x21, 0) I1(T("LD A,(HL)"), 0x7E)
        I1(T("INC HL"), 0x23) I1(T("OR (HL)"), 0xB6) I1(T("INC HL"), 0x23) I1(T("OR (HL)"), 0xB6)
        I1(T("INC HL"), 0x23) I1(T("OR (HL)"), 0xB6) I2(T("JR NZ,$-46"), 0x20, 0xD0)
        IREFO(T("LD (%s),A"), "rt_fs+6", 0x32, 6) I2(T("JR $-51"), 0x18, 0xCB)
        IFIN
};

/* FMUL: 155 B */
#define rt_n_fmul 183
static const RtI rt_c_fmul[] = {
        IREFE(T("LD (%s),BC"), "rt_fs+16", 0xED, 0x43, 16) I1(T("PUSH DE"), 0xD5)
        IREFO(T("LD DE,%s"), "rt_fs", 0x11, 0) ICALLRT(RT_FUNP) I1(T("POP HL"), 0xE1)
        IREFO(T("LD DE,%s"), "rt_fs+8", 0x11, 8) ICALLRT(RT_FUNP) IREFO(T("LD A,(%s)"), "rt_fs+6", 0x3A, 6)
        I1(T("LD B,A"), 0x47) IREFO(T("LD A,(%s)"), "rt_fs+14", 0x3A, 14) I1(T("XOR B"), 0xA8)
        IREFO(T("LD (%s),A"), "rt_fs+6", 0x32, 6) IREFO(T("LD A,(%s)"), "rt_fs+4", 0x3A, 4) I1(T("OR A"), 0xB7)
        I2(T("JR Z,$+8"), 0x28, 0x06) IREFO(T("LD A,(%s)"), "rt_fs+12", 0x3A, 12) I1(T("OR A"), 0xB7)
        I2(T("JR NZ,$+15"), 0x20, 0x0D) IREFO(T("LD HL,%s"), "rt_fs", 0x21, 0) I2(T("LD B,4"), 0x06, 0x04)
        I2(T("LD (HL),0"), 0x36, 0x00) I1(T("INC HL"), 0x23) I2(T("DJNZ $-3"), 0x10, 0xFB) IJPRT(RT_FNORM)
        IREFO(T("LD HL,%s"), "rt_fs+9", 0x21, 9) IREFO(T("LD DE,%s"), "rt_fs+20", 0x11, 20)
        I3(T("LD BC,%s"), "3", 0x01, 0x03, 0x00) I2(T("LDIR"), 0xED, 0xB0) I1(T("XOR A"), 0xAF)
        IREFO(T("LD (%s),A"), "rt_fs+23", 0x32, 23) IREFO(T("LD (%s),A"), "rt_fs+24", 0x32, 24)
        IREFO(T("LD (%s),A"), "rt_fs+25", 0x32, 25) I2(T("LD A,24"), 0x3E, 0x18)
        IREFO(T("LD (%s),A"), "rt_fs+19", 0x32, 19) I1(T("OR A"), 0xB7) IREFO(T("LD HL,%s"), "rt_fs+20", 0x21, 20)
        I2(T("BIT 0,(HL)"), 0xCB, 0x46) I2(T("JR Z,$+13"), 0x28, 0x0B) IREFO(T("LD HL,%s"), "rt_fs+1", 0x21, 1)
        IREFO(T("LD DE,%s"), "rt_fs+23", 0x11, 23) I2(T("LD B,3"), 0x06, 0x03) ICALLRT(RT_BADD)
        IREFO(T("LD HL,%s"), "rt_fs+25", 0x21, 25) I2(T("LD B,6"), 0x06, 0x06) ICALLRT(RT_BSHR)
        IREFO(T("LD HL,%s"), "rt_fs+19", 0x21, 19) I1(T("DEC (HL)"), 0x35) I2(T("JR NZ,$-31"), 0x20, 0xDF)
        IREFO(T("LD HL,%s"), "rt_fs+22", 0x21, 22) IREFO(T("LD DE,%s"), "rt_fs", 0x11, 0)
        I3(T("LD BC,%s"), "4", 0x01, 0x04, 0x00) I2(T("LDIR"), 0xED, 0xB0) IREFO(T("LD HL,%s"), "rt_fs+20", 0x21, 20)
        I1(T("LD A,(HL)"), 0x7E) I1(T("INC HL"), 0x23) I1(T("OR (HL)"), 0xB6) I2(T("JR Z,$+7"), 0x28, 0x05)
        IREFO(T("LD HL,%s"), "rt_fs", 0x21, 0) I2(T("SET 0,(HL)"), 0xCB, 0xC6)
        IREFO(T("LD HL,(%s)"), "rt_fs+4", 0x2A, 4) IREFE(T("LD DE,(%s)"), "rt_fs+12", 0xED, 0x5B, 12)
        I1(T("ADD HL,DE"), 0x19) I3(T("LD DE,%s"), "65410", 0x11, 0x82, 0xFF) I1(T("ADD HL,DE"), 0x19)
        IREFO(T("LD (%s),HL"), "rt_fs+4", 0x22, 4) IJPRT(RT_FNORM)
        IFIN
};

/* FDIV: 214 B */
#define rt_n_fdiv 263
static const RtI rt_c_fdiv[] = {
        IREFE(T("LD (%s),BC"), "rt_fs+16", 0xED, 0x43, 16) I1(T("PUSH DE"), 0xD5)
        IREFO(T("LD DE,%s"), "rt_fs", 0x11, 0) ICALLRT(RT_FUNP) I1(T("POP HL"), 0xE1)
        IREFO(T("LD DE,%s"), "rt_fs+8", 0x11, 8) ICALLRT(RT_FUNP) IREFO(T("LD A,(%s)"), "rt_fs+6", 0x3A, 6)
        I1(T("LD B,A"), 0x47) IREFO(T("LD A,(%s)"), "rt_fs+14", 0x3A, 14) I1(T("XOR B"), 0xA8)
        IREFO(T("LD (%s),A"), "rt_fs+6", 0x32, 6) IREFO(T("LD A,(%s)"), "rt_fs+12", 0x3A, 12) I1(T("OR A"), 0xB7)
        I2(T("JR Z,$+21"), 0x28, 0x13) IREFO(T("LD A,(%s)"), "rt_fs+4", 0x3A, 4) I1(T("OR A"), 0xB7)
        I2(T("JR NZ,$+27"), 0x20, 0x19) IREFO(T("LD HL,%s"), "rt_fs", 0x21, 0) I2(T("LD B,4"), 0x06, 0x04)
        I2(T("LD (HL),0"), 0x36, 0x00) I1(T("INC HL"), 0x23) I2(T("DJNZ $-3"), 0x10, 0xFB) IJPRT(RT_FNORM)
        IREFO(T("LD A,(%s)"), "rt_fs+4", 0x3A, 4) I1(T("OR A"), 0xB7) I2(T("LD A,1"), 0x3E, 0x01)
        I2(T("JR NZ,$+3"), 0x20, 0x01) I1(T("INC A"), 0x3C) IJPRT(RT_ERR) IREFO(T("LD HL,%s"), "rt_fs+1", 0x21, 1)
        IREFO(T("LD DE,%s"), "rt_fs+20", 0x11, 20) I3(T("LD BC,%s"), "3", 0x01, 0x03, 0x00) I2(T("LDIR"), 0xED, 0xB0)
        I1(T("XOR A"), 0xAF) IREFO(T("LD (%s),A"), "rt_fs+23", 0x32, 23) IREFO(T("LD HL,%s"), "rt_fs+9", 0x21, 9)
        IREFO(T("LD DE,%s"), "rt_fs+24", 0x11, 24) I3(T("LD BC,%s"), "3", 0x01, 0x03, 0x00) I2(T("LDIR"), 0xED, 0xB0)
        I1(T("XOR A"), 0xAF) IREFO(T("LD (%s),A"), "rt_fs+27", 0x32, 27) IREFO(T("LD HL,%s"), "rt_fs", 0x21, 0)
        I2(T("LD B,4"), 0x06, 0x04) I1(T("LD (HL),A"), 0x77) I1(T("INC HL"), 0x23) I2(T("DJNZ $-2"), 0x10, 0xFC)
        I2(T("LD A,26"), 0x3E, 0x1A) IREFO(T("LD (%s),A"), "rt_fs+19", 0x32, 19)
        IREFO(T("LD HL,%s"), "rt_fs+24", 0x21, 24) IREFO(T("LD DE,%s"), "rt_fs+20", 0x11, 20)
        I2(T("LD B,4"), 0x06, 0x04) I1(T("OR A"), 0xB7) ICALLRT(RT_BSUB) I2(T("JR NC,$+17"), 0x30, 0x0F)
        IREFO(T("LD HL,%s"), "rt_fs+24", 0x21, 24) IREFO(T("LD DE,%s"), "rt_fs+20", 0x11, 20)
        I2(T("LD B,4"), 0x06, 0x04) I1(T("OR A"), 0xB7) ICALLRT(RT_BADD) I1(T("OR A"), 0xB7)
        I2(T("JR $+3"), 0x18, 0x01) I1(T("SCF"), 0x37) IREFO(T("LD HL,%s"), "rt_fs", 0x21, 0)
        I2(T("LD B,4"), 0x06, 0x04) ICALLRT(RT_BSHL) IREFO(T("LD HL,%s"), "rt_fs+20", 0x21, 20)
        I2(T("LD B,4"), 0x06, 0x04) I1(T("OR A"), 0xB7) ICALLRT(RT_BSHL) IREFO(T("LD HL,%s"), "rt_fs+19", 0x21, 19)
        I1(T("DEC (HL)"), 0x35) I2(T("JR NZ,$-51"), 0x20, 0xCB) I2(T("LD C,6"), 0x0E, 0x06)
        IREFO(T("LD HL,%s"), "rt_fs", 0x21, 0) I2(T("LD B,4"), 0x06, 0x04) I1(T("OR A"), 0xB7) ICALLRT(RT_BSHL)
        I1(T("DEC C"), 0x0D) I2(T("JR NZ,$-10"), 0x20, 0xF4) IREFO(T("LD HL,%s"), "rt_fs+20", 0x21, 20)
        I1(T("LD A,(HL)"), 0x7E) I1(T("INC HL"), 0x23) I1(T("OR (HL)"), 0xB6) I1(T("INC HL"), 0x23)
        I1(T("OR (HL)"), 0xB6) I1(T("INC HL"), 0x23) I1(T("OR (HL)"), 0xB6) I2(T("JR Z,$+7"), 0x28, 0x05)
        IREFO(T("LD HL,%s"), "rt_fs", 0x21, 0) I2(T("SET 0,(HL)"), 0xCB, 0xC6)
        IREFO(T("LD HL,(%s)"), "rt_fs+4", 0x2A, 4) IREFE(T("LD DE,(%s)"), "rt_fs+12", 0xED, 0x5B, 12)
        I1(T("OR A"), 0xB7) I2(T("SBC HL,DE"), 0xED, 0x52) I3(T("LD DE,%s"), "127", 0x11, 0x7F, 0x00)
        I1(T("ADD HL,DE"), 0x19) IREFO(T("LD (%s),HL"), "rt_fs+4", 0x22, 4) IJPRT(RT_FNORM)
        IFIN
};

/* FCMP: 83 B */
#define rt_n_fcmp 113
static const RtI rt_c_fcmp[] = {
        I1(T("PUSH DE"), 0xD5) IREFO(T("LD DE,%s"), "rt_fs", 0x11, 0) ICALLRT(RT_FUNP) I1(T("POP HL"), 0xE1)
        IREFO(T("LD DE,%s"), "rt_fs+8", 0x11, 8) ICALLRT(RT_FUNP) IREFO(T("LD A,(%s)"), "rt_fs+4", 0x3A, 4)
        I1(T("OR A"), 0xB7) I2(T("JR NZ,$+5"), 0x20, 0x03) IREFO(T("LD (%s),A"), "rt_fs+6", 0x32, 6)
        IREFO(T("LD A,(%s)"), "rt_fs+12", 0x3A, 12) I1(T("OR A"), 0xB7) I2(T("JR NZ,$+5"), 0x20, 0x03)
        IREFO(T("LD (%s),A"), "rt_fs+14", 0x32, 14) IREFO(T("LD A,(%s)"), "rt_fs+6", 0x3A, 6) I1(T("LD B,A"), 0x47)
        IREFO(T("LD A,(%s)"), "rt_fs+14", 0x3A, 14) I1(T("SUB B"), 0x90) I2(T("JR Z,$+7"), 0x28, 0x05)
        I1(T("LD L,A"), 0x6F) I1(T("ADD A,A"), 0x87) I1(T("SBC A,A"), 0x9F) I1(T("LD H,A"), 0x67) I1(T("RET"), 0xC9)
        IREFO(T("LD HL,%s"), "rt_fs+4", 0x21, 4) IREFO(T("LD DE,%s"), "rt_fs+12", 0x11, 12)
        I2(T("LD B,4"), 0x06, 0x04) I1(T("LD A,(DE)"), 0x1A) I1(T("CP (HL)"), 0xBE) I2(T("JR NZ,$+10"), 0x20, 0x08)
        I1(T("DEC HL"), 0x2B) I1(T("DEC DE"), 0x1B) I2(T("DJNZ $-6"), 0x10, 0xF8)
        I3(T("LD HL,%s"), "0", 0x21, 0x00, 0x00) I1(T("RET"), 0xC9) I1(T("SBC A,A"), 0x9F) I2(T("AND 1"), 0xE6, 0x01)
        I1(T("LD B,A"), 0x47) IREFO(T("LD A,(%s)"), "rt_fs+6", 0x3A, 6) I1(T("XOR B"), 0xA8)
        I3(T("LD HL,%s"), "65535", 0x21, 0xFF, 0xFF) I1(T("RET Z"), 0xC8) I3(T("LD HL,%s"), "1", 0x21, 0x01, 0x00)
        I1(T("RET"), 0xC9)
        IFIN
};

/* ITOF: 47 B */
#define rt_n_itof 61
static const RtI rt_c_itof[] = {
        IREFE(T("LD (%s),BC"), "rt_fs+16", 0xED, 0x43, 16) I1(T("XOR A"), 0xAF)
        IREFO(T("LD (%s),A"), "rt_fs", 0x32, 0) IREFO(T("LD (%s),A"), "rt_fs+1", 0x32, 1)
        IREFO(T("LD (%s),A"), "rt_fs+6", 0x32, 6) I2(T("BIT 7,H"), 0xCB, 0x7C) I2(T("JR Z,$+14"), 0x28, 0x0C)
        I1(T("LD A,L"), 0x7D) I1(T("CPL"), 0x2F) I1(T("LD L,A"), 0x6F) I1(T("LD A,H"), 0x7C) I1(T("CPL"), 0x2F)
        I1(T("LD H,A"), 0x67) I1(T("INC HL"), 0x23) I2(T("LD A,1"), 0x3E, 0x01)
        IREFO(T("LD (%s),A"), "rt_fs+6", 0x32, 6) I1(T("LD A,H"), 0x7C) IREFO(T("LD (%s),A"), "rt_fs+3", 0x32, 3)
        I1(T("LD A,L"), 0x7D) IREFO(T("LD (%s),A"), "rt_fs+2", 0x32, 2) I3(T("LD HL,%s"), "142", 0x21, 0x8E, 0x00)
        IREFO(T("LD (%s),HL"), "rt_fs+4", 0x22, 4) IJPRT(RT_FNORM)
        IFIN
};

/* FTOI: 52 B */
#define rt_n_ftoi 76
static const RtI rt_c_ftoi[] = {
        IREFO(T("LD DE,%s"), "rt_fs", 0x11, 0) ICALLRT(RT_FUNP) IREFO(T("LD A,(%s)"), "rt_fs+4", 0x3A, 4)
        I2(T("CP 142"), 0xFE, 0x8E) I2(T("JR NC,$+29"), 0x30, 0x1B) I1(T("LD C,A"), 0x4F)
        I2(T("LD A,142"), 0x3E, 0x8E) I1(T("SUB C"), 0x91) I1(T("LD B,A"), 0x47)
        IREFO(T("LD HL,(%s)"), "rt_fs+2", 0x2A, 2) I2(T("SRL H"), 0xCB, 0x3C) I2(T("RR L"), 0xCB, 0x1D)
        I2(T("DJNZ $-4"), 0x10, 0xFA) IREFO(T("LD A,(%s)"), "rt_fs+6", 0x3A, 6) I1(T("OR A"), 0xB7)
        I1(T("RET Z"), 0xC8) I1(T("LD A,L"), 0x7D) I1(T("CPL"), 0x2F) I1(T("LD L,A"), 0x6F) I1(T("LD A,H"), 0x7C)
        I1(T("CPL"), 0x2F) I1(T("LD H,A"), 0x67) I1(T("INC HL"), 0x23) I1(T("RET"), 0xC9)
        IREFO(T("LD A,(%s)"), "rt_fs+6", 0x3A, 6) I1(T("OR A"), 0xB7) I3(T("LD HL,%s"), "32767", 0x21, 0xFF, 0x7F)
        I1(T("RET Z"), 0xC8) I3(T("LD HL,%s"), "32768", 0x21, 0x00, 0x80) I1(T("RET"), 0xC9)
        IFIN
};

/* FNEG: 13 B */
#define rt_n_fneg 23
static const RtI rt_c_fneg[] = {
        I1(T("LD D,B"), 0x50) I1(T("LD E,C"), 0x59) I3(T("LD BC,%s"), "4", 0x01, 0x04, 0x00) I2(T("LDIR"), 0xED, 0xB0)
        I1(T("DEC DE"), 0x1B) I1(T("LD A,(DE)"), 0x1A) I2(T("XOR 0x80"), 0xEE, 0x80) I1(T("LD (DE),A"), 0x12)
        I1(T("RET"), 0xC9)
        IFIN
};

/* FPUT: 541 B */
#define rt_n_fput 718
static const RtI rt_c_fput[] = {
        I1(T("PUSH HL"), 0xE5) I2(T("LD A,32"), 0x3E, 0x20) ICALLRT(RT_PUTC) I1(T("POP HL"), 0xE1)
        IREFO(T("LD DE,%s"), "rt_fs", 0x11, 0) ICALLRT(RT_FUNP) IREFO(T("LD A,(%s)"), "rt_fs+6", 0x3A, 6)
        I1(T("OR A"), 0xB7) I2(T("JR Z,$+7"), 0x28, 0x05) I2(T("LD A,45"), 0x3E, 0x2D) ICALLRT(RT_PUTC)
        IREFO(T("LD A,(%s)"), "rt_fs+4", 0x3A, 4) I1(T("OR A"), 0xB7) I2(T("JR NZ,$+22"), 0x20, 0x14)
        I2(T("LD A,48"), 0x3E, 0x30) ICALLRT(RT_PUTC) I2(T("LD A,46"), 0x3E, 0x2E) ICALLRT(RT_PUTC)
        I2(T("LD B,8"), 0x06, 0x08) I2(T("LD A,48"), 0x3E, 0x30) ICALLRT(RT_PUTC) I2(T("DJNZ $-5"), 0x10, 0xF9)
        I1(T("RET"), 0xC9) IREFO(T("LD HL,%s"), "rt_fs+48", 0x21, 48) I2(T("LD (HL),0"), 0x36, 0x00)
        IREFO(T("LD DE,%s"), "rt_fs+49", 0x11, 49) I3(T("LD BC,%s"), "118", 0x01, 0x76, 0x00)
        I2(T("LDIR"), 0xED, 0xB0) IREFO(T("LD A,(%s)"), "rt_fs+4", 0x3A, 4) I2(T("ADD A,2"), 0xC6, 0x02)
        I1(T("LD C,A"), 0x4F) I1(T("RRA"), 0x1F) I2(T("SRL A"), 0xCB, 0x3F) I2(T("SRL A"), 0xCB, 0x3F)
        I1(T("LD E,A"), 0x5F) I2(T("LD D,0"), 0x16, 0x00) IREFO(T("LD HL,%s"), "rt_fs+48", 0x21, 48)
        I1(T("ADD HL,DE"), 0x19) I1(T("EX DE,HL"), 0xEB) IREFO(T("LD HL,%s"), "rt_fs+1", 0x21, 1)
        I1(T("PUSH BC"), 0xC5) I3(T("LD BC,%s"), "3", 0x01, 0x03, 0x00) I2(T("LDIR"), 0xED, 0xB0)
        I1(T("POP BC"), 0xC1) I1(T("LD A,C"), 0x79) I2(T("AND 7"), 0xE6, 0x07) I2(T("JR Z,$+15"), 0x28, 0x0D)
        I1(T("LD C,A"), 0x4F) IREFO(T("LD HL,%s"), "rt_fs+48", 0x21, 48) I2(T("LD B,35"), 0x06, 0x23)
        I1(T("OR A"), 0xB7) ICALLRT(RT_BSHL) I1(T("DEC C"), 0x0D) I2(T("JR NZ,$-10"), 0x20, 0xF4)
        IREFO(T("LD HL,%s"), "rt_fs+151", 0x21, 151) IREFO(T("LD (%s),HL"), "rt_fs+169", 0x22, 169)
        IREFO(T("LD (%s),HL"), "rt_fs+167", 0x22, 167) I1(T("XOR A"), 0xAF)
        IREFO(T("LD (%s),A"), "rt_fs+171", 0x32, 171) IREFO(T("LD (%s),A"), "rt_fs+172", 0x32, 172)
        IREFO(T("LD (%s),A"), "rt_fs+173", 0x32, 173) IREFO(T("LD HL,%s"), "rt_fs+67", 0x21, 67)
        I2(T("LD B,16"), 0x06, 0x10) I1(T("OR (HL)"), 0xB6) I1(T("INC HL"), 0x23) I2(T("DJNZ $-2"), 0x10, 0xFC)
        I2(T("JR Z,$+52"), 0x28, 0x32) IREFO(T("LD HL,%s"), "rt_fs+82", 0x21, 82) I2(T("LD D,16"), 0x16, 0x10)
        I1(T("XOR A"), 0xAF) I1(T("LD C,(HL)"), 0x4E) I2(T("LD B,8"), 0x06, 0x08) I2(T("SLA C"), 0xCB, 0x21)
        I1(T("RLA"), 0x17) I2(T("CP 10"), 0xFE, 0x0A) I2(T("JR C,$+5"), 0x38, 0x03) I2(T("SUB 10"), 0xD6, 0x0A)
        I1(T("INC C"), 0x0C) I2(T("DJNZ $-10"), 0x10, 0xF4) I1(T("LD (HL),C"), 0x71) I1(T("DEC HL"), 0x2B)
        I1(T("DEC D"), 0x15) I2(T("JR NZ,$-18"), 0x20, 0xEC) IREFO(T("LD HL,(%s)"), "rt_fs+167", 0x2A, 167)
        I1(T("DEC HL"), 0x2B) I1(T("LD (HL),A"), 0x77) IREFO(T("LD (%s),HL"), "rt_fs+167", 0x22, 167)
        IREFO(T("LD HL,%s"), "rt_fs+172", 0x21, 172) I1(T("INC (HL)"), 0x34)
        IREFO(T("LD HL,%s"), "rt_fs+67", 0x21, 67) I2(T("LD B,16"), 0x06, 0x10) I1(T("XOR A"), 0xAF)
        I1(T("OR (HL)"), 0xB6) I1(T("INC HL"), 0x23) I2(T("DJNZ $-2"), 0x10, 0xFC) I2(T("JR NZ,$-48"), 0x20, 0xCE)
        IREFO(T("LD A,(%s)"), "rt_fs+172", 0x3A, 172) IREFO(T("LD (%s),A"), "rt_fs+171", 0x32, 171)
        IREFO(T("LD A,(%s)"), "rt_fs+171", 0x3A, 171) I2(T("CP 10"), 0xFE, 0x0A) I2(T("JR NC,$+110"), 0x30, 0x6C)
        IREFO(T("LD HL,%s"), "rt_fs+48", 0x21, 48) I2(T("LD B,19"), 0x06, 0x13) I1(T("XOR A"), 0xAF)
        I1(T("OR (HL)"), 0xB6) I1(T("INC HL"), 0x23) I2(T("DJNZ $-2"), 0x10, 0xFC) I2(T("JR Z,$+98"), 0x28, 0x60)
        IREFO(T("LD HL,%s"), "rt_fs+48", 0x21, 48) IREFO(T("LD DE,%s"), "rt_fs+83", 0x11, 83)
        I3(T("LD BC,%s"), "19", 0x01, 0x13, 0x00) I2(T("LDIR"), 0xED, 0xB0) I1(T("XOR A"), 0xAF)
        IREFO(T("LD (%s),A"), "rt_fs+102", 0x32, 102) IREFO(T("LD HL,%s"), "rt_fs+83", 0x21, 83)
        I2(T("LD B,20"), 0x06, 0x14) I1(T("OR A"), 0xB7) ICALLRT(RT_BSHL) IREFO(T("LD HL,%s"), "rt_fs+83", 0x21, 83)
        I2(T("LD B,20"), 0x06, 0x14) I1(T("OR A"), 0xB7) ICALLRT(RT_BSHL) IREFO(T("LD HL,%s"), "rt_fs+48", 0x21, 48)
        IREFO(T("LD DE,%s"), "rt_fs+83", 0x11, 83) I2(T("LD B,20"), 0x06, 0x14) I1(T("OR A"), 0xB7) ICALLRT(RT_BADD)
        IREFO(T("LD HL,%s"), "rt_fs+83", 0x21, 83) I2(T("LD B,20"), 0x06, 0x14) I1(T("OR A"), 0xB7) ICALLRT(RT_BSHL)
        IREFO(T("LD HL,%s"), "rt_fs+83", 0x21, 83) IREFO(T("LD DE,%s"), "rt_fs+48", 0x11, 48)
        I3(T("LD BC,%s"), "19", 0x01, 0x13, 0x00) I2(T("LDIR"), 0xED, 0xB0)
        IREFO(T("LD A,(%s)"), "rt_fs+102", 0x3A, 102) I1(T("LD B,A"), 0x47)
        IREFO(T("LD A,(%s)"), "rt_fs+171", 0x3A, 171) I1(T("OR B"), 0xB0) I2(T("JR NZ,$+8"), 0x20, 0x06)
        IREFO(T("LD HL,%s"), "rt_fs+173", 0x21, 173) I1(T("INC (HL)"), 0x34) I2(T("JR $-98"), 0x18, 0x9C)
        I1(T("LD A,B"), 0x78) IREFO(T("LD HL,(%s)"), "rt_fs+169", 0x2A, 169) I1(T("LD (HL),A"), 0x77)
        I1(T("INC HL"), 0x23) IREFO(T("LD (%s),HL"), "rt_fs+169", 0x22, 169)
        IREFO(T("LD HL,%s"), "rt_fs+171", 0x21, 171) I1(T("INC (HL)"), 0x34) I2(T("JR $-113"), 0x18, 0x8D)
        IREFO(T("LD A,(%s)"), "rt_fs+172", 0x3A, 172) I1(T("OR A"), 0xB7) I2(T("JR Z,$+8"), 0x28, 0x06)
        I1(T("DEC A"), 0x3D) IREFO(T("LD (%s),A"), "rt_fs+174", 0x32, 174) I2(T("JR $+9"), 0x18, 0x07)
        IREFO(T("LD A,(%s)"), "rt_fs+173", 0x3A, 173) I1(T("CPL"), 0x2F) IREFO(T("LD (%s),A"), "rt_fs+174", 0x32, 174)
        IREFO(T("LD HL,(%s)"), "rt_fs+167", 0x2A, 167) I3(T("LD DE,%s"), "10", 0x11, 0x0A, 0x00)
        I1(T("ADD HL,DE"), 0x19) I1(T("EX DE,HL"), 0xEB) IREFO(T("LD HL,%s"), "rt_fs+167", 0x21, 167)
        I1(T("OR A"), 0xB7) I2(T("SBC HL,DE"), 0xED, 0x52) I1(T("LD B,L"), 0x45) I1(T("EX DE,HL"), 0xEB)
        I1(T("XOR A"), 0xAF) I1(T("OR (HL)"), 0xB6) I1(T("INC HL"), 0x23) I2(T("DJNZ $-2"), 0x10, 0xFC)
        I1(T("LD C,A"), 0x4F) IREFO(T("LD HL,%s"), "rt_fs+48", 0x21, 48) I2(T("LD B,19"), 0x06, 0x13)
        I1(T("XOR A"), 0xAF) I1(T("OR (HL)"), 0xB6) I1(T("INC HL"), 0x23) I2(T("DJNZ $-2"), 0x10, 0xFC)
        I1(T("OR C"), 0xB1) IREFO(T("LD (%s),A"), "rt_fs+175", 0x32, 175)
        IREFO(T("LD HL,(%s)"), "rt_fs+167", 0x2A, 167) I3(T("LD DE,%s"), "9", 0x11, 0x09, 0x00)
        I1(T("ADD HL,DE"), 0x19) I1(T("LD A,(HL)"), 0x7E) I1(T("DEC HL"), 0x2B) I2(T("CP 5"), 0xFE, 0x05)
        I2(T("JR C,$+35"), 0x38, 0x21) I2(T("JR NZ,$+13"), 0x20, 0x0B) IREFO(T("LD A,(%s)"), "rt_fs+175", 0x3A, 175)
        I1(T("OR A"), 0xB7) I2(T("JR NZ,$+7"), 0x20, 0x05) I1(T("LD A,(HL)"), 0x7E) I2(T("AND 1"), 0xE6, 0x01)
        I2(T("JR Z,$+22"), 0x28, 0x14) I2(T("LD B,9"), 0x06, 0x09) I1(T("INC (HL)"), 0x34) I1(T("LD A,(HL)"), 0x7E)
        I2(T("CP 10"), 0xFE, 0x0A) I2(T("JR NZ,$+14"), 0x20, 0x0C) I2(T("LD (HL),0"), 0x36, 0x00)
        I1(T("DEC HL"), 0x2B) I2(T("DJNZ $-9"), 0x10, 0xF5) I1(T("INC HL"), 0x23) I2(T("LD (HL),1"), 0x36, 0x01)
        IREFO(T("LD HL,%s"), "rt_fs+174", 0x21, 174) I1(T("INC (HL)"), 0x34)
        IREFO(T("LD A,(%s)"), "rt_fs+174", 0x3A, 174) I1(T("INC A"), 0x3C) I2(T("CP 10"), 0xFE, 0x0A)
        I2(T("JR NC,$+50"), 0x30, 0x30) I1(T("LD B,A"), 0x47) IREFO(T("LD HL,(%s)"), "rt_fs+167", 0x2A, 167)
        I1(T("OR A"), 0xB7) I2(T("JR NZ,$+9"), 0x20, 0x07) I2(T("LD A,48"), 0x3E, 0x30) ICALLRT(RT_PUTC)
        I2(T("JR $+11"), 0x18, 0x09) I1(T("LD A,(HL)"), 0x7E) I2(T("ADD A,48"), 0xC6, 0x30) ICALLRT(RT_PUTC)
        I1(T("INC HL"), 0x23) I2(T("DJNZ $-7"), 0x10, 0xF7) I2(T("LD A,46"), 0x3E, 0x2E) ICALLRT(RT_PUTC)
        IREFO(T("LD A,(%s)"), "rt_fs+174", 0x3A, 174) I1(T("INC A"), 0x3C) I1(T("LD C,A"), 0x4F)
        I2(T("LD A,9"), 0x3E, 0x09) I1(T("SUB C"), 0x91) I1(T("RET Z"), 0xC8) I1(T("LD B,A"), 0x47)
        I1(T("LD A,(HL)"), 0x7E) I2(T("ADD A,48"), 0xC6, 0x30) ICALLRT(RT_PUTC) I1(T("INC HL"), 0x23)
        I2(T("DJNZ $-7"), 0x10, 0xF7) I1(T("RET"), 0xC9) IREFO(T("LD HL,(%s)"), "rt_fs+167", 0x2A, 167)
        I1(T("LD A,(HL)"), 0x7E) I2(T("ADD A,48"), 0xC6, 0x30) ICALLRT(RT_PUTC) I1(T("INC HL"), 0x23)
        I2(T("LD A,46"), 0x3E, 0x2E) ICALLRT(RT_PUTC) I2(T("LD B,8"), 0x06, 0x08) I1(T("LD A,(HL)"), 0x7E)
        I2(T("ADD A,48"), 0xC6, 0x30) ICALLRT(RT_PUTC) I1(T("INC HL"), 0x23) I2(T("DJNZ $-7"), 0x10, 0xF7)
        I2(T("LD A,69"), 0x3E, 0x45) ICALLRT(RT_PUTC) IREFO(T("LD A,(%s)"), "rt_fs+174", 0x3A, 174)
        I2(T("LD B,43"), 0x06, 0x2B) I2(T("BIT 7,A"), 0xCB, 0x7F) I2(T("JR Z,$+6"), 0x28, 0x04)
        I2(T("NEG"), 0xED, 0x44) I2(T("LD B,45"), 0x06, 0x2D) I1(T("LD C,A"), 0x4F) I1(T("LD A,B"), 0x78)
        ICALLRT(RT_PUTC) I1(T("LD A,C"), 0x79) I2(T("LD B,48"), 0x06, 0x30) I2(T("CP 10"), 0xFE, 0x0A)
        I2(T("JR C,$+7"), 0x38, 0x05) I2(T("SUB 10"), 0xD6, 0x0A) I1(T("INC B"), 0x04) I2(T("JR $-7"), 0x18, 0xF7)
        I1(T("PUSH AF"), 0xF5) I1(T("LD A,B"), 0x78) ICALLRT(RT_PUTC) I1(T("POP AF"), 0xF1)
        I2(T("ADD A,48"), 0xC6, 0x30) ICALLRT(RT_PUTC) I1(T("RET"), 0xC9)
        IFIN
};

/* FINT: 88 B */
#define rt_n_fint 117
static const RtI rt_c_fint[] = {
        IREFO(T("LD (%s),A"), "rt_fs+18", 0x32, 18) IREFE(T("LD (%s),BC"), "rt_fs+16", 0xED, 0x43, 16)
        IREFO(T("LD DE,%s"), "rt_fs", 0x11, 0) ICALLRT(RT_FUNP) IREFO(T("LD A,(%s)"), "rt_fs+4", 0x3A, 4)
        I1(T("OR A"), 0xB7) I2(T("JR Z,$+55"), 0x28, 0x35) I2(T("CP 150"), 0xFE, 0x96) I2(T("JR NC,$+51"), 0x30, 0x31)
        I2(T("CP 126"), 0xFE, 0x7E) I2(T("JR C,$+50"), 0x38, 0x30) I1(T("LD C,A"), 0x4F) I2(T("LD A,150"), 0x3E, 0x96)
        I1(T("SUB C"), 0x91) I1(T("LD B,A"), 0x47) IREFO(T("LD HL,%s"), "rt_fs+3", 0x21, 3)
        I2(T("SRL (HL)"), 0xCB, 0x3E) I1(T("DEC HL"), 0x2B) I2(T("RR (HL)"), 0xCB, 0x1E) I1(T("DEC HL"), 0x2B)
        I2(T("RR (HL)"), 0xCB, 0x1E) I2(T("DJNZ $-11"), 0x10, 0xF3) IREFO(T("LD A,(%s)"), "rt_fs+18", 0x3A, 18)
        I2(T("BIT 0,A"), 0xCB, 0x47) I2(T("JR Z,$+16"), 0x28, 0x0E) I2(T("JR NC,$+14"), 0x30, 0x0C)
        IREFO(T("LD HL,%s"), "rt_fs+1", 0x21, 1) I1(T("INC (HL)"), 0x34) I2(T("JR NZ,$+8"), 0x20, 0x06)
        I1(T("INC HL"), 0x23) I1(T("INC (HL)"), 0x34) I2(T("JR NZ,$+4"), 0x20, 0x02) I1(T("INC HL"), 0x23)
        I1(T("INC (HL)"), 0x34) I3(T("LD HL,%s"), "150", 0x21, 0x96, 0x00) IREFO(T("LD (%s),HL"), "rt_fs+4", 0x22, 4)
        IJPRT(RT_FNORM) IREFO(T("LD HL,%s"), "rt_fs", 0x21, 0) I2(T("LD B,4"), 0x06, 0x04)
        I2(T("LD (HL),0"), 0x36, 0x00) I1(T("INC HL"), 0x23) I2(T("DJNZ $-3"), 0x10, 0xFB) IJPRT(RT_FNORM)
        IFIN
};

/* FSQRT: 246 B */
#define rt_n_fsqrt 303
static const RtI rt_c_fsqrt[] = {
        IREFE(T("LD (%s),BC"), "rt_fs+16", 0xED, 0x43, 16) IREFO(T("LD DE,%s"), "rt_fs", 0x11, 0) ICALLRT(RT_FUNP)
        IREFO(T("LD A,(%s)"), "rt_fs+4", 0x3A, 4) I1(T("OR A"), 0xB7) I2(T("JR NZ,$+5"), 0x20, 0x03) IJPRT(RT_FNORM)
        IREFO(T("LD A,(%s)"), "rt_fs+6", 0x3A, 6) I1(T("OR A"), 0xB7) I2(T("JR Z,$+7"), 0x28, 0x05)
        I2(T("LD A,4"), 0x3E, 0x04) IJPRT(RT_ERR) IREFO(T("LD HL,%s"), "rt_fs", 0x21, 0)
        IREFO(T("LD DE,%s"), "rt_fs+20", 0x11, 20) I3(T("LD BC,%s"), "4", 0x01, 0x04, 0x00) I2(T("LDIR"), 0xED, 0xB0)
        IREFO(T("LD A,(%s)"), "rt_fs+4", 0x3A, 4) I2(T("LD B,1"), 0x06, 0x01) I1(T("RRA"), 0x1F)
        I2(T("JR C,$+3"), 0x38, 0x01) I1(T("INC B"), 0x04) I1(T("PUSH BC"), 0xC5)
        IREFO(T("LD HL,%s"), "rt_fs+23", 0x21, 23) I2(T("LD B,4"), 0x06, 0x04) I1(T("OR A"), 0xB7) ICALLRT(RT_BSHR)
        I1(T("POP BC"), 0xC1) I2(T("DJNZ $-11"), 0x10, 0xF3) IREFO(T("LD HL,%s"), "rt_fs+24", 0x21, 24)
        I2(T("LD B,8"), 0x06, 0x08) I1(T("XOR A"), 0xAF) I1(T("LD (HL),A"), 0x77) I1(T("INC HL"), 0x23)
        I2(T("DJNZ $-2"), 0x10, 0xFC) I2(T("LD A,27"), 0x3E, 0x1B) IREFO(T("LD (%s),A"), "rt_fs+19", 0x32, 19)
        I2(T("LD C,2"), 0x0E, 0x02) IREFO(T("LD HL,%s"), "rt_fs+20", 0x21, 20) I2(T("LD B,4"), 0x06, 0x04)
        I1(T("OR A"), 0xB7) ICALLRT(RT_BSHL) IREFO(T("LD HL,%s"), "rt_fs+24", 0x21, 24) I2(T("LD B,4"), 0x06, 0x04)
        ICALLRT(RT_BSHL) I1(T("DEC C"), 0x0D) I2(T("JR NZ,$-18"), 0x20, 0xEC)
        IREFO(T("LD HL,%s"), "rt_fs+28", 0x21, 28) IREFO(T("LD DE,%s"), "rt_fs+32", 0x11, 32)
        I3(T("LD BC,%s"), "4", 0x01, 0x04, 0x00) I2(T("LDIR"), 0xED, 0xB0) I2(T("LD C,2"), 0x0E, 0x02)
        IREFO(T("LD HL,%s"), "rt_fs+32", 0x21, 32) I2(T("LD B,4"), 0x06, 0x04) I1(T("OR A"), 0xB7) ICALLRT(RT_BSHL)
        I1(T("DEC C"), 0x0D) I2(T("JR NZ,$-10"), 0x20, 0xF4) IREFO(T("LD HL,%s"), "rt_fs+32", 0x21, 32)
        I2(T("SET 0,(HL)"), 0xCB, 0xC6) IREFO(T("LD HL,%s"), "rt_fs+28", 0x21, 28) I2(T("LD B,4"), 0x06, 0x04)
        I1(T("OR A"), 0xB7) ICALLRT(RT_BSHL) IREFO(T("LD HL,%s"), "rt_fs+32", 0x21, 32)
        IREFO(T("LD DE,%s"), "rt_fs+24", 0x11, 24) I2(T("LD B,4"), 0x06, 0x04) I1(T("OR A"), 0xB7) ICALLRT(RT_BSUB)
        I2(T("JR C,$+9"), 0x38, 0x07) IREFO(T("LD HL,%s"), "rt_fs+28", 0x21, 28) I2(T("SET 0,(HL)"), 0xCB, 0xC6)
        I2(T("JR $+14"), 0x18, 0x0C) IREFO(T("LD HL,%s"), "rt_fs+32", 0x21, 32)
        IREFO(T("LD DE,%s"), "rt_fs+24", 0x11, 24) I2(T("LD B,4"), 0x06, 0x04) I1(T("OR A"), 0xB7) ICALLRT(RT_BADD)
        IREFO(T("LD A,(%s)"), "rt_fs+19", 0x3A, 19) I1(T("DEC A"), 0x3D) IREFO(T("LD (%s),A"), "rt_fs+19", 0x32, 19)
        I2(T("JR NZ,$-101"), 0x20, 0x99) IREFO(T("LD HL,%s"), "rt_fs+24", 0x21, 24) I1(T("LD A,(HL)"), 0x7E)
        I1(T("INC HL"), 0x23) I1(T("OR (HL)"), 0xB6) I1(T("INC HL"), 0x23) I1(T("OR (HL)"), 0xB6)
        I1(T("INC HL"), 0x23) I1(T("OR (HL)"), 0xB6) IREFO(T("LD (%s),A"), "rt_fs+36", 0x32, 36)
        IREFO(T("LD HL,%s"), "rt_fs+28", 0x21, 28) IREFO(T("LD DE,%s"), "rt_fs", 0x11, 0)
        I3(T("LD BC,%s"), "4", 0x01, 0x04, 0x00) I2(T("LDIR"), 0xED, 0xB0) I2(T("LD C,5"), 0x0E, 0x05)
        IREFO(T("LD HL,%s"), "rt_fs", 0x21, 0) I2(T("LD B,4"), 0x06, 0x04) I1(T("OR A"), 0xB7) ICALLRT(RT_BSHL)
        I1(T("DEC C"), 0x0D) I2(T("JR NZ,$-10"), 0x20, 0xF4) IREFO(T("LD A,(%s)"), "rt_fs+36", 0x3A, 36)
        I1(T("OR A"), 0xB7) I2(T("JR Z,$+7"), 0x28, 0x05) IREFO(T("LD HL,%s"), "rt_fs", 0x21, 0)
        I2(T("SET 0,(HL)"), 0xCB, 0xC6) IREFO(T("LD A,(%s)"), "rt_fs+4", 0x3A, 4) I2(T("SRL A"), 0xCB, 0x3F)
        I2(T("ADD A,64"), 0xC6, 0x40) I1(T("LD L,A"), 0x6F) I2(T("LD H,0"), 0x26, 0x00)
        IREFO(T("LD (%s),HL"), "rt_fs+4", 0x22, 4) IJPRT(RT_FNORM)
        IFIN
};

/* FMOD: 126 B */
#define rt_n_fmod 151
static const RtI rt_c_fmod[] = {
        IREFE(T("LD (%s),BC"), "rt_fs+16", 0xED, 0x43, 16) I1(T("PUSH DE"), 0xD5)
        IREFO(T("LD DE,%s"), "rt_fs", 0x11, 0) ICALLRT(RT_FUNP) I1(T("POP HL"), 0xE1)
        IREFO(T("LD DE,%s"), "rt_fs+8", 0x11, 8) ICALLRT(RT_FUNP) IREFO(T("LD A,(%s)"), "rt_fs+12", 0x3A, 12)
        I1(T("OR A"), 0xB7) I2(T("JR NZ,$+14"), 0x20, 0x0C) IREFO(T("LD A,(%s)"), "rt_fs+4", 0x3A, 4)
        I1(T("OR A"), 0xB7) I2(T("LD A,1"), 0x3E, 0x01) I2(T("JR NZ,$+3"), 0x20, 0x01) I1(T("INC A"), 0x3C)
        IJPRT(RT_ERR) IREFO(T("LD A,(%s)"), "rt_fs+4", 0x3A, 4) I1(T("OR A"), 0xB7) I2(T("JR Z,$+83"), 0x28, 0x51)
        I1(T("LD B,A"), 0x47) IREFO(T("LD A,(%s)"), "rt_fs+12", 0x3A, 12) I1(T("LD C,A"), 0x4F) I1(T("LD A,B"), 0x78)
        I1(T("SUB C"), 0x91) I2(T("JR C,$+74"), 0x38, 0x48) I1(T("INC A"), 0x3C)
        IREFO(T("LD (%s),A"), "rt_fs+19", 0x32, 19) I1(T("XOR A"), 0xAF) IREFO(T("LD (%s),A"), "rt_fs+36", 0x32, 36)
        IREFO(T("LD HL,%s"), "rt_fs+8", 0x21, 8) IREFO(T("LD DE,%s"), "rt_fs", 0x11, 0) I2(T("LD B,4"), 0x06, 0x04)
        I1(T("OR A"), 0xB7) ICALLRT(RT_BSUB) I2(T("JR NC,$+20"), 0x30, 0x12)
        IREFO(T("LD A,(%s)"), "rt_fs+36", 0x3A, 36) I1(T("OR A"), 0xB7) I2(T("JR NZ,$+14"), 0x20, 0x0C)
        IREFO(T("LD HL,%s"), "rt_fs+8", 0x21, 8) IREFO(T("LD DE,%s"), "rt_fs", 0x11, 0) I2(T("LD B,4"), 0x06, 0x04)
        I1(T("OR A"), 0xB7) ICALLRT(RT_BADD) IREFO(T("LD A,(%s)"), "rt_fs+19", 0x3A, 19) I1(T("DEC A"), 0x3D)
        IREFO(T("LD (%s),A"), "rt_fs+19", 0x32, 19) I2(T("JR Z,$+19"), 0x28, 0x11)
        IREFO(T("LD HL,%s"), "rt_fs", 0x21, 0) I2(T("LD B,4"), 0x06, 0x04) I1(T("OR A"), 0xB7) ICALLRT(RT_BSHL)
        I1(T("SBC A,A"), 0x9F) I2(T("AND 1"), 0xE6, 0x01) IREFO(T("LD (%s),A"), "rt_fs+36", 0x32, 36)
        I2(T("JR $-56"), 0x18, 0xC6) IREFO(T("LD HL,(%s)"), "rt_fs+12", 0x2A, 12)
        IREFO(T("LD (%s),HL"), "rt_fs+4", 0x22, 4) IJPRT(RT_FNORM)
        IFIN
};

/* FPOW: 191 B */
#define rt_n_fpow 243
static const RtI rt_c_fpow[] = {
        IREFE(T("LD (%s),BC"), "rt_fs+59", 0xED, 0x43, 59) I1(T("LD A,D"), 0x7A) I2(T("AND 0x80"), 0xE6, 0x80)
        IREFO(T("LD (%s),A"), "rt_fs+58", 0x32, 58) I2(T("BIT 7,D"), 0xCB, 0x7A) I2(T("JR Z,$+9"), 0x28, 0x07)
        I1(T("LD A,E"), 0x7B) I1(T("CPL"), 0x2F) I1(T("LD E,A"), 0x5F) I1(T("LD A,D"), 0x7A) I1(T("CPL"), 0x2F)
        I1(T("LD D,A"), 0x57) I1(T("INC DE"), 0x13) IREFE(T("LD (%s),DE"), "rt_fs+56", 0xED, 0x53, 56)
        IREFO(T("LD DE,%s"), "rt_fs+48", 0x11, 48) I3(T("LD BC,%s"), "4", 0x01, 0x04, 0x00) I2(T("LDIR"), 0xED, 0xB0)
        IREFO(T("LD A,(%s)"), "rt_fs+51", 0x3A, 51) I2(T("AND 0x7F"), 0xE6, 0x7F) I2(T("JR NZ,$+27"), 0x20, 0x19)
        IREFO(T("LD A,(%s)"), "rt_fs+50", 0x3A, 50) I2(T("AND 0x80"), 0xE6, 0x80) I2(T("JR NZ,$+20"), 0x20, 0x12)
        IREFO(T("LD A,(%s)"), "rt_fs+58", 0x3A, 58) I1(T("OR A"), 0xB7) I2(T("JR NZ,$+9"), 0x20, 0x07)
        IREFO(T("LD HL,(%s)"), "rt_fs+56", 0x2A, 56) I1(T("LD A,H"), 0x7C) I1(T("OR L"), 0xB5)
        I2(T("JR NZ,$+7"), 0x20, 0x05) I2(T("LD A,5"), 0x3E, 0x05) IJPRT(RT_ERR)
        IREFO(T("LD HL,(%s)"), "rt_fs+56", 0x2A, 56) I2(T("BIT 0,L"), 0xCB, 0x45) I2(T("JR Z,$+15"), 0x28, 0x0D)
        IREFO(T("LD HL,%s"), "rt_fs+48", 0x21, 48) IREFO(T("LD DE,%s"), "rt_fs+52", 0x11, 52)
        I3(T("LD BC,%s"), "4", 0x01, 0x04, 0x00) I2(T("LDIR"), 0xED, 0xB0) I2(T("JR $+16"), 0x18, 0x0E)
        IREFO(T("LD HL,%s"), "rt_fs+52", 0x21, 52) I2(T("LD (HL),0"), 0x36, 0x00) I1(T("INC HL"), 0x23)
        I2(T("LD (HL),0"), 0x36, 0x00) I1(T("INC HL"), 0x23) I2(T("LD (HL),0x80"), 0x36, 0x80) I1(T("INC HL"), 0x23)
        I2(T("LD (HL),0x3F"), 0x36, 0x3F) IREFO(T("LD HL,(%s)"), "rt_fs+56", 0x2A, 56) I2(T("SRL H"), 0xCB, 0x3C)
        I2(T("RR L"), 0xCB, 0x1D) IREFO(T("LD (%s),HL"), "rt_fs+56", 0x22, 56) I1(T("LD A,H"), 0x7C)
        I1(T("OR L"), 0xB5) I2(T("JR Z,$+35"), 0x28, 0x21) IREFO(T("LD HL,%s"), "rt_fs+48", 0x21, 48)
        IREFO(T("LD DE,%s"), "rt_fs+48", 0x11, 48) IREFO(T("LD BC,%s"), "rt_fs+48", 0x01, 48) ICALLRT(RT_FMUL)
        IREFO(T("LD HL,(%s)"), "rt_fs+56", 0x2A, 56) I2(T("BIT 0,L"), 0xCB, 0x45) I2(T("JR Z,$-31"), 0x28, 0xDF)
        IREFO(T("LD HL,%s"), "rt_fs+52", 0x21, 52) IREFO(T("LD DE,%s"), "rt_fs+48", 0x11, 48)
        IREFO(T("LD BC,%s"), "rt_fs+52", 0x01, 52) ICALLRT(RT_FMUL) I2(T("JR $-45"), 0x18, 0xD1)
        IREFO(T("LD A,(%s)"), "rt_fs+58", 0x3A, 58) I1(T("OR A"), 0xB7) I2(T("JR Z,$+28"), 0x28, 0x1A)
        IREFO(T("LD HL,%s"), "rt_fs+61", 0x21, 61) I2(T("LD (HL),0"), 0x36, 0x00) I1(T("INC HL"), 0x23)
        I2(T("LD (HL),0"), 0x36, 0x00) I1(T("INC HL"), 0x23) I2(T("LD (HL),0x80"), 0x36, 0x80) I1(T("INC HL"), 0x23)
        I2(T("LD (HL),0x3F"), 0x36, 0x3F) IREFO(T("LD HL,%s"), "rt_fs+61", 0x21, 61)
        IREFO(T("LD DE,%s"), "rt_fs+52", 0x11, 52) IREFO(T("LD BC,%s"), "rt_fs+52", 0x01, 52) ICALLRT(RT_FDIV)
        IREFO(T("LD HL,%s"), "rt_fs+52", 0x21, 52) IREFE(T("LD DE,(%s)"), "rt_fs+59", 0xED, 0x5B, 59)
        I3(T("LD BC,%s"), "4", 0x01, 0x04, 0x00) I2(T("LDIR"), 0xED, 0xB0) I1(T("RET"), 0xC9)
        IFIN
};

/* ERR: 191 B */
#define rt_n_err 258
static const RtI rt_c_err[] = {
        IREFR(T("LD HL,%s"), "rt_err+57", 0x21, 57) I1(T("DEC A"), 0x3D) I2(T("JR Z,$+35"), 0x28, 0x21)
        IREFR(T("LD HL,%s"), "rt_err+74", 0x21, 74) I1(T("DEC A"), 0x3D) I2(T("JR Z,$+29"), 0x28, 0x1B)
        IREFR(T("LD HL,%s"), "rt_err+95", 0x21, 95) I1(T("DEC A"), 0x3D) I2(T("JR Z,$+23"), 0x28, 0x15)
        IREFR(T("LD HL,%s"), "rt_err+109", 0x21, 109) I1(T("DEC A"), 0x3D) I2(T("JR Z,$+17"), 0x28, 0x0F)
        IREFR(T("LD HL,%s"), "rt_err+135", 0x21, 135) I1(T("DEC A"), 0x3D) I2(T("JR Z,$+11"), 0x28, 0x09)
        IREFR(T("LD HL,%s"), "rt_err+164", 0x21, 164) I1(T("DEC A"), 0x3D) I2(T("JR Z,$+5"), 0x28, 0x03)
        IREFR(T("LD HL,%s"), "rt_err+178", 0x21, 178) I1(T("LD A,(HL)"), 0x7E) I1(T("INC HL"), 0x23)
        I1(T("OR A"), 0xB7) I2(T("JR Z,$+9"), 0x28, 0x07) I1(T("PUSH HL"), 0xE5) ICALLRT(RT_PUTC)
        I1(T("POP HL"), 0xE1) I2(T("JR $-10"), 0x18, 0xF4) I2(T("LD A,13"), 0x3E, 0x0D) ICALLRT(RT_PUTC)
        I1(T("HALT"), 0x76)
        IDB3(0x44, 0x69, 0x76) IDB3(0x69, 0x73, 0x69) IDB3(0x6F, 0x6E, 0x20) IDB3(0x62, 0x79, 0x20) IDB3(0x7A, 0x65, 0x72) IDB2(0x6F, 0x00)
        IDB3(0x30, 0x2F, 0x30) IDB3(0x20, 0x69, 0x73) IDB3(0x20, 0x69, 0x6E) IDB3(0x64, 0x65, 0x74) IDB3(0x65, 0x72, 0x6D) IDB3(0x69, 0x6E, 0x61) IDB3(0x74, 0x65, 0x00)
        IDB3(0x52, 0x45, 0x41) IDB3(0x4C, 0x20, 0x6F) IDB3(0x76, 0x65, 0x72) IDB3(0x66, 0x6C, 0x6F) IDB2(0x77, 0x00)
        IDB3(0x53, 0x51, 0x52) IDB3(0x54, 0x20, 0x6F) IDB3(0x66, 0x20, 0x61) IDB3(0x20, 0x6E, 0x65) IDB3(0x67, 0x61, 0x74) IDB3(0x69, 0x76, 0x65) IDB3(0x20, 0x6E, 0x75) IDB3(0x6D, 0x62, 0x65) IDB2(0x72, 0x00)
        IDB3(0x5A, 0x65, 0x72) IDB3(0x6F, 0x20, 0x74) IDB3(0x6F, 0x20, 0x61) IDB3(0x20, 0x6E, 0x6F) IDB3(0x6E, 0x2D, 0x70) IDB3(0x6F, 0x73, 0x69) IDB3(0x74, 0x69, 0x76) IDB3(0x65, 0x20, 0x70) IDB3(0x6F, 0x77, 0x65) IDB2(0x72, 0x00)
        IDB3(0x49, 0x6E, 0x76) IDB3(0x61, 0x6C, 0x69) IDB3(0x64, 0x20, 0x69) IDB3(0x6E, 0x70, 0x75) IDB2(0x74, 0x00)
        IDB3(0x45, 0x6E, 0x64) IDB3(0x20, 0x6F, 0x66) IDB3(0x20, 0x69, 0x6E) IDB3(0x70, 0x75, 0x74) IDB1(0x00)
        IFIN
};

