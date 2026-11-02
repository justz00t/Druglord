extern int loandays, day, totaldays, month, year;
extern int dim[12];
extern long debt, bank;

void nextday()
{
    loandays--;
    day++;
    totaldays++;
    if (dim[month - 1] < day) {
        day = 1;
        month++;
        if (month > 12) {
            month = 1;
            year++;
            if (year == 100)
                year = 0;
        }
    }
    debt = debt * 1.05;
    bank = bank * 1.000192;
}
