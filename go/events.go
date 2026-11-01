package main

import "math"

// ---------------------------------------------------------------------------
//  Random events (f_14ab).  0 cops  1 cops sell  2 cheap stuff  3 newspaper
//  4 factory  5 brownies  6 mugger  7 trenchcoat  8 gun smuggler  9 blackmail
// ---------------------------------------------------------------------------
func doCopFight() {
	copFlag = 0
	result := 0 // 0 fighting, 1 escaped, 2 failed to run
	shots := 1
	if gunType >= 4 {
		shots = 3
	}
	cpT("x142a", 27, 17)
	sfx(5)
	cpT("x144d", 15, 19)
	cpT("x1474", 30, 20)
	cpT("x1498", 30, 21)
	choice := 0
	for {
		frame(1, 23, 80, 25)
		cpT("x14bc", 20, 23)
		cursor(1)
		readField(1, 0)
		cursor(0)
		choice = int(atol(buf))
		if choice < 1 || choice > 3 {
			continue
		}
		if (choice == 2 && status == 0) || (choice == 3 && status < 5) {
			cpT("x14c9", 25, 25)
			sfx(3)
			delayMs(2000)
			frame(25, 25, 80, 25)
			continue
		}
		break
	}
	nf := 0
	switch choice {
	case 1:
		nf = 0
	case 2:
		nf = 1
		status--
	case 3:
		nf = rnd()%4 + int(idiv(int64(status), 100)) + 4
		if nf > 8 {
			nf = 8
		}
		status -= 5
	}
	// ----- set up the fight screen
	frame(1, 1, 80, 25)
	puttext(1, 1, 80, 22, scr["fight"])
	setColor(14, 0)
	cpT("x11fc", 24, 2)
	cpT("x11fc", 52, 2)
	setColor(12, 0); cpT("x1202", 32, 2)
	setColor(10, 0)
	cpT("x141c", 9, 8); cpT("x141c", 49, 8)
	cpT("x1423", 28, 8); cpT("x1423", 68, 8)
	setColor(14, 0)
	cpT("x1269", 10, 6); cpT("x1269", 50, 6)
	cpT("x125e", 9, 5)
	setColor(12, 0)
	gotoxy(20, 5); cputs(padR(itoa(numGuns), 10))
	setColor(6, 0)
	gotoxy(20, 6); cputs(padR(gunName[gunType], 15))
	var php [10]int // your side: [0] = you
	php[0] = health
	setColor(6, 0)
	for i := 1; i <= nf; i++ {
		php[i] = rnd()%10 + dice(10, int(idiv(int64(status), 50))) + 25
		gotoxy(1, i+10)
		if choice == 2 {
			cputs(padL("Friend", 10) + ":")
		}
		if choice == 3 {
			cputs(padL("Posse", 6) + " #" + padL(itoa(i), 2) + ":")
		}
	}
	var nc, ct int
	for { // how many cops, and what do they carry
		nc = rnd()%8 + 3
		ct = int(idiv(int64(status), int64(nc*2)))
		if rnd()%20 == 0 {
			if ct < 2 {
				r := rnd()%4 + 1
				ct = int(idiv(int64(status), int64(r)))
				if ct > 3 {
					ct = 3
				}
				if ct < 2 {
					ct = 2
				}
			} else {
				nc = rnd()%3 + 8
				ct = 3
			}
			break
		} else {
			if ct < 2 {
				continue
			}
			if ct > 3 {
				continue
			}
			break
		}
	}
	gotoxy(60, 6)
	setColor(9, 0)
	cputs(padR(gunName[ct], 15))
	setColor(9, 0)
	var chp [12]int
	for i := 0; i < nc; i++ {
		gotoxy(42, i+11)
		cputs(padL("Cop", 6) + " #" + padL(itoa(i+1), 2))
		chp[i] = dice(15, 2) + 15
	}
	copsAlive := nc
	partyAlive := nf + 1
	for i := 0; i <= nf; i++ {
		drawHP(php[i], i, 0)
	}
	for i := 0; i < nc; i++ {
		drawHP(chp[i], i, 1)
	}

	for {
		frame(1, 24, 80, 25)
		cpT("x1525", 5, 24)
		cursor(1)
		key := askKey("FR")
		cursor(0)
		putch(key)
		if key == 82 { // R: run
			cpT("x15a1", 20, 25)
			delayMs(3000)
			a := rnd()%10 + int(idiv(int64(nc), 3))
			b := rnd()%10 + int(idiv(int64(nf), 3))
			if a < b {
				cpT("x15bc", 99, 99)
				health = php[0]
				result = 1
			} else {
				cpT("x15d1", 99, 99)
				result = 2
				key = 70
			}
		}
		if key == 70 { // F: fight
			if result != 2 {
				for si := 0; si <= nf; si++ {
					if php[si] == 0 {
						continue
					}
					if si < numGuns {
						cpT("x154e", 28, si+10)
					} else {
						cpT("x1557", 27, si+10)
					}
					att := 0
					for {
						var t int
						for {
							t = rnd() % nc
							if chp[t] != 0 {
								break
							}
						}
						sfx(gunType + 10)
						if rnd()%2 == 0 {
							cpT("x1562", 67, t+11)
							sfx(8)
							delayMs(100)
							if si < numGuns {
								chp[t] -= dice(6, gunDice[gunType])
							} else {
								chp[t] -= dice(4, 1)
							}
							if chp[t] <= 0 {
								chp[t] = 0
								copsAlive--
							}
						} else {
							cpT("x156f", 67, t+11)
							delayMs(200)
						}
						drawHP(chp[t], t, 1)
						att++
						if php[0] != 0 && copsAlive != 0 && gunType > 3 && att < shots && copsAlive > 0 && si < numGuns {
							continue
						}
						break
					}
					drawHP(php[si], si, 0)
					if copsAlive == 0 {
						break
					}
				}
			}
			if copsAlive != 0 {
				result = 0
				for si := 0; si < nc; si++ {
					if chp[si] == 0 {
						continue
					}
					cpT("x157d", 68, si+11)
					var t int
					for {
						t = rnd() % (nf + 1)
						if php[t] != 0 {
							break
						}
					}
					sfx(ct + 10)
					if rnd()%2 == 0 {
						cpT("x1586", 26, t+10)
						sfx(8)
						delayMs(100)
						php[t] -= dice(6, gunDice[ct])
						if php[t] <= 0 {
							status--
							php[t] = 0
							partyAlive--
						}
					} else {
						cpT("x1593", 26, t+10)
						delayMs(200)
					}
					drawHP(php[t], t, 0)
					drawHP(chp[si], si, 1)
					if php[0] == 0 || partyAlive == 0 {
						break
					}
				}
			}
		}
		if !(copsAlive > 0 && partyAlive > 0 && php[0] > 0 && result != 1) {
			break
		}
	}

	if result == 1 {
		delayMs(3000)
		drawMain()
		return
	}
	health = php[0]
	if health == 0 {
		dead = true
		return
	}
	// victory
	frame(1, 23, 80, 25)
	cpT("x15ed", 23, 23)
	delayMs(1000)
	sell := 1
	if !(ct < gunType && nc <= numGuns) {
		setColor(7, 0)
		gotoxy(21, 24)
		what := "their guns"
		if nc == 1 {
			what = "his gun"
		}
		cputs("Do you wanna take " + what + ", man? (Y/N) ")
		cursor(1)
		k := askKey("YN")
		cursor(0)
		if k == 89 {
			cpT("x1649", 15, 25)
			sell = 0
			if gunType == ct {
				numGuns += nc
			} else {
				gunType = ct
				numGuns = nc
			}
		} else {
			cpT("x1688", 29, 25)
			delayMs(2000)
		}
	}
	if sell == 1 {
		cpT("x16a2", 5, 24)
		gold := nc * ct * 200
		cputs(itoa(nc) + " guns that were on the pigs you wasted for " + itoa(gold) + " bucks!")
		cpT("x16e9", 20, 25)
		cash += int64(gold)
		status += nc
		delayMs(4000)
	}
	if !dead {
		drawMain()
	}
}

func doEvent(ev int) {
	cursor(0)
	if done[ev] {
		return
	}
	frame(1, 17, 80, 25)
	switch ev {
	case 0:
		doCopFight()
	case 1:
		if mktToday != 1 {
			cpT("x1717", 1, 18)
			setColor(6, 0)
			marketEvent(3, 30, f32(0.7))
			setColor(11, 0)
			cputs(T["x180a"])
			sfx(6)
			pressAnyKey()
		}
	case 2:
		cpT("x180d", 1, 18)
		setColor(6, 0)
		marketEvent(3, 40, f32(0.6))
		setColor(7, 0)
		cputs(T["x18c9"])
		pressAnyKey()
	case 3:
		window(15, 17, 80, 25)
		cpT("x18cd", 15, 17)
		window(1, 1, 80, 25)
		for si := 0; si < 8; si++ {
			f := float64(rnd()%25)/100.0 + 1.05
			price[si] = ftol(float64(price[si]) * f)
		}
		sfx(7)
		pressAnyKey()
	case 4:
		cpT("x1a7e", 1, 18)
		setColor(6, 0)
		marketEvent(1, 40, f32(1.2))
		setColor(13, 0)
		if rnd()%2 == 1 {
			cputs(T["x1ad4"])
		} else {
			cputs(T["x1ae1"])
		}
		cpT("x1aea", 99, 99)
		pressAnyKey()
	case 5:
		if inv[5] != 0 {
			cpT("x1b2e", 1, 17)
			inv[5] = 0
			pressAnyKey()
		}
	case 6:
		cpT("x1c5d", 1, 17)
		used := false
		if status >= 10 && rnd()%20 != 0 {
			cpT("x1cf6", 1, 20)
			cursor(1)
			k := askKey("YN")
			cursor(0)
			if k == 89 {
				cpT("x1d44", 1, 22)
				status -= 10
				used = true
			}
		}
		if !used {
			if cash < 50 {
				cpT("x1dae", 20, 21)
				cpT("x1dd8", 10, 22)
				cpT("x1e13", 28, 23)
				sfx(13)
				health -= rnd()%6 + rnd()%6 + 2
			} else {
				cpT("x1e31", 5, 22)
			}
			cash = 0
		}
		pressAnyKey()
	case 7:
		if hold < 300 {
			var cap int
			for {
				cap = rnd()%10 + 1
				if rnd()%5 == 0 {
					cap += rnd()%10 + 1
					if rnd()%5 == 0 {
						cap += rnd()%10 + 1
					}
				}
				cap *= 10
				if !(hold+50 < cap || cap <= hold) {
					break
				}
			}
			a := ftol(math.Pow(float64(cap), 3.34) * 0.0634)
			t := int64(rnd()%200+1) * a
			pr := ftol(float64(t)/10000.0) * 100
			cpT("x1e77", 1, 17)
			cpT("x1ec7", 10, 19)
			cpT("x1f0c", 11, 20)
			setColor(12, 0)
			cputs(itoa(cap))
			cpT("x1f27", 99, 99)
			setColor(12, 0)
			cputs("$" + ltoa(pr))
			cpT("x1f47", 99, 99)
			cpT("x1f53", 1, 23)
			cursor(1)
			k := askKey("YN")
			cursor(0)
			if k == 89 {
				if cash < pr {
					cpT("x1f6c", 1, 24)
				} else {
					cpT("x1fbc", 16, 24)
					cash -= pr
					hold = cap
				}
			} else {
				cpT("x1fee", 20, 24)
			}
			pressAnyKey()
		}
	case 8:
		gi := rnd() % 5
		var qty int
		if rnd()%10 == 0 {
			qty = rnd()%6 + 5
		} else {
			qty = rnd()%5 + 2
		}
		each := gi*200 + 200
		total := qty * each
		cpT("x201a", 1, 17)
		cpT("x2080", 1, 20)
		setColor(12, 0)
		cputs(gunLong[gi] + "'s")
		cpT("x209e", 99, 99)
		cpT("x20b9", 1, 21)
		setColor(12, 0)
		cputs(itoa(qty) + " ")
		cpT("x20e0", 99, 99)
		setColor(12, 0)
		cputs("$" + itoa(each) + " ")
		cpT("x20eb", 99, 99)
		setColor(12, 0)
		cputs("$" + itoa(total))
		setColor(7, 0)
		cputs(T["x2107"])
		cpT("x210a", 20, 23)
		cursor(1)
		k := askKey("YN")
		cursor(0)
		if k == 89 {
			if int64(total) > cash {
				cpT("x2135", 5, 24)
			} else {
				cpT("x2185", 1, 24)
				numGuns = qty
				gunType = gi + 1
				cash -= int64(total)
			}
		} else {
			cpT("x21d7", 15, 24)
		}
		pressAnyKey()
	case 9:
		if cash >= 10000 {
			f := f32(float64(rnd()%30)/100.0 + 0.7)
			amt := ftol(float64(cash)*f/1000.0) * 1000
			cpT("x220f", 1, 17)
			setColor(12, 0)
			cputs("$" + ltoa(amt) + " ")
			cpT("x227a", 99, 99)
			cpT("x22d9", 1, 21)
			cursor(1)
			k := askKey("YN")
			cursor(0)
			if k == 89 {
				cpT("x22fe", 1, 24)
				cash -= amt
			} else {
				cpT("x2345", 9, 24)
				if rnd()%5 != 0 {
					copFlag = 1
					done[0] = false
				}
			}
			pressAnyKey()
		}
	}
	done[ev] = true
	if !dead {
		drawStatus()
	}
}
