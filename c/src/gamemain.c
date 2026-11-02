#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

struct text_info ti;
extern int dead, flag79fa, flag7920, flag79f6, g_mono, esc, curcity, loanflag, loandays, ngun, guntype, status, health;
extern int holdfree, hold, goodDeals, totalDeals, delaytime, flag791e, totaldays;
extern int inv[8];
extern unsigned basis[8];
extern long price[8];
extern char flag10[10];
extern long cash, bank, debt;
extern char tbuf2[];
extern void cursor();
extern void frame();
extern void cprint();
extern void flushkeys();
extern void sfx();
extern void intro();
extern void pressany();
extern void nextday();
extern void restorescreen();
extern void endgame();
extern int playagain();
extern void event();
extern char buf67d0[];
extern void newgame();
extern void drawmain();
extern void drawstatus();
extern void drawcities();
extern void drawdruglist();
extern void genprices();
extern int travelcost();
int readfield(int width, char flag, char *buf);
void setcolor(int fg, int bg, char blink);

void gamemain()
{
    int v;
    unsigned long lamt;
    char inbuf[81];
    char key;
    int amount;
    int running;
    int days;
    long maxbuy;
    int bonus;
    int oldstatus;
    char bigbuf[1280];
    register int si, di;

    srand((unsigned)time(0));
    gettextinfo(&ti);
    if (ti.currmode == 7 || ti.currmode == 2) g_mono = 1;
    else g_mono = 0;
    if (ti.currmode == 0 || ti.currmode == 1) {
        cputs("DRUGLORD requires 80 column mode.\r\n");
        exit(0);
    }
    intro();
    running = 1;
    do {
        newgame();
        drawmain();
        drawstatus();
        while (1) {
            cursor(0);
            frame(1, 17, 80, 25);
            cprint("\221\n(\221\034Q\221\n)uit de game", 67, 17);
            window(15, 20, 80, 25);
            cprint("\221\n(\221\034B\221\n)uy Drugs,                       (\221\034V\221\n)isit de Bank,\r\n(\221\034S\221\n)ell Drugs,                      Go to de (\221\034H\221\n)ospital,\r\n(\221\034J\221\n)et to another city,             See de (\221\034L\221\n)oan Shark", 15, 20);
            window(1, 1, 80, 25);
            cprint("\221\007Here's a list of yer possible plans, dude:", 20, 18);
            cprint("\221\007Make up your mind \221\011\020 ", 30, 25);
            if (status >= 300) break;
            setcolor(15, 0, 0);
            cursor(1);
            do {
                flushkeys();
                key = toupper(getch());
            } while (key != 'B' && key != 'S' && key != 'J' && key != 'V' && key != 'L' && key != 'H' && key != 'Q');
            cursor(0);
            frame(1, 17, 80, 25);
            switch (key) {
            case 'J':
                drawcities();
                do {
                    gotoxy(1, 25);
                    clreol();
                    cprint("\221\nWhere ya wanna go? ", 1, 25);
                    esc = readfield(2, 1, inbuf);
                    cursor(0);
                    if (esc == 1) break;
                    si = atol(inbuf);
                    if (si < 1 || si > 8) sfx(2);
                    if (si - 1 == curcity) {
                        cprint("\221\rAre you having \221\037flashbacks? \221\rThat's where you are now!", 28, 25);
                        sfx(3);
                        si = 0;
                        delay(500);
                    }
                } while (si < 1 || si > 8);
                cursor(0);
                if (esc == 1) continue;
                if ((long)travelcost(curcity, si - 1) > cash) {
                    cprint("\221\034Start flappin' yer arms, cause you ain't got de dough!", 25, 25);
                    sfx(3);
                    delay(2000);
                    continue;
                }
                cash -= travelcost(curcity, si - 1);
                curcity = si - 1;
                flag791e = 0;
                genprices(&price[0], &price[1], &price[2], &price[3], &price[4], &price[5], &price[6], &price[7]);
                gotoxy(30, 25);
                setcolor(15, 0, 0);
                for (v = 79; v > 1; v -= 2) {
                    gettext(3, 19, v, 22, bigbuf);
                    puttext(1, 19, v - 2, 22, bigbuf);
                    sound((79 - v) * 20 + 1000);
                    delay(delaytime);
                }
                nosound();
                drawstatus();
                for (v = 0; v < 10; v++) flag10[v] = 0;
                break;
            case 'B':
                drawdruglist();
                do {
                    cprint("\221\004What kinda drugs are\r\nya interested in? ", 1, 19);
                    setcolor(14, 0, 0);
                    esc = readfield(1, 0, inbuf);
                    if (esc == 1) break;
                    di = atol(inbuf);
                    if (di < 1 || di > 8) sfx(2);
                } while (di < 1 || di > 8);
                if (esc == 1) continue;
                di--;
                maxbuy = cash / price[di];
                cprint("\221\004How many do ya wanna buy?\r\n(", 1, 22);
                setcolor(11, 0, 1);
                cprintf("%ld", maxbuy);
                cprint("\221\004 max.)", 99, 99);
                setcolor(14, 0, 0);
                do {
                    gotoxy(27, 22);
                    esc = readfield(5, 0, inbuf);
                    if (esc == 1) break;
                    amount = atol(inbuf);
                } while (amount < 0 || inbuf[0] == 0);
                cursor(0);
                if (esc == 1) continue;
                if ((long)amount > maxbuy) {
                    cprint("\221\035Hey man, if ya don't got enough\r\n  cash, don't come 'round here!", 3, 24);
                    sfx(3);
                    delay(1000);
                    continue;
                }
                if (amount > holdfree) {
                    cprint("\221\nDude, you can't hold that much!\r\n  Everyone's got a limit, right?", 3, 24);
                    sfx(3);
                    delay(3000);
                    continue;
                }
                if (amount > 0) {
                    cprint("\221\016* * *   \221\037S O L D ! ! !   \221\016* * *", 1, 25);
                    sfx(4);
                    delay(2000);
                    inv[di] += amount;
                    cash -= (long)amount * price[di];
                    if (basis[di] == 0)
                        basis[di] = price[di];
                    else
                        basis[di] = (price[di] + basis[di]) / 2;
                }
                break;
            case 'S':
                drawdruglist();
                do {
                    cprint("\221\004What kinda drugs do\r\n ya want to unload? ", 1, 19);
                    setcolor(14, 0, 0);
                    esc = readfield(1, 0, inbuf);
                    if (esc == 1) break;
                    di = atol(inbuf);
                    if (di < 1 || di > 8) sfx(2);
                } while (di < 1 || di > 8);
                cursor(0);
                if (esc == 1) continue;
                di--;
                if (inv[di] == 0) {
                    cprint("\221\rGive me a break, man, you ain't got\r\nany of that!", 1, 22);
                    sfx(3);
                    delay(1000);
                    continue;
                }
                do {
                    cprint("\221\004How many do ya wanna sell? ", 1, 22);
                    setcolor(14, 0, 0);
                    esc = readfield(5, 0, inbuf);
                    if (esc == 1) break;
                    amount = atol(inbuf);
                } while (amount < 0 || inbuf[0] == 0);
                cursor(0);
                if (esc == 1) continue;
                if (inv[di] < amount) {
                    cprint("\221\025I happen to know that you ain't that\r\nmuch on ya, so quit trying to burn me!", 1, 24);
                    sfx(3);
                    delay(3000);
                    continue;
                }
                if (amount > 0) {
                    cprint("\221\036You got yerself a deal, dude!", 1, 25);
                    sfx(4);
                    delay(2000);
                    if (inv[di] - amount == 0) {
                        if (di < 3) bonus = (3 - di) * 3;
                        else bonus = 1;
                        oldstatus = status;
                        status += (int)(((double)price[di] / (double)(unsigned long)basis[di] - 1.0) * 3.0 * bonus);
                        totalDeals++;
                        if (status > oldstatus) goodDeals++;
                        basis[di] = 0;
                    }
                    inv[di] -= amount;
                    cash += (long)amount * price[di];
                }
                break;
            case 'V':
                cprint("\221\017You arrive at the First World Bank of America.\r\n\n               \221\nWould ya like to (\221\034D\221\n)eposit  some moola,\r\n                             or (\221\034W\221\n)ithdraw some moola? ", 20, 18);
                cursor(1);
                do {
                    si = getch();
                    si = toupper(si);
                } while (si != 'D' && si != 'W' && si != 27);
                if (si == 27) continue;
                switch (si) {
                case 'D':
                    cprint("\221\013How much 'ya gonna rathole? ", 5, 23);
                    break;
                case 'W':
                    cprint("\221\013How much 'ya gonna pull out? ", 5, 23);
                    break;
                }
                setcolor(15, 0, 0);
                esc = readfield(10, 0, inbuf);
                cursor(0);
                if (esc == 1) continue;
                lamt = atol(inbuf);
                if (lamt <= 0) continue;
                switch (si) {
                case 'D':
                    if (lamt > cash) {
                        cprint("\221\035You ain't got that much to deposit!", 25, 25);
                        sfx(3);
                        delay(1000);
                        continue;
                    } else {
                        cprint("\221\036Done!", 37, 25);
                        sfx(2);
                        delay(2000);
                        cash -= lamt;
                        bank += lamt;
                        drawstatus();
                    }
                    continue;
                case 'W':
                    if (lamt > bank) {
                        cprint("\221\035You ain't got that much to pull out!", 25, 25);
                        sfx(3);
                        delay(1000);
                    } else {
                        cprint("\221\036Done!", 37, 25);
                        sfx(2);
                        delay(2000);
                        cash += lamt;
                        bank -= lamt;
                        drawstatus();
                    }
                    continue;
                }
                if (esc == 1) continue;
            case 'L':
                cprint("\221\013Ya set foot in the 'sharks office.  You've been in locker rooms that smelled\r\nbetter than dis place!  His boys are lookin' tough in their nice goon suits,\r\nbut you don't pay dem no never-mind, cause the head-honcho, Guido is talkin...\r\n\n                    \221\nHe say, you wanna (\221\034B\221\n)orrow some cash,\r\n                      or do you wanna (\221\034P\221\n)ay me back? ", 1, 17);
                cursor(1);
                do {
                    si = getch();
                    si = toupper(si);
                } while (si != 'B' && si != 'P' && si != 27);
                cursor(0);
                if (si == 27) continue;
                switch (si) {
                case 'B':
                    if (flag79fa == 1) {
                        cprint("\221\032\"Man, yer pushin' yer luck luck! I ought'ta kill you just for askin'!!\"", 5, 24);
                        delay(2000);
                        break;
                    }
                    if (status <= 0) {
                        cprint("\221\n\"Look, whoever you are, you're \221\037NOBODY \221\nto me.  Get outta here!\"", 5, 24);
                        delay(2000);
                        break;
                    }
                    frame(1, 21, 80, 25);
                    cprint("\221\nHow much ya wanna borrow? ", 10, 22);
                    esc = readfield(10, 0, inbuf);
                    cursor(0);
                    if (esc == 1) continue;
                    lamt = atol(inbuf);
                    if (lamt <= 0) continue;
                    if (lamt < 1000) {
                        cprint("\221\034\"Sorry, bud, but I got a \221\036$1000 \221\034minimum.  Got no time for penny-ante stuff!\"", 1, 25);
                        sfx(3);
                        delay(1000);
                        continue;
                    }
                    if (lamt / 10000 >= status) {
                        cprint("\221\034\"No deal, too big a risk for me! I'm a businessman, ya'know!\"", 10, 25);
                        sfx(3);
                        delay(1000);
                        continue;
                    }
                    days = (int)((log((double)status * 10000.0) - log((double)(debt + lamt))) / 0.182321556794) + 4;
                    if ((lamt + debt) / 10000 > status || (days >= loandays && loanflag == 1)) {
                        cprint("\221\013\"Look, you already owe me ", 1, 24);
                        setcolor(4, 0, 1);
                        cprintf("%ld ", debt);
                        setcolor(11, 0, 0);
                        cputs("bucks, I'm not givin ya THAT much more!!!");
                        cprint("\221\013Christ, what do I look like, Santi-Claus?\"", 20, 25);
                        sfx(3);
                        delay(2000);
                        continue;
                    } else {
                        cprint("\221\r\"Alright, I'll loan ya de money, but if you welch, my boys will come lookin\r\nfor ya! I expect to see you here to pay me in full in ", 1, 24);
                        debt += lamt;
                        cash += lamt;
                        loanflag = 1;
                        loandays = days;
                        setcolor(4, 0, 1);
                        cprintf("%d", loandays - 1);
                        cprint("\221\r days.\"", 99, 99);
                        sfx(2);
                        delay(2000);
                        break;
                    }
                case 'P':
                    if (debt == 0) {
                        cprint("\221\013\"My books show dat you don't owe me nut'in... would youse like to take out\r\na little loan?\"", 1, 24);
                        sfx(3);
                        delay(2000);
                        continue;
                    }
                    frame(1, 21, 80, 25);
                    cprint("\221\nHow much ya wanna pay back? ", 10, 22);
                    esc = readfield(10, 0, inbuf);
                    cursor(0);
                    if (esc == 1) continue;
                    lamt = atol(inbuf);
                    if (lamt <= 0) continue;
                    if (lamt > cash) {
                        cprint("\221\034'fraid you don't got that in cash, my man.", 20, 25);
                        sfx(3);
                        delay(1000);
                        continue;
                    }
                    if (lamt >= debt) debt = 0;
                    else debt -= lamt;
                    cash -= lamt;
                    if (debt > 0) {
                        cprint("\221\013\"OK, but you still owe me ", 20, 24);
                        setcolor(12, 0, 1);
                        cprintf("%ld ", debt);
                        cprint("\221\013buck", 99, 99);
                        if (debt > 1) cputs("s!\"");
                        else cputs("!\"");
                        sfx(3);
                        delay(2000);
                        break;
                    } else {
                        cprint("\221\013\"Paid in full? Lucky for you, eh boys? Anytime yous need a loan, just come\r\nto me, I'll take care of ya...\"", 1, 24);
                        loandays = 0;
                        loanflag = 0;
                        flag79fa = 0;
                        flag7920 = 0;
                        sfx(2);
                        delay(2000);
                        break;
                    }
                }
                break;
            case 'H':
                if (health == 100) {
                    cprint("\221\017Ya have de doc check yous out, but aside from yer bunions\r\n              on yer feet, youse is fit as a fiddle!", 15, 19);
                    pressany();
                    continue;
                }
                bonus = (100 - health) * 10;
                cprint("\221\017You drag yer sorry butt into de loc'l Hospital.  The docs check ye\r\n   out, and come to the conclusion that it'll cost ", 4, 19);
                setcolor(10, 0, 1);
                cprintf("%d", bonus);
                cprint("\221\017 dollars to fix ya up.", 99, 99);
                cprint("\221\rDo ya sign the consent forms? (Y/N) \020 ", 5, 22);
                cursor(1);
                do {
                    key = toupper(getch());
                } while (key != 'Y' && key != 'N' && key != 27);
                cursor(0);
                if (key == 'Y') {
                    if ((long)bonus > cash) {
                        cprint("\221\034You don't have enough dinero up front to foot de bill!", 12, 24);
                        sfx(3);
                        pressany();
                    } else {
                        cash -= bonus;
                        cprint("\221\016OK, yer laid up for ", 5, 24);
                        setcolor(12, 0, 1);
                        cprintf("%d", bonus / 15);
                        cprint("\221\016 days recoup'ratin', and yous feel good as new!", 99, 99);
                        sfx(2);
                        for (v = 1; v < bonus / 15; v++)
                            nextday();
                        health = 100;
                        pressany();
                    }
                }
                break;
            case 'Q':
                cprint("\221\007Are ya sure ya wanna \221\026chicken out \221\007an' \221\024QUIT? \221\007(Y/N) ", 15, 20);
                cursor(1);
                do {
                    si = toupper(getch());
                } while (si != 'Y' && si != 'N');
                if (si == 'N') continue;
                running = 0;
                break;
            }
            nextday();
            drawstatus();
            if (dead || !running) break;
            if (rand() % 10 == 0 && rand() % (status / 20 + 1) > 4) {
                if (rand() % 2 == 0) event(7);
                else event(8);
            }
            if (flag79f6 == 1 || (status >= 100 && rand() % 400 < status && rand() % 8 == 0))
                event(0);
            if (dead || !running) break;
            if (rand() % 5 == 0)
                event(rand() % 10);
            if (dead || !running) break;
            if (loandays < 0 && loanflag == 1) {
                frame(1, 17, 80, 25);
                cprint("\221\013You're ambushed in a dark alley by Guido's thugs, must be 'bout ten guys!\r\nThe boss-man say \221\027\"Oh, man, you should'na missed de deadline, now me and\r\nboys are gonna have to break your legs! Maybe next time you'll learn, eh?\"\221\013\r\nDey take yous from all directions, you ain't gotta chance...\r\nOne steps out with a \221\026baseball bat \221\013and...\r\n", 1, 17);
                delay(3000);
                flag7920++;
                for (v = 0; v < flag7920; v++) {
                    frame(1, 23, 80, 23);
                    delay(500);
                    cprint("\221\014* * *    \221\024C R A C K ! !   \221\014* * *", 25, 23);
                    health -= 25;
                    flag79fa = 1;
                    drawstatus();
                    cursor(0);
                    sfx(3);
                    delay(1500);
                }
                if (health > 0) {
                    cprint("\221\016\"Oh, one more 'ting... you got a week to come up with dinero, or we'll\r\n be back, comprende?\"", 1, 24);
                    delay(3000);
                    loandays = 7;
                } else
                    dead = 1;
                pressany();
            }
            if (dead || !running) break;
            drawstatus();
        }
        if (running) {
            if (dead) restorescreen();
            else endgame();
        }
        running = playagain();
    } while (running);
    clrscr();
    puttext(3, 2, 77, 20, buf67d0);
    cprint("\221\036Thank you for playing DrugLord.", 25, 22);
    cprint("Send For The Registered Version Today!", 22, 23);
    cursor(1);
    gotoxy(1, 24);
    exit(0);
}
