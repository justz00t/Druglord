package main

import (
	"flag"
	"fmt"
	"os"
	"strconv"
	"strings"
)

const version = "v2.10"

func main() {
	nosound := flag.Bool("nosound", false, "disable all sound effects")
	speed := flag.Float64("speed", 1.0, "speed of the whole program: 1.0 = original, 2.0 = twice as fast (0.1 - 10)")
	jet := flag.Float64("jetspeed", 2.0, "speed of the 'jet to another city' slide and sweep (0.1 - 10)")
	soundmode := flag.String("soundmode", "wave", "wave (synthesised PC speaker) or beep (one Beep call per tone)")
	wave := flag.String("wave", "square", "wave shape: square, sine, saw, triangle")
	duty := flag.Float64("duty", 0.5, "square-wave pulse width 0.05 - 0.95")
	volume := flag.Float64("volume", 0.35, "sound volume 0 - 1")
	pitch := flag.Float64("pitch", 1.0, "multiplies every tone frequency (0.25 - 4)")
	stretch := flag.Float64("tonestretch", 1.0, "multiplies the length of every tone (0.1 - 10)")
	sdir := flag.String("savedir", "", "folder for save files (default: next to the program)")
	lab := flag.Bool("soundlab", false, "open the interactive sound lab")
	flag.Usage = func() {
		fmt.Fprintf(os.Stderr, "DRUGLORD %s - ported to Windows by justz00t\n\nOptions:\n", version)
		flag.PrintDefaults()
	}
	flag.Parse()

	clampF := func(v, lo, hi float64) float64 {
		if v < lo {
			return lo
		}
		if v > hi {
			return hi
		}
		return v
	}
	runSpeed = clampF(*speed, 0.1, 10)
	jetSpeed = clampF(*jet, 0.1, 10)
	sfxDuty = clampF(*duty, 0.05, 0.95)
	sfxVol = clampF(*volume, 0, 1)
	sfxPitch = clampF(*pitch, 0.25, 4)
	sfxStretch = clampF(*stretch, 0.1, 10)
	if strings.EqualFold(*soundmode, "beep") {
		sfxMode = "Beep"
	}
	switch strings.ToLower(*wave) {
	case "sine":
		waveName = "Sine"
	case "saw":
		waveName = "Saw"
	case "triangle":
		waveName = "Triangle"
	}
	muted = *nosound
	saveDir = *sdir

	headless = os.Getenv("DRUGLORD_HEADLESS") != ""
	body := func() {
		if *lab {
			runSoundLab()
			return
		}
		runGame()
	}
	if headless {
		initHeadlessKeys()
		if s := os.Getenv("DRUGLORD_SEED"); s != "" {
			if n, err := strconv.ParseInt(s, 10, 64); err == nil {
				seed = n
			}
		}
		runHeadless(body)
		return
	}
	runWindowed(body)
}

func runHeadless(body func()) {
	defer func() {
		if r := recover(); r != nil {
			if hs, ok := r.(headlessStop); ok {
				fmt.Println("HEADLESS STOP: " + string(hs))
				dumpScreen()
				return
			}
			panic(r)
		}
	}()
	body()
	if os.Getenv("DRUGLORD_DUMPEND") != "" {
		dumpScreen()
	}
}
