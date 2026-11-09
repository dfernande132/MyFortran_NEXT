#!/usr/bin/env python3
"""analizar_map.py - Reparte los bytes de un .map de z88dk entre NUESTROS
modulos y el sistema (runtime, libreria C).

Uso:   python tools/analizar_map.py build/next/core.map

Formato de las lineas del .map (docs/referencia, seccion 2.4):
    _nombre = $961F ; addr, public, , <modulo>, <seccion>, <fichero>::...

Reglas aprendidas (cada una costo un error real):
  - Tamano de un simbolo = distancia al SIGUIENTE simbolo de direccion mayor.
  - Hay que descartar ANTES las etiquetas internas i_NN: si no, cada funcion
    "termina" en su primera etiqueta y todo sale 4 veces mas pequeno.
  - Los simbolos de datos sin etiqueta propia (cadenas literales) se suman al
    ultimo simbolo anterior: el tamano de una tabla de rodata es un MAXIMO.
  - Las secciones de libreria estan intercaladas: el reparto por seccion usa
    la distancia entre la primera direccion de cada una y la de la siguiente.
"""
import os
import re
import sys
from collections import defaultdict

LINEA = re.compile(r'^(\S+)\s*=\s*\$([0-9A-Fa-f]+)\s*;\s*(.*)$')
NUESTRO = re.compile(r'(?:compiler_src|tools_medir)_(.+)_c$')


def leer(mapa):
    simbolos = []
    for linea in open(mapa, encoding='latin-1'):
        m = LINEA.match(linea.rstrip('\n'))
        if not m:
            continue
        campos = [c.strip() for c in m.group(3).split(',')]
        if len(campos) < 5 or campos[0] != 'addr':
            continue
        nombre = m.group(1)
        if nombre.startswith('i_'):          # etiquetas internas: fuera
            continue
        simbolos.append((int(m.group(2), 16), nombre, campos[3], campos[4]))
    simbolos.sort()
    return simbolos


def tamanos(simbolos):
    res = []
    for i, (a, n, mod, sec) in enumerate(simbolos):
        j = i + 1
        while j < len(simbolos) and simbolos[j][0] == a:
            j += 1
        res.append((simbolos[j][0] - a) if j < len(simbolos) else 0)
    return res


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 1
    mapa = sys.argv[1]
    simbolos = leer(mapa)
    tam = tamanos(simbolos)

    # Tamano de la imagen: el fichero binario hermano, sin extension.
    imagen = os.path.splitext(mapa)[0]
    n_imagen = os.path.getsize(imagen) if os.path.isfile(imagen) else None

    por_modulo = defaultdict(lambda: defaultdict(int))
    funciones, datos = [], []
    for (a, n, mod, sec), t in zip(simbolos, tam):
        m = NUESTRO.search(mod)
        if not m:
            continue
        nombre = m.group(1) + '.c'
        por_modulo[nombre][sec] += t
        if sec.startswith('code'):
            funciones.append((t, nombre, n))
        else:
            datos.append((t, nombre, sec, n))

    print("== Nuestros modulos (bytes por seccion) ==")
    total = defaultdict(int)
    for nombre in sorted(por_modulo):
        print(f"  {nombre:16s}", "  ".join(f"{s}={b}" for s, b in sorted(por_modulo[nombre].items())))
        for s, b in por_modulo[nombre].items():
            total[s] += b
    print(f"  {'TOTAL':16s}", "  ".join(f"{s}={b}" for s, b in sorted(total.items())))

    print("\n== Funciones mas grandes ==")
    for t, nombre, n in sorted(funciones, reverse=True)[:20]:
        print(f"  {t:5d}  {nombre:14s} {n}")

    print("\n== Datos de nuestros modulos (los de rodata son un maximo) ==")
    for t, nombre, sec, n in sorted(datos, reverse=True):
        print(f"  {t:5d}  {nombre:14s} {sec:16s} {n}")

    # Reparto por seccion: de la primera direccion de cada una a la de la siguiente.
    primera = {}
    for a, n, mod, sec in simbolos:
        if sec and sec != 'code_l':          # code_l esta dentro de code_l_sccz80
            primera[sec] = min(a, primera.get(sec, a))
    orden = sorted((a, s) for s, a in primera.items())
    fin = 0x8000 + n_imagen if n_imagen else orden[-1][0]
    reparto = {}
    for i, (a, s) in enumerate(orden):
        b = orden[i + 1][0] if i + 1 < len(orden) else fin
        reparto[s] = b - a
    nuestro = sum(t for s, t in reparto.items() if s.endswith('_compiler'))
    sistema = sum(t for s, t in reparto.items() if not s.endswith('_compiler'))

    print("\n== Reparto de la imagen ==")
    for s, t in reparto.items():
        marca = '  <- nuestro' if s.endswith('_compiler') else ''
        print(f"  {s:18s} {t:6d}{marca}")
    print(f"  {'NUESTRO':18s} {nuestro:6d}")
    print(f"  {'SISTEMA':18s} {sistema:6d}  (runtime + libreria C + fuente 4x8 + crt)")
    if n_imagen:
        print(f"\nImagen: {n_imagen} bytes  ->  margen (32767 - imagen) = {32767 - n_imagen} bytes")
    return 0


if __name__ == '__main__':
    sys.exit(main())
