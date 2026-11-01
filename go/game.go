package main

import (
	"math"
	"os"
	"strconv"
	"strings"
)

// ---------------------------------------------------------------------------
//  Title, setup, endings
// ---------------------------------------------------------------------------
func showTitle() { // f_32aa
	cursor(0)
	window(1, 1, 80, 25)
	setColor(7, 0)
	clrscr()
	setColor(14, 0)
	puttext(10, 5, 71, 10, scr["title"])
	cpT("x24c5", 35, 13)
	cprint("@01^10@09^10@0b^10     @1eREGISTERED VERSION    @0b^11@09^11@01^11", 24, 16)
	setColor(13, 0); cprint("Registered To: ", 8, 17)
	setColor(15, 2); cprint(padR("justz00t", 25), 23, 17)
	setColor(13, 0); cprint("Serial Number: ", 50, 17)
	setColor(15, 2); cprint("000001", 65, 17)
	cpT("x2504", 30, 19)
	cpT("x251c", 23, 20)
	cprint("@0cPorted to Windows by justz00t", 26, 21)
	delayMs(500)
	if headless {
		return
	}
	title := scr["title"]
	flag := 1
	outer := 0
	for outer < 10 { // shimmer the block letters in random colours
		count := 0
		flag = b2i(flag == 0)
		setColor(rnd()%15, 0)
		si, di := 0, 0
		step := rnd()%6*62 + 13
		for count < 441 {
			si += step
			if si >= 62 {
				for {
					si -= 62
					di++
					if di > 6 {
						di = 0
					}
					if si < 62 {
						break
					}
				}
			}
			if di < 6 && title[di*124+si*2] == 0xB1 {
				gotoxy(si+10, di+5)
				putch(0xDB)
				delayMs(5)
				count++
			}
			if testKey() {
				count = 701
				outer = 11
			}
		}
		outer++
	}
}

func initGame() { // f_34a7
	cursor(0)
	frame(28, 22, 53, 25)
	setColor(14, 0)
	box(T["x2542"], 30, 22, 51, 24)
	textcolor(0x8D)
	textbackground(0)
	cpT("x254b", 34, 23)
	for i := 0; i < 8; i++ {
		inv[i] = 0
		offs[i] = 1.0
		basis[i] = 0
	}
	for i := 0; i < 10; i++ {
		done[i] = false
	}
	genPrices() // (uses the previous city, as the original does)
	city = rnd() % 8
	cash = 500
	hold = 10
	bank = 0
	debt = 0
	loanFlag = 0
	numGuns = 0
	gunType = 0
	status = 1
	health = 100
	dead = false
	angry = 0
	copFlag = 0
	thugs = 0
	totalDays = 0
	month = rnd()%12 + 1
	day = rnd()%28 + 1
	year = rnd()%6 + 86
	delayMs(1000)
}

func showDeath() { // f_28c0
	delayMs(2000)
	puttext(23, 5, 57, 19, scr["tomb"])
	delayMs(5000)
	flushKeys()
	readKey()
}

func askPlayAgain() bool { // f_2812
	setColor(15, 0)
	box(T["x2466"], 29, 10, 51, 15)
	frame(30, 11, 50, 14)
	cpT("x23c0", 32, 12)
	cpT("x23d3", 32, 13)
	cursor(1)
	gotoxy(40, 14)
	k := askKey("YN")
	return k == 89
}

func repSet(rep []byte, line, col int, src []byte) { // strcpy into a 50-byte report record
	o := line*50 + col
	copy(rep[o:], src)
	rep[o+len(src)] = 0
}
func repNum(rep []byte, line, endCol int, n int64) {
	b := []byte(strconv.FormatInt(n, 10))
	repSet(rep, line, endCol-len(b), b)
}
func repLine(rep []byte, line int) []byte {
	r := make([]byte, 50)
	copy(r, rep[line*50:line*50+50])
	return r
}

func showWin() { // f_28f9: the DRUGLORD ending
	delayMs(1000)
	frame(1, 17, 80, 25)
	word := "DRUGLORD!!!"
	for it := 1; it < 6; it++ {
		frame(15, 20, 65, 20)
		setColor(it+9, 0)
		for j := 0; j < 22; j++ {
			gotoxy((j/2)*5+15, 20)
			putch(int(word[j/2]))
			if j < 16 {
				tone(freqTab[j], 20)
			}
		}
		delayMs(500)
	}
	puttext(1, 1, 80, 25, scr["final"])
	// flames: one per gun (max 10)
	nf := numGuns
	if nf > 10 {
		nf = 10
	}
	for k := 1; k <= nf; k++ {
		even := b2i(k%2 == 0)
		y := even*3 + 2
		var x int
		if k <= 4 {
			x = ((k-1)/2)*11 + even*5 + 55
		} else {
			x = ((k-5)/2)*11 + even*5 + 1
		}
		puttext(x, y, x+7, y+2, scr["fire"])
	}
	// stacks of money
	bars := int(idiv(cash+bank-1, 2000000)) + 1
	for si := 0; si < 4; si++ {
		row := si + 11
		for j := 0; j < 5; j++ {
			x := j*7 + 18 - 2*si
			d := 15 - 3*si - j
			if d < 0 {
				d = 0
			}
			for m := 0; m < bars; m++ {
				val := int64(si*5+j+1)*100000 + int64(m)*2000000
				if val > cash {
					break
				}
				puttext(x, row-m, x+4, row-m, scr["bar"])
				tone(1000+m*500, d+5)
				tone(3000+m*500, d)
			}
		}
	}
	// ---- score report
	pct := 0
	if totalDeals != 0 {
		pct = int(idiv(int64(goodDeals*100), int64(totalDeals)))
	}
	rep := append([]byte{}, reportTpl...)
	repNum(rep, 6, 0x29, cash)
	repNum(rep, 7, 0x29, bank)
	repNum(rep, 8, 0x29, debt)
	repNum(rep, 9, 0x29, int64(goodDeals))
	repNum(rep, 10, 0x29, int64(totalDeals))
	repNum(rep, 11, 0x28, int64(pct))
	o := 11 * 50
	e := o
	for rep[e] != 0 {
		e++
	}
	rep[e] = 37
	rep[e+1] = 0 // strcat "%"
	gn := []byte(gunName[gunType])
	repSet(rep, 12, 0x17-len(gn), gn)
	repSet(rep, 12, 0x17, strBytes(T["x2454"]))
	repNum(rep, 12, 0x29, int64(numGuns))
	repNum(rep, 13, 0x29, int64(totalDays))
	total := float64(cash + bank - debt)
	var p1 int64
	ex := math.Exp(total * 6.9315e-08)
	if ex > 100000 {
		p1 = 700000
	} else {
		p1 = ftol(ex) * 75000
		if p1 > 700000 {
			p1 = 700000
		}
	}
	var p2 int64
	if totalDays > 0 {
		p2 = ftol(math.Pow(float64(totalDays), -2.12) * 104000000000.0)
		if p2 > 500000 {
			p2 = 500000
		}
	}
	p3 := ftol(math.Pow(float64(pct), 8.14) * 2.48e-11)
	if p3 > 500000 {
		p3 = 500000
	}
	p4 := int64(gunType*numGuns) * 1500
	score := p1 + p2 + p3 + p4
	repNum(rep, 16, 0x29, score)
	rk := 0
	for rk < 10 && rankThr[rk] <= score {
		rk++
	}
	rn := []byte(T[rankName[rk]])
	repSet(rep, 17, 0x29-len(rn), rn)

	setColor(9, 0)
	box(T["x2466"], 10, 18, 55, 25)
	cpT("x246f", 50, 25)
	cpT("x2475", 12, 25)
	setColor(14, 0)
	for si := 0; si < 5; si++ {
		cprintBytes(repLine(rep, si), 15, 19+si)
	}
	si := 0
	for {
		c := readKey()
		setColor(14, 0)
		if c == 0 {
			c2 := readKey()
			if c2 == 0x48 && si > 0 {
				si--
				b := gettext(15, 19, 55, 23)
				puttext(15, 20, 55, 24, b)
				cprintBytes(repLine(rep, si), 15, 19)
				sfx(20)
			}
			if c2 == 0x50 && si < 12 {
				si++
				b := gettext(15, 20, 55, 24)
				puttext(15, 19, 55, 23, b)
				cprintBytes(repLine(rep, si+5), 15, 24)
				sfx(20)
			}
			if si > 0 {
				cpT("x248d", 50, 18)
			} else {
				cpT("x2493", 50, 18)
			}
			if si < 10 {
				cpT("x2499", 50, 25)
			} else {
				cpT("x249f", 50, 25)
			}
		}
		if c == 27 {
			break
		}
	}
}

// ---------------------------------------------------------------------------
//  The game
// ---------------------------------------------------------------------------
var playing = true

func drawMenu() {
	cursor(0)
	frame(1, 17, 80, 25)
	cpT("x257f", 67, 17)
	window(15, 20, 80, 25)
	cpT("x2594", 15, 20)
	gotoxy(1, 4)
	cprint("@0a(@1cF1@0a) Load Game,"+strings.Repeat(" ", 20)+"@0a(@1cF2@0a) Save Game", 99, 0)
	window(1, 1, 80, 25)
	cpT("x2653", 20, 18)
	cpT("x2680", 30, 25)
}

func menuBuy() bool {
	ok := false
	var d int
	for {
		drawDrugList()
		for {
			cpT("x2723", 1, 19)
			setColor(14, 0)
			if readField(1, 0) {
				break
			}
			d = int(atol(buf))
			if d < 1 || d > 8 {
				sfx(2)
			}
			if d >= 1 && d <= 8 {
				ok = true
				break
			}
		}
		break
	}
	if !ok {
		return false
	}
	cursor(0)
	d--
	maxBuy := idiv(cash, price[d])
	cpT("x274e", 1, 22)
	setColor(11, 0)
	cputs(ltoa(maxBuy))
	cpT("x2771", 99, 99)
	setColor(14, 0)
	got := false
	var qty int64
	for {
		gotoxy(27, 22)
		if readField(5, 0) {
			break
		}
		qty = atol(buf)
		if qty < 0 || len(buf) == 0 {
			continue
		}
		got = true
		break
	}
	cursor(0)
	if !got {
		return false
	}
	if qty > maxBuy {
		cpT("x277a", 3, 24)
		sfx(3)
		delayMs(1000)
		return false
	}
	if qty > int64(free) {
		cpT("x27bd", 3, 24)
		sfx(3)
		delayMs(3000)
		return false
	}
	if qty > 0 {
		cpT("x2801", 1, 25)
		sfx(4)
		delayMs(2000)
		inv[d] += int(qty)
		cash -= qty * price[d]
		if basis[d] == 0 {
			basis[d] = price[d] & 0xFFFF
		} else {
			basis[d] = idiv(price[d]+basis[d], 2) & 0xFFFF
		}
	}
	return true
}

func menuSell() bool {
	ok := false
	var d int
	for {
		drawDrugList()
		cpT("x2825", 1, 19)
		setColor(14, 0)
		if readField(1, 0) {
			break
		}
		d = int(atol(buf))
		if d < 1 || d > 8 {
			sfx(2)
		}
		if d >= 1 && d <= 8 {
			ok = true
			break
		}
	}
	cursor(0)
	if !ok {
		return false
	}
	d--
	if inv[d] == 0 {
		cpT("x2851", 1, 22)
		sfx(3)
		delayMs(1000)
		return false
	}
	got := false
	var qty int64
	for {
		cpT("x2885", 1, 22)
		setColor(14, 0)
		if readField(5, 0) {
			break
		}
		qty = atol(buf)
		if qty < 0 || len(buf) == 0 {
			continue
		}
		got = true
		break
	}
	cursor(0)
	if !got {
		return false
	}
	if int64(inv[d]) < qty {
		cpT("x28a3", 1, 24)
		sfx(3)
		delayMs(3000)
		return false
	}
	if qty > 0 {
		cpT("x28f2", 1, 25)
		sfx(4)
		delayMs(2000)
		if int64(inv[d])-qty == 0 {
			var w int
			if d < 3 {
				w = (3 - d) * 3
			} else {
				w = 1
			}
			old := status
			bs := float64(basis[d])
			if bs <= 0 {
				bs = float64(price[d])
			}
			gain := ftol(((float64(price[d]) / bs) - 1.0) * 3.0 * float64(w))
			status += int(gain)
			totalDeals++
			if status > old {
				goodDeals++
			}
			basis[d] = 0
		}
		inv[d] -= int(qty)
		cash += qty * price[d]
	}
	return true
}

func menuJet() bool {
	drawCityMenu()
	si := 0
	esc := false
	for {
		gotoxy(1, 25)
		clreol()
		cpT("x2699", 1, 25)
		if readField(2, 1) {
			esc = true
			break
		}
		cursor(0)
		si = int(atol(buf))
		if si < 1 || si > 8 {
			sfx(2)
		}
		if si-1 == city {
			cpT("x26af", 28, 25)
			sfx(3)
			si = 0
			delayMs(500)
		}
		if si >= 1 && si <= 8 {
			break
		}
	}
	cursor(0)
	if esc {
		return false
	}
	cost := travelCost(si-1, city)
	if cost > cash {
		cpT("x26ea", 25, 25)
		sfx(3)
		delayMs(2000)
		return false
	}
	cash -= cost
	city = si - 1
	mktToday = 0
	genPrices()
	gotoxy(30, 25)
	setColor(15, 0)
	jetSlide(false) // slide the city list off the screen, rising sweep
	drawStatus()
	for i := 0; i < 10; i++ {
		done[i] = false
	}
	return true
}

func menuBank() bool {
	cpT("x2912", 20, 18)
	cursor(1)
	k := askKey("DW\x1b")
	if k == 27 {
		return false
	}
	if k == 68 {
		cpT("x29bf", 5, 23)
	} else {
		cpT("x29de", 5, 23)
	}
	setColor(15, 0)
	if readField(10, 0) {
		cursor(0)
		return false
	}
	cursor(0)
	amt := atol(buf)
	if amt <= 0 {
		return false
	}
	if k == 68 {
		if amt > cash {
			cpT("x29fe", 25, 25)
			sfx(3)
			delayMs(1000)
		} else {
			cpT("x2a24", 37, 25)
			sfx(2)
			delayMs(2000)
			cash -= amt
			bank += amt
			drawStatus()
		}
	} else {
		if amt > bank {
			cpT("x2a2c", 25, 25)
			sfx(3)
			delayMs(1000)
		} else {
			cpT("x2a53", 37, 25)
			sfx(2)
			delayMs(2000)
			cash += amt
			bank -= amt
			drawStatus()
		}
	}
	return false
}

func menuLoan() bool {
	cpT("x2a5b", 1, 17)
	cursor(1)
	k := askKey("BP\x1b")
	cursor(0)
	if k == 27 {
		return false
	}
	if k == 66 { // borrow
		if angry == 1 {
			cpT("x2bc5", 5, 24)
			delayMs(2000)
			return true
		}
		if status <= 0 {
			cpT("x2c0f", 5, 24)
			delayMs(2000)
			return true
		}
		frame(1, 21, 80, 25)
		cpT("x2c54", 10, 22)
		if readField(10, 0) {
			cursor(0)
			return false
		}
		cursor(0)
		amt := atol(buf)
		if amt <= 0 {
			return false
		}
		if amt < 1000 {
			cpT("x2c71", 1, 25)
			sfx(3)
			delayMs(1000)
			return false
		}
		if idiv(amt, 10000) >= int64(status) {
			cpT("x2cc3", 10, 25)
			sfx(3)
			delayMs(1000)
			return false
		}
		days := int(ftol((math.Log(float64(status)*10000.0)-math.Log(float64(debt+amt)))/0.182321556794)) + 4
		refuse := false
		if idiv(debt+amt, 10000) > int64(status) {
			refuse = true
		} else if days < loanDays {
			refuse = false
		} else if loanFlag == 1 {
			refuse = true
		}
		if refuse {
			cpT("x2d03", 1, 24)
			setColor(4, 0)
			cputs(ltoa(debt) + " ")
			setColor(11, 0)
			cputs(T["x2d25"])
			cpT("x2d4f", 20, 25)
			sfx(3)
			delayMs(2000)
			return false
		}
		cpT("x2d7c", 1, 24)
		debt += amt
		cash += amt
		loanFlag = 1
		loanDays = days
		setColor(4, 0)
		cputs(itoa(loanDays - 1))
		cpT("x2e05", 99, 99)
		sfx(2)
		delayMs(2000)
		return true
	}
	// pay back
	if debt == 0 {
		cpT("x2e0f", 1, 24)
		sfx(3)
		delayMs(2000)
		return false
	}
	frame(1, 21, 80, 25)
	cpT("x2e6d", 10, 22)
	if readField(10, 0) {
		cursor(0)
		return false
	}
	cursor(0)
	amt := atol(buf)
	if amt <= 0 {
		return false
	}
	if amt > cash {
		cpT("x2e8c", 20, 25)
		sfx(3)
		delayMs(1000)
		return false
	}
	if amt >= debt {
		debt = 0
	} else {
		debt -= amt
	}
	cash -= amt
	if debt > 0 {
		cpT("x2eb9", 20, 24)
		setColor(12, 0)
		cputs(ltoa(debt) + " ")
		cpT("x2edb", 99, 99)
		if debt > 1 {
			cputs(T["x2ee2"])
		} else {
			cputs(T["x2ee6"])
		}
		sfx(3)
		delayMs(2000)
	} else {
		cpT("x2ee9", 1, 24)
		loanDays, loanFlag, angry, thugs = 0, 0, 0, 0
		sfx(2)
		delayMs(2000)
	}
	return true
}

func menuHospital() bool {
	if health == 100 {
		cpT("x2f57", 15, 19)
		pressAnyKey()
		return false
	}
	cost := int64((100 - health) * 10)
	cpT("x2fc9", 4, 19)
	setColor(10, 0)
	cputs(ltoa(cost))
	cpT("x3046", 99, 99)
	cpT("x305f", 5, 22)
	cursor(1)
	k := askKey("YN\x1b")
	cursor(0)
	if k == 89 {
		if cost > cash {
			cpT("x3088", 12, 24)
			sfx(3)
			pressAnyKey()
		} else {
			cash -= cost
			cpT("x30c1", 5, 24)
			setColor(12, 0)
			stay := int(idiv(cost, 15))
			cputs(itoa(stay))
			cpT("x30db", 99, 99)
			sfx(2)
			for n := 1; stay > n; n++ {
				nextDay()
			}
			health = 100
			pressAnyKey()
		}
	}
	return true
}

func menuQuit() bool {
	cpT("x310d", 15, 20)
	cursor(1)
	k := askKey("YN")
	if k == 78 {
		return false
	}
	playing = false
	return true
}

func setVar(name string, v int64) {
	switch name {
	case "cash": cash = v
	case "bank": bank = v
	case "debt": debt = v
	case "status": status = int(v)
	case "health": health = int(v)
	case "hold": hold = int(v)
	case "numGuns": numGuns = int(v)
	case "gunType": gunType = int(v)
	case "city": city = int(v)
	case "loanFlag": loanFlag = int(v)
	case "loanDays": loanDays = int(v)
	case "angry": angry = int(v)
	case "thugs": thugs = int(v)
	case "copFlag": copFlag = int(v)
	case "month": month = int(v)
	case "day": day = int(v)
	case "year": year = int(v)
	case "goodDeals": goodDeals = int(v)
	case "totalDeals": totalDeals = int(v)
	case "totalDays": totalDays = int(v)
	case "mktToday": mktToday = int(v)
	}
}

func runGame() {
	showTitle()
	playing = true
	initGame()
	drawMain()
	drawStatus()
	if headless && os.Getenv("DRUGLORD_SET") != "" { // test hook
		for _, kv := range strings.Split(os.Getenv("DRUGLORD_SET"), ";") {
			p := strings.Split(kv, "=")
			if len(p) == 2 {
				v, _ := strconv.ParseInt(p[1], 10, 64)
				setVar(p[0], v)
			}
		}
		drawStatus()
	}
	if headless && os.Getenv("DRUGLORD_FORCE") != "" {
		n, _ := strconv.Atoi(os.Getenv("DRUGLORD_FORCE"))
		doEvent(n)
	}
	for { // one pass = one game
		for { // menu loop (0x36ff)
			drawMenu()
			if status >= 300 {
				break
			}
			setColor(15, 0)
			var key int
			for { // (the original flushes the keyboard on every try)
				cursor(1)
				flushKeys()
				key = readKey()
				if key == 0 && queueCount() > 0 { // F1 = load, F2 = save (Ctrl-L / Ctrl-S work too)
					sc := peekKey()
					if sc == 0x3C {
						readKey()
						key = 19
					} else if sc == 0x3B {
						readKey()
						key = 12
					}
				}
				if key >= 97 && key <= 122 {
					key -= 32
				}
				if key == 12 || key == 19 {
					break
				}
				if key > 0 && strings.IndexByte("BSJVLHQ", byte(key)) >= 0 {
					break
				}
			}
			cursor(0)
			frame(1, 17, 80, 25)
			advance := false

			if key == 19 { // ---- save game
				saveGameDialog()
				continue
			}
			if key == 12 { // ---- load game
				if loadGameDialog() {
					drawMain()
					drawStatus()
				}
				continue
			}
			switch key {
			case 'B':
				advance = menuBuy()
			case 'S':
				advance = menuSell()
			case 'J':
				advance = menuJet()
			case 'V':
				advance = menuBank()
			case 'L':
				advance = menuLoan()
			case 'H':
				advance = menuHospital()
			case 'Q':
				advance = menuQuit()
			}

			if !advance {
				continue
			}

			// ------------------------------------------------ a day passes (0x47bf)
			nextDay()
			drawStatus()
			if dead || !playing {
				break
			}
			if rnd()%10 == 0 && rnd()%(int(idiv(int64(status), 20))+1) > 4 {
				if rnd()%2 == 0 {
					doEvent(7)
				} else {
					doEvent(8)
				}
			}
			if copFlag == 1 || (status >= 100 && rnd()%400 < status && rnd()%8 == 0) {
				doEvent(0)
			}
			if dead || !playing {
				break
			}
			if rnd()%5 == 0 {
				doEvent(rnd() % 10)
			}
			if dead || !playing {
				break
			}
			if loanDays < 0 && loanFlag == 1 {
				frame(1, 17, 80, 25)
				cpT("x3149", 1, 17)
				delayMs(3000)
				thugs++
				for n := 0; n < thugs; n++ {
					frame(1, 23, 80, 25)
					delayMs(500)
					cpT("x329c", 25, 23)
					health -= 25
					angry = 1
					drawStatus()
					cursor(0)
					sfx(3)
					delayMs(1500)
				}
				if health > 0 {
					cpT("x32c1", 1, 24)
					delayMs(3000)
					loanDays = 7
				} else {
					dead = true
				}
				pressAnyKey()
			}
			if dead || !playing {
				break
			}
			drawStatus()
		}

		// ---------------------------------------------------- the game is over
		if playing {
			if dead {
				showDeath()
			} else {
				showWin()
			}
		}
		if askPlayAgain() {
			playing = true
			initGame()
			drawMain()
			drawStatus()
			continue
		}
		break
	}
	// ---- exit screen: only the NOTICE box (registered edition), centred
	clrscr()
	sub := make([]byte, 0, 41*19*2)
	for y := 0; y < 19; y++ {
		sub = append(sub, scr["exit"][y*75*2:y*75*2+41*2]...)
	}
	puttext(20, 2, 60, 20, sub)
	cpT("x3321", 25, 22)
	cursor(0)
	gotoxy(1, 24)
	flushScreen()
}
