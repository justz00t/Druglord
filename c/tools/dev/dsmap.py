#!/usr/bin/env python3
"""Work out where every global variable / library routine used by src/*.c lives in the
original DRUGLORD.EXE.  Each src file is compiled to an OBJ (Turbo C in DOSBox-X); every
linker fixup in the OBJ is compared with the bytes of the original function.

usage: dsmap.py            -> prints  kind symbol address  (data = DS offset, code = image offset)
"""
import os, re, struct, subprocess, sys, glob, collections
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import match

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "..", "src")
DOS = match.DOS


def compile_all(files):
    w = os.path.join(DOS, "cdrive", "M")
    os.makedirs(w, exist_ok=True)
    for f in os.listdir(w):
        os.remove(os.path.join(w, f))
    lines = ["path C:\\TC;Z:\\", "cd \\M"]
    for f in files:
        n = os.path.splitext(os.path.basename(f))[0].upper()[:8]
        s = open(f, "rb").read().replace(b"\r\n", b"\n").replace(b"\n", b"\r\n")
        open(os.path.join(w, n + ".C"), "wb").write(s)
        lines.append("tcc -ms -c -IC:\\TC\\INCLUDE %s.C >> OUT.TXT" % n)
    open(os.path.join(DOS, "cdrive", "MATCH.BAT"), "w").write("\r\n".join(lines) + "\r\n")
    subprocess.run([os.path.join(DOS, "run.sh"), "MATCH.BAT"], capture_output=True, text=True)
    return w


def read_index(b, j):
    if b[j] & 0x80:
        return ((b[j] & 0x7F) << 8) | b[j + 1], j + 2
    return b[j], j + 1


def parse_obj(path):
    d = open(path, "rb").read()
    i = 0
    ext, seg, code = [], [], {}
    fixes = []        # (offset in code, M, loctype, kind, index)
    lnames = []
    last = 0
    while i < len(d):
        t, ln = d[i], struct.unpack("<H", d[i + 1:i + 3])[0]
        b = d[i + 3:i + 3 + ln - 1]
        if t == 0x8C:       # EXTDEF
            j = 0
            while j < len(b):
                n = b[j]; ext.append(b[j + 1:j + 1 + n].decode()); j += 2 + n
        elif t == 0xA0:     # LEDATA
            sg = b[0]; off = struct.unpack("<H", b[1:3])[0]
            if sg == 1:
                for k, x in enumerate(b[3:]):
                    code[off + k] = x
            last = off if sg == 1 else None
        elif t == 0x9C and last is not None:    # FIXUPP
            j = 0
            while j < len(b):
                b0 = b[j]
                if b0 & 0x80:
                    M = (b0 >> 6) & 1
                    lt = (b0 >> 2) & 15
                    loc = ((b0 & 3) << 8) | b[j + 1]; j += 2
                    fd = b[j]; j += 1
                    if not (fd & 0x80) and ((fd >> 4) & 7) < 3:
                        _, j = read_index(b, j)
                    tm = fd & 3
                    idx = None
                    if not (fd & 0x08):
                        idx, j = read_index(b, j)
                    else:
                        idx = ("thread", fd & 3)
                    disp = 0
                    if not (fd & 0x04):
                        disp = struct.unpack("<H", b[j:j + 2])[0]; j += 2
                    fixes.append((last + loc, M, lt, tm, idx, disp))
                else:           # THREAD subrecord: header [+ index]
                    j += 1
                    if ((b0 >> 2) & 7) < 3:
                        _, j = read_index(b, j)
        i += 3 + ln
    n = max(code) + 1 if code else 0
    return ext, bytes(code.get(k, 0) for k in range(n)), fixes


def main():
    files = sorted(glob.glob(os.path.join(SRC, "*.c")))
    w = compile_all(files)
    og = match.orig_image()
    funcs = []
    for l in open(os.path.join(HERE, "..", "functions.txt")):
        if l[0] != "#" and l.strip():
            a, ln, _ = l.split(); funcs.append((int(a, 16), int(ln, 16)))
    result = collections.defaultdict(set)
    for f in files:
        n = os.path.splitext(os.path.basename(f))[0].upper()[:8]
        p = os.path.join(w, n + ".OBJ")
        if not os.path.exists(p):
            print("# no obj for", f, file=sys.stderr); continue
        ext, code, fixes = parse_obj(p)
        fixset = set()
        for fx in fixes:
            for k in range(2): fixset.add(fx[0] + k)
        base = None
        for a, ln in funcs:
            if a + len(code) <= len(og) and all(og[a + k] == code[k] for k in range(len(code)) if k not in fixset) \
               and ln <= len(code):
                base = a; break
        if base is None:
            print("# no match for", f, file=sys.stderr); continue
        for off, M, lt, tm, idx, disp in fixes:
            if tm != 2 or not isinstance(idx, int) or idx - 1 >= len(ext):
                continue
            name = ext[idx - 1]
            mine = code[off] | (code[off + 1] << 8)
            if M == 0:     # self relative: call
                tgt = (struct.unpack("<h", og[base + off:base + off + 2])[0] + base + off + 2 - mine) & 0xFFFF
                result[("code", name)].add(tgt)
            else:
                orig = og[base + off] | (og[base + off + 1] << 8)
                result[("data", name)].add((orig - mine) & 0xFFFF)
    for (k, n), v in sorted(result.items(), key=lambda kv: (kv[0][0], min(kv[1]))):
        print(k, n, " ".join("%04x" % x for x in sorted(v)))


if __name__ == "__main__":
    main()
