#include <stdio.h>
#include <conio.h>
#include <bios.h>

void setcolor(int fg, int bg, char blink);
extern void cursor();
extern void cprint();
extern void monofix();
extern char tbuf[7][124];
extern int delaytime;

void intro()
{
    char ch;
    int prog;
    int count;
    int step;
    int round;
    long t0;
    int limit;
    int toggle;
    register int x, y;

    round = 0;
    cursor(0);
    clrscr();
    setcolor(14, 0, 0);
    monofix(tbuf);
    count = 0;
    t0 = biostime(0, 0L);
    while (biostime(0, 0L) == t0)
        ;
    t0 = biostime(0, 0L);
    while (biostime(0, 0L) == t0) {
        puttext(10, 5, 71, 10, tbuf);
        count++;
    }
    if (count == 0) count = 1;
    delaytime = 31 - 55 / count;
    if (delaytime < 0) delaytime = 0;
    cprint("\221\012Version v2.01", 35, 13);
    cprint("\221\001\020\221\011\020\221\013\020   \221\036UNREGISTERED VERSION   \221\013\021\221\011\021\221\001\021", 24, 16);
    cprint("\221\014Written by Phil Erwin", 30, 19);
    cprint("Copyright 1991-93  FantasyWare Inc.", 23, 20);
    sound(0);
    delay(500);
    nosound();
    limit = 441;
    toggle = 1;
    for (; round < 10; round++) {
        prog = 0;
        toggle = !toggle;
        setcolor(rand() % 15, 0, toggle);
        x = 0;
        y = 0;
        step = rand() % 6 * 62 + 13;
        while (prog < limit) {
            x += step;
            if (x >= 62) {
                do {
                    x -= 62;
                    y++;
                    if (y > 6) y = 0;
                } while (x >= 62);
            }
            ch = tbuf[y][x * 2];
            if (ch == (char)0xb1) {
                gotoxy(x + 10, y + 5);
                cputs("\261");
                delay(5);
                prog++;
            }
            if (kbhit()) {
                prog = 0x2bd;
                round = 11;
            }
        }
    }
}
