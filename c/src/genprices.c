#include <stdlib.h>

struct range { long lo, hi; };
extern struct range prange[8];
extern float bias[8];
extern float citymul[8][8];
extern int curcity;

void genprices(long *p0, long *p1, long *p2, long *p3, long *p4, long *p5, long *p6, long *p7)
{
    *p0 = (long)(((double)(rand() % (prange[0].hi - prange[0].lo) + prange[0].lo) + bias[0]) * citymul[curcity][0]);
    *p1 = (long)(((double)(rand() % (prange[1].hi - prange[1].lo) + prange[1].lo) + bias[1]) * citymul[curcity][1]);
    *p2 = (long)(((double)(rand() % (prange[2].hi - prange[2].lo) + prange[2].lo) + bias[2]) * citymul[curcity][2]);
    *p3 = (long)(((double)(rand() % (prange[3].hi - prange[3].lo) + prange[3].lo) + bias[3]) * citymul[curcity][3]);
    *p4 = (long)(((double)(rand() % (prange[4].hi - prange[4].lo) + prange[4].lo) + bias[4]) * citymul[curcity][4]);
    *p5 = (long)(((double)(rand() % (prange[5].hi - prange[5].lo) + prange[5].lo) + bias[5]) * citymul[curcity][5]);
    *p6 = (long)(((double)(rand() % (prange[6].hi - prange[6].lo) + prange[6].lo) + bias[6]) * citymul[curcity][6]);
    *p7 = (long)(((double)(rand() % (prange[7].hi - prange[7].lo) + prange[7].lo) + bias[7]) * citymul[curcity][7]);
}
