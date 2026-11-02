# Matching the original DRUGLORD.EXE (work in progress)

Goal: recreate source that Turbo C compiles to the same bytes as the original
`DRUGLORD.EXE`. Nothing here is the game yet; this records what has been found.

## Findings so far

- **Compiler family:** `DRUGLORD.EXE` contains `Turbo-C - Copyright (c) 1988 Borland Intl.`
  The startup code (`C0S.OBJ`) of **Turbo C 2.01** contains the identical string.
- **Memory model: small (`-ms`).** A test program (`tests/hello.c`: rand, atol, conio,
  sqrt/pow, cprintf) built with Turbo C 2.01 in each model was compared with the game by
  14-byte windows. Share of the test program's code found byte-for-byte in the game:
  small 83%, tiny 72%, medium 66%, compact 51%, large 47%, huge 45%.
- **Not yet known:** 2.01 versus 2.0 (or 1.x). Only 2.01 has been tested so far; the
  other versions are needed to tell them apart.

## How it was done

Turbo C 2.01 runs under DOSBox-X (headless). `tools/run-dosbox.sh` runs a `.bat` file
there with `C:` mapped to a folder. Source files need DOS (CRLF) line endings.
Compile line: `tcc -ms -IC:\TC\INCLUDE -LC:\TC\LIB -eNAME FILE.C`.
Turbo C and DOSBox themselves are not stored in this repo.

## Next steps

1. Test other Turbo C versions (2.0, 1.5, 1.0) the same way to pin the exact version.
2. Find the floating-point and optimization options that make the library code match fully.
3. Write the game as C (`conio`, `dos.h` sound, `rand`, `atol`, `math`), then match it function by function.
