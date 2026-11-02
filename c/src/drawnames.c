#include <stdio.h>
#include <conio.h>

extern char *drugname[];
void setcolor(int fg, int bg, char blink);

void drawnames()
{
    int i;

    setcolor(15, 0, 0);
    for (i = 0; i < 8; i++) {
        gotoxy(5, i + 7);
        cprintf("%-7s", drugname[i]);
        gotoxy(28, i + 7);
        cprintf("%-7s", drugname[i]);
    }
}
