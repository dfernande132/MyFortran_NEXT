/* z80emu.c - Emulador Z80 minimo (ver z80emu.h). */
#include <string.h>
#include "z80emu.h"

#define FS 0x80
#define FZ 0x40
#define FC 0x01

typedef struct {
    uint8_t a, f, b, c, d, e, h, l;
    uint8_t a2, f2, b2, c2, d2, e2, h2, l2;       /* registros alternativos (EXX, EX AF,AF') */
    uint16_t sp, pc;
    uint8_t *m;
} Cpu;

static uint8_t rd(const Cpu *c, uint16_t a) { return c->m[a]; }
static uint16_t rd16(const Cpu *c, uint16_t a) { return (uint16_t)(c->m[a] | (c->m[(uint16_t)(a + 1)] << 8)); }
static void wr(Cpu *c, uint16_t a, uint8_t v) { c->m[a] = v; }
static void wr16(Cpu *c, uint16_t a, uint16_t v) { c->m[a] = (uint8_t)v; c->m[(uint16_t)(a + 1)] = (uint8_t)(v >> 8); }

static uint16_t bc(const Cpu *c) { return (uint16_t)((c->b << 8) | c->c); }
static uint16_t de(const Cpu *c) { return (uint16_t)((c->d << 8) | c->e); }
static uint16_t hl(const Cpu *c) { return (uint16_t)((c->h << 8) | c->l); }
static void set_bc(Cpu *c, uint16_t v) { c->b = (uint8_t)(v >> 8); c->c = (uint8_t)v; }
static void set_de(Cpu *c, uint16_t v) { c->d = (uint8_t)(v >> 8); c->e = (uint8_t)v; }
static void set_hl(Cpu *c, uint16_t v) { c->h = (uint8_t)(v >> 8); c->l = (uint8_t)v; }

/* registro 0..7 = B C D E H L (HL) A */
static uint8_t getr(Cpu *c, int r)
{
    switch (r) {
    case 0: return c->b; case 1: return c->c; case 2: return c->d; case 3: return c->e;
    case 4: return c->h; case 5: return c->l; case 6: return rd(c, hl(c)); default: return c->a;
    }
}

static void setr(Cpu *c, int r, uint8_t v)
{
    switch (r) {
    case 0: c->b = v; break; case 1: c->c = v; break; case 2: c->d = v; break; case 3: c->e = v; break;
    case 4: c->h = v; break; case 5: c->l = v; break; case 6: wr(c, hl(c), v); break; default: c->a = v; break;
    }
}

static uint16_t getrr(Cpu *c, int p)      /* 0 BC 1 DE 2 HL 3 SP */
{
    return p == 0 ? bc(c) : p == 1 ? de(c) : p == 2 ? hl(c) : c->sp;
}

static void setrr(Cpu *c, int p, uint16_t v)
{
    if (p == 0) set_bc(c, v); else if (p == 1) set_de(c, v); else if (p == 2) set_hl(c, v); else c->sp = v;
}

static void flags_szc(Cpu *c, uint8_t r, int carry)
{
    c->f = (uint8_t)((r & FS) | (r == 0 ? FZ : 0) | (carry ? FC : 0));
}

/* ADD, ADC, SUB, SBC, AND, XOR, OR, CP sobre A */
static void alu(Cpu *c, int op, uint8_t v)
{
    int cin = (c->f & FC) ? 1 : 0;
    int r;

    switch (op) {
    case 0: r = c->a + v; flags_szc(c, (uint8_t)r, r > 255); c->a = (uint8_t)r; break;
    case 1: r = c->a + v + cin; flags_szc(c, (uint8_t)r, r > 255); c->a = (uint8_t)r; break;
    case 2: r = c->a - v; flags_szc(c, (uint8_t)r, r < 0); c->a = (uint8_t)r; break;
    case 3: r = c->a - v - cin; flags_szc(c, (uint8_t)r, r < 0); c->a = (uint8_t)r; break;
    case 4: c->a &= v; flags_szc(c, c->a, 0); break;
    case 5: c->a ^= v; flags_szc(c, c->a, 0); break;
    case 6: c->a |= v; flags_szc(c, c->a, 0); break;
    default: r = c->a - v; flags_szc(c, (uint8_t)r, r < 0); break;      /* CP */
    }
}

static int cond(const Cpu *c, int cc)    /* NZ Z NC C PO PE P M */
{
    switch (cc) {
    case 0: return !(c->f & FZ);
    case 1: return (c->f & FZ) != 0;
    case 2: return !(c->f & FC);
    case 3: return (c->f & FC) != 0;
    case 6: return !(c->f & FS);
    case 7: return (c->f & FS) != 0;
    default: return -1;                  /* PO/PE: no implementados */
    }
}

static void push(Cpu *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2); wr16(c, c->sp, v); }
static uint16_t pop(Cpu *c) { uint16_t v = rd16(c, c->sp); c->sp = (uint16_t)(c->sp + 2); return v; }

void z80emu_ejecutar(uint8_t *mem, uint16_t pc, uint32_t max_pasos,
                     char *salida, size_t n, Z80EmuRes *res)
{
    Cpu c;
    size_t u = 0;
    uint16_t sp_ini, sp_min;

    memset(&c, 0, sizeof(c));
    memset(res, 0, sizeof(*res));
    c.m = mem;
    c.sp = 0xFFF0;
    sp_ini = c.sp;
    sp_min = c.sp;
    c.pc = pc;
    wr16(&c, (uint16_t)(c.sp - 2), 0x0000);       /* RET final -> direccion 0 */
    c.sp = (uint16_t)(c.sp - 2);
    salida[0] = '\0';
    res->estado = Z80EMU_OK;

    while (c.pc != 0) {
        uint8_t op;
        uint16_t nn;
        int r, s, cc;

        if (c.sp < sp_min) {
            sp_min = c.sp;
        }
        if (res->pasos++ >= max_pasos) {
            res->estado = Z80EMU_LIMITE;
            break;
        }
        res->pc = c.pc;
        op = rd(&c, c.pc++);
        res->op = op;

        if (op >= 0x40 && op <= 0x7F && op != 0x76) {             /* LD r,r' */
            setr(&c, (op >> 3) & 7, getr(&c, op & 7));
            continue;
        }
        if (op >= 0x80 && op <= 0xBF) {                             /* ALU A,r */
            alu(&c, (op >> 3) & 7, getr(&c, op & 7));
            continue;
        }
        switch (op) {
        case 0x00: break;
        case 0x76: goto fin;                                       /* HALT: fin del programa (error de ejecucion) */
        case 0x0A: c.a = rd(&c, bc(&c)); break;
        case 0x07: { int cy = c.a >> 7; c.a = (uint8_t)((c.a << 1) | cy); c.f = (uint8_t)((c.f & (FS | FZ)) | cy); break; }
        case 0x0F: { int cy = c.a & 1; c.a = (uint8_t)((c.a >> 1) | (cy << 7)); c.f = (uint8_t)((c.f & (FS | FZ)) | cy); break; }
        case 0x17: { int cy = c.a >> 7; c.a = (uint8_t)((c.a << 1) | (c.f & FC)); c.f = (uint8_t)((c.f & (FS | FZ)) | cy); break; }
        case 0x1F: { int cy = c.a & 1; c.a = (uint8_t)((c.a >> 1) | ((c.f & FC) << 7)); c.f = (uint8_t)((c.f & (FS | FZ)) | cy); break; }
        case 0x37: c.f = (uint8_t)((c.f & (FS | FZ)) | FC); break;
        case 0x3F: c.f = (uint8_t)((c.f & (FS | FZ)) | ((c.f & FC) ? 0 : FC)); break;
        case 0x08: { uint8_t t = c.a; c.a = c.a2; c.a2 = t; t = c.f; c.f = c.f2; c.f2 = t; break; }
        case 0xD9: {
            uint8_t t;
            t = c.b; c.b = c.b2; c.b2 = t; t = c.c; c.c = c.c2; c.c2 = t;
            t = c.d; c.d = c.d2; c.d2 = t; t = c.e; c.e = c.e2; c.e2 = t;
            t = c.h; c.h = c.h2; c.h2 = t; t = c.l; c.l = c.l2; c.l2 = t;
            break;
        }
        case 0xCE: alu(&c, 1, rd(&c, c.pc++)); break;
        case 0xD6: alu(&c, 2, rd(&c, c.pc++)); break;
        case 0xDE: alu(&c, 3, rd(&c, c.pc++)); break;
        case 0x01: case 0x11: case 0x21:
            nn = rd16(&c, c.pc); c.pc = (uint16_t)(c.pc + 2);
            setrr(&c, op >> 4, nn);
            break;
        case 0x2A: nn = rd16(&c, c.pc); c.pc = (uint16_t)(c.pc + 2); set_hl(&c, rd16(&c, nn)); break;
        case 0x22: nn = rd16(&c, c.pc); c.pc = (uint16_t)(c.pc + 2); wr16(&c, nn, hl(&c)); break;
        case 0x06: case 0x0E: case 0x16: case 0x1E: case 0x26: case 0x2E: case 0x36: case 0x3E:
            setr(&c, (op >> 3) & 7, rd(&c, c.pc++));
            break;
        case 0x04: case 0x0C: case 0x14: case 0x1C: case 0x24: case 0x2C: case 0x34: case 0x3C:
        case 0x05: case 0x0D: case 0x15: case 0x1D: case 0x25: case 0x2D: case 0x35: case 0x3D: {
            uint8_t v = getr(&c, (op >> 3) & 7);
            uint8_t cy = (uint8_t)(c.f & FC);

            v = (uint8_t)((op & 1) ? v - 1 : v + 1);
            setr(&c, (op >> 3) & 7, v);
            flags_szc(&c, v, cy);
            break;
        }
        case 0x03: case 0x13: case 0x23: setrr(&c, op >> 4, (uint16_t)(getrr(&c, op >> 4) + 1)); break;
        case 0x0B: case 0x1B: case 0x2B: setrr(&c, op >> 4, (uint16_t)(getrr(&c, op >> 4) - 1)); break;
        case 0x09: case 0x19: case 0x29: {
            uint32_t v = (uint32_t)hl(&c) + getrr(&c, op >> 4);

            c.f = (uint8_t)((c.f & (FS | FZ)) | (v > 0xFFFF ? FC : 0));
            set_hl(&c, (uint16_t)v);
            break;
        }
        case 0x3A: nn = rd16(&c, c.pc); c.pc = (uint16_t)(c.pc + 2); c.a = rd(&c, nn); break;
        case 0x32: nn = rd16(&c, c.pc); c.pc = (uint16_t)(c.pc + 2); wr(&c, nn, c.a); break;
        case 0x12: wr(&c, de(&c), c.a); break;
        case 0x1A: c.a = rd(&c, de(&c)); break;
        case 0x02: wr(&c, bc(&c), c.a); break;
        case 0xF6: alu(&c, 6, rd(&c, c.pc++)); break;
        case 0x2F: c.a = (uint8_t)~c.a; break;
        case 0xEB: { uint16_t t = hl(&c); set_hl(&c, de(&c)); set_de(&c, t); break; }
        case 0xC6: alu(&c, 0, rd(&c, c.pc++)); break;
        case 0xE6: alu(&c, 4, rd(&c, c.pc++)); break;
        case 0xEE: alu(&c, 5, rd(&c, c.pc++)); break;
        case 0xFE: alu(&c, 7, rd(&c, c.pc++)); break;
        case 0xF5: push(&c, (uint16_t)((c.a << 8) | c.f)); break;
        case 0xC5: push(&c, bc(&c)); break;
        case 0xD5: push(&c, de(&c)); break;
        case 0xE5: push(&c, hl(&c)); break;
        case 0xF1: nn = pop(&c); c.a = (uint8_t)(nn >> 8); c.f = (uint8_t)nn; break;
        case 0xC1: set_bc(&c, pop(&c)); break;
        case 0xD1: set_de(&c, pop(&c)); break;
        case 0xE1: set_hl(&c, pop(&c)); break;
        case 0xC3: c.pc = rd16(&c, c.pc); break;
        case 0xCD: nn = rd16(&c, c.pc); push(&c, (uint16_t)(c.pc + 2)); c.pc = nn; break;
        case 0xC9: c.pc = pop(&c); break;
        case 0xC2: case 0xCA: case 0xD2: case 0xDA: case 0xF2: case 0xFA:
            cc = cond(&c, (op >> 3) & 7);
            nn = rd16(&c, c.pc);
            if (cc < 0) { res->estado = Z80EMU_OPCODE; goto fin; }
            c.pc = cc ? nn : (uint16_t)(c.pc + 2);
            break;
        case 0xC4: case 0xCC:
            cc = cond(&c, (op >> 3) & 7);
            nn = rd16(&c, c.pc);
            c.pc = (uint16_t)(c.pc + 2);
            if (cc) { push(&c, c.pc); c.pc = nn; }
            break;
        case 0xC0: case 0xC8: case 0xD0: case 0xD8: case 0xF0:
            if (cond(&c, (op >> 3) & 7)) c.pc = pop(&c);
            break;
        case 0x18: r = (int8_t)rd(&c, c.pc++); c.pc = (uint16_t)(c.pc + r); break;
        case 0x20: case 0x28: case 0x30: case 0x38:
            r = (int8_t)rd(&c, c.pc++);
            if (cond(&c, ((op >> 3) & 7) - 4)) c.pc = (uint16_t)(c.pc + r);
            break;
        case 0x10:
            r = (int8_t)rd(&c, c.pc++);
            c.b--;
            if (c.b != 0) c.pc = (uint16_t)(c.pc + r);
            break;
        case 0xD7:                                                  /* RST 16 */
            if (u + 1 < n) {
                salida[u++] = c.a == 13 ? '\n' : (char)c.a;
                salida[u] = '\0';
            }
            break;
        case 0xCB:
            op = rd(&c, c.pc++);
            r = op & 7;
            s = op >> 6;
            if (s == 1) {                                           /* BIT b,r */
                uint8_t v = getr(&c, r);
                int bit = (op >> 3) & 7;

                c.f = (uint8_t)((c.f & FC) | ((v >> bit) & 1 ? 0 : FZ));
            } else if (s == 2 || s == 3) {                          /* RES / SET b,r */
                uint8_t v = getr(&c, r);
                int bit = (op >> 3) & 7;

                setr(&c, r, s == 2 ? (uint8_t)(v & ~(1 << bit)) : (uint8_t)(v | (1 << bit)));
            } else if (((op >> 3) & 7) != 6) {                      /* RLC RRC RL RR SLA SRA SRL */
                uint8_t v = getr(&c, r);
                int cy, cin = (c.f & FC) ? 1 : 0;

                switch ((op >> 3) & 7) {
                case 0: cy = v >> 7; v = (uint8_t)((v << 1) | cy); break;
                case 1: cy = v & 1; v = (uint8_t)((v >> 1) | (cy << 7)); break;
                case 2: cy = v >> 7; v = (uint8_t)((v << 1) | cin); break;
                case 3: cy = v & 1; v = (uint8_t)((v >> 1) | (cin << 7)); break;
                case 4: cy = v >> 7; v = (uint8_t)(v << 1); break;
                case 5: cy = v & 1; v = (uint8_t)((v >> 1) | (v & 0x80)); break;
                default: cy = v & 1; v = (uint8_t)(v >> 1); break;
                }
                setr(&c, r, v);
                flags_szc(&c, v, cy);
            } else {
                res->estado = Z80EMU_OPCODE;
                res->op = op;
                goto fin;
            }
            break;
        case 0xED:
            op = rd(&c, c.pc++);
            if (op == 0x5B) {                                       /* LD DE,(nn) */
                nn = rd16(&c, c.pc); c.pc = (uint16_t)(c.pc + 2);
                set_de(&c, rd16(&c, nn));
            } else if (op == 0x4B) {                                /* LD BC,(nn) */
                nn = rd16(&c, c.pc); c.pc = (uint16_t)(c.pc + 2);
                set_bc(&c, rd16(&c, nn));
            } else if (op == 0x73) {                                /* LD (nn),SP */
                nn = rd16(&c, c.pc); c.pc = (uint16_t)(c.pc + 2);
                wr16(&c, nn, c.sp);
            } else if (op == 0x7B) {                                /* LD SP,(nn) */
                nn = rd16(&c, c.pc); c.pc = (uint16_t)(c.pc + 2);
                c.sp = rd16(&c, nn);
            } else if (op == 0x53) {                                /* LD (nn),DE */
                nn = rd16(&c, c.pc); c.pc = (uint16_t)(c.pc + 2);
                wr16(&c, nn, de(&c));
            } else if (op == 0x43) {                                /* LD (nn),BC */
                nn = rd16(&c, c.pc); c.pc = (uint16_t)(c.pc + 2);
                wr16(&c, nn, bc(&c));
            } else if (op == 0x44) {                                /* NEG */
                uint8_t v = (uint8_t)(0 - c.a);

                flags_szc(&c, v, c.a != 0);
                c.a = v;
            } else if (op == 0xA0 || op == 0xA8) {                  /* LDI / LDD */
                int dlt = op == 0xA0 ? 1 : -1;

                wr(&c, de(&c), rd(&c, hl(&c)));
                set_hl(&c, (uint16_t)(hl(&c) + dlt));
                set_de(&c, (uint16_t)(de(&c) + dlt));
                set_bc(&c, (uint16_t)(bc(&c) - 1));
            } else if (op == 0xB8) {                                /* LDDR */
                do {
                    wr(&c, de(&c), rd(&c, hl(&c)));
                    set_hl(&c, (uint16_t)(hl(&c) - 1));
                    set_de(&c, (uint16_t)(de(&c) - 1));
                    set_bc(&c, (uint16_t)(bc(&c) - 1));
                } while (bc(&c) != 0);
            } else if (op == 0xB0) {                                /* LDIR */
                do {                                                /* como el real: BC = 0 copia 65536 */
                    wr(&c, de(&c), rd(&c, hl(&c)));
                    set_hl(&c, (uint16_t)(hl(&c) + 1));
                    set_de(&c, (uint16_t)(de(&c) + 1));
                    set_bc(&c, (uint16_t)(bc(&c) - 1));
                } while (bc(&c) != 0);
            } else if ((op & 0xCF) == 0x42 || (op & 0xCF) == 0x4A) {   /* SBC / ADC HL,rr */
                uint32_t a = hl(&c), b = getrr(&c, (op >> 4) & 3), cin = (c.f & FC) ? 1u : 0u, v;
                int carry;
                uint16_t rr;

                if ((op & 0xCF) == 0x42) { v = a - b - cin; carry = (a < b + cin); }
                else { v = a + b + cin; carry = v > 0xFFFF; }
                rr = (uint16_t)v;
                set_hl(&c, rr);
                c.f = (uint8_t)((rr & 0x8000 ? FS : 0) | (rr == 0 ? FZ : 0) | (carry ? FC : 0));
            } else {
                res->estado = Z80EMU_OPCODE;
                res->op = op;
                goto fin;
            }
            break;
        default:
            res->estado = Z80EMU_OPCODE;
            goto fin;
        }
    }
fin:
    if (c.sp < sp_min) {
        sp_min = c.sp;
    }
    res->pila_usada = (uint16_t)(sp_ini - sp_min);
    res->ciclos_aprox = res->pasos;
}
