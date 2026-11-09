#!/usr/bin/env python3
"""Fuzzer diferencial de argumentos CHARACTER (decision 0014, S3a).

Genera programas con SUBROUTINEs cuyos formales son CHARACTER*m (y algun INTEGER) que asignan literales, otros formales y
subcadenas (con truncado y relleno de blancos), y un programa principal que las llama con variables, elementos de array,
literales y concatenaciones de longitud >= m. Compara la salida del compilador (probador `runf`: TAC y Z80 iguales) con el
modelo de este script (copia de entrada y de salida con la longitud del formal).

uso: fuzz_char.py RUNF [N] [SEMILLA]
"""
import random, subprocess, sys, os, tempfile

def ajusta(s, n):
    return (s + ' ' * n)[:n]

class Gen:
    def __init__(self, r):
        self.r = r

    def literal(self, n=None):
        n = n or self.r.randint(1, 6)
        return ''.join(self.r.choice('abcxyz019#+-') for _ in range(n))

    def sub(self, nombre):
        r = self.r
        forms = []
        for k in range(r.randint(1, 3)):
            forms.append(('C', r.randint(1, 6)) if r.random() < 0.8 else ('I', 0))
        if not any(f[0] == 'C' for f in forms):
            forms[0] = ('C', r.randint(1, 6))
        nm = [chr(ord('P') + k) for k in range(len(forms))]
        src = ['      SUBROUTINE %s(%s)' % (nombre, ', '.join(nm))]
        for (t, m), n in zip(forms, nm):
            src.append('      CHARACTER*%d %s' % (m, n) if t == 'C' else '      INTEGER %s' % n)
        ops = []
        cs = [(n, m) for (t, m), n in zip(forms, nm) if t == 'C']
        for _ in range(r.randint(1, 4)):
            n, m = r.choice(cs)
            c = r.random()
            if c < 0.3:
                i = r.randint(1, m); j = r.randint(i, m)
                lit = self.literal()
                src.append("      %s(%d:%d) = '%s'" % (n, i, j, lit))
                ops.append(('sub', n, i, j, lit))
            elif c < 0.5:
                lit = self.literal()
                src.append("      %s = '%s'" % (n, lit))
                ops.append(('all', n, lit))
            elif c < 0.75 and len(cs) > 1:
                n2, m2 = r.choice([x for x in cs if x[0] != n])
                src.append('      %s = %s' % (n, n2))
                ops.append(('mov', n, n2))
            elif c < 0.9:
                n2, m2 = r.choice(cs)
                k = r.randint(1, m2); l = r.randint(k, m2)
                i = r.randint(1, m); j = r.randint(i, m)
                src.append('      %s(%d:%d) = %s(%d:%d)' % (n, i, j, n2, k, l))
                ops.append(('subsub', n, i, j, n2, k, l))
            else:
                n2, m2 = r.choice(cs)
                src.append("      %s = %s // '%s'" % (n, n2, self.literal(2)))
                ops.append(('cat', n, n2, src[-1].split("'")[1]))
        for (t, m), n in zip(forms, nm):
            if t == 'I':
                src.append('      %s = %s + 1' % (n, n))
        src.append('      END')
        return forms, nm, ops, src

    def ejecuta(self, forms, nm, ops, vals):
        st = dict(zip(nm, vals))
        lens = dict((n, m) for (t, m), n in zip(forms, nm) if t == 'C')
        for op in ops:
            if op[0] == 'sub':
                _, n, i, j, lit = op
                s = st[n]
                st[n] = s[:i - 1] + ajusta(lit, j - i + 1) + s[j:]
            elif op[0] == 'all':
                st[op[1]] = ajusta(op[2], lens[op[1]])
            elif op[0] == 'mov':
                st[op[1]] = ajusta(st[op[2]], lens[op[1]])
            elif op[0] == 'subsub':
                _, n, i, j, n2, k, l = op
                s = st[n]
                st[n] = s[:i - 1] + ajusta(st[n2][k - 1:l], j - i + 1) + s[j:]
            elif op[0] == 'cat':
                st[op[1]] = ajusta(st[op[2]] + op[3], lens[op[1]])
        for (t, m), n in zip(forms, nm):
            if t == 'I':
                st[n] += 1
        return st

    def programa(self):
        r = self.r
        subs = [self.sub('S%d' % k) for k in range(r.randint(1, 3))]
        vars_ = {}       # nombre -> longitud
        decl = []
        for k in range(r.randint(2, 5)):
            vars_['V%d' % k] = r.randint(3, 8)
        arr = ('T', r.randint(2, 4), r.randint(3, 8))
        main = []
        main.append('      CHARACTER*%d T(%d)' % (arr[2], arr[1]))
        for n, m in vars_.items():
            main.append('      CHARACTER*%d %s' % (m, n))
        main.append('      INTEGER IK')
        val = {}
        for n, m in vars_.items():
            val[n] = ajusta(self.literal(r.randint(1, m)), m)
            main.append("      %s = '%s'" % (n, val[n].rstrip()))
        tv = []
        for k in range(arr[1]):
            tv.append(ajusta(self.literal(r.randint(1, arr[2])), arr[2]))
            main.append("      T(%d) = '%s'" % (k + 1, tv[k].rstrip()))
        main.append('      IK = 2')
        exp = []
        for _ in range(r.randint(2, 6)):
            si = r.randrange(len(subs))
            forms, nm, ops, _s = subs[si]
            args, vals, outs = [], [], []
            usados = set()
            for (t, m) in forms:
                if t == 'I':
                    v = r.randint(-5, 5)
                    args.append(str(v)); vals.append(v); outs.append(None)
                    continue
                cands = [n for n, mm in vars_.items() if mm >= m and n not in usados]
                eltos = [k for k in range(arr[1]) if arr[2] >= m and ('T', k) not in usados]
                c = r.random()
                if c < 0.45 and cands:
                    n = r.choice(cands); usados.add(n)
                    args.append(n); vals.append(val[n]); outs.append(('v', n))
                elif c < 0.75 and eltos:
                    k = r.choice(eltos); usados.add(('T', k))
                    if k == 1 and r.random() < 0.5:
                        args.append('T(IK)')
                    else:
                        args.append('T(%d)' % (k + 1))
                    vals.append(tv[k]); outs.append(('t', k))
                elif c < 0.9:
                    lit = self.literal(r.randint(m, m + 3))
                    args.append("'%s'" % lit); vals.append(ajusta(lit, m)); outs.append(None)
                else:
                    a = self.literal(m); b = self.literal(r.randint(0, 3) or 1)
                    args.append("'%s' // '%s'" % (a, b)); vals.append(ajusta(a + b, m)); outs.append(None)
            main.append('      CALL S%d(%s)' % (si, ', '.join(args)))
            vals = [v if isinstance(v, int) else v[:m] for v, (t, m) in zip(vals, forms)]
            st = self.ejecuta(forms, nm, ops, vals)
            for o, n, (t, m) in zip(outs, nm, forms):
                if o is None or t == 'I':
                    continue
                if o[0] == 'v':
                    val[o[1]] = st[n] + val[o[1]][m:]
                else:
                    tv[o[1]] = st[n] + tv[o[1]][m:]
            main.append('      PRINT *, ' + ', '.join(list(vars_) + ['T(%d)' % (k + 1) for k in range(arr[1])]))
            exp.append(' ' + ' '.join([val[n] for n in vars_] + tv))
        main.append('      END')
        src = main
        for s in subs:
            src += s[3]
        return '\n'.join(src) + '\n', exp

def main():
    runf = sys.argv[1]
    n = int(sys.argv[2]) if len(sys.argv) > 2 else 200
    seed = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    ok = bad = 0
    d = tempfile.mkdtemp()
    for k in range(n):
        g = Gen(random.Random(seed * 100003 + k))
        src, exp = g.programa()
        f = os.path.join(d, 'p.f')
        open(f, 'w').write(src)
        p = subprocess.run([runf, f], capture_output=True, text=True)
        out = p.stdout.rstrip('\n').split('\n')
        if p.returncode != 0 or 'IGUALES' not in out[-1] or out[:-1] != exp:
            bad += 1
            print('FALLO semilla', seed, 'n', k)
            print(src)
            print('esperado:', exp)
            print('obtenido:', out, p.stderr)
            if bad >= 3:
                break
        else:
            ok += 1
    print('fuzz_char: %d correctos, %d fallos' % (ok, bad))
    sys.exit(1 if bad else 0)

main()
