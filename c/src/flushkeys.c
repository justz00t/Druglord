#include <conio.h>

void flushkeys()
{
    while (kbhit())
        getch();
}
