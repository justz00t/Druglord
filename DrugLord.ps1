<#
.SYNOPSIS
    DRUGLORD - a faithful PowerShell port of the 1991-93 DOS game DRUGLORD.EXE v2.01.

.DESCRIPTION
    Text-mode 80x25 game (centred when the window is bigger): buy and sell drugs between eight cities, dodge the cops,
    watch your loan, build your status. Original screens, text, prices, events and
    PC-speaker sound effects (played through [Console]::Beep) are reproduced from
    the executable.

    Run it in a normal Windows console (conhost / Windows Terminal), PowerShell 5.1 or 7+:
        powershell -ExecutionPolicy Bypass -File .\DrugLord.ps1
    Not in the ISE (it has no real console).

    Keys: B)uy S)ell J)et V)isit bank L)oan shark H)ospital Q)uit.  ESC cancels prompts.

    F2 saves the game (save1.sav, save2.sav, ...), F1 loads one. Ctrl-S / Ctrl-L also work where the terminal passes them on.

.PARAMETER NoSound
    Disable the PC-speaker sound effects.

.PARAMETER Speed
    Speed of the whole program: every pause, animation and sound effect (tone lengths
    and gaps) is scaled by it. 1.0 = as shipped, 2.0 = twice as fast, 0.5 = half
    speed. Range 0.1 - 10. (-SoundSpeed is accepted as an alias.)
        .\DrugLord.ps1 -Speed 1.25

.PARAMETER SoundLab
    Opens an interactive sound lab: play every effect and adjust the settings below
    live; quitting prints the command line that reproduces what you settled on.

.PARAMETER SoundMode
    Wave (default): each effect is synthesised as a continuous PC-speaker-style wave
    and played in one piece. Beep: one Console.Beep call per tone (older behaviour).

.PARAMETER Wave
    Wave shape for -SoundMode Wave: Square (default, like the PC speaker), Sine, Saw, Triangle.

.PARAMETER Duty
    Square-wave pulse width 0.05-0.95 (0.5 = true square; smaller sounds thinner/sharper).

.PARAMETER Volume
    0 - 1 (default 0.35).

.PARAMETER Pitch
    Multiplies every tone frequency (default 1.0).

.PARAMETER ToneStretch
    Multiplies the length of every tone, not the gaps between them (default 1.0).
    Try 1.5 - 2 if the effects feel too short.

.PARAMETER JetSpeed
    Speed of the "jet to another city" slide and its rising sweep sound (default 2.0;
    1.0 = the original pace, higher = faster). Independent of -Speed.

.PARAMETER SaveDir
    Folder for save files. Default: the folder the script lives in.
#>
param([switch]$NoSound,
      [ValidateSet('Wave', 'Beep')][string]$SoundMode = 'Wave',
      [ValidateSet('Square', 'Sine', 'Saw', 'Triangle')][string]$Wave = 'Square',
      [ValidateRange(0.05, 0.95)][double]$Duty = 0.5,
      [ValidateRange(0.0, 1.0)][double]$Volume = 0.35,
      [ValidateRange(0.25, 4.0)][double]$Pitch = 1.0,
      [ValidateRange(0.1, 10.0)][double]$ToneStretch = 1.0,
      [ValidateRange(0.1, 10.0)][double]$JetSpeed = 2.0,
      [switch]$SoundLab, [Alias('SoundSpeed')][double]$Speed = 1.0, [string]$SaveDir = '')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Off

# ---------------------------------------------------------------------------
#  Console layer: emulates the Borland/Turbo C conio text mode used by the
#  original (80x25 color text, windows, gotoxy, textcolor, puttext, ...).
# ---------------------------------------------------------------------------
$script:Headless = [bool]$env:DRUGLORD_HEADLESS
$script:KeyQueue = New-Object System.Collections.Generic.Queue[int]
$script:CP437 = [int[]]@(32,9786,9787,9829,9830,9827,9824,8226,9688,9675,9689,9794,9792,9834,9835,9788,9658,9668,8597,8252,182,167,9644,8616,8593,8595,8594,8592,8735,8596,9650,9660,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,8962,199,252,233,226,228,224,229,231,234,235,232,239,238,236,196,197,201,230,198,244,246,242,251,249,255,214,220,162,163,165,8359,402,225,237,243,250,241,209,170,186,191,8976,172,189,188,161,171,187,9617,9618,9619,9474,9508,9569,9570,9558,9557,9571,9553,9559,9565,9564,9563,9488,9492,9524,9516,9500,9472,9532,9566,9567,9562,9556,9577,9574,9568,9552,9580,9575,9576,9572,9573,9561,9560,9554,9555,9579,9578,9496,9484,9608,9604,9612,9616,9600,945,223,915,960,931,963,181,964,934,920,937,948,8734,966,949,8745,8801,177,8805,8804,8992,8993,247,8776,176,8729,183,8730,8319,178,9632,160)
$script:SW = 80; $script:SH = 25
$script:CB = New-Object 'int[]' 2000
$script:AT = New-Object 'int[]' 2000
for ($i = 0; $i -lt 2000; $i++) { $script:CB[$i] = 32; $script:AT[$i] = 7 }
$script:wx1 = 1; $script:wy1 = 1; $script:wx2 = 80; $script:wy2 = 25
$script:cx = 1; $script:cy = 1
$script:attr = 7
$script:dirty = New-Object 'int[]' 50          # per row: minX, maxX (0-based), -1 = clean
for ($i = 0; $i -lt 25; $i++) { $script:dirty[$i * 2] = 999; $script:dirty[$i * 2 + 1] = -1 }
$script:anyDirty = $false
$script:CurVisible = $true
$script:Muted = $false
if ($script:Headless -and $env:DRUGLORD_KEYS) {
    foreach ($t in ($env:DRUGLORD_KEYS -split '\s+')) { if ($t -ne '') { $script:KeyQueue.Enqueue([int]$t) } }
}
function Dump-Screen {                         # test aid (headless only): print the shadow screen
    for ($y = 0; $y -lt 25; $y++) {
        $line = New-Object System.Text.StringBuilder
        for ($x = 0; $x -lt 80; $x++) { $c = $script:CP437[$script:CB[$y * 80 + $x]]; [void]$line.Append([char]$c) }
        $line.ToString().TrimEnd()
    }
}

$script:OX = 0; $script:OY = 0; $script:WPX = 0; $script:WPY = 0; $script:LW = 0; $script:LH = 0
function Init-Console {
    if ($script:Headless) { return }
    try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch {}
    try {
        $raw = $Host.UI.RawUI
        $bs = $raw.BufferSize; $ws = $raw.WindowSize
        if ($ws.Width -lt 80 -or $ws.Height -lt 25) {            # too small: grow it; bigger windows are kept and centred
            $w = [Math]::Max($ws.Width, 80); $h = [Math]::Max($ws.Height, 25)
            $raw.BufferSize = New-Object System.Management.Automation.Host.Size([Math]::Max($bs.Width, $w), [Math]::Max($bs.Height, $h))
            $raw.WindowSize = New-Object System.Management.Automation.Host.Size($w, $h)
        }
    } catch {}
    try { $raw.WindowTitle = 'DRUGLORD' } catch {}
    Update-Layout $true
}

# Centre the 80x25 screen in whatever window there is; called again whenever the window is resized or scrolled.
function Update-Layout([bool]$force) {
    if ($script:Headless) { return }
    try {
        $raw = $Host.UI.RawUI
        $ws = $raw.WindowSize; $wp = $raw.WindowPosition
        if (-not $force -and $ws.Width -eq $script:LW -and $ws.Height -eq $script:LH -and $wp.X -eq $script:WPX -and $wp.Y -eq $script:WPY) { return }
        $script:LW = $ws.Width; $script:LH = $ws.Height; $script:WPX = $wp.X; $script:WPY = $wp.Y
        $script:OX = [Math]::Max([int][Math]::Floor(($ws.Width - 80) / 2), 0)
        $script:OY = [Math]::Max([int][Math]::Floor(($ws.Height - 25) / 2), 0)
        $rect = New-Object System.Management.Automation.Host.Rectangle($wp.X, $wp.Y, ($wp.X + $ws.Width - 1), ($wp.Y + $ws.Height - 1))
        $blank = [System.Management.Automation.Host.BufferCell]::new([char]32, [System.ConsoleColor]::Gray, [System.ConsoleColor]::Black, 'Complete')
        $raw.SetBufferContents($rect, $blank)
        for ($y = 0; $y -lt 25; $y++) { $script:dirty[$y * 2] = 0; $script:dirty[$y * 2 + 1] = 79 }
        $script:anyDirty = $true
    } catch {}
}

function Test-ConsoleSize {
    if ($script:Headless) { return $true }
    try { $ws = $Host.UI.RawUI.WindowSize; return ($ws.Width -ge 80 -and $ws.Height -ge 25) } catch { return $true }
}

function Mark-Dirty([int]$x, [int]$y) {
    $k = $y * 2
    if ($x -lt $script:dirty[$k]) { $script:dirty[$k] = $x }
    if ($x -gt $script:dirty[$k + 1]) { $script:dirty[$k + 1] = $x }
    $script:anyDirty = $true
}

function Flush-Screen {
    if (-not $script:anyDirty) { return }
    if (-not $script:Headless) {
        $raw = $Host.UI.RawUI
        for ($y = 0; $y -lt 25; $y++) {
            $a = $script:dirty[$y * 2]; $b = $script:dirty[$y * 2 + 1]
            if ($b -ge $a) {
                $cells = New-Object 'System.Management.Automation.Host.BufferCell[,]' 1, ($b - $a + 1)
                for ($x = $a; $x -le $b; $x++) {
                    $i = $y * 80 + $x; $t = $script:AT[$i]
                    $cells[0, ($x - $a)] = [System.Management.Automation.Host.BufferCell]::new([char]$script:CP437[$script:CB[$i]], [System.ConsoleColor]$(if (($t -band 0x80) -and -not $script:blinkOn) { ($t -shr 4) -band 7 } else { $t -band 15 }), [System.ConsoleColor](($t -shr 4) -band 7), 'Complete')
                }
                try { $raw.SetBufferContents((New-Object System.Management.Automation.Host.Coordinates(($a + $script:OX + $script:WPX), ($y + $script:OY + $script:WPY))), $cells) } catch {}
            }
        }
    }
    for ($y = 0; $y -lt 25; $y++) { $script:dirty[$y * 2] = 999; $script:dirty[$y * 2 + 1] = -1 }
    $script:anyDirty = $false
    Place-Cursor
}

$script:blinkOn = $true
function Blink-Tick {                                  # text-mode blink (attribute bit 7): toggles about twice a second
    if ($script:Headless) { return }
    Update-Layout $false
    if ($script:anyDirty) { Flush-Screen }
    $on = ([int]([Environment]::TickCount / 500) % 2) -eq 0
    if ($on -eq $script:blinkOn) { return }
    $script:blinkOn = $on
    for ($i = 0; $i -lt 2000; $i++) { if ($script:AT[$i] -band 0x80) { Mark-Dirty ($i % 80) ([int][Math]::Floor($i / 80)) } }
    Flush-Screen
}

# Leave blinking cells blinking after the script exits: only Windows Terminal is known to honour SGR 5.
function Persist-Blink {
    if ($script:Headless -or -not $env:WT_SESSION) { return }
    $fg = 30,34,32,36,31,35,33,37,90,94,92,96,91,95,93,97
    $bg = 40,44,42,46,41,45,43,47
    $e = [string][char]27
    $sb = New-Object System.Text.StringBuilder
    [void]$sb.Append($e + '[s')
    for ($y = 0; $y -lt 25; $y++) {
        $x = 0
        while ($x -lt 80) {
            $i = $y * 80 + $x
            if (-not ($script:AT[$i] -band 0x80)) { $x++; continue }
            $a = $script:AT[$i]; $run = ''
            while ($x -lt 80 -and $script:AT[$y * 80 + $x] -eq $a) { $run += [string][char]$script:CP437[$script:CB[$y * 80 + $x]]; $x++ }
            [void]$sb.Append($e + '[' + ($y + 1 + $script:OY) + ';' + ($x - $run.Length + 1 + $script:OX) + 'H' + $e + '[5;' + $fg[$a -band 15] + ';' + $bg[($a -shr 4) -band 7] + 'm' + $run + $e + '[0m')
        }
    }
    [void]$sb.Append($e + '[u')
    try { [Console]::Out.Write($sb.ToString()); [Console]::Out.Flush() } catch {}
}

function Place-Cursor {
    if ($script:Headless) { return }
    try {
        $x = $script:wx1 + $script:cx - 2; $y = $script:wy1 + $script:cy - 2
        if ($x -lt 0) { $x = 0 }; if ($x -gt 79) { $x = 79 }; if ($y -lt 0) { $y = 0 }; if ($y -gt 24) { $y = 24 }
        [Console]::SetCursorPosition(($x + $script:OX + $script:WPX), ($y + $script:OY + $script:WPY))
        [Console]::CursorVisible = $script:CurVisible
    } catch {}
}

function Set-CursorVisible([bool]$v) { $script:CurVisible = $v; if (-not $script:Headless) { try { [Console]::CursorVisible = $v } catch {} } }

# ---- conio primitives ------------------------------------------------------
function Window([int]$x1, [int]$y1, [int]$x2, [int]$y2) {
    if ($x1 -lt 1 -or $y1 -lt 1 -or $x2 -gt 80 -or $y2 -gt 25 -or $x1 -gt $x2 -or $y1 -gt $y2) { return }
    $script:wx1 = $x1; $script:wy1 = $y1; $script:wx2 = $x2; $script:wy2 = $y2
    $script:cx = 1; $script:cy = 1
}
function Gotoxy([int]$x, [int]$y) {
    if ($x -ge 1 -and $y -ge 1 -and $x -le ($script:wx2 - $script:wx1 + 1) -and $y -le ($script:wy2 - $script:wy1 + 1)) {
        $script:cx = $x; $script:cy = $y
    }
}
function Textcolor([int]$c) { $script:attr = ($script:attr -band 0x70) -bor ($c -band 0x8F) }
function Textbackground([int]$c) { $script:attr = ($script:attr -band 0x8F) -bor (($c -band 7) -shl 4) }
function Set-Color([int]$fg, [int]$bg) { Textcolor $fg; Textbackground $bg }

function Clrscr {
    for ($y = $script:wy1 - 1; $y -le $script:wy2 - 1; $y++) {
        for ($x = $script:wx1 - 1; $x -le $script:wx2 - 1; $x++) {
            $i = $y * 80 + $x; $script:CB[$i] = 32; $script:AT[$i] = $script:attr
        }
        Mark-Dirty ($script:wx1 - 1) $y; Mark-Dirty ($script:wx2 - 1) $y
    }
    $script:cx = 1; $script:cy = 1
    Flush-Screen
}

function Scroll-Window {
    for ($y = $script:wy1; $y -le $script:wy2 - 1; $y++) {
        for ($x = $script:wx1 - 1; $x -le $script:wx2 - 1; $x++) {
            $script:CB[($y - 1) * 80 + $x] = $script:CB[$y * 80 + $x]
            $script:AT[($y - 1) * 80 + $x] = $script:AT[$y * 80 + $x]
        }
    }
    for ($x = $script:wx1 - 1; $x -le $script:wx2 - 1; $x++) {
        $i = ($script:wy2 - 1) * 80 + $x; $script:CB[$i] = 32; $script:AT[$i] = $script:attr
    }
    for ($y = $script:wy1 - 1; $y -le $script:wy2 - 1; $y++) { Mark-Dirty ($script:wx1 - 1) $y; Mark-Dirty ($script:wx2 - 1) $y }
}

# write one byte-code (CP437) at the cursor, honouring window rules (no flush)
function Put-Code([int]$c) {
    $ww = $script:wx2 - $script:wx1 + 1; $wh = $script:wy2 - $script:wy1 + 1
    switch ($c) {
        7  { Beep-Fx 800 50; return }
        8  { if ($script:cx -gt 1) { $script:cx-- }; return }
        10 { $script:cy++; if ($script:cy -gt $wh) { Scroll-Window; $script:cy = $wh }; return }
        13 { $script:cx = 1; return }
    }
    $x = $script:wx1 + $script:cx - 2; $y = $script:wy1 + $script:cy - 2
    $i = $y * 80 + $x
    $script:CB[$i] = $c; $script:AT[$i] = $script:attr
    Mark-Dirty $x $y
    $script:cx++
    if ($script:cx -gt $ww) {
        $script:cx = 1; $script:cy++
        if ($script:cy -gt $wh) { Scroll-Window; $script:cy = $wh }
    }
}

# cputs / printf: plain text (no colour tokens); '~' is NOT special here
function Cputs([string]$s) {
    foreach ($ch in $s.ToCharArray()) { Put-Code ([int]$ch) }
    Flush-Screen
}
function Putch([int]$c) { Put-Code $c; Flush-Screen }

# Convert a token-encoded game string to its original byte form:
#   @hh = colour code (0x91,hh)   ~ = CR LF   ^hh = raw CP437 byte   @@ ~~ ^^ = literal characters
function Str-Bytes([string]$s) {
    $o = New-Object System.Collections.Generic.List[int]
    $i = 0; $n = $s.Length
    while ($i -lt $n) {
        $c = $s[$i]
        if ($c -eq '@') {
            if ($i + 1 -lt $n -and $s[$i + 1] -eq '@') { $o.Add(64); $i += 2 }
            else { $o.Add(0x91); $o.Add([Convert]::ToInt32($s.Substring($i + 1, 2), 16)); $i += 3 }
        } elseif ($c -eq '~') {
            if ($i + 1 -lt $n -and $s[$i + 1] -eq '~') { $o.Add(126); $i += 2 } else { $o.Add(13); $o.Add(10); $i++ }
        } elseif ($c -eq '^') {
            if ($i + 1 -lt $n -and $s[$i + 1] -eq '^') { $o.Add(94); $i += 2 }
            else { $o.Add([Convert]::ToInt32($s.Substring($i + 1, 2), 16)); $i += 3 }
        } else { $o.Add([int]$c); $i++ }
    }
    return ,$o.ToArray()
}

# cprint on raw bytes: 0x91 vv = colour (vv > 0x0F => vv-0x10), background 0
function Cprint-Bytes($b, [int]$x, [int]$y) {
    if ($x -ne 99) { Gotoxy $x $y }
    $n = $b.Length; $i = 0
    while ($i -lt $n) {
        $c = [int]$b[$i]
        if ($c -eq 0x91 -and $i + 1 -lt $n) {
            $v = [int]$b[$i + 1]; if ($v -gt 15) { $v -= 16 }
            Set-Color $v 0; $i += 2
        } else { if ($c -eq 0) { break }; Put-Code $c; $i++ }
    }
    Flush-Screen
}
function Cprint([string]$s, [int]$x, [int]$y) { Cprint-Bytes (Str-Bytes $s) $x $y }

function Gettext([int]$x1, [int]$y1, [int]$x2, [int]$y2) {
    $r = New-Object 'byte[]' (($x2 - $x1 + 1) * ($y2 - $y1 + 1) * 2); $k = 0
    for ($y = $y1 - 1; $y -le $y2 - 1; $y++) { for ($x = $x1 - 1; $x -le $x2 - 1; $x++) {
        $i = $y * 80 + $x; $r[$k] = [byte]$script:CB[$i]; $r[$k + 1] = [byte]($script:AT[$i] -band 0xFF); $k += 2 } }
    return ,$r
}
function Clreol {
    for ($x = $script:cx; $x -le ($script:wx2 - $script:wx1 + 1); $x++) {
        $i = ($script:wy1 + $script:cy - 2) * 80 + $script:wx1 + $x - 2
        $script:CB[$i] = 32; $script:AT[$i] = $script:attr; Mark-Dirty ($script:wx1 + $x - 2) ($script:wy1 + $script:cy - 2)
    }
    Flush-Screen
}

# blit a screen stored as (char, attr) byte pairs; absolute coords like puttext()
function Puttext([int]$x1, [int]$y1, [int]$x2, [int]$y2, [byte[]]$buf) {
    $k = 0
    for ($y = $y1 - 1; $y -le $y2 - 1; $y++) {
        for ($x = $x1 - 1; $x -le $x2 - 1; $x++) {
            $i = $y * 80 + $x
            $script:CB[$i] = [int]$buf[$k]; $script:AT[$i] = [int]$buf[$k + 1]
            $k += 2
        }
        Mark-Dirty ($x1 - 1) ($y); Mark-Dirty ($x2 - 1) ($y)
    }
    Flush-Screen
}

# ---- keyboard ---------------------------------------------------------------
function Test-Key {
    if ($script:KeyQueue.Count -gt 0) { return $true }
    if ($script:Headless) { return $false }
    try { return [Console]::KeyAvailable } catch { return $false }
}
function Read-Key {         # getch(): char code; extended keys return 0 then the DOS scan code
    Flush-Screen
    if ($script:KeyQueue.Count -gt 0) { return $script:KeyQueue.Dequeue() }
    if ($script:Headless) { throw 'HEADLESS_OUT_OF_KEYS' }
    while (-not [Console]::KeyAvailable) { Blink-Tick; Start-Sleep -Milliseconds 25 }
    $k = [Console]::ReadKey($true)
    if (($k.Modifiers -band [ConsoleModifiers]::Control) -ne 0) {      # Ctrl-L / Ctrl-S whatever the host reports as KeyChar
        if ([string]$k.Key -eq 'L') { return 12 }
        if ([string]$k.Key -eq 'S') { return 19 }
    }
    $c = [int]$k.KeyChar
    if ($c -gt 0 -and $c -lt 256) { return $c }
    if ($k.Key -eq 'Escape') { return 27 }
    $scan = switch ([string]$k.Key) {
        'UpArrow' { 0x48 } 'DownArrow' { 0x50 } 'LeftArrow' { 0x4B } 'RightArrow' { 0x4D }
        'F1' { 0x3B } 'F2' { 0x3C } 'Home' { 0x47 } 'End' { 0x4F } 'PageUp' { 0x49 } 'PageDown' { 0x51 } 'Insert' { 0x52 } 'Delete' { 0x53 }
        default { 0 }
    }
    if ($scan -ne 0) { $script:KeyQueue.Enqueue($scan) }
    return 0
}
function Flush-Keys { if ($script:Headless) { return }; while (Test-Key) { [void](Read-Key) } }

# ---- timing / sound -----------------------------------------------------------
function Delay-Ms([int]$ms) {               # delay(): precise even for a few ms (Start-Sleep rounds up to ~15 ms)
    Flush-Screen
    if ($script:Headless) { return }
    $ms = [int][Math]::Round($ms / $script:RunSpeed)
    if ($ms -le 0) { return }
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($true) {
        $left = $ms - $sw.ElapsedMilliseconds
        if ($left -le 0) { break }
        if ($left -gt 20) { [System.Threading.Thread]::Sleep([int][Math]::Min($left - 16, 60)); Blink-Tick }
        else { [System.Threading.Thread]::SpinWait(200) }
    }
}
$script:RunSpeed = 1.0                      # -Speed: scales every delay, tone and pause

# ---------------------------------------------------------------------------
#  Data recovered from DRUGLORD.EXE (v2.01)
# ---------------------------------------------------------------------------

$script:T = @{
    x1155 = 'Cocaine'
    x115d = 'Crack'
    x1163 = 'Heroin'
    x116a = 'Acid'
    x116f = 'Crystal'
    x1177 = 'Grass'
    x117d = 'Speed'
    x1183 = 'Ludes'
    x1189 = 'None'
    x118e = 'Sat. Nt. Spcl'
    x119c = 'Ruger'
    x11a2 = 'Auto Mag'
    x11ab = 'Uzi'
    x11af = 'AK-47'
    x11b5 = 'Saturday Night Special'
    x11cc = 'Ruger Revolver'
    x11db = 'Auto Mag'
    x11e4 = 'Uzi'
    x11e8 = 'AK-47 Assault Rifle'
    x11fc = '* * *'
    x1202 = 'D R U G   L O R D'
    x1214 = 'Date:'
    x121a = 'Drugs on Hand'
    x1228 = 'Street Prices'
    x1236 = 'Location:'
    x1240 = 'Hold:'
    x1246 = 'Cash:'
    x124c = 'In Bank:'
    x1255 = 'In Debt:'
    x125e = '# of Guns:'
    x1269 = 'Gun Type:'
    x1273 = 'Status Pts:'
    x127f = 'Health:'
    x1287 = 'Chicago'
    x128f = 'Detroit'
    x1297 = 'Las Vegas'
    x12a1 = 'Los Angeles'
    x12ad = 'Miami'
    x12b3 = 'New York'
    x12bc = 'San Diego'
    x12c6 = 'Washington D.C.'
    x12d6 = ' '
    x12d8 = ' '
    x12da = '%c'
    x12dd = ' '
    x12df = ' '
    x12e1 = '%-7s'
    x12e6 = '%-7s'
    x12eb = '%6d'
    x12ef = '%10ld'
    x12f5 = '%-15s'
    x12fb = '%-6d (%d max)'
    x1309 = '$%-10ld'
    x1311 = '$%-10ld'
    x1319 = '$%-10ld'
    x1321 = '(   )'
    x1327 = '%3d'
    x132b = '     '
    x1331 = '%-6d'
    x1336 = '%-13s'
    x133c = '%-6d'
    x1341 = '%-4d'
    x1346 = '%#02d/%#02d/%#02d'
    x1358 = 'v2.01'
    x137d = '%2d. '
    x1383 = '%-20s'
    x1389 = '('
    x138b = '$%d'
    x138f = '@0b)'
    x1393 = '@0d* * *  @19D R U G S  @0d* * *'
    x13b1 = '%1d. '
    x13b7 = '%-15s'
    x13bd = '%1d. '
    x13c3 = '%-15s'
    x13c9 = 'Alive'
    x13cf = 'Dead'
    x13d4 = '%4d            %-12s'
    x13eb = '%s%s'
    x13f0 = ' and '
    x13f6 = ', '
    x13f9 = ', and '
    x1400 = '@1aPress any key to continue'
    x141c = 'Health'
    x1423 = 'Status'
    x142a = '@04The @09cops @04are on your tail!!!'
    x144d = '@0aDo you wanna   @1c1@0a) Go at it alone'
    x1474 = '@1c2@0a) Call a friend (1 status pt.)'
    x1498 = '@1c3@0a) Call a posse (5 status pts.)'
    x14bc = '@0eCall it ^10 '
    x14c9 = '@0cYou don''t have enough points!'
    x14e9 = '%-10d'
    x14ef = '%-15s'
    x14f5 = '%10s:'
    x14fb = 'Friend'
    x1502 = '%6s #%2d:'
    x150c = 'Posse'
    x1512 = '%-15s'
    x1518 = '%6s #%2d'
    x1521 = 'Cop'
    x1525 = '@0fDo ya wanna (@1cF@0f)ight or (@1cR@0f)un? '
    x154e = '@0cFiring'
    x1557 = '@0cPunching'
    x1562 = '@14*  Hit!  *'
    x156f = '@0e- Missed! -'
    x157d = '@0cFiring'
    x1586 = '@14*  Hit!  *'
    x1593 = '@0e- Missed! -'
    x15a1 = '@0bYou take off running... '
    x15bc = '@1eand ya got away!!!'
    x15d1 = '@0band ya didn''t get away!!!'
    x15ed = '@0aYous wasted ''em!!! Way ta go, man!!!'
    x1614 = 'his gun'
    x161c = 'their guns'
    x1627 = 'Do you wanna take %s, man? (Y/N) '
    x1649 = '@0eYou @0cwant@0e ''em, you @0cgot @0e''em!  What a bunch''a @06chumps!'
    x1688 = '@09OK, man, it''s yer call.'
    x16a2 = '@0bYou sell the '
    x16b2 = '%d guns that were on the pigs you wasted for %d bucks!'
    x16e9 = '@0bThis''ll teach ''em not to mess with ya, huh?'
    x1717 = '@0b"Hey man, check dis out!  Word has it on de street dat deres des cops, see?~They musta decided that workin'' for 25 G''s a year ain''t enough, so de''ve~decided ta sell some of de take drugs confiscated on de side! De market is~flooded with '
    x180a = '!"'
    x180d = '@07"Hey, I heard it through de grapevine dat deres some cheap homemade stuff~available from some of the homeboys downtown.  Real primo stuff, dey say!~All I hear on de streets is ''Cheap '
    x18c9 = '!''"'
    x18cd = '@07^da^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^bf~^b3  * * *   @1fE X T R A ! !   E X T R A ! !@07   * * *  ^b3~^b3               @1fRead all about it!!@07               ^b3~^c3^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^b4~^b3  After six months of undercover investigation,  ^b3~^b3  police close in and make a major drug bust!    ^b3~^b3  This will no doubt affect street drug prices.  ^b3~^c0^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^c4^d9'
    x1a7e = '@0d"Dude, I got some news for ya... a factory of one of de cities biggest suppliersof '
    x1ad4 = ' burned down'
    x1ae1 = ' blew up'
    x1aea = 'last night.  I don''t have to tell ya what this''ll on de~street..."'
    x1b2e = '@06Ya come back to yer place tonight and find dat yer sexy main squeeze tought~dat it wouldst be a good idea to use some of yer weed to make brownies!!!~All tings considered, yous take it pretty well.  After all, if ya were mad,~you would''ve @07chopped @06up her body @0cbefore @06dumping it in de river!'
    x1c5d = '@0cYer walking along de street at night, when from outta nowhere jumps dis goon!~He waves a 9mm Beretta at yous and demands yer fork over all yer money.'
    x1cf6 = '@0eDo ya wanna use yer status as a major drug dealer to get outta this? (Y/N) '
    x1d44 = '@0cYous tell him whos he''s dealin'' wit.  He looks nervous, asks fer yer pardon,den runs off down an alley!'
    x1dae = '@0b"This is all you got? &%&*! this, man!"'
    x1dd8 = 'He looks at ya like a crazed animal, and pulls de trigger!'
    x1e13 = '@0e* * *   @1cB L A M   @0e* * *'
    x1e31 = '@0dHe laughs merrily as he takes yer cash and runs off down de street!'
    x1e77 = '@0aYou are approached by a shady-lookin guy in a long dark trenchcoat.  He says:'
    x1ec7 = '@09"Hey, bud, I got a @1especial @09trenchcoat for ya, just your style!'
    x1f0c = 'It''s got a capacity of '
    x1f24 = '%d'
    x1f27 = '@09 and I won''t take under '
    x1f42 = '$%ld'
    x1f47 = '@09 for it."'
    x1f53 = '@0eIs it a deal? (Y/N) ^10 '
    x1f6c = '@09"Sorry Charly, I don''t take credit! Maybe when you''re a little more solvent!"'
    x1fbc = '@09"You bought yourself one hell of a trenchcoat!"'
    x1fee = '@09"OK, it''s your loss, man.  What a shame!"'
    x201a = '@0dJuan, one of the most powerful underground gun smugglers in the world,~comes to you with an offer:'
    x2080 = '@07"I have a shipment of '
    x2099 = '%s''s'
    x209e = '@07 coming in tonight.  I''m'
    x20b9 = 'distributing them in shipments of '
    x20dc = '%d '
    x20e0 = '@07at '
    x20e6 = '$%d '
    x20eb = '@07each, for a total of '
    x2103 = '$%d'
    x2107 = '."'
    x210a = '@0dAre you interested in an order? (Y/N) ^10 '
    x2135 = '@07"I''m sorry, sir, but I only accept cash @0fup-front@07.  Business is business."'
    x2185 = '@07"Very well, they will be delivered by tomorrow.  Good doing business with you."'
    x21d7 = '@07"As you wish.  Perhaps next time we may do business."'
    x220f = '@0aYous get a note in de mail from a business associate of yers.  He says dat~if yous don''t deposit '
    x2274 = '$%ld '
    x227a = '@0ain his Swiss bank account, he''s gonna @16rat @0ato de~cops everything dat he knows about you.'
    x22d9 = '@0dDo you pay the blackmail? (Y/N) ^10 '
    x22fe = '@0dYou wire de money into his account.  That really @1c^^$&?*@@ @0dsucks!!!'
    x2345 = '@0cYeah, man, nobody pushes you around! I respect yer integrity!!!'
    x23c0 = '@0cWould ya like ta'
    x23d3 = 'play again? (Y/N)'
    x23e5 = 'Loser!'
    x23ec = 'Dopehead'
    x23f5 = 'Peddler'
    x23fd = 'Pusher'
    x2404 = 'Gangster'
    x240d = 'Dealer'
    x2414 = 'Trafficker'
    x241f = 'Racketeer'
    x2429 = 'Marketeer'
    x2433 = 'Entrepreneur'
    x2440 = 'DRUGLORD'
    x2449 = 'OVERLORD'
    x2452 = '%'
    x2454 = '''s:@1e            '
    x2466 = '^c9^cd^bb^ba^bc^cd^c8^ba'
    x246f = ' @1c^19 '
    x2475 = '@09[@1bESC@09] @1bFinished '
    x248d = ' @0c^18 '
    x2493 = '@09^cd^cd^cd'
    x2499 = ' @1c^19 '
    x249f = '@09^cd^cd^cd'
    x24d5 = '@01^10@09^10@0b^10   @1eUNREGISTERED VERSION   @0b^11@09^11@01^11'
    x2504 = '@0cWritten by Phil Erwin'
    x251c = 'Copyright 1991-93  FantasyWare Inc.'
    x2540 = '^b1'
    x2542 = '^c9^cd^bb^ba^bc^cd^c8^ba'
    x254b = 'Initializing...'
    x255b = 'DRUGLORD requires 80 column mode.~'
    x257f = '@0a(@1cQ@0a)uit de game'
    x2594 = '@0a(@1cB@0a)uy Drugs,                       (@1cV@0a)isit de Bank,~(@1cS@0a)ell Drugs,                      Go to de (@1cH@0a)ospital,~(@1cJ@0a)et to another city,             See de (@1cL@0a)oan Shark'
    x2653 = '@07Here''s a list of yer possible plans, dude:'
    x2680 = '@07Make up your mind @09^10 '
    x2699 = '@0aWhere ya wanna go? '
    x26af = '@0dAre you having @1fflashbacks? @0dThat''s where you are now!'
    x26ea = '@1cStart flappin'' yer arms, cause you ain''t got de dough!'
    x2723 = '@04What kinda drugs are~ya interested in? '
    x274e = '@04How many do ya wanna buy?~('
    x276d = '%ld'
    x2771 = '@04 max.)'
    x277a = '@1dHey man, if ya don''t got enough~  cash, don''t come ''round here!'
    x27bd = '@0aDude, you can''t hold that much!~  Everyone''s got a limit, right?'
    x2801 = '@0e* * *   @1fS O L D ! ! !   @0e* * *'
    x2825 = '@04What kinda drugs do~ ya want to unload? '
    x2851 = '@0dGive me a break, man, you ain''t got~any of that!'
    x2885 = '@04How many do ya wanna sell? '
    x28a3 = '@15I happen to know that you ain''t that~much on ya, so quit trying to burn me!'
    x28f2 = '@1eYou got yerself a deal, dude!'
    x2912 = '@0fYou arrive at the First World Bank of America.~^0a               @0aWould ya like to (@1cD@0a)eposit  some moola,~                             or (@1cW@0a)ithdraw some moola? '
    x29bf = '@0bHow much ''ya gonna rathole? '
    x29de = '@0bHow much ''ya gonna pull out? '
    x29fe = '@1dYou ain''t got that much to deposit!'
    x2a24 = '@1eDone!'
    x2a2c = '@1dYou ain''t got that much to pull out!'
    x2a53 = '@1eDone!'
    x2a5b = '@0bYa set foot in the ''sharks office.  You''ve been in locker rooms that smelled~better than dis place!  His boys are lookin'' tough in their nice goon suits,~but you don''t pay dem no never-mind, cause the head-honcho, Guido is talkin...~^0a                    @0aHe say, you wanna (@1cB@0a)orrow some cash,~                      or do you wanna (@1cP@0a)ay me back? '
    x2bc5 = '@1a"Man, yer pushin'' yer luck luck! I ought''ta kill you just for askin''!!"'
    x2c0f = '@0a"Look, whoever you are, you''re @1fNOBODY @0ato me.  Get outta here!"'
    x2c54 = '@0aHow much ya wanna borrow? '
    x2c71 = '@1c"Sorry, bud, but I got a @1e$1000 @1cminimum.  Got no time for penny-ante stuff!"'
    x2cc3 = '@1c"No deal, too big a risk for me! I''m a businessman, ya''know!"'
    x2d03 = '@0b"Look, you already owe me '
    x2d20 = '%ld '
    x2d25 = 'bucks, I''m not givin ya THAT much more!!!'
    x2d4f = '@0bChrist, what do I look like, Santi-Claus?"'
    x2d7c = '@0d"Alright, I''ll loan ya de money, but if you welch, my boys will come lookin~for ya! I expect to see you here to pay me in full in '
    x2e02 = '%d'
    x2e05 = '@0d days."'
    x2e0f = '@0b"My books show dat you don''t owe me nut''in... would youse like to take out~a little loan?"'
    x2e6d = '@0aHow much ya wanna pay back? '
    x2e8c = '@1c''fraid you don''t got that in cash, my man.'
    x2eb9 = '@0b"OK, but you still owe me '
    x2ed6 = '%ld '
    x2edb = '@0bbuck'
    x2ee2 = 's!"'
    x2ee6 = '!"'
    x2ee9 = '@0b"Paid in full? Lucky for you, eh boys? Anytime yous need a loan, just come~to me, I''ll take care of ya..."'
    x2f57 = '@0fYa have de doc check yous out, but aside from yer bunions~              on yer feet, youse is fit as a fiddle!'
    x2fc9 = '@0fYou drag yer sorry butt into de loc''l Hospital.  The docs check ye~   out, and come to the conclusion that it''ll cost '
    x3043 = '%d'
    x3046 = '@0f dollars to fix ya up.'
    x305f = '@0dDo ya sign the consent forms? (Y/N) ^10 '
    x3088 = '@1cYou don''t have enough dinero up front to foot de bill!'
    x30c1 = '@0eOK, yer laid up for '
    x30d8 = '%d'
    x30db = '@0e days recoup''ratin'', and yous feel good as new!'
    x310d = '@07Are ya sure ya wanna @16chicken out @07an'' @14QUIT? @07(Y/N) '
    x3149 = '@0bYou''re ambushed in a dark alley by Guido''s thugs, must be ''bout ten guys!~The boss-man say @17"Oh, man, you should''na missed de deadline, now me and~boys are gonna have to break your legs! Maybe next time you''ll learn, eh?"@0b~Dey take yous from all directions, you ain''t gotta chance...~One steps out with a @16baseball bat @0band...~'
    x329c = '@0c* * *    @14C R A C K ! !   @0c* * *'
    x32c1 = '@0e"Oh, one more ''ting... you got a week to come up with dinero, or we''ll~ be back, comprende?"'
    x3321 = '@1eThank you for playing DrugLord.'
    x3343 = 'Send For The Registered Version Today!'
    x136e = '@05Drug Cities:'
    x24c5 = '@0aVersion v2.01'
}

$script:ScrDef = @{
    main = @{ W=80; H=16; Data='u8p+dhDDHeyb2RX4By/czH4M6lIFAu4kNwQuUhR+W2kefpSZT2T4sWGGBf7wRg4BytxHiv/ICYvNlLsPAomzi2i1o+4bdd+o+0bdNwTcN6jL5ysk1Nbnyajhz1PUPtjHDgA=' }
    fight = @{ W=80; H=22; Data='O8J+ZFBDhUEOqevbQ4M8/DYP8tgYdd/gcx++1EhcmkWIb6ayeUMtfiPZ89lL2a1G09+o+0bdN+q+UfeNum/UfTR1H3Vb4wep3r4HAA==' }
    final = @{ W=80; H=25; Data='7ZVBDoIwEABvi0kT38CP/EJPXkj6NlFPXjTqJ9SDH+gjhNKWtmwrxRY9mEkA6bKZ7G7WkpTJuMNVczaejyDOi2hIWp7FFuVZZPOL+moHuN8ubf2KqX4Xj99+il9czVoWP1I/8ibbvH4QAP/KG3/y+N1C2ULAG2LjDQ6B6Jq6BHKTPKz8c9D4baCDL9tr4zcu61IDjwyUjLOSmb+49caKBTl7iJ84XxswQTUJ1qN8Ok+uaaKoQtp56yfPqZn3Iz/tqG2cu3i27ZSfUz/tRpP6oXZDQ9XZGqseWG6YX/UJmJHsdXtOzcmr3akbug39qjRw5tK7qf7VSE+/jeli+f2Mm8dPvLP3o4yiWfZ2s4276fL38u83h1+m/+URfvZWzlirIIFdTML1+/vF+s1vN37KXw==' }
    fire = @{ W=8; H=3; Data='u8e+AQ7vAyGIfsmuAIcgPpBmA0N2BXYA' }
    tomb = @{ W=35; H=15; Data='rVNBboMwEDwksoXW+A37gIo/9NBD77306MQOoBCQwGnL19I+oD30Tx3boYVjFbTy7rJ4Zmdt+Mq/b7b3/JIz3WqXwJI/0As56mkkTxXV1FKJdxUZGhAN1qC97rXRrS6106xPutahNmre8CaxNNTBjtjtERk8nopo/9Cy0kTPspNnydLIXjrEQXrkrbSyhi9RaVF/Q5WxOvgxInpkwRKLFU4YYQWLnegQR1EgZ3EPvgnB4HRyD14bO73KCp38jCXx2+inrnczfNLWz7QcsMvJZqGlUk4Z5RWrc5zDRa1/qhPGyyJinqDCLaactFS/JzD1P+K5g+6lJhfxB9Wg6wm9B8Vb3l5vGhx74CYmA63N9bQDPuiba9shs3gu5hOtdNPJHsFaopuD8iGeY5gzZMn7rMpcxlhtZjPoWP2r+1zhn/7IfwA=' }
    bar = @{ W=5; H=1; Data='28S0iUlFbxPTJiYA' }
    title = @{ W=62; H=6; Data='jVHJDcAwCPsyBvv/mKdIWaWVSgolhkR+WI6JnUNIDEr8QALUdGbkc7kb66kuGsaV5sV/4evevU+bGmXLN1N157P/u0/Tqnvl7B3X3XXayT19otcK3e4dRvg9Np05TvQauTiN6QY=' }
    exit = @{ W=75; H=19; Data='tZZfTttAEMal3dhSFHl9hRVCQippACGqljdEAk3VEgSpeDa2k1g4dmpvSNNDcJkcAB64BJym344N+dMUHuJotLszs6v9fpnZBJ5Lj4XafUnCniuPK9h9ZVKSdpE2IapJRcLaRmxI48Lwja4RGKmh4CXI1MsX5Z/l0/L3cgtevVxFLsVeZHjwpDFZoPpwJ+9ehrYzjBZGG6OJcYzRyPdmzv6HatPes3ftGsYudraht2nvv8aX9o79FasC+6Exd8f6arXKHZXsFllq8x4PeMol73KH97kPL4sDHnGFOOIeZg+ZFLk+H/CQjxE5+tTSWmk7EY6IhMKcirG4wpoIX1SFFE3kXVETb1E5uN+D2i2UE9KNwSF5B1EMBol8tjfGOYd4QpzqIFd7g8oVOyIGw7noiUCE8BrgGsGPxPu1ahJDBALJR7mqQzRNDJcYPZp17brIxSB8+7VrOxBfxCexC5or1ChF1aRogUtXsIuMFHVEgbglP7Mple6Q1h5Cy4Hqi7aHeJh7PnbCeaqZ/cwWqU5RHx8EHjjCvHdHxPEHNYyQleKzOBD7IN/7h2qIrvn5K9JViqhaWdWq9JZ68Ac4U30l6tDaX97BVV/72n6vzs3Q9E3HTDFLsw/vhjzX7GF1EaWIBsiPMa7ptITNfgf1i/qFbsT0jZt95w7Vz+e/0WMfPQzpG6nyHYX6flzaQWXG0DgHQwBFaTbMxBzBj8waqV+CQnMoolz22qfvow+WWyhlvTqzfGtkSeub5ViudQPv2AosZY2tQ/5erVr2hV23G5hr9gnmH9jvEKFm6YA5weohUqiVzqfEu64OPqz0d/Apr9UGa7Me85lkHkvYkHXh+WzAAsQ+62N1kQlYyiK2xRR8B+OahVhddsPmqAr7bLKkQBVAs8uqpK+WUIzojCL66fkaW7ivQKomdLZAkenrCjmkvKAPO4LvzZ9ZG5XHYqhMa+NCMaGq6Ao5eS5Gz2LkJRtjHdJn0KdqbIOIJSuMqujXXsAtDwX/L/pUCNVf' }
}

$script:ReportB64 = 'ICAgICAgICAgICCRHENPTkdSQVRVTEFUSU9OUyEhISAgICAgICAgIAAAAAAAAAAAAAAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAAAAAAAAAAAAAAAAJEOWW91cyBoYXMgYmVjb21lIGEgkR1EUlVHTE9SRCEhIZEOICBJJ20gICAgAAAAAAAAcHJvdWQgdGEga25vdyB5b3UsIG1hbiEgIEFueXdheSwgY2hlY2sAAAAAAAAAAAAAAABkaXMgb3V0IC0tICAgICAgICAgICAgICAgICAgICAgICAgICAgIAAAAAAAAAAAAAAAACAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgAAAAAAAAAAAAAAAAkQsgICAgICAgICAgIENhc2ggaW4gSGFuZDqRHiAgICAgICAgICAgICAgAAAAAAAAAACRCyAgICAgICAgICAgQ2FzaCBpbiBCYW5rOpEeICAgICAgICAgICAgICAAAAAAAAAAAJELICAgICAgICAgIFN0aWxsIGluIERlYnQ6kR4gICAgICAgICAgICAgIAAAAAAAAAAAkQsgICBOdW1iZXIgb2YgR29vZCBEZWFsczqRHiAgICAgICAgICAgICAgAAAAAAAAAACRCyAgTnVtYmVyIG9mIFRvdGFsIERlYWxzOpEeICAgICAgICAgICAgICAAAAAAAAAAAJELICAgICAgICAgICAgICAgICBERUFMICU6kR4gICAgICAgICAgICAgIAAAAAAAAAAAkQsgICAgICAgICAgICAgICAgICAgICAgICCRHiAgICAgICAgICAgICAgAAAAAAAAAACRCyAgICAgICAgICAgICBUb3RhbCBEYXlzOpEeICAgICAgICAgICAgICAAAAAAAAAAAJELICAgICAgICAgICAgICAgICAgICAgICAgkR4gICAgICAgICAgICAgIAAAAAAAAAAAkQ8tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0gICAgIC0tLS0tLS0tIAAAAAAAAAAAAACRDCAgICAgICAgICBPVkVSQUxMIFNDT1JFOpEeICAgICAgICAgICAgICAAAAAAAAAAAJEHICAgICAgICAgICAgICAgICAgIFJBTks6kR4gICAgICAgICAgICAgIAAAAAAAAAAA'

$script:Mult = @(
    @(0.949999988079071, 0.8999999761581421, 0.8999999761581421, 1.0, 1.0499999523162842, 0.949999988079071, 1.100000023841858, 1.0499999523162842),
    @(1.100000023841858, 0.8999999761581421, 0.949999988079071, 0.949999988079071, 1.100000023841858, 1.100000023841858, 1.0, 0.8999999761581421),
    @(1.0, 1.100000023841858, 0.8999999761581421, 1.0499999523162842, 1.0499999523162842, 0.8999999761581421, 0.8999999761581421, 1.100000023841858),
    @(1.100000023841858, 1.0499999523162842, 0.949999988079071, 0.8999999761581421, 1.0499999523162842, 1.0499999523162842, 0.8999999761581421, 1.0),
    @(0.8999999761581421, 0.949999988079071, 1.100000023841858, 1.0499999523162842, 0.8999999761581421, 1.100000023841858, 1.0499999523162842, 0.949999988079071),
    @(0.8999999761581421, 0.949999988079071, 1.100000023841858, 1.100000023841858, 0.949999988079071, 1.0, 1.0, 1.0),
    @(1.0499999523162842, 0.949999988079071, 1.0, 1.0499999523162842, 0.8999999761581421, 1.0, 0.949999988079071, 1.100000023841858),
    @(0.949999988079071, 1.100000023841858, 1.0499999523162842, 1.0499999523162842, 0.949999988079071, 0.8999999761581421, 1.100000023841858, 0.8999999761581421)
)
$script:PriceLo = @(40000,25000,10000,2000,600,200,70,10)
$script:PriceHi = @(50000,30000,15000,3000,1000,400,100,20)
$script:RankThr = @(250000,500000,600000,700000,800000,850000,900000,950000,1000000,1100000,1250000)
$script:DaysInMonth = @(31,28,31,30,31,30,31,31,30,31,30,31)
$script:GunDice = @(0,1,2,4,2,4)
$script:CityXY = @(@(3,2),@(3,1),@(1,2),@(0,3),@(5,4),@(5,2),@(0,3),@(5,2))
$script:Freq = @(100,150,500,400,100,75,25,500,1000,1500,1000,750,500,200,100,50)


# ---------------------------------------------------------------------------
#  Runtime helpers (Borland C behaviour)
# ---------------------------------------------------------------------------
$script:Seed = [long]([Environment]::TickCount -band 0xFFFF)
function Rand {                      # Borland rand(): 32-bit LCG, returns bits 16..30
    $script:Seed = (($script:Seed * 22695477 + 1) -band 4294967295L)
    return [int](($script:Seed -shr 16) -band 0x7FFF)
}
function IDiv($a, $b) { return [long][Math]::Truncate([double]$a / [double]$b) }
function Dice([int]$sides, [int]$count) {      # f_1182: sum of 'count' rolls of 1..sides
    $s = 0
    for ($i = 0; $i -lt $count; $i++) { $s += (Rand) % $sides + 1 }
    return $s
}
function Ftol([double]$x) { return [long][Math]::Truncate($x) }
function Atol([string]$s) {          # C atol()
    $m = [regex]::Match($s, '^\s*([+-]?\d+)')
    if (-not $m.Success) { return [long]0 }
    $v = [decimal]0
    if (-not [decimal]::TryParse($m.Groups[1].Value, [ref]$v)) { return [long]0 }
    return [long][Math]::Truncate($v)
}
function F32([double]$x) { return [double][single]$x }      # value as stored in a 4-byte float

function Pad-R([string]$s, [int]$w) { if ($s.Length -ge $w) { return $s } else { return $s + (' ' * ($w - $s.Length)) } }
function Pad-L([string]$s, [int]$w) { if ($s.Length -ge $w) { return $s } else { return (' ' * ($w - $s.Length)) + $s } }

# ---------------------------------------------------------------------------
#  Sound effects (PC speaker): sound(f); delay(ms)  ->  Beep
# ---------------------------------------------------------------------------
function Sfx-Body([int]$id) {
    switch ($id) {
        2  { Tone 2000 10; Tone 5000 20; Tone 500 30; Tone 1000 40 }
        3  { for ($si = 500; $si -gt 200; $si -= 100) { Tone $si 100; Tone ($si - 200) 100; Rest-Ms 50 } }
        4  { for ($si = 500; $si -lt 1000; $si += 100) { Tone $si 10; Tone ($si + 2000) 20; Rest-Ms 10 } }
        5  { for ($k = 0; $k -lt 6; $k++) {
                 for ($si = 7; $si -lt 15; $si++) { Tone ($si * 100) 25 }
                 for ($si = 15; $si -gt 7; $si--) { Tone ($si * 100) 25 } } }
        6  { for ($si = 0; $si -lt 5; $si++) { for ($di = 0; $di -lt 10; $di++) { Tone ($si * 1000 + $di * 1000) 40 } } }
        7  { for ($si = 100; $si -gt 0; $si--) { Tone (5000 - $si * 50) 5; Tone ($si * 50 + 5000) 5 } }
        8  { Tone 2000 30; Tone 500 25; Tone 70 40 }
        11 { Tone 1000 50 }
        12 { Tone 500 30; Tone 200 20 }
        13 { Tone 100 30; Tone 50 30 }
        14 { Tone 150 30; Tone 100 50 }
        15 { for ($si = 1; $si -lt 3; $si++) { Tone 100 20; Tone 50 60 } }
        20 { Tone 800 100 }
        default { }
    }
}

# ---------------------------------------------------------------------------
#  Screens
# ---------------------------------------------------------------------------
$script:Scr = @{}
foreach ($k in $script:ScrDef.Keys) {
    $def = $script:ScrDef[$k]
    $ms = New-Object System.IO.MemoryStream (, [Convert]::FromBase64String($def.Data))
    $ds = New-Object System.IO.Compression.DeflateStream($ms, [System.IO.Compression.CompressionMode]::Decompress)
    $out = New-Object System.IO.MemoryStream
    $ds.CopyTo($out); $ds.Dispose()
    $script:Scr[$k] = $out.ToArray()
}

# ---------------------------------------------------------------------------
#  Game state
# ---------------------------------------------------------------------------
$script:DrugName = @('Cocaine', 'Crack', 'Heroin', 'Acid', 'Crystal', 'Grass', 'Speed', 'Ludes')
$script:GunName  = @('None', 'Sat. Nt. Spcl', 'Ruger', 'Auto Mag', 'Uzi', 'AK-47')
$script:GunLong  = @('Saturday Night Special', 'Ruger Revolver', 'Auto Mag', 'Uzi', 'AK-47 Assault Rifle')
$script:CityName = @('Chicago', 'Detroit', 'Las Vegas', 'Los Angeles', 'Miami', 'New York', 'San Diego', 'Washington D.C.')
$script:RankName = @('x23e5', 'x23ec', 'x23f5', 'x23fd', 'x2404', 'x240d', 'x2414', 'x241f', 'x2429', 'x2433', 'x2440', 'x2449')

[long]$script:cash = 0; [long]$script:bank = 0; [long]$script:debt = 0
$script:city = 0; $script:month = 1; $script:day = 1; $script:year = 86
$script:hold = 10; $script:free = 0
$script:inv = New-Object 'int[]' 8
$script:basis = New-Object 'long[]' 8
[long[]]$script:price = New-Object 'long[]' 8
$script:offs = New-Object 'double[]' 8
$script:numGuns = 0; $script:gunType = 0; $script:status = 1; $script:health = 100
$script:dead = $false; $script:loanFlag = 0; $script:loanDays = 0; $script:angry = 0
$script:thugs = 0; $script:copFlag = 0
$script:done = New-Object 'bool[]' 10
$script:mktToday = 0; $script:totalDays = 0
$script:goodDeals = 0; $script:totalDeals = 0           # (the original never resets these between games)
$script:Buf = ''; $script:EscPressed = $false

# ---------------------------------------------------------------------------
#  Small UI helpers
# ---------------------------------------------------------------------------
function Frame([int]$x1, [int]$y1, [int]$x2, [int]$y2) {      # f_814: clear a rectangle, back to full-screen window
    Window $x1 $y1 $x2 $y2
    Clrscr
    Window 1 1 80 25
}
function Box([string]$chars, [int]$x1, [int]$y1, [int]$x2, [int]$y2) {     # f_5ca: single/double frame from 8 chars
    $c = Str-Bytes $chars
    Gotoxy $x1 $y1; Putch $c[0]
    Gotoxy $x2 $y1; Putch $c[2]
    Gotoxy $x2 $y2; Putch $c[4]
    Gotoxy $x1 $y2; Putch $c[6]
    Gotoxy ($x1 + 1) $y1; for ($i = $x1 + 1; $i -lt $x2; $i++) { Put-Code $c[1] }; Flush-Screen
    Gotoxy ($x1 + 1) $y2; for ($i = $x1 + 1; $i -lt $x2; $i++) { Put-Code $c[5] }; Flush-Screen
    for ($y = $y1 + 1; $y -lt $y2; $y++) {
        Gotoxy $x1 $y; Putch $c[3]
        Gotoxy $x2 $y; Putch $c[7]
    }
}
function Cursor([int]$on) { Set-CursorVisible ($on -ne 0) }        # f_2f1

function Press-AnyKey {              # f_1406: animated "Press any key to continue"
    $spin = @(0xF9, 0x09, 0x4F)
    Cursor 0
    Flush-Keys
    Cprint $script:T.x1400 27 25
    Set-Color 14 0
    do {
        for ($si = 0; $si -lt 3; $si++) {
            Gotoxy 25 25; Putch $spin[$si]
            Gotoxy 53 25; Putch $spin[$si]
            Delay-Ms 100
        }
        if ($script:Headless -and $script:KeyQueue.Count -eq 0) { throw 'HEADLESS_OUT_OF_KEYS' }
    } while (-not (Test-Key))
    if ($script:Headless) { [void]$script:KeyQueue.Dequeue() }      # (test mode: the key is consumed by the flush that follows)
    Frame 25 25 54 25
}

# input line (f_6d5). Returns $true when ESC was pressed. Text in $script:Buf.
function Read-Field([int]$width, [int]$flag) {
    $sx = $script:cx; $sy = $script:cy
    $buf = New-Object 'int[]' 64
    $si = 0
    Cursor 1
    Cputs (' ' * $width)
    while ($true) {
        if ($width - 1 -gt $si) { Gotoxy ($sx + $si) $sy; Cputs ' ' }
        Gotoxy ($sx + $si) $sy
        $c = Read-Key
        if ($c -eq 0) { Flush-Keys; continue }
        if ($c -eq 27) { $script:Buf = ''; $script:EscPressed = $true; return $true }
        if ($c -eq 13) { $buf[$si] = 0; break }
        if ($c -eq 8) {
            if ($si -gt 0) { $buf[$si] = 0; $si-- }
            $buf[$si] = 0
            Cputs ' '
            continue
        }
        $buf[$si] = $c
        Gotoxy ($sx + $si) $sy
        Putch $c
        if ($width - 1 -gt $si) { Cputs ' ' }
        if ($width - 1 -eq $si) {
            if ($flag -eq 0) { break }
            Sfx 2
            Gotoxy ($sx + $si) $sy
            Cputs ' '
            $si--
        }
        $si++
    }
    $buf[$si + 1] = 0
    if ($flag -eq 0 -and $width -gt 1) { $buf[$si] = 0 }
    $t = ''
    for ($i = 0; $i -lt 64 -and $buf[$i] -ne 0; $i++) { $t += [string][char]$buf[$i] }
    $script:Buf = $t
    $script:EscPressed = $false
    return $false
}

function Ask-Key([string]$allowed) {             # getch loop: toupper, only accept listed chars (ESC = 27)
    while ($true) {
        $c = Read-Key
        if ($c -ge 97 -and $c -le 122) { $c -= 32 }
        if ($c -gt 0 -and $c -lt 256 -and $allowed.IndexOf([char]$c) -ge 0) { return $c }
        if ($allowed.IndexOf([char]27) -ge 0 -and $c -eq 27) { return 27 }
    }
}

# ---------------------------------------------------------------------------
#  Prices / travel
# ---------------------------------------------------------------------------
function Travel-Cost([int]$a, [int]$b) {          # f_d17
    $dx = $script:CityXY[$b][0] - $script:CityXY[$a][0]
    $dy = $script:CityXY[$b][1] - $script:CityXY[$a][1]
    return (Ftol ([Math]::Sqrt([Math]::Pow($dx, 2) + [Math]::Pow($dy, 2)) * 60.0 + 25.0))
}
function Gen-Prices {                              # f_edd
    for ($d = 0; $d -lt 8; $d++) {
        $r = [long](Rand) % ([long]($script:PriceHi[$d] - $script:PriceLo[$d]))
        $v = ([double]($r + $script:PriceLo[$d])) + $script:offs[$d]
        $script:price[$d] = Ftol ($v * (F32 $script:Mult[$script:city][$d]))
    }
}

# ---------------------------------------------------------------------------
#  Screens: main frame, status, menus
# ---------------------------------------------------------------------------
function Draw-Main {                               # f_b22
    Cursor 0
    Set-Color 7 0
    Window 1 1 80 25
    Clrscr
    Puttext 1 1 80 16 $script:Scr.main
    Set-Color 14 0
    Cprint $script:T.x11fc 24 2
    Cprint $script:T.x11fc 52 2
    Set-Color 9 0;  Cprint $script:T.x1202 32 2
    Set-Color 11 0; Cprint $script:T.x1214 6 3
    Set-Color 6 0;  Cprint $script:T.x121a 6 5
    Set-Color 4 0;  Cprint $script:T.x1228 30 5
    Set-Color 6 0;  Cprint $script:T.x1236 53 4; Cprint $script:T.x1240 57 5
    Set-Color 10 0; Cprint $script:T.x1246 57 7; Cprint $script:T.x124c 54 8; Cprint $script:T.x1255 54 9
    Set-Color 13 0; Cprint $script:T.x125e 52 11; Cprint $script:T.x1269 53 12
    Set-Color 11 0; Cprint $script:T.x1273 51 14; Cprint $script:T.x127f 55 15
    Set-Color 11 0
    Cprint $script:T.x1358 74 2
}

function Draw-Status {                             # f_89c
    $script:free = 0
    Cursor 0
    Set-Color 15 0
    for ($i = 0; $i -lt 8; $i++) {
        Gotoxy 5 ($i + 7);  Cputs (Pad-R $script:DrugName[$i] 7)
        Gotoxy 28 ($i + 7); Cputs (Pad-R $script:DrugName[$i] 7)
    }
    for ($i = 0; $i -lt 8; $i++) {
        Gotoxy 16 ($i + 7)
        if ($script:inv[$i] -gt 0) { Set-Color 14 0 } else { Set-Color 7 0 }
        Cputs (Pad-L ([string]$script:inv[$i]) 6)
        Gotoxy 38 ($i + 7)
        Set-Color 12 0
        Cputs (Pad-L ([string]$script:price[$i]) 10)
        $script:free += $script:inv[$i]
    }
    Set-Color 14 0
    $script:free = $script:hold - $script:free
    Gotoxy 63 4;  Cputs (Pad-R $script:CityName[$script:city] 15)
    Gotoxy 63 5;  Cputs ((Pad-R ([string]$script:free) 6) + " ($($script:hold) max)")
    Gotoxy 63 7;  Cputs ('$' + (Pad-R ([string]$script:cash) 10))
    Gotoxy 63 8;  Cputs ('$' + (Pad-R ([string]$script:bank) 10))
    Gotoxy 63 9;  Cputs ('$' + (Pad-R ([string]$script:debt) 10))
    Gotoxy 74 9
    if ($script:loanFlag -eq 1) {
        Cputs '(   )'
        if ($script:loanDays -le 2) { Textcolor 0x8C }
        Gotoxy 75 9; Cputs (Pad-L ([string]$script:loanDays) 3)
        Set-Color 14 0
    } else { Cputs '     ' }
    Gotoxy 63 11; Cputs (Pad-R ([string]$script:numGuns) 6)
    Gotoxy 63 12; Cputs (Pad-R $script:GunName[$script:gunType] 13)
    Gotoxy 63 14; Cputs (Pad-R ([string]$script:status) 6)
    Gotoxy 63 15
    if ($script:health -le 25) { Textcolor 0x84 }
    Cputs (Pad-R ([string]$script:health) 4)
    Set-Color 14 0
    Gotoxy 12 3
    Cputs ('{0:00}/{1:00}/{2:00}' -f $script:month, $script:day, $script:year)
}

function Draw-DrugList {                           # f_11a5
    Cursor 0
    Cprint $script:T.x1393 45 18
    Set-Color 14 0
    Cprint (' ' * 40) 36 19
    for ($si = 0; $si -lt 4; $si++) {
        Gotoxy 43 ($si + 21)
        Set-Color 12 0; Cputs ((Pad-L ([string]($si + 1)) 1) + '. ')
        Set-Color 10 0; Cputs (Pad-R $script:DrugName[$si] 15)
        Gotoxy 60 ($si + 21)
        Set-Color 12 0; Cputs ((Pad-L ([string]($si + 5)) 1) + '. ')
        Set-Color 10 0; Cputs (Pad-R $script:DrugName[$si + 4] 15)
    }
    Cursor 1
}

function Draw-CityMenu {                           # f_dd3
    Cursor 0
    Frame 1 17 80 25
    Cprint $script:T.x136e 34 17
    for ($di = 0; $di -lt 4; $di++) {
        for ($j = 0; $j -lt 2; $j++) {
            $si = $j * 4 + $di
            Gotoxy ($j * 35 + 10) ($di + 19)
            Set-Color 12 0; Cputs ((Pad-L ([string]($si + 1)) 2) + '. ')
            Set-Color 11 0; Cputs (Pad-R $script:CityName[$si] 20)
            if ($script:city -ne $si) {
                Cputs '('
                Set-Color 2 0
                Cputs ('$' + (Travel-Cost $script:city $si))
                Cprint '@0b)' 99 99
            }
        }
    }
}

function Draw-HP([int]$hp, [int]$row, [int]$side) {         # f_12a9
    if ($side -eq 0) { Gotoxy 12 ($row + 10) } else { Gotoxy 52 ($row + 11) }
    if ($hp -gt 0) { Set-Color 15 0; $st = 'Alive' } else { Set-Color 7 0; $st = 'Dead' }
    Cputs ((Pad-L ([string]$hp) 4) + (' ' * 12) + (Pad-R $st 12))
}

# f_1314: drop the price of 1..n distinct drugs; prints their names
function Market-Event([int]$count, [int]$range, [double]$mult) {
    $n = (Rand) % $count + 1
    $used = New-Object 'bool[]' 8
    $sep = ''
    for ($k = 0; $k -lt $n; $k++) {
        do { $d = (Rand) % 8 } while ($used[$d])
        Cputs ($sep + $script:DrugName[$d])
        if ($n -eq 2) { $sep = ' and ' } else { $sep = ', ' }
        $used[$d] = $true
        $f = [double][int]([Math]::Truncate(((Rand) % $range) / 100.0)) + $mult
        $script:price[$d] = Ftol ([double]$script:price[$d] * $f)
        if ($n -gt 2 -and $k + 2 -eq $n) { $sep = ', and ' }
    }
    $script:mktToday = 1
}

# one day passes (f_3215): loan clock, calendar, interest
function Next-Day {
    $script:loanDays--
    $script:day++
    $script:totalDays++
    if ($script:DaysInMonth[$script:month - 1] -lt $script:day) {
        $script:day = 1
        $script:month++
        if ($script:month -gt 12) {
            $script:month = 1
            $script:year++
            if ($script:year -eq 100) { $script:year = 0 }
        }
    }
    $script:debt = Ftol ([double]$script:debt * 1.05)
    $script:bank = Ftol ([double]$script:bank * 1.000192)
}

# ---------------------------------------------------------------------------
#  PC-speaker emulation.  Each sound effect is rendered once into a WAV in memory
#  (continuous square wave, exact lengths, no gaps) and played with SoundPlayer.
#  -SoundMode Beep falls back to Console.Beep (one call per tone).
# ---------------------------------------------------------------------------
$script:SfxMode = 'Wave'; $script:WaveName = 'Square'; $script:SfxDuty = 0.5
$script:SfxVol = 0.35; $script:SfxPitch = 1.0; $script:SfxStretch = 1.0
$script:SynthOK = $false; $script:SfxCache = @{}
$script:JetSpeed = 2.0
$script:Rec = $false; $script:RecF = $null; $script:RecM = $null

function Init-Synth {
    if ($script:SynthOK) { return }
    try {
        Add-Type -TypeDefinition @'
using System;
using System.IO;
public static class DLSynth {
    public static byte[] Render(double[] f, double[] ms, int rate, double vol, int wave, double duty) {
        int[] n = new int[ms.Length]; int total = 0;
        for (int i = 0; i < ms.Length; i++) { n[i] = (int)Math.Round(ms[i] * rate / 1000.0); if (n[i] < 1) n[i] = 1; total += n[i]; }
        short[] s = new short[total];
        int p = 0; double ph = 0, amp = vol * 32767.0;
        for (int i = 0; i < f.Length; i++) {
            if (f[i] <= 0) { p += n[i]; ph = 0; continue; }
            double step = f[i] / rate;
            for (int k = 0; k < n[i]; k++) {
                double v;
                switch (wave) {
                    case 1: v = Math.Sin(2 * Math.PI * ph); break;
                    case 2: v = 2 * ph - 1; break;
                    case 3: v = ph < 0.5 ? 4 * ph - 1 : 3 - 4 * ph; break;
                    default: v = ph < duty ? 1 : -1; break;
                }
                s[p++] = (short)(v * amp);
                ph += step; if (ph >= 1) ph -= 1;
            }
        }
        MemoryStream ms2 = new MemoryStream();
        BinaryWriter w = new BinaryWriter(ms2);
        int dataLen = total * 2;
        w.Write(new char[] { 'R', 'I', 'F', 'F' }); w.Write(36 + dataLen);
        w.Write(new char[] { 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ' }); w.Write(16);
        w.Write((short)1); w.Write((short)1); w.Write(rate); w.Write(rate * 2); w.Write((short)2); w.Write((short)16);
        w.Write(new char[] { 'd', 'a', 't', 'a' }); w.Write(dataLen);
        for (int i = 0; i < total; i++) w.Write(s[i]);
        w.Flush();
        return ms2.ToArray();
    }
}
'@
        $script:SynthOK = $true
    } catch { $script:SynthOK = $false }
}

function Clear-SoundCache { $script:SfxCache = @{} }

function Render-Wav($f, $m) {
    $w = 0
    switch ($script:WaveName) { 'Sine' { $w = 1 } 'Saw' { $w = 2 } 'Triangle' { $w = 3 } default { $w = 0 } }
    return , [DLSynth]::Render([double[]]$f, [double[]]$m, 44100, [double]$script:SfxVol, [int]$w, [double]$script:SfxDuty)
}
function New-Player([byte[]]$bytes) {
    $ms = New-Object System.IO.MemoryStream (, $bytes)
    $p = New-Object System.Media.SoundPlayer $ms
    $p.Load()
    return $p
}

# scaled values shared by the wave and beep paths
function Tone-Ms([int]$ms) { return [Math]::Max([int][Math]::Round($ms * $script:SfxStretch / $script:RunSpeed), 1) }
function Tone-Hz([int]$f) { return [Math]::Min([Math]::Max([int][Math]::Round($f * $script:SfxPitch), 1), 20000) }

function Beep-Fx([int]$freq, [int]$ms) {   # sound(freq); delay(ms); nosound()
    if ($script:Headless) { return }
    $ms = Tone-Ms $ms
    if ($script:Muted -or $freq -lt 37) { Start-Sleep -Milliseconds $ms; return }
    $f = Tone-Hz $freq
    if ($script:SfxMode -eq 'Wave' -and $script:SynthOK) {
        $key = "t$f,$ms"
        $p = $script:SfxCache[$key]
        if ($null -eq $p) { $p = New-Player (Render-Wav @($f) @($ms)); $script:SfxCache[$key] = $p }
        try { $p.PlaySync(); return } catch { }
    }
    if ($f -gt 32767) { $f = 32767 }
    try { [Console]::Beep($f, $ms) } catch { Start-Sleep -Milliseconds $ms }
}

function Tone([int]$f, [int]$ms) {
    if ($script:Rec) {
        if ($f -lt 37) { $script:RecF.Add(0.0) } else { $script:RecF.Add([double](Tone-Hz $f)) }
        $script:RecM.Add([double](Tone-Ms $ms))
        return
    }
    Beep-Fx $f $ms
}
function Rest-Ms([int]$ms) {
    if ($script:Rec) { $script:RecF.Add(0.0); $script:RecM.Add([double][Math]::Max([Math]::Round($ms / $script:RunSpeed), 1)); return }
    if (-not $script:Headless) { Start-Sleep -Milliseconds ([Math]::Max([int][Math]::Round($ms / $script:RunSpeed), 1)) }
}

# Record an effect's tones into lists (no sound is made)
function Record-Sfx([int]$id) {
    $script:RecF = New-Object 'System.Collections.Generic.List[double]'
    $script:RecM = New-Object 'System.Collections.Generic.List[double]'
    $script:Rec = $true
    try { Sfx-Body $id } finally { $script:Rec = $false }
}

function Sfx([int]$id) {
    if ($script:Headless) {
        if ($env:DRUGLORD_WAVEDIR -and $script:SynthOK) {            # test hook: write the rendered effect to a file
            Record-Sfx $id
            if ($script:RecF.Count -gt 0) { [System.IO.File]::WriteAllBytes((Join-Path $env:DRUGLORD_WAVEDIR "sfx$id.wav"), [byte[]](Render-Wav $script:RecF.ToArray() $script:RecM.ToArray())) }
        }
        return
    }
    if ($script:SfxMode -eq 'Wave' -and $script:SynthOK -and -not $script:Muted) {
        $key = "s$id"
        $p = $script:SfxCache[$key]
        if ($null -eq $p) {
            Record-Sfx $id
            if ($script:RecF.Count -eq 0) { return }
            $p = New-Player (Render-Wav $script:RecF.ToArray() $script:RecM.ToArray())
            $script:SfxCache[$key] = $p
        }
        try { $p.PlaySync(); return } catch { $script:SfxMode = 'Beep' }
    }
    Sfx-Body $id
}

# ---------------------------------------------------------------------------
#  Sound lab (-SoundLab): play every effect and tweak the sound settings live
# ---------------------------------------------------------------------------
function Lab-CommandLine {
    $a = @()
    if ($script:SfxMode -ne 'Wave') { $a += "-SoundMode $($script:SfxMode)" }
    if ($script:WaveName -ne 'Square') { $a += "-Wave $($script:WaveName)" }
    if ([Math]::Abs($script:SfxDuty - 0.5) -gt 0.001) { $a += ('-Duty {0:0.##}' -f $script:SfxDuty) }
    if ([Math]::Abs($script:SfxVol - 0.35) -gt 0.001) { $a += ('-Volume {0:0.##}' -f $script:SfxVol) }
    if ([Math]::Abs($script:SfxPitch - 1) -gt 0.001) { $a += ('-Pitch {0:0.##}' -f $script:SfxPitch) }
    if ([Math]::Abs($script:SfxStretch - 1) -gt 0.001) { $a += ('-ToneStretch {0:0.##}' -f $script:SfxStretch) }
    if ([Math]::Abs($script:JetSpeed - 2) -gt 0.001) { $a += ('-JetSpeed {0:0.##}' -f $script:JetSpeed) }
    if ([Math]::Abs($script:RunSpeed - 1) -gt 0.001) { $a += ('-Speed {0:0.##}' -f $script:RunSpeed) }
    return ('.\DrugLord.ps1 ' + ($a -join ' ')).Trim()
}

function Run-SoundLab {
    $fx = @(
        @(5, 'Cop siren (start of a fight)'), @(2, 'Bad input buzz'), @(3, 'Penalty / hit'), @(4, 'Success chirp'),
        @(6, 'Rising sweep'), @(7, 'Warble'), @(8, 'Gun click'),
        @(11, 'Gunshot: type 1'), @(12, 'Gunshot: type 2'), @(13, 'Gunshot: type 3'), @(14, 'Gunshot: type 4'), @(15, 'Gunshot: type 5'),
        @(20, 'Menu beep'), @(99, 'Jet to another city (sweep)'))
    $names = 'SoundMode', 'Wave', 'Duty', 'Volume', 'Pitch', 'ToneStretch', 'JetSpeed', 'Speed'
    $total = $fx.Count + $names.Count
    $sel = 0; $last = -1; $msg = ''
    while ($true) {
        if (-not $script:Headless) { try { Clear-Host } catch {} }
        Write-Host ' DRUGLORD SOUND LAB' -ForegroundColor Yellow
        Write-Host ' Up/Down = move   Enter/Space = play (or change a setting)   Left/Right = adjust a setting'
        Write-Host ' A = play all effects   R = replay last   Q / Esc = quit and show the command line'
        Write-Host ''
        for ($i = 0; $i -lt $total; $i++) {
            $mark = if ($i -eq $sel) { '>' } else { ' ' }
            $col = if ($i -eq $sel) { 'Cyan' } else { 'Gray' }
            if ($i -lt $fx.Count) { $t = ('{0} Effect {1,-3} {2}' -f $mark, $fx[$i][0], $fx[$i][1]) }
            else {
                $nm = $names[$i - $fx.Count]
                $v = switch ($nm) {
                    'SoundMode' { $script:SfxMode } 'Wave' { $script:WaveName } 'Duty' { '{0:0.00}' -f $script:SfxDuty }
                    'Volume' { '{0:0.00}' -f $script:SfxVol } 'Pitch' { '{0:0.00}' -f $script:SfxPitch }
                    'ToneStretch' { '{0:0.00}' -f $script:SfxStretch } 'JetSpeed' { '{0:0.00}' -f $script:JetSpeed } 'Speed' { '{0:0.00}' -f $script:RunSpeed }
                }
                $t = ('{0} {1,-12} {2}' -f $mark, $nm, $v)
                if ($i -eq $fx.Count) { Write-Host '' }
            }
            Write-Host $t -ForegroundColor $col
        }
        Write-Host ''
        if ($msg) { Write-Host " $msg" -ForegroundColor Green }
        Write-Host (' Command line: ' + (Lab-CommandLine)) -ForegroundColor White
        $k = Read-Key
        $msg = ''
        if ($k -eq 0) { $k = 1000 + (Read-Key) }               # extended key: 1000 + scan code
        if ($k -ge 97 -and $k -le 122) { $k -= 32 }
        $play = -1
        switch ($k) {
            1072 { $sel = ($sel + $total - 1) % $total }       # up
            1080 { $sel = ($sel + 1) % $total }                # down
            { $_ -eq 13 -or $_ -eq 32 -or $_ -eq 1077 -or $_ -eq 1075 } {
                if ($sel -lt $fx.Count) { if ($_ -eq 13 -or $_ -eq 32) { $play = $sel } }
                else {
                    $dir = if ($_ -eq 1075) { -1 } else { 1 }
                    switch ($names[$sel - $fx.Count]) {
                        'SoundMode' { $script:SfxMode = if ($script:SfxMode -eq 'Wave') { 'Beep' } else { 'Wave' } }
                        'Wave' { $w = 'Square', 'Sine', 'Saw', 'Triangle'; $ix = [Array]::IndexOf($w, $script:WaveName); $script:WaveName = $w[($ix + $dir + 4) % 4] }
                        'Duty' { $script:SfxDuty = [Math]::Round([Math]::Min(0.95, [Math]::Max(0.05, $script:SfxDuty + 0.05 * $dir)), 2) }
                        'Volume' { $script:SfxVol = [Math]::Round([Math]::Min(1.0, [Math]::Max(0.0, $script:SfxVol + 0.05 * $dir)), 2) }
                        'Pitch' { $script:SfxPitch = [Math]::Round([Math]::Min(4.0, [Math]::Max(0.25, $script:SfxPitch + 0.05 * $dir)), 2) }
                        'ToneStretch' { $script:SfxStretch = [Math]::Round([Math]::Min(10.0, [Math]::Max(0.1, $script:SfxStretch + 0.1 * $dir)), 2) }
                        'JetSpeed' { $script:JetSpeed = [Math]::Round([Math]::Min(10.0, [Math]::Max(0.1, $script:JetSpeed + 0.1 * $dir)), 2) }
                        'Speed' { $script:RunSpeed = [Math]::Round([Math]::Min(10.0, [Math]::Max(0.1, $script:RunSpeed + 0.1 * $dir)), 2) }
                    }
                    Clear-SoundCache
                    $msg = 'Changed - press R to replay the last effect.'
                }
            }
            65 { $play = -2 }                                  # A
            82 { if ($last -ge 0) { $play = $last } }          # R
            { $_ -eq 81 -or $_ -eq 27 } { Write-Host ''; return }
        }
        if ($play -ge 0) { $last = $play; $msg = "Played effect $($fx[$play][0])."; if ($fx[$play][0] -eq 99) { Jet-Slide -SoundOnly } else { Sfx $fx[$play][0] } }
        elseif ($play -eq -2) { foreach ($e in $fx) { if ($e[0] -eq 99) { Jet-Slide -SoundOnly } else { Sfx $e[0] }; Start-Sleep -Milliseconds 250 }; $msg = 'Played all.' }
    }
}


# ---------------------------------------------------------------------------
#  Jet to another city: the city list slides off to the left while a rising sweep plays.
#  The sweep is one continuous sound played in the background; the slide is paced by the
#  clock (skipping frames if drawing is slow) so picture and sound stay together.
# ---------------------------------------------------------------------------
function Jet-Slide([switch]$SoundOnly) {
    $xs = @(); for ($x = 79; $x -gt 1; $x -= 2) { $xs += $x }
    $n = $xs.Count
    $step = [Math]::Max([int][Math]::Round($script:ToneMs / $script:JetSpeed / $script:RunSpeed), 1)
    $async = $false
    if (-not $script:Headless -and $script:SfxMode -eq 'Wave' -and $script:SynthOK -and -not $script:Muted) {
        $key = "jet$step"
        $p = $script:SfxCache[$key]
        if ($null -eq $p) {
            $fl = New-Object 'System.Collections.Generic.List[double]'; $ml = New-Object 'System.Collections.Generic.List[double]'
            foreach ($x in $xs) { $fl.Add([double](Tone-Hz (1000 + (79 - $x) * 20))); $ml.Add([double]$step) }
            $p = New-Player (Render-Wav $fl.ToArray() $ml.ToArray())
            $script:SfxCache[$key] = $p
        }
        try { $p.Play(); $async = $true } catch { }
    }
    if ($async) {
        $sw = [System.Diagnostics.Stopwatch]::StartNew()
        $done = 0
        while ($done -lt $n) {
            $target = [Math]::Min([int][Math]::Floor($sw.ElapsedMilliseconds / $step) + 1, $n)
            if ($target -gt $done) {
                while ($done -lt $target) {
                    if (-not $SoundOnly) { $x = $xs[$done]; $buf = Gettext 3 19 $x 22; Puttext 1 19 ($x - 2) 22 $buf }
                    $done++
                }
                if (-not $SoundOnly) { Flush-Screen }
            } else { [System.Threading.Thread]::SpinWait(200) }
        }
        while ($sw.ElapsedMilliseconds -lt $n * $step) { [System.Threading.Thread]::SpinWait(200) }
    } else {
        foreach ($x in $xs) {
            if (-not $SoundOnly) { $buf = Gettext 3 19 $x 22; Puttext 1 19 ($x - 2) 22 $buf }
            Tone (1000 + (79 - $x) * 20) ([int][Math]::Max([Math]::Round($script:ToneMs / $script:JetSpeed), 1))
        }
    }
}


# ---------------------------------------------------------------------------
#  Random events (f_14ab).  0 cops  1 cops sell  2 cheap stuff  3 newspaper
#  4 factory  5 brownies  6 mugger  7 trenchcoat  8 gun smuggler  9 blackmail
# ---------------------------------------------------------------------------
function Do-CopFight {
    $script:copFlag = 0
    $result = 0                                   # 0 fighting, 1 escaped, 2 failed to run
    $shots = 1; if ($script:gunType -ge 4) { $shots = 3 }
    Cprint $script:T.x142a 27 17
    Sfx 5
    Cprint $script:T.x144d 15 19
    Cprint $script:T.x1474 30 20
    Cprint $script:T.x1498 30 21
    $choice = 0
    while ($true) {
        Frame 1 23 80 25
        Cprint $script:T.x14bc 20 23
        Cursor 1
        [void](Read-Field 1 0)
        Cursor 0
        $choice = [int](Atol $script:Buf)
        if ($choice -lt 1 -or $choice -gt 3) { continue }
        if (($choice -eq 2 -and $script:status -eq 0) -or ($choice -eq 3 -and $script:status -lt 5)) {
            Cprint $script:T.x14c9 25 25
            Sfx 3
            Delay-Ms 2000
            Frame 25 25 80 25
            continue
        }
        break
    }
    switch ($choice) {
        1 { $nf = 0 }
        2 { $nf = 1; $script:status-- }
        3 { $nf = (Rand) % 4 + [int](IDiv $script:status 100) + 4; if ($nf -gt 8) { $nf = 8 }; $script:status -= 5 }
    }
    # ----- set up the fight screen
    Frame 1 1 80 25
    Puttext 1 1 80 22 $script:Scr.fight
    Set-Color 14 0
    Cprint $script:T.x11fc 24 2
    Cprint $script:T.x11fc 52 2
    Set-Color 12 0; Cprint $script:T.x1202 32 2
    Set-Color 10 0
    Cprint $script:T.x141c 9 8;  Cprint $script:T.x141c 49 8
    Cprint $script:T.x1423 28 8; Cprint $script:T.x1423 68 8
    Set-Color 14 0
    Cprint $script:T.x1269 10 6; Cprint $script:T.x1269 50 6
    Cprint $script:T.x125e 9 5
    Set-Color 12 0
    Gotoxy 20 5; Cputs (Pad-R ([string]$script:numGuns) 10)
    Set-Color 6 0
    Gotoxy 20 6; Cputs (Pad-R $script:GunName[$script:gunType] 15)
    $php = New-Object 'int[]' 10                  # your side: [0] = you
    $php[0] = $script:health
    Set-Color 6 0
    for ($i = 1; $i -le $nf; $i++) {
        $php[$i] = (Rand) % 10 + (Dice 10 ([int](IDiv $script:status 50))) + 25
        Gotoxy 1 ($i + 10)
        if ($choice -eq 2) { Cputs ((Pad-L 'Friend' 10) + ':') }
        if ($choice -eq 3) { Cputs ((Pad-L 'Posse' 6) + ' #' + (Pad-L ([string]$i) 2) + ':') }
    }
    while ($true) {                               # how many cops, and what do they carry
        $nc = (Rand) % 8 + 3
        $ct = [int](IDiv $script:status ($nc * 2))
        if ((Rand) % 20 -eq 0) {
            if ($ct -lt 2) {
                $r = (Rand) % 4 + 1
                $ct = [int](IDiv $script:status $r)
                if ($ct -gt 3) { $ct = 3 }
                if ($ct -lt 2) { $ct = 2 }
            } else {
                $nc = (Rand) % 3 + 8
                $ct = 3
            }
            break
        } else {
            if ($ct -lt 2) { continue }
            if ($ct -gt 3) { continue }
            break
        }
    }
    Gotoxy 60 6
    Set-Color 9 0
    Cputs (Pad-R $script:GunName[$ct] 15)
    Set-Color 9 0
    $chp = New-Object 'int[]' 12
    for ($i = 0; $i -lt $nc; $i++) {
        Gotoxy 42 ($i + 11)
        Cputs ((Pad-L 'Cop' 6) + ' #' + (Pad-L ([string]($i + 1)) 2))
        $chp[$i] = (Dice 15 2) + 15
    }
    $copsAlive = $nc
    $partyAlive = $nf + 1
    for ($i = 0; $i -le $nf; $i++) { Draw-HP $php[$i] $i 0 }
    for ($i = 0; $i -lt $nc; $i++) { Draw-HP $chp[$i] $i 1 }

    do {
        Frame 1 24 80 25
        Cprint $script:T.x1525 5 24
        Cursor 1
        $key = Ask-Key 'FR'
        Cursor 0
        Putch $key
        if ($key -eq 82) {                               # R: run
            Cprint $script:T.x15a1 20 25
            Delay-Ms 3000
            $a = (Rand) % 10 + [int](IDiv $nc 3)
            $b = (Rand) % 10 + [int](IDiv $nf 3)
            if ($a -lt $b) {
                Cprint $script:T.x15bc 99 99
                $script:health = $php[0]
                $result = 1
            } else {
                Cprint $script:T.x15d1 99 99
                $result = 2
                $key = 70
            }
        }
        if ($key -eq 70) {                               # F: fight
            if ($result -ne 2) {
                for ($si = 0; $si -le $nf; $si++) {
                    if ($php[$si] -eq 0) { continue }
                    if ($si -lt $script:numGuns) { Cprint $script:T.x154e 28 ($si + 10) }
                    else { Cprint $script:T.x1557 27 ($si + 10) }
                    $att = 0
                    while ($true) {
                        do { $t = (Rand) % $nc } while ($chp[$t] -eq 0)
                        Sfx ($script:gunType + 10)
                        if ((Rand) % 2 -eq 0) {
                            Cprint $script:T.x1562 67 ($t + 11)
                            Sfx 8
                            Delay-Ms 100
                            if ($si -lt $script:numGuns) { $chp[$t] -= (Dice 6 $script:GunDice[$script:gunType]) }
                            else { $chp[$t] -= (Dice 4 1) }
                            if ($chp[$t] -le 0) { $chp[$t] = 0; $copsAlive-- }
                        } else {
                            Cprint $script:T.x156f 67 ($t + 11)
                            Delay-Ms 200
                        }
                        Draw-HP $chp[$t] $t 1
                        $att++
                        if ($php[0] -ne 0 -and $copsAlive -ne 0 -and $script:gunType -gt 3 -and $att -lt $shots -and $copsAlive -gt 0 -and $si -lt $script:numGuns) { continue }
                        break
                    }
                    Draw-HP $php[$si] $si 0
                    if ($copsAlive -eq 0) { break }
                }
            }
            if ($copsAlive -ne 0) {
                $result = 0
                for ($si = 0; $si -lt $nc; $si++) {
                    if ($chp[$si] -eq 0) { continue }
                    Cprint $script:T.x157d 68 ($si + 11)
                    do { $t = (Rand) % ($nf + 1) } while ($php[$t] -eq 0)
                    Sfx ($ct + 10)
                    if ((Rand) % 2 -eq 0) {
                        Cprint $script:T.x1586 26 ($t + 10)
                        Sfx 8
                        Delay-Ms 100
                        $php[$t] -= (Dice 6 $script:GunDice[$ct])
                        if ($php[$t] -le 0) { $script:status--; $php[$t] = 0; $partyAlive-- }
                    } else {
                        Cprint $script:T.x1593 26 ($t + 10)
                        Delay-Ms 200
                    }
                    Draw-HP $php[$t] $t 0
                    Draw-HP $chp[$si] $si 1
                    if ($php[0] -eq 0 -or $partyAlive -eq 0) { break }
                }
            }
        }
    } while ($copsAlive -gt 0 -and $partyAlive -gt 0 -and $php[0] -gt 0 -and $result -ne 1)

    if ($result -eq 1) {
        Delay-Ms 3000
        Draw-Main
        return
    }
    $script:health = $php[0]
    if ($script:health -eq 0) { $script:dead = $true; return }
    # victory
    Frame 1 23 80 25
    Cprint $script:T.x15ed 23 23
    Delay-Ms 1000
    $sell = 1
    if (-not ($ct -lt $script:gunType -and $nc -le $script:numGuns)) {
        Set-Color 7 0
        Gotoxy 21 24
        if ($nc -eq 1) { $what = 'his gun' } else { $what = 'their guns' }
        Cputs ("Do you wanna take $what, man? (Y/N) ")
        Cursor 1
        $k = Ask-Key 'YN'
        Cursor 0
        if ($k -eq 89) {
            Cprint $script:T.x1649 15 25
            $sell = 0
            if ($script:gunType -eq $ct) { $script:numGuns += $nc }
            else { $script:gunType = $ct; $script:numGuns = $nc }
        } else {
            Cprint $script:T.x1688 29 25
            Delay-Ms 2000
        }
    }
    if ($sell -eq 1) {
        Cprint $script:T.x16a2 5 24
        $gold = $nc * $ct * 200
        Cputs ("$nc guns that were on the pigs you wasted for $gold bucks!")
        Cprint $script:T.x16e9 20 25
        $script:cash += $gold
        $script:status += $nc
        Delay-Ms 4000
    }
    if (-not $script:dead) { Draw-Main }
}

function Do-Event([int]$ev) {
    Cursor 0
    if ($script:done[$ev]) { return }
    Frame 1 17 80 25
    switch ($ev) {
        0 { Do-CopFight }
        1 {
            if ($script:mktToday -ne 1) {
                Cprint $script:T.x1717 1 18
                Set-Color 6 0
                Market-Event 3 30 (F32 0.7)
                Set-Color 11 0
                Cputs $script:T.x180a
                Sfx 6
                Press-AnyKey
            }
        }
        2 {
            Cprint $script:T.x180d 1 18
            Set-Color 6 0
            Market-Event 3 40 (F32 0.6)
            Set-Color 7 0
            Cputs $script:T.x18c9
            Press-AnyKey
        }
        3 {
            Window 15 17 80 25
            Cprint $script:T.x18cd 15 17
            Window 1 1 80 25
            for ($si = 0; $si -lt 8; $si++) {
                $f = ((Rand) % 25) / 100.0 + 1.05
                $script:price[$si] = Ftol ([double]$script:price[$si] * $f)
            }
            Sfx 7
            Press-AnyKey
        }
        4 {
            Cprint $script:T.x1a7e 1 18
            Set-Color 6 0
            Market-Event 1 40 (F32 1.2)
            Set-Color 13 0
            if ((Rand) % 2 -eq 1) { Cputs $script:T.x1ad4 } else { Cputs $script:T.x1ae1 }
            Cprint $script:T.x1aea 99 99
            Press-AnyKey
        }
        5 {
            if ($script:inv[5] -ne 0) {
                Cprint $script:T.x1b2e 1 17
                $script:inv[5] = 0
                Press-AnyKey
            }
        }
        6 {
            Cprint $script:T.x1c5d 1 17
            $used = $false
            if ($script:status -ge 10 -and (Rand) % 20 -ne 0) {
                Cprint $script:T.x1cf6 1 20
                Cursor 1
                $k = Ask-Key 'YN'
                Cursor 0
                if ($k -eq 89) {
                    Cprint $script:T.x1d44 1 22
                    $script:status -= 10
                    $used = $true
                }
            }
            if (-not $used) {
                if ($script:cash -lt 50) {
                    Cprint $script:T.x1dae 20 21
                    Cprint $script:T.x1dd8 10 22
                    Cprint $script:T.x1e13 28 23
                    Sfx 13
                    $script:health -= ((Rand) % 6 + (Rand) % 6 + 2)
                } else {
                    Cprint $script:T.x1e31 5 22
                }
                $script:cash = 0
            }
            Press-AnyKey
        }
        7 {
            if ($script:hold -lt 300) {
                do {
                    $cap = (Rand) % 10 + 1
                    if ((Rand) % 5 -eq 0) {
                        $cap += (Rand) % 10 + 1
                        if ((Rand) % 5 -eq 0) { $cap += (Rand) % 10 + 1 }
                    }
                    $cap *= 10
                } while ($script:hold + 50 -lt $cap -or $cap -le $script:hold)
                $a = Ftol ([Math]::Pow($cap, 3.34) * 0.0634)
                $t = ([long]((Rand) % 200 + 1)) * $a
                $pr = (Ftol ([double]$t / 10000.0)) * 100
                Cprint $script:T.x1e77 1 17
                Cprint $script:T.x1ec7 10 19
                Cprint $script:T.x1f0c 11 20
                Set-Color 12 0
                Cputs ([string]$cap)
                Cprint $script:T.x1f27 99 99
                Set-Color 12 0
                Cputs ('$' + $pr)
                Cprint $script:T.x1f47 99 99
                Cprint $script:T.x1f53 1 23
                Cursor 1
                $k = Ask-Key 'YN'
                Cursor 0
                if ($k -eq 89) {
                    if ($script:cash -lt $pr) { Cprint $script:T.x1f6c 1 24 }
                    else {
                        Cprint $script:T.x1fbc 16 24
                        $script:cash -= $pr
                        $script:hold = $cap
                    }
                } else { Cprint $script:T.x1fee 20 24 }
                Press-AnyKey
            }
        }
        8 {
            $gi = (Rand) % 5
            if ((Rand) % 10 -eq 0) { $qty = (Rand) % 6 + 5 } else { $qty = (Rand) % 5 + 2 }
            $each = $gi * 200 + 200
            $total = $qty * $each
            Cprint $script:T.x201a 1 17
            Cprint $script:T.x2080 1 20
            Set-Color 12 0
            Cputs ($script:GunLong[$gi] + "'s")
            Cprint $script:T.x209e 99 99
            Cprint $script:T.x20b9 1 21
            Set-Color 12 0
            Cputs ("$qty ")
            Cprint $script:T.x20e0 99 99
            Set-Color 12 0
            Cputs ('$' + $each + ' ')
            Cprint $script:T.x20eb 99 99
            Set-Color 12 0
            Cputs ('$' + $total)
            Set-Color 7 0
            Cputs $script:T.x2107
            Cprint $script:T.x210a 20 23
            Cursor 1
            $k = Ask-Key 'YN'
            Cursor 0
            if ($k -eq 89) {
                if ($total -gt $script:cash) { Cprint $script:T.x2135 5 24 }
                else {
                    Cprint $script:T.x2185 1 24
                    $script:numGuns = $qty
                    $script:gunType = $gi + 1
                    $script:cash -= $total
                }
            } else { Cprint $script:T.x21d7 15 24 }
            Press-AnyKey
        }
        9 {
            if ($script:cash -ge 10000) {
                $f = F32 (((Rand) % 30) / 100.0 + 0.7)
                $amt = (Ftol ([double]$script:cash * $f / 1000.0)) * 1000
                Cprint $script:T.x220f 1 17
                Set-Color 12 0
                Cputs ('$' + $amt + ' ')
                Cprint $script:T.x227a 99 99
                Cprint $script:T.x22d9 1 21
                Cursor 1
                $k = Ask-Key 'YN'
                Cursor 0
                if ($k -eq 89) {
                    Cprint $script:T.x22fe 1 24
                    $script:cash -= $amt
                } else {
                    Cprint $script:T.x2345 9 24
                    if ((Rand) % 5 -ne 0) { $script:copFlag = 1; $script:done[0] = $false }
                }
                Press-AnyKey
            }
        }
    }
    $script:done[$ev] = $true
    if (-not $script:dead) { Draw-Status }
}

# ---------------------------------------------------------------------------
#  Save / load  (Ctrl-S / Ctrl-L)
#  Saves are small JSON text files named <name>.sav, kept next to the script
#  (or wherever -SaveDir points).
# ---------------------------------------------------------------------------
$script:SaveMagic = 'DRUGLORD-SAVE'
$script:SaveVer = 1

function Get-SaveDir {
    $d = $script:SaveDirPath
    if ([string]::IsNullOrEmpty($d)) {
        if ($PSScriptRoot) { $d = $PSScriptRoot } else { $d = (Get-Location).Path }
    }
    return $d
}

function Clean-SaveName([string]$s) {                 # letters, digits, - and _ only, max 20 chars
    $t = ($s -replace '[^A-Za-z0-9_\-]', '')
    if ($t.Length -gt 20) { $t = $t.Substring(0, 20) }
    return $t
}

# dialog frame: double box with a centred title; returns the screen underneath so it can be restored
function Open-Dialog([string]$title, [int]$x1, [int]$y1, [int]$x2, [int]$y2) {
    $under = Gettext $x1 $y1 $x2 $y2
    Cursor 0
    Frame $x1 $y1 $x2 $y2
    Set-Color 15 0
    Box $script:T.x2466 $x1 $y1 $x2 $y2
    Set-Color 14 0
    Cprint $title ([int](($x1 + $x2 - $title.Length) / 2 + 1)) ($y1 + 1)
    return , $under
}
function Close-Dialog($under, [int]$x1, [int]$y1, [int]$x2, [int]$y2) {
    Puttext $x1 $y1 $x2 $y2 $under
    Cursor 0
}
function Dialog-Msg([string]$s, [int]$color, [int]$row) {
    Gotoxy 17 $row
    Set-Color $color 0
    Cputs (Pad-R $s 47)
}

function Make-SaveState {
    return [ordered]@{
        magic = $script:SaveMagic; version = $script:SaveVer
        cash = $script:cash; bank = $script:bank; debt = $script:debt
        city = $script:city; month = $script:month; day = $script:day; year = $script:year
        hold = $script:hold; inv = @($script:inv); basis = @($script:basis)
        price = @($script:price); offs = @($script:offs)
        numGuns = $script:numGuns; gunType = $script:gunType
        status = $script:status; health = $script:health
        loanFlag = $script:loanFlag; loanDays = $script:loanDays
        angry = $script:angry; thugs = $script:thugs; copFlag = $script:copFlag
        done = @($script:done); mktToday = $script:mktToday; totalDays = $script:totalDays
        goodDeals = $script:goodDeals; totalDeals = $script:totalDeals
    }
}

function Test-InRange($v, [double]$lo, [double]$hi) {
    if ($v -is [array] -or $null -eq $v) { return $false }
    $d = 0.0
    if (-not [double]::TryParse([string]$v, [System.Globalization.NumberStyles]::Float, [System.Globalization.CultureInfo]::InvariantCulture, [ref]$d)) { return $false }
    return ($d -ge $lo -and $d -le $hi)
}

# returns an error string, or '' after applying the save to the game
function Apply-SaveState($o) {
    if ($null -eq $o -or $o.magic -ne $script:SaveMagic) { return 'Not a DrugLord save file.' }
    if ($o.version -ne $script:SaveVer) { return 'Unknown save file version.' }
    foreach ($a in 'inv', 'basis', 'price', 'offs') {
        if (@($o.$a).Count -ne 8) { return 'Save file is damaged.' }
        foreach ($v in @($o.$a)) { if (-not (Test-InRange $v -1e15 1e15)) { return 'Save file is damaged.' } }
    }
    if (@($o.done).Count -ne 10) { return 'Save file is damaged.' }
    $limits = @{ cash = @(-1e15, 1e15); bank = @(-1e15, 1e15); debt = @(-1e15, 1e15)
        city = @(0, 7); month = @(1, 12); day = @(1, 31); year = @(0, 200); hold = @(0, 100000)
        numGuns = @(0, 1000); gunType = @(0, 5); status = @(-1e9, 1e9); health = @(-1e6, 1e6)
        loanFlag = @(0, 1); loanDays = @(-1e6, 1e6); angry = @(0, 1); thugs = @(0, 1e6); copFlag = @(0, 1)
        mktToday = @(0, 1e6); totalDays = @(0, 1e9); goodDeals = @(0, 1e9); totalDeals = @(0, 1e9) }
    foreach ($k in $limits.Keys) {
        if (-not (Test-InRange $o.$k $limits[$k][0] $limits[$k][1])) { return 'Save file is damaged.' }
    }
    $script:cash = [long]$o.cash; $script:bank = [long]$o.bank; $script:debt = [long]$o.debt
    $script:city = [int]$o.city; $script:month = [int]$o.month; $script:day = [int]$o.day; $script:year = [int]$o.year
    $script:hold = [int]$o.hold
    for ($i = 0; $i -lt 8; $i++) {
        $script:inv[$i] = [int]@($o.inv)[$i]; $script:basis[$i] = [long]@($o.basis)[$i]
        $script:price[$i] = [long]@($o.price)[$i]; $script:offs[$i] = [double]@($o.offs)[$i]
    }
    for ($i = 0; $i -lt 10; $i++) { $script:done[$i] = [bool]@($o.done)[$i] }
    $script:numGuns = [int]$o.numGuns; $script:gunType = [int]$o.gunType
    $script:status = [int]$o.status; $script:health = [int]$o.health
    $script:loanFlag = [int]$o.loanFlag; $script:loanDays = [int]$o.loanDays
    $script:angry = [int]$o.angry; $script:thugs = [int]$o.thugs; $script:copFlag = [int]$o.copFlag
    $script:mktToday = [int]$o.mktToday; $script:totalDays = [int]$o.totalDays
    $script:goodDeals = [int]$o.goodDeals; $script:totalDeals = [int]$o.totalDeals
    $script:dead = $false
    return ''
}

function Save-GameDialog {                              # F2 / Ctrl-S: ask for a name (Enter alone = next free saveN)
    $dir = Get-SaveDir
    $n = 1
    while (Test-Path -LiteralPath (Join-Path $dir ("save$n.sav"))) { $n++ }
    $default = "save$n"
    $x1 = 14; $y1 = 7; $x2 = 68; $y2 = 16
    $under = Open-Dialog 'Save Game' $x1 $y1 $x2 $y2
    Set-Color 7 0
    Cprint "Enter alone = $default    ESC = cancel" 17 13
    Cprint 'Letters, digits, - and _ only.' 17 14
    $name = ''
    while ($true) {
        Set-Color 11 0
        Cprint 'Save as filename: ' 17 9
        Set-Color 14 0
        if (Read-Field 21 0) { Close-Dialog $under $x1 $y1 $x2 $y2; return $false }
        $name = Clean-SaveName $script:Buf
        if ($name.Length -eq 0) { if ($script:Buf.Trim().Length -eq 0) { $name = $default } else { Sfx 2; continue } }
        $path = Join-Path $dir ($name + '.sav')
        if (Test-Path -LiteralPath $path) {
            Dialog-Msg "'$name' exists - overwrite? (Y/N)" 12 9
            Cursor 1
            $k = Ask-Key 'YN'
            Cursor 0
            if ($k -eq 78) { Dialog-Msg '' 7 9; continue }
        }
        break
    }
    $ok = $false
    try {
        $json = (Make-SaveState) | ConvertTo-Json -Depth 4
        [System.IO.File]::WriteAllText($path, $json, (New-Object System.Text.UTF8Encoding($false)))
        $ok = $true
    } catch { }
    Frame ($x1 + 1) ($y1 + 2) ($x2 - 1) ($y2 - 1)
    if ($ok) {
        Set-Color 10 0
        Cprint "Game saved as: $name.sav" 17 9
        Set-Color 11 0
        Cprint 'Location:' 17 11
        Set-Color 15 0
        $row = 12; $p = $dir
        while ($p.Length -gt 0 -and $row -le 13) {                 # wrap the folder path over two lines
            $take = [Math]::Min(48, $p.Length)
            Cprint $p.Substring(0, $take) 17 $row
            $p = $p.Substring($take); $row++
        }
        Sfx 4
    } else {
        Set-Color 12 0
        Cprint 'Could not write the save file.' 17 9
        Set-Color 7 0
        Cprint ($path.Substring(0, [Math]::Min(48, $path.Length))) 17 11
        Sfx 3
    }
    Set-Color 7 0
    Cprint 'Press any key to continue' 17 15
    Cursor 0
    Flush-Keys
    [void](Read-Key)
    Close-Dialog $under $x1 $y1 $x2 $y2
    return $ok
}

function Get-SaveList {
    $r = @()
    $dir = Get-SaveDir
    if (-not (Test-Path -LiteralPath $dir)) { return $r }
    foreach ($f in (Get-ChildItem -LiteralPath $dir -Filter '*.sav' -File -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending)) {
        $info = ''
        try {
            $o = Get-Content -LiteralPath $f.FullName -Raw | ConvertFrom-Json
            if ($o.magic -ne $script:SaveMagic) { continue }
            $info = ('{0:00}/{1:00}/{2:00}  ${3}' -f [int]$o.month, [int]$o.day, [int]$o.year, [long]$o.cash)
        } catch { continue }
        $r += , @([System.IO.Path]::GetFileNameWithoutExtension($f.Name), $info)
    }
    return $r
}

function Load-GameDialog {
    $list = @(Get-SaveList)
    $show = [Math]::Min($list.Count, 8)
    $x1 = 14; $y1 = 5; $x2 = 68
    if ($show -gt 0) { $y2 = 13 + $show } else { $y2 = 11 }
    $under = Open-Dialog 'Load Game' $x1 $y1 $x2 $y2
    $loaded = $false
    if ($list.Count -eq 0) {
        Set-Color 12 0
        Cprint 'No saved games found.' 17 8
        Set-Color 7 0
        Cprint ('Folder: ' + (Pad-R (Get-SaveDir) 40).Substring(0, 40)) 17 9
        Sfx 2
        Delay-Ms 1800
        Close-Dialog $under $x1 $y1 $x2 $y2
        return $false
    }
    Set-Color 7 0
    for ($i = 0; $i -lt $show; $i++) {
        Set-Color 10 0
        Cprint ('{0}. ' -f ($i + 1)) 17 (4 + $y1 + $i)
        Set-Color 15 0
        Cputs (Pad-R $list[$i][0] 22)
        Set-Color 7 0
        Cputs $list[$i][1]
    }
    $inRow = 5 + $y1 + $show
    Set-Color 7 0
    Cprint 'Type a name or a number.   ESC = cancel' 17 ($inRow + 2)
    while ($true) {
        Set-Color 11 0
        Cprint 'Load filename: ' 17 $inRow
        Set-Color 14 0
        if (Read-Field 21 0) { break }
        $t = $script:Buf.Trim()
        $name = ''
        if ($t -match '^\d+$' -and [int]$t -ge 1 -and [int]$t -le $show) { $name = [string]$list[[int]$t - 1][0] }
        else { $name = Clean-SaveName $t }
        $path = Join-Path (Get-SaveDir) ($name + '.sav')
        if ($name.Length -eq 0 -or -not (Test-Path -LiteralPath $path)) {
            Set-Color 12 0; Cprint ('File not found.' + (' ' * 30)) 17 ($inRow + 1)
            Sfx 2
            continue
        }
        $err = ''
        try { $err = Apply-SaveState (Get-Content -LiteralPath $path -Raw | ConvertFrom-Json) }
        catch { $err = 'Not a DrugLord save file.' }
        if ($err -ne '') {
            Set-Color 12 0; Cprint ((Pad-R $err 40)) 17 ($inRow + 1)
            Sfx 2
            continue
        }
        Set-Color 10 0; Cprint ((Pad-R "Loaded '$name'." 40)) 17 ($inRow + 1)
        Sfx 4
        Delay-Ms 1000
        $loaded = $true
        break
    }
    if (-not $loaded) { Close-Dialog $under $x1 $y1 $x2 $y2 }
    return $loaded
}


# ---------------------------------------------------------------------------
#  Title, setup, endings
# ---------------------------------------------------------------------------
$script:ToneMs = 31                                   # original: calibrated against the BIOS clock (31 on a fast PC)
$script:ESC = [string][char]27

function Show-Title {                                # f_32aa
    Cursor 0
    Window 1 1 80 25
    Set-Color 7 0
    Clrscr
    Set-Color 14 0
    Puttext 10 5 71 10 $script:Scr.title
    Cprint $script:T.x24c5 35 13
    Cprint '@01^10@09^10@0b^10     @1eREGISTERED VERSION    @0b^11@09^11@01^11' 24 16
    Set-Color 13 0; Cprint 'Registered To: ' 8 17
    Set-Color 15 2; Cprint (Pad-R 'justz00t' 25) 23 17
    Set-Color 13 0; Cprint 'Serial Number: ' 50 17
    Set-Color 15 2; Cprint '000001' 65 17
    Cprint $script:T.x2504 30 19
    Cprint $script:T.x251c 23 20
    Cprint '@0cPorted to PowerShell by justz00t' 25 21
    Delay-Ms 500
    if ($script:Headless) { return }
    $title = $script:Scr.title
    $flag = 1
    $outer = 0
    while ($outer -lt 10) {                          # shimmer the block letters in random colours
        $count = 0
        $flag = [int]($flag -eq 0)
        Set-Color ((Rand) % 15) 0
        $si = 0; $di = 0
        $step = (Rand) % 6 * 62 + 13
        while ($count -lt 441) {
            $si += $step
            if ($si -ge 62) {
                do { $si -= 62; $di++; if ($di -gt 6) { $di = 0 } } while ($si -ge 62)
            }
            if ($di -lt 6 -and $title[$di * 124 + $si * 2] -eq 0xB1) {
                Gotoxy ($si + 10) ($di + 5)
                Putch 0xDB
                Delay-Ms 5
                $count++
            }
            if (Test-Key) { $count = 701; $outer = 11 }
        }
        $outer++
    }
}

function Init-Game {                                 # f_34a7
    Cursor 0
    Frame 28 22 53 25
    Set-Color 14 0
    Box $script:T.x2542 30 22 51 24
    Textcolor 0x8D; Textbackground 0
    Cprint $script:T.x254b 34 23
    for ($i = 0; $i -lt 8; $i++) { $script:inv[$i] = 0; $script:offs[$i] = 1.0; $script:basis[$i] = 0 }
    for ($i = 0; $i -lt 10; $i++) { $script:done[$i] = $false }
    Gen-Prices                                        # (uses the previous city, as the original does)
    $script:city = (Rand) % 8
    $script:cash = 500; $script:hold = 10; $script:bank = 0; $script:debt = 0
    $script:loanFlag = 0; $script:numGuns = 0; $script:gunType = 0
    $script:status = 1; $script:health = 100; $script:dead = $false
    $script:angry = 0; $script:copFlag = 0; $script:thugs = 0; $script:totalDays = 0
    $script:month = (Rand) % 12 + 1
    $script:day = (Rand) % 28 + 1
    $script:year = (Rand) % 6 + 86
    Delay-Ms 1000
}

function Show-Death {                                # f_28c0
    Delay-Ms 2000
    Puttext 23 5 57 19 $script:Scr.tomb
    Delay-Ms 5000
    Flush-Keys
    [void](Read-Key)
}

function Ask-PlayAgain {                             # f_2812
    Set-Color 15 0
    Box $script:T.x2466 29 10 51 15
    Frame 30 11 50 14
    Cprint $script:T.x23c0 32 12
    Cprint $script:T.x23d3 32 13
    Cursor 1
    Gotoxy 40 14
    $k = Ask-Key 'YN'
    return ($k -eq 89)
}

function Rep-Set([byte[]]$rep, [int]$line, [int]$col, [byte[]]$src) {       # strcpy into a 50-byte report record
    $o = $line * 50 + $col
    for ($i = 0; $i -lt $src.Length; $i++) { $rep[$o + $i] = $src[$i] }
    $rep[$o + $src.Length] = 0
}
function Num-Bytes($n) { return ,[System.Text.Encoding]::ASCII.GetBytes([string]$n) }
function Rep-Num([byte[]]$rep, [int]$line, [int]$endCol, $n) {
    $b = Num-Bytes $n
    Rep-Set $rep $line ($endCol - $b.Length) $b
}
function Rep-Line([byte[]]$rep, [int]$line) {
    $r = New-Object 'byte[]' 50
    [Array]::Copy($rep, $line * 50, $r, 0, 50)
    return ,$r
}

function Show-Win {                                  # f_28f9: the DRUGLORD ending
    Delay-Ms 1000
    Frame 1 17 80 25
    $word = 'DRUGLORD!!!'
    for ($it = 1; $it -lt 6; $it++) {
        Frame 15 20 65 20
        Set-Color ($it + 9) 0
        for ($j = 0; $j -lt 22; $j++) {
            Gotoxy ([int][Math]::Floor($j / 2) * 5 + 15) 20
            Putch ([int][char]$word[[int][Math]::Floor($j / 2)])
            if ($j -lt 16) { Tone $script:Freq[$j] 20 }
        }
        Delay-Ms 500
    }
    Puttext 1 1 80 25 $script:Scr.final
    # flames: one per gun (max 10)
    $nf = [Math]::Min($script:numGuns, 10)
    for ($k = 1; $k -le $nf; $k++) {
        $even = [int](($k % 2) -eq 0)
        $y = $even * 3 + 2
        if ($k -le 4) { $x = [int][Math]::Floor(($k - 1) / 2) * 11 + $even * 5 + 55 }
        else { $x = [int][Math]::Floor(($k - 5) / 2) * 11 + $even * 5 + 1 }
        Puttext $x $y ($x + 7) ($y + 2) $script:Scr.fire
    }
    # stacks of money
    $bars = [int](IDiv ($script:cash + $script:bank - 1) 2000000) + 1
    for ($si = 0; $si -lt 4; $si++) {
        $row = $si + 11
        for ($j = 0; $j -lt 5; $j++) {
            $x = $j * 7 + 18 - 2 * $si
            $d = 15 - 3 * $si - $j; if ($d -lt 0) { $d = 0 }
            for ($m = 0; $m -lt $bars; $m++) {
                $val = [long]($si * 5 + $j + 1) * 100000 + [long]$m * 2000000
                if ($val -gt $script:cash) { break }
                Puttext $x ($row - $m) ($x + 4) ($row - $m) $script:Scr.bar
                Tone (1000 + $m * 500) ($d + 5)
                Tone (3000 + $m * 500) $d
            }
        }
    }
    # ---- score report
    if ($script:totalDeals -ne 0) { $pct = [int](IDiv ($script:goodDeals * 100) $script:totalDeals) } else { $pct = 0 }
    $rep = [Convert]::FromBase64String($script:ReportB64)
    Rep-Num $rep 6 0x29 $script:cash
    Rep-Num $rep 7 0x29 $script:bank
    Rep-Num $rep 8 0x29 $script:debt
    Rep-Num $rep 9 0x29 $script:goodDeals
    Rep-Num $rep 10 0x29 $script:totalDeals
    Rep-Num $rep 11 0x28 $pct
    $o = 11 * 50; $e = $o
    while ($rep[$e] -ne 0) { $e++ }
    $rep[$e] = 37; $rep[$e + 1] = 0                                   # strcat "%"
    $gn = [System.Text.Encoding]::ASCII.GetBytes($script:GunName[$script:gunType])
    Rep-Set $rep 12 (0x17 - $gn.Length) $gn
    Rep-Set $rep 12 0x17 ([byte[]](Str-Bytes $script:T.x2454))
    Rep-Num $rep 12 0x29 $script:numGuns
    Rep-Num $rep 13 0x29 $script:totalDays
    $total = [double]($script:cash + $script:bank - $script:debt)
    $p1 = 0
    $ex = [Math]::Exp($total * 6.9315e-08)
    if ($ex -gt 100000) { $p1 = 700000 } else { $p1 = (Ftol $ex) * 75000; if ($p1 -gt 700000) { $p1 = 700000 } }
    $p2 = 0
    if ($script:totalDays -gt 0) { $p2 = Ftol ([Math]::Pow($script:totalDays, -2.12) * 104000000000.0); if ($p2 -gt 500000) { $p2 = 500000 } }
    $p3 = Ftol ([Math]::Pow($pct, 8.14) * 2.48e-11)
    if ($p3 -gt 500000) { $p3 = 500000 }
    $p4 = [long]($script:gunType * $script:numGuns) * 1500
    $score = $p1 + $p2 + $p3 + $p4
    Rep-Num $rep 16 0x29 $score
    $rk = 0
    while ($rk -lt 10 -and $script:RankThr[$rk] -le $score) { $rk++ }
    $rn = [System.Text.Encoding]::ASCII.GetBytes($script:T[$script:RankName[$rk]])
    Rep-Set $rep 17 (0x29 - $rn.Length) $rn

    Set-Color 9 0
    Box $script:T.x2466 10 18 55 25
    Cprint $script:T.x246f 50 25
    Cprint $script:T.x2475 12 25
    Set-Color 14 0
    for ($si = 0; $si -lt 5; $si++) { Cprint-Bytes (Rep-Line $rep $si) 15 (19 + $si) }
    $si = 0
    while ($true) {
        $c = Read-Key
        Set-Color 14 0
        if ($c -eq 0) {
            $c2 = Read-Key
            if ($c2 -eq 0x48 -and $si -gt 0) {
                $si--
                $buf = Gettext 15 19 55 23
                Puttext 15 20 55 24 $buf
                Cprint-Bytes (Rep-Line $rep $si) 15 19
                Sfx 20
            }
            if ($c2 -eq 0x50 -and $si -lt 12) {
                $si++
                $buf = Gettext 15 20 55 24
                Puttext 15 19 55 23 $buf
                Cprint-Bytes (Rep-Line $rep ($si + 5)) 15 24
                Sfx 20
            }
            if ($si -gt 0) { Cprint $script:T.x248d 50 18 } else { Cprint $script:T.x2493 50 18 }
            if ($si -lt 10) { Cprint $script:T.x2499 50 25 } else { Cprint $script:T.x249f 50 25 }
        }
        if ($c -eq 27) { break }
    }
}

# ---------------------------------------------------------------------------
#  The game
# ---------------------------------------------------------------------------
function Run-Game {
    Init-Console
    if (-not (Test-ConsoleSize)) {
        Write-Host 'DRUGLORD requires an 80x25 (or larger) console window.'
        return
    }
    Show-Title
    $playing = $true
    Init-Game
    Draw-Main
    Draw-Status
    if ($script:Headless -and $env:DRUGLORD_SET) {               # test hook
        foreach ($kv in ($env:DRUGLORD_SET -split ';')) {
            $p = $kv -split '='
            if ($p.Count -eq 2) { Set-Variable -Scope script -Name $p[0] -Value ([long]$p[1]) }
        }
        Draw-Status
    }
    if ($script:Headless -and $env:DRUGLORD_FORCE -ne $null -and $env:DRUGLORD_FORCE -ne '') { Do-Event ([int]$env:DRUGLORD_FORCE) }
    while ($true) {                                              # one pass = one game
        while ($true) {                                          # menu loop (0x36ff)
            Cursor 0
            Frame 1 17 80 25
            Cprint $script:T.x257f 67 17
            Window 15 20 80 25
            Cprint $script:T.x2594 15 20
            Gotoxy 1 4
            Cprint ('@0a(@1cF1@0a) Load Game,' + (' ' * 20) + '@0a(@1cF2@0a) Save Game') 99 0
            Window 1 1 80 25
            Cprint $script:T.x2653 20 18
            Cprint $script:T.x2680 30 25
            if ($script:status -ge 300) { break }
            Set-Color 15 0
            while ($true) {                                      # (the original flushes the keyboard on every try)
                Cursor 1
                Flush-Keys
                $key = Read-Key
                if ($key -eq 0 -and $script:KeyQueue.Count -gt 0) {          # F1 = load, F2 = save (Ctrl-L / Ctrl-S still work too)
                    $sc = $script:KeyQueue.Peek()
                    if ($sc -eq 0x3C) { [void]$script:KeyQueue.Dequeue(); $key = 19 }
                    elseif ($sc -eq 0x3B) { [void]$script:KeyQueue.Dequeue(); $key = 12 }
                }
                if ($key -ge 97 -and $key -le 122) { $key -= 32 }
                if ($key -eq 12 -or $key -eq 19) { break }                   # Ctrl-L / Ctrl-S
                if ($key -gt 0 -and 'BSJVLHQ'.IndexOf([char]$key) -ge 0) { break }
            }
            Cursor 0
            Frame 1 17 80 25
            $advance = $false

            if ($key -eq 19) {                                   # ---- Ctrl-S: save game
                [void](Save-GameDialog)
                continue
            }
            if ($key -eq 12) {                                   # ---- Ctrl-L: load game
                if (Load-GameDialog) { Draw-Main; Draw-Status }
                continue
            }
            switch ([char]$key) {
                'B' {                                            # ---- buy
                    $ok = $false
                    while ($true) {
                        Draw-DrugList
                        while ($true) {
                            Cprint $script:T.x2723 1 19
                            Set-Color 14 0
                            if (Read-Field 1 0) { break }
                            $d = [int](Atol $script:Buf)
                            if ($d -lt 1 -or $d -gt 8) { Sfx 2 }
                            if ($d -ge 1 -and $d -le 8) { $ok = $true; break }
                        }
                        break
                    }
                    if (-not $ok) { break }
                    Cursor 0
                    $d--
                    $maxBuy = IDiv $script:cash $script:price[$d]
                    Cprint $script:T.x274e 1 22
                    Set-Color 11 0
                    Cputs ([string]$maxBuy)
                    Cprint $script:T.x2771 99 99
                    Set-Color 14 0
                    $got = $false
                    while ($true) {
                        Gotoxy 27 22
                        if (Read-Field 5 0) { break }
                        $qty = [int](Atol $script:Buf)
                        if ($qty -lt 0 -or $script:Buf.Length -eq 0) { continue }
                        $got = $true; break
                    }
                    Cursor 0
                    if (-not $got) { break }
                    if ($qty -gt $maxBuy) {
                        Cprint $script:T.x277a 3 24
                        Sfx 3
                        Delay-Ms 1000
                        break
                    }
                    if ($qty -gt $script:free) {
                        Cprint $script:T.x27bd 3 24
                        Sfx 3
                        Delay-Ms 3000
                        break
                    }
                    if ($qty -gt 0) {
                        Cprint $script:T.x2801 1 25
                        Sfx 4
                        Delay-Ms 2000
                        $script:inv[$d] += $qty
                        $script:cash -= $qty * $script:price[$d]
                        if ($script:basis[$d] -eq 0) { $script:basis[$d] = $script:price[$d] -band 0xFFFF }
                        else { $script:basis[$d] = (IDiv ($script:price[$d] + $script:basis[$d]) 2) -band 0xFFFF }
                    }
                    $advance = $true
                }
                'S' {                                            # ---- sell
                    $ok = $false
                    while ($true) {
                        Draw-DrugList
                        Cprint $script:T.x2825 1 19
                        Set-Color 14 0
                        if (Read-Field 1 0) { break }
                        $d = [int](Atol $script:Buf)
                        if ($d -lt 1 -or $d -gt 8) { Sfx 2 }
                        if ($d -ge 1 -and $d -le 8) { $ok = $true; break }
                    }
                    Cursor 0
                    if (-not $ok) { break }
                    $d--
                    if ($script:inv[$d] -eq 0) {
                        Cprint $script:T.x2851 1 22
                        Sfx 3
                        Delay-Ms 1000
                        break
                    }
                    $got = $false
                    while ($true) {
                        Cprint $script:T.x2885 1 22
                        Set-Color 14 0
                        if (Read-Field 5 0) { break }
                        $qty = [int](Atol $script:Buf)
                        if ($qty -lt 0 -or $script:Buf.Length -eq 0) { continue }
                        $got = $true; break
                    }
                    Cursor 0
                    if (-not $got) { break }
                    if ($script:inv[$d] -lt $qty) {
                        Cprint $script:T.x28a3 1 24
                        Sfx 3
                        Delay-Ms 3000
                        break
                    }
                    if ($qty -gt 0) {
                        Cprint $script:T.x28f2 1 25
                        Sfx 4
                        Delay-Ms 2000
                        if ($script:inv[$d] - $qty -eq 0) {
                            if ($d -lt 3) { $w = (3 - $d) * 3 } else { $w = 1 }
                            $old = $script:status
                            $bs = [double]$script:basis[$d]; if ($bs -le 0) { $bs = [double]$script:price[$d] }
                            $gain = Ftol ((([double]$script:price[$d] / $bs) - 1.0) * 3.0 * $w)
                            $script:status += [int]$gain
                            $script:totalDeals++
                            if ($script:status -gt $old) { $script:goodDeals++ }
                            $script:basis[$d] = 0
                        }
                        $script:inv[$d] -= $qty
                        $script:cash += [long]$qty * $script:price[$d]
                    }
                    $advance = $true
                }
                'J' {                                            # ---- jet to another city
                    Draw-CityMenu
                    $si = 0
                    $esc = $false
                    while ($true) {
                        Gotoxy 1 25
                        Clreol
                        Cprint $script:T.x2699 1 25
                        if (Read-Field 2 1) { $esc = $true; break }
                        Cursor 0
                        $si = [int](Atol $script:Buf)
                        if ($si -lt 1 -or $si -gt 8) { Sfx 2 }
                        if ($si - 1 -eq $script:city) {
                            Cprint $script:T.x26af 28 25
                            Sfx 3
                            $si = 0
                            Delay-Ms 500
                        }
                        if ($si -ge 1 -and $si -le 8) { break }
                    }
                    Cursor 0
                    if ($esc) { break }
                    $cost = Travel-Cost ($si - 1) $script:city
                    if ($cost -gt $script:cash) {
                        Cprint $script:T.x26ea 25 25
                        Sfx 3
                        Delay-Ms 2000
                        break
                    }
                    $script:cash -= $cost
                    $script:city = $si - 1
                    $script:mktToday = 0
                    Gen-Prices
                    Gotoxy 30 25
                    Set-Color 15 0
                    Jet-Slide                                    # slide the city list off the screen, rising sweep
                    Draw-Status
                    for ($i = 0; $i -lt 10; $i++) { $script:done[$i] = $false }
                    $advance = $true
                }
                'V' {                                            # ---- bank
                    Cprint $script:T.x2912 20 18
                    Cursor 1
                    $k = Ask-Key ('DW' + $script:ESC)
                    if ($k -eq 27) { break }
                    if ($k -eq 68) { Cprint $script:T.x29bf 5 23 } else { Cprint $script:T.x29de 5 23 }
                    Set-Color 15 0
                    if (Read-Field 10 0) { Cursor 0; break }
                    Cursor 0
                    $amt = Atol $script:Buf
                    if ($amt -le 0) { break }
                    if ($k -eq 68) {
                        if ($amt -gt $script:cash) {
                            Cprint $script:T.x29fe 25 25
                            Sfx 3
                            Delay-Ms 1000
                        } else {
                            Cprint $script:T.x2a24 37 25
                            Sfx 2
                            Delay-Ms 2000
                            $script:cash -= $amt
                            $script:bank += $amt
                            Draw-Status
                        }
                    } else {
                        if ($amt -gt $script:bank) {
                            Cprint $script:T.x2a2c 25 25
                            Sfx 3
                            Delay-Ms 1000
                        } else {
                            Cprint $script:T.x2a53 37 25
                            Sfx 2
                            Delay-Ms 2000
                            $script:cash += $amt
                            $script:bank -= $amt
                            Draw-Status
                        }
                    }
                }
                'L' {                                            # ---- loan shark
                    Cprint $script:T.x2a5b 1 17
                    Cursor 1
                    $k = Ask-Key ('BP' + $script:ESC)
                    Cursor 0
                    if ($k -eq 27) { break }
                    if ($k -eq 66) {                             # borrow
                        if ($script:angry -eq 1) {
                            Cprint $script:T.x2bc5 5 24
                            Delay-Ms 2000
                            $advance = $true; break
                        }
                        if ($script:status -le 0) {
                            Cprint $script:T.x2c0f 5 24
                            Delay-Ms 2000
                            $advance = $true; break
                        }
                        Frame 1 21 80 25
                        Cprint $script:T.x2c54 10 22
                        if (Read-Field 10 0) { Cursor 0; break }
                        Cursor 0
                        $amt = Atol $script:Buf
                        if ($amt -le 0) { break }
                        if ($amt -lt 1000) {
                            Cprint $script:T.x2c71 1 25
                            Sfx 3
                            Delay-Ms 1000
                            break
                        }
                        if ((IDiv $amt 10000) -ge $script:status) {
                            Cprint $script:T.x2cc3 10 25
                            Sfx 3
                            Delay-Ms 1000
                            break
                        }
                        $days = [int](Ftol (([Math]::Log([double]$script:status * 10000.0) - [Math]::Log([double]($script:debt + $amt))) / 0.182321556794)) + 4
                        $refuse = $false
                        if ((IDiv ($script:debt + $amt) 10000) -gt $script:status) { $refuse = $true }
                        elseif ($days -lt $script:loanDays) { $refuse = $false }
                        elseif ($script:loanFlag -eq 1) { $refuse = $true }
                        if ($refuse) {
                            Cprint $script:T.x2d03 1 24
                            Set-Color 4 0
                            Cputs ("$($script:debt) ")
                            Set-Color 11 0
                            Cputs $script:T.x2d25
                            Cprint $script:T.x2d4f 20 25
                            Sfx 3
                            Delay-Ms 2000
                            break
                        }
                        Cprint $script:T.x2d7c 1 24
                        $script:debt += $amt
                        $script:cash += $amt
                        $script:loanFlag = 1
                        $script:loanDays = $days
                        Set-Color 4 0
                        Cputs ([string]($script:loanDays - 1))
                        Cprint $script:T.x2e05 99 99
                        Sfx 2
                        Delay-Ms 2000
                        $advance = $true
                    } else {                                     # pay back
                        if ($script:debt -eq 0) {
                            Cprint $script:T.x2e0f 1 24
                            Sfx 3
                            Delay-Ms 2000
                            break
                        }
                        Frame 1 21 80 25
                        Cprint $script:T.x2e6d 10 22
                        if (Read-Field 10 0) { Cursor 0; break }
                        Cursor 0
                        $amt = Atol $script:Buf
                        if ($amt -le 0) { break }
                        if ($amt -gt $script:cash) {
                            Cprint $script:T.x2e8c 20 25
                            Sfx 3
                            Delay-Ms 1000
                            break
                        }
                        if ($amt -ge $script:debt) { $script:debt = 0 } else { $script:debt -= $amt }
                        $script:cash -= $amt
                        if ($script:debt -gt 0) {
                            Cprint $script:T.x2eb9 20 24
                            Set-Color 12 0
                            Cputs ("$($script:debt) ")
                            Cprint $script:T.x2edb 99 99
                            if ($script:debt -gt 1) { Cputs $script:T.x2ee2 } else { Cputs $script:T.x2ee6 }
                            Sfx 3
                            Delay-Ms 2000
                        } else {
                            Cprint $script:T.x2ee9 1 24
                            $script:loanDays = 0; $script:loanFlag = 0; $script:angry = 0; $script:thugs = 0
                            Sfx 2
                            Delay-Ms 2000
                        }
                        $advance = $true
                    }
                }
                'H' {                                            # ---- hospital
                    if ($script:health -eq 100) {
                        Cprint $script:T.x2f57 15 19
                        Press-AnyKey
                        break
                    }
                    $cost = (100 - $script:health) * 10
                    Cprint $script:T.x2fc9 4 19
                    Set-Color 10 0
                    Cputs ([string]$cost)
                    Cprint $script:T.x3046 99 99
                    Cprint $script:T.x305f 5 22
                    Cursor 1
                    $k = Ask-Key ('YN' + $script:ESC)
                    Cursor 0
                    if ($k -eq 89) {
                        if ($cost -gt $script:cash) {
                            Cprint $script:T.x3088 12 24
                            Sfx 3
                            Press-AnyKey
                        } else {
                            $script:cash -= $cost
                            Cprint $script:T.x30c1 5 24
                            Set-Color 12 0
                            $stay = [int](IDiv $cost 15)
                            Cputs ([string]$stay)
                            Cprint $script:T.x30db 99 99
                            Sfx 2
                            for ($n = 1; $stay -gt $n; $n++) { Next-Day }
                            $script:health = 100
                            Press-AnyKey
                        }
                    }
                    $advance = $true
                }
                'Q' {                                            # ---- quit
                    Cprint $script:T.x310d 15 20
                    Cursor 1
                    $k = Ask-Key 'YN'
                    if ($k -eq 78) { break }
                    $playing = $false
                    $advance = $true
                }
            }

            if (-not $advance) { continue }

            # ------------------------------------------------ a day passes (0x47bf)
            Next-Day
            Draw-Status
            if ($script:dead -or -not $playing) { break }
            if ((Rand) % 10 -eq 0 -and (Rand) % ([int](IDiv $script:status 20) + 1) -gt 4) {
                if ((Rand) % 2 -eq 0) { Do-Event 7 } else { Do-Event 8 }
            }
            if ($script:copFlag -eq 1 -or ($script:status -ge 100 -and (Rand) % 400 -lt $script:status -and (Rand) % 8 -eq 0)) {
                Do-Event 0
            }
            if ($script:dead -or -not $playing) { break }
            if ((Rand) % 5 -eq 0) { Do-Event ((Rand) % 10) }
            if ($script:dead -or -not $playing) { break }
            if ($script:loanDays -lt 0 -and $script:loanFlag -eq 1) {
                Frame 1 17 80 25
                Cprint $script:T.x3149 1 17
                Delay-Ms 3000
                $script:thugs++
                for ($n = 0; $n -lt $script:thugs; $n++) {
                    Frame 1 23 80 25
                    Delay-Ms 500
                    Cprint $script:T.x329c 25 23
                    $script:health -= 25
                    $script:angry = 1
                    Draw-Status
                    Cursor 0
                    Sfx 3
                    Delay-Ms 1500
                }
                if ($script:health -gt 0) {
                    Cprint $script:T.x32c1 1 24
                    Delay-Ms 3000
                    $script:loanDays = 7
                } else { $script:dead = $true }
                Press-AnyKey
            }
            if ($script:dead -or -not $playing) { break }
            Draw-Status
        }

        # ---------------------------------------------------- the game is over
        if ($playing) {
            if ($script:dead) { Show-Death } else { Show-Win }
        }
        if (Ask-PlayAgain) {
            $playing = $true
            Init-Game
            Draw-Main
            Draw-Status
            continue
        }
        break
    }
    Clrscr
    $sub = New-Object 'byte[]' (41 * 19 * 2)                    # registered exit screen: only the NOTICE box, centred
    for ($y = 0; $y -lt 19; $y++) { [Array]::Copy($script:Scr.exit, $y * 75 * 2, $sub, $y * 41 * 2, 41 * 2) }
    Puttext 20 2 60 20 $sub
    Cprint $script:T.x3321 25 22
    Cursor 1
    Gotoxy 1 24
    Flush-Screen
    Persist-Blink
}

if ($NoSound) { $script:Muted = $true }
$script:RunSpeed = [Math]::Min(10.0, [Math]::Max(0.1, $Speed))
$script:SfxMode = $SoundMode; $script:WaveName = $Wave; $script:SfxDuty = $Duty
$script:JetSpeed = $JetSpeed; $script:SfxVol = $Volume; $script:SfxPitch = $Pitch; $script:SfxStretch = $ToneStretch
if (($SoundMode -eq 'Wave' -and -not $NoSound) -or $SoundLab -or $env:DRUGLORD_WAVEDIR) { Init-Synth }
if ($SoundLab) {
    try { Run-SoundLab } catch { if ($script:Headless) { Write-Output ('HEADLESS STOP: ' + $_.Exception.Message) } else { throw } }
    Write-Host ''
    Write-Host ('Use:  ' + (Lab-CommandLine))
    return
}
$script:SaveDirPath = $SaveDir
if (-not $script:Headless) { try { Clear-Host } catch {} }
if ($script:Headless -and $env:DRUGLORD_SEED) { $script:Seed = [long]$env:DRUGLORD_SEED }
try {
    Run-Game
} catch {
    if ($script:Headless) { Write-Output ('HEADLESS STOP: ' + $_.Exception.Message); Dump-Screen } else { throw }
} finally {
    if (-not $script:Headless) {
        try { [Console]::ResetColor(); [Console]::CursorVisible = $true; [Console]::SetCursorPosition(0, $script:WPY + $script:OY + 24); [Console]::WriteLine() } catch {}
    }
}
