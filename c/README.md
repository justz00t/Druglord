# DRUGLORD v2.01 – byte-matched C source

This folder contains a C reconstruction of **DRUGLORD.EXE v2.01** (Phil Erwin,
FantasyWare Inc., 1991-93) that, compiled with **Borland Turbo C 2.01** in the
**small memory model**, produces an EXE that is **byte-for-byte identical** to the original
`DRUGLORD.EXE` (74,286 bytes, MD5 `1ea0bd4c15546a1dc44365e80fb4b7cf`).

## Files

| File | What it is |
|------|------------|
| `DRUGLORD.C` | The game: all 28 functions, the tables and the game state. Heavily commented. |
| `SCREENS.C` | Second module with the picture data (title, fight, death, end scene...). |
| `SCRMAIN.INC` | The main-screen picture (hex bytes), included by `DRUGLORD.C`. |
| `tools/` | Scripts used to build and verify (see "Verifying" below). `tools/dev/` holds the helper scripts used while reconstructing. |

## Compiler

* **Borland Turbo C 2.01** (the original EXE contains the text
  `Turbo-C - Copyright (c) 1988 Borland Intl.`).  Version 2.0 does *not* match; it has to be 2.01.
* **Small memory model** (`-ms`, which is also the default of `tcc`).
* No optimisation options, no debug information, default stack size, default floating-point
  setting (software emulation, `-f`).

## How to compile it with Borland / Turbo C 2.01

You need real DOS or an emulator such as DOSBox with Turbo C 2.01 installed (for example in
`C:\TC`).

1. Copy `DRUGLORD.C`, `SCREENS.C` and `SCRMAIN.INC` into one folder, e.g. `C:\DL`.
   Keep the files' **DOS (CRLF) line endings** – if you transferred them through something
   that converted them to Unix line endings, convert them back, otherwise the result will
   differ.
2. Open a DOS prompt and make sure Turbo C is on the path:
   ```
   path C:\TC;%PATH%
   cd \DL
   ```
3. Compile and link both files in one command:
   ```
   tcc -ms DRUGLORD.C SCREENS.C
   ```
   (If `tcc` cannot find the headers or libraries add
   `-IC:\TC\INCLUDE -LC:\TC\LIB`.)
4. The result is `DRUGLORD.EXE` in the same folder.  It should be exactly 74,286 bytes and
   identical to the original:
   ```
   fc /b DRUGLORD.EXE ORIGINAL\DRUGLORD.EXE
   ```
   (`FC: no differences encountered`).

Using the Turbo C **IDE** instead: create a project (`Project > Open project...`) containing
`DRUGLORD.C` and `SCREENS.C`, set `Options > Compiler > Model` to **Small**, and choose
`Compile > Build all`.  The IDE must have Floating point = Emulation (the default) and
Optimization/Debug settings at their defaults.

The module order matters: **`DRUGLORD.C` first, `SCREENS.C` second**, because the linker lays
out data in that order.

## Why the source looks the way it does

To get identical bytes the source has to mimic what the author's compiler saw, so a few
things look odd.  Each one is also explained where it appears in the code.

* **One big `struct GameState g`.** In the original these were separate global variables.
  Turbo C's linker orders uninitialised globals by a hash of their *names*, which cannot be
  recovered, so the original's memory layout is reproduced by listing the fields in the
  original address order inside one struct.
* **K&R function headers** (`int f(a, b) int a; int b; { ... }`).  The compiler generates
  different code for old-style definitions (arguments are pushed as `int`), so they are used
  wherever the original did.  ANSI prototypes are used only where an argument really is a
  `char` or `float` (`setColor`, `fillChars`, `readField`, `marketEvent`).
* **Strings and tables in the original order.** Turbo C stores string literals in the order
  they first appear, without merging duplicates, so the order of the text and the odd
  unused `"";` in `marketEvent` are significant.
* **Odd constructs** such as `generatePrices()` taking eight `long *` pointers, `padByte`, the
  unused main-screen copy in `SCREENS.C`, or `bonus` being reused as the hospital bill are
  there because the original does it that way.
* **Two modules.** The program was compiled from two source files, so the picture data
  is in `SCREENS.C`.
* **Register variables.**  `register int` declarations (SI, then DI) and the order of local
  declarations decide the stack layout and the register use.

Because of all this: if you change the code, you will most likely no longer get an
identical EXE.  That is fine for experiments, but compare again if byte-exactness matters.

## Verifying (how the match was checked)

`tools/build.py` copies the three source files into a DOSBox-X folder where Turbo C 2.01 is
installed, runs `tcc -ms DRUGLORD.C SCREENS.C`, and compares the resulting EXE with the
original:

```
python3 tools/build.py          # prints IDENTICAL when all 74,286 bytes match
```

The paths of DOSBox and of the original EXE are set at the top of the script (or through the
environment variables `DL_DOSBOX` and `DL_ORIG`); `tools/run-dosbox.sh` starts DOSBox-X
headless.  Turbo C and DOSBox are not part of this repository.  `tools/cmpexe.py` compares
two EXEs header by header, which helps when something differs.

The final build was also compared with `cmp` and the MD5 checksum above.

## How it was done (short version)

1. The compiler and memory model were identified by comparing library code (startup code, `printf`
   family, floating-point emulator, ...) in the EXE with test programs compiled with several Turbo C
   versions and models.  Turbo C 2.01 / small was by far the best fit.
2. The functions were located and disassembled; the calls to library routines were resolved
   from the object-file fixups.
3. Each function was rewritten in C and compiled, and the machine code compared with the
   original (addresses the linker fills in were masked out).  Idioms that influence the code
   were found by trial: `else` after `break`, `while(1)` versus `for(;;)`, jump tables for `switch`, etc.
4. Finally all functions, the data tables and the literal strings were put together in the
   original order, the globals were collected into one struct, and the whole EXE was linked
   and compared until every byte was identical.
