#include <conio.h>
#include <string.h>

extern void setcolor();

void cprint(s, x, y)
char *s;
int x;
int y;
{
    int blink;
    char save;
    char buf[500];
    register int i, j;

    i = 0;
    strcpy(buf, s);
    if (x != 99)
        gotoxy(x, y);
    while (buf[i] != 0) {
        if (buf[i] == (char)0x91) {
            if (buf[i + 1] > 15) {
                buf[i + 1] -= 16;
                blink = 1;
            } else
                blink = 0;
            setcolor(buf[i + 1], 0, blink);
            i += 2;
        }
        j = i;
        while (buf[i] != 0 && buf[i] != (char)0x91)
            i++;
        save = buf[i];
        buf[i] = 0;
        cputs(buf + j);
        buf[i] = save;
    }
}
