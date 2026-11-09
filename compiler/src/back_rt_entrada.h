/* back_rt_entrada.h - GENERADO por tools/rtgen/rtasm.py desde tools/rtgen/rt_entrada.asm. NO EDITAR A MANO.
 * Rutinas de apoyo del programa generado: 1193 B de codigo y datos en total. */

/* RDD: 109 B */
#define rt_n_rdd 3
static const RtI rt_c_rdd[] = {
        IDATA(109)
        IFIN
};

/* KEYSC: 54 B */
#define rt_n_keysc 89
static const RtI rt_c_keysc[] = {
        I2(T("LD B,0xFE"), 0x06, 0xFE) I2(T("LD D,0"), 0x16, 0x00) I2(T("LD C,0xFE"), 0x0E, 0xFE)
        I2(T("IN A,(C)"), 0xED, 0x78) I1(T("CPL"), 0x2F) I2(T("AND 0x1F"), 0xE6, 0x1F) I2(T("JR Z,$+20"), 0x28, 0x12)
        I1(T("LD E,A"), 0x5F) I1(T("LD A,B"), 0x78) I2(T("CP 0xFE"), 0xFE, 0xFE) I2(T("JR NZ,$+4"), 0x20, 0x02)
        I2(T("RES 0,E"), 0xCB, 0x83) I2(T("CP 0x7F"), 0xFE, 0x7F) I2(T("JR NZ,$+4"), 0x20, 0x02)
        I2(T("RES 1,E"), 0xCB, 0x8B) I1(T("LD A,E"), 0x7B) I1(T("OR A"), 0xB7) I2(T("JR NZ,$+15"), 0x20, 0x0D)
        I1(T("LD A,D"), 0x7A) I2(T("ADD A,5"), 0xC6, 0x05) I1(T("LD D,A"), 0x57) I2(T("RLC B"), 0xCB, 0x00)
        I1(T("LD A,B"), 0x78) I2(T("CP 0xFE"), 0xFE, 0xFE) I2(T("JR NZ,$-36"), 0x20, 0xDA) I1(T("XOR A"), 0xAF)
        I1(T("RET"), 0xC9) I1(T("LD A,D"), 0x7A) I2(T("SRL E"), 0xCB, 0x3B) I2(T("JR C,$+5"), 0x38, 0x03)
        I1(T("INC A"), 0x3C) I2(T("JR $-5"), 0x18, 0xF9) I1(T("INC A"), 0x3C) I1(T("RET"), 0xC9)
        IFIN
};

/* KEYH: 150 B */
#define rt_n_keyh 222
static const RtI rt_c_keyh[] = {
        ICALLRT(RT_KEYSC) I1(T("OR A"), 0xB7) I2(T("JR NZ,$-4"), 0x20, 0xFA) ICALLRT(RT_KEYSC) I1(T("OR A"), 0xB7)
        I2(T("JR Z,$-4"), 0x28, 0xFA) I1(T("DEC A"), 0x3D) I1(T("LD E,A"), 0x5F) I2(T("LD D,0"), 0x16, 0x00)
        I3(T("LD BC,%s"), "65278", 0x01, 0xFE, 0xFE) I2(T("IN A,(C)"), 0xED, 0x78) I2(T("AND 1"), 0xE6, 0x01)
        I1(T("LD H,A"), 0x67) I3(T("LD BC,%s"), "32766", 0x01, 0xFE, 0x7F) I2(T("IN A,(C)"), 0xED, 0x78)
        I2(T("AND 2"), 0xE6, 0x02) I1(T("LD B,A"), 0x47) I1(T("LD C,H"), 0x4C)
        IREFR(T("LD HL,%s"), "rt_keyh+70", 0x21, 70) I1(T("ADD HL,DE"), 0x19) I1(T("LD A,B"), 0x78)
        I1(T("OR A"), 0xB7) I2(T("JR NZ,$+11"), 0x20, 0x09) I1(T("PUSH HL"), 0xE5)
        I3(T("LD DE,%s"), "40", 0x11, 0x28, 0x00) I1(T("ADD HL,DE"), 0x19) I1(T("LD A,(HL)"), 0x7E)
        I1(T("POP HL"), 0xE1) I1(T("OR A"), 0xB7) I1(T("RET NZ"), 0xC0) I1(T("LD A,C"), 0x79) I1(T("OR A"), 0xB7)
        I1(T("LD A,(HL)"), 0x7E) I1(T("RET NZ"), 0xC0) I2(T("CP 48"), 0xFE, 0x30) I2(T("JR NZ,$+5"), 0x20, 0x03)
        I2(T("LD A,8"), 0x3E, 0x08) I1(T("RET"), 0xC9) I2(T("CP 97"), 0xFE, 0x61) I1(T("RET C"), 0xD8)
        I2(T("CP 123"), 0xFE, 0x7B) I1(T("RET NC"), 0xD0) I2(T("SUB 32"), 0xD6, 0x20) I1(T("RET"), 0xC9)
        IDB3(0x00, 0x7A, 0x78) IDB2(0x63, 0x76) IDB3(0x61, 0x73, 0x64) IDB2(0x66, 0x67)
        IDB3(0x71, 0x77, 0x65) IDB2(0x72, 0x74) IDB3(0x31, 0x32, 0x33) IDB2(0x34, 0x35)
        IDB3(0x30, 0x39, 0x38) IDB2(0x37, 0x36) IDB3(0x70, 0x6F, 0x69) IDB2(0x75, 0x79)
        IDB3(0x0D, 0x6C, 0x6B) IDB2(0x6A, 0x68) IDB3(0x20, 0x00, 0x6D) IDB2(0x6E, 0x62)
        IDB3(0x00, 0x3A, 0x60) IDB2(0x3F, 0x2F) IDB3(0x7E, 0x7C, 0x5C) IDB2(0x7B, 0x7D)
        IDB3(0x00, 0x00, 0x00) IDB2(0x3C, 0x3E) IDB3(0x21, 0x40, 0x23) IDB2(0x24, 0x25)
        IDB3(0x5F, 0x29, 0x28) IDB2(0x27, 0x26) IDB3(0x22, 0x3B, 0x00) IDB2(0x5D, 0x5B)
        IDB3(0x00, 0x3D, 0x2B) IDB2(0x2D, 0x5E) IDB3(0x00, 0x00, 0x2E) IDB2(0x2C, 0x2A)
        IFIN
};

/* KEYT: 16 B */
#define rt_n_keyt 27
static const RtI rt_c_keyt[] = {
        I3(T("LD HL,(%s)"), "0xF000", 0x2A, 0x00, 0xF0) I1(T("LD A,(HL)"), 0x7E) I1(T("OR A"), 0xB7)
        I1(T("RET Z"), 0xC8) I1(T("INC HL"), 0x23) I3(T("LD (%s),HL"), "0xF000", 0x22, 0x00, 0xF0)
        I2(T("CP 10"), 0xFE, 0x0A) I1(T("RET NZ"), 0xC0) I2(T("LD A,13"), 0x3E, 0x0D) I1(T("RET"), 0xC9)
        IFIN
};

/* RDLINE: 94 B */
#define rt_n_rdline 129
static const RtI rt_c_rdline[] = {
        IREFD(T("LD HL,%s"), "rt_rd", 0x21, 0) I2(T("LD B,0"), 0x06, 0x00) I1(T("PUSH HL"), 0xE5)
        I1(T("PUSH BC"), 0xC5) ICALLRT(RT_KEY) I1(T("POP BC"), 0xC1) I1(T("POP HL"), 0xE1) I1(T("OR A"), 0xB7)
        I2(T("JR Z,$+56"), 0x28, 0x36) I2(T("CP 13"), 0xFE, 0x0D) I2(T("JR Z,$+59"), 0x28, 0x39)
        I2(T("CP 8"), 0xFE, 0x08) I2(T("JR Z,$+25"), 0x28, 0x17) I2(T("CP 32"), 0xFE, 0x20)
        I2(T("JR C,$-20"), 0x38, 0xEA) I2(T("CP 127"), 0xFE, 0x7F) I2(T("JR NC,$-24"), 0x30, 0xE6)
        I1(T("LD C,A"), 0x4F) I1(T("LD A,B"), 0x78) I2(T("CP 79"), 0xFE, 0x4F) I2(T("JR NC,$-30"), 0x30, 0xE0)
        I1(T("LD A,C"), 0x79) I1(T("LD (HL),A"), 0x77) I1(T("INC HL"), 0x23) I1(T("INC B"), 0x04) ICALLRT(RT_PUTC)
        I2(T("JR $-39"), 0x18, 0xD7) I1(T("LD A,B"), 0x78) I1(T("OR A"), 0xB7) I2(T("JR Z,$-43"), 0x28, 0xD3)
        I1(T("DEC HL"), 0x2B) I1(T("DEC B"), 0x05) I2(T("LD A,8"), 0x3E, 0x08) ICALLRT(RT_PUTC)
        I2(T("LD A,32"), 0x3E, 0x20) ICALLRT(RT_PUTC) I2(T("LD A,8"), 0x3E, 0x08) ICALLRT(RT_PUTC)
        I2(T("JR $-62"), 0x18, 0xC0) I1(T("LD A,B"), 0x78) I1(T("OR A"), 0xB7) I2(T("LD A,7"), 0x3E, 0x07)
        IRTOP(T("JP Z,%s"), 0xCA, RT_ERR) I2(T("LD A,13"), 0x3E, 0x0D) ICALLRT(RT_PUTC) I2(T("LD (HL),0"), 0x36, 0x00)
        IREFD(T("LD HL,%s"), "rt_rd", 0x21, 0) IREFD(T("LD (%s),HL"), "rt_rd+80", 0x22, 80) I1(T("XOR A"), 0xAF)
        IREFD(T("LD (%s),A"), "rt_rd+82", 0x32, 82) I1(T("RET"), 0xC9)
        IFIN
};

/* RDLN: 7 B */
#define rt_n_rdln 8
static const RtI rt_c_rdln[] = {
        I1(T("XOR A"), 0xAF) IREFD(T("LD (%s),A"), "rt_rd+83", 0x32, 83) IJPRT(RT_RDLINE)
        IFIN
};

/* RDTOK: 156 B */
#define rt_n_rdtok 229
static const RtI rt_c_rdtok[] = {
        IREFD(T("LD A,(%s)"), "rt_rd+83", 0x3A, 83) I1(T("OR A"), 0xB7) I2(T("JR NZ,$+41"), 0x20, 0x27)
        IREFD(T("LD A,(%s)"), "rt_rd+82", 0x3A, 82) I1(T("OR A"), 0xB7) I2(T("JR Z,$+5"), 0x28, 0x03)
        ICALLRT(RT_RDLINE) IREFD(T("LD HL,(%s)"), "rt_rd+80", 0x2A, 80) I1(T("LD A,(HL)"), 0x7E)
        I2(T("CP 32"), 0xFE, 0x20) I2(T("JR NZ,$+5"), 0x20, 0x03) I1(T("INC HL"), 0x23) I2(T("JR $-6"), 0x18, 0xF8)
        I1(T("OR A"), 0xB7) I2(T("JR NZ,$+9"), 0x20, 0x07) I2(T("LD A,1"), 0x3E, 0x01)
        IREFD(T("LD (%s),A"), "rt_rd+82", 0x32, 82) I2(T("JR $-28"), 0x18, 0xE2) I1(T("LD C,A"), 0x4F)
        I2(T("CP 44"), 0xFE, 0x2C) I2(T("JR NZ,$+9"), 0x20, 0x07) I1(T("INC HL"), 0x23)
        IREFD(T("LD (%s),HL"), "rt_rd+80", 0x22, 80) I2(T("LD A,1"), 0x3E, 0x01) I1(T("RET"), 0xC9)
        I2(T("CP 47"), 0xFE, 0x2F) I2(T("JR NZ,$+8"), 0x20, 0x06) IREFD(T("LD (%s),A"), "rt_rd+83", 0x32, 83)
        I2(T("LD A,1"), 0x3E, 0x01) I1(T("RET"), 0xC9) I2(T("CP 39"), 0xFE, 0x27) I2(T("JR Z,$+37"), 0x28, 0x23)
        I2(T("CP 34"), 0xFE, 0x22) I2(T("JR Z,$+33"), 0x28, 0x1F) IREFD(T("LD (%s),HL"), "rt_rd+84", 0x22, 84)
        I2(T("LD B,0"), 0x06, 0x00) I1(T("LD A,(HL)"), 0x7E) I1(T("OR A"), 0xB7) I2(T("JR Z,$+18"), 0x28, 0x10)
        I2(T("CP 32"), 0xFE, 0x20) I2(T("JR Z,$+14"), 0x28, 0x0C) I2(T("CP 44"), 0xFE, 0x2C)
        I2(T("JR Z,$+10"), 0x28, 0x08) I2(T("CP 47"), 0xFE, 0x2F) I2(T("JR Z,$+6"), 0x28, 0x04) I1(T("INC HL"), 0x23)
        I1(T("INC B"), 0x04) I2(T("JR $-18"), 0x18, 0xEC) I1(T("XOR A"), 0xAF)
        IREFD(T("LD (%s),A"), "rt_rd+87", 0x32, 87) I2(T("JR $+34"), 0x18, 0x20) I1(T("INC HL"), 0x23)
        IREFD(T("LD (%s),HL"), "rt_rd+84", 0x22, 84) I1(T("LD D,H"), 0x54) I1(T("LD E,L"), 0x5D)
        I2(T("LD B,0"), 0x06, 0x00) I1(T("LD A,(HL)"), 0x7E) I1(T("OR A"), 0xB7) I2(T("JR Z,$+44"), 0x28, 0x2A)
        I1(T("INC HL"), 0x23) I1(T("CP C"), 0xB9) I2(T("JR NZ,$+8"), 0x20, 0x06) I1(T("LD A,(HL)"), 0x7E)
        I1(T("CP C"), 0xB9) I2(T("JR NZ,$+9"), 0x20, 0x07) I1(T("INC HL"), 0x23) I1(T("LD A,C"), 0x79)
        I1(T("LD (DE),A"), 0x12) I1(T("INC DE"), 0x13) I1(T("INC B"), 0x04) I2(T("JR $-17"), 0x18, 0xED)
        I2(T("LD A,1"), 0x3E, 0x01) IREFD(T("LD (%s),A"), "rt_rd+87", 0x32, 87) I1(T("LD A,B"), 0x78)
        IREFD(T("LD (%s),A"), "rt_rd+86", 0x32, 86) I1(T("LD A,(HL)"), 0x7E) I2(T("CP 32"), 0xFE, 0x20)
        I2(T("JR NZ,$+5"), 0x20, 0x03) I1(T("INC HL"), 0x23) I2(T("JR $-6"), 0x18, 0xF8) I2(T("CP 44"), 0xFE, 0x2C)
        I2(T("JR NZ,$+3"), 0x20, 0x01) I1(T("INC HL"), 0x23) IREFD(T("LD (%s),HL"), "rt_rd+80", 0x22, 80)
        I1(T("XOR A"), 0xAF) I1(T("RET"), 0xC9) I2(T("LD A,6"), 0x3E, 0x06) IJPRT(RT_ERR)
        IFIN
};

/* RDI: 119 B */
#define rt_n_rdi 187
static const RtI rt_c_rdi[] = {
        IREFD(T("LD (%s),HL"), "rt_rd+88", 0x22, 88) ICALLRT(RT_RDTOK) I1(T("DEC A"), 0x3D) I1(T("RET Z"), 0xC8)
        IREFD(T("LD A,(%s)"), "rt_rd+87", 0x3A, 87) I1(T("OR A"), 0xB7) I2(T("JR NZ,$+102"), 0x20, 0x64)
        IREFD(T("LD A,(%s)"), "rt_rd+86", 0x3A, 86) I1(T("LD B,A"), 0x47) I1(T("OR A"), 0xB7)
        I2(T("JR Z,$+95"), 0x28, 0x5D) IREFD(T("LD HL,(%s)"), "rt_rd+84", 0x2A, 84) I2(T("LD C,0"), 0x0E, 0x00)
        I1(T("LD A,(HL)"), 0x7E) I2(T("CP 43"), 0xFE, 0x2B) I2(T("JR Z,$+7"), 0x28, 0x05) I2(T("CP 45"), 0xFE, 0x2D)
        I2(T("JR NZ,$+7"), 0x20, 0x05) I1(T("INC C"), 0x0C) I1(T("INC HL"), 0x23) I1(T("DEC B"), 0x05)
        I2(T("JR Z,$+76"), 0x28, 0x4A) I3(T("LD DE,%s"), "0", 0x11, 0x00, 0x00) I1(T("LD A,(HL)"), 0x7E)
        I2(T("SUB 48"), 0xD6, 0x30) I2(T("CP 10"), 0xFE, 0x0A) I2(T("JR NC,$+66"), 0x30, 0x40) I1(T("PUSH HL"), 0xE5)
        I1(T("LD H,D"), 0x62) I1(T("LD L,E"), 0x6B) I1(T("ADD HL,HL"), 0x29) I2(T("JR C,$+60"), 0x38, 0x3A)
        I1(T("LD D,H"), 0x54) I1(T("LD E,L"), 0x5D) I1(T("ADD HL,HL"), 0x29) I2(T("JR C,$+55"), 0x38, 0x35)
        I1(T("ADD HL,HL"), 0x29) I2(T("JR C,$+52"), 0x38, 0x32) I1(T("ADD HL,DE"), 0x19)
        I2(T("JR C,$+49"), 0x38, 0x2F) I1(T("LD E,A"), 0x5F) I2(T("LD D,0"), 0x16, 0x00) I1(T("ADD HL,DE"), 0x19)
        I2(T("JR C,$+43"), 0x38, 0x29) I1(T("LD A,H"), 0x7C) I2(T("CP 128"), 0xFE, 0x80) I2(T("JR C,$+8"), 0x38, 0x06)
        I2(T("JR NZ,$+36"), 0x20, 0x22) I1(T("LD A,L"), 0x7D) I1(T("OR A"), 0xB7) I2(T("JR NZ,$+32"), 0x20, 0x1E)
        I1(T("EX DE,HL"), 0xEB) I1(T("POP HL"), 0xE1) I1(T("INC HL"), 0x23) I1(T("DEC B"), 0x05)
        I2(T("JR NZ,$-45"), 0x20, 0xD1) I1(T("LD A,C"), 0x79) I1(T("OR A"), 0xB7) I2(T("JR Z,$+11"), 0x28, 0x09)
        I1(T("LD A,E"), 0x7B) I1(T("CPL"), 0x2F) I1(T("LD E,A"), 0x5F) I1(T("LD A,D"), 0x7A) I1(T("CPL"), 0x2F)
        I1(T("LD D,A"), 0x57) I1(T("INC DE"), 0x13) I2(T("JR $+6"), 0x18, 0x04) I2(T("BIT 7,D"), 0xCB, 0x7A)
        I2(T("JR NZ,$+9"), 0x20, 0x07) IREFD(T("LD HL,(%s)"), "rt_rd+88", 0x2A, 88) I1(T("LD (HL),E"), 0x73)
        I1(T("INC HL"), 0x23) I1(T("LD (HL),D"), 0x72) I1(T("RET"), 0xC9) I2(T("LD A,6"), 0x3E, 0x06) IJPRT(RT_ERR)
        IFIN
};

/* RDL: 62 B */
#define rt_n_rdl 88
static const RtI rt_c_rdl[] = {
        IREFD(T("LD (%s),HL"), "rt_rd+88", 0x22, 88) ICALLRT(RT_RDTOK) I1(T("DEC A"), 0x3D) I1(T("RET Z"), 0xC8)
        IREFD(T("LD A,(%s)"), "rt_rd+87", 0x3A, 87) I1(T("OR A"), 0xB7) I2(T("JR NZ,$+45"), 0x20, 0x2B)
        IREFD(T("LD A,(%s)"), "rt_rd+86", 0x3A, 86) I1(T("LD B,A"), 0x47) I1(T("OR A"), 0xB7)
        I2(T("JR Z,$+38"), 0x28, 0x24) IREFD(T("LD HL,(%s)"), "rt_rd+84", 0x2A, 84) I1(T("LD A,(HL)"), 0x7E)
        I2(T("CP 46"), 0xFE, 0x2E) I2(T("JR NZ,$+7"), 0x20, 0x05) I1(T("INC HL"), 0x23) I1(T("DEC B"), 0x05)
        I2(T("JR Z,$+26"), 0x28, 0x18) I1(T("LD A,(HL)"), 0x7E) I2(T("OR 32"), 0xF6, 0x20)
        I3(T("LD DE,%s"), "0", 0x11, 0x00, 0x00) I2(T("CP 116"), 0xFE, 0x74) I2(T("JR Z,$+8"), 0x28, 0x06)
        I2(T("CP 102"), 0xFE, 0x66) I2(T("JR NZ,$+12"), 0x20, 0x0A) I2(T("JR $+3"), 0x18, 0x01) I1(T("INC E"), 0x1C)
        IREFD(T("LD HL,(%s)"), "rt_rd+88", 0x2A, 88) I1(T("LD (HL),E"), 0x73) I1(T("INC HL"), 0x23)
        I1(T("LD (HL),D"), 0x72) I1(T("RET"), 0xC9) I2(T("LD A,6"), 0x3E, 0x06) IJPRT(RT_ERR)
        IFIN
};

/* RDS: 35 B */
#define rt_n_rds 57
static const RtI rt_c_rds[] = {
        I1(T("PUSH DE"), 0xD5) I1(T("PUSH BC"), 0xC5) ICALLRT(RT_RDTOK) I1(T("POP BC"), 0xC1) I1(T("POP DE"), 0xD1)
        I1(T("DEC A"), 0x3D) I1(T("RET Z"), 0xC8) IREFD(T("LD HL,(%s)"), "rt_rd+84", 0x2A, 84)
        IREFD(T("LD A,(%s)"), "rt_rd+86", 0x3A, 86) I1(T("LD B,A"), 0x47) I1(T("LD A,C"), 0x79) I1(T("OR A"), 0xB7)
        I1(T("RET Z"), 0xC8) I1(T("DEC C"), 0x0D) I1(T("LD A,B"), 0x78) I1(T("OR A"), 0xB7)
        I2(T("JR Z,$+7"), 0x28, 0x05) I1(T("DEC B"), 0x05) I1(T("LD A,(HL)"), 0x7E) I1(T("INC HL"), 0x23)
        I2(T("JR $+4"), 0x18, 0x02) I2(T("LD A,32"), 0x3E, 0x20) I1(T("LD (DE),A"), 0x12) I1(T("INC DE"), 0x13)
        I2(T("JR $-17"), 0x18, 0xED)
        IFIN
};

/* RDF: 391 B */
#define rt_n_rdf 501
static const RtI rt_c_rdf[] = {
        I2(T("JR $+7"), 0x18, 0x05) I2(T("LD A,6"), 0x3E, 0x06) IJPRT(RT_ERR)
        IREFD(T("LD (%s),HL"), "rt_rd+88", 0x22, 88) ICALLRT(RT_RDTOK) I1(T("DEC A"), 0x3D) I1(T("RET Z"), 0xC8)
        IREFD(T("LD A,(%s)"), "rt_rd+87", 0x3A, 87) I1(T("OR A"), 0xB7) IREFR(T("JP NZ,%s"), "rt_rdf+2", 0xC2, 2)
        IREFD(T("LD A,(%s)"), "rt_rd+86", 0x3A, 86) I1(T("LD B,A"), 0x47) I1(T("OR A"), 0xB7)
        IREFR(T("JP Z,%s"), "rt_rdf+2", 0xCA, 2) I1(T("XOR A"), 0xAF) IREFD(T("LD (%s),A"), "rt_rd+102", 0x32, 102)
        IREFD(T("LD (%s),A"), "rt_rd+103", 0x32, 103) IREFD(T("LD (%s),A"), "rt_rd+104", 0x32, 104)
        IREFD(T("LD (%s),A"), "rt_rd+107", 0x32, 107) IREFD(T("LD (%s),A"), "rt_rd+108", 0x32, 108)
        IREFD(T("LD (%s),A"), "rt_rd+90", 0x32, 90) IREFD(T("LD (%s),A"), "rt_rd+91", 0x32, 91)
        IREFD(T("LD (%s),A"), "rt_rd+92", 0x32, 92) IREFD(T("LD (%s),A"), "rt_rd+93", 0x32, 93)
        IREFD(T("LD HL,(%s)"), "rt_rd+84", 0x2A, 84) I1(T("LD A,(HL)"), 0x7E) I2(T("CP 43"), 0xFE, 0x2B)
        I2(T("JR Z,$+11"), 0x28, 0x09) I2(T("CP 45"), 0xFE, 0x2D) I2(T("JR NZ,$+12"), 0x20, 0x0A)
        I2(T("LD A,1"), 0x3E, 0x01) IREFD(T("LD (%s),A"), "rt_rd+102", 0x32, 102) I1(T("INC HL"), 0x23)
        I1(T("DEC B"), 0x05) IREFR(T("JP Z,%s"), "rt_rdf+2", 0xCA, 2) I1(T("LD A,(HL)"), 0x7E)
        I2(T("CP 46"), 0xFE, 0x2E) I2(T("JR NZ,$+15"), 0x20, 0x0D) IREFD(T("LD A,(%s)"), "rt_rd+104", 0x3A, 104)
        I1(T("OR A"), 0xB7) IREFR(T("JP NZ,%s"), "rt_rdf+2", 0xC2, 2) I1(T("INC A"), 0x3C)
        IREFD(T("LD (%s),A"), "rt_rd+104", 0x32, 104) I2(T("JR $+83"), 0x18, 0x51) I2(T("SUB 48"), 0xD6, 0x30)
        I2(T("CP 10"), 0xFE, 0x0A) I2(T("JR NC,$+83"), 0x30, 0x51) I1(T("PUSH HL"), 0xE5) I1(T("PUSH BC"), 0xC5)
        IREFD(T("LD (%s),A"), "rt_rd+105", 0x32, 105) I3(T("LD HL,%s"), "10", 0x21, 0x0A, 0x00)
        IREFD(T("LD BC,%s"), "rt_rd+94", 0x01, 94) ICALLRT(RT_ITOF) IREFD(T("LD HL,%s"), "rt_rd+90", 0x21, 90)
        IREFD(T("LD DE,%s"), "rt_rd+94", 0x11, 94) IREFD(T("LD BC,%s"), "rt_rd+90", 0x01, 90) ICALLRT(RT_FMUL)
        IREFD(T("LD A,(%s)"), "rt_rd+105", 0x3A, 105) I1(T("LD L,A"), 0x6F) I2(T("LD H,0"), 0x26, 0x00)
        IREFD(T("LD BC,%s"), "rt_rd+98", 0x01, 98) ICALLRT(RT_ITOF) IREFD(T("LD HL,%s"), "rt_rd+90", 0x21, 90)
        IREFD(T("LD DE,%s"), "rt_rd+98", 0x11, 98) IREFD(T("LD BC,%s"), "rt_rd+90", 0x01, 90) I1(T("XOR A"), 0xAF)
        ICALLRT(RT_FADD) I1(T("POP BC"), 0xC1) I1(T("POP HL"), 0xE1) IREFD(T("LD A,(%s)"), "rt_rd+103", 0x3A, 103)
        I1(T("INC A"), 0x3C) IREFD(T("LD (%s),A"), "rt_rd+103", 0x32, 103)
        IREFD(T("LD A,(%s)"), "rt_rd+104", 0x3A, 104) I1(T("OR A"), 0xB7) I2(T("JR Z,$+11"), 0x28, 0x09)
        I1(T("PUSH HL"), 0xE5) IREFD(T("LD HL,(%s)"), "rt_rd+107", 0x2A, 107) I1(T("DEC HL"), 0x2B)
        IREFD(T("LD (%s),HL"), "rt_rd+107", 0x22, 107) I1(T("POP HL"), 0xE1) I1(T("INC HL"), 0x23)
        I1(T("DEC B"), 0x05) I2(T("JR NZ,$-101"), 0x20, 0x99) I2(T("JR $+89"), 0x18, 0x57) I1(T("LD A,(HL)"), 0x7E)
        I2(T("OR 32"), 0xF6, 0x20) I2(T("CP 101"), 0xFE, 0x65) I2(T("JR Z,$+7"), 0x28, 0x05)
        I2(T("CP 100"), 0xFE, 0x64) IREFR(T("JP NZ,%s"), "rt_rdf+2", 0xC2, 2) I1(T("INC HL"), 0x23)
        I1(T("DEC B"), 0x05) IREFR(T("JP Z,%s"), "rt_rdf+2", 0xCA, 2) I2(T("LD C,0"), 0x0E, 0x00)
        I1(T("LD A,(HL)"), 0x7E) I2(T("CP 43"), 0xFE, 0x2B) I2(T("JR Z,$+7"), 0x28, 0x05) I2(T("CP 45"), 0xFE, 0x2D)
        I2(T("JR NZ,$+8"), 0x20, 0x06) I1(T("INC C"), 0x0C) I1(T("INC HL"), 0x23) I1(T("DEC B"), 0x05)
        IREFR(T("JP Z,%s"), "rt_rdf+2", 0xCA, 2) I3(T("LD DE,%s"), "0", 0x11, 0x00, 0x00) I1(T("LD A,(HL)"), 0x7E)
        I2(T("SUB 48"), 0xD6, 0x30) I2(T("CP 10"), 0xFE, 0x0A) IREFR(T("JP NC,%s"), "rt_rdf+2", 0xD2, 2)
        I1(T("PUSH HL"), 0xE5) I1(T("LD H,D"), 0x62) I1(T("LD L,E"), 0x6B) I1(T("ADD HL,HL"), 0x29)
        I1(T("LD D,H"), 0x54) I1(T("LD E,L"), 0x5D) I1(T("ADD HL,HL"), 0x29) I1(T("ADD HL,HL"), 0x29)
        I1(T("ADD HL,DE"), 0x19) I1(T("LD E,A"), 0x5F) I2(T("LD D,0"), 0x16, 0x00) I1(T("ADD HL,DE"), 0x19)
        I1(T("EX DE,HL"), 0xEB) I1(T("POP HL"), 0xE1) I1(T("LD A,D"), 0x7A) I2(T("CP 16"), 0xFE, 0x10)
        I2(T("JR C,$+5"), 0x38, 0x03) I3(T("LD DE,%s"), "4000", 0x11, 0xA0, 0x0F) I1(T("INC HL"), 0x23)
        I1(T("DEC B"), 0x05) I2(T("JR NZ,$-33"), 0x20, 0xDD) IREFD(T("LD HL,(%s)"), "rt_rd+107", 0x2A, 107)
        I1(T("LD A,C"), 0x79) I1(T("OR A"), 0xB7) I2(T("JR Z,$+6"), 0x28, 0x04) I2(T("SBC HL,DE"), 0xED, 0x52)
        I2(T("JR $+3"), 0x18, 0x01) I1(T("ADD HL,DE"), 0x19) IREFD(T("LD (%s),HL"), "rt_rd+107", 0x22, 107)
        IREFD(T("LD A,(%s)"), "rt_rd+103", 0x3A, 103) I1(T("OR A"), 0xB7) IREFR(T("JP Z,%s"), "rt_rdf+2", 0xCA, 2)
        IREFD(T("LD HL,(%s)"), "rt_rd+107", 0x2A, 107) I1(T("LD A,H"), 0x7C) I1(T("OR L"), 0xB5)
        I2(T("JR Z,$+83"), 0x28, 0x51) IREFD(T("LD A,(%s)"), "rt_rd+93", 0x3A, 93) I2(T("AND 127"), 0xE6, 0x7F)
        I2(T("JR NZ,$+9"), 0x20, 0x07) IREFD(T("LD A,(%s)"), "rt_rd+92", 0x3A, 92) I2(T("AND 128"), 0xE6, 0x80)
        I2(T("JR Z,$+69"), 0x28, 0x43) I1(T("XOR A"), 0xAF) IREFD(T("LD (%s),A"), "rt_rd+106", 0x32, 106)
        I2(T("BIT 7,H"), 0xCB, 0x7C) I2(T("JR Z,$+13"), 0x28, 0x0B) I1(T("INC A"), 0x3C)
        IREFD(T("LD (%s),A"), "rt_rd+106", 0x32, 106) I1(T("LD A,L"), 0x7D) I1(T("CPL"), 0x2F) I1(T("LD L,A"), 0x6F)
        I1(T("LD A,H"), 0x7C) I1(T("CPL"), 0x2F) I1(T("LD H,A"), 0x67) I1(T("INC HL"), 0x23)
        IREFD(T("LD (%s),HL"), "rt_rd+107", 0x22, 107) I3(T("LD HL,%s"), "10", 0x21, 0x0A, 0x00)
        IREFD(T("LD BC,%s"), "rt_rd+98", 0x01, 98) ICALLRT(RT_ITOF) IREFD(T("LD HL,%s"), "rt_rd+98", 0x21, 98)
        IREFDE(T("LD DE,(%s)"), "rt_rd+107", 0xED, 0x5B, 107) IREFD(T("LD BC,%s"), "rt_rd+94", 0x01, 94)
        ICALLRT(RT_FPOW) IREFD(T("LD HL,%s"), "rt_rd+90", 0x21, 90) IREFD(T("LD DE,%s"), "rt_rd+94", 0x11, 94)
        IREFD(T("LD BC,%s"), "rt_rd+90", 0x01, 90) IREFD(T("LD A,(%s)"), "rt_rd+106", 0x3A, 106) I1(T("OR A"), 0xB7)
        I2(T("JR NZ,$+7"), 0x20, 0x05) ICALLRT(RT_FMUL) I2(T("JR $+5"), 0x18, 0x03) ICALLRT(RT_FDIV)
        IREFD(T("LD A,(%s)"), "rt_rd+102", 0x3A, 102) I1(T("OR A"), 0xB7) IREFD(T("LD HL,%s"), "rt_rd+90", 0x21, 90)
        IREFDE(T("LD DE,(%s)"), "rt_rd+88", 0xED, 0x5B, 88) I2(T("JR Z,$+7"), 0x28, 0x05) I1(T("LD B,D"), 0x42)
        I1(T("LD C,E"), 0x4B) IJPRT(RT_FNEG) I3(T("LD BC,%s"), "4", 0x01, 0x04, 0x00) I2(T("LDIR"), 0xED, 0xB0)
        I1(T("RET"), 0xC9)
        IFIN
};

