#include <conio.h>

extern char gbuf[];
void fillch(char c, int n, char *buf);

void box(chars, x1, y1, x2, y2)
char *chars;
int x1, y1, x2, y2;
{
    int y;

    gotoxy(x1, y1); putch(chars[0]);
    gotoxy(x2, y1); putch(chars[2]);
    gotoxy(x2, y2); putch(chars[4]);
    gotoxy(x1, y2); putch(chars[6]);
    gotoxy(x1 + 1, y1);
    fillch(chars[1], x2 - x1 - 1, gbuf);
    cputs(gbuf);
    gotoxy(x1 + 1, y2);
    fillch(chars[5], x2 - x1 - 1, gbuf);
    cputs(gbuf);
    for (y = y1 + 1; y < y2; y++) {
        gotoxy(x1, y); putch(chars[3]);
        gotoxy(x2, y); putch(chars[7]);
    }
}
