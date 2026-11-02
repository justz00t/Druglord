#include <stdio.h>
#include <conio.h>

extern char *cityname[];
extern int curcity;
void setcolor(int fg, int bg, char blink);
extern void cursor();
extern void cprint();
extern void box();
extern int travelcost();

void drawcities()
{
    int j;
    register int i, d;

    cursor(0);
    box(1, 17, 80, 25);
    cprint("\x91\005Drug Cities:", 34, 17);
    for (d = 0; d < 4; d++) {
        for (j = 0; j <= 1; j++) {
            i = j * 4 + d;
            if (i > 8) break;
            gotoxy(j * 35 + 10, d + 19);
            setcolor(12, 0, 1);
            cprintf("%2d. ", i + 1);
            setcolor(11, 0, 0);
            cprintf("%-20s", cityname[i]);
            if (i == curcity) continue;
            cputs("(");
            setcolor(2, 0, 0);
            cprintf("$%d", travelcost(curcity, i));
            cprint("\x91\x0b)", 11, 0);
        }
    }
}
