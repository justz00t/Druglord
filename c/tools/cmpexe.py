#!/usr/bin/env python3
"""Compare two DOS EXEs: header fields, then the load image from the first difference."""
import struct, sys
def load(p):
    d = open(p, 'rb').read(); h = struct.unpack('<14H', d[:28]); return d, h, d[h[4] * 16:]
a, ha, ia = load(sys.argv[1]); b, hb, ib = load(sys.argv[2])
print('header A', [hex(x) for x in ha]); print('header B', [hex(x) for x in hb])
print('image sizes', hex(len(ia)), hex(len(ib)))
n = min(len(ia), len(ib))
diffs = [i for i in range(n) if ia[i] != ib[i]]
print('image diffs', len(diffs), 'first', hex(diffs[0]) if diffs else None)
for i in diffs[:6]:
    print(hex(i), ia[i:i + 12].hex(' '), '|', ib[i:i + 12].hex(' '))
