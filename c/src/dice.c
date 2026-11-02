#include <stdlib.h>

int dice(sides, count)
int sides, count;
{
    register int i, sum;

    sum = 0;
    for (i = 0; i < count; i++)
        sum += rand() % sides + 1;
    return sum;
}
