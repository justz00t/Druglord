# Compares the library/startup code of test programs built in each Turbo C memory
# model against DRUGLORD.EXE, using 14-byte windows. The best-matching model is the
# one the game was most likely built with.
# usage: python3 compare_models.py DRUGLORD.EXE ms.exe mm.exe mc.exe ml.exe mh.exe
import struct, sys
def img(p):
    d = open(p, 'rb').read()
    return d[struct.unpack('<H', d[8:10])[0] * 16:]
W = 14
game = img(sys.argv[1])
gset = set(game[i:i + W] for i in range(len(game) - W))
for p in sys.argv[2:]:
    t = img(p)
    wins = [t[i:i + W] for i in range(len(t) - W)]
    hit = sum(1 for w in wins if w in gset)
    print(p, "matched %d of %d (%.0f%%)" % (hit, len(wins), 100.0 * hit / len(wins)))
