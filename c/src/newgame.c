#include <stdlib.h>
#include <conio.h>

void setcolor(int fg, int bg, char blink);
extern void cursor();
extern void frame();
extern void box();
extern void cprint();
extern void genprices();
extern void monofix();
extern int inv[8];
extern int flag8[8];
extern float bias[8];
extern char flag10[10];
extern long price[8];
extern int curcity, hold, loanflag, guntype, status, health, ngun;
extern int cashmisc, miscA, miscB, miscC, miscD, miscE;
extern long cash, bank, debt;
extern int date1, date2, date3;
extern char buf1[], buf2[], buf3[], buf4[], buf5[];

void newgame()
{
    register int i;

    cursor(0);
    frame(28, 21, 53, 25);
    setcolor(14, 0, 1);
    box("\xc9\xcd\xbb\xba\xbc\xcd\xc8\xba", 30, 22, 51, 24);
    setcolor(141, 0, 0);
    cprint("Initializing...", 34, 23);
    for (i = 0; i < 8; i++) {
        inv[i] = 0;
        bias[i] = 1.0;
        flag8[i] = 0;
    }
    for (i = 0; i < 10; i++)
        flag10[i] = 0;
    genprices(&price[0], &price[1], &price[2], &price[3], &price[4], &price[5], &price[6], &price[7]);
    curcity = rand() % 8;
    cash = 500L;
    hold = 10;
    bank = 0L;
    debt = 0L;
    loanflag = 0;
    miscA = 0;
    guntype = 0;
    status = 1;
    health = 100;
    miscB = 0;
    miscC = 0;
    miscD = 0;
    miscE = 0;
    cashmisc = 0;
    date1 = rand() % 12 + 1;
    date2 = rand() % 28 + 1;
    date3 = rand() % 6 + 86;
    monofix(buf1);
    monofix(buf2);
    monofix(buf3);
    monofix(buf4);
    monofix(buf5);
    delay(1000);
}
