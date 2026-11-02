#include <stdio.h>
#include <conio.h>

extern char *drugname[];
extern char gbuf[];
void setcolor(int fg, int bg, char blink);
void fillch(char c, int n, char *buf);
extern void cursor();
extern void cprint();

void drawdruglist()
{
    register int i;

    cursor(0);
    cprint("Which drug?", 45, 18);
    fillch(0xc4, 40, gbuf);
    setcolor(14, 0, 0);
    cprint(gbuf, 36, 19);
    for (i = 0; i < 4; i++) {
        gotoxy(43, i + 21);
        setcolor(12, 0, 1);
        cprintf("%1d. ", i + 1);
        setcolor(10, 0, 0);
        cprintf("%-15s", drugname[i]);
        gotoxy(60, i + 21);
        setcolor(12, 0, 1);
        cprintf("%1d. ", i + 5);
        setcolor(10, 0, 0);
        cprintf("%-15s", drugname[i + 4]);
    }
    cursor(1);
}
