package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"math"
	"os"
	"path/filepath"
	"time"
)

// ---------------------------------------------------------------------------
//  PC-speaker emulation.  Each sound effect is rendered once into a WAV in memory
//  (continuous square wave, exact lengths, no gaps) and played in one piece.
//  -soundmode beep falls back to one Beep() call per tone.
// ---------------------------------------------------------------------------

var (
	sfxMode    = "Wave"
	waveName   = "Square"
	sfxDuty    = 0.5
	sfxVol     = 0.35
	sfxPitch   = 1.0
	sfxStretch = 1.0
	runSpeed   = 1.0
	jetSpeed   = 2.0
	toneMsBase = 31 // original: calibrated against the BIOS clock
	muted      = false

	sfxCache = map[string][]byte{}
	rec      bool
	recF     []float64
	recM     []float64
)

// platform hooks (set by the Windows build)
var (
	playWavHook func(wav []byte, async bool)
	beepHook    func(freq, ms int)
)

func clearSoundCache() { sfxCache = map[string][]byte{} }

func render(f, ms []float64, rate int, vol float64, wave int, duty float64) []byte {
	n := make([]int, len(ms))
	total := 0
	for i := range ms {
		n[i] = int(math.RoundToEven(ms[i] * float64(rate) / 1000.0))
		if n[i] < 1 {
			n[i] = 1
		}
		total += n[i]
	}
	s := make([]int16, total)
	p := 0
	ph := 0.0
	amp := vol * 32767.0
	for i := range f {
		if f[i] <= 0 {
			p += n[i]
			ph = 0
			continue
		}
		step := f[i] / float64(rate)
		for k := 0; k < n[i]; k++ {
			var v float64
			switch wave {
			case 1:
				v = math.Sin(2 * math.Pi * ph)
			case 2:
				v = 2*ph - 1
			case 3:
				if ph < 0.5 {
					v = 4*ph - 1
				} else {
					v = 3 - 4*ph
				}
			default:
				if ph < duty {
					v = 1
				} else {
					v = -1
				}
			}
			s[p] = int16(v * amp)
			p++
			ph += step
			if ph >= 1 {
				ph -= 1
			}
		}
	}
	var b bytes.Buffer
	dataLen := total * 2
	b.WriteString("RIFF")
	binary.Write(&b, binary.LittleEndian, int32(36+dataLen))
	b.WriteString("WAVEfmt ")
	binary.Write(&b, binary.LittleEndian, int32(16))
	binary.Write(&b, binary.LittleEndian, int16(1))
	binary.Write(&b, binary.LittleEndian, int16(1))
	binary.Write(&b, binary.LittleEndian, int32(rate))
	binary.Write(&b, binary.LittleEndian, int32(rate*2))
	binary.Write(&b, binary.LittleEndian, int16(2))
	binary.Write(&b, binary.LittleEndian, int16(16))
	b.WriteString("data")
	binary.Write(&b, binary.LittleEndian, int32(dataLen))
	binary.Write(&b, binary.LittleEndian, s)
	return b.Bytes()
}

func renderWav(f, m []float64) []byte {
	w := 0
	switch waveName {
	case "Sine":
		w = 1
	case "Saw":
		w = 2
	case "Triangle":
		w = 3
	}
	return render(f, m, 44100, sfxVol, w, sfxDuty)
}

// scaled values shared by the wave and beep paths
func toneMs(ms int) int {
	v := int(math.RoundToEven(float64(ms) * sfxStretch / runSpeed))
	if v < 1 {
		v = 1
	}
	return v
}
func toneHz(f int) int {
	v := int(math.RoundToEven(float64(f) * sfxPitch))
	if v < 1 {
		v = 1
	}
	if v > 20000 {
		v = 20000
	}
	return v
}

func sleepMs(ms int) {
	if ms > 0 {
		time.Sleep(time.Duration(ms) * time.Millisecond)
	}
}

func beepFx(freq, ms int) { // sound(freq); delay(ms); nosound()
	if headless {
		return
	}
	ms = toneMs(ms)
	if muted || freq < 37 {
		sleepMs(ms)
		return
	}
	f := toneHz(freq)
	if sfxMode == "Wave" && playWavHook != nil {
		key := fmt.Sprintf("t%d,%d", f, ms)
		w, ok := sfxCache[key]
		if !ok {
			w = renderWav([]float64{float64(f)}, []float64{float64(ms)})
			sfxCache[key] = w
		}
		playWavHook(w, false)
		return
	}
	if f > 32767 {
		f = 32767
	}
	if beepHook != nil {
		beepHook(f, ms)
	} else {
		sleepMs(ms)
	}
}

func tone(f, ms int) {
	if rec {
		if f < 37 {
			recF = append(recF, 0)
		} else {
			recF = append(recF, float64(toneHz(f)))
		}
		recM = append(recM, float64(toneMs(ms)))
		return
	}
	beepFx(f, ms)
}

func restMs(ms int) {
	if rec {
		v := math.Max(math.RoundToEven(float64(ms)/runSpeed), 1)
		recF = append(recF, 0)
		recM = append(recM, v)
		return
	}
	if !headless {
		v := int(math.Max(math.RoundToEven(float64(ms)/runSpeed), 1))
		sleepMs(v)
	}
}

func recordSfx(id int) {
	recF, recM = nil, nil
	rec = true
	func() {
		defer func() { rec = false }()
		sfxBody(id)
	}()
}

func sfx(id int) {
	if headless {
		if dir := os.Getenv("DRUGLORD_WAVEDIR"); dir != "" { // test hook: write the rendered effect to a file
			recordSfx(id)
			if len(recF) > 0 {
				os.WriteFile(filepath.Join(dir, fmt.Sprintf("sfx%d.wav", id)), renderWav(recF, recM), 0644)
			}
		}
		return
	}
	if sfxMode == "Wave" && playWavHook != nil && !muted {
		key := fmt.Sprintf("s%d", id)
		w, ok := sfxCache[key]
		if !ok {
			recordSfx(id)
			if len(recF) == 0 {
				return
			}
			w = renderWav(recF, recM)
			sfxCache[key] = w
		}
		playWavHook(w, false)
		return
	}
	sfxBody(id)
}

func sfxBody(id int) {
	switch id {
	case 2:
		tone(2000, 10); tone(5000, 20); tone(500, 30); tone(1000, 40)
	case 3:
		for si := 500; si > 200; si -= 100 {
			tone(si, 100); tone(si-200, 100); restMs(50)
		}
	case 4:
		for si := 500; si < 1000; si += 100 {
			tone(si, 10); tone(si+2000, 20); restMs(10)
		}
	case 5:
		for k := 0; k < 6; k++ {
			for si := 7; si < 15; si++ {
				tone(si*100, 25)
			}
			for si := 15; si > 7; si-- {
				tone(si*100, 25)
			}
		}
	case 6:
		for si := 0; si < 5; si++ {
			for di := 0; di < 10; di++ {
				tone(si*1000+di*1000, 40)
			}
		}
	case 7:
		for si := 100; si > 0; si-- {
			tone(5000-si*50, 5); tone(si*50+5000, 5)
		}
	case 8:
		tone(2000, 30); tone(500, 25); tone(70, 40)
	case 11:
		tone(1000, 50)
	case 12:
		tone(500, 30); tone(200, 20)
	case 13:
		tone(100, 30); tone(50, 30)
	case 14:
		tone(150, 30); tone(100, 50)
	case 15:
		for si := 1; si < 3; si++ {
			tone(100, 20); tone(50, 60)
		}
	case 20:
		tone(800, 100)
	}
}

// Jet to another city: the city list slides off to the left while a rising sweep plays.
// The sweep is one continuous sound played in the background; the slide is paced by the
// clock (skipping frames if drawing is slow) so picture and sound stay together.
func jetSlide(soundOnly bool) {
	var xs []int
	for x := 79; x > 1; x -= 2 {
		xs = append(xs, x)
	}
	n := len(xs)
	step := int(math.Max(math.RoundToEven(float64(toneMsBase)/jetSpeed/runSpeed), 1))
	async := false
	if !headless && sfxMode == "Wave" && playWavHook != nil && !muted {
		key := fmt.Sprintf("jet%d", step)
		w, ok := sfxCache[key]
		if !ok {
			var fl, ml []float64
			for _, x := range xs {
				fl = append(fl, float64(toneHz(1000+(79-x)*20)))
				ml = append(ml, float64(step))
			}
			w = renderWav(fl, ml)
			sfxCache[key] = w
		}
		playWavHook(w, true)
		async = true
	}
	shift := func(x int) {
		if !soundOnly {
			buf := gettext(3, 19, x, 22)
			puttext(1, 19, x-2, 22, buf)
		}
	}
	if async {
		start := time.Now()
		done := 0
		for done < n {
			target := int(time.Since(start).Milliseconds()/int64(step)) + 1
			if target > n {
				target = n
			}
			if target > done {
				for done < target {
					shift(xs[done])
					done++
				}
				if !soundOnly {
					flushScreen()
				}
			} else {
				time.Sleep(time.Millisecond)
			}
		}
		for time.Since(start) < time.Duration(n*step)*time.Millisecond {
			time.Sleep(time.Millisecond)
		}
	} else {
		for _, x := range xs {
			shift(x)
			d := int(math.Max(math.RoundToEven(float64(toneMsBase)/jetSpeed), 1))
			tone(1000+(79-x)*20, d)
		}
	}
}
