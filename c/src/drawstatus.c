#include <stdio.h>
#include <conio.h>

extern char *cityname[];
extern char *gunname[];
extern int inv[8];
extern long price[8];
extern int holdused, hold, curcity, loanflag, loandays, ngun, guntype, status, health, hp2;
extern int date1, date2, date3;
extern long cash, bank, debt;
void setcolor(int fg, int bg, char blink);
extern void cursor();
extern void drawnames();

void drawstatus()
{
    register int i;

    holdused = 0;
    cursor(0);
    drawnames();
    for (i = 0; i < 8; i++) {
        gotoxy(16, i + 7);
        if (inv[i] > 0) setcolor(14, 0, 1);
        else setcolor(7, 0, 0);
        cprintf("%6d", inv[i]);
        gotoxy(38, i + 7);
        setcolor(12, 0, 1);
        cprintf("%10ld", price[i]);
        holdused += inv[i];
    }
    setcolor(14, 0, 1);
    holdused = hold - holdused;
    gotoxy(63, 4);
    cprintf("%-15s", cityname[curcity]);
    gotoxy(63, 5);
    cprintf("%-6d (%d max)", holdused, hold);
    gotoxy(63, 7);
    cprintf("$%-10ld", cash);
    gotoxy(63, 8);
    cprintf("$%-10ld", bank);
    gotoxy(63, 9);
    cprintf("$%-10ld", debt);
    gotoxy(74, 9);
    if (loanflag == 1) {
        cputs("(   )");
        if (loandays <= 2) setcolor(140, 0, 1);
        gotoxy(75, 9);
        cprintf("%3d", loandays);
        setcolor(14, 0, 1);
    } else
        cputs("     ");
    gotoxy(63, 11);
    cprintf("%-6d", ngun);
    gotoxy(63, 12);
    cprintf("%-13s", gunname[guntype]);
    gotoxy(63, 14);
    cprintf("%-6d", status);
    gotoxy(63, 15);
    if (health <= 25) setcolor(132, 0, 1);
    cprintf("%-4d", health);
    setcolor(14, 0, 1);
    gotoxy(12, 3);
    cprintf("%#02d/%#02d/%#02d", date1, date2, date3);
}
