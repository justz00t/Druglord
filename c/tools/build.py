#!/usr/bin/env python3
"""Build the whole game with Turbo C 2.01 (in DOSBox-X) and compare the EXE with the original.
usage: build.py [source dir (default: ..)]"""
import os, subprocess, sys, shutil
HERE = os.path.dirname(os.path.abspath(__file__))
DOS = os.environ.get("DL_DOSBOX", "/tmp/claude-0/dosbox")
ORIG = os.environ.get("DL_ORIG", "/mnt/user-data/uploads/DRUGLORD.EXE")
src = os.path.abspath(sys.argv[1]) if len(sys.argv) > 1 else os.path.abspath(os.path.join(HERE, ".."))
w = os.path.join(DOS, "cdrive", "M")
os.makedirs(w, exist_ok=True)
for f in os.listdir(w): os.remove(os.path.join(w, f))
for f in ("DRUGLORD.C", "SCREENS.C"):
    shutil.copy(os.path.join(src, f), os.path.join(w, f))
open(os.path.join(DOS, "cdrive", "MATCH.BAT"), "w").write(
    "path C:\\TC;Z:\\\r\ncd \\M\r\ntcc -ms -IC:\\TC\\INCLUDE -LC:\\TC\\LIB DRUGLORD.C SCREENS.C > OUT.TXT\r\n")
subprocess.run([os.path.join(DOS, "run.sh"), "MATCH.BAT"], capture_output=True, text=True)
out = open(os.path.join(w, "OUT.TXT"), errors="replace").read()
print(out.strip())
exe = os.path.join(w, "DRUGLORD.EXE")
if not os.path.exists(exe):
    print("BUILD FAILED"); sys.exit(2)
a, b = open(exe, "rb").read(), open(ORIG, "rb").read()
shutil.copy(exe, "/tmp/claude-0/built.exe")
print("built %d bytes, original %d bytes" % (len(a), len(b)))
diffs = [i for i in range(min(len(a), len(b))) if a[i] != b[i]]
if not diffs and len(a) == len(b):
    print("IDENTICAL"); sys.exit(0)
print("%d differing bytes; first at 0x%x" % (len(diffs), diffs[0] if diffs else -1))
sys.exit(1)
