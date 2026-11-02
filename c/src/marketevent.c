#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

extern char *drugname[];
extern int used[8];
extern long price[8];
extern int mkttoday;

void marketevent(int count, int rng, float mul)
{
    int n;
    char *sep;
    register int k, d;

    n = rand() % count + 1;
    for (k = 0; k < 8; k++)
        used[k] = 0;
    sep = "";
    for (k = 0; k < n; k++) {
        do {
            d = rand() % 8;
        } while (used[d] == 1);
        cprintf("%s%s", sep, drugname[d]);
        if (n == 2)
            sep = " and ";
        else
            sep = ", ";
        used[d] = 1;
        price[d] = price[d] * ((double)(rand() % rng / 100) + mul);
        if (n > 2 && k + 2 == n)
            sep = ", and ";
    }
    mkttoday = 1;
}
