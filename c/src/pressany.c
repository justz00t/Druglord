#include <conio.h>

extern void cursor();
extern void flushkeys();
extern void cprint();
void setcolor(int fg, int bg, char blink);
extern void frame();

void pressany()
{
    char spin[3] = { 0xf9, 0x09, 0x4f };
    register int i;

    cursor(0);
    flushkeys();
    cprint("Press any key to continue", 27, 25);
    setcolor(14, 0, 0);
    do {
        for (i = 0; i < 3; i++) {
            gotoxy(25, 25);
            putch(spin[i]);
            gotoxy(53, 25);
            putch(spin[i]);
            delay(100);
        }
    } while (!kbhit());
    frame(25, 25, 54, 25);
}
