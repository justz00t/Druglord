#include <conio.h>

extern char *txt[];
extern char scrmain[];
void setcolor(int fg, int bg, char blink);
extern void cprint();
extern void cursor();

void drawmain()
{
    cursor(0);
    setcolor(7, 0, 0);
    clrscr();
    puttext(1, 1, 80, 16, scrmain);
    setcolor(14, 0, 0);
    cprint(txt[0], 24, 2);
    cprint(txt[0], 52, 2);
    setcolor(9, 0, 1);
    cprint(txt[1], 32, 2);
    setcolor(11, 0, 0);
    cprint(txt[2], 6, 3);
    setcolor(6, 0, 1);
    cprint(txt[3], 6, 5);
    setcolor(4, 0, 1);
    cprint(txt[4], 30, 5);
    setcolor(6, 0, 0);
    cprint(txt[5], 53, 4);
    cprint(txt[6], 57, 5);
    setcolor(10, 0, 0);
    cprint(txt[7], 57, 7);
    cprint(txt[8], 54, 8);
    cprint(txt[9], 54, 9);
    setcolor(13, 0, 0);
    cprint(txt[10], 52, 11);
    cprint(txt[11], 53, 12);
    setcolor(11, 0, 0);
    cprint(txt[12], 51, 14);
    cprint(txt[13], 55, 15);
    setcolor(11, 0, 0);
    cprint("v2.01", 74, 2);
}
