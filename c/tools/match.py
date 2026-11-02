#!/usr/bin/env python3
"""Compile one C file with Turbo C 2.01 (-ms) in DOSBox-X and compare its code with a function
of the original DRUGLORD.EXE.

usage: match.py FILE.C ORIG_ADDR [LENGTH]
   FILE.C     C source containing ONE function (plus extern declarations for what it uses)
   ORIG_ADDR  hex offset of that function in the original code image (see c/functions.txt)
   LENGTH     hex byte length of the original function (default: taken from c/functions.txt)

Bytes that are filled in by the linker (calls, data addresses) are masked out, so only the
compiler's own output is compared. Prints OK, or the first mismatch with a disassembly-friendly dump.
"""
import os, struct, subprocess, sys, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
DOS = os.environ.get("DL_DOSBOX", "/tmp/claude-0/dosbox")           # holds run.sh and cdrive/
ORIG = os.environ.get("DL_ORIG", "/mnt/user-data/uploads/DRUGLORD.EXE")
CFLAGS = os.environ.get("DL_CFLAGS", "-ms")


def orig_image():
    d = open(ORIG, "rb").read()
    return d[struct.unpack("<H", d[8:10])[0] * 16:]


def parse_omf(path):
    """Return (code bytes of the _TEXT segment, set of byte offsets covered by fixups)."""
    d = open(path, "rb").read()
    i, code, fix = 0, {}, set()
    last_off, last_len = 0, 0
    while i < len(d):
        t, ln = d[i], struct.unpack("<H", d[i + 1:i + 3])[0]
        body = d[i + 3:i + 3 + ln - 1]
        if t == 0xA0:  # LEDATA: segment index, offset, data
            seg, off = body[0], struct.unpack("<H", body[1:3])[0]
            for k, b in enumerate(body[3:]):
                code[off + k] = b
            last_off = off
        elif t == 0x9C:  # FIXUPP: THREAD or FIXUP subrecords
            j = 0
            while j < len(body):
                b0 = body[j]
                if b0 & 0x80:  # FIXUP: locat(2) fixdata(1) [frame datum] [target datum] [displacement]
                    loc = ((b0 & 3) << 8) | body[j + 1]
                    kind = (b0 >> 2) & 15
                    j += 2
                    fd = body[j]; j += 1
                    if not (fd & 0x80) and ((fd >> 4) & 7) < 3:
                        j += 1                      # explicit frame datum (index)
                    if not (fd & 0x08):
                        j += 1                      # explicit target datum (index)
                    if not (fd & 0x04):
                        j += 2                      # displacement
                    width = {0: 1, 1: 2, 2: 2, 3: 4, 4: 2, 5: 2}.get(kind, 2)
                    for k in range(width):
                        fix.add(last_off + loc + k)
                else:  # THREAD
                    j += 2 if (b0 & 0x40) == 0 else 1  # thread: header byte + optional index
        i += 3 + ln
    n = max(code) + 1 if code else 0
    return bytes(code.get(k, 0) for k in range(n)), fix


def main():
    cfile, addr = sys.argv[1], int(sys.argv[2], 16)
    length = int(sys.argv[3], 16) if len(sys.argv) > 3 else None
    name = os.path.splitext(os.path.basename(cfile))[0].upper()[:8]
    w = os.path.join(DOS, "cdrive", "M")
    os.makedirs(w, exist_ok=True)
    for f in os.listdir(w):
        os.remove(os.path.join(w, f))
    src = open(cfile, "rb").read().replace(b"\r\n", b"\n").replace(b"\n", b"\r\n")
    open(os.path.join(w, name + ".C"), "wb").write(src)
    bat = os.path.join(DOS, "cdrive", "MATCH.BAT")
    open(bat, "w").write("path C:\\TC;Z:\\\ncd \\M\ntcc %s -c -IC:\\TC\\INCLUDE %s.C > OUT.TXT\n" % (CFLAGS, name))
    subprocess.run([os.path.join(DOS, "run.sh"), "MATCH.BAT"], capture_output=True, text=True)
    out = open(os.path.join(w, "OUT.TXT"), errors="replace").read()
    objp = os.path.join(w, name + ".OBJ")
    if not os.path.exists(objp):
        print(out); print("COMPILE FAILED"); sys.exit(2)
    if "Error" in out or "Warning" in out:
        print(out.strip().split("\n", 2)[-1])
    mine, fix = parse_omf(objp)
    og = orig_image()
    if length is None:
        length = len(mine)
    want = og[addr:addr + length]
    print("mine %d bytes, original %d bytes" % (len(mine), len(want)))
    ok = len(mine) == len(want)
    first = None
    for k in range(min(len(mine), len(want))):
        if k not in fix and mine[k] != want[k]:
            first = k; ok = False; break
    if ok:
        print("OK  (matches, %d bytes, %d masked)" % (len(mine), len(fix)))
        return
    print("MISMATCH" + (" at +0x%x" % first if first is not None else " (length differs)"))
    dis = {}
    for lab, b in (("mine", mine), ("orig", want)):
        open("/tmp/claude-0/_%s.bin" % lab, "wb").write(b)
        r = subprocess.run(["ndisasm", "-b", "16", "/tmp/claude-0/_%s.bin" % lab], capture_output=True, text=True)
        dis[lab] = [l.split(None, 1)[1] if len(l.split(None, 1)) > 1 else l for l in r.stdout.splitlines()]
    n = max(len(dis["mine"]), len(dis["orig"]))
    print("%-48s | %s" % ("MINE (addresses show as 0000)", "ORIGINAL"))
    for k in range(min(n, int(os.environ.get("DL_LINES", "60")))):
        a = dis["mine"][k] if k < len(dis["mine"]) else ""
        b = dis["orig"][k] if k < len(dis["orig"]) else ""
        print("%-48s %s %s" % (a[:48], "|" if a.split()[1:] == b.split()[1:] or True else "|", b[:60]))
    shutil.copy(objp, "/tmp/claude-0/last.obj")
    sys.exit(1)


main()
