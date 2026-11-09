#!/usr/bin/env python3
"""oraculo.py - Salida esperada de los programas pNN_*.f, calculada AQUI, en Python, sin usar el compilador.

Cada funcion repite la logica del programa Fortran con las reglas del lenguaje (division entera que
trunca hacia cero, MOD con el signo del dividendo, REAL de 32 bits redondeado a mas cercano) y da las
lineas que debe escribir PRINT * (formato por lista de MyFortran: un blanco delante de cada elemento,
REAL con 9 cifras significativas, CHARACTER rellenado con blancos hasta su longitud declarada).
Uso:  python3 oraculo.py            escribe los pNN_*.esperado.txt
"""
import math
import os
import struct
from decimal import Decimal


def f32(x):
    return struct.unpack('f', struct.pack('f', x))[0]


def i16(v):
    assert -32768 <= v <= 32767, "desbordamiento INTEGER: %d" % v
    return v


def fdiv(a, b):
    q = abs(a) // abs(b)
    return q if (a < 0) == (b < 0) else -q


def fmod(a, b):
    return a - fdiv(a, b) * b


def nint(x):
    return int(math.floor(abs(x) + 0.5)) * (1 if x >= 0 else -1)


def real(x):
    x = f32(x)
    if x == 0:
        return '0.00000000'
    s = '-' if x < 0 else ''
    m, e = ('%.8e' % Decimal(abs(x))).split('e')
    d = m.replace('.', '')
    e = int(e)
    if -1 <= e <= 8:
        if e == -1:
            return s + '0.' + d
        return s + d[:e + 1] + '.' + d[e + 1:]
    return '%s%s.%sE%s%02d' % (s, d[0], d[1:], '-' if e < 0 else '+', abs(e))


def item(v):
    if isinstance(v, bool):
        return 'T' if v else 'F'
    if isinstance(v, int):
        return str(i16(v))
    if isinstance(v, float):
        return real(v)
    return v


def pr(*items):
    return ''.join(' ' + item(v) for v in items)


def do_n(a, b, c=1):
    """Vueltas de DO i = a, b, c (enteros)."""
    return max(0, fdiv(b - a + c, c))


def ch(s, n):
    return s.ljust(n)[:n]


def p01():
    o = []
    A, B = 17, 5
    o.append(pr(A + B, A - B, A * B, fdiv(A, B), fmod(A, B)))
    A = -17
    o.append(pr(fdiv(A, B), fmod(A, B), fdiv(-A, B), fmod(-A, B)))
    B = -5
    o.append(pr(fdiv(17, B), fmod(17, B), fdiv(A, B), fmod(A, B)))
    o.append(pr(2 ** 0, 2 ** 1, 3 ** 4, (-2) ** 3, (-2) ** 4))
    o.append(pr(2 + 3 * 4, (2 + 3) * 4, 2 * 3 ** 2, fdiv(fdiv(100, 10), 5)))
    o.append(pr(-(2 ** 2), 10 - 4 - 3, 2 ** (3 ** 2)))
    a, b, i = 1234, 0, 0
    while a != 0:
        b += fmod(a, 10)
        a = fdiv(a, 10)
        i += 1
    o.append(pr('DIGITOS', i, 'SUMA', b))
    o.append(pr(abs(-7), abs(7), abs(-3), max(3, 9, 4), min(3, 9, 4, -1)))
    sign = lambda a, b: abs(a) if b >= 0 else -abs(a)
    o.append(pr(sign(5, -1), sign(-5, 1), sign(4, 3)))
    dim = lambda a, b: max(a - b, 0)
    o.append(pr(dim(7, 3), dim(3, 7), dim(2, 9)))
    o.append(pr(max(-5, -2), min(10, 20), fmod(7, 7), fmod(0, 5)))
    o.append(pr(32767, -32767, 0))
    return o


def p02():
    o = []
    for I in range(-2, 13, 3):
        if I < 0:
            o.append(pr(I, 'NEGATIVO'))
        elif I == 0:
            o.append(pr(I, 'CERO'))
        elif I < 7:
            o.append(pr(I, 'PEQUENO'))
        else:
            o.append(pr(I, 'GRANDE'))
    S = 0
    n = do_n(10, 1, -3)
    for k in range(n):
        S += 10 - 3 * k
    I = 10 + n * -3
    o.append(pr('PASO NEGATIVO', S, I))
    I = 5
    if do_n(5, 1) > 0:
        S = 999
    o.append(pr('CERO VUELTAS', S, I))
    N = sum(i * j for i in range(1, 5) for j in range(1, i + 1))
    o.append(pr('ANIDADOS', N))
    for K in (-1, 0, 1):
        o.append(pr(K, 'MENOR' if K < 0 else 'IGUAL' if K == 0 else 'MAYOR'))
    for K in range(1, 5):
        o.append(pr(K, ('UNO', 'DOS', 'TRES', 'FUERA DE RANGO')[K - 1]))
    N = 0
    for I in range(1, 101):
        N += I
        if N > 50:
            break
    o.append(pr('SALIDA', I, N))
    if N > 0:
        if N > 60:
            o.append(pr('ALTO'))
        else:
            if N % 2 == 0:
                o.append(pr('PAR'))
            if N % 2 != 0:
                o.append(pr('IMPAR'))
    return o


def p03():
    o = []
    N = 200
    es = [True] * (N + 1)
    es[0] = False
    es[1] = False
    for i in range(2, N + 1):
        if i * i > N:
            break
        if es[i]:
            for j in range(i * i, N + 1, i):
                es[j] = False
    cuenta = suma = ult = 0
    for i in range(1, N + 1):
        if es[i]:
            cuenta += 1
            suma += i
            ult = i
            if cuenta <= 10:
                o.append(pr(cuenta, i))
    o.append(pr('PRIMOS HASTA', N, cuenta))
    o.append(pr('SUMA', suma, 'ULTIMO', ult))
    return o


def p04():
    o = []
    x = 7
    v = []
    for _ in range(20):
        x = (x * 25 + 13) % 251
        v.append(x)
    o.append(pr('ANTES'))
    for i in range(0, 20, 5):
        o.append(pr(*v[i:i + 5]))
    v.sort()
    o.append(pr('DESPUES'))
    for i in range(0, 20, 5):
        o.append(pr(*v[i:i + 5]))
    suma = 0
    for i in range(1, 20):
        suma = (suma + i * v[i - 1]) % 1000
    o.append(pr('ORDENADO', all(v[i] <= v[i + 1] for i in range(19)), 'CONTROL', suma))
    return o


def p05():
    o = []
    A = {(i, j): i + j for i in range(1, 5) for j in range(1, 5)}
    B = {(i, j): i - j for i in range(1, 5) for j in range(1, 5)}
    C = {(i, j): sum(A[i, k] * B[k, j] for k in range(1, 5)) for i in range(1, 5) for j in range(1, 5)}
    for i in range(1, 5):
        o.append(pr(*[C[i, j] for j in range(1, 5)]))
    o.append(pr('TRAZA', sum(C[i, i] for i in range(1, 5))))
    for i in range(1, 5):
        for j in range(1, 5):
            B[j, i] = A[i, j] * 10 + i
    o.append(pr(B[1, 1], B[1, 4], B[4, 1], B[2, 3]))
    o.append(pr(*[i * i for i in range(-2, 3)]))
    return o


def p06():
    o = []
    S = ch('ANITA LAVA LA TINA', 20)
    o.append(pr(S, 20))
    N = len(S.rstrip(' '))
    o.append(pr('LONGITUD UTIL', N))
    R = ch(S[:N][::-1], 20)
    o.append(pr(R))
    t = S[:N].replace(' ', '')
    o.append(pr('PALINDROMO', t == t[::-1]))
    U = ch('hola, Mundo 123', 20).upper()
    o.append(pr(U))
    o.append(pr('VOCALES', sum(1 for c in U if c in 'AEIOU')))
    A, B = ch('AB', 6), ch('CD', 6)
    o.append(pr(A + B, A + '-' + B))
    pos = 0
    for i in range(1, N):
        if pos == 0 and S[i - 1:i + 1] == 'LA':
            pos = i
    o.append(pr('LA EN', pos))
    cmp_ = lambda a, b: (a.ljust(max(len(a), len(b))), b.ljust(max(len(a), len(b))))
    lt = lambda a, b: cmp_(a, b)[0] < cmp_(a, b)[1]
    eq = lambda a, b: cmp_(a, b)[0] == cmp_(a, b)[1]
    o.append(pr(lt('ABC', 'ABD'), eq('ABC', 'ABC '), lt('AZ', 'B')))
    o.append(pr(S[6:10], S[0:1], S[N - 4:N]))
    return o


def p07():
    o = []
    mcd = math.gcd
    o.append(pr('MCD', mcd(48, 18), mcd(17, 5), mcd(100, 75), mcd(7, 7)))

    def fib(n):
        a, b = 0, 1
        for _ in range(n):
            a, b = b, a + b
        return a
    for i in range(0, 8):
        o.append(pr(i, math.factorial(i), fib(i + 10), i * i))
    v = [i * i - 7 for i in range(1, 6)]
    o.append(pr('SUMA', sum(v), 'MAYOR', max(v)))
    v = [x * 3 for x in v]
    o.append(pr(*v))
    I, N = 4, 9
    I, N = N, I
    o.append(pr(I, N))
    tmp, N2 = I + 0, N
    tmp, N2 = N2, tmp
    N = N2
    o.append(pr(I, N))
    return o


def p08():
    o = []
    X, Y = f32(1.5), f32(0.25)
    o.append(pr(X + Y, X - Y, X * Y, X / Y))
    q = lambda a, b: f32(f32(a) / f32(b))
    o.append(pr(q(1.0, 3.0), q(2.0, 3.0), q(10.0, 4.0), q(100.0, 7.0)))
    S = f32(0.0)
    for _ in range(10):
        S = f32(S + f32(0.1))
    o.append(pr('SUMA 0.1', S))
    S = f32(0.0)
    for _ in range(10):
        S = f32(S + f32(0.5))
    o.append(pr('SUMA 0.5', S))
    I = 11
    o.append(pr(f32(I + 0.5), f32(7.0 / 2.0), f32(f32(float(I)) * f32(1.5))))
    o.append(pr(int(3.99), int(-3.99), nint(2.5), nint(-2.5), nint(2.4)))
    o.append(pr(int(0.5), nint(0.5), nint(-0.4), int(100.7)))
    o.append(pr(float(math.trunc(2.7)), float(math.trunc(-2.7)), float(nint(2.5)), float(nint(-2.5))))
    o.append(pr(math.sqrt(16.0), math.sqrt(2.25), math.sqrt(0.25), math.sqrt(100.0)))
    o.append(pr(abs(-2.5), max(1.5, 2.5), min(1.5, 2.5), math.fmod(7.5, 2.0)))
    o.append(pr(-3.0, 5.5 - 2.0, 0.0))
    o.append(pr(f32(X * X), 2.0 ** 3, 0.5 ** 4, 1.0))
    Z = 3.0
    o.append(pr(-Z, -Z * Z, (-Z) ** 2, -Z + 5.0))
    for n in range(1, 6):
        o.append(pr(n, 0.25 * (n - 1)))
    for x in (2.0, 1.5, 1.0):
        o.append(pr(x))
    X = 0.5
    o.append(pr(X > f32(0.9), X < f32(0.9), f32(f32(0.1) + f32(0.2)) == f32(0.3), False))
    o.append(pr(123.456, 0.0, 99999.5, 1234.5678))
    return o


def p09():
    o = []
    for i in (0, 1):
        for j in (0, 1):
            P, Q = i == 1, j == 1
            o.append(pr(P, Q, P and Q, P or Q, not P))
            o.append(pr(P == Q, P != Q, (not P) and Q))
    P, Q, R = True, False, True
    o.append(pr(P or (Q and R), (P or Q) and not R))
    o.append(pr((not Q) or Q, not (Q or P)))
    o.append(pr(P == (Q or R), P != (Q and R)))
    N = 0
    for i in range(1, 4):
        for j in range(1, 4):
            for k in range(1, 4):
                if i < j and j < k:
                    N += 100
                if i == j or j == k or i == k:
                    N += 1
    o.append(pr('N', N))
    o.append(pr(3 < 4, 3 <= 3, 3 > 4))
    o.append(pr(3 >= 4, 3 == 3, 3 != 3))
    o.append(pr(-3 < 2, 2 * 3 == 6))
    o.append(pr(fdiv(7, 2) == 3, fmod(7, 2) == 1))
    return o


def p10():
    o = []
    N, M = 5, 11
    PI, DOS = f32(3.140625), f32(2.0)
    V = [1, 2, 3, 4, 5]
    Z = [7, 99, 7]
    W = [0] * 11
    T = [0.5, 1.5, 2.5, 3.5]
    NOM = [ch('UNO', 4), ch('DOS', 4), ch('TRES', 4)]
    o.append(pr(N, M, PI, DOS, f32(PI * DOS)))
    o.append(pr('SUMA V', sum(V), Z[0], Z[1], Z[2]))
    for i in range(1, M + 1):
        W[i - 1] += i * i
    o.append(pr(W[0], W[4], W[M - 1], M))
    o.append(pr(T[0] + T[3], T[1] * T[2]))
    for i in range(1, 4):
        o.append(pr(i, NOM[i - 1]))
    o.append(pr(True, False))
    o.append(pr(Z[0], Z[1], 4))
    return o


def p11():
    o = []
    N = 4
    o.append(pr('CUANTOS ENTEROS?'))
    o.append('4')
    vals = [10, -3, 7, 22]
    for i, v in enumerate(vals, 1):
        o.append(pr('VALOR', i))
        o.append(str(v))
    o.append(pr('N', N, 'SUMA', sum(vals), 'MAX', max(vals), 'MIN', min(vals)))
    X, Y = f32(2.5), f32(-1.25)
    o.append(pr('DOS REALES:'))
    o.append('2.5 -1.25')
    o.append(pr(X, Y, f32(X * Y), f32(X + Y)))
    B, C = True, False
    o.append(pr('DOS LOGICOS:'))
    o.append('T F')
    o.append(pr(B, C, B and C, B or C))
    o.append(pr('DOS CADENAS:'))
    o.append("'Fortran' 'abc'")
    o.append(pr(ch('Fortran', 8), ch('abc', 3)))
    o.append(pr('ENTERO REAL CADENA:'))
    o.append("42 0.5 'x y z'")
    o.append(pr(42, 0.5, ch('x y z', 8)))
    o.append(pr('ULTIMO ENTERO:'))
    o.append('-32000')
    o.append(pr(-32000, 32000))
    return o


def p12():
    o = []
    mejor = quien = 0
    for i in range(1, 31):
        n, pasos = i, 0
        while n != 1:
            n = n // 2 if n % 2 == 0 else 3 * n + 1
            pasos += 1
        if pasos > mejor:
            mejor, quien = pasos, i
    o.append(pr('COLLATZ', quien, mejor))
    for i in range(2, 501):
        s = 1 + sum(j for j in range(2, i // 2 + 1) if i % j == 0)
        if s == i:
            o.append(pr('PERFECTO', i))
    for i in range(1, 5):
        o.append(pr(i * 7, int(bin(i * 7)[2:])))
    n = 12321
    o.append(pr(n, int(str(n)[::-1]), True))
    for i in range(1896, 2101, 52):
        o.append(pr(i, i % 4 == 0 and (i % 100 != 0 or i % 400 == 0)))
    return o


def p13():
    return [pr(5), pr(4), pr(7, 2.5), pr(1), pr(2), pr(3), pr(3), pr('ETIQUETA'), pr(10),
            pr(sum(range(1, 26))), pr("UNA CADENA CON ' COMILLA Y CON  DOS ESPACIOS"),
            pr('C', 'DO 10 I = 1, 3', 'IF (X) 1, 2, 3'), pr('K', 10), pr(3), '', pr('FIN')]


def p14():
    return [pr(3), pr(-3), pr(3), pr(3), pr(4)]


if __name__ == '__main__':
    aqui = os.path.dirname(os.path.abspath(__file__))
    for nombre in sorted(n for n in globals() if n.startswith('p') and n[1:].isdigit()):
        f = [x for x in os.listdir(aqui) if x.startswith(nombre) and x.endswith('.f')][0]
        with open(os.path.join(aqui, f[:-2] + '.esperado.txt'), 'w', newline='\n') as w:
            w.write('\n'.join(globals()[nombre]()) + '\n')
        print('escrito', f[:-2] + '.esperado.txt')
