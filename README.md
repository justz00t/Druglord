# DrugLord v2.10

A recreation of the DOS game DRUGLORD.EXE v2.01 (Turbo C, by Phil Erwin).
Ported to Windows by justz00t.

- `DrugLord.exe` (see Releases): native 64-bit Windows window, scales with the window size.
- `go/`: Go source (standard library only). See "Building the exe" below.
  `GOOS=windows GOARCH=amd64 go build -ldflags "-H=windowsgui -s -w" -o DrugLord.exe .`
- `DrugLord.ps1`: the earlier PowerShell edition.

Keys: F1 = Load, F2 = Save (Ctrl-L / Ctrl-S also work).

Options: `-nosound -speed -jetspeed -soundmode -wave -duty -volume -pitch -tonestretch -savedir -soundlab`

Saves go next to the exe, or `%AppData%\DrugLord` if that folder is not writable.
Unsigned exe: Windows SmartScreen may warn ("More info" > "Run anyway").

## Building the exe

You only need [Go](https://go.dev/dl/) 1.24 or newer. No other dependencies.

**On Windows (PowerShell or Command Prompt):**

```
cd go
go build -ldflags "-H=windowsgui -s -w" -o DrugLord.exe .
```

**On Linux or macOS (cross-compile for Windows):**

```
cd go
GOOS=windows GOARCH=amd64 go build -ldflags "-H=windowsgui -s -w" -o DrugLord.exe .
```

Notes:
- `-H=windowsgui` makes it a window program (no console box). Leave it out if you want a console attached for debugging.
- `-s -w` shrinks the file. They are optional.
- For 32-bit Windows use `GOARCH=386`, for Windows on ARM use `GOARCH=arm64`.
- The window code is Windows-only. Building on Linux or macOS without `GOOS=windows` makes a stub that only runs the headless test mode (`DRUGLORD_HEADLESS=1`).
