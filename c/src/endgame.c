#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <math.h>

void setcolor(int fg, int bg, char blink);
extern void frame();
extern void box();
extern void cprint();
extern void sfx();
extern char *gunname[];
extern char rep[][50];
extern char savebuf2[], firebuf[], barbuf[];
extern long cash, bank, debt;
extern int goodDeals, totalDeals, guntype, ngun, totaldays;

void endgame()
{
    int i, j, k, x, row, n, d, m, pct, rank;
    char *titles[12] = { "Loser!", "Dopehead", "Peddler", "Pusher", "Gangster", "Dealer",
                         "Trafficker", "Racketeer", "Marketeer", "Entrepreneur", "DRUGLORD", "OVERLORD" };
    long thr[11] = { 250000L, 500000L, 600000L, 700000L, 800000L, 850000L,
                     900000L, 950000L, 1000000L, 1100000L, 1250000L };
    char msg[11] = "DRUGLORD!!!";
    int notes[16] = { 100, 150, 500, 400, 100, 75, 25, 500, 1000, 1500, 1000, 750, 500, 200, 100, 50 };
    char bigbuf[1121];
    char key;
    char s[10];
    long score;
    long p2;
    int guns;
    double ex;
    register int si, di;

    delay(1000);
    frame(1, 17, 80, 25);
    for (i = 1; i < 6; i++) {
        frame(15, 20, 65, 20);
        setcolor(i + 9, 0, i % 2 == 1);
        for (j = 0; j < 22; j++) {
            gotoxy(j / 2 * 5 + 15, 20);
            putch(msg[j / 2]);
            if (j < 16) {
                sound(notes[j]);
                delay(20);
            }
        }
        nosound();
        delay(500);
    }
    puttext(1, 1, 80, 25, savebuf2);
    if (ngun < 10) guns = ngun;
    else guns = 10;
    for (i = 1; i <= guns; i++) {
        if (i <= 4) {
            k = (i - 1) / 2 * 11 + (i % 2 == 0) * 5 + 55;
            si = (i % 2 == 0) * 3 + 2;
            puttext(k, si, k + 7, si + 2, firebuf);
        } else {
            k = (i - 5) / 2 * 11 + !(i % 2) * 5 + 1;
            si = (i % 2 == 0) * 3 + 2;
            puttext(k, si, k + 7, si + 2, firebuf);
        }
    }
    n = (int)((cash + bank - 1) / 2000000L) + 1;
    for (si = 0; si < 4; si++) {
        row = si + 11;
        for (k = 0; k < 5; k++) {
            x = k * 7 + 18 - si * 2;
            d = 15 - si * 3 - k;
            if (d < 0) d = 0;
            for (m = 0; m < n; m++) {
                if ((long)(si * 5 + k + 1) * 100000L + (long)m * 2000000L > cash) break;
                puttext(x, row - m, x + 4, row - m, barbuf);
                sound(m * 500 + 1000);
                delay(d + 5);
                sound(m * 500 + 3000);
                delay(d);
            }
        }
    }
    nosound();
    pct = goodDeals * 100 / totalDeals;
    ltoa(cash, s, 10);
    di = strlen(s);
    stpcpy(rep[6] + (0x29 - di), s);
    ltoa(bank, s, 10);
    di = strlen(s);
    stpcpy(rep[7] + (0x29 - di), s);
    ltoa(debt, s, 10);
    di = strlen(s);
    stpcpy(rep[8] + (0x29 - di), s);
    itoa(goodDeals, s, 10);
    di = strlen(s);
    stpcpy(rep[9] + (0x29 - di), s);
    itoa(totalDeals, s, 10);
    di = strlen(s);
    stpcpy(rep[10] + (0x29 - di), s);
    itoa(pct, s, 10);
    di = strlen(s);
    stpcpy(rep[11] + (0x28 - di), s);
    strcat(rep[11], "%");
    di = strlen(gunname[guntype]);
    stpcpy(rep[12] + (0x17 - di), gunname[guntype]);
    stpcpy(rep[12] + 0x17, "'s:\221\036            ");
    itoa(ngun, s, 10);
    di = strlen(s);
    stpcpy(rep[12] + (0x29 - di), s);
    itoa(totaldays, s, 10);
    di = strlen(s);
    stpcpy(rep[13] + (0x29 - di), s);
    ex = exp((double)(cash + bank - debt) * 6.9315e-08);
    score = (long)ex * 75000L;
    if (score > 700000L) score = 700000L;
    p2 = (long)(pow((double)totaldays, -2.12) * 104000000000.0);
    if (p2 > 500000L) p2 = 500000L;
    score += p2;
    score += (long)(pow((double)pct, 8.14) * 2.48e-11);
    score += (long)(guntype * ngun) * 1500L;
    ltoa(score, s, 10);
    di = strlen(s);
    stpcpy(rep[16] + (0x29 - di), s);
    for (rank = 0; rank < 10; rank++) {
        if (thr[rank] > score) break;
    }
    di = strlen(titles[rank]);
    stpcpy(rep[17] + (0x29 - di), titles[rank]);
    setcolor(9, 0, 0);
    box("\311\315\273\272\274\315\310\272", 10, 18, 55, 25);
    cprint(" \221\034\031 ", 50, 25);
    cprint("\221\011[\221\033ESC\221\011] \221\033Finished ", 12, 25);
    setcolor(14, 0, 0);
    for (si = 0; si < 5; si++)
        cprint(rep[si], 15, si + 19);
    si = 0;
    while (1) {
        key = getch();
        setcolor(14, 0, 0);
        if (key == 0) {
            key = getch();
            if (key == 0x48 && si > 0) {
                si--;
                gettext(15, 19, 55, 23, bigbuf);
                puttext(15, 20, 55, 24, bigbuf);
                cprint(rep[si], 15, 19);
                sfx(20);
            }
            if (key == 0x50 && si < 12) {
                si++;
                gettext(15, 20, 55, 24, bigbuf);
                puttext(15, 19, 55, 23, bigbuf);
                cprint(rep[si + 5], 15, 24);
                sfx(20);
            }
            if (si > 0) cprint(" \221\014\030 ", 50, 18);
            else cprint("\221\011\315\315\315", 50, 18);
            if (si < 10) cprint(" \221\034\031 ", 50, 25);
            else cprint("\221\011\315\315\315", 50, 25);
        }
        if (key == 0x1b) break;
    }
}
