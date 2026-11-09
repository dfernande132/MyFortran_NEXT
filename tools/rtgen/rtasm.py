#!/usr/bin/env python3
"""rtasm.py - mini ensamblador Z80 que convierte tools/rtgen/rt_real.asm en las tablas de compiler/src/back_rt_real.h.

Solo entiende las instrucciones que usan las rutinas REAL. Los saltos JR/DJNZ llevan el desplazamiento ya calculado (no usan
las etiquetas locales del backend); las llamadas a otras rutinas son CALL/JP RT_xxx; los datos del bloque de trabajo se
nombran @simbolo (+n) y salen como instrucciones RK_REFO / RK_REFE (direccion = base del bloque RT_FPD + desplazamiento).

Uso: rtasm.py rt_real.asm back_rt_real.h
"""
import re, sys

R8 = {'B': 0, 'C': 1, 'D': 2, 'E': 3, 'H': 4, 'L': 5, '(HL)': 6, 'A': 7}
RR = {'BC': 0, 'DE': 1, 'HL': 2, 'SP': 3}
CC = {'NZ': 0, 'Z': 1, 'NC': 2, 'C': 3, 'PO': 4, 'PE': 5, 'P': 6, 'M': 7}
ALU = {'ADD': 0, 'ADC': 1, 'SUB': 2, 'SBC': 3, 'AND': 4, 'XOR': 5, 'OR': 6, 'CP': 7}
ROT = {'RLC': 0, 'RRC': 1, 'RL': 2, 'RR': 3, 'SLA': 4, 'SRA': 5, 'SRL': 7}
SIMPLE = {'NOP': [0x00], 'RLCA': [7], 'RRCA': [0xF], 'RLA': [0x17], 'RRA': [0x1F], 'CPL': [0x2F], 'SCF': [0x37], 'CCF': [0x3F],
          'EXX': [0xD9], 'RET': [0xC9], 'NEG': [0xED, 0x44], 'LDI': [0xED, 0xA0], 'LDIR': [0xED, 0xB0],
          'LDD': [0xED, 0xA8], 'LDDR': [0xED, 0xB8], 'HALT': [0x76]}


class Err(Exception):
    pass


def num(s):
    s = s.strip()
    if s.startswith("'") and s.endswith("'") and len(s) == 3:
        return ord(s[1])
    return int(s, 0)


class Rut:
    def __init__(self, nombre):
        self.nombre = nombre
        self.items = []      # ('b', bytes, texto, arg) | ('jr', ...) | ('call', ...) | ('ref', ...) | ('lbl', nombre) | ('dat', n)
        self.syms = {}


def parse(fuente):
    rutinas = []
    simbolos = {}
    actual = None
    for n, linea in enumerate(fuente.splitlines(), 1):
        linea = linea.split(';')[0].strip()
        if not linea:
            continue
        m = re.match(r'^\.sym\s+(\w+)\s*=\s*(\S+)$', linea)
        if m:
            simbolos[m.group(1)] = num(m.group(2))
            continue
        m = re.match(r'^\.rut\s+(\w+)$', linea)
        if m:
            actual = Rut(m.group(1))
            rutinas.append(actual)
            continue
        m = re.match(r'^\.data\s+(\d+)$', linea)
        if m:
            actual.items.append(('dat', int(m.group(1)), n))
            continue
        m = re.match(r'^\.db\s+(.+)$', linea)
        if m:
            bs = []
            for tok in re.findall(r'"[^"]*"|[^,\s][^,]*', m.group(1)):
                tok = tok.strip()
                if tok.startswith('"'):
                    bs += [ord(c) for c in tok[1:-1]]
                else:
                    bs.append(num(tok) & 0xFF)
            actual.items.append(('db', bs, n))
            continue
        m = re.match(r'^(\w+):$', linea)
        if m:
            actual.items.append(('lbl', m.group(1), n))
            continue
        actual.items.append(('ins', linea, n))
    return rutinas, simbolos


def sym_off(tok, simbolos):
    """@nombre[+n] -> (desplazamiento, texto)"""
    m = re.match(r'^([@$])(\w+)(?:\+(\d+))?$', tok)
    if not m:
        return None
    base = simbolos[m.group(2)]
    add = int(m.group(3)) if m.group(3) else 0
    off = base + add
    nom = 'rt_fs' if m.group(1) == '@' else 'rt_rd'
    return off, ('%s+%d' % (nom, off) if off else nom), ('F' if m.group(1) == '@' else 'D')


def asm_ins(linea, simbolos, n):
    """Devuelve un item: ('b', [bytes], texto) | ('ref', [pref,op], off, texto, arg) | ('jr', op, etiqueta, texto_fmt) | ('call', op, rt, kind)"""
    m = re.match(r'^(\w+)\s*(.*)$', linea)
    mn = m.group(1).upper()
    ops = [o.strip() for o in m.group(2).split(',')] if m.group(2).strip() else []
    ops = [re.sub(r'\s+', '', o) for o in ops]
    up = [o.upper() if o[:1] not in ('@', '$') else o for o in ops]
    txt = mn + (' ' + ','.join(ops) if ops else '')

    def err(msg):
        raise Err('linea %d: %s: %s' % (n, msg, linea))

    if mn in SIMPLE and not ops:
        return ('b', SIMPLE[mn], txt)
    if mn == 'IN' and up == ['A', '(C)']:
        return ('b', [0xED, 0x78], txt)
    if mn == 'EX':
        if up == ['DE', 'HL']:
            return ('b', [0xEB], txt)
        if up == ['AF', "AF'"]:
            return ('b', [0x08], txt)
        err('EX')
    if mn in ('PUSH', 'POP'):
        r = {'BC': 0, 'DE': 1, 'HL': 2, 'AF': 3}[up[0]]
        return ('b', [(0xC5 if mn == 'PUSH' else 0xC1) | (r << 4)], txt)
    if mn == 'LD':
        d, s = up
        # LD r,r' / LD r,n / LD r,(HL) / LD (HL),r
        if d in R8 and s in R8 and not (d == '(HL)' and s == '(HL)'):
            return ('b', [0x40 | (R8[d] << 3) | R8[s]], txt)
        ma = re.match(r'^\(=(0[xX][0-9A-Fa-f]+|\d+)\)$', ops[1]) if len(ops) > 1 else None
        mb = re.match(r'^\(=(0[xX][0-9A-Fa-f]+|\d+)\)$', ops[0])
        if ma and d in ('A', 'HL', 'DE', 'BC'):                     # LD r,(nn) con nn absoluto
            v = int(ma.group(1), 0) & 0xFFFF
            pre = {'A': [0x3A], 'HL': [0x2A], 'DE': [0xED, 0x5B], 'BC': [0xED, 0x4B]}[d]
            return ('b', pre + [v & 255, v >> 8], txt)
        if mb and s in ('A', 'HL', 'DE', 'BC'):                     # LD (nn),r con nn absoluto
            v = int(mb.group(1), 0) & 0xFFFF
            pre = {'A': [0x32], 'HL': [0x22], 'DE': [0xED, 0x53], 'BC': [0xED, 0x43]}[s]
            return ('b', pre + [v & 255, v >> 8], txt)
        if d in R8 and not s.startswith('(') and not s[:1] in ('@', '$') and s not in RR:
            return ('b', [0x06 | (R8[d] << 3), num(ops[1]) & 0xFF], txt)
        # acumulador con (BC)/(DE)
        if d == 'A' and s in ('(BC)', '(DE)'):
            return ('b', [0x0A if s == '(BC)' else 0x1A], txt)
        if s == 'A' and d in ('(BC)', '(DE)'):
            return ('b', [0x02 if d == '(BC)' else 0x12], txt)
        # LD A,(nn) / LD (nn),A / LD HL,(nn) / ...
        mm = re.match(r'^\(([@$].*)\)$', ops[1]) if len(ops) > 1 else None
        if mm and d in ('A', 'HL', 'DE', 'BC'):
            off, a, bs = sym_off(mm.group(1), simbolos)
            if d == 'A': return ('ref', [], 0x3A, off, 'LD A,(%s)', a, bs)
            if d == 'HL': return ('ref', [], 0x2A, off, 'LD HL,(%s)', a, bs)
            if d == 'DE': return ('ref', [0xED], 0x5B, off, 'LD DE,(%s)', a, bs)
            if d == 'BC': return ('ref', [0xED], 0x4B, off, 'LD BC,(%s)', a, bs)
        mm = re.match(r'^\(([@$].*)\)$', ops[0])
        if mm and s in ('A', 'HL', 'DE', 'BC'):
            off, a, bs = sym_off(mm.group(1), simbolos)
            if s == 'A': return ('ref', [], 0x32, off, 'LD (%s),A', a, bs)
            if s == 'HL': return ('ref', [], 0x22, off, 'LD (%s),HL', a, bs)
            if s == 'DE': return ('ref', [0xED], 0x53, off, 'LD (%s),DE', a, bs)
            if s == 'BC': return ('ref', [0xED], 0x43, off, 'LD (%s),BC', a, bs)
        if d in RR and d != 'SP':
            if s[:1] in ('@', '$'):
                off, a, bs = sym_off(s, simbolos)
                return ('ref', [], 0x01 | (RR[d] << 4), off, 'LD %s,%%s' % d, a, bs)
            if re.match(r'^[A-Za-z_]\w*$', ops[1]):
                return ('refr', 0x01 | (RR[d] << 4), ops[1], 'LD %s,%%s' % d)
            v = num(ops[1]) & 0xFFFF
            return ('b', [0x01 | (RR[d] << 4), v & 255, v >> 8], txt)
        if d == '(HL)' and s not in R8:
            return ('b', [0x36, num(ops[1]) & 0xFF], txt)
        err('LD')
    if mn in ALU or (mn in ('SUB', 'AND', 'XOR', 'OR', 'CP')):
        if mn in ('ADD', 'ADC', 'SBC') and len(ops) == 2 and up[0] == 'HL':
            r = RR[up[1]]
            if mn == 'ADD':
                return ('b', [0x09 | (r << 4)], txt)
            return ('b', [0xED, (0x4A if mn == 'ADC' else 0x42) | (r << 4)], txt)
        if len(ops) == 2 and up[0] == 'A':
            ops, up = ops[1:], up[1:]
        if len(ops) != 1:
            err('ALU')
        a = ALU[mn]
        if up[0] in R8:
            return ('b', [0x80 | (a << 3) | R8[up[0]]], txt)
        return ('b', [0xC6 | (a << 3), num(ops[0]) & 0xFF], txt)
    if mn in ('INC', 'DEC'):
        if up[0] in R8:
            return ('b', [(0x04 if mn == 'INC' else 0x05) | (R8[up[0]] << 3)], txt)
        r = RR[up[0]]
        return ('b', [(0x03 if mn == 'INC' else 0x0B) | (r << 4)], txt)
    if mn in ROT:
        return ('b', [0xCB, (ROT[mn] << 3) | R8[up[0]]], txt)
    if mn in ('BIT', 'SET', 'RES'):
        base = {'BIT': 0x40, 'RES': 0x80, 'SET': 0xC0}[mn]
        return ('b', [0xCB, base | (num(ops[0]) << 3) | R8[up[1]]], txt)
    if mn in ('JR', 'DJNZ'):
        if mn == 'DJNZ':
            return ('jr', 0x10, ops[0], 'DJNZ')
        if len(ops) == 2:
            c = CC[up[0]]
            if c > 3: err('JR cc')
            return ('jr', 0x20 | (c << 3), ops[1], 'JR ' + up[0])
        return ('jr', 0x18, ops[0], 'JR')
    if mn == 'JP' and up == ['(HL)']:
        return ('b', [0xE9], txt)
    if mn == 'JP' and not ops[-1].upper().startswith('RT_'):      # salto a una etiqueta de la propia rutina (direccion absoluta)
        if len(ops) == 2:
            c = CC[up[0]]
            return ('refr', 0xC2 | (c << 3), ops[1], 'JP %s,%%s' % up[0])
        return ('refr', 0xC3, ops[0], 'JP %s')
    if mn in ('CALL', 'JP'):
        if len(ops) == 2:
            c = CC[up[0]]
            op = (0xC4 if mn == 'CALL' else 0xC2) | (c << 3)
            return ('call', op, ops[1], mn + ' ' + up[0], True)
        return ('call', 0xCD if mn == 'CALL' else 0xC3, ops[0], mn, False)
    if mn == 'RET' and len(ops) == 1:
        return ('b', [0xC0 | (CC[up[0]] << 3)], txt)
    err('instruccion desconocida')


def ensamblar(rut, simbolos):
    """Primer paso: tamano de cada item; segundo: desplazamientos."""
    ins = []
    for it in rut.items:
        if it[0] == 'ins':
            ins.append(asm_ins(it[1], simbolos, it[2]))
        elif it[0] == 'lbl':
            ins.append(('lbl', it[1]))
        elif it[0] == 'db':
            ins.append(('db', it[1]))
        else:
            ins.append(('dat', it[1]))
    pos = 0
    etq = {}
    tam = []
    for x in ins:
        if x[0] == 'lbl':
            etq[x[1]] = pos
            tam.append(0)
        elif x[0] == 'dat':
            tam.append(x[1])
        elif x[0] == 'db':
            tam.append(len(x[1]))
        elif x[0] == 'refr':
            tam.append(3)
        elif x[0] == 'b':
            tam.append(len(x[1]))
        elif x[0] == 'ref':
            tam.append(3 + len(x[1]))
        elif x[0] == 'jr':
            tam.append(2)
        else:
            tam.append(3)
        pos += tam[-1]
    # items finales
    salida = []
    pos = 0
    for x, t in zip(ins, tam):
        if x[0] == 'jr':
            if x[2] not in etq:
                raise Err('%s: etiqueta %s' % (rut.nombre, x[2]))
            d = etq[x[2]] - (pos + 2)
            if d < -128 or d > 127:
                raise Err('%s: salto %s fuera de rango (%d)' % (rut.nombre, x[2], d))
            salida.append(('b', [x[1], d & 255], '%s,$%+d' % (x[3], d + 2) if x[3] != 'JR' and x[3] != 'DJNZ' else '%s $%+d' % (x[3], d + 2)))
        elif x[0] == 'refr':
            if x[2] not in etq or etq[x[2]] > 255:
                raise Err('%s: etiqueta %s' % (rut.nombre, x[2]))
            salida.append(('refr', x[1], etq[x[2]], x[3]))
        else:
            salida.append(x)
        pos += t
    return salida, pos


def c_hex(b):
    return '0x%02X' % b


def emitir(rutinas, simbolos):
    out = []
    tam_total = 0
    for r in rutinas:
        salida, tam = ensamblar(r, simbolos)
        tam_total += tam
        # bytes de la tabla plana (build sin listado): sccz80 no admite sizeof en inicializadores
        nb = 1  # IFIN
        for x in salida:
            if x[0] == 'lbl':
                continue
            if x[0] == 'dat':
                nb += 2
            elif x[0] == 'db':
                nb += len(x[1]) + (len(x[1]) + 2) // 3
            elif x[0] == 'refr':
                nb += 3
            elif x[0] == 'b':
                nb += 1 + len(x[1])
            elif x[0] == 'ref':
                nb += 3 + (1 if x[1] else 0)
            else:
                nb += 3 if x[1] not in (0xCD, 0xC3) else 2
        out.append('/* %s: %d B */' % (r.nombre, tam))
        out.append('#define rt_n_%s %d' % (r.nombre.lower(), nb))
        out.append('static const RtI rt_c_%s[] = {' % r.nombre.lower())
        linea = '       '
        for x in salida:
            if x[0] == 'lbl':
                continue
            if x[0] == 'dat':
                s = ' IDATA(%d)' % x[1]
            elif x[0] == 'db':
                s = ''
                bs = x[1]
                for i in range(0, len(bs), 3):
                    g = bs[i:i + 3]
                    s += ' IDB%d(%s)' % (len(g), ', '.join(c_hex(c) for c in g))
            elif x[0] == 'refr':
                s = ' IREFR(T("%s"), "rt_%s+%d", %s, %d)' % (x[3], r.nombre.lower(), x[2], c_hex(x[1]), x[2])
            elif x[0] == 'b':
                b, t = x[1], x[2]
                mabs = re.match(r'^(.*)\(=(0[xX][0-9A-Fa-f]+|\d+)\)(.*)$', t)
                if mabs and len(b) == 3:
                    s = ' I3(T("%s(%%s)%s"), "0x%04X", %s, %s, %s)' % (mabs.group(1), mabs.group(3), b[1] | (b[2] << 8), c_hex(b[0]), c_hex(b[1]), c_hex(b[2]))
                elif len(b) == 1:
                    s = ' I1(T("%s"), %s)' % (t, c_hex(b[0]))
                elif len(b) == 2:
                    s = ' I2(T("%s"), %s, %s)' % (t, c_hex(b[0]), c_hex(b[1]))
                else:
                    s = ' I3(T("%s,%%s"), "%d", %s, %s, %s)' % (t.rsplit(',', 1)[0], b[1] | (b[2] << 8), c_hex(b[0]), c_hex(b[1]), c_hex(b[2]))
            elif x[0] == 'ref':
                _, pref, op, off, fmt, a, bs = x
                if pref:
                    s = ' IREF%s(T("%s"), "%s", %s, %s, %d)' % ('E' if bs == 'F' else 'DE', fmt, a, c_hex(pref[0]), c_hex(op), off)
                else:
                    s = ' IREF%s(T("%s"), "%s", %s, %d)' % ('O' if bs == 'F' else 'D', fmt, a, c_hex(op), off)
            else:  # call
                _, op, rt, nom, cond = x
                rtn = rt.upper()
                if not rtn.startswith('RT_'):
                    raise Err('%s: destino %s' % (r.nombre, rt))
                if op in (0xCD,):
                    s = ' ICALLRT(%s)' % rtn
                elif op == 0xC3:
                    s = ' IJPRT(%s)' % rtn
                else:
                    s = ' IRTOP(T("%s,%%s"), %s, %s)' % (nom, c_hex(op), rtn)
            if len(linea) + len(s) > 118:
                out.append(linea)
                linea = '       '
            linea += s
        out.append(linea)
        out.append('        IFIN')
        out.append('};')
        out.append('')
    return out, tam_total


def main():
    nombre_h = sys.argv[3] if len(sys.argv) > 3 else 'back_rt_real.h'
    fuente = open(sys.argv[1], encoding='utf-8').read()
    rutinas, simbolos = parse(fuente)
    out, tam = emitir(rutinas, simbolos)
    cab = ['/* %s - GENERADO por tools/rtgen/rtasm.py desde tools/rtgen/%s. NO EDITAR A MANO.' % (nombre_h, sys.argv[1].replace('\\', '/').split('/')[-1]),
           ' * Rutinas de apoyo del programa generado: %d B de codigo y datos en total. */' % tam, '']
    open(sys.argv[2], 'w', encoding='utf-8', newline='\n').write('\n'.join(cab + out) + '\n')
    print('rutinas: %d, %d bytes' % (len(rutinas), tam))
    for r in rutinas:
        print('  %-10s %4d' % (r.nombre, ensamblar(r, simbolos)[1]))


if __name__ == '__main__':
    try:
        main()
    except Err as e:
        sys.exit('ERROR: %s' % e)
