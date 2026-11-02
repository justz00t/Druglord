#include <math.h>

extern char cityxy[8][2];

long travelcost(a, b)
int a, b;
{
    return (long)(sqrt(pow((double)(cityxy[b][0] - cityxy[a][0]), 2.0) +
                       pow((double)(cityxy[b][1] - cityxy[a][1]), 2.0)) * 60.0 + 25.0);
}
