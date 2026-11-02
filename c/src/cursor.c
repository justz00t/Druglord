#include <dos.h>

extern unsigned char vmode;

void cursor(on)
int on;
{
    if (on == 0)
        _CX = 0x2807;
    else if (vmode == 7)
        _CX = 0x90a;
    else
        _CX = 0x607;
    _AH = 1;
    geninterrupt(0x10);
}
