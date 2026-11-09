#!/usr/bin/env python3
"""Fuzzer diferencial de arrays y subcadenas como argumentos (decision 0014, S3b/S3c/S3d).

Genera programas con SUBROUTINEs de varias formas (vector INTEGER con tamano constante / ajustable / supuesto, matriz con
primera dimension ajustable, escalar INTEGER, CHARACTER*n, CHARACTER*(*)) y un principal que las llama con arrays enteros,
elementos de arrays (que se convierten en el trozo que empieza ahi), columnas de una matriz, variables CHARACTER, subcadenas
S(i:j), elementos y subcadenas de elementos de un array CHARACTER. Compara la salida del compilador (probador `runf`: TAC y Z80
iguales) con un modelo escrito aqui: asociacion de almacenamiento (vistas de una lista), copia de entrada y de salida.

uso: fuzz_arr.py RUNF [N] [SEMILLA]
"""
import random, subprocess, sys, os, tempfile

def w16(v):
    v &= 0xFFFF
    return v - 0x10000 if v >= 0x8000 else v

def ajusta(s, n):
    return (s + ' ' * n)[:n]

class Vista:
    """Trozo de una lista que empieza en off (lo que ve un formal array)."""
    def __init__(self, lst, off):
        self.l, self.o = lst, off
    def __getitem__(self, i):
        return self.l[self.o + i]
    def __setitem__(self, i, v):
        self.l[self.o + i] = w16(v)
    def __len__(self):
        return len(self.l) - self.o

class Gen:
    def __init__(self, r):
        self.r = r
        self.n = 0

    def lit(self, k):
        return ''.join(self.r.choice('abcxyz019#+-') for _ in range(k))

    # --- subprogramas ---------------------------------------------------------------------------------------------
    def sub_vec(self, nombre):
        """SUBROUTINE nombre(X, N): X(N) / X(*) / X(c) y operaciones sobre los N primeros."""
        r = self.r
        forma = r.choice(['aj', 'ast', 'cte'])
        c = r.randint(2, 4)                                 # elementos que usa si es 'cte'
        ops = []
        src = ['      SUBROUTINE %s(X, N)' % nombre, '      INTEGER N']
        src.append({'aj': '      INTEGER X(N)', 'ast': '      INTEGER X(*)', 'cte': '      INTEGER X(%d)' % c}[forma])
        for _ in range(r.randint(1, 3)):
            k = r.random()
            i = r.randint(1, 2); j = r.randint(1, 2); a = r.randint(-3, 3)
            if k < 0.4:
                src.append('      X(%d) = X(%d) + (%d)' % (i, j, a)); ops.append(('add', i, j, a))
            elif k < 0.7:
                et = 10 * (len(ops) + 1)
                src.append('      DO %d I = 1, %s' % (et, 'N' if forma != 'cte' else str(c)))
                src.append('        X(I) = X(I) * 2 + (%d)' % a); src.append('%5d CONTINUE' % et); ops.append(('all', a))
            else:
                src.append('      X(%d) = N' % i); ops.append(('n', i))
        src.append('      END')
        def f(v, n):
            m = n if forma != 'cte' else c
            for op in ops:
                if op[0] == 'add':
                    v[op[1] - 1] = v[op[2] - 1] + op[3]
                elif op[0] == 'all':
                    for q in range(m):
                        v[q] = v[q] * 2 + op[1]
                else:
                    v[op[1] - 1] = n
        return ('vec', forma, c, f, src)

    def sub_mat(self, nombre):
        """SUBROUTINE nombre(Y, LD, NC): Y(LD, *) o Y(LD, NC)."""
        r = self.r
        ops = []
        src = ['      SUBROUTINE %s(Y, LD, NC)' % nombre, '      INTEGER LD, NC', '      INTEGER Y(LD, %s)' % r.choice(['*', 'NC'])]
        for _ in range(r.randint(1, 3)):
            i = r.randint(1, 2); j = r.randint(1, 2); a = r.randint(-4, 4)
            src.append('      Y(%d, %d) = Y(%d, %d) + LD * (%d)' % (i, j, j, i, a)); ops.append((i, j, a))
        src.append('      Y(1, NC) = NC')
        src.append('      END')
        def f(m, ld, nc):                                   # m: lista plana por columnas
            for (i, j, a) in ops:
                m[(i - 1) + (j - 1) * ld] = w16(m[(j - 1) + (i - 1) * ld] + ld * a)
            m[(nc - 1) * ld] = nc
        return ('mat', f, src)

    def sub_esc(self, nombre):
        a = self.r.randint(-5, 5)
        src = ['      SUBROUTINE %s(K)' % nombre, '      INTEGER K', '      K = K * 2 + (%d)' % a, '      END']
        return ('esc', lambda k: w16(k * 2 + a), src)

    def sub_cadfija(self, nombre):
        m = self.r.randint(1, 4)
        lit = self.lit(m)
        src = ['      SUBROUTINE %s(W)' % nombre, '      CHARACTER*%d W' % m, "      W(1:1) = '%%'" % (), "      W(%d:%d) = '%s'" % (m, m, lit[-1]), '      END']
        def f(s):
            s = '%' + s[1:]
            return s[:m - 1] + lit[-1]
        return ('cfija', m, f, src)

    def sub_cadasum(self, nombre):
        lit = self.lit(1)
        src = ['      SUBROUTINE %s(Z)' % nombre, '      CHARACTER*(*) Z', "      Z(1:1) = '#'", "      Z(LEN(Z):LEN(Z)) = '%s'" % lit,
               '      IF (LEN(Z) .GT. 2) Z(2:2) = CHAR1(LEN(Z))', '      END']
        return None

    # --- programa -------------------------------------------------------------------------------------------------
    def programa(self):
        r = self.r
        na = r.randint(4, 8)
        nr, nc = r.randint(2, 4), r.randint(2, 4)
        A = [r.randint(-9, 9) for _ in range(na)]
        M = [r.randint(-9, 9) for _ in range(nr * nc)]
        ms = r.randint(6, 10)
        S = ajusta(self.lit(ms), ms)
        nt, mt = r.randint(2, 4), r.randint(3, 6)
        T = [ajusta(self.lit(mt), mt) for _ in range(nt)]
        subs = {}
        src_subs = []
        for k in range(r.randint(2, 5)):
            kind = r.choice(['vec', 'vec', 'mat', 'esc', 'cfija', 'cas'])
            nombre = 'S%d' % k
            if kind == 'vec':
                s = self.sub_vec(nombre)
            elif kind == 'mat':
                s = self.sub_mat(nombre)
            elif kind == 'esc':
                s = self.sub_esc(nombre)
            elif kind == 'cfija':
                s = self.sub_cadfija(nombre)
            else:
                lit = self.lit(1)
                l2 = self.lit(2)
                src = ['      SUBROUTINE %s(Z)' % nombre, '      CHARACTER*(*) Z', "      Z(1:1) = '#'",
                       "      Z(LEN(Z):LEN(Z)) = '%s'" % lit, "      IF (LEN(Z) .GE. 4) Z(2:3) = '%s'" % l2, '      END']
                def f(z, lit=lit, l2=l2):
                    z = '#' + z[1:]
                    z = z[:-1] + lit
                    if len(z) >= 4:
                        z = z[:1] + l2 + z[3:]
                    return z
                s = ('cas', f, src)
            subs[nombre] = s
            src_subs += s[-1]
        main = ['      INTEGER A(%d), M(%d, %d), I, J, IK' % (na, nr, nc), '      CHARACTER*%d S' % ms, '      CHARACTER*%d T(%d)' % (mt, nt)]
        for k, v in enumerate(A):
            main.append('      A(%d) = %d' % (k + 1, v))
        for j in range(nc):
            for i in range(nr):
                main.append('      M(%d, %d) = %d' % (i + 1, j + 1, M[i + j * nr]))
        main.append("      S = '%s'" % S.rstrip())
        for k, v in enumerate(T):
            main.append("      T(%d) = '%s'" % (k + 1, v.rstrip()))
        main.append('      IK = 2')
        for _ in range(r.randint(2, 6)):
            nombre = r.choice(list(subs))
            s = subs[nombre]
            if s[0] == 'vec':
                _, forma, c, f, _src = s
                c3 = r.random()
                if c3 < 0.4:
                    n = na if forma != 'cte' else max(na, c)
                    if forma == 'cte':
                        n = na
                    if forma == 'cte' and na < c:
                        continue
                    main.append('      CALL %s(A, %d)' % (nombre, n)); f(Vista(A, 0), n)
                elif c3 < 0.7:
                    o = r.randint(0, na - 2)
                    if forma == 'cte' and na - o < c:
                        continue
                    n = na - o
                    arg = 'A(IK)' if o == 1 and r.random() < 0.5 else 'A(%d)' % (o + 1)
                    main.append('      CALL %s(%s, %d)' % (nombre, arg, n)); f(Vista(A, o), n)
                else:
                    j = r.randint(1, nc)
                    if forma == 'cte' and nr < c:
                        continue
                    main.append('      CALL %s(M(1, %d), %d)' % (nombre, j, nr)); f(Vista(M, (j - 1) * nr), nr)
            elif s[0] == 'mat':
                if nr < 2 or nc < 2:
                    continue
                main.append('      CALL %s(M, %d, %d)' % (nombre, nr, nc)); s[1](M, nr, nc)
            elif s[0] == 'esc':
                if r.random() < 0.5:
                    i = r.randint(1, na)
                    main.append('      CALL %s(A(%d))' % (nombre, i)); A[i - 1] = s[1](A[i - 1])
                else:
                    i = r.randint(1, nr); j = r.randint(1, nc)
                    main.append('      CALL %s(M(%d, %d))' % (nombre, i, j)); M[(i - 1) + (j - 1) * nr] = s[1](M[(i - 1) + (j - 1) * nr])
            elif s[0] == 'cfija':
                _, m, f, _src = s
                c3 = r.random()
                if c3 < 0.4 and ms >= m:
                    i = r.randint(1, ms - m + 1)
                    main.append('      CALL %s(S(%d:%d))' % (nombre, i, i + m - 1))
                    S = S[:i - 1] + f(S[i - 1:i - 1 + m]) + S[i - 1 + m:]
                elif c3 < 0.7 and mt >= m:
                    k = r.randint(1, nt); i = r.randint(1, mt - m + 1)
                    main.append('      CALL %s(T(%d)(%d:%d))' % (nombre, k, i, i + m - 1))
                    T[k - 1] = T[k - 1][:i - 1] + f(T[k - 1][i - 1:i - 1 + m]) + T[k - 1][i - 1 + m:]
                elif mt >= m:
                    k = r.randint(1, nt)
                    main.append('      CALL %s(T(%d))' % (nombre, k))
                    T[k - 1] = f(T[k - 1][:m]) + T[k - 1][m:]
                else:
                    continue
            else:
                f = s[1]
                c3 = r.random()
                if c3 < 0.3:
                    main.append('      CALL %s(S)' % nombre); S = f(S)
                elif c3 < 0.6:
                    i = r.randint(1, ms); j = r.randint(i, ms)
                    main.append('      CALL %s(S(%d:%d))' % (nombre, i, j))
                    S = S[:i - 1] + f(S[i - 1:j]) + S[j:]
                elif c3 < 0.8:
                    k = r.randint(1, nt)
                    main.append('      CALL %s(T(%d))' % (nombre, k)); T[k - 1] = f(T[k - 1])
                else:
                    k = r.randint(1, nt); i = r.randint(1, mt); j = r.randint(i, mt)
                    main.append('      CALL %s(T(%d)(%d:%d))' % (nombre, k, i, j))
                    T[k - 1] = T[k - 1][:i - 1] + f(T[k - 1][i - 1:j]) + T[k - 1][j:]
        main += ['      DO 90 I = 1, %d' % na, '        PRINT *, A(I)', '   90 CONTINUE',
                 '      DO 91 J = 1, %d' % nc, '        DO 91 I = 1, %d' % nr, '          PRINT *, M(I, J)', '   91 CONTINUE',
                 '      PRINT *, S', '      DO 92 I = 1, %d' % nt, '        PRINT *, T(I)', '   92 CONTINUE', '      END']
        exp = [' %d' % v for v in A] + [' %d' % v for v in M] + [' ' + S] + [' ' + t for t in T]
        return '\n'.join(main + src_subs) + '\n', exp

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
    print('fuzz_arr: %d correctos, %d fallos' % (ok, bad))
    sys.exit(1 if bad else 0)

main()
