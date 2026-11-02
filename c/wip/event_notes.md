# Work in progress: the event handler at 0x14ab (length 0x1367)

`event(kind)` switches on `kind` (0..9) after `if (flag10[kind] == 1) return;`.
Locals: `char *lbl[2] = {"Health","Status"}` at bp-0x10 (copied from DS 0xd5e),
`int friends` bp-0x62, `int choice` bp-0x64, `int copcount` bp-0x30, hp arrays at
bp-0x60 (player + friends) and bp-0x4a (cops), cop type bp-0x34.
Case 0 = the cops chase: pick 1/2/3 (alone / friend / posse), draw the fight screen with
`drawhp` (0x12a9), then F)ight / R)un rounds.
Strings are in /tmp/.../strs_c.txt (octal-escaped) around DS offsets 0x1400-0x2100.
Remaining to read: disassembly from 0x1a44 on (fight loop, run, loot, other cases).
Tools: `python3 tools/dis.py 14ab 1367`, `tools/match.py src/event.c 14ab 1367`.
