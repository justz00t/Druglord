package main

import (
	"encoding/base64"
	"math"
	"strings"
	"time"
)

// ---------------------------------------------------------------------------
//  Game state
// ---------------------------------------------------------------------------
var drugName = []string{"Cocaine", "Crack", "Heroin", "Acid", "Crystal", "Grass", "Speed", "Ludes"}
var gunName = []string{"None", "Sat. Nt. Spcl", "Ruger", "Auto Mag", "Uzi", "AK-47"}
var gunLong = []string{"Saturday Night Special", "Ruger Revolver", "Auto Mag", "Uzi", "AK-47 Assault Rifle"}
var cityName = []string{"Chicago", "Detroit", "Las Vegas", "Los Angeles", "Miami", "New York", "San Diego", "Washington D.C."}
var rankName = []string{"x23e5", "x23ec", "x23f5", "x23fd", "x2404", "x240d", "x2414", "x241f", "x2429", "x2433", "x2440", "x2449"}

var (
	cash, bank, debt                int64
	city, month, day, year          int
	hold, free                      int
	inv                             [8]int
	basis                           [8]int64
	price                           [8]int64
	offs                            [8]float64
	numGuns, gunType, status, health int
	dead                            bool
	loanFlag, loanDays, angry       int
	thugs, copFlag                  int
	done                            [10]bool
	mktToday, totalDays             int
	goodDeals, totalDeals           int
	buf                             string // text of the last input field
	escPressed                      bool
	scr                             = map[string][]byte{}
	reportTpl                       []byte
)

func init() {
	for k, v := range scrB64 {
		b, err := base64.StdEncoding.DecodeString(v)
		if err != nil {
			panic(err)
		}
		scr[k] = b
	}
	reportTpl, _ = base64.StdEncoding.DecodeString(reportB64)
	year = 86
	month, day = 1, 1
	hold = 10
	health = 100
}

// ---------------------------------------------------------------------------
//  Small UI helpers
// ---------------------------------------------------------------------------
func frame(x1, y1, x2, y2 int) { // f_814: clear a rectangle, back to full-screen window
	window(x1, y1, x2, y2)
	clrscr()
	window(1, 1, 80, 25)
}

func box(chars string, x1, y1, x2, y2 int) { // f_5ca: single/double frame from 8 chars
	c := strBytes(chars)
	gotoxy(x1, y1); putch(int(c[0]))
	gotoxy(x2, y1); putch(int(c[2]))
	gotoxy(x2, y2); putch(int(c[4]))
	gotoxy(x1, y2); putch(int(c[6]))
	gotoxy(x1+1, y1)
	for i := x1 + 1; i < x2; i++ {
		putCode(int(c[1]))
	}
	flushScreen()
	gotoxy(x1+1, y2)
	for i := x1 + 1; i < x2; i++ {
		putCode(int(c[5]))
	}
	flushScreen()
	for y := y1 + 1; y < y2; y++ {
		gotoxy(x1, y); putch(int(c[3]))
		gotoxy(x2, y); putch(int(c[7]))
	}
}

// f_1406: animated "Press any key to continue"
func pressAnyKey() {
	spin := []int{0xF9, 0x09, 0x4F}
	cursor(0)
	flushKeys()
	cpT("x1400", 27, 25)
	setColor(14, 0)
	for {
		for si := 0; si < 3; si++ {
			gotoxy(25, 25); putch(spin[si])
			gotoxy(53, 25); putch(spin[si])
			delayMs(100)
		}
		if headless && queueCount() == 0 {
			panic(headlessStop("HEADLESS_OUT_OF_KEYS"))
		}
		if testKey() {
			break
		}
	}
	readKey() // the key that ended the wait
	frame(25, 25, 54, 25)
}

func delayMs(ms int) {
	flushScreen()
	if headless {
		return
	}
	v := int(math.RoundToEven(float64(ms) / runSpeed))
	if v <= 0 {
		return
	}
	time.Sleep(time.Duration(v) * time.Millisecond)
}

// input line (f_6d5). Returns true when ESC was pressed. Text in buf.
func readField(width, flag int) bool {
	sx, sy := cx, cy
	b := make([]int, 64)
	si := 0
	cursor(1)
	cputs(strings.Repeat(" ", width))
	for {
		if width-1 > si {
			gotoxy(sx+si, sy)
			cputs(" ")
		}
		gotoxy(sx+si, sy)
		c := readKey()
		if c == 0 {
			flushKeys()
			continue
		}
		if c == 27 {
			buf = ""
			escPressed = true
			return true
		}
		if c == 13 {
			b[si] = 0
			break
		}
		if c == 8 {
			if si > 0 {
				b[si] = 0
				si--
			}
			b[si] = 0
			cputs(" ")
			continue
		}
		b[si] = c
		gotoxy(sx+si, sy)
		putch(c)
		if width-1 > si {
			cputs(" ")
		}
		if width-1 == si {
			if flag == 0 {
				break
			}
			sfx(2)
			gotoxy(sx+si, sy)
			cputs(" ")
			si--
		}
		si++
	}
	b[si+1] = 0
	if flag == 0 && width > 1 {
		b[si] = 0
	}
	var t strings.Builder
	for i := 0; i < 64 && b[i] != 0; i++ {
		t.WriteRune(rune(b[i]))
	}
	buf = t.String()
	escPressed = false
	return false
}

// getch loop: toupper, only accept listed chars (ESC = 27)
func askKey(allowed string) int {
	for {
		c := readKey()
		if c >= 97 && c <= 122 {
			c -= 32
		}
		if c > 0 && c < 256 && strings.IndexByte(allowed, byte(c)) >= 0 {
			return c
		}
		if strings.IndexByte(allowed, 27) >= 0 && c == 27 {
			return 27
		}
	}
}

// ---------------------------------------------------------------------------
//  Prices / travel
// ---------------------------------------------------------------------------
func travelCost(a, b int) int64 { // f_d17
	dx := float64(cityXY[b][0] - cityXY[a][0])
	dy := float64(cityXY[b][1] - cityXY[a][1])
	return ftol(math.Sqrt(math.Pow(dx, 2)+math.Pow(dy, 2))*60.0 + 25.0)
}

func genPrices() { // f_edd
	for d := 0; d < 8; d++ {
		r := int64(rnd()) % int64(priceHi[d]-priceLo[d])
		v := float64(r+int64(priceLo[d])) + offs[d]
		price[d] = ftol(v * f32(float64(mult[city][d])))
	}
}

// ---------------------------------------------------------------------------
//  Screens: main frame, status, menus
// ---------------------------------------------------------------------------
func drawMain() { // f_b22
	cursor(0)
	setColor(7, 0)
	window(1, 1, 80, 25)
	clrscr()
	puttext(1, 1, 80, 16, scr["main"])
	setColor(14, 0)
	cpT("x11fc", 24, 2)
	cpT("x11fc", 52, 2)
	setColor(9, 0); cpT("x1202", 32, 2)
	setColor(11, 0); cpT("x1214", 6, 3)
	setColor(6, 0); cpT("x121a", 6, 5)
	setColor(4, 0); cpT("x1228", 30, 5)
	setColor(6, 0); cpT("x1236", 53, 4); cpT("x1240", 57, 5)
	setColor(10, 0); cpT("x1246", 57, 7); cpT("x124c", 54, 8); cpT("x1255", 54, 9)
	setColor(13, 0); cpT("x125e", 52, 11); cpT("x1269", 53, 12)
	setColor(11, 0); cpT("x1273", 51, 14); cpT("x127f", 55, 15)
	setColor(11, 0)
	cpT("x1358", 74, 2)
}

func drawStatus() { // f_89c
	free = 0
	cursor(0)
	setColor(15, 0)
	for i := 0; i < 8; i++ {
		gotoxy(5, i+7); cputs(padR(drugName[i], 7))
		gotoxy(28, i+7); cputs(padR(drugName[i], 7))
	}
	for i := 0; i < 8; i++ {
		gotoxy(16, i+7)
		if inv[i] > 0 {
			setColor(14, 0)
		} else {
			setColor(7, 0)
		}
		cputs(padL(itoa(inv[i]), 6))
		gotoxy(38, i+7)
		setColor(12, 0)
		cputs(padL(ltoa(price[i]), 10))
		free += inv[i]
	}
	setColor(14, 0)
	free = hold - free
	gotoxy(63, 4); cputs(padR(cityName[city], 15))
	gotoxy(63, 5); cputs(padR(itoa(free), 6) + " (" + itoa(hold) + " max)")
	gotoxy(63, 7); cputs("$" + padR(ltoa(cash), 10))
	gotoxy(63, 8); cputs("$" + padR(ltoa(bank), 10))
	gotoxy(63, 9); cputs("$" + padR(ltoa(debt), 10))
	gotoxy(74, 9)
	if loanFlag == 1 {
		cputs("(   )")
		if loanDays <= 2 {
			textcolor(0x8C)
		}
		gotoxy(75, 9); cputs(padL(itoa(loanDays), 3))
		setColor(14, 0)
	} else {
		cputs("     ")
	}
	gotoxy(63, 11); cputs(padR(itoa(numGuns), 6))
	gotoxy(63, 12); cputs(padR(gunName[gunType], 13))
	gotoxy(63, 14); cputs(padR(itoa(status), 6))
	gotoxy(63, 15)
	if health <= 25 {
		textcolor(0x84)
	}
	cputs(padR(itoa(health), 4))
	setColor(14, 0)
	gotoxy(12, 3)
	cputs(twoDig(month) + "/" + twoDig(day) + "/" + twoDig(year))
}

func twoDig(n int) string {
	s := itoa(n)
	if len(s) < 2 {
		s = "0" + s
	}
	return s
}

func drawDrugList() { // f_11a5
	cursor(0)
	cpT("x1393", 45, 18)
	setColor(14, 0)
	cprint(strings.Repeat(" ", 40), 36, 19)
	for si := 0; si < 4; si++ {
		gotoxy(43, si+21)
		setColor(12, 0); cputs(padL(itoa(si+1), 1) + ". ")
		setColor(10, 0); cputs(padR(drugName[si], 15))
		gotoxy(60, si+21)
		setColor(12, 0); cputs(padL(itoa(si+5), 1) + ". ")
		setColor(10, 0); cputs(padR(drugName[si+4], 15))
	}
	cursor(1)
}

func drawCityMenu() { // f_dd3
	cursor(0)
	frame(1, 17, 80, 25)
	cpT("x136e", 34, 17)
	for di := 0; di < 4; di++ {
		for j := 0; j < 2; j++ {
			si := j*4 + di
			gotoxy(j*35+10, di+19)
			setColor(12, 0); cputs(padL(itoa(si+1), 2) + ". ")
			setColor(11, 0); cputs(padR(cityName[si], 20))
			if city != si {
				cputs("(")
				setColor(2, 0)
				cputs("$" + ltoa(travelCost(city, si)))
				cprint("@0b)", 99, 99)
			}
		}
	}
}

func drawHP(hp, row, side int) { // f_12a9
	if side == 0 {
		gotoxy(12, row+10)
	} else {
		gotoxy(52, row+11)
	}
	var st string
	if hp > 0 {
		setColor(15, 0)
		st = "Alive"
	} else {
		setColor(7, 0)
		st = "Dead"
	}
	cputs(padL(itoa(hp), 4) + strings.Repeat(" ", 12) + padR(st, 12))
}

// f_1314: drop the price of 1..n distinct drugs; prints their names
func marketEvent(count, rng int, mul float64) {
	n := rnd()%count + 1
	var used [8]bool
	sep := ""
	for k := 0; k < n; k++ {
		var d int
		for {
			d = rnd() % 8
			if !used[d] {
				break
			}
		}
		cputs(sep + drugName[d])
		if n == 2 {
			sep = " and "
		} else {
			sep = ", "
		}
		used[d] = true
		f := float64(int(math.Trunc(float64(rnd()%rng)/100.0))) + mul
		price[d] = ftol(float64(price[d]) * f)
		if n > 2 && k+2 == n {
			sep = ", and "
		}
	}
	mktToday = 1
}

// one day passes (f_3215): loan clock, calendar, interest
func nextDay() {
	loanDays--
	day++
	totalDays++
	if daysInMonth[month-1] < day {
		day = 1
		month++
		if month > 12 {
			month = 1
			year++
			if year == 100 {
				year = 0
			}
		}
	}
	debt = ftol(float64(debt) * 1.05)
	bank = ftol(float64(bank) * 1.000192)
}
