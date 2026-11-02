#include <conio.h>

extern int g_mono;

void setcolor(fg, bg, blink)
int fg;
int bg;
char blink;
{
    if (g_mono == 1) {
        textcolor(15);
        if (bg)
            textbackground(7);
        else
            textbackground(0);
        if (blink == 0)
            lowvideo();
        else
            highvideo();
    } else {
        textcolor(fg);
        textbackground(bg);
    }
}
