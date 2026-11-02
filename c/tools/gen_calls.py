#!/usr/bin/env python3
"""Turns a straight-line function of the original (only pushes + calls) into C calls.
usage: gen_calls.py ADDR LEN   -> prints C statements. Handles setcolor/cprint/cursor/clrscr/puttext."""
import re, subprocess, sys, os
HERE = os.path.dirname(os.path.abspath(__file__))
out = subprocess.run([sys.executable, os.path.join(HERE, "dis.py")] + sys.argv[1:3], capture_output=True, text=True).stdout
ARGS = {"setcolor": 3, "f_243": 3, "cursor": 1, "clrscr": 0, "puttext": 5, "gotoxy": 2, "sfx": 1}
NAME = {"f_243": "cprint"}
stack, last = [], None
for l in out.splitlines():
    p = l.split(None, 2)
    if len(p) < 2:
        continue
    ins = l.split(None, 1)[1].strip() if not re.match(r"^[0-9A-F]+\s+[0-9A-F]{2,}\s", l) else l.split(None, 2)[2].strip()
    m = re.match(r"mov (ax|al),(0x[0-9a-f]+|\d+)$", ins)
    if m:
        last = int(m.group(2), 0); continue
    if re.match(r"xor ax,ax$", ins):
        last = 0; continue
    m = re.match(r"push word \[(0x[0-9a-f]+)\]$", ins)
    if m:
        stack.append("*%s" % m.group(1)); continue
    if ins == "push ax":
        stack.append(last); continue
    m = re.match(r"call (\S+)$", ins)
    if m:
        f = m.group(1); n = ARGS.get(f)
        if n is None:
            continue
        a = [stack.pop() for _ in range(n)]
        print("%s(%s);" % (NAME.get(f, f), ", ".join(str(x) for x in a)))
