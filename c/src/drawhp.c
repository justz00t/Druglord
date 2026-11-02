#include <conio.h>
#include <stdio.h>

void setcolor(int fg, int bg, char blink);

void drawhp(hp, row, side)
int hp, row, side;
{
    register char *s;

    if (side == 0)
        gotoxy(12, row + 10);
    else
        gotoxy(52, row + 11);
    if (hp > 0) {
        s = "Alive";
        setcolor(15, 0, 1);
    } else {
        s = "Dead";
        setcolor(7, 0, 0);
    }
    cprintf("%4d            %-12s", hp, s);
}
