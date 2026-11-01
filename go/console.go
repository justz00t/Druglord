package main

import (
	"fmt"
	"os"
	"strings"
	"unicode"
	"sync"
)

// ---------------------------------------------------------------------------
//  Console layer: emulates the Borland/Turbo C conio text mode used by the
//  original (80x25 colour text, windows, gotoxy, textcolor, puttext, ...).
// ---------------------------------------------------------------------------

var (
	cb, at           [2000]int
	wx1, wy1, wx2, wy2 = 1, 1, 80, 25
	cx, cy           = 1, 1
	attr             = 7
	curVisible       = true
	anyDirty         = false
)

// what the window thread draws (a copy made by flushScreen)
type frameBuf struct {
	cb, at       [2000]int
	curX, curY   int
	curVis       bool
	ver          uint64
}

var (
	front   frameBuf
	frontMu sync.Mutex
)

func init() {
	for i := range cb {
		cb[i] = 32
		at[i] = 7
	}
	anyDirty = true
}

func flushScreen() {
	if !anyDirty {
		return
	}
	anyDirty = false
	placeCursor()
}

func placeCursor() {
	x := wx1 + cx - 2
	y := wy1 + cy - 2
	if x < 0 {
		x = 0
	}
	if x > 79 {
		x = 79
	}
	if y < 0 {
		y = 0
	}
	if y > 24 {
		y = 24
	}
	frontMu.Lock()
	front.cb = cb
	front.at = at
	front.curX, front.curY, front.curVis = x, y, curVisible
	front.ver++
	frontMu.Unlock()
}

func setCursorVisible(v bool) {
	curVisible = v
	anyDirty = true
	flushScreen()
}
func cursor(on int) { setCursorVisible(on != 0) }

func window(x1, y1, x2, y2 int) {
	if x1 < 1 || y1 < 1 || x2 > 80 || y2 > 25 || x1 > x2 || y1 > y2 {
		return
	}
	wx1, wy1, wx2, wy2 = x1, y1, x2, y2
	cx, cy = 1, 1
}

func gotoxy(x, y int) {
	if x >= 1 && y >= 1 && x <= wx2-wx1+1 && y <= wy2-wy1+1 {
		cx, cy = x, y
	}
}

func textcolor(c int)      { attr = (attr & 0x70) | (c & 0x8F) }
func textbackground(c int) { attr = (attr & 0x8F) | ((c & 7) << 4) }
func setColor(fg, bg int)  { textcolor(fg); textbackground(bg) }

func clrscr() {
	for y := wy1 - 1; y <= wy2-1; y++ {
		for x := wx1 - 1; x <= wx2-1; x++ {
			i := y*80 + x
			cb[i] = 32
			at[i] = attr
		}
	}
	anyDirty = true
	cx, cy = 1, 1
	flushScreen()
}

func scrollWindow() {
	for y := wy1; y <= wy2-1; y++ {
		for x := wx1 - 1; x <= wx2-1; x++ {
			cb[(y-1)*80+x] = cb[y*80+x]
			at[(y-1)*80+x] = at[y*80+x]
		}
	}
	for x := wx1 - 1; x <= wx2-1; x++ {
		i := (wy2-1)*80 + x
		cb[i] = 32
		at[i] = attr
	}
	anyDirty = true
}

// write one byte-code (CP437) at the cursor, honouring window rules (no flush)
func putCode(c int) {
	ww := wx2 - wx1 + 1
	wh := wy2 - wy1 + 1
	switch c {
	case 7:
		beepFx(800, 50)
		return
	case 8:
		if cx > 1 {
			cx--
		}
		return
	case 10:
		cy++
		if cy > wh {
			scrollWindow()
			cy = wh
		}
		return
	case 13:
		cx = 1
		return
	}
	x := wx1 + cx - 2
	y := wy1 + cy - 2
	i := y*80 + x
	cb[i] = c
	at[i] = attr
	anyDirty = true
	cx++
	if cx > ww {
		cx = 1
		cy++
		if cy > wh {
			scrollWindow()
			cy = wh
		}
	}
}

// cputs / printf: plain text (no colour tokens); '~' is NOT special here
func cputs(s string) {
	for i := 0; i < len(s); i++ {
		putCode(int(s[i]))
	}
	flushScreen()
}
func putch(c int) { putCode(c); flushScreen() }

// Convert a token-encoded game string to its original byte form:
//   @hh = colour code (0x91,hh)   ~ = CR LF   ^hh = raw CP437 byte   @@ ~~ ^^ = literal characters
func strBytes(s string) []byte {
	o := make([]byte, 0, len(s))
	n := len(s)
	for i := 0; i < n; {
		c := s[i]
		switch {
		case c == '@':
			if i+1 < n && s[i+1] == '@' {
				o = append(o, 64)
				i += 2
			} else {
				o = append(o, 0x91, byte(hex2(s[i+1:i+3])))
				i += 3
			}
		case c == '~':
			if i+1 < n && s[i+1] == '~' {
				o = append(o, 126)
				i += 2
			} else {
				o = append(o, 13, 10)
				i++
			}
		case c == '^':
			if i+1 < n && s[i+1] == '^' {
				o = append(o, 94)
				i += 2
			} else {
				o = append(o, byte(hex2(s[i+1:i+3])))
				i += 3
			}
		default:
			o = append(o, c)
			i++
		}
	}
	return o
}

func hex2(s string) int {
	v := 0
	for i := 0; i < 2 && i < len(s); i++ {
		c := s[i]
		v <<= 4
		switch {
		case c >= '0' && c <= '9':
			v |= int(c - '0')
		case c >= 'a' && c <= 'f':
			v |= int(c-'a') + 10
		case c >= 'A' && c <= 'F':
			v |= int(c-'A') + 10
		}
	}
	return v
}

// cprint on raw bytes: 0x91 vv = colour (vv > 0x0F => vv-0x10), background 0
func cprintBytes(b []byte, x, y int) {
	if x != 99 {
		gotoxy(x, y)
	}
	n := len(b)
	for i := 0; i < n; {
		c := int(b[i])
		if c == 0x91 && i+1 < n {
			v := int(b[i+1])
			if v > 15 {
				v -= 16
			}
			setColor(v, 0)
			i += 2
		} else {
			if c == 0 {
				break
			}
			putCode(c)
			i++
		}
	}
	flushScreen()
}
func cprint(s string, x, y int) { cprintBytes(strBytes(s), x, y) }

// cprint of a table string by key
func cpT(key string, x, y int) { cprint(T[key], x, y) }

func gettext(x1, y1, x2, y2 int) []byte {
	r := make([]byte, 0, (x2-x1+1)*(y2-y1+1)*2)
	for y := y1 - 1; y <= y2-1; y++ {
		for x := x1 - 1; x <= x2-1; x++ {
			i := y*80 + x
			r = append(r, byte(cb[i]), byte(at[i]&0xFF))
		}
	}
	return r
}

func clreol() {
	for x := cx; x <= wx2-wx1+1; x++ {
		i := (wy1+cy-2)*80 + wx1 + x - 2
		cb[i] = 32
		at[i] = attr
	}
	anyDirty = true
	flushScreen()
}

// blit a screen stored as (char, attr) byte pairs; absolute coords like puttext()
func puttext(x1, y1, x2, y2 int, buf []byte) {
	k := 0
	for y := y1 - 1; y <= y2-1; y++ {
		for x := x1 - 1; x <= x2-1; x++ {
			i := y*80 + x
			cb[i] = int(buf[k])
			at[i] = int(buf[k+1])
			k += 2
		}
	}
	anyDirty = true
	flushScreen()
}

// test aid (headless only): print the shadow screen
func dumpScreen() {
	var sb strings.Builder
	for y := 0; y < 25; y++ {
		var line strings.Builder
		for x := 0; x < 80; x++ {
			line.WriteRune(cp437[cb[y*80+x]])
		}
		sb.WriteString(strings.TrimRightFunc(line.String(), unicode.IsSpace))
		sb.WriteString("\n")
	}
	fmt.Fprint(os.Stdout, sb.String())
}
