#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <dos.h>
int main(void)
{
    long v = atol("123");
    textcolor(14);
    gotoxy(3, 4);
    cprintf("hi %ld %d", v, rand());
    printf("%d\n", (int)sqrt(pow(3.0, 2.0) + 16.0));
    return 0;
}
