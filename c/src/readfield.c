#include <stdio.h>
#include <conio.h>

extern char gbuf[];
void fillch(char c, int n, char *buf);
extern void cursor();
extern void flushkeys();
extern void sfx();

int readfield(width, flag, buf)
int width;
char flag;
char *buf;
{
    int sx;
    int sy;
    char c;
    register int i;

    i = 0;
    sx = wherex();
    sy = wherey();
    cursor(1);
    fillch(' ', width, gbuf);
    cputs(gbuf);
    while (1) {
        if (width - 1 > i) {
            gotoxy(sx + i, sy);
            cputs(" ");
        }
        gotoxy(sx + i, sy);
        c = getch();
        if (c == 0) {
            flushkeys();
            continue;
        }
        if (c == 27)
            return 1;
        if (c == 13) {
            buf[i] = 0;
            break;
        }
        if (c == 8) {
            if (i > 0) {
                buf[i] = 0;
                i--;
            }
            buf[i] = 0;
            cputs(" ");
            continue;
        }
        buf[i] = c;
        gotoxy(sx + i, sy);
        cprintf("%c", buf[i]);
        if (width - 1 > i)
            cputs(" ");
        if (width - 1 == i) {
            if (flag == 0)
                break;
            else {
                sfx(2);
                gotoxy(sx + i, sy);
                cputs(" ");
                i--;
            }
        }
        i++;
    }
    buf[i + 1] = 0;
    if (flag == 0 && width > 1)
        buf[i] = 0;
    return 0;
}
