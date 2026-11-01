package main

import (
	"fmt"
	"math"
	"strings"
)

// ---------------------------------------------------------------------------
//  Sound lab (-soundlab): play every effect and tweak the sound settings live
// ---------------------------------------------------------------------------
type labFx struct {
	id   int
	name string
}

var labEffects = []labFx{
	{5, "Cop siren (start of a fight)"}, {2, "Bad input buzz"}, {3, "Penalty / hit"}, {4, "Success chirp"},
	{6, "Rising sweep"}, {7, "Warble"}, {8, "Gun click"},
	{11, "Gunshot: type 1"}, {12, "Gunshot: type 2"}, {13, "Gunshot: type 3"}, {14, "Gunshot: type 4"}, {15, "Gunshot: type 5"},
	{20, "Menu beep"}, {99, "Jet to another city (sweep)"},
}
var labSettings = []string{"SoundMode", "Wave", "Duty", "Volume", "Pitch", "ToneStretch", "JetSpeed", "Speed"}

func labCommandLine() string {
	a := []string{"DrugLord.exe"}
	if sfxMode != "Wave" {
		a = append(a, "-soundmode "+strings.ToLower(sfxMode))
	}
	if waveName != "Square" {
		a = append(a, "-wave "+strings.ToLower(waveName))
	}
	add := func(name string, v, def float64) {
		if math.Abs(v-def) > 0.001 {
			a = append(a, fmt.Sprintf("-%s %.4g", name, v))
		}
	}
	add("duty", sfxDuty, 0.5)
	add("volume", sfxVol, 0.35)
	add("pitch", sfxPitch, 1)
	add("tonestretch", sfxStretch, 1)
	add("jetspeed", jetSpeed, 2)
	add("speed", runSpeed, 1)
	return strings.Join(a, " ")
}

func labValue(name string) string {
	switch name {
	case "SoundMode":
		return sfxMode
	case "Wave":
		return waveName
	case "Duty":
		return fmt.Sprintf("%.2f", sfxDuty)
	case "Volume":
		return fmt.Sprintf("%.2f", sfxVol)
	case "Pitch":
		return fmt.Sprintf("%.2f", sfxPitch)
	case "ToneStretch":
		return fmt.Sprintf("%.2f", sfxStretch)
	case "JetSpeed":
		return fmt.Sprintf("%.2f", jetSpeed)
	case "Speed":
		return fmt.Sprintf("%.2f", runSpeed)
	}
	return ""
}

func clamp(v, lo, hi float64) float64 { return math.Round(math.Min(hi, math.Max(lo, v))*100) / 100 }

func labPlay(id int) {
	if id == 99 {
		jetSlide(true)
	} else {
		sfx(id)
	}
}

func runSoundLab() {
	total := len(labEffects) + len(labSettings)
	sel, last := 0, -1
	msg := ""
	waves := []string{"Square", "Sine", "Saw", "Triangle"}
	for {
		cursor(0)
		window(1, 1, 80, 25)
		setColor(7, 0)
		clrscr()
		setColor(14, 0)
		cputsAt("DRUGLORD SOUND LAB", 2, 1)
		setColor(7, 0)
		cputsAt("Up/Down = move   Enter/Space = play or change   Left/Right = adjust a setting", 2, 2)
		cputsAt("A = play all   R = replay last   Q / Esc = quit", 2, 3)
		for i := 0; i < total; i++ {
			var x, y int
			var line string
			if i < len(labEffects) {
				x, y = 2, 5+i
				line = fmt.Sprintf("Effect %-3d %s", labEffects[i].id, labEffects[i].name)
			} else {
				x, y = 47, 5+(i-len(labEffects))
				line = fmt.Sprintf("%-12s %s", labSettings[i-len(labEffects)], labValue(labSettings[i-len(labEffects)]))
			}
			if i == sel {
				setColor(11, 0)
				cputsAt("> ", x-2, y)
				cputs(line)
			} else {
				setColor(7, 0)
				cputsAt("  ", x-2, y)
				cputs(line)
			}
		}
		if msg != "" {
			setColor(10, 0)
			cputsAt(msg, 2, 21)
		}
		setColor(15, 0)
		cputsAt("Command line:", 2, 23)
		setColor(14, 0)
		cputsAt(labCommandLine(), 2, 24)
		k := readKey()
		msg = ""
		if k == 0 {
			k = 1000 + readKey() // extended key: 1000 + scan code
		}
		if k >= 97 && k <= 122 {
			k -= 32
		}
		play := -1
		switch {
		case k == 1072:
			sel = (sel + total - 1) % total
		case k == 1080:
			sel = (sel + 1) % total
		case k == 13 || k == 32 || k == 1077 || k == 1075:
			if sel < len(labEffects) {
				if k == 13 || k == 32 {
					play = sel
				}
			} else {
				dir := 1.0
				if k == 1075 {
					dir = -1
				}
				switch labSettings[sel-len(labEffects)] {
				case "SoundMode":
					if sfxMode == "Wave" {
						sfxMode = "Beep"
					} else {
						sfxMode = "Wave"
					}
				case "Wave":
					ix := 0
					for i, w := range waves {
						if w == waveName {
							ix = i
						}
					}
					waveName = waves[(ix+int(dir)+4)%4]
				case "Duty":
					sfxDuty = clamp(sfxDuty+0.05*dir, 0.05, 0.95)
				case "Volume":
					sfxVol = clamp(sfxVol+0.05*dir, 0, 1)
				case "Pitch":
					sfxPitch = clamp(sfxPitch+0.05*dir, 0.25, 4)
				case "ToneStretch":
					sfxStretch = clamp(sfxStretch+0.1*dir, 0.1, 10)
				case "JetSpeed":
					jetSpeed = clamp(jetSpeed+0.1*dir, 0.1, 10)
				case "Speed":
					runSpeed = clamp(runSpeed+0.1*dir, 0.1, 10)
				}
				clearSoundCache()
				msg = "Changed - press R to replay the last effect."
			}
		case k == 'A':
			play = -2
		case k == 'R':
			if last >= 0 {
				play = last
			}
		case k == 'Q' || k == 27:
			return
		}
		if play >= 0 {
			last = play
			msg = fmt.Sprintf("Played effect %d.", labEffects[play].id)
			cputsAt(msg, 2, 21)
			labPlay(labEffects[play].id)
		} else if play == -2 {
			for _, e := range labEffects {
				labPlay(e.id)
				delayMs(250)
			}
			msg = "Played all."
		}
	}
}
