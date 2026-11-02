#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>
#include <dos.h>
#include <math.h>
#include <time.h>
#include <bios.h>

void setcolor(int fg, int bg, char blink);
void fillch(char c, int n, char *buf);
int readfield(int width, char flag, char *buf);
void marketevent(int count, int rng, float mul);

extern void cprint(), cursor(), sfx(), box(), frame(), drawnames(), drawstatus(), drawmain(), drawcities(), drawdruglist(), drawhp(), pressany(), flushkeys(), genprices(), nextday(), intro(), newgame(), monofix(), endgame(), event(), restorescreen();
extern int dice(), playagain(), travelcost();

struct range { long lo, hi; };

extern char buf4328[], firebuf[], savebuf2[], savebuf[], barbuf[], buf67d0[];
extern char tbuf[7][124];
/* ---- initialised data ---- */
unsigned char scrmain[2560] = {
    0xd5, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xb8, 0x07,
    0xb3, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07,
    0xc6, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0x20, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xd1, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xb5, 0x07,
    0xb3, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0xb3, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07,
    0xc6, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0x20, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x06, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xd1, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0x20, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xb5, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07,
    0xb3, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0xb3, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07,
    0xb3, 0x07, 0x20, 0x07, 0x20, 0x07, 0x20, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07, 0x20, 0x07,
    0x20, 0x07, 0x20, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0xb3, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07,
    0xb3, 0x07, 0x20, 0x07, 0x20, 0x07, 0x20, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07, 0x20, 0x07,
    0x20, 0x07, 0x20, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0xb3, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07,
    0xb3, 0x07, 0x20, 0x07, 0x20, 0x07, 0x20, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07, 0x20, 0x07,
    0x20, 0x07, 0x20, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0xb3, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07,
    0xb3, 0x07, 0x20, 0x07, 0x20, 0x07, 0x20, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07, 0x20, 0x07,
    0x20, 0x07, 0x20, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0xb3, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07,
    0xb3, 0x07, 0x20, 0x07, 0x20, 0x07, 0x20, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07, 0x20, 0x07,
    0x20, 0x07, 0x20, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0xb3, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07,
    0xb3, 0x07, 0x20, 0x07, 0x20, 0x07, 0x20, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07, 0x20, 0x07,
    0x20, 0x07, 0x20, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0xb3, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07,
    0xb3, 0x07, 0x20, 0x07, 0x20, 0x07, 0x20, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07, 0x20, 0x07,
    0x20, 0x07, 0x20, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0xb3, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07,
    0xb3, 0x07, 0x20, 0x07, 0x20, 0x07, 0x20, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07, 0x20, 0x07,
    0x20, 0x07, 0x20, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0xb3, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07,
    0xb3, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0xb3, 0x07, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f,
    0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0x20, 0x0f, 0xb3, 0x07,
    0xd4, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcf, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcf, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07,
    0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xcd, 0x07, 0xbe, 0x07
};
char unusedbyte = 0;
char *drugname[8] = {
    "Cocaine",
    "Crack",
    "Heroin",
    "Acid",
    "Crystal",
    "Grass",
    "Speed",
    "Ludes"
};
char *gunname[6] = {
    "None",
    "Sat. Nt. Spcl",
    "Ruger",
    "Auto Mag",
    "Uzi",
    "AK-47"
};
char *gunlong[5] = {
    "Saturday Night Special",
    "Ruger Revolver",
    "Auto Mag",
    "Uzi",
    "AK-47 Assault Rifle"
};
char *txt[14] = {
    "* * *",
    "D R U G   L O R D",
    "Date:",
    "Drugs on Hand",
    "Street Prices",
    "Location:",
    "Hold:",
    "Cash:",
    "In Bank:",
    "In Debt:",
    "# of Guns:",
    "Gun Type:",
    "Status Pts:",
    "Health:"
};
char *cityname[8] = {
    "Chicago",
    "Detroit",
    "Las Vegas",
    "Los Angeles",
    "Miami",
    "New York",
    "San Diego",
    "Washington D.C."
};
char cityxy[8][2] = {
    {3, 2},
    {3, 1},
    {1, 2},
    {0, 3},
    {5, 4},
    {5, 2},
    {0, 3},
    {5, 2}
};
float citymul[8][8] = {
    {0.95f, 0.9f, 0.9f, 1.0f, 1.05f, 0.95f, 1.1f, 1.05f},
    {1.1f, 0.9f, 0.95f, 0.95f, 1.1f, 1.1f, 1.0f, 0.9f},
    {1.0f, 1.1f, 0.9f, 1.05f, 1.05f, 0.9f, 0.9f, 1.1f},
    {1.1f, 1.05f, 0.95f, 0.9f, 1.05f, 1.05f, 0.9f, 1.0f},
    {0.9f, 0.95f, 1.1f, 1.05f, 0.9f, 1.1f, 1.05f, 0.95f},
    {0.9f, 0.95f, 1.1f, 1.1f, 0.95f, 1.0f, 1.0f, 1.0f},
    {1.05f, 0.95f, 1.0f, 1.05f, 0.9f, 1.0f, 0.95f, 1.1f},
    {0.95f, 1.1f, 1.05f, 1.05f, 0.95f, 0.9f, 1.1f, 0.9f}
};
int gunpow[6] = { 0, 1, 2, 4, 2, 4 };
struct range prange[8] = {
    {40000L, 50000L},
    {25000L, 30000L},
    {10000L, 15000L},
    {2000L, 3000L},
    {600L, 1000L},
    {200L, 400L},
    {70L, 100L},
    {10L, 20L}
};
int dim[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
/* ---- uninitialised data ---- */
struct GameState {
    int marketNews;
    int beatings;
    float priceBias[8];
    unsigned avgCost[8];
    int escPressed;
    int dead;
    int holdFree;
    long cash;
    int day;
    int monochrome;
    int month;
    int year;
    int health;
    long bank;
    int gunType;
    int holdSize;
    int status;
    long debt;
    int city;
    int hasLoan;
    char lineBuf[80];
    int inventory[8];
    int loanDays;
    int goodDeals;
    int picked[8];
    char eventDone[10];
    int copChase;
    int delayTime;
    int sharkAngry;
    int totalDeals;
    struct text_info textInfo;
    int totalDays;
    long price[8];
    int numGuns;
};
struct GameState g;

void setcolor(fg, bg, blink)
int fg;
int bg;
char blink;
{
    if (g.monochrome == 1) {
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

void flushkeys()
{
    while (kbhit())
        getch();
}

void cursor(on)
int on;
{
    if (on == 0)
        _CX = 0x2807;
    else if (g.textInfo.currmode == 7)
        _CX = 0x90a;
    else
        _CX = 0x607;
    _AH = 1;
    geninterrupt(0x10);
}

void sfx(id)
int id;
{
    register int i, j;

    switch (id) {
    case 2:
        sound(2000); delay(10);
        sound(5000); delay(20);
        sound(500);  delay(30);
        sound(1000); delay(40);
        break;
    case 3:
        for (i = 500; i > 200; i -= 100) {
            sound(i);       delay(100);
            sound(i - 200); delay(100);
            nosound();      delay(50);
        }
        break;
    case 4:
        for (i = 500; i < 1000; i += 100) {
            sound(i);        delay(10);
            sound(i + 2000); delay(20);
            nosound();       delay(10);
        }
        break;
    case 5:
        for (j = 0; j < 6; j++) {
            for (i = 7; i < 15; i++) {
                sound(i * 100); delay(25);
            }
            for (i = 15; i > 7; i--) {
                sound(i * 100); delay(25);
            }
        }
        break;
    case 6:
        for (i = 0; i < 5; i++)
            for (j = 0; j < 10; j++) {
                sound(i * 1000 + j * 1000); delay(40);
            }
        break;
    case 7:
        for (i = 100; i > 0; i--) {
            sound(5000 - i * 50); delay(5);
            sound(i * 50 + 5000); delay(5);
        }
        break;
    case 8:
        sound(2000); delay(30);
        sound(500);  delay(25);
        sound(70);   delay(40);
        break;
    case 11:
        sound(1000); delay(50);
        break;
    case 12:
        sound(500); delay(30);
        sound(200); delay(20);
        break;
    case 13:
        sound(100); delay(30);
        sound(50);  delay(30);
        break;
    case 14:
        for (i = 1; i < 2; i++) {
            sound(150); delay(30);
            sound(100); delay(50);
        }
        break;
    case 15:
        for (i = 1; i < 3; i++) {
            sound(100); delay(20);
            sound(50);  delay(60);
        }
        break;
    case 20:
        sound(800); delay(100);
        break;
    }
    nosound();
}

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
    fillch(chars[1], x2 - x1 - 1, g.lineBuf);
    cputs(g.lineBuf);
    gotoxy(x1 + 1, y2);
    fillch(chars[5], x2 - x1 - 1, g.lineBuf);
    cputs(g.lineBuf);
    for (y = y1 + 1; y < y2; y++) {
        gotoxy(x1, y); putch(chars[3]);
        gotoxy(x2, y); putch(chars[7]);
    }
}

void fillch(c, n, buf)
char c;
int n;
char *buf;
{
    int i;

    for (i = 0; i < n; i++)
        buf[i] = c;
    buf[n] = 0;
}

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
    fillch(' ', width, g.lineBuf);
    cputs(g.lineBuf);
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

void frame(x1, y1, x2, y2)
int x1, y1, x2, y2;
{
    gettextinfo(&g.textInfo);
    window(x1, y1, x2, y2);
    clrscr();
    textmode(g.textInfo.currmode);
}

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

void drawstatus()
{
    register int i;

    g.holdFree = 0;
    cursor(0);
    drawnames();
    for (i = 0; i < 8; i++) {
        gotoxy(16, i + 7);
        if (g.inventory[i] > 0) setcolor(14, 0, 1);
        else setcolor(7, 0, 0);
        cprintf("%6d", g.inventory[i]);
        gotoxy(38, i + 7);
        setcolor(12, 0, 1);
        cprintf("%10ld", g.price[i]);
        g.holdFree += g.inventory[i];
    }
    setcolor(14, 0, 1);
    g.holdFree = g.holdSize - g.holdFree;
    gotoxy(63, 4);
    cprintf("%-15s", cityname[g.city]);
    gotoxy(63, 5);
    cprintf("%-6d (%d max)", g.holdFree, g.holdSize);
    gotoxy(63, 7);
    cprintf("$%-10ld", g.cash);
    gotoxy(63, 8);
    cprintf("$%-10ld", g.bank);
    gotoxy(63, 9);
    cprintf("$%-10ld", g.debt);
    gotoxy(74, 9);
    if (g.hasLoan == 1) {
        cputs("(   )");
        if (g.loanDays <= 2) setcolor(140, 0, 1);
        gotoxy(75, 9);
        cprintf("%3d", g.loanDays);
        setcolor(14, 0, 1);
    } else
        cputs("     ");
    gotoxy(63, 11);
    cprintf("%-6d", g.numGuns);
    gotoxy(63, 12);
    cprintf("%-13s", gunname[g.gunType]);
    gotoxy(63, 14);
    cprintf("%-6d", g.status);
    gotoxy(63, 15);
    if (g.health <= 25) setcolor(132, 0, 1);
    cprintf("%-4d", g.health);
    setcolor(14, 0, 1);
    gotoxy(12, 3);
    cprintf("%#02d/%#02d/%#02d", g.month, g.day, g.year);
}

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

int travelcost(a, b)
int a, b;
{
    return (int)(sqrt(pow((double)(cityxy[b][0] - cityxy[a][0]), 2.0) +
                       pow((double)(cityxy[b][1] - cityxy[a][1]), 2.0)) * 60.0 + 25.0);
}

void drawcities()
{
    int j;
    register int i, d;

    cursor(0);
    frame(1, 17, 80, 25);
    cprint("\x91\005Drug Cities:", 34, 17);
    for (d = 0; d < 4; d++) {
        for (j = 0; j <= 1; j++) {
            i = j * 4 + d;
            if (i > 8) break;
            gotoxy(j * 35 + 10, d + 19);
            setcolor(12, 0, 1);
            cprintf("%2d. ", i + 1);
            setcolor(11, 0, 0);
            cprintf("%-20s", cityname[i]);
            if (i == g.city) continue;
            cputs("(");
            setcolor(2, 0, 0);
            cprintf("$%d", travelcost(g.city, i));
            cprint("\x91\x0b)", 11, 0);
        }
    }
}

void genprices(long *p0, long *p1, long *p2, long *p3, long *p4, long *p5, long *p6, long *p7)
{
    *p0 = (long)(((double)(rand() % (prange[0].hi - prange[0].lo) + prange[0].lo) + g.priceBias[0]) * citymul[g.city][0]);
    *p1 = (long)(((double)(rand() % (prange[1].hi - prange[1].lo) + prange[1].lo) + g.priceBias[1]) * citymul[g.city][1]);
    *p2 = (long)(((double)(rand() % (prange[2].hi - prange[2].lo) + prange[2].lo) + g.priceBias[2]) * citymul[g.city][2]);
    *p3 = (long)(((double)(rand() % (prange[3].hi - prange[3].lo) + prange[3].lo) + g.priceBias[3]) * citymul[g.city][3]);
    *p4 = (long)(((double)(rand() % (prange[4].hi - prange[4].lo) + prange[4].lo) + g.priceBias[4]) * citymul[g.city][4]);
    *p5 = (long)(((double)(rand() % (prange[5].hi - prange[5].lo) + prange[5].lo) + g.priceBias[5]) * citymul[g.city][5]);
    *p6 = (long)(((double)(rand() % (prange[6].hi - prange[6].lo) + prange[6].lo) + g.priceBias[6]) * citymul[g.city][6]);
    *p7 = (long)(((double)(rand() % (prange[7].hi - prange[7].lo) + prange[7].lo) + g.priceBias[7]) * citymul[g.city][7]);
}

int dice(sides, count)
int sides, count;
{
    register int i, sum;

    sum = 0;
    for (i = 0; i < count; i++)
        sum += rand() % sides + 1;
    return sum;
}

void drawdruglist()
{
    register int i;

    cursor(0);
    cprint("\221\r* * *  \221\031D R U G S  \221\r* * *", 45, 18);
    fillch(0xc4, 40, g.lineBuf);
    setcolor(14, 0, 0);
    cprint(g.lineBuf, 36, 19);
    for (i = 0; i < 4; i++) {
        gotoxy(43, i + 21);
        setcolor(12, 0, 1);
        cprintf("%1d. ", i + 1);
        setcolor(10, 0, 0);
        cprintf("%-15s", drugname[i]);
        gotoxy(60, i + 21);
        setcolor(12, 0, 1);
        cprintf("%1d. ", i + 5);
        setcolor(10, 0, 0);
        cprintf("%-15s", drugname[i + 4]);
    }
    cursor(1);
}

void drawhp(hp, row, side)
int hp, row, side;
{
    register char *s;

    if (side == 0)
        gotoxy(12, row + 10);
    else
        gotoxy(52, row + 11);
    if (hp > 0) {
        s = "Alive";
        setcolor(15, 0, 1);
    } else {
        s = "Dead";
        setcolor(7, 0, 0);
    }
    cprintf("%4d            %-12s", hp, s);
}

void marketevent(int count, int rng, float mul)
{
    int n;
    char *sep;
    register int k, d;

    n = rand() % count + 1;
    for (k = 0; k < 8; k++)
        g.picked[k] = 0;
    sep = "";
    "";
    for (k = 0; k < n; k++) {
        do {
            d = rand() % 8;
        } while (g.picked[d] == 1);
        cprintf("%s%s", sep, drugname[d]);
        if (n == 2)
            sep = " and ";
        else
            sep = ", ";
        g.picked[d] = 1;
        g.price[d] = g.price[d] * ((double)(rand() % rng / 100) + mul);
        if (n > 2 && k + 2 == n)
            sep = ", and ";
    }
    g.marketNews = 1;
}

void pressany()
{
    char spin[3] = { 0xf9, 0x09, 0x4f };
    register int i;

    cursor(0);
    flushkeys();
    cprint("\221\032Press any key to continue", 27, 25);
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

void event(kind)
int kind;
{
    int choice;
    int nfr;
    int hp[11];
    int chp[11];
    int ctype;
    int tgt;
    int copsleft;
    int alive;
    int ftgt;
    int loot;
    int state;
    int maxshots;
    int shots;
    char unused1;
    char key;
    int v20;
    int v1e;
    int v1c;
    int v1a;
    long bm;
    float f;
    char *lbl[2] = { "Health", "Status" };
    char buf[12];
    register int si, di;

    cursor(0);
    if (g.eventDone[kind] == 1) return;
    frame(1, 17, 80, 25);
    switch (kind) {
    case 0:
        g.copChase = 0;
        state = 0;
        if (g.gunType < 4) maxshots = 1;
        else maxshots = 3;
        cprint("\221\004The \221\011cops \221\004are on your tail!!!", 27, 17);
        sfx(5);
        cprint("\221\012Do you wanna   \221\0341\221\012) Go at it alone", 15, 19);
        cprint("\221\0342\221\012) Call a friend (1 status pt.)", 30, 20);
        cprint("\221\0343\221\012) Call a posse (5 status pts.)", 30, 21);
        choice = 0;
        while (1) {
            do {
                frame(1, 23, 80, 25);
                cprint("\221\016Call it \020 ", 20, 23);
                cursor(1);
                g.escPressed = readfield(1, 0, g.lineBuf);
                cursor(0);
                choice = atol(g.lineBuf);
            } while (choice < 1 || choice > 3);
            if ((choice == 2 && g.status == 0) || (choice == 3 && g.status < 5)) {
                cprint("\221\014You don't have enough points!", 25, 25);
                sfx(3);
                delay(2000);
                frame(25, 25, 80, 25);
            } else
                break;
        }
        switch (choice) {
        case 1:
            nfr = 0;
            break;
        case 2:
            nfr = 1;
            g.status--;
            break;
        case 3:
            nfr = rand() % 4 + g.status / 100 + 4;
            if (nfr > 8) nfr = 8;
            g.status -= 5;
            break;
        }
        frame(1, 1, 80, 25);
        puttext(1, 1, 80, 22, buf4328);
        setcolor(14, 0, 0);
        cprint(txt[0], 24, 2);
        cprint(txt[0], 52, 2);
        setcolor(12, 0, 1);
        cprint(txt[1], 32, 2);
        setcolor(10, 0, 1);
        cprint(lbl[0], 9, 8);
        cprint(lbl[0], 49, 8);
        cprint(lbl[1], 28, 8);
        cprint(lbl[1], 68, 8);
        setcolor(14, 0, 1);
        cprint(txt[11], 10, 6);
        cprint(txt[11], 50, 6);
        cprint(txt[10], 9, 5);
        setcolor(12, 0, 0);
        gotoxy(20, 5);
        cprintf("%-10d", g.numGuns);
        setcolor(6, 0, 0);
        gotoxy(20, 6);
        cprintf("%-15s", gunname[g.gunType]);
        hp[0] = g.health;
        setcolor(6, 0, 0);
        for (si = 1; si <= nfr; si++) {
            hp[si] = rand() % 10 + dice(10, g.status / 50) + 25;
            gotoxy(1, si + 10);
            if (choice == 2) cprintf("%10s:", "Friend");
            if (choice == 3) cprintf("%6s #%2d:", "Posse", si);
        }
again:
        di = rand() % 8 + 3;
        ctype = g.status / (di * 2);
        if (rand() % 20 == 0) {
            if (ctype < 2) {
                di = rand() % 4 + 1;
                ctype = g.status / di;
                if (ctype > 3) ctype = 3;
                if (ctype < 2) ctype = 2;
            } else {
                di = rand() % 3 + 8;
                ctype = 3;
            }
        } else {
            if (ctype < 2) goto again;
            if (ctype > 3) goto again;
        }
        gotoxy(60, 6);
        setcolor(9, 0, 0);
        cprintf("%-15s", gunname[ctype]);
        setcolor(9, 0, 0);
        for (si = 0; si < di; si++) {
            gotoxy(42, si + 11);
            cprintf("%6s #%2d", "Cop", si + 1);
            chp[si] = dice(15, 2) + 15;
        }
        copsleft = di;
        alive = nfr + 1;
        for (si = 0; si <= nfr; si++)
            drawhp(hp[si], si, 0);
        for (si = 0; si < di; si++)
            drawhp(chp[si], si, 1);
        do {
            frame(1, 24, 80, 25);
            cprint("\221\017Do ya wanna (\221\034F\221\017)ight or (\221\034R\221\017)un? ", 5, 24);
            cursor(1);
            do {
                key = toupper(getch());
            } while (key != 'F' && key != 'R');
            cursor(0);
            putch(key);
            while (1) {
                switch (key) {
                case 'F':
                    if (state != 2) {
                        for (si = 0; si <= nfr; si++) {
                            if (hp[si] == 0) continue;
                            if (si < g.numGuns) cprint("\221\014Firing", 28, si + 10);
                            else cprint("\221\014Punching", 27, si + 10);
                            shots = 0;
                            do {
                                do {
                                    tgt = rand() % di;
                                } while (chp[tgt] == 0);
                                sfx(g.gunType + 10);
                                if (rand() % 2 == 0) {
                                    cprint("\221\024*  Hit!  *", 67, tgt + 11);
                                    sfx(8);
                                    delay(100);
                                    if (si < g.numGuns) chp[tgt] -= dice(6, gunpow[g.gunType]);
                                    else chp[tgt] -= dice(4, 1);
                                    if (chp[tgt] <= 0) {
                                        chp[tgt] = 0;
                                        copsleft--;
                                    }
                                } else
                                    cprint("\221\016- Missed! -", 67, tgt + 11);
                                delay(200);
                                drawhp(chp[tgt], tgt, 1);
                                shots++;
                                if (hp[0] == 0 || copsleft == 0) break;
                            } while (g.gunType > 3 && shots < maxshots && copsleft > 0 && si < g.numGuns);
                            drawhp(hp[si], si, 0);
                            if (copsleft == 0) break;
                        }
                    }
                    if (copsleft == 0) break;
                    state = 0;
                    for (si = 0; si < di; si++) {
                        if (chp[si] == 0) continue;
                        cprint("\221\014Firing", 68, si + 11);
                        shots = 0;
                        do {
                            ftgt = rand() % (nfr + 1);
                        } while (hp[ftgt] == 0);
                        sfx(ctype + 10);
                        if (rand() % 2 == 0) {
                            cprint("\221\024*  Hit!  *", 26, ftgt + 10);
                            sfx(8);
                            delay(100);
                            hp[ftgt] -= dice(6, gunpow[ctype]);
                            if (hp[ftgt] <= 0) {
                                g.status--;
                                hp[ftgt] = 0;
                                alive--;
                            }
                        } else
                            cprint("\221\016- Missed! -", 26, ftgt + 10);
                        delay(200);
                        drawhp(hp[ftgt], ftgt, 0);
                        shots++;
                        drawhp(chp[si], si, 1);
                        if (hp[0] == 0 || alive == 0) break;
                    }
                    break;
                case 'R':
                    cprint("\221\013You take off running... ", 20, 25);
                    delay(3000);
                    if (rand() % 10 + di / 3 < rand() % 10 + nfr / 3) {
                        cprint("\221\036and ya got away!!!", 99, 99);
                        g.health = hp[0];
                        state = 1;
                    } else {
                        cprint("\221\013and ya didn't get away!!!", 99, 99);
                        state = 2;
                        key = 'F';
                        continue;
                    }
                    break;
                }
                break;
            }
        } while (copsleft > 0 && alive > 0 && hp[0] > 0 && state != 1);
        if (state == 1) {
            delay(3000);
            drawmain();
            break;
        }
        g.health = hp[0];
        if (g.health == 0) {
            g.dead = 1;
            break;
        }
        frame(1, 23, 80, 25);
        cprint("\221\012Yous wasted 'em!!! Way ta go, man!!!", 23, 23);
        delay(1000);
        v20 = 1;
        if (ctype < g.gunType && di <= g.numGuns) goto skipask;
        setcolor(7, 0, 0);
        gotoxy(21, 24);
        if (di == 1) strcpy(buf, "his gun");
        else strcpy(buf, "their guns");
        cprintf("Do you wanna take %s, man? (Y/N) ", buf);
        cursor(1);
        do {
            key = toupper(getch());
        } while (key != 'Y' && key != 'N');
        cursor(0);
        if (key == 'Y') {
            cprint("\221\016You \221\014want\221\016 'em, you \221\014got \221\016'em!  What a bunch'a \221\006chumps!", 15, 25);
            v20 = 0;
            if (g.gunType == ctype) g.numGuns += di;
            else {
                g.gunType = ctype;
                g.numGuns = di;
            }
        } else
            cprint("\221\011OK, man, it's yer call.", 29, 25);
        delay(2000);
skipask:
        if (v20 == 1) {
            cprint("\221\013You sell the ", 5, 24);
            loot = di * ctype * 200;
            cprintf("%d guns that were on the pigs you wasted for %d bucks!", di, loot);
            cprint("\221\013This'll teach 'em not to mess with ya, huh?", 20, 25);
            g.cash += loot;
            g.status += di;
            delay(4000);
        }
        if (g.dead == 0) drawmain();
        break;
    case 1:
        if (g.marketNews == 1) break;
        cprint("\221\013\"Hey man, check dis out!  Word has it on de street dat deres des cops, see?\r\nThey musta decided that workin' for 25 G's a year ain't enough, so de've\r\ndecided ta sell some of de take drugs confiscated on de side! De market is\r\nflooded with ", 1, 18);
        setcolor(6, 0, 1);
        marketevent(3, 30, 0.7f);
        setcolor(11, 0, 0);
        cprintf("!\"");
        sfx(6);
        pressany();
        break;
    case 2:
        cprint("\221\007\"Hey, I heard it through de grapevine dat deres some cheap homemade stuff\r\navailable from some of the homeboys downtown.  Real primo stuff, dey say!\r\nAll I hear on de streets is 'Cheap ", 1, 18);
        setcolor(6, 0, 1);
        marketevent(3, 40, 0.6f);
        setcolor(7, 0, 0);
        cprintf("!'\"");
        pressany();
        break;
    case 3:
        window(15, 17, 80, 25);
        cprint("\221\007\332\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\277\r\n\263  * * *   \221\037E X T R A ! !   E X T R A ! !\221\007   * * *  \263\r\n\263               \221\037Read all about it!!\221\007               \263\r\n\303\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\264\r\n\263  After six months of undercover investigation,  \263\r\n\263  police close in and make a major drug bust!    \263\r\n\263  This will no doubt affect street drug prices.  \263\r\n\300\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\331", 15, 17);
        window(1, 1, 80, 25);
        for (si = 0; si < 8; si++)
            g.price[si] = g.price[si] * ((double)(rand() % 25) / 100.0 + 1.05);
        sfx(7);
        pressany();
        break;
    case 4:
        cprint("\221\r\"Dude, I got some news for ya... a factory of one of de cities biggest suppliersof ", 1, 18);
        setcolor(6, 0, 1);
        marketevent(1, 40, 1.2f);
        setcolor(13, 0, 0);
        if (rand() % 2 == 1) cputs(" burned down");
        else cputs(" blew up");
        cprint("last night.  I don't have to tell ya what this'll on de\r\nstreet...\"", 99, 99);
        pressany();
        break;
    case 5:
        if (g.inventory[5] == 0) break;
        cprint("\221\006Ya come back to yer place tonight and find dat yer sexy main squeeze tought\r\ndat it wouldst be a good idea to use some of yer weed to make brownies!!!\r\nAll tings considered, yous take it pretty well.  After all, if ya were mad,\r\nyou would've \221\007chopped \221\006up her body \221\014before \221\006dumping it in de river!", 1, 17);
        g.inventory[5] = 0;
        pressany();
        break;
    case 6:
        cprint("\221\014Yer walking along de street at night, when from outta nowhere jumps dis goon!\r\nHe waves a 9mm Beretta at yous and demands yer fork over all yer money.", 1, 17);
        while (1) {
            if (g.status >= 10 && rand() % 20 != 0) {
                cprint("\221\016Do ya wanna use yer status as a major drug dealer to get outta this? (Y/N) ", 1, 20);
                cursor(1);
                do {
                    key = toupper(getch());
                } while (key != 'Y' && key != 'N');
                cursor(0);
                if (key == 'Y') {
                    cprint("\221\014Yous tell him whos he's dealin' wit.  He looks nervous, asks fer yer pardon,den runs off down an alley!", 1, 22);
                    g.status -= 10;
                    break;
                }
            }
            if (g.cash < 50) {
                cprint("\221\013\"This is all you got? &%&*! this, man!\"", 20, 21);
                cprint("He looks at ya like a crazed animal, and pulls de trigger!", 10, 22);
                cprint("\221\016* * *   \221\034B L A M   \221\016* * *", 28, 23);
                sfx(13);
                g.health -= rand() % 6 + rand() % 6 + 2;
            } else
                cprint("\221\rHe laughs merrily as he takes yer cash and runs off down de street!", 5, 22);
            g.cash = 0;
            break;
        }
        pressany();
        break;
    case 7:
        if (g.holdSize >= 300) break;
        do {
            v20 = rand() % 10 + 1;
            if (rand() % 5 == 0) {
                v20 += rand() % 10 + 1;
                if (rand() % 5 == 0)
                    v20 += rand() % 10 + 1;
            }
            v20 *= 10;
        } while (g.holdSize + 50 < v20 || v20 <= g.holdSize);
        bm = pow((double)v20, 3.34) * 0.0634;
        bm = (long)((double)((rand() % 200 + 1) * bm) / 10000.0) * 100;
        cprint("\221\nYou are approached by a shady-lookin guy in a long dark trenchcoat.  He says:", 1, 17);
        cprint("\221\011\"Hey, bud, I got a \221\036special \221\011trenchcoat for ya, just your style!", 10, 19);
        cprint("It's got a capacity of ", 11, 20);
        setcolor(12, 0, 1);
        cprintf("%d", v20);
        cprint("\221\011 and I won't take under ", 99, 99);
        setcolor(12, 0, 1);
        cprintf("$%ld", bm);
        cprint("\221\011 for it.\"", 99, 99);
        cprint("\221\016Is it a deal? (Y/N) \020 ", 1, 23);
        cursor(1);
        do {
            key = toupper(getch());
        } while (key != 'Y' && key != 'N');
        cursor(0);
        if (key == 'Y') {
            if (g.cash < bm)
                cprint("\221\011\"Sorry Charly, I don't take credit! Maybe when you're a little more solvent!\"", 1, 24);
            else {
                cprint("\221\011\"You bought yourself one hell of a trenchcoat!\"", 16, 24);
                g.cash -= bm;
                g.holdSize = v20;
            }
        } else
            cprint("\221\011\"OK, it's your loss, man.  What a shame!\"", 20, 24);
        pressany();
        break;
    case 8:
        v20 = rand() % 5;
        if (rand() % 10 == 0) v1e = rand() % 6 + 5;
        else v1e = rand() % 5 + 2;
        v1c = v20 * 200 + 200;
        v1a = v1e * v1c;
        cprint("\221\rJuan, one of the most powerful underground gun smugglers in the world,\r\ncomes to you with an offer:", 1, 17);
        cprint("\221\007\"I have a shipment of ", 1, 20);
        setcolor(12, 0, 1);
        cprintf("%s's", gunlong[v20]);
        cprint("\221\007 coming in tonight.  I'm", 99, 99);
        cprint("distributing them in shipments of ", 1, 21);
        setcolor(12, 0, 1);
        cprintf("%d ", v1e);
        cprint("\221\007at ", 99, 99);
        setcolor(12, 0, 1);
        cprintf("$%d ", v1c);
        cprint("\221\007each, for a total of ", 99, 99);
        setcolor(12, 0, 1);
        cprintf("$%d", v1a);
        setcolor(7, 0, 0);
        cputs(".\"");
        cprint("\221\rAre you interested in an order? (Y/N) \020 ", 20, 23);
        cursor(1);
        do {
            key = toupper(getch());
        } while (key != 'Y' && key != 'N');
        cursor(0);
        if (key == 'Y') {
            if (v1a > g.cash)
                cprint("\221\007\"I'm sorry, sir, but I only accept cash \221\017up-front\221\007.  Business is business.\"", 5, 24);
            else {
                cprint("\221\007\"Very well, they will be delivered by tomorrow.  Good doing business with you.\"", 1, 24);
                g.numGuns = v1e;
                g.gunType = v20 + 1;
                g.cash -= v1a;
            }
        } else
            cprint("\221\007\"As you wish.  Perhaps next time we may do business.\"", 15, 24);
        pressany();
        break;
    case 9:
        if (g.cash < 10000L) break;
        f = rand() % 30 / 100.0 + 0.7;
        bm = g.cash * f / 1000.0;
        bm = bm * 1000;
        cprint("\221\nYous get a note in de mail from a business associate of yers.  He says dat\r\nif yous don't deposit ", 1, 17);
        setcolor(12, 0, 1);
        cprintf("$%ld ", bm);
        cprint("\221\nin his Swiss bank account, he's gonna \221\026rat \221\nto de\r\ncops everything dat he knows about you.", 99, 99);
        cprint("\221\rDo you pay the blackmail? (Y/N) \020 ", 1, 21);
        cursor(1);
        do {
            key = toupper(getch());
        } while (key != 'Y' && key != 'N');
        cursor(0);
        if (key == 'Y') {
            cprint("\221\rYou wire de money into his account.  That really \221\034^$&?*@ \221\rsucks!!!", 1, 24);
            g.cash -= bm;
        } else {
            cprint("\221\014Yeah, man, nobody pushes you around! I respect yer integrity!!!", 9, 24);
            if (rand() % 5 != 0) {
                g.copChase = 1;
                g.eventDone[0] = 0;
            }
        }
        pressany();
        break;
    }
    g.eventDone[kind] = 1;
    if (g.dead == 0) drawstatus();
}

char rep[18][50] = {
    "           \221\034CONGRATULATIONS!!!         ",
    "                                      ",
    "\221\016Yous has become a \221\035DRUGLORD!!!\221\016  I'm    ",
    "proud ta know you, man!  Anyway, check",
    "dis out --                            ",
    "                                      ",
    "\221\013           Cash in Hand:\221\036              ",
    "\221\013           Cash in Bank:\221\036              ",
    "\221\013          Still in Debt:\221\036              ",
    "\221\013   Number of Good Deals:\221\036              ",
    "\221\013  Number of Total Deals:\221\036              ",
    "\221\013                 DEAL %:\221\036              ",
    "\221\013                        \221\036              ",
    "\221\013             Total Days:\221\036              ",
    "\221\013                        \221\036              ",
    "\221\017------------------------     -------- ",
    "\221\014          OVERALL SCORE:\221\036              ",
    "\221\007                   RANK:\221\036              "
};

int playagain()
{
    char c;
    register int r;

    setcolor(15, 0, 0);
    box("\xc9\xcd\xbb\xba\xbc\xcd\xc8\xba", 29, 10, 51, 15);
    frame(30, 11, 50, 14);
    cprint("\x91\014Would ya like ta", 32, 12);
    cprint("play again? (Y/N)", 32, 13);
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

void endgame()
{
    int i, j, k, x, row, n, d, m, pct, rank;
    char *titles[12] = { "Loser!", "Dopehead", "Peddler", "Pusher", "Gangster", "Dealer",
                         "Trafficker", "Racketeer", "Marketeer", "Entrepreneur", "DRUGLORD", "OVERLORD" };
    long thr[11] = { 250000L, 500000L, 600000L, 700000L, 800000L, 850000L,
                     900000L, 950000L, 1000000L, 1100000L, 1250000L };
    char msg[11] = "DRUGLORD!!!";
    int notes[16] = { 100, 150, 500, 400, 100, 75, 25, 500, 1000, 1500, 1000, 750, 500, 200, 100, 50 };
    char bigbuf[1121];
    char key;
    char s[10];
    long score;
    long p2;
    int guns;
    double ex;
    register int si, di;

    delay(1000);
    frame(1, 17, 80, 25);
    for (i = 1; i < 6; i++) {
        frame(15, 20, 65, 20);
        setcolor(i + 9, 0, i % 2 == 1);
        for (j = 0; j < 22; j++) {
            gotoxy(j / 2 * 5 + 15, 20);
            putch(msg[j / 2]);
            if (j < 16) {
                sound(notes[j]);
                delay(20);
            }
        }
        nosound();
        delay(500);
    }
    puttext(1, 1, 80, 25, savebuf2);
    if (g.numGuns < 10) guns = g.numGuns;
    else guns = 10;
    for (i = 1; i <= guns; i++) {
        if (i <= 4) {
            k = (i - 1) / 2 * 11 + (i % 2 == 0) * 5 + 55;
            si = (i % 2 == 0) * 3 + 2;
            puttext(k, si, k + 7, si + 2, firebuf);
        } else {
            k = (i - 5) / 2 * 11 + !(i % 2) * 5 + 1;
            si = (i % 2 == 0) * 3 + 2;
            puttext(k, si, k + 7, si + 2, firebuf);
        }
    }
    n = (int)((g.cash + g.bank - 1) / 2000000L) + 1;
    for (si = 0; si < 4; si++) {
        row = si + 11;
        for (k = 0; k < 5; k++) {
            x = k * 7 + 18 - si * 2;
            d = 15 - si * 3 - k;
            if (d < 0) d = 0;
            for (m = 0; m < n; m++) {
                if ((long)(si * 5 + k + 1) * 100000L + (long)m * 2000000L > g.cash) break;
                puttext(x, row - m, x + 4, row - m, barbuf);
                sound(m * 500 + 1000);
                delay(d + 5);
                sound(m * 500 + 3000);
                delay(d);
            }
        }
    }
    nosound();
    pct = g.goodDeals * 100 / g.totalDeals;
    ltoa(g.cash, s, 10);
    di = strlen(s);
    stpcpy(rep[6] + (0x29 - di), s);
    ltoa(g.bank, s, 10);
    di = strlen(s);
    stpcpy(rep[7] + (0x29 - di), s);
    ltoa(g.debt, s, 10);
    di = strlen(s);
    stpcpy(rep[8] + (0x29 - di), s);
    itoa(g.goodDeals, s, 10);
    di = strlen(s);
    stpcpy(rep[9] + (0x29 - di), s);
    itoa(g.totalDeals, s, 10);
    di = strlen(s);
    stpcpy(rep[10] + (0x29 - di), s);
    itoa(pct, s, 10);
    di = strlen(s);
    stpcpy(rep[11] + (0x28 - di), s);
    strcat(rep[11], "%");
    di = strlen(gunname[g.gunType]);
    stpcpy(rep[12] + (0x17 - di), gunname[g.gunType]);
    stpcpy(rep[12] + 0x17, "'s:\221\036            ");
    itoa(g.numGuns, s, 10);
    di = strlen(s);
    stpcpy(rep[12] + (0x29 - di), s);
    itoa(g.totalDays, s, 10);
    di = strlen(s);
    stpcpy(rep[13] + (0x29 - di), s);
    ex = exp((double)(g.cash + g.bank - g.debt) * 6.9315e-08);
    score = (long)ex * 75000L;
    if (score > 700000L) score = 700000L;
    p2 = (long)(pow((double)g.totalDays, -2.12) * 104000000000.0);
    if (p2 > 500000L) p2 = 500000L;
    score += p2;
    score += (long)(pow((double)pct, 8.14) * 2.48e-11);
    score += (long)(g.gunType * g.numGuns) * 1500L;
    ltoa(score, s, 10);
    di = strlen(s);
    stpcpy(rep[16] + (0x29 - di), s);
    for (rank = 0; rank < 10; rank++) {
        if (thr[rank] > score) break;
    }
    di = strlen(titles[rank]);
    stpcpy(rep[17] + (0x29 - di), titles[rank]);
    setcolor(9, 0, 0);
    box("\311\315\273\272\274\315\310\272", 10, 18, 55, 25);
    cprint(" \221\034\031 ", 50, 25);
    cprint("\221\011[\221\033ESC\221\011] \221\033Finished ", 12, 25);
    setcolor(14, 0, 0);
    for (si = 0; si < 5; si++)
        cprint(rep[si], 15, si + 19);
    si = 0;
    while (1) {
        key = getch();
        setcolor(14, 0, 0);
        if (key == 0) {
            key = getch();
            if (key == 0x48 && si > 0) {
                si--;
                gettext(15, 19, 55, 23, bigbuf);
                puttext(15, 20, 55, 24, bigbuf);
                cprint(rep[si], 15, 19);
                sfx(20);
            }
            if (key == 0x50 && si < 12) {
                si++;
                gettext(15, 20, 55, 24, bigbuf);
                puttext(15, 19, 55, 23, bigbuf);
                cprint(rep[si + 5], 15, 24);
                sfx(20);
            }
            if (si > 0) cprint(" \221\014\030 ", 50, 18);
            else cprint("\221\011\315\315\315", 50, 18);
            if (si < 10) cprint(" \221\034\031 ", 50, 25);
            else cprint("\221\011\315\315\315", 50, 25);
        }
        if (key == 0x1b) break;
    }
}

void nextday()
{
    g.loanDays--;
    g.day++;
    g.totalDays++;
    if (dim[g.month - 1] < g.day) {
        g.day = 1;
        g.month++;
        if (g.month > 12) {
            g.month = 1;
            g.year++;
            if (g.year == 100)
                g.year = 0;
        }
    }
    g.debt = g.debt * 1.05;
    g.bank = g.bank * 1.000192;
}

void intro()
{
    char ch;
    int prog;
    int count;
    int step;
    int round;
    long t0;
    int limit;
    int toggle;
    register int x, y;

    round = 0;
    cursor(0);
    clrscr();
    setcolor(14, 0, 0);
    monofix(tbuf);
    count = 0;
    t0 = biostime(0, 0L);
    while (biostime(0, 0L) == t0)
        ;
    t0 = biostime(0, 0L);
    while (biostime(0, 0L) == t0) {
        puttext(10, 5, 71, 10, tbuf);
        count++;
    }
    if (count == 0) count = 1;
    g.delayTime = 31 - 55 / count;
    if (g.delayTime < 0) g.delayTime = 0;
    cprint("\221\012Version v2.01", 35, 13);
    cprint("\221\001\020\221\011\020\221\013\020   \221\036UNREGISTERED VERSION   \221\013\021\221\011\021\221\001\021", 24, 16);
    cprint("\221\014Written by Phil Erwin", 30, 19);
    cprint("Copyright 1991-93  FantasyWare Inc.", 23, 20);
    sound(0);
    delay(500);
    nosound();
    limit = 441;
    toggle = 1;
    for (; round < 10; round++) {
        prog = 0;
        toggle = !toggle;
        setcolor(rand() % 15, 0, toggle);
        x = 0;
        y = 0;
        step = rand() % 6 * 62 + 13;
        while (prog < limit) {
            x += step;
            if (x >= 62) {
                do {
                    x -= 62;
                    y++;
                    if (y > 6) y = 0;
                } while (x >= 62);
            }
            ch = tbuf[y][x * 2];
            if (ch == (char)0xb1) {
                gotoxy(x + 10, y + 5);
                cputs("\261");
                delay(5);
                prog++;
            }
            if (kbhit()) {
                prog = 0x2bd;
                round = 11;
            }
        }
    }
}

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
        g.inventory[i] = 0;
        g.priceBias[i] = 1.0;
        g.avgCost[i] = 0;
    }
    for (i = 0; i < 10; i++)
        g.eventDone[i] = 0;
    genprices(&g.price[0], &g.price[1], &g.price[2], &g.price[3], &g.price[4], &g.price[5], &g.price[6], &g.price[7]);
    g.city = rand() % 8;
    g.cash = 500L;
    g.holdSize = 10;
    g.bank = 0L;
    g.debt = 0L;
    g.hasLoan = 0;
    g.numGuns = 0;
    g.gunType = 0;
    g.status = 1;
    g.health = 100;
    g.dead = 0;
    g.sharkAngry = 0;
    g.copChase = 0;
    g.beatings = 0;
    g.totalDays = 0;
    g.month = rand() % 12 + 1;
    g.day = rand() % 28 + 1;
    g.year = rand() % 6 + 86;
    monofix(buf67d0);
    monofix(firebuf);
    monofix(barbuf);
    monofix(savebuf);
    monofix(savebuf2);
    delay(1000);
}

void monofix(s)
char *s;
{
    int len;
    register int i;

    if (g.monochrome == 1) {
        len = strlen(s);
        for (i = 1; i < len; i += 2) {
            if (s[i] <= 7)
                s[i] = (s[i] & 0x80) | 7;
            else
                s[i] = (s[i] & 0x80) | 0xf;
        }
    }
}

void main()
{
    int v;
    unsigned long lamt;
    char inbuf[81];
    char key;
    int amount;
    int running;
    int days;
    long maxbuy;
    int bonus;
    int oldstatus;
    char bigbuf[1280];
    register int si, di;

    srand((unsigned)time(0));
    gettextinfo(&g.textInfo);
    if (g.textInfo.currmode == 7 || g.textInfo.currmode == 2) g.monochrome = 1;
    else g.monochrome = 0;
    if (g.textInfo.currmode == 0 || g.textInfo.currmode == 1) {
        cputs("DRUGLORD requires 80 column mode.\r\n");
        exit(0);
    }
    intro();
    running = 1;
    do {
        newgame();
        drawmain();
        drawstatus();
        while (1) {
            cursor(0);
            frame(1, 17, 80, 25);
            cprint("\221\n(\221\034Q\221\n)uit de game", 67, 17);
            window(15, 20, 80, 25);
            cprint("\221\n(\221\034B\221\n)uy Drugs,                       (\221\034V\221\n)isit de Bank,\r\n(\221\034S\221\n)ell Drugs,                      Go to de (\221\034H\221\n)ospital,\r\n(\221\034J\221\n)et to another city,             See de (\221\034L\221\n)oan Shark", 15, 20);
            window(1, 1, 80, 25);
            cprint("\221\007Here's a list of yer possible plans, dude:", 20, 18);
            cprint("\221\007Make up your mind \221\011\020 ", 30, 25);
            if (g.status >= 300) break;
            setcolor(15, 0, 0);
            cursor(1);
            do {
                flushkeys();
                key = toupper(getch());
            } while (key != 'B' && key != 'S' && key != 'J' && key != 'V' && key != 'L' && key != 'H' && key != 'Q');
            cursor(0);
            frame(1, 17, 80, 25);
            switch (key) {
            case 'J':
                drawcities();
                do {
                    gotoxy(1, 25);
                    clreol();
                    cprint("\221\nWhere ya wanna go? ", 1, 25);
                    g.escPressed = readfield(2, 1, inbuf);
                    cursor(0);
                    if (g.escPressed == 1) break;
                    si = atol(inbuf);
                    if (si < 1 || si > 8) sfx(2);
                    if (si - 1 == g.city) {
                        cprint("\221\rAre you having \221\037flashbacks? \221\rThat's where you are now!", 28, 25);
                        sfx(3);
                        si = 0;
                        delay(500);
                    }
                } while (si < 1 || si > 8);
                cursor(0);
                if (g.escPressed == 1) continue;
                if ((long)travelcost(g.city, si - 1) > g.cash) {
                    cprint("\221\034Start flappin' yer arms, cause you ain't got de dough!", 25, 25);
                    sfx(3);
                    delay(2000);
                    continue;
                }
                g.cash -= travelcost(g.city, si - 1);
                g.city = si - 1;
                g.marketNews = 0;
                genprices(&g.price[0], &g.price[1], &g.price[2], &g.price[3], &g.price[4], &g.price[5], &g.price[6], &g.price[7]);
                gotoxy(30, 25);
                setcolor(15, 0, 0);
                for (v = 79; v > 1; v -= 2) {
                    gettext(3, 19, v, 22, bigbuf);
                    puttext(1, 19, v - 2, 22, bigbuf);
                    sound((79 - v) * 20 + 1000);
                    delay(g.delayTime);
                }
                nosound();
                drawstatus();
                for (v = 0; v < 10; v++) g.eventDone[v] = 0;
                break;
            case 'B':
                drawdruglist();
                do {
                    cprint("\221\004What kinda drugs are\r\nya interested in? ", 1, 19);
                    setcolor(14, 0, 0);
                    g.escPressed = readfield(1, 0, inbuf);
                    if (g.escPressed == 1) break;
                    di = atol(inbuf);
                    if (di < 1 || di > 8) sfx(2);
                } while (di < 1 || di > 8);
                if (g.escPressed == 1) continue;
                di--;
                maxbuy = g.cash / g.price[di];
                cprint("\221\004How many do ya wanna buy?\r\n(", 1, 22);
                setcolor(11, 0, 1);
                cprintf("%ld", maxbuy);
                cprint("\221\004 max.)", 99, 99);
                setcolor(14, 0, 0);
                do {
                    gotoxy(27, 22);
                    g.escPressed = readfield(5, 0, inbuf);
                    if (g.escPressed == 1) break;
                    amount = atol(inbuf);
                } while (amount < 0 || inbuf[0] == 0);
                cursor(0);
                if (g.escPressed == 1) continue;
                if ((long)amount > maxbuy) {
                    cprint("\221\035Hey man, if ya don't got enough\r\n  cash, don't come 'round here!", 3, 24);
                    sfx(3);
                    delay(1000);
                    continue;
                }
                if (amount > g.holdFree) {
                    cprint("\221\nDude, you can't hold that much!\r\n  Everyone's got a limit, right?", 3, 24);
                    sfx(3);
                    delay(3000);
                    continue;
                }
                if (amount > 0) {
                    cprint("\221\016* * *   \221\037S O L D ! ! !   \221\016* * *", 1, 25);
                    sfx(4);
                    delay(2000);
                    g.inventory[di] += amount;
                    g.cash -= (long)amount * g.price[di];
                    if (g.avgCost[di] == 0)
                        g.avgCost[di] = g.price[di];
                    else
                        g.avgCost[di] = (g.price[di] + g.avgCost[di]) / 2;
                }
                break;
            case 'S':
                drawdruglist();
                do {
                    cprint("\221\004What kinda drugs do\r\n ya want to unload? ", 1, 19);
                    setcolor(14, 0, 0);
                    g.escPressed = readfield(1, 0, inbuf);
                    if (g.escPressed == 1) break;
                    di = atol(inbuf);
                    if (di < 1 || di > 8) sfx(2);
                } while (di < 1 || di > 8);
                cursor(0);
                if (g.escPressed == 1) continue;
                di--;
                if (g.inventory[di] == 0) {
                    cprint("\221\rGive me a break, man, you ain't got\r\nany of that!", 1, 22);
                    sfx(3);
                    delay(1000);
                    continue;
                }
                do {
                    cprint("\221\004How many do ya wanna sell? ", 1, 22);
                    setcolor(14, 0, 0);
                    g.escPressed = readfield(5, 0, inbuf);
                    if (g.escPressed == 1) break;
                    amount = atol(inbuf);
                } while (amount < 0 || inbuf[0] == 0);
                cursor(0);
                if (g.escPressed == 1) continue;
                if (g.inventory[di] < amount) {
                    cprint("\221\025I happen to know that you ain't that\r\nmuch on ya, so quit trying to burn me!", 1, 24);
                    sfx(3);
                    delay(3000);
                    continue;
                }
                if (amount > 0) {
                    cprint("\221\036You got yerself a deal, dude!", 1, 25);
                    sfx(4);
                    delay(2000);
                    if (g.inventory[di] - amount == 0) {
                        if (di < 3) bonus = (3 - di) * 3;
                        else bonus = 1;
                        oldstatus = g.status;
                        g.status += (int)(((double)g.price[di] / (double)(unsigned long)g.avgCost[di] - 1.0) * 3.0 * bonus);
                        g.totalDeals++;
                        if (g.status > oldstatus) g.goodDeals++;
                        g.avgCost[di] = 0;
                    }
                    g.inventory[di] -= amount;
                    g.cash += (long)amount * g.price[di];
                }
                break;
            case 'V':
                cprint("\221\017You arrive at the First World Bank of America.\r\n\n               \221\nWould ya like to (\221\034D\221\n)eposit  some moola,\r\n                             or (\221\034W\221\n)ithdraw some moola? ", 20, 18);
                cursor(1);
                do {
                    si = getch();
                    si = toupper(si);
                } while (si != 'D' && si != 'W' && si != 27);
                if (si == 27) continue;
                switch (si) {
                case 'D':
                    cprint("\221\013How much 'ya gonna rathole? ", 5, 23);
                    break;
                case 'W':
                    cprint("\221\013How much 'ya gonna pull out? ", 5, 23);
                    break;
                }
                setcolor(15, 0, 0);
                g.escPressed = readfield(10, 0, inbuf);
                cursor(0);
                if (g.escPressed == 1) continue;
                lamt = atol(inbuf);
                if (lamt <= 0) continue;
                switch (si) {
                case 'D':
                    if (lamt > g.cash) {
                        cprint("\221\035You ain't got that much to deposit!", 25, 25);
                        sfx(3);
                        delay(1000);
                        continue;
                    } else {
                        cprint("\221\036Done!", 37, 25);
                        sfx(2);
                        delay(2000);
                        g.cash -= lamt;
                        g.bank += lamt;
                        drawstatus();
                    }
                    continue;
                case 'W':
                    if (lamt > g.bank) {
                        cprint("\221\035You ain't got that much to pull out!", 25, 25);
                        sfx(3);
                        delay(1000);
                    } else {
                        cprint("\221\036Done!", 37, 25);
                        sfx(2);
                        delay(2000);
                        g.cash += lamt;
                        g.bank -= lamt;
                        drawstatus();
                    }
                    continue;
                }
                if (g.escPressed == 1) continue;
            case 'L':
                cprint("\221\013Ya set foot in the 'sharks office.  You've been in locker rooms that smelled\r\nbetter than dis place!  His boys are lookin' tough in their nice goon suits,\r\nbut you don't pay dem no never-mind, cause the head-honcho, Guido is talkin...\r\n\n                    \221\nHe say, you wanna (\221\034B\221\n)orrow some cash,\r\n                      or do you wanna (\221\034P\221\n)ay me back? ", 1, 17);
                cursor(1);
                do {
                    si = getch();
                    si = toupper(si);
                } while (si != 'B' && si != 'P' && si != 27);
                cursor(0);
                if (si == 27) continue;
                switch (si) {
                case 'B':
                    if (g.sharkAngry == 1) {
                        cprint("\221\032\"Man, yer pushin' yer luck luck! I ought'ta kill you just for askin'!!\"", 5, 24);
                        delay(2000);
                        break;
                    }
                    if (g.status <= 0) {
                        cprint("\221\n\"Look, whoever you are, you're \221\037NOBODY \221\nto me.  Get outta here!\"", 5, 24);
                        delay(2000);
                        break;
                    }
                    frame(1, 21, 80, 25);
                    cprint("\221\nHow much ya wanna borrow? ", 10, 22);
                    g.escPressed = readfield(10, 0, inbuf);
                    cursor(0);
                    if (g.escPressed == 1) continue;
                    lamt = atol(inbuf);
                    if (lamt <= 0) continue;
                    if (lamt < 1000) {
                        cprint("\221\034\"Sorry, bud, but I got a \221\036$1000 \221\034minimum.  Got no time for penny-ante stuff!\"", 1, 25);
                        sfx(3);
                        delay(1000);
                        continue;
                    }
                    if (lamt / 10000 >= g.status) {
                        cprint("\221\034\"No deal, too big a risk for me! I'm a businessman, ya'know!\"", 10, 25);
                        sfx(3);
                        delay(1000);
                        continue;
                    }
                    days = (int)((log((double)g.status * 10000.0) - log((double)(g.debt + lamt))) / 0.182321556794) + 4;
                    if ((lamt + g.debt) / 10000 > g.status || (days >= g.loanDays && g.hasLoan == 1)) {
                        cprint("\221\013\"Look, you already owe me ", 1, 24);
                        setcolor(4, 0, 1);
                        cprintf("%ld ", g.debt);
                        setcolor(11, 0, 0);
                        cputs("bucks, I'm not givin ya THAT much more!!!");
                        cprint("\221\013Christ, what do I look like, Santi-Claus?\"", 20, 25);
                        sfx(3);
                        delay(2000);
                        continue;
                    } else {
                        cprint("\221\r\"Alright, I'll loan ya de money, but if you welch, my boys will come lookin\r\nfor ya! I expect to see you here to pay me in full in ", 1, 24);
                        g.debt += lamt;
                        g.cash += lamt;
                        g.hasLoan = 1;
                        g.loanDays = days;
                        setcolor(4, 0, 1);
                        cprintf("%d", g.loanDays - 1);
                        cprint("\221\r days.\"", 99, 99);
                        sfx(2);
                        delay(2000);
                        break;
                    }
                case 'P':
                    if (g.debt == 0) {
                        cprint("\221\013\"My books show dat you don't owe me nut'in... would youse like to take out\r\na little loan?\"", 1, 24);
                        sfx(3);
                        delay(2000);
                        continue;
                    }
                    frame(1, 21, 80, 25);
                    cprint("\221\nHow much ya wanna pay back? ", 10, 22);
                    g.escPressed = readfield(10, 0, inbuf);
                    cursor(0);
                    if (g.escPressed == 1) continue;
                    lamt = atol(inbuf);
                    if (lamt <= 0) continue;
                    if (lamt > g.cash) {
                        cprint("\221\034'fraid you don't got that in cash, my man.", 20, 25);
                        sfx(3);
                        delay(1000);
                        continue;
                    }
                    if (lamt >= g.debt) g.debt = 0;
                    else g.debt -= lamt;
                    g.cash -= lamt;
                    if (g.debt > 0) {
                        cprint("\221\013\"OK, but you still owe me ", 20, 24);
                        setcolor(12, 0, 1);
                        cprintf("%ld ", g.debt);
                        cprint("\221\013buck", 99, 99);
                        if (g.debt > 1) cputs("s!\"");
                        else cputs("!\"");
                        sfx(3);
                        delay(2000);
                        break;
                    } else {
                        cprint("\221\013\"Paid in full? Lucky for you, eh boys? Anytime yous need a loan, just come\r\nto me, I'll take care of ya...\"", 1, 24);
                        g.loanDays = 0;
                        g.hasLoan = 0;
                        g.sharkAngry = 0;
                        g.beatings = 0;
                        sfx(2);
                        delay(2000);
                        break;
                    }
                }
                break;
            case 'H':
                if (g.health == 100) {
                    cprint("\221\017Ya have de doc check yous out, but aside from yer bunions\r\n              on yer feet, youse is fit as a fiddle!", 15, 19);
                    pressany();
                    continue;
                }
                bonus = (100 - g.health) * 10;
                cprint("\221\017You drag yer sorry butt into de loc'l Hospital.  The docs check ye\r\n   out, and come to the conclusion that it'll cost ", 4, 19);
                setcolor(10, 0, 1);
                cprintf("%d", bonus);
                cprint("\221\017 dollars to fix ya up.", 99, 99);
                cprint("\221\rDo ya sign the consent forms? (Y/N) \020 ", 5, 22);
                cursor(1);
                do {
                    key = toupper(getch());
                } while (key != 'Y' && key != 'N' && key != 27);
                cursor(0);
                if (key == 'Y') {
                    if ((long)bonus > g.cash) {
                        cprint("\221\034You don't have enough dinero up front to foot de bill!", 12, 24);
                        sfx(3);
                        pressany();
                    } else {
                        g.cash -= bonus;
                        cprint("\221\016OK, yer laid up for ", 5, 24);
                        setcolor(12, 0, 1);
                        cprintf("%d", bonus / 15);
                        cprint("\221\016 days recoup'ratin', and yous feel good as new!", 99, 99);
                        sfx(2);
                        for (v = 1; v < bonus / 15; v++)
                            nextday();
                        g.health = 100;
                        pressany();
                    }
                }
                break;
            case 'Q':
                cprint("\221\007Are ya sure ya wanna \221\026chicken out \221\007an' \221\024QUIT? \221\007(Y/N) ", 15, 20);
                cursor(1);
                do {
                    si = toupper(getch());
                } while (si != 'Y' && si != 'N');
                if (si == 'N') continue;
                running = 0;
                break;
            }
            nextday();
            drawstatus();
            if (g.dead || !running) break;
            if (rand() % 10 == 0 && rand() % (g.status / 20 + 1) > 4) {
                if (rand() % 2 == 0) event(7);
                else event(8);
            }
            if (g.copChase == 1 || (g.status >= 100 && rand() % 400 < g.status && rand() % 8 == 0))
                event(0);
            if (g.dead || !running) break;
            if (rand() % 5 == 0)
                event(rand() % 10);
            if (g.dead || !running) break;
            if (g.loanDays < 0 && g.hasLoan == 1) {
                frame(1, 17, 80, 25);
                cprint("\221\013You're ambushed in a dark alley by Guido's thugs, must be 'bout ten guys!\r\nThe boss-man say \221\027\"Oh, man, you should'na missed de deadline, now me and\r\nboys are gonna have to break your legs! Maybe next time you'll learn, eh?\"\221\013\r\nDey take yous from all directions, you ain't gotta chance...\r\nOne steps out with a \221\026baseball bat \221\013and...\r\n", 1, 17);
                delay(3000);
                g.beatings++;
                for (v = 0; v < g.beatings; v++) {
                    frame(1, 23, 80, 23);
                    delay(500);
                    cprint("\221\014* * *    \221\024C R A C K ! !   \221\014* * *", 25, 23);
                    g.health -= 25;
                    g.sharkAngry = 1;
                    drawstatus();
                    cursor(0);
                    sfx(3);
                    delay(1500);
                }
                if (g.health > 0) {
                    cprint("\221\016\"Oh, one more 'ting... you got a week to come up with dinero, or we'll\r\n be back, comprende?\"", 1, 24);
                    delay(3000);
                    g.loanDays = 7;
                } else
                    g.dead = 1;
                pressany();
            }
            if (g.dead || !running) break;
            drawstatus();
        }
        if (running) {
            if (g.dead) restorescreen();
            else endgame();
        }
        running = playagain();
    } while (running);
    clrscr();
    puttext(3, 2, 77, 20, buf67d0);
    cprint("\221\036Thank you for playing DrugLord.", 25, 22);
    cprint("Send For The Registered Version Today!", 22, 23);
    cursor(1);
    gotoxy(1, 24);
    exit(0);
}

