//go:build windows

package main

import (
	"fmt"
	"os"
	"path/filepath"
	"runtime"
	"runtime/debug"
	"syscall"
	"time"
	"unsafe"
)

var (
	user32   = syscall.NewLazyDLL("user32.dll")
	gdi32    = syscall.NewLazyDLL("gdi32.dll")
	kernel32 = syscall.NewLazyDLL("kernel32.dll")
	winmm    = syscall.NewLazyDLL("winmm.dll")

	pRegisterClassExW   = user32.NewProc("RegisterClassExW")
	pCreateWindowExW    = user32.NewProc("CreateWindowExW")
	pDefWindowProcW     = user32.NewProc("DefWindowProcW")
	pGetMessageW        = user32.NewProc("GetMessageW")
	pTranslateMessage   = user32.NewProc("TranslateMessage")
	pDispatchMessageW   = user32.NewProc("DispatchMessageW")
	pPostQuitMessage    = user32.NewProc("PostQuitMessage")
	pPostMessageW       = user32.NewProc("PostMessageW")
	pBeginPaint         = user32.NewProc("BeginPaint")
	pEndPaint           = user32.NewProc("EndPaint")
	pGetClientRect      = user32.NewProc("GetClientRect")
	pInvalidateRect     = user32.NewProc("InvalidateRect")
	pSetTimer           = user32.NewProc("SetTimer")
	pLoadCursorW        = user32.NewProc("LoadCursorW")
	pFillRect           = user32.NewProc("FillRect")
	pAdjustWindowRect   = user32.NewProc("AdjustWindowRect")
	pSetProcessDPIAware = user32.NewProc("SetProcessDPIAware")
	pMessageBoxW        = user32.NewProc("MessageBoxW")
	pSetWindowTextW     = user32.NewProc("SetWindowTextW")
	pShowWindow         = user32.NewProc("ShowWindow")

	pCreateCompatibleDC     = gdi32.NewProc("CreateCompatibleDC")
	pCreateCompatibleBitmap = gdi32.NewProc("CreateCompatibleBitmap")
	pSelectObject           = gdi32.NewProc("SelectObject")
	pDeleteObject           = gdi32.NewProc("DeleteObject")
	pDeleteDC               = gdi32.NewProc("DeleteDC")
	pBitBlt                 = gdi32.NewProc("BitBlt")
	pStretchBlt             = gdi32.NewProc("StretchBlt")
	pSetStretchBltMode      = gdi32.NewProc("SetStretchBltMode")
	pCreateSolidBrush       = gdi32.NewProc("CreateSolidBrush")
	pCreateBitmap           = gdi32.NewProc("CreateBitmap")
	pCreateFontW            = gdi32.NewProc("CreateFontW")
	pSetTextColor           = gdi32.NewProc("SetTextColor")
	pSetBkColor             = gdi32.NewProc("SetBkColor")
	pSetBkMode              = gdi32.NewProc("SetBkMode")
	pExtTextOutW            = gdi32.NewProc("ExtTextOutW")

	pBeep      = kernel32.NewProc("Beep")
	pGetModule = kernel32.NewProc("GetModuleHandleW")
	pPlaySound = winmm.NewProc("PlaySoundW")
)

const (
	wmDestroy    = 0x0002
	wmSize       = 0x0005
	wmPaint      = 0x000F
	wmClose      = 0x0010
	wmEraseBkgnd = 0x0014
	wmKeyDown    = 0x0100
	wmChar       = 0x0102
	wmTimer      = 0x0113

	wsOverlappedWindow = 0x00CF0000
	wsVisible          = 0x10000000
	csHRedraw          = 0x0002
	csVRedraw          = 0x0001
	cwUseDefault       = 0x80000000
)

type rect struct{ left, top, right, bottom int32 }
type point struct{ x, y int32 }
type wndClassEx struct {
	size       uint32
	style      uint32
	wndProc    uintptr
	clsExtra   int32
	wndExtra   int32
	instance   uintptr
	icon       uintptr
	cursor     uintptr
	background uintptr
	menuName   *uint16
	className  *uint16
	iconSm     uintptr
}
type msgT struct {
	hwnd    uintptr
	message uint32
	wParam  uintptr
	lParam  uintptr
	time    uint32
	pt      point
	priv    uint32
}
type paintStruct struct {
	hdc      uintptr
	erase    int32
	rcPaint  rect
	restore  int32
	incUpd   int32
	reserved [32]byte
}

// VGA palette as COLORREF (0x00BBGGRR)
var vga = [16]uint32{
	0x000000, 0xAA0000, 0x00AA00, 0xAAAA00, 0x0000AA, 0xAA00AA, 0x0055AA, 0xAAAAAA,
	0x555555, 0xFF5555, 0x55FF55, 0xFFFF55, 0x5555FF, 0xFF55FF, 0x55FFFF, 0xFFFFFF,
}

var (
	hwndMain     uintptr
	brushes      [16]uintptr
	shadeBmp     [3]uintptr
	fontH        uintptr
	fontCW       int
	lastVer      uint64 = ^uint64(0)
	lastPhase    int    = -1
	lastW, lastH int
	asyncKeep    []byte
	gameOver     bool
)

func u16(s string) *uint16 { p, _ := syscall.UTF16PtrFromString(s); return p }

func runWindowed(body func()) {
	runtime.LockOSThread()
	pSetProcessDPIAware.Call()

	playWavHook = func(wav []byte, async bool) {
		if len(wav) == 0 {
			return
		}
		flags := uintptr(0x4 | 0x2) // SND_MEMORY | SND_NODEFAULT
		if async {
			asyncKeep = wav
			flags |= 0x1
		}
		pPlaySound.Call(uintptr(unsafe.Pointer(&wav[0])), 0, flags)
		runtime.KeepAlive(wav)
	}
	beepHook = func(freq, ms int) {
		if freq < 37 {
			freq = 37
		}
		if freq > 32767 {
			freq = 32767
		}
		if ms < 1 {
			return
		}
		pBeep.Call(uintptr(freq), uintptr(ms))
	}

	for i, c := range vga {
		r, _, _ := pCreateSolidBrush.Call(uintptr(c))
		brushes[i] = r
	}
	// shade bitmaps: monochrome, 1 = background colour, 0 = text colour, so patterns are inverted
	pats := [3][2]byte{{0x22, 0x88}, {0xAA, 0x55}, {0xDD, 0x77}}
	for i := 0; i < 3; i++ {
		var d [32]byte
		for row := 0; row < 16; row++ {
			p := pats[i][row&1]
			d[row*2] = ^p
			d[row*2+1] = 0xFF
		}
		r, _, _ := pCreateBitmap.Call(8, 16, 1, 1, uintptr(unsafe.Pointer(&d[0])))
		shadeBmp[i] = r
		runtime.KeepAlive(&d)
	}

	inst, _, _ := pGetModule.Call(0)
	cur, _, _ := pLoadCursorW.Call(0, 32512)
	cls := u16("DrugLordWnd")
	wc := wndClassEx{
		style:     csHRedraw | csVRedraw,
		wndProc:   syscall.NewCallback(wndProc),
		instance:  inst,
		cursor:    cur,
		className: cls,
	}
	wc.size = uint32(unsafe.Sizeof(wc))
	pRegisterClassExW.Call(uintptr(unsafe.Pointer(&wc)))

	rc := rect{0, 0, 960, 600}
	pAdjustWindowRect.Call(uintptr(unsafe.Pointer(&rc)), wsOverlappedWindow, 0)
	hw, _, _ := pCreateWindowExW.Call(0, uintptr(unsafe.Pointer(cls)), uintptr(unsafe.Pointer(u16("DRUGLORD "+version))),
		wsOverlappedWindow|wsVisible, cwUseDefault, cwUseDefault,
		uintptr(rc.right-rc.left), uintptr(rc.bottom-rc.top), 0, 0, inst, 0)
	if hw == 0 {
		os.Exit(1)
	}
	hwndMain = hw
	pSetTimer.Call(hw, 1, 40, 0)
	pShowWindow.Call(hw, 5)

	go func() {
		defer func() {
			if r := recover(); r != nil {
				reportCrash(r)
				os.Exit(1)
			}
		}()
		body()
		flushScreen()
		gameOver = true
		pSetWindowTextW.Call(hwndMain, uintptr(unsafe.Pointer(u16("DRUGLORD "+version+"  -  press any key to close"))))
		flushKeys()
		readKey()
		pPostMessageW.Call(hwndMain, wmClose, 0, 0)
	}()

	var m msgT
	for {
		r, _, _ := pGetMessageW.Call(uintptr(unsafe.Pointer(&m)), 0, 0, 0)
		if int32(r) <= 0 {
			break
		}
		pTranslateMessage.Call(uintptr(unsafe.Pointer(&m)))
		pDispatchMessageW.Call(uintptr(unsafe.Pointer(&m)))
	}
	os.Exit(0)
}

func reportCrash(r interface{}) {
	txt := fmt.Sprintf("DRUGLORD %s crashed: %v\n\n%s", version, r, debug.Stack())
	exe, _ := os.Executable()
	_ = os.WriteFile(filepath.Join(filepath.Dir(exe), "druglord_error.txt"), []byte(txt), 0644)
	short := fmt.Sprintf("DRUGLORD hit an error and has to close:\n\n%v\n\n(Details were saved to druglord_error.txt next to the program.)", r)
	pMessageBoxW.Call(0, uintptr(unsafe.Pointer(u16(short))), uintptr(unsafe.Pointer(u16("DRUGLORD"))), 0x10)
}

func wndProc(hwnd, msg, wp, lp uintptr) uintptr {
	switch msg {
	case wmEraseBkgnd:
		return 1
	case wmPaint:
		var ps paintStruct
		hdc, _, _ := pBeginPaint.Call(hwnd, uintptr(unsafe.Pointer(&ps)))
		var rc rect
		pGetClientRect.Call(hwnd, uintptr(unsafe.Pointer(&rc)))
		paintAll(hdc, int(rc.right), int(rc.bottom))
		pEndPaint.Call(hwnd, uintptr(unsafe.Pointer(&ps)))
		return 0
	case wmSize:
		pInvalidateRect.Call(hwnd, 0, 0)
		return 0
	case wmTimer:
		frontMu.Lock()
		v := front.ver
		frontMu.Unlock()
		ph := int(time.Now().UnixMilli()/500) & 1
		if v != lastVer || ph != lastPhase {
			pInvalidateRect.Call(hwnd, 0, 0)
		}
		return 0
	case wmChar:
		if wp < 128 {
			pushKey(int(wp))
		}
		return 0
	case wmKeyDown:
		sc := 0
		switch wp {
		case 0x70:
			sc = 0x3B
		case 0x71:
			sc = 0x3C
		case 0x26:
			sc = 0x48
		case 0x28:
			sc = 0x50
		case 0x25:
			sc = 0x4B
		case 0x27:
			sc = 0x4D
		case 0x24:
			sc = 0x47
		case 0x23:
			sc = 0x4F
		case 0x21:
			sc = 0x49
		case 0x22:
			sc = 0x51
		case 0x2D:
			sc = 0x52
		case 0x2E:
			sc = 0x53
		}
		if sc != 0 {
			pushKey(0, sc)
			return 0
		}
	case wmDestroy:
		pPostQuitMessage.Call(0)
		return 0
	}
	r, _, _ := pDefWindowProcW.Call(hwnd, msg, wp, lp)
	return r
}

func fillR(dc uintptr, x0, y0, x1, y1 int, br uintptr) {
	if x1 <= x0 || y1 <= y0 {
		return
	}
	r := rect{int32(x0), int32(y0), int32(x1), int32(y1)}
	pFillRect.Call(dc, uintptr(unsafe.Pointer(&r)), br)
}

func paintAll(hdc uintptr, w, h int) {
	if w < 1 || h < 1 {
		return
	}
	frontMu.Lock()
	f := front
	frontMu.Unlock()
	lastVer = f.ver
	phase := int(time.Now().UnixMilli()/500) & 1
	lastPhase = phase

	mdc, _, _ := pCreateCompatibleDC.Call(hdc)
	bmp, _, _ := pCreateCompatibleBitmap.Call(hdc, uintptr(w), uintptr(h))
	old, _, _ := pSelectObject.Call(mdc, bmp)
	fillR(mdc, 0, 0, w, h, brushes[0])

	cw := w / 80
	if h/50 < cw {
		cw = h / 50
	}
	if cw < 2 {
		cw = 2
	}
	ch := cw * 2
	ox := (w - 80*cw) / 2
	oy := (h - 25*ch) / 2
	if ox < 0 {
		ox = 0
	}
	if oy < 0 {
		oy = 0
	}
	if fontH == 0 || fontCW != cw {
		if fontH != 0 {
			pDeleteObject.Call(fontH)
		}
		eh := ch * 9 / 10
		fh, _, _ := pCreateFontW.Call(uintptr(int32(-eh)), uintptr(cw), 0, 0, 400, 0, 0, 0, 1, 0, 0, 4, 0x31,
			uintptr(unsafe.Pointer(u16("Consolas"))))
		fontH = fh
		fontCW = cw
	}
	oldF, _, _ := pSelectObject.Call(mdc, fontH)
	pSetBkMode.Call(mdc, 1)
	pSetStretchBltMode.Call(mdc, 3) // COLORONCOLOR
	sdc, _, _ := pCreateCompatibleDC.Call(hdc)
	dxv := int32(cw)

	for row := 0; row < 25; row++ {
		for col := 0; col < 80; col++ {
			i := row*80 + col
			code := f.cb[i] & 0xFF
			a := f.at[i]
			fg := a & 15
			bg := (a >> 4) & 7
			if a&0x80 != 0 && phase == 1 {
				fg = bg
			}
			x := ox + col*cw
			y := oy + row*ch
			fillR(mdc, x, y, x+cw, y+ch, brushes[bg])
			if fg == bg || code == 32 || code == 0 || code == 255 {
				continue
			}
			switch {
			case code >= 0xB0 && code <= 0xB2:
				pSelectObject.Call(sdc, shadeBmp[code-0xB0])
				pSetTextColor.Call(mdc, uintptr(vga[fg]))
				pSetBkColor.Call(mdc, uintptr(vga[bg]))
				pStretchBlt.Call(mdc, uintptr(x), uintptr(y), uintptr(cw), uintptr(ch), sdc, 0, 0, 8, 16, 0x00CC0020)
			case code == 0xDB:
				fillR(mdc, x, y, x+cw, y+ch, brushes[fg])
			case code == 0xDC:
				fillR(mdc, x, y+ch/2, x+cw, y+ch, brushes[fg])
			case code == 0xDF:
				fillR(mdc, x, y, x+cw, y+ch/2, brushes[fg])
			case code == 0xDD:
				fillR(mdc, x, y, x+cw/2, y+ch, brushes[fg])
			case code == 0xDE:
				fillR(mdc, x+cw/2, y, x+cw, y+ch, brushes[fg])
			default:
				if bt, ok := boxTab[byte(code)]; ok {
					drawBox(mdc, x, y, cw, ch, bt, brushes[fg])
				} else {
					ru := uint16(cp437[code])
					pSetTextColor.Call(mdc, uintptr(vga[fg]))
					pExtTextOutW.Call(mdc, uintptr(x), uintptr(y+ch/20), 0, 0, uintptr(unsafe.Pointer(&ru)), 1, uintptr(unsafe.Pointer(&dxv)))
				}
			}
		}
	}
	if f.curVis && phase == 0 && !gameOver {
		i := f.curY*80 + f.curX
		fg := f.at[i] & 15
		t := ch / 8
		if t < 2 {
			t = 2
		}
		x := ox + f.curX*cw
		y := oy + f.curY*ch
		fillR(mdc, x, y+ch-t, x+cw, y+ch, brushes[fg])
	}

	pBitBlt.Call(hdc, 0, 0, uintptr(w), uintptr(h), mdc, 0, 0, 0x00CC0020)
	pSelectObject.Call(mdc, oldF)
	pSelectObject.Call(mdc, old)
	pDeleteObject.Call(bmp)
	pDeleteDC.Call(sdc)
	pDeleteDC.Call(mdc)
}

func imax(a, b int) int {
	if a > b {
		return a
	}
	return b
}

// drawBox draws a CP437 line-drawing character; k = {up, down, left, right}, each 0 none / 1 single / 2 double
func drawBox(dc uintptr, x0, y0, cw, ch int, k [4]uint8, br uintptr) {
	up, dn, lf, rt := int(k[0]), int(k[1]), int(k[2]), int(k[3])
	u := ch / 16
	if u < 1 {
		u = 1
	}
	cx, cy := cw/2, ch/2
	ext := func(c, kind int) (int, int) {
		switch kind {
		case 0:
			return c, c
		case 1:
			return c - u/2, c - u/2 + u
		}
		return c - 2*u, c + 2*u
	}
	bands := func(c, kind int) [][2]int {
		switch kind {
		case 1:
			a := c - u/2
			return [][2]int{{a, a + u}}
		case 2:
			return [][2]int{{c - 2*u, c - u}, {c + u, c + 2*u}}
		}
		return nil
	}
	fill := func(xa, ya, xb, yb int) { fillR(dc, x0+xa, y0+ya, x0+xb, y0+yb, br) }
	vm := imax(up, dn)
	hm := imax(lf, rt)
	vx0, vx1 := ext(cx, vm)
	hy0, hy1 := ext(cy, hm)

	for side := 0; side < 2; side++ {
		kk := lf
		if side == 1 {
			kk = rt
		}
		for bi, b := range bands(cy, kk) {
			xa, xb := 0, vx1
			if side == 1 {
				xa, xb = vx0, cw
			}
			inner := (kk == 2 && vm == 2 && ((bi == 0 && up > 0) || (bi == 1 && dn > 0))) || (kk == 1 && vm == 2)
			if inner {
				if side == 0 {
					xb = vx0 + u
				} else {
					xa = vx1 - u
				}
			}
			fill(xa, b[0], xb, b[1])
		}
	}
	for side := 0; side < 2; side++ {
		kk := up
		if side == 1 {
			kk = dn
		}
		for bi, b := range bands(cx, kk) {
			ya, yb := 0, hy1
			if side == 1 {
				ya, yb = hy0, ch
			}
			inner := (kk == 2 && hm == 2 && ((bi == 0 && lf > 0) || (bi == 1 && rt > 0))) || (kk == 1 && hm == 2)
			if inner {
				if side == 0 {
					yb = hy0 + u
				} else {
					ya = hy1 - u
				}
			}
			fill(b[0], ya, b[1], yb)
		}
	}
}
