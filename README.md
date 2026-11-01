# DrugLord v2.10

A recreation of the DOS game DRUGLORD.EXE v2.01 (Turbo C, by Phil Erwin).
Ported to Windows by justz00t.

- `DrugLord.exe` (see Releases): native 64-bit Windows window, scales with the window size.
- `go/`: Go source (standard library only). Build from `go/`:
  `GOOS=windows GOARCH=amd64 go build -ldflags "-H=windowsgui -s -w" -o DrugLord.exe .`
- `DrugLord.ps1`: the earlier PowerShell edition.

Keys: F1 = Load, F2 = Save (Ctrl-L / Ctrl-S also work).

Options: `-nosound -speed -jetspeed -soundmode -wave -duty -volume -pitch -tonestretch -savedir -soundlab`

Saves go next to the exe, or `%AppData%\DrugLord` if that folder is not writable.
Unsigned exe: Windows SmartScreen may warn ("More info" > "Run anyway").
