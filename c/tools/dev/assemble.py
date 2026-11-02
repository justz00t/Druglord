#!/usr/bin/env python3
"""One-off helper that stitched the per-function files in src/ and the data tables of the
original EXE into DRUGLORD.C / SCREENS.C.  Kept for reference only."""
import re, struct, sys
d = open('/mnt/user-data/uploads/DRUGLORD.EXE', 'rb').read()
img = d[0x20 * 16:]
ds = img[0xa710:]

def cstr(a): return ds[a:ds.index(0, a)]
def esc(b):
    o = ''
    for c in b:
        ch = chr(c)
        if ch == '"': o += '\\"'
        elif ch == '\\': o += '\\\\'
        elif ch == '\r': o += '\\r'
        elif ch == '\n': o += '\\n'
        elif 32 <= c < 127 and ch != '?': o += ch
        else: o += '\\%03o' % c
    return o
def ptrs(a, n): return [struct.unpack('<H', ds[a + 2 * i:a + 2 * i + 2])[0] for i in range(n)]
def strlist(a, n): return ',\n    '.join('"%s"' % esc(cstr(p)) for p in ptrs(a, n))
def f32(x):
    for p in range(1, 10):
        s = '%.*g' % (p, x)
        if struct.pack('<f', float(s)) == struct.pack('<f', x):
            if '.' not in s and 'e' not in s: s += '.0'
            return s + 'f'
def hexrows(b, per=16, ind='    '):
    return ',\n'.join(ind + ', '.join('0x%02x' % c for c in b[i:i + per]) for i in range(0, len(b), per))

addr = {'setcolor':0x1fa,'cprint':0x243,'flushkeys':0x2e4,'cursor':0x2f1,'sfx':0x314,'box':0x5ca,'fillch':0x6b3,'readfield':0x6d5,'frame':0x814,'drawnames':0x83f,'drawstatus':0x89c,'drawmain':0xb22,'travelcost':0xd17,'drawcities':0xdd3,'genprices':0xedd,'dice':0x1182,'drawdruglist':0x11a5,'drawhp':0x12a9,'marketevent':0x1314,'pressany':0x1406,'event':0x14ab,'playagain':0x2812,'endgame':0x28f8,'nextday':0x3215,'intro':0x32a9,'newgame':0x34a6,'monofix':0x3647,'gamemain':0x3694}
order = sorted(addr, key=lambda k: addr[k])

o = []
w = o.append
w('#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <conio.h>\n#include <ctype.h>\n#include <dos.h>\n#include <math.h>\n#include <time.h>\n')
w('void setcolor(int fg, int bg, char blink);\nvoid fillch(char c, int n, char *buf);\nint readfield(int width, char flag, char *buf);\nvoid marketevent(int count, int rng, float mul);\n')
w('extern void cprint(), cursor(), sfx(), box(), frame(), drawnames(), drawstatus(), drawmain(), drawcities(), drawdruglist(), drawhp(), pressany(), flushkeys(), genprices(), nextday(), intro(), newgame(), monofix(), endgame(), event(), restorescreen();\nextern int dice(), playagain();\nextern long travelcost();\n')
w('struct range { long lo, hi; };\n')
w('/* ---- initialised data ---- */')
w('unsigned char scrmain[2560] = {\n%s\n};' % hexrows(ds[0x194:0xb94]))
w('char unusedbyte = 0;')
w('char *drugname[8] = {\n    %s\n};' % strlist(0xb95, 8))
w('char *gunname[6] = {\n    %s\n};' % strlist(0xba5, 6))
w('char *gunlong[5] = {\n    %s\n};' % strlist(0xbb1, 5))
w('char *txt[14] = {\n    %s\n};' % strlist(0xbbb, 14))
w('char *cityname[8] = {\n    %s\n};' % strlist(0xbd7, 8))
w('char cityxy[8][2] = {\n' + ',\n'.join('    {%d, %d}' % (ds[0xbe7 + 2 * i], ds[0xbe8 + 2 * i]) for i in range(8)) + '\n};')
fl = [struct.unpack('<f', ds[0xbf7 + 4 * i:0xbfb + 4 * i])[0] for i in range(64)]
w('float citymul[8][8] = {\n' + ',\n'.join('    {' + ', '.join(f32(x) for x in fl[8 * r:8 * r + 8]) + '}' for r in range(8)) + '\n};')
w('int gunpow[6] = { %s };' % ', '.join(str(struct.unpack('<h', ds[0xcf7 + 2 * i:0xcf9 + 2 * i])[0]) for i in range(6)))
w('struct range prange[8] = {\n' + ',\n'.join('    {%dL, %dL}' % struct.unpack('<ll', ds[0xd03 + 8 * i:0xd0b + 8 * i]) for i in range(8)) + '\n};')
w('int dim[12] = { %s };' % ', '.join(str(struct.unpack('<h', ds[0xd43 + 2 * i:0xd45 + 2 * i])[0]) for i in range(12)))
w('/* ---- uninitialised data ---- */')
w('''int mkttoday;
int flag7920;
float bias[8];
unsigned basis[8];
int esc;
int dead;
int holdused;
long cash;
int day;
int g_mono;
int month;
int year;
int health;
long bank;
int guntype;
int hold;
int status;
long debt;
int curcity;
int loanflag;
char inbuf2[80];
int inv[8];
int loandays;
int goodDeals;
int used[8];
char flag10[10];
int flag79f6;
int delaytime;
int flag79fa;
int totalDeals;
struct text_info ti;
int totaldays;
long price[8];
int ngun;
''')
# endtext rows
rows = [ds[0xd62 + 50 * i:0xd62 + 50 * i + 50] for i in range(18)]
ok = all(r.rstrip(b'\0') == cstr_ for r, cstr_ in [(r, r.split(b'\0')[0]) for r in rows])
assert ok, 'endtext rows not simple strings'
endtext = 'char rep[18][50] = {\n    ' + ',\n    '.join('"%s"' % esc(r.split(b'\0')[0]) for r in rows) + '\n};\n'

for k in order:
    L = open('src/%s.c' % k).read().replace('\r\n', '\n').split('\n')
    keep = []
    for l in L:
        if re.match(r'#include|extern ', l): continue
        if re.match(r'(void|int|char|long|float) \w+\(.*\);\s*$', l): continue
        if l.startswith('struct range') or l.startswith('struct text_info ti;'): continue
        keep.append(l)
    body = '\n'.join(keep).strip('\n')
    if k == 'playagain': w(endtext)
    w(body + '\n')
pass  # open('DRUGLORD.C', 'w').write('\r\n'.join('\n'.join(o).split('\n')) + '\r\n')

# second module: the screen images
blocks = [(0x3382, 0x4328, 'screen_title'), (0x4328, 0x50ea, 'buf4328'), (0x50ea, 0x511c, 'firebuf'), (0x511c, 0x60be, 'savebuf2'),
          (0x60be, 0x64da, 'savebuf'), (0x64da, 0x64e6, 'barbuf'), (0x64e6, 0x67d0, 'tbuf'), (0x67d0, 0x72f4, 'buf67d0')]
s = ['/* screen images, linked after DRUGLORD.C */']
for a, b, n in blocks:
    s.append('unsigned char %s[%d] = {\n%s\n};' % (n, b - a, hexrows(ds[a:b])))
open('SCREENS.C', 'w').write('\r\n'.join('\n'.join(s).split('\n')) + '\r\n')
