#include <conio.h>

extern struct text_info ti;

void frame(x1, y1, x2, y2)
int x1, y1, x2, y2;
{
    gettextinfo(&ti);
    window(x1, y1, x2, y2);
    clrscr();
    textmode(ti.currmode);
}
