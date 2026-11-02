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
