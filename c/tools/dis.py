#!/usr/bin/env python3
"""Disassemble a function of the original exe: dis.py ADDR_HEX [LENGTH_HEX]
Call targets are replaced by library names (from names.txt) or f_ADDR, and string/data
addresses are shown as-is."""
import os, re, struct, subprocess, sys
HERE = os.path.dirname(os.path.abspath(__file__))
ORIG = os.environ.get("DL_ORIG", "/mnt/user-data/uploads/DRUGLORD.EXE")
names = {}
for l in open(os.path.join(HERE, "..", "names.txt")):
    if l.strip() and not l.startswith("#"):
        a, n = l.split()[:2]; names[int(a, 16)] = n
d = open(ORIG, "rb").read()
img = d[struct.unpack("<H", d[8:10])[0] * 16:]
a = int(sys.argv[1], 16)
n = int(sys.argv[2], 16) if len(sys.argv) > 2 else 0x100
open("/tmp/claude-0/_d.bin", "wb").write(img[a:a + n])
out = subprocess.run(["ndisasm", "-b", "16", "-o", str(a), "/tmp/claude-0/_d.bin"], capture_output=True, text=True).stdout
for l in out.splitlines():
    m = re.match(r"(\S+)\s+(\S+)\s+call (0x[0-9a-f]+)", l)
    if m:
        t = int(m.group(3), 16)
        l = "%s  call %s" % (m.group(1)[4:], names.get(t, "f_%x" % t))
    else:
        l = l[4:]
    print(l)
