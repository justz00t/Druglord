#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>
#include <math.h>

void setcolor(int fg, int bg, char blink);
int readfield(int width, char flag, char *buf);
void marketevent(int count, int rng, float mul);
extern int dice();
extern void drawhp();
extern void drawmain();
extern void drawstatus();
extern void pressany();
extern void frame();
extern void cprint();
extern void cursor();
extern void sfx();
extern char *txt[];
extern char *gunname[];
extern char *gunlong[];
extern int gunpow[];
extern char flag10[10];
extern char inbuf2[];
extern char buf4328[];
extern int esc, ngun, guntype, status, health, hold, dead, flag79f6, flag791e, flag79d2;
extern long cash;
extern long price[8];

void event(kind)
int kind;
{
    int choice;
    int nfr;
    int hp[11];
    int chp[11];
    int ctype;
    int tgt;
    int copsleft;
    int alive;
    int ftgt;
    int loot;
    int state;
    int maxshots;
    int shots;
    char unused1;
    char key;
    int v20;
    int v1e;
    int v1c;
    int v1a;
    long bm;
    float f;
    char *lbl[2] = { "Health", "Status" };
    char buf[12];
    register int si, di;

    cursor(0);
    if (flag10[kind] == 1) return;
    frame(1, 17, 80, 25);
    switch (kind) {
    case 0:
        flag79f6 = 0;
        state = 0;
        if (guntype < 4) maxshots = 1;
        else maxshots = 3;
        cprint("\221\004The \221\011cops \221\004are on your tail!!!", 27, 17);
        sfx(5);
        cprint("\221\012Do you wanna   \221\0341\221\012) Go at it alone", 15, 19);
        cprint("\221\0342\221\012) Call a friend (1 status pt.)", 30, 20);
        cprint("\221\0343\221\012) Call a posse (5 status pts.)", 30, 21);
        choice = 0;
        while (1) {
            do {
                frame(1, 23, 80, 25);
                cprint("\221\016Call it \020 ", 20, 23);
                cursor(1);
                esc = readfield(1, 0, inbuf2);
                cursor(0);
                choice = atol(inbuf2);
            } while (choice < 1 || choice > 3);
            if ((choice == 2 && status == 0) || (choice == 3 && status < 5)) {
                cprint("\221\014You don't have enough points!", 25, 25);
                sfx(3);
                delay(2000);
                frame(25, 25, 80, 25);
            } else
                break;
        }
        switch (choice) {
        case 1:
            nfr = 0;
            break;
        case 2:
            nfr = 1;
            status--;
            break;
        case 3:
            nfr = rand() % 4 + status / 100 + 4;
            if (nfr > 8) nfr = 8;
            status -= 5;
            break;
        }
        frame(1, 1, 80, 25);
        puttext(1, 1, 80, 22, buf4328);
        setcolor(14, 0, 0);
        cprint(txt[0], 24, 2);
        cprint(txt[0], 52, 2);
        setcolor(12, 0, 1);
        cprint(txt[1], 32, 2);
        setcolor(10, 0, 1);
        cprint(lbl[0], 9, 8);
        cprint(lbl[0], 49, 8);
        cprint(lbl[1], 28, 8);
        cprint(lbl[1], 68, 8);
        setcolor(14, 0, 1);
        cprint(txt[11], 10, 6);
        cprint(txt[11], 50, 6);
        cprint(txt[10], 9, 5);
        setcolor(12, 0, 0);
        gotoxy(20, 5);
        cprintf("%-10d", ngun);
        setcolor(6, 0, 0);
        gotoxy(20, 6);
        cprintf("%-15s", gunname[guntype]);
        hp[0] = health;
        setcolor(6, 0, 0);
        for (si = 1; si <= nfr; si++) {
            hp[si] = rand() % 10 + dice(10, status / 50) + 25;
            gotoxy(1, si + 10);
            if (choice == 2) cprintf("%10s:", "Friend");
            if (choice == 3) cprintf("%6s #%2d:", "Posse", si);
        }
again:
        di = rand() % 8 + 3;
        ctype = status / (di * 2);
        if (rand() % 20 == 0) {
            if (ctype < 2) {
                di = rand() % 4 + 1;
                ctype = status / di;
                if (ctype > 3) ctype = 3;
                if (ctype < 2) ctype = 2;
            } else {
                di = rand() % 3 + 8;
                ctype = 3;
            }
        } else {
            if (ctype < 2) goto again;
            if (ctype > 3) goto again;
        }
        gotoxy(60, 6);
        setcolor(9, 0, 0);
        cprintf("%-15s", gunname[ctype]);
        setcolor(9, 0, 0);
        for (si = 0; si < di; si++) {
            gotoxy(42, si + 11);
            cprintf("%6s #%2d", "Cop", si + 1);
            chp[si] = dice(15, 2) + 15;
        }
        copsleft = di;
        alive = nfr + 1;
        for (si = 0; si <= nfr; si++)
            drawhp(hp[si], si, 0);
        for (si = 0; si < di; si++)
            drawhp(chp[si], si, 1);
        do {
            frame(1, 24, 80, 25);
            cprint("\221\017Do ya wanna (\221\034F\221\017)ight or (\221\034R\221\017)un? ", 5, 24);
            cursor(1);
            do {
                key = toupper(getch());
            } while (key != 'F' && key != 'R');
            cursor(0);
            putch(key);
            while (1) {
                switch (key) {
                case 'F':
                    if (state != 2) {
                        for (si = 0; si <= nfr; si++) {
                            if (hp[si] == 0) continue;
                            if (si < ngun) cprint("\221\014Firing", 28, si + 10);
                            else cprint("\221\014Punching", 27, si + 10);
                            shots = 0;
                            do {
                                do {
                                    tgt = rand() % di;
                                } while (chp[tgt] == 0);
                                sfx(guntype + 10);
                                if (rand() % 2 == 0) {
                                    cprint("\221\024*  Hit!  *", 67, tgt + 11);
                                    sfx(8);
                                    delay(100);
                                    if (si < ngun) chp[tgt] -= dice(6, gunpow[guntype]);
                                    else chp[tgt] -= dice(4, 1);
                                    if (chp[tgt] <= 0) {
                                        chp[tgt] = 0;
                                        copsleft--;
                                    }
                                } else
                                    cprint("\221\016- Missed! -", 67, tgt + 11);
                                delay(200);
                                drawhp(chp[tgt], tgt, 1);
                                shots++;
                                if (hp[0] == 0 || copsleft == 0) break;
                            } while (guntype > 3 && shots < maxshots && copsleft > 0 && si < ngun);
                            drawhp(hp[si], si, 0);
                            if (copsleft == 0) break;
                        }
                    }
                    if (copsleft == 0) break;
                    state = 0;
                    for (si = 0; si < di; si++) {
                        if (chp[si] == 0) continue;
                        cprint("\221\014Firing", 68, si + 11);
                        shots = 0;
                        do {
                            ftgt = rand() % (nfr + 1);
                        } while (hp[ftgt] == 0);
                        sfx(ctype + 10);
                        if (rand() % 2 == 0) {
                            cprint("\221\024*  Hit!  *", 26, ftgt + 10);
                            sfx(8);
                            delay(100);
                            hp[ftgt] -= dice(6, gunpow[ctype]);
                            if (hp[ftgt] <= 0) {
                                status--;
                                hp[ftgt] = 0;
                                alive--;
                            }
                        } else
                            cprint("\221\016- Missed! -", 26, ftgt + 10);
                        delay(200);
                        drawhp(hp[ftgt], ftgt, 0);
                        shots++;
                        drawhp(chp[si], si, 1);
                        if (hp[0] == 0 || alive == 0) break;
                    }
                    break;
                case 'R':
                    cprint("\221\013You take off running... ", 20, 25);
                    delay(3000);
                    if (rand() % 10 + di / 3 < rand() % 10 + nfr / 3) {
                        cprint("\221\036and ya got away!!!", 99, 99);
                        health = hp[0];
                        state = 1;
                    } else {
                        cprint("\221\013and ya didn't get away!!!", 99, 99);
                        state = 2;
                        key = 'F';
                        continue;
                    }
                    break;
                }
                break;
            }
        } while (copsleft > 0 && alive > 0 && hp[0] > 0 && state != 1);
        if (state == 1) {
            delay(3000);
            drawmain();
            break;
        }
        health = hp[0];
        if (health == 0) {
            dead = 1;
            break;
        }
        frame(1, 23, 80, 25);
        cprint("\221\012Yous wasted 'em!!! Way ta go, man!!!", 23, 23);
        delay(1000);
        v20 = 1;
        if (ctype < guntype && di <= ngun) goto skipask;
        setcolor(7, 0, 0);
        gotoxy(21, 24);
        if (di == 1) strcpy(buf, "his gun");
        else strcpy(buf, "their guns");
        cprintf("Do you wanna take %s, man? (Y/N) ", buf);
        cursor(1);
        do {
            key = toupper(getch());
        } while (key != 'Y' && key != 'N');
        cursor(0);
        if (key == 'Y') {
            cprint("\221\016You \221\014want\221\016 'em, you \221\014got \221\016'em!  What a bunch'a \221\006chumps!", 15, 25);
            v20 = 0;
            if (guntype == ctype) ngun += di;
            else {
                guntype = ctype;
                ngun = di;
            }
        } else
            cprint("\221\011OK, man, it's yer call.", 29, 25);
        delay(2000);
skipask:
        if (v20 == 1) {
            cprint("\221\013You sell the ", 5, 24);
            loot = di * ctype * 200;
            cprintf("%d guns that were on the pigs you wasted for %d bucks!", di, loot);
            cprint("\221\013This'll teach 'em not to mess with ya, huh?", 20, 25);
            cash += loot;
            status += di;
            delay(4000);
        }
        if (dead == 0) drawmain();
        break;
    case 1:
        if (flag791e == 1) break;
        cprint("\221\013\"Hey man, check dis out!  Word has it on de street dat deres des cops, see?\r\nThey musta decided that workin' for 25 G's a year ain't enough, so de've\r\ndecided ta sell some of de take drugs confiscated on de side! De market is\r\nflooded with ", 1, 18);
        setcolor(6, 0, 1);
        marketevent(3, 30, 0.7f);
        setcolor(11, 0, 0);
        cprintf("!\"");
        sfx(6);
        pressany();
        break;
    case 2:
        cprint("\221\007\"Hey, I heard it through de grapevine dat deres some cheap homemade stuff\r\navailable from some of the homeboys downtown.  Real primo stuff, dey say!\r\nAll I hear on de streets is 'Cheap ", 1, 18);
        setcolor(6, 0, 1);
        marketevent(3, 40, 0.6f);
        setcolor(7, 0, 0);
        cprintf("!'\"");
        pressany();
        break;
    case 3:
        window(15, 17, 80, 25);
        cprint("\221\007\332\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\277\r\n\263  * * *   \221\037E X T R A ! !   E X T R A ! !\221\007   * * *  \263\r\n\263               \221\037Read all about it!!\221\007               \263\r\n\303\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\264\r\n\263  After six months of undercover investigation,  \263\r\n\263  police close in and make a major drug bust!    \263\r\n\263  This will no doubt affect street drug prices.  \263\r\n\300\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\331", 15, 17);
        window(1, 1, 80, 25);
        for (si = 0; si < 8; si++)
            price[si] = price[si] * ((double)(rand() % 25) / 100.0 + 1.05);
        sfx(7);
        pressany();
        break;
    case 4:
        cprint("\221\r\"Dude, I got some news for ya... a factory of one of de cities biggest suppliersof ", 1, 18);
        setcolor(6, 0, 1);
        marketevent(1, 40, 1.2f);
        setcolor(13, 0, 0);
        if (rand() % 2 == 1) cputs(" burned down");
        else cputs(" blew up");
        cprint("last night.  I don't have to tell ya what this'll on de\r\nstreet...\"", 99, 99);
        pressany();
        break;
    case 5:
        if (flag79d2 == 0) break;
        cprint("\221\006Ya come back to yer place tonight and find dat yer sexy main squeeze tought\r\ndat it wouldst be a good idea to use some of yer weed to make brownies!!!\r\nAll tings considered, yous take it pretty well.  After all, if ya were mad,\r\nyou would've \221\007chopped \221\006up her body \221\014before \221\006dumping it in de river!", 1, 17);
        flag79d2 = 0;
        pressany();
        break;
    case 6:
        cprint("\221\014Yer walking along de street at night, when from outta nowhere jumps dis goon!\r\nHe waves a 9mm Beretta at yous and demands yer fork over all yer money.", 1, 17);
        while (1) {
            if (status >= 10 && rand() % 20 != 0) {
                cprint("\221\016Do ya wanna use yer status as a major drug dealer to get outta this? (Y/N) ", 1, 20);
                cursor(1);
                do {
                    key = toupper(getch());
                } while (key != 'Y' && key != 'N');
                cursor(0);
                if (key == 'Y') {
                    cprint("\221\014Yous tell him whos he's dealin' wit.  He looks nervous, asks fer yer pardon,den runs off down an alley!", 1, 22);
                    status -= 10;
                    break;
                }
            }
            if (cash < 50) {
                cprint("\221\013\"This is all you got? &%&*! this, man!\"", 20, 21);
                cprint("He looks at ya like a crazed animal, and pulls de trigger!", 10, 22);
                cprint("\221\016* * *   \221\034B L A M   \221\016* * *", 28, 23);
                sfx(13);
                health -= rand() % 6 + rand() % 6 + 2;
            } else
                cprint("\221\rHe laughs merrily as he takes yer cash and runs off down de street!", 5, 22);
            cash = 0;
            break;
        }
        pressany();
        break;
    case 7:
        if (hold >= 300) break;
        do {
            v20 = rand() % 10 + 1;
            if (rand() % 5 == 0) {
                v20 += rand() % 10 + 1;
                if (rand() % 5 == 0)
                    v20 += rand() % 10 + 1;
            }
            v20 *= 10;
        } while (hold + 50 < v20 || v20 <= hold);
        bm = pow((double)v20, 3.34) * 0.0634;
        bm = (long)((double)((rand() % 200 + 1) * bm) / 10000.0) * 100;
        cprint("\221\nYou are approached by a shady-lookin guy in a long dark trenchcoat.  He says:", 1, 17);
        cprint("\221\011\"Hey, bud, I got a \221\036special \221\011trenchcoat for ya, just your style!", 10, 19);
        cprint("It's got a capacity of ", 11, 20);
        setcolor(12, 0, 1);
        cprintf("%d", v20);
        cprint("\221\011 and I won't take under ", 99, 99);
        setcolor(12, 0, 1);
        cprintf("$%ld", bm);
        cprint("\221\011 for it.\"", 99, 99);
        cprint("\221\016Is it a deal? (Y/N) \020 ", 1, 23);
        cursor(1);
        do {
            key = toupper(getch());
        } while (key != 'Y' && key != 'N');
        cursor(0);
        if (key == 'Y') {
            if (cash < bm)
                cprint("\221\011\"Sorry Charly, I don't take credit! Maybe when you're a little more solvent!\"", 1, 24);
            else {
                cprint("\221\011\"You bought yourself one hell of a trenchcoat!\"", 16, 24);
                cash -= bm;
                hold = v20;
            }
        } else
            cprint("\221\011\"OK, it's your loss, man.  What a shame!\"", 20, 24);
        pressany();
        break;
    case 8:
        v20 = rand() % 5;
        if (rand() % 10 == 0) v1e = rand() % 6 + 5;
        else v1e = rand() % 5 + 2;
        v1c = v20 * 200 + 200;
        v1a = v1e * v1c;
        cprint("\221\rJuan, one of the most powerful underground gun smugglers in the world,\r\ncomes to you with an offer:", 1, 17);
        cprint("\221\007\"I have a shipment of ", 1, 20);
        setcolor(12, 0, 1);
        cprintf("%s's", gunlong[v20]);
        cprint("\221\007 coming in tonight.  I'm", 99, 99);
        cprint("distributing them in shipments of ", 1, 21);
        setcolor(12, 0, 1);
        cprintf("%d ", v1e);
        cprint("\221\007at ", 99, 99);
        setcolor(12, 0, 1);
        cprintf("$%d ", v1c);
        cprint("\221\007each, for a total of ", 99, 99);
        setcolor(12, 0, 1);
        cprintf("$%d", v1a);
        setcolor(7, 0, 0);
        cputs(".\"");
        cprint("\221\rAre you interested in an order? (Y/N) \020 ", 20, 23);
        cursor(1);
        do {
            key = toupper(getch());
        } while (key != 'Y' && key != 'N');
        cursor(0);
        if (key == 'Y') {
            if (v1a > cash)
                cprint("\221\007\"I'm sorry, sir, but I only accept cash \221\017up-front\221\007.  Business is business.\"", 5, 24);
            else {
                cprint("\221\007\"Very well, they will be delivered by tomorrow.  Good doing business with you.\"", 1, 24);
                ngun = v1e;
                guntype = v20 + 1;
                cash -= v1a;
            }
        } else
            cprint("\221\007\"I'm sorry, sir, but I only accept cash \221\017up-front\221\007.  Business is business.\"", 15, 24);
        pressany();
        break;
    case 9:
        if (cash < 10000L) break;
        f = rand() % 30 / 100.0 + 0.7;
        bm = cash * f / 1000.0;
        bm = bm * 1000;
        cprint("\221\nYous get a note in de mail from a business associate of yers.  He says dat\r\nif yous don't deposit ", 1, 17);
        setcolor(12, 0, 1);
        cprintf("$%ld ", bm);
        cprint("\221\nin his Swiss bank account, he's gonna \221\026rat \221\nto de\r\ncops everything dat he knows about you.", 99, 99);
        cprint("\221\rDo you pay the blackmail? (Y/N) \020 ", 1, 21);
        cursor(1);
        do {
            key = toupper(getch());
        } while (key != 'Y' && key != 'N');
        cursor(0);
        if (key == 'Y') {
            cprint("\221\rYou wire de money into his account.  That really \221\034^$&?*@ \221\rsucks!!!", 1, 24);
            cash -= bm;
        } else {
            cprint("\221\014Yeah, man, nobody pushes you around! I respect yer integrity!!!", 9, 24);
            if (rand() % 5 != 0) {
                flag79f6 = 1;
                flag10[0] = 0;
            }
        }
        pressany();
        break;
    }
    flag10[kind] = 1;
    if (dead == 0) drawstatus();
}
