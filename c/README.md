# Matching the original DRUGLORD.EXE (work in progress)

Goal: recreate source that Turbo C compiles to the same bytes as the original
`DRUGLORD.EXE`. Nothing here is the game yet; this records what has been found.

## Status: 26 of 28 game functions match the original bytes

Each file in `src/` is compiled with Turbo C 2.01 (`-ms`) and compared with the same
function in the original `DRUGLORD.EXE`; bytes that the linker fills in (addresses of
globals and library calls) are masked out via the OBJ fixups. Only the **original** game is
reproduced here: nothing from the later Go/PowerShell ports (gun-count fix, extra screens,
wording) is part of this C source.

Matched: setcolor, cprint, flushkeys, cursor, sfx, fillch, box, frame, drawnames,
readfield, drawmain, travelcost, dice, drawhp, monofix, nextday, pressany, drawdruglist,
marketevent, drawcities, drawstatus, genprices, playagain (+ restorescreen), newgame,
intro, endgame.

Still open: the two big functions at 0x14ab (random events and actions) and 0x3694
(`main`: the game loop).

## Findings

- **Compiler:** `DRUGLORD.EXE` contains `Turbo-C - Copyright (c) 1988 Borland Intl.`
  **Turbo C 2.01** reproduces the library code better than 2.0 (83% versus 62% of a test
  program's code found byte-for-byte), and every function so far matches with 2.01.
- **Memory model: small (`-ms`).** Test program share found in the game: small 83%,
  tiny 72%, medium 66%, compact 51%, large 47%, huge 45%.
- **Style idioms that matter for a byte match:**
  - K&R definitions (`f(a, b) int a; int b;`) for functions whose callers push words;
    ANSI prototypes only where a `float` parameter is really passed as a 4-byte float
    (`marketevent`).
  - `register int i, j;` gives SI then DI; other locals get stack slots, the last declared
    one at the highest address.
  - `else` after a `break` creates a dead `jmp`, so the original has none.
  - Source files need DOS (CRLF) line endings and octal escapes (`\221`) next to digits.
- **Names:** the names of globals and helper routines in `src/` are guesses; only the
  code bytes are verified.

## How it was done

Turbo C 2.01 runs under DOSBox-X (headless). `tools/run-dosbox.sh` runs a `.bat` file
there with `C:` mapped to a folder. Source files need DOS (CRLF) line endings.
Compile line: `tcc -ms -IC:\TC\INCLUDE -LC:\TC\LIB -eNAME FILE.C`.
Turbo C and DOSBox themselves are not stored in this repo.

## Next steps

1. Match 0x14ab and 0x3694.
2. Link everything into one program (needs the exact order of globals and string
   literals) and compare the whole EXE.
