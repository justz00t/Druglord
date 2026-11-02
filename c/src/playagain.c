#include <conio.h>
#include <ctype.h>

void setcolor(int fg, int bg, char blink);
extern void box();
extern void frame();
extern void cprint();
extern void cursor();
extern void flushkeys();
extern char savebuf[];

int playagain()
{
    char c;
    register int r;

    setcolor(15, 0, 0);
    box("\xc9\xcd\xbb\xba\xbc\xcd\xc8\xba", 29, 10, 51, 15);
    frame(30, 11, 50, 14);
    cprint("\x91\014Would ya like ta", 32, 12);
    cprint("\x91\015play again? (Y/N)", 32, 13);
    cursor(1);
    gotoxy(40, 14);
    while (1) {
        c = toupper(getch());
        if (c == 'Y') {
            r = 1;
            break;
        }
        if (c == 'N') {
            r = 0;
            break;
        }
    }
    return r;
}

void restorescreen()
{
    register int x, y;

    delay(2000);
    x = 23;
    y = 5;
    puttext(x, y, x + 34, y + 14, savebuf);
    delay(5000);
    flushkeys();
    getch();
}
