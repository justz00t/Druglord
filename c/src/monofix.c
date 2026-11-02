#include <string.h>

extern int g_mono;

void monofix(s)
char *s;
{
    int len;
    register int i;

    if (g_mono == 1) {
        len = strlen(s);
        for (i = 1; i < len; i += 2) {
            if (s[i] <= 7)
                s[i] = (s[i] & 0x80) | 7;
            else
                s[i] = (s[i] & 0x80) | 0xf;
        }
    }
}
