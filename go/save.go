package main

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"regexp"
	"sort"
	"strings"
)

// ---------------------------------------------------------------------------
//  Save / load  (F2 / F1, or Ctrl-S / Ctrl-L)
//  Saves are small JSON text files named <name>.sav, kept next to the program
//  (or wherever -savedir points).  The format is the same as the PowerShell edition's.
// ---------------------------------------------------------------------------
const saveMagic = "DRUGLORD-SAVE"
const saveVer = 1

var saveDir string

type saveState struct {
	Magic      string    `json:"magic"`
	Version    int       `json:"version"`
	Cash       int64     `json:"cash"`
	Bank       int64     `json:"bank"`
	Debt       int64     `json:"debt"`
	City       int       `json:"city"`
	Month      int       `json:"month"`
	Day        int       `json:"day"`
	Year       int       `json:"year"`
	Hold       int       `json:"hold"`
	Inv        []int     `json:"inv"`
	Basis      []int64   `json:"basis"`
	Price      []int64   `json:"price"`
	Offs       []float64 `json:"offs"`
	NumGuns    int       `json:"numGuns"`
	GunType    int       `json:"gunType"`
	Status     int       `json:"status"`
	Health     int       `json:"health"`
	LoanFlag   int       `json:"loanFlag"`
	LoanDays   int       `json:"loanDays"`
	Angry      int       `json:"angry"`
	Thugs      int       `json:"thugs"`
	CopFlag    int       `json:"copFlag"`
	Done       []bool    `json:"done"`
	MktToday   int       `json:"mktToday"`
	TotalDays  int       `json:"totalDays"`
	GoodDeals  int       `json:"goodDeals"`
	TotalDeals int       `json:"totalDeals"`
}

var nameRe = regexp.MustCompile(`[^A-Za-z0-9_\-]`)

func cleanSaveName(s string) string { // letters, digits, - and _ only, max 20 chars
	t := nameRe.ReplaceAllString(s, "")
	if len(t) > 20 {
		t = t[:20]
	}
	return t
}

// default save folder: next to the program if writable, else the user's config folder
func chooseSaveDir() string {
	if saveDir != "" {
		return saveDir
	}
	if exe, err := os.Executable(); err == nil {
		d := filepath.Dir(exe)
		p := filepath.Join(d, ".druglord_write_test")
		if f, err := os.Create(p); err == nil {
			f.Close()
			os.Remove(p)
			return d
		}
	}
	if c, err := os.UserConfigDir(); err == nil {
		d := filepath.Join(c, "DrugLord")
		os.MkdirAll(d, 0755)
		return d
	}
	d, _ := os.Getwd()
	return d
}

func makeSaveState() saveState {
	s := saveState{Magic: saveMagic, Version: saveVer,
		Cash: cash, Bank: bank, Debt: debt, City: city, Month: month, Day: day, Year: year, Hold: hold,
		Inv: append([]int{}, inv[:]...), Basis: append([]int64{}, basis[:]...),
		Price: append([]int64{}, price[:]...), Offs: append([]float64{}, offs[:]...),
		NumGuns: numGuns, GunType: gunType, Status: status, Health: health,
		LoanFlag: loanFlag, LoanDays: loanDays, Angry: angry, Thugs: thugs, CopFlag: copFlag,
		Done: append([]bool{}, done[:]...), MktToday: mktToday, TotalDays: totalDays,
		GoodDeals: goodDeals, TotalDeals: totalDeals}
	return s
}

func inRange(v, lo, hi int64) bool { return v >= lo && v <= hi }

// returns an error string, or "" after applying the save to the game
func applySaveState(o *saveState) string {
	if o.Magic != saveMagic {
		return "Not a DrugLord save file."
	}
	if o.Version != saveVer {
		return "Unknown save file version."
	}
	if len(o.Inv) != 8 || len(o.Basis) != 8 || len(o.Price) != 8 || len(o.Offs) != 8 || len(o.Done) != 10 {
		return "Save file is damaged."
	}
	const big = 1000000000000000
	ok := inRange(o.Cash, -big, big) && inRange(o.Bank, -big, big) && inRange(o.Debt, -big, big) &&
		inRange(int64(o.City), 0, 7) && inRange(int64(o.Month), 1, 12) && inRange(int64(o.Day), 1, 31) &&
		inRange(int64(o.Year), 0, 200) && inRange(int64(o.Hold), 0, 100000) && inRange(int64(o.NumGuns), 0, 1000) &&
		inRange(int64(o.GunType), 0, 5) && inRange(int64(o.Status), -1e9, 1e9) && inRange(int64(o.Health), -1e6, 1e6) &&
		inRange(int64(o.LoanFlag), 0, 1) && inRange(int64(o.LoanDays), -1e6, 1e6) && inRange(int64(o.Angry), 0, 1) &&
		inRange(int64(o.Thugs), 0, 1e6) && inRange(int64(o.CopFlag), 0, 1) && inRange(int64(o.MktToday), 0, 1e6) &&
		inRange(int64(o.TotalDays), 0, 1e9) && inRange(int64(o.GoodDeals), 0, 1e9) && inRange(int64(o.TotalDeals), 0, 1e9)
	for i := 0; i < 8 && ok; i++ {
		ok = inRange(int64(o.Inv[i]), -big, big) && inRange(o.Basis[i], -big, big) && inRange(o.Price[i], -big, big)
	}
	if !ok {
		return "Save file is damaged."
	}
	cash, bank, debt = o.Cash, o.Bank, o.Debt
	city, month, day, year, hold = o.City, o.Month, o.Day, o.Year, o.Hold
	for i := 0; i < 8; i++ {
		inv[i], basis[i], price[i], offs[i] = o.Inv[i], o.Basis[i], o.Price[i], o.Offs[i]
	}
	for i := 0; i < 10; i++ {
		done[i] = o.Done[i]
	}
	numGuns, gunType, status, health = o.NumGuns, o.GunType, o.Status, o.Health
	loanFlag, loanDays, angry, thugs, copFlag = o.LoanFlag, o.LoanDays, o.Angry, o.Thugs, o.CopFlag
	mktToday, totalDays, goodDeals, totalDeals = o.MktToday, o.TotalDays, o.GoodDeals, o.TotalDeals
	dead = false
	return ""
}

// dialog frame: double box with a centred title; returns the screen underneath so it can be restored
func openDialog(title string, x1, y1, x2, y2 int) []byte {
	under := gettext(x1, y1, x2, y2)
	cursor(0)
	frame(x1, y1, x2, y2)
	setColor(15, 0)
	box(T["x2466"], x1, y1, x2, y2)
	setColor(14, 0)
	cprint(title, (x1+x2-len(title))/2+1, y1+1)
	return under
}
func closeDialog(under []byte, x1, y1, x2, y2 int) {
	puttext(x1, y1, x2, y2, under)
	cursor(0)
}
func dialogMsg(s string, color, row int) {
	gotoxy(17, row)
	setColor(color, 0)
	cputs(padR(s, 47))
}

func saveGameDialog() bool { // F2 / Ctrl-S: ask for a name (Enter alone = next free saveN)
	dir := chooseSaveDir()
	n := 1
	for fileExists(filepath.Join(dir, fmt.Sprintf("save%d.sav", n))) {
		n++
	}
	def := fmt.Sprintf("save%d", n)
	x1, y1, x2, y2 := 14, 7, 68, 16
	under := openDialog("Save Game", x1, y1, x2, y2)
	setColor(7, 0)
	cprint("Enter alone = "+def+"    ESC = cancel", 17, 13)
	cprint("Letters, digits, - and _ only.", 17, 14)
	var name, path string
	for {
		setColor(11, 0)
		cprint("Save as filename: ", 17, 9)
		setColor(14, 0)
		if readField(21, 0) {
			closeDialog(under, x1, y1, x2, y2)
			return false
		}
		name = cleanSaveName(buf)
		if len(name) == 0 {
			if strings.TrimSpace(buf) == "" {
				name = def
			} else {
				sfx(2)
				continue
			}
		}
		path = filepath.Join(dir, name+".sav")
		if fileExists(path) {
			dialogMsg("'"+name+"' exists - overwrite? (Y/N)", 12, 9)
			cursor(1)
			k := askKey("YN")
			cursor(0)
			if k == 78 {
				dialogMsg("", 7, 9)
				continue
			}
		}
		break
	}
	ok := false
	if js, err := json.MarshalIndent(makeSaveState(), "", "  "); err == nil {
		if os.WriteFile(path, js, 0644) == nil {
			ok = true
		}
	}
	frame(x1+1, y1+2, x2-1, y2-1)
	if ok {
		setColor(10, 0)
		cprint("Game saved as: "+name+".sav", 17, 9)
		setColor(11, 0)
		cprint("Location:", 17, 11)
		setColor(15, 0)
		row := 12
		p := dir
		for len(p) > 0 && row <= 13 { // wrap the folder path over two lines
			take := 48
			if len(p) < take {
				take = len(p)
			}
			cputsAt(p[:take], 17, row)
			p = p[take:]
			row++
		}
		sfx(4)
	} else {
		setColor(12, 0)
		cprint("Could not write the save file.", 17, 9)
		setColor(7, 0)
		pp := path
		if len(pp) > 48 {
			pp = pp[:48]
		}
		cputsAt(pp, 17, 11)
		sfx(3)
	}
	setColor(7, 0)
	cprint("Press any key to continue", 17, 15)
	cursor(0)
	flushKeys()
	readKey()
	closeDialog(under, x1, y1, x2, y2)
	return ok
}

// plain text (paths may contain @ ~ ^ which must not be treated as tokens)
func cputsAt(s string, x, y int) {
	gotoxy(x, y)
	cputs(s)
}

func fileExists(p string) bool {
	_, err := os.Stat(p)
	return err == nil
}

type saveInfo struct{ name, info string }

func getSaveList() []saveInfo {
	var r []saveInfo
	dir := chooseSaveDir()
	ents, err := os.ReadDir(dir)
	if err != nil {
		return r
	}
	type fe struct {
		name string
		mt   int64
	}
	var files []fe
	for _, e := range ents {
		if e.IsDir() || !strings.HasSuffix(strings.ToLower(e.Name()), ".sav") {
			continue
		}
		fi, err := e.Info()
		if err != nil {
			continue
		}
		files = append(files, fe{e.Name(), fi.ModTime().UnixNano()})
	}
	sort.Slice(files, func(i, j int) bool { return files[i].mt > files[j].mt })
	for _, f := range files {
		data, err := os.ReadFile(filepath.Join(dir, f.name))
		if err != nil {
			continue
		}
		var o saveState
		if json.Unmarshal(data, &o) != nil || o.Magic != saveMagic {
			continue
		}
		info := fmt.Sprintf("%02d/%02d/%02d  $%d", o.Month, o.Day, o.Year, o.Cash)
		r = append(r, saveInfo{strings.TrimSuffix(f.name, f.name[len(f.name)-4:]), info})
	}
	return r
}

func loadGameDialog() bool {
	list := getSaveList()
	show := len(list)
	if show > 8 {
		show = 8
	}
	x1, y1, x2 := 14, 5, 68
	y2 := 11
	if show > 0 {
		y2 = 13 + show
	}
	under := openDialog("Load Game", x1, y1, x2, y2)
	loaded := false
	if len(list) == 0 {
		setColor(12, 0)
		cprint("No saved games found.", 17, 8)
		setColor(7, 0)
		d := padR(chooseSaveDir(), 40)
		cputsAt("Folder: "+d[:40], 17, 9)
		sfx(2)
		delayMs(1800)
		closeDialog(under, x1, y1, x2, y2)
		return false
	}
	setColor(7, 0)
	for i := 0; i < show; i++ {
		setColor(10, 0)
		cprint(fmt.Sprintf("%d. ", i+1), 17, 4+y1+i)
		setColor(15, 0)
		cputs(padR(list[i].name, 22))
		setColor(7, 0)
		cputs(list[i].info)
	}
	inRow := 5 + y1 + show
	setColor(7, 0)
	cprint("Type a name or a number.   ESC = cancel", 17, inRow+2)
	for {
		setColor(11, 0)
		cprint("Load filename: ", 17, inRow)
		setColor(14, 0)
		if readField(21, 0) {
			break
		}
		t := strings.TrimSpace(buf)
		name := ""
		if matched, _ := regexp.MatchString(`^\d+$`, t); matched && atol(t) >= 1 && atol(t) <= int64(show) {
			name = list[atol(t)-1].name
		} else {
			name = cleanSaveName(t)
		}
		path := filepath.Join(chooseSaveDir(), name+".sav")
		if len(name) == 0 || !fileExists(path) {
			setColor(12, 0)
			cprint("File not found."+strings.Repeat(" ", 30), 17, inRow+1)
			sfx(2)
			continue
		}
		errMsg := ""
		data, err := os.ReadFile(path)
		var o saveState
		if err != nil || json.Unmarshal(data, &o) != nil {
			errMsg = "Not a DrugLord save file."
		} else {
			errMsg = applySaveState(&o)
		}
		if errMsg != "" {
			setColor(12, 0)
			cprint(padR(errMsg, 40), 17, inRow+1)
			sfx(2)
			continue
		}
		setColor(10, 0)
		cputsAt(padR("Loaded '"+name+"'.", 40), 17, inRow+1)
		sfx(4)
		delayMs(1000)
		loaded = true
		break
	}
	if !loaded {
		closeDialog(under, x1, y1, x2, y2)
	}
	return loaded
}
