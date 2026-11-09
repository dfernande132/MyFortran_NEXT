#!/usr/bin/env python3
"""Fuzzer diferencial de funciones de sentencia (decision 0014, S2c).

Genera programas con funciones de sentencia INTEGER y LOGICAL (cuerpos con ficticios, variables de la unidad que a veces se
llaman igual que un ficticio, constantes, intrinsecas y llamadas a otras funciones de sentencia con argumentos arbitrarios) y
compara la salida del compilador (probador `runf`: TAC y Z80 iguales) con la que calcula este script con aritmetica de 16 bits.

uso: fuzz_sf.py RUNF [N] [SEMILLA]      (RUNF = ejecutable que compila un .f y lo ejecuta, p. ej. scratch run_f)
"""
import random, subprocess, sys, os, tempfile

def partir(linea):
    """Parte una linea de mas de 72 columnas con lineas de continuacion (columna 6)."""
    out = []
    while len(linea) > 72:
        i = linea.rfind(' ', 7, 71)
        out.append(linea[:i])
        linea = '     +' + linea[i + 1:]
    out.append(linea)
    return '\n'.join(out)

def wrap(x):
    return ((x + 32768) & 0xFFFF) - 32768

def tdiv(a, b):
    q = abs(a) // abs(b)
    return -q if (a < 0) != (b < 0) else q

def tmod(a, b):
    return a - tdiv(a, b) * b

HOST = ['IA', 'IB', 'IC', 'IX', 'IY', 'IZ']     # variables del programa (IX, IY, IZ tambien se usan de ficticios)
DUMMY = ['IX', 'IY', 'IZ']

class Gen:
    def __init__(self, rnd):
        self.r = rnd
        self.fns = []          # (nombre, nargs, ficticios, cuerpo, es_logica)
        self.host = {}

    # expresion entera: ('c',n) ('v',nombre) ('op',op,a,b) ('neg',a) ('fn',idx,[args]) ('abs',a) ('mod',a,k) ('max'|'min',a,b)
    def iexpr(self, d, dummies, nfn):
        r = self.r
        k = r.random()
        if d == 0 or k < 0.25:
            if r.random() < 0.4:
                return ('c', r.choice([0, 1, 2, 3, 5, 7, 10, 100, 1000, 30000]))
            names = list(dummies) + HOST
            return ('v', r.choice(names))
        if k < 0.55:
            return ('op', r.choice('+-*'), self.iexpr(d - 1, dummies, nfn), self.iexpr(d - 1, dummies, nfn))
        if k < 0.62:
            return ('neg', self.iexpr(d - 1, dummies, nfn))
        if k < 0.68:
            return ('abs', self.iexpr(d - 1, dummies, nfn))
        if k < 0.74:
            return ('mod', self.iexpr(d - 1, dummies, nfn), r.choice([2, 3, 5, 7, 9]))
        if k < 0.78:
            return ('div', self.iexpr(d - 1, dummies, nfn), r.choice([2, 3, 4, 5]))
        if k < 0.84:
            return (r.choice(['max', 'min']), self.iexpr(d - 1, dummies, nfn), self.iexpr(d - 1, dummies, nfn))
        cands = [i for i in range(nfn) if not self.fns[i][4]]
        if cands:
            i = r.choice(cands)
            return ('fn', i, [self.iexpr(d - 1, dummies, nfn) for _ in range(self.fns[i][1])])
        return ('c', r.randint(0, 20))

    def lexpr(self, d, dummies, nfn):
        r = self.r
        k = r.random()
        if k < 0.5 or d == 0:
            return ('rel', r.choice(['.LT.', '.LE.', '.GT.', '.GE.', '.EQ.', '.NE.']),
                    self.iexpr(1, dummies, nfn), self.iexpr(1, dummies, nfn))
        if k < 0.7:
            return ('not', self.lexpr(d - 1, dummies, nfn))
        if k < 0.9:
            return (r.choice(['and', 'or']), self.lexpr(d - 1, dummies, nfn), self.lexpr(d - 1, dummies, nfn))
        cands = [i for i in range(nfn) if self.fns[i][4]]
        if cands:
            i = r.choice(cands)
            return ('fn', i, [self.iexpr(1, dummies, nfn) for _ in range(self.fns[i][1])])
        return ('rel', '.EQ.', ('c', 1), ('c', 1))

    def text(self, e):
        t = e[0]
        if t == 'c': return str(e[1])
        if t == 'v': return e[1]
        if t == 'op': return '(%s %s %s)' % (self.text(e[2]), e[1], self.text(e[3]))
        if t == 'neg': return '(-%s)' % self.text(e[1])
        if t == 'abs': return 'ABS(%s)' % self.text(e[1])
        if t == 'mod': return 'MOD(%s, %d)' % (self.text(e[1]), e[2])
        if t == 'div': return '(%s / %d)' % (self.text(e[1]), e[2])
        if t == 'max': return 'MAX(%s, %s)' % (self.text(e[1]), self.text(e[2]))
        if t == 'min': return 'MIN(%s, %s)' % (self.text(e[1]), self.text(e[2]))
        if t == 'fn': return '%s(%s)' % (self.fns[e[1]][0], ', '.join(self.text(a) for a in e[2]))
        if t == 'rel': return '(%s %s %s)' % (self.text(e[2]), e[1], self.text(e[3]))
        if t == 'not': return '(.NOT. %s)' % self.text(e[1])
        if t == 'and': return '(%s .AND. %s)' % (self.text(e[1]), self.text(e[2]))
        if t == 'or': return '(%s .OR. %s)' % (self.text(e[1]), self.text(e[2]))
        raise ValueError(t)

    def ev(self, e, env):
        t = e[0]
        if t == 'c': return e[1]
        if t == 'v': return env[e[1]] if e[1] in env else self.host[e[1]]
        if t == 'op':
            a, b = self.ev(e[2], env), self.ev(e[3], env)
            return wrap(a + b if e[1] == '+' else a - b if e[1] == '-' else a * b)
        if t == 'neg': return wrap(-self.ev(e[1], env))
        if t == 'abs': return wrap(abs(self.ev(e[1], env)))
        if t == 'mod': return tmod(self.ev(e[1], env), e[2])
        if t == 'div': return tdiv(self.ev(e[1], env), e[2])
        if t == 'max': return max(self.ev(e[1], env), self.ev(e[2], env))
        if t == 'min': return min(self.ev(e[1], env), self.ev(e[2], env))
        if t == 'fn':
            name, n, dums, body, lg = self.fns[e[1]]
            vals = [self.ev(a, env) for a in e[2]]
            return self.ev(body, dict(zip(dums, vals)))
        if t == 'rel':
            a, b = self.ev(e[2], env), self.ev(e[3], env)
            return {'.LT.': a < b, '.LE.': a <= b, '.GT.': a > b, '.GE.': a >= b, '.EQ.': a == b, '.NE.': a != b}[e[1]]
        if t == 'not': return not self.ev(e[1], env)
        if t == 'and': return self.ev(e[1], env) and self.ev(e[2], env)
        if t == 'or': return self.ev(e[1], env) or self.ev(e[2], env)
        raise ValueError(t)

    def programa(self):
        r = self.r
        self.host = {n: r.choice([0, 1, -1, 2, 7, -9, 100, 32767, -32768, r.randint(-50, 50)]) for n in HOST}
        nf = r.randint(1, 5)
        decl_l = []
        defs = []
        for i in range(nf):
            lg = r.random() < 0.3
            name = ('LA%d' if lg else 'NA%d') % i
            nd = r.randint(0, 3)
            dums = r.sample(DUMMY, nd)
            self.fns.append((name, nd, dums, None, lg))
            body = self.lexpr(2, dums, i) if lg else self.iexpr(r.randint(1, 3), dums, i)
            self.fns[i] = (name, nd, dums, body, lg)
            if lg:
                decl_l.append(name)
            defs.append('      %s(%s) = %s' % (name, ', '.join(dums), self.text(body)))
        src = ['      INTEGER %s' % ', '.join(HOST)]
        if decl_l:
            src.append('      LOGICAL %s' % ', '.join(decl_l))
        src += defs
        for n in HOST:
            src.append('      %s = %d' % (n, self.host[n]))
        exp = []
        for _ in range(r.randint(2, 6)):
            i = r.randrange(nf)
            name, nd, dums, body, lg = self.fns[i]
            args = [self.iexpr(2, [], i + 1) if False else self.iexpr(2, [], nf) for _ in range(nd)]
            call = ('fn', i, args)
            if lg:
                src.append("      IF (%s) THEN" % self.text(call))
                src.append("         PRINT *, 'T'")
                src.append('      ELSE')
                src.append("         PRINT *, 'F'")
                src.append('      END IF')
                exp.append(' T' if self.ev(call, {}) else ' F')
            else:
                src.append('      PRINT *, %s' % self.text(call))
                exp.append(' %d' % self.ev(call, {}))
        src.append('      END')
        return '\n'.join(partir(l) for l in src) + '\n', exp

def main():
    runf = sys.argv[1]
    n = int(sys.argv[2]) if len(sys.argv) > 2 else 200
    seed = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    ok = skip = bad = largas = 0
    motivos = {}
    d = tempfile.mkdtemp()
    for k in range(n):
        g = Gen(random.Random(seed * 100003 + k))
        src, exp = g.programa()
        long_line = any(len(l) > 72 for l in src.split('\n'))
        if long_line:
            largas += 1
            continue
        f = os.path.join(d, 'p.f')
        open(f, 'w').write(src)
        p = subprocess.run([runf, f], capture_output=True, text=True)
        out = p.stdout.rstrip('\n').split('\n')
        err = ((p.stderr or '') + ' ' + out[0] if out else (p.stderr or '')).strip()
        if 'too large' in err or 'too complex' in err or 'expression' in err:
            skip += 1
            m = err.split(' ', 1)[1] if ' ' in err else err
            motivos[m] = motivos.get(m, 0) + 1
            continue
        if p.returncode != 0 or out[-1].startswith('---') is False or 'IGUALES' not in out[-1] or out[:-1] != exp:
            bad += 1
            print('FALLO semilla', seed, 'n', k)
            print(src)
            print('esperado:', exp)
            print('obtenido:', out, err)
            if bad >= 3:
                break
        else:
            ok += 1
    print('fuzz_sf: %d correctos, %d omitidos por lineas de mas de 72 columnas, %d omitidos por tamano %s, %d fallos' % (ok, largas, skip, motivos, bad))
    sys.exit(1 if bad else 0)

main()
