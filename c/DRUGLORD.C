/*
 * DRUGLORD.C -- source reconstruction of DRUGLORD.EXE v2.01
 *               (shareware DOS game by Phil Erwin, FantasyWare Inc., 1991-93)
 *
 * This is NOT the author's original source.  It is a reconstruction written so
 * that, compiled with Borland Turbo C 2.01 in the SMALL memory model, together
 * with SCREENS.C and SCRMAIN.INC, it produces an EXE that is BYTE-FOR-BYTE
 * IDENTICAL to the original DRUGLORD.EXE v2.01.
 *
 *     tcc -ms DRUGLORD.C SCREENS.C
 *
 * See README.md for the exact build steps.  Because the result has to match
 * to the byte, the source looks a little odd in places (K&R style function
 * headers, a few redundant statements, one struct that holds all globals).
 * Each such spot is explained where it appears.  Don't "tidy" it up unless
 * you re-run the comparison afterwards -- any change in the compiled code
 * breaks the match.
 *
 * Game idea (a "Dope Wars" clone): you have 30-ish days of street dealing.
 * Buy drugs where they are cheap, sell where they are expensive, travel
 * between 8 cities, borrow from the loan shark, dodge the cops and muggers,
 * and see which rank you reach at the end.  Status points ("status") measure
 * your reputation as a dealer; reaching 300 ends the game early.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>
#include <dos.h>
#include <math.h>
#include <time.h>
#include <bios.h>

/* Prototypes.  Only the functions whose arguments must be pushed in a special
 * way (a char argument, a float argument) need a full prototype; the rest are
 * declared old-style, exactly like the original. */
void setColor(int fg, int bg, char blink);
void fillChars(char c, int n, char *buf);
int readField(int width, char flag, char *buf);
void marketEvent(int count, int rng, float mul);

extern void printColored(), setCursor(), playSound(), drawBox(), clearBox(), drawDrugNames(), drawStatus(), drawMainScreen(), drawCityMenu(), drawDrugMenu(), drawHitPoints(), pressAnyKey(), flushKeys(), generatePrices(), nextDay(), showIntro(), newGame(), fixForMono(), showEndGame(), randomEvent(), showDeathScreen();
extern int rollDice(), askPlayAgain(), travelCost();

/* Low and high price for one drug (see priceRange). */
struct range { long lo, hi; };

/* Big screen images that live in SCREENS.C (a second module, linked after this one). */
extern char fightScreen[], gunSprite[], endScene[], deathScreen[], barSprite[], farewellScreen[];
extern char titleArt[7][124];

/* =========================================================================
 *  DATA (initialised)
 * ========================================================================= */

/* Top part (80x16 cells) of the main game screen: the frame, the headings
 * and the "Drugs on Hand / Street Prices" boxes.  Every cell is two bytes:
 * the IBM-PC character, then its colour attribute.  The bytes are in
 * SCRMAIN.INC.  drawMainScreen() puts this on the screen. */
unsigned char mainScreenTop[2560] = {
#include "SCRMAIN.INC"
};
/* One unused byte; present in the original data segment between the two tables. */
char padByte = 0;
/* The eight drugs.  Index order matters everywhere: inventory[], price[], ... */
char *drugName[8] = {
    "Cocaine",
    "Crack",
    "Heroin",
    "Acid",
    "Crystal",
    "Grass",
    "Speed",
    "Ludes"
};
/* Short gun names shown in the status panel, indexed by gunType (0 = no gun). */
char *gunName[6] = {
    "None",
    "Sat. Nt. Spcl",
    "Ruger",
    "Auto Mag",
    "Uzi",
    "AK-47"
};
/* Long gun names used by the gun smuggler event; index = gunType - 1. */
char *gunLongName[5] = {
    "Saturday Night Special",
    "Ruger Revolver",
    "Auto Mag",
    "Uzi",
    "AK-47 Assault Rifle"
};
/* Text labels used when the main screen is drawn (see drawMainScreen). */
char *label[14] = {
    "* * *",
    "D R U G   L O R D",
    "Date:",
    "Drugs on Hand",
    "Street Prices",
    "Location:",
    "Hold:",
    "Cash:",
    "In Bank:",
    "In Debt:",
    "# of Guns:",
    "Gun Type:",
    "Status Pts:",
    "Health:"
};
/* The eight cities you can travel to. */
char *cityName[8] = {
    "Chicago",
    "Detroit",
    "Las Vegas",
    "Los Angeles",
    "Miami",
    "New York",
    "San Diego",
    "Washington D.C."
};
/* Map position (x, y) of each city; travelCost() uses the distance between two. */
char cityCoords[8][2] = {
    {3, 2},
    {3, 1},
    {1, 2},
    {0, 3},
    {5, 4},
    {5, 2},
    {0, 3},
    {5, 2}
};
/* Price multiplier for [city][drug]: how much dearer (>1) or cheaper (<1) a
 * drug is in a city compared with the average. */
float cityPriceMult[8][8] = {
    {0.95f, 0.9f, 0.9f, 1.0f, 1.05f, 0.95f, 1.1f, 1.05f},
    {1.1f, 0.9f, 0.95f, 0.95f, 1.1f, 1.1f, 1.0f, 0.9f},
    {1.0f, 1.1f, 0.9f, 1.05f, 1.05f, 0.9f, 0.9f, 1.1f},
    {1.1f, 1.05f, 0.95f, 0.9f, 1.05f, 1.05f, 0.9f, 1.0f},
    {0.9f, 0.95f, 1.1f, 1.05f, 0.9f, 1.1f, 1.05f, 0.95f},
    {0.9f, 0.95f, 1.1f, 1.1f, 0.95f, 1.0f, 1.0f, 1.0f},
    {1.05f, 0.95f, 1.0f, 1.05f, 0.9f, 1.0f, 0.95f, 1.1f},
    {0.95f, 1.1f, 1.05f, 1.05f, 0.95f, 0.9f, 1.1f, 0.9f}
};
/* Damage multiplier per gun type (index 0 = fists / no gun). */
int gunPower[6] = { 0, 1, 2, 4, 2, 4 };
/* Base price range of each drug before the city multiplier is applied. */
struct range priceRange[8] = {
    {40000L, 50000L},
    {25000L, 30000L},
    {10000L, 15000L},
    {2000L, 3000L},
    {600L, 1000L},
    {200L, 400L},
    {70L, 100L},
    {10L, 20L}
};
/* Days per month (no leap years). */
int daysInMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
/* =========================================================================
 *  DATA (uninitialised): the whole state of the game
 * =========================================================================
 *
 * In the original program these were ordinary global variables.  Turbo C's
 * linker places uninitialised globals in an order that depends on a hash of
 * their NAMES, which we do not know, so the only reliable way to reproduce the
 * original memory layout is to put them in one struct, listed here in their
 * original address order.  Everything is accessed as g.something.
 */
struct GameState {
    int  marketNews;     /* 1 = a "street news" price event already happened in this city */
    int  beatings;       /* how many times the loan shark's thugs have beaten you up */
    float priceBias[8];  /* per-drug price bias added in generatePrices() (1.0 at the start) */
    unsigned avgCost[8]; /* average price you paid per drug (0 = none held); used to score profits */
    int  escPressed;     /* last readField() result: 1 = the player pressed ESC */
    int  dead;           /* 1 = you died */
    int  holdFree;       /* free space left in the coat (scratch value, see drawStatus) */
    long cash;           /* cash in hand */
    int  day;            /* current date: day of month */
    int  monochrome;     /* 1 = monochrome display (colours are mapped to grey) */
    int  month;          /* current date: month */
    int  year;           /* current date: year (two digits, starts in 1986-91) */
    int  health;         /* hit points, 0-100; 0 = dead */
    long bank;           /* money in the bank (earns interest) */
    int  gunType;        /* 0 = no gun, 1..5 = type of gun carried */
    int  holdSize;       /* trenchcoat capacity: how many items you can carry */
    int  status;         /* status points (reputation); 300 or more ends the game */
    long debt;           /* what you owe the loan shark (grows 5% a day) */
    int  city;           /* index of the city you are in */
    int  hasLoan;        /* 1 = you currently have a loan with a due date */
    char lineBuf[80];    /* scratch text line (also used to build "----" lines) */
    int  inventory[8];   /* how much of each drug you carry */
    int  loanDays;       /* days left to repay the loan before the thugs come */
    int  goodDeals;      /* number of sales that made a profit (for the final score) */
    int  picked[8];      /* scratch: which drugs a market event already picked */
    char eventDone[10];  /* 1 = random event number n already happened in this city */
    int  copChase;       /* 1 = the cops are after you (you refused them earlier) */
    int  delayTime;      /* animation speed, measured at start-up from the BIOS clock */
    int  sharkAngry;     /* 1 = the loan shark won't lend to you any more */
    int  totalDeals;     /* number of times you sold a whole stack of a drug */
    struct text_info textInfo; /* video mode info from gettextinfo() */
    int  totalDays;      /* days played so far */
    long price[8];       /* today's street price of each drug */
    int  numGuns;        /* how many guns you own (all of gunType) */
};
struct GameState g;

/* =========================================================================
 *  SCREEN AND SOUND HELPERS
 * ========================================================================= */

/* Select the text colour.  fg/bg are the usual 16 PC text colours.  blink != 0
 * means "bright / blinking".  On a monochrome display everything is white,
 * a non-zero background becomes grey, and blink selects high intensity. */
void setColor(fg, bg, blink)
int fg;
int bg;
char blink;
{
    if (g.monochrome == 1) {
        textcolor(15);
        if (bg)
            textbackground(7);
        else
            textbackground(0);
        if (blink == 0)
            lowvideo();
        else
            highvideo();
    } else {
        textcolor(fg);
        textbackground(bg);
    }
}

/* Print a string that may contain colour escapes.  The byte 0x91 introduces
 * one: the next byte is the colour (values above 15 mean "colour - 16, and
 * blinking").  Text is printed at (x,y) unless x == 99 (= "stay here"). */
void printColored(s, x, y)
char *s;
int x;
int y;
{
    int blink;
    char save;
    char buf[500];
    register int i, j;

    i = 0;
    strcpy(buf, s);
    if (x != 99)
        gotoxy(x, y);
    while (buf[i] != 0) {
        if (buf[i] == (char)0x91) {
            if (buf[i + 1] > 15) {
                buf[i + 1] -= 16;
                blink = 1;
            } else
                blink = 0;
            setColor(buf[i + 1], 0, blink);
            i += 2;
        }
        j = i;
        while (buf[i] != 0 && buf[i] != (char)0x91)
            i++;
        save = buf[i];
        buf[i] = 0;
        cputs(buf + j);
        buf[i] = save;
    }
}

/* Throw away every pending key press. */
void flushKeys()
{
    while (kbhit())
        getch();
}

/* Show (on != 0) or hide (on == 0) the text cursor, using BIOS int 10h/AH=1.
 * Hiding is done by giving the cursor an invalid scan-line range. */
void setCursor(on)
int on;
{
    if (on == 0)
        _CX = 0x2807;
    else if (g.textInfo.currmode == 7)
        _CX = 0x90a;
    else
        _CX = 0x607;
    _AH = 1;
    geninterrupt(0x10);
}

/* Sound effects through the PC speaker.  id 2 = cash/ding jingle, id 3 = the
 * descending "bad news" tune.  Other ids just silence the speaker. */
void playSound(id)
int id;
{
    register int i, j;

    switch (id) {
    case 2:
        sound(2000); delay(10);
        sound(5000); delay(20);
        sound(500);  delay(30);
        sound(1000); delay(40);
        break;
    case 3:
        for (i = 500; i > 200; i -= 100) {
            sound(i);       delay(100);
            sound(i - 200); delay(100);
            nosound();      delay(50);
        }
        break;
    case 4:
        for (i = 500; i < 1000; i += 100) {
            sound(i);        delay(10);
            sound(i + 2000); delay(20);
            nosound();       delay(10);
        }
        break;
    case 5:
        for (j = 0; j < 6; j++) {
            for (i = 7; i < 15; i++) {
                sound(i * 100); delay(25);
            }
            for (i = 15; i > 7; i--) {
                sound(i * 100); delay(25);
            }
        }
        break;
    case 6:
        for (i = 0; i < 5; i++)
            for (j = 0; j < 10; j++) {
                sound(i * 1000 + j * 1000); delay(40);
            }
        break;
    case 7:
        for (i = 100; i > 0; i--) {
            sound(5000 - i * 50); delay(5);
            sound(i * 50 + 5000); delay(5);
        }
        break;
    case 8:
        sound(2000); delay(30);
        sound(500);  delay(25);
        sound(70);   delay(40);
        break;
    case 11:
        sound(1000); delay(50);
        break;
    case 12:
        sound(500); delay(30);
        sound(200); delay(20);
        break;
    case 13:
        sound(100); delay(30);
        sound(50);  delay(30);
        break;
    case 14:
        for (i = 1; i < 2; i++) {
            sound(150); delay(30);
            sound(100); delay(50);
        }
        break;
    case 15:
        for (i = 1; i < 3; i++) {
            sound(100); delay(20);
            sound(50);  delay(60);
        }
        break;
    case 20:
        sound(800); delay(100);
        break;
    }
    nosound();
}

/* Draw a frame from (x1,y1) to (x2,y2).  chars[] holds the 8 border pieces
 * as 8 bytes: top-left, top, top-right, right..., see the frame tables. */
void drawBox(chars, x1, y1, x2, y2)
char *chars;
int x1, y1, x2, y2;
{
    int y;

    gotoxy(x1, y1); putch(chars[0]);
    gotoxy(x2, y1); putch(chars[2]);
    gotoxy(x2, y2); putch(chars[4]);
    gotoxy(x1, y2); putch(chars[6]);
    gotoxy(x1 + 1, y1);
    fillChars(chars[1], x2 - x1 - 1, g.lineBuf);
    cputs(g.lineBuf);
    gotoxy(x1 + 1, y2);
    fillChars(chars[5], x2 - x1 - 1, g.lineBuf);
    cputs(g.lineBuf);
    for (y = y1 + 1; y < y2; y++) {
        gotoxy(x1, y); putch(chars[3]);
        gotoxy(x2, y); putch(chars[7]);
    }
}

/* Fill buf with n copies of c and terminate it (used to build horizontal lines). */
void fillChars(c, n, buf)
char c;
int n;
char *buf;
{
    int i;

    for (i = 0; i < n; i++)
        buf[i] = c;
    buf[n] = 0;
}

/* Read a line of text from the keyboard at the cursor into buf (a field of
 * "width" characters, ENTER ends it, ESC cancels and sets g.escPressed).
 * flag != 0 allows digits only (numeric entry). */
int readField(width, flag, buf)
int width;
char flag;
char *buf;
{
    int sx;
    int sy;
    char c;
    register int i;

    i = 0;
    sx = wherex();
    sy = wherey();
    setCursor(1);
    fillChars(' ', width, g.lineBuf);
    cputs(g.lineBuf);
    while (1) {
        if (width - 1 > i) {
            gotoxy(sx + i, sy);
            cputs(" ");
        }
        gotoxy(sx + i, sy);
        c = getch();
        if (c == 0) {
            flushKeys();
            continue;
        }
        if (c == 27)
            return 1;
        if (c == 13) {
            buf[i] = 0;
            break;
        }
        if (c == 8) {
            if (i > 0) {
                buf[i] = 0;
                i--;
            }
            buf[i] = 0;
            cputs(" ");
            continue;
        }
        buf[i] = c;
        gotoxy(sx + i, sy);
        cprintf("%c", buf[i]);
        if (width - 1 > i)
            cputs(" ");
        if (width - 1 == i) {
            if (flag == 0)
                break;
            else {
                playSound(2);
                gotoxy(sx + i, sy);
                cputs(" ");
                i--;
            }
        }
        i++;
    }
    buf[i + 1] = 0;
    if (flag == 0 && width > 1)
        buf[i] = 0;
    return 0;
}

/* Blank the screen rectangle (x1,y1)-(x2,y2) by making it a text window and
 * clearing it, then restoring the full-screen window. */
void clearBox(x1, y1, x2, y2)
int x1, y1, x2, y2;
{
    gettextinfo(&g.textInfo);
    window(x1, y1, x2, y2);
    clrscr();
    textmode(g.textInfo.currmode);
}

/* =========================================================================
 *  MAIN SCREEN DRAWING
 * ========================================================================= */

/* Print the eight drug names in both lists (inventory on the left, street
 * prices in the middle). */
void drawDrugNames()
{
    int i;

    setColor(15, 0, 0);
    for (i = 0; i < 8; i++) {
        gotoxy(5, i + 7);
        cprintf("%-7s", drugName[i]);
        gotoxy(28, i + 7);
        cprintf("%-7s", drugName[i]);
    }
}

/* Redraw all the numbers on the main screen: inventory, prices, city, free
 * space in the coat, cash, bank, debt (+ days left on the loan), guns, gun
 * name, status points, health (red & blinking when 25 or less) and the date. */
void drawStatus()
{
    register int i;

    g.holdFree = 0;
    setCursor(0);
    drawDrugNames();
    for (i = 0; i < 8; i++) {
        gotoxy(16, i + 7);
        if (g.inventory[i] > 0) setColor(14, 0, 1);
        else setColor(7, 0, 0);
        cprintf("%6d", g.inventory[i]);
        gotoxy(38, i + 7);
        setColor(12, 0, 1);
        cprintf("%10ld", g.price[i]);
        g.holdFree += g.inventory[i];
    }
    setColor(14, 0, 1);
    g.holdFree = g.holdSize - g.holdFree;
    gotoxy(63, 4);
    cprintf("%-15s", cityName[g.city]);
    gotoxy(63, 5);
    cprintf("%-6d (%d max)", g.holdFree, g.holdSize);
    gotoxy(63, 7);
    cprintf("$%-10ld", g.cash);
    gotoxy(63, 8);
    cprintf("$%-10ld", g.bank);
    gotoxy(63, 9);
    cprintf("$%-10ld", g.debt);
    gotoxy(74, 9);
    if (g.hasLoan == 1) {
        cputs("(   )");
        if (g.loanDays <= 2) setColor(140, 0, 1);
        gotoxy(75, 9);
        cprintf("%3d", g.loanDays);
        setColor(14, 0, 1);
    } else
        cputs("     ");
    gotoxy(63, 11);
    cprintf("%-6d", g.numGuns);
    gotoxy(63, 12);
    cprintf("%-13s", gunName[g.gunType]);
    gotoxy(63, 14);
    cprintf("%-6d", g.status);
    gotoxy(63, 15);
    if (g.health <= 25) setColor(132, 0, 1);
    cprintf("%-4d", g.health);
    setColor(14, 0, 1);
    gotoxy(12, 3);
    cprintf("%#02d/%#02d/%#02d", g.month, g.day, g.year);
}

/* Draw the static part of the main screen: clear, copy the 80x16 template
 * (mainScreenTop) to the screen and add the coloured labels on top of it. */
void drawMainScreen()
{
    setCursor(0);
    setColor(7, 0, 0);
    clrscr();
    puttext(1, 1, 80, 16, mainScreenTop);
    setColor(14, 0, 0);
    printColored(label[0], 24, 2);
    printColored(label[0], 52, 2);
    setColor(9, 0, 1);
    printColored(label[1], 32, 2);
    setColor(11, 0, 0);
    printColored(label[2], 6, 3);
    setColor(6, 0, 1);
    printColored(label[3], 6, 5);
    setColor(4, 0, 1);
    printColored(label[4], 30, 5);
    setColor(6, 0, 0);
    printColored(label[5], 53, 4);
    printColored(label[6], 57, 5);
    setColor(10, 0, 0);
    printColored(label[7], 57, 7);
    printColored(label[8], 54, 8);
    printColored(label[9], 54, 9);
    setColor(13, 0, 0);
    printColored(label[10], 52, 11);
    printColored(label[11], 53, 12);
    setColor(11, 0, 0);
    printColored(label[12], 51, 14);
    printColored(label[13], 55, 15);
    setColor(11, 0, 0);
    printColored("v2.01", 74, 2);
}

/* Price of a plane ticket between cities a and b: straight-line distance on
 * the map (cityCoords) times 60, plus 25 dollars. */
int travelCost(a, b)
int a, b;
{
    return (int)(sqrt(pow((double)(cityCoords[b][0] - cityCoords[a][0]), 2.0) +
                       pow((double)(cityCoords[b][1] - cityCoords[a][1]), 2.0)) * 60.0 + 25.0);
}

/* Draw the "Drug Cities" menu (two columns of four) with the ticket price
 * next to every city except the one you are in. */
void drawCityMenu()
{
    int j;
    register int i, d;

    setCursor(0);
    clearBox(1, 17, 80, 25);
    printColored("\x91\005Drug Cities:", 34, 17);
    for (d = 0; d < 4; d++) {
        for (j = 0; j <= 1; j++) {
            i = j * 4 + d;
            if (i > 8) break;
            gotoxy(j * 35 + 10, d + 19);
            setColor(12, 0, 1);
            cprintf("%2d. ", i + 1);
            setColor(11, 0, 0);
            cprintf("%-20s", cityName[i]);
            if (i == g.city) continue;
            cputs("(");
            setColor(2, 0, 0);
            cprintf("$%d", travelCost(g.city, i));
            printColored("\x91\x0b)", 11, 0);
        }
    }
}

/* Roll new street prices for all eight drugs.  Each price is a random value in
 * the drug's range, plus g.priceBias, times the current city's multiplier.
 * (The original passes the eight price slots as eight pointers -- an odd
 * choice, but it has to stay for the bytes to match.) */
void generatePrices(long *p0, long *p1, long *p2, long *p3, long *p4, long *p5, long *p6, long *p7)
{
    *p0 = (long)(((double)(rand() % (priceRange[0].hi - priceRange[0].lo) + priceRange[0].lo) + g.priceBias[0]) * cityPriceMult[g.city][0]);
    *p1 = (long)(((double)(rand() % (priceRange[1].hi - priceRange[1].lo) + priceRange[1].lo) + g.priceBias[1]) * cityPriceMult[g.city][1]);
    *p2 = (long)(((double)(rand() % (priceRange[2].hi - priceRange[2].lo) + priceRange[2].lo) + g.priceBias[2]) * cityPriceMult[g.city][2]);
    *p3 = (long)(((double)(rand() % (priceRange[3].hi - priceRange[3].lo) + priceRange[3].lo) + g.priceBias[3]) * cityPriceMult[g.city][3]);
    *p4 = (long)(((double)(rand() % (priceRange[4].hi - priceRange[4].lo) + priceRange[4].lo) + g.priceBias[4]) * cityPriceMult[g.city][4]);
    *p5 = (long)(((double)(rand() % (priceRange[5].hi - priceRange[5].lo) + priceRange[5].lo) + g.priceBias[5]) * cityPriceMult[g.city][5]);
    *p6 = (long)(((double)(rand() % (priceRange[6].hi - priceRange[6].lo) + priceRange[6].lo) + g.priceBias[6]) * cityPriceMult[g.city][6]);
    *p7 = (long)(((double)(rand() % (priceRange[7].hi - priceRange[7].lo) + priceRange[7].lo) + g.priceBias[7]) * cityPriceMult[g.city][7]);
}

/* Roll "count" dice with "sides" sides each and return the total. */
int rollDice(sides, count)
int sides, count;
{
    register int i, sum;

    sum = 0;
    for (i = 0; i < count; i++)
        sum += rand() % sides + 1;
    return sum;
}

/* Draw the numbered list of drugs (1-8) in the bottom panel, used when buying or selling. */
void drawDrugMenu()
{
    register int i;

    setCursor(0);
    printColored("\221\r* * *  \221\031D R U G S  \221\r* * *", 45, 18);
    fillChars(0xc4, 40, g.lineBuf);
    setColor(14, 0, 0);
    printColored(g.lineBuf, 36, 19);
    for (i = 0; i < 4; i++) {
        gotoxy(43, i + 21);
        setColor(12, 0, 1);
        cprintf("%1d. ", i + 1);
        setColor(10, 0, 0);
        cprintf("%-15s", drugName[i]);
        gotoxy(60, i + 21);
        setColor(12, 0, 1);
        cprintf("%1d. ", i + 5);
        setColor(10, 0, 0);
        cprintf("%-15s", drugName[i + 4]);
    }
    setCursor(1);
}

/* Fight screen: show the hit points of one fighter.  side 0 = you, otherwise
 * the opponent.  Shows "Alive" or "Dead". */
void drawHitPoints(hp, row, side)
int hp, row, side;
{
    register char *s;

    if (side == 0)
        gotoxy(12, row + 10);
    else
        gotoxy(52, row + 11);
    if (hp > 0) {
        s = "Alive";
        setColor(15, 0, 1);
    } else {
        s = "Dead";
        setColor(7, 0, 0);
    }
    cprintf("%4d            %-12s", hp, s);
}

/* A "street news" price event: pick 1..count different drugs, announce them
 * ("X, Y, and Z") and multiply their price by (random 0..rng/100) + mul.
 * Used for both price spikes (mul large) and busts (mul small). */
void marketEvent(int count, int rng, float mul)
{
    int n;
    char *sep;
    register int k, d;

    n = rand() % count + 1;
    for (k = 0; k < 8; k++)
        g.picked[k] = 0;
    sep = "";
    "";
    for (k = 0; k < n; k++) {
        do {
            d = rand() % 8;
        } while (g.picked[d] == 1);
        cprintf("%s%s", sep, drugName[d]);
        if (n == 2)
            sep = " and ";
        else
            sep = ", ";
        g.picked[d] = 1;
        g.price[d] = g.price[d] * ((double)(rand() % rng / 100) + mul);
        if (n > 2 && k + 2 == n)
            sep = ", and ";
    }
    g.marketNews = 1;
}

/* Show "Press any key to continue" with a spinning marker and wait for a key. */
void pressAnyKey()
{
    char spin[3] = { 0xf9, 0x09, 0x4f };
    register int i;

    setCursor(0);
    flushKeys();
    printColored("\221\032Press any key to continue", 27, 25);
    setColor(14, 0, 0);
    do {
        for (i = 0; i < 3; i++) {
            gotoxy(25, 25);
            putch(spin[i]);
            gotoxy(53, 25);
            putch(spin[i]);
            delay(100);
        }
    } while (!kbhit());
    clearBox(25, 25, 54, 25);
}

/* =========================================================================
 *  RANDOM EVENTS
 * =========================================================================
 *
 * Handles one random event.  "kind" is the event number:
 *   0  the cops attack (a fight screen)            5  girlfriend bakes your weed
 *   1  cops sell confiscated drugs  (prices down)  6  a mugger robs you
 *   2  cheap homemade drugs         (prices down)  7  trenchcoat salesman
 *   3  big drug bust                (all up)       8  gun smuggler
 *   4  a factory burns down         (one up)       9  blackmail letter
 * Each event can happen only once per city visit (g.eventDone[kind]).
 *
 * The first case is by far the longest: it is the cop fight.  The unusual
 * local names (v20, v1e, ...) are the original compiler's stack slots reused
 * by several cases; their meaning is explained in each case. */
void randomEvent(kind)
int kind;
{
    int choice;
    int nfr;
    int hp[11];
    int chp[11];
    int ctype;
    int tgt;
    int copsleft;
    int alive;
    int ftgt;
    int loot;
    int state;
    int maxshots;
    int shots;
    char unused1;
    char key;
    int v20;
    int v1e;
    int v1c;
    int v1a;
    long bm;
    float f;
    char *lbl[2] = { "Health", "Status" };
    char buf[12];
    register int si, di;

    setCursor(0);
    if (g.eventDone[kind] == 1) return;
    clearBox(1, 17, 80, 25);
    switch (kind) {
    /* ---- Event 0: the cops fight --------------------------------
     * Fighters on your side: hp[0] is you, hp[1..nfr] are friends/posse.
     * Cops: chp[0..di-1] (di = number of cops, ctype = their gun type).
     * state: 0 = normal, 1 = escaped, 2 = tried to run and failed (cops shoot first).
     */
    case 0:
        g.copChase = 0;
        state = 0;
        if (g.gunType < 4) maxshots = 1;
        else maxshots = 3;
        printColored("\221\004The \221\011cops \221\004are on your tail!!!", 27, 17);
        /* Ask how you want to face them: alone, with a friend (costs 1 status
         * point) or with a posse (costs 5 status points).
         */
        playSound(5);
        printColored("\221\012Do you wanna   \221\0341\221\012) Go at it alone", 15, 19);
        printColored("\221\0342\221\012) Call a friend (1 status pt.)", 30, 20);
        printColored("\221\0343\221\012) Call a posse (5 status pts.)", 30, 21);
        choice = 0;
        while (1) {
            do {
                clearBox(1, 23, 80, 25);
                printColored("\221\016Call it \020 ", 20, 23);
                setCursor(1);
                g.escPressed = readField(1, 0, g.lineBuf);
                setCursor(0);
                choice = atol(g.lineBuf);
            } while (choice < 1 || choice > 3);
            if ((choice == 2 && g.status == 0) || (choice == 3 && g.status < 5)) {
                printColored("\221\014You don't have enough points!", 25, 25);
                playSound(3);
                delay(2000);
                clearBox(25, 25, 80, 25);
            } else
                break;
        }
        /* nfr = number of helpers who fight next to you. */
        switch (choice) {
        case 1:
            nfr = 0;
            break;
        case 2:
            nfr = 1;
            g.status--;
            break;
        case 3:
            nfr = rand() % 4 + g.status / 100 + 4;
            if (nfr > 8) nfr = 8;
            g.status -= 5;
            break;
        }
        /* Draw the fight screen and its captions. */
        clearBox(1, 1, 80, 25);
        puttext(1, 1, 80, 22, fightScreen);
        setColor(14, 0, 0);
        printColored(label[0], 24, 2);
        printColored(label[0], 52, 2);
        setColor(12, 0, 1);
        printColored(label[1], 32, 2);
        setColor(10, 0, 1);
        printColored(lbl[0], 9, 8);
        printColored(lbl[0], 49, 8);
        printColored(lbl[1], 28, 8);
        printColored(lbl[1], 68, 8);
        setColor(14, 0, 1);
        printColored(label[11], 10, 6);
        printColored(label[11], 50, 6);
        printColored(label[10], 9, 5);
        setColor(12, 0, 0);
        gotoxy(20, 5);
        cprintf("%-10d", g.numGuns);
        setColor(6, 0, 0);
        gotoxy(20, 6);
        cprintf("%-15s", gunName[g.gunType]);
        /* Hit points: yours, then every helper (25..34 plus bonus dice from status).
         */
        hp[0] = g.health;
        setColor(6, 0, 0);
        for (si = 1; si <= nfr; si++) {
            hp[si] = rand() % 10 + rollDice(10, g.status / 50) + 25;
            gotoxy(1, si + 10);
            if (choice == 2) cprintf("%10s:", "Friend");
            if (choice == 3) cprintf("%6s #%2d:", "Posse", si);
        }
/* Choose the cops: how many (di) and how strong (ctype is the gun type,
 * limited to 2 or 3, tougher with more status).  Re-roll until valid.
 * Once in 20 times a bigger squad appears.
 */
again:
        di = rand() % 8 + 3;
        ctype = g.status / (di * 2);
        if (rand() % 20 == 0) {
            if (ctype < 2) {
                di = rand() % 4 + 1;
                ctype = g.status / di;
                if (ctype > 3) ctype = 3;
                if (ctype < 2) ctype = 2;
            } else {
                di = rand() % 3 + 8;
                ctype = 3;
            }
        } else {
            if (ctype < 2) goto again;
            if (ctype > 3) goto again;
        }
        gotoxy(60, 6);
        setColor(9, 0, 0);
        cprintf("%-15s", gunName[ctype]);
        setColor(9, 0, 0);
        for (si = 0; si < di; si++) {
            gotoxy(42, si + 11);
            cprintf("%6s #%2d", "Cop", si + 1);
            chp[si] = rollDice(15, 2) + 15;
        }
        /* Cop hit points: two 15-sided dice + 15 each. */
        copsleft = di;
        alive = nfr + 1;
        for (si = 0; si <= nfr; si++)
            drawHitPoints(hp[si], si, 0);
        for (si = 0; si < di; si++)
            drawHitPoints(chp[si], si, 1);
        /* Fight round loop: ask Fight or Run, then play the round. */
        do {
            clearBox(1, 24, 80, 25);
            printColored("\221\017Do ya wanna (\221\034F\221\017)ight or (\221\034R\221\017)un? ", 5, 24);
            setCursor(1);
            do {
                key = toupper(getch());
            } while (key != 'F' && key != 'R');
            setCursor(0);
            putch(key);
            while (1) {
                switch (key) {
                /* Your side shoots first (unless you just failed to run).  Each fighter
                 * fires at a random living cop; 50% hit chance.  Guns hit with
                 * rollDice(6, gunPower), bare fists with a single d4.
                 */
                case 'F':
                    if (state != 2) {
                        for (si = 0; si <= nfr; si++) {
                            if (hp[si] == 0) continue;
                            if (si < g.numGuns) printColored("\221\014Firing", 28, si + 10);
                            else printColored("\221\014Punching", 27, si + 10);
                            shots = 0;
                            do {
                                do {
                                    tgt = rand() % di;
                                } while (chp[tgt] == 0);
                                playSound(g.gunType + 10);
                                if (rand() % 2 == 0) {
                                    printColored("\221\024*  Hit!  *", 67, tgt + 11);
                                    playSound(8);
                                    delay(100);
                                    if (si < g.numGuns) chp[tgt] -= rollDice(6, gunPower[g.gunType]);
                                    else chp[tgt] -= rollDice(4, 1);
                                    if (chp[tgt] <= 0) {
                                        chp[tgt] = 0;
                                        copsleft--;
                                    }
                                } else
                                    printColored("\221\016- Missed! -", 67, tgt + 11);
                                delay(200);
                                drawHitPoints(chp[tgt], tgt, 1);
                                shots++;
                                if (hp[0] == 0 || copsleft == 0) break;
                            } while (g.gunType > 3 && shots < maxshots && copsleft > 0 && si < g.numGuns);
                            drawHitPoints(hp[si], si, 0);
                            if (copsleft == 0) break;
                        }
                    }
                    /* The surviving cops shoot back at random living members of your side.
                     * Every one of your helpers who dies costs you a status point.
                     */
                    if (copsleft == 0) break;
                    state = 0;
                    for (si = 0; si < di; si++) {
                        if (chp[si] == 0) continue;
                        printColored("\221\014Firing", 68, si + 11);
                        shots = 0;
                        do {
                            ftgt = rand() % (nfr + 1);
                        } while (hp[ftgt] == 0);
                        playSound(ctype + 10);
                        if (rand() % 2 == 0) {
                            printColored("\221\024*  Hit!  *", 26, ftgt + 10);
                            playSound(8);
                            delay(100);
                            hp[ftgt] -= rollDice(6, gunPower[ctype]);
                            if (hp[ftgt] <= 0) {
                                g.status--;
                                hp[ftgt] = 0;
                                alive--;
                            }
                        } else
                            printColored("\221\016- Missed! -", 26, ftgt + 10);
                        delay(200);
                        drawHitPoints(hp[ftgt], ftgt, 0);
                        shots++;
                        drawHitPoints(chp[si], si, 1);
                        if (hp[0] == 0 || alive == 0) break;
                    }
                    break;
                /* Run away: succeeds when a random roll (+cops/3) is lower than yours (+helpers/3).
                 */
                case 'R':
                    printColored("\221\013You take off running... ", 20, 25);
                    delay(3000);
                    if (rand() % 10 + di / 3 < rand() % 10 + nfr / 3) {
                        printColored("\221\036and ya got away!!!", 99, 99);
                        g.health = hp[0];
                        state = 1;
                    } else {
                        printColored("\221\013and ya didn't get away!!!", 99, 99);
                        state = 2;
                        key = 'F';
                        continue;
                    }
                    break;
                }
                break;
            }
        } while (copsleft > 0 && alive > 0 && hp[0] > 0 && state != 1);
        /* You got away. */
        if (state == 1) {
            delay(3000);
            drawMainScreen();
            break;
        }
        /* Fight over.  If you died the game ends; otherwise the cops are dead. */
        g.health = hp[0];
        if (g.health == 0) {
            g.dead = 1;
            break;
        }
        clearBox(1, 23, 80, 25);
        printColored("\221\012Yous wasted 'em!!! Way ta go, man!!!", 23, 23);
        delay(1000);
        /* v20 = 1 means "sell the guns" (default); 0 if you took them instead. */
        v20 = 1;
        /* If you already own better and enough guns, don't even ask. */
        if (ctype < g.gunType && di <= g.numGuns) goto skipask;
        setColor(7, 0, 0);
        gotoxy(21, 24);
        if (di == 1) strcpy(buf, "his gun");
        else strcpy(buf, "their guns");
        cprintf("Do you wanna take %s, man? (Y/N) ", buf);
        setCursor(1);
        do {
            key = toupper(getch());
        } while (key != 'Y' && key != 'N');
        setCursor(0);
        /* Taking their guns: add to your collection or swap to the better type. */
        if (key == 'Y') {
            printColored("\221\016You \221\014want\221\016 'em, you \221\014got \221\016'em!  What a bunch'a \221\006chumps!", 15, 25);
            v20 = 0;
            if (g.gunType == ctype) g.numGuns += di;
            else {
                g.gunType = ctype;
                g.numGuns = di;
            }
        } else
            printColored("\221\011OK, man, it's yer call.", 29, 25);
        delay(2000);
skipask:
        /* Otherwise sell the guns: 200 dollars per gun per gun-type level, plus status.
         */
        if (v20 == 1) {
            printColored("\221\013You sell the ", 5, 24);
            loot = di * ctype * 200;
            cprintf("%d guns that were on the pigs you wasted for %d bucks!", di, loot);
            printColored("\221\013This'll teach 'em not to mess with ya, huh?", 20, 25);
            g.cash += loot;
            g.status += di;
            delay(4000);
        }
        if (g.dead == 0) drawMainScreen();
        break;
    /* ---- Event 1: police sell confiscated drugs.  1-3 drugs drop to
     * 70..99% of their price.  Skipped if a price event already ran.
     */
    case 1:
        if (g.marketNews == 1) break;
        printColored("\221\013\"Hey man, check dis out!  Word has it on de street dat deres des cops, see?\r\nThey musta decided that workin' for 25 G's a year ain't enough, so de've\r\ndecided ta sell some of de take drugs confiscated on de side! De market is\r\nflooded with ", 1, 18);
        setColor(6, 0, 1);
        marketEvent(3, 30, 0.7f);
        setColor(11, 0, 0);
        cprintf("!\"");
        playSound(6);
        pressAnyKey();
        break;
    /* ---- Event 2: cheap homemade stuff.  1-3 drugs drop to 60..89%. */
    case 2:
        printColored("\221\007\"Hey, I heard it through de grapevine dat deres some cheap homemade stuff\r\navailable from some of the homeboys downtown.  Real primo stuff, dey say!\r\nAll I hear on de streets is 'Cheap ", 1, 18);
        setColor(6, 0, 1);
        marketEvent(3, 40, 0.6f);
        setColor(7, 0, 0);
        cprintf("!'\"");
        pressAnyKey();
        break;
    /* ---- Event 3: "EXTRA!" newspaper - a drug bust.  Every price rises 5..29%.
     * The newspaper is a boxed text drawn with line-drawing characters.
     */
    case 3:
        window(15, 17, 80, 25);
        printColored("\221\007\332\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\277\r\n\263  * * *   \221\037E X T R A ! !   E X T R A ! !\221\007   * * *  \263\r\n\263               \221\037Read all about it!!\221\007               \263\r\n\303\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\264\r\n\263  After six months of undercover investigation,  \263\r\n\263  police close in and make a major drug bust!    \263\r\n\263  This will no doubt affect street drug prices.  \263\r\n\300\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\331", 15, 17);
        window(1, 1, 80, 25);
        for (si = 0; si < 8; si++)
            g.price[si] = g.price[si] * ((double)(rand() % 25) / 100.0 + 1.05);
        playSound(7);
        pressAnyKey();
        break;
    /* ---- Event 4: a supplier's factory is destroyed.  One drug costs
     * 120..159% of the normal price.
     */
    case 4:
        printColored("\221\r\"Dude, I got some news for ya... a factory of one of de cities biggest suppliersof ", 1, 18);
        setColor(6, 0, 1);
        marketEvent(1, 40, 1.2f);
        setColor(13, 0, 0);
        if (rand() % 2 == 1) cputs(" burned down");
        else cputs(" blew up");
        printColored("last night.  I don't have to tell ya what this'll on de\r\nstreet...\"", 99, 99);
        pressAnyKey();
        break;
    /* ---- Event 5: your girlfriend turns all your weed (drug #5) into brownies.
     * Only happens if you actually carry some.
     */
    case 5:
        if (g.inventory[5] == 0) break;
        printColored("\221\006Ya come back to yer place tonight and find dat yer sexy main squeeze tought\r\ndat it wouldst be a good idea to use some of yer weed to make brownies!!!\r\nAll tings considered, yous take it pretty well.  After all, if ya were mad,\r\nyou would've \221\007chopped \221\006up her body \221\014before \221\006dumping it in de river!", 1, 17);
        g.inventory[5] = 0;
        pressAnyKey();
        break;
    /* ---- Event 6: a mugger.  With 10+ status you may talk your way out
     * (costs 10 status).  Otherwise he takes ALL your cash; if you had less
     * than $50 he shoots you instead (2..12 damage).
     */
    case 6:
        printColored("\221\014Yer walking along de street at night, when from outta nowhere jumps dis goon!\r\nHe waves a 9mm Beretta at yous and demands yer fork over all yer money.", 1, 17);
        while (1) {
            if (g.status >= 10 && rand() % 20 != 0) {
                printColored("\221\016Do ya wanna use yer status as a major drug dealer to get outta this? (Y/N) ", 1, 20);
                setCursor(1);
                do {
                    key = toupper(getch());
                } while (key != 'Y' && key != 'N');
                setCursor(0);
                if (key == 'Y') {
                    printColored("\221\014Yous tell him whos he's dealin' wit.  He looks nervous, asks fer yer pardon,den runs off down an alley!", 1, 22);
                    g.status -= 10;
                    break;
                }
            }
            if (g.cash < 50) {
                printColored("\221\013\"This is all you got? &%&*! this, man!\"", 20, 21);
                printColored("He looks at ya like a crazed animal, and pulls de trigger!", 10, 22);
                printColored("\221\016* * *   \221\034B L A M   \221\016* * *", 28, 23);
                playSound(13);
                g.health -= rand() % 6 + rand() % 6 + 2;
            } else
                printColored("\221\rHe laughs merrily as he takes yer cash and runs off down de street!", 5, 22);
            g.cash = 0;
            break;
        }
        pressAnyKey();
        break;
    /* ---- Event 7: trenchcoat salesman.  Offers a bigger coat (capacity in
     * v20, a multiple of 10, up to 50 more than yours).  The price bm grows
     * with the capacity: capacity^3.34 * 0.0634, times a random 1-200%.
     */
    case 7:
        if (g.holdSize >= 300) break;
        do {
            v20 = rand() % 10 + 1;
            if (rand() % 5 == 0) {
                v20 += rand() % 10 + 1;
                if (rand() % 5 == 0)
                    v20 += rand() % 10 + 1;
            }
            v20 *= 10;
        } while (g.holdSize + 50 < v20 || v20 <= g.holdSize);
        bm = pow((double)v20, 3.34) * 0.0634;
        bm = (long)((double)((rand() % 200 + 1) * bm) / 10000.0) * 100;
        printColored("\221\nYou are approached by a shady-lookin guy in a long dark trenchcoat.  He says:", 1, 17);
        printColored("\221\011\"Hey, bud, I got a \221\036special \221\011trenchcoat for ya, just your style!", 10, 19);
        printColored("It's got a capacity of ", 11, 20);
        setColor(12, 0, 1);
        cprintf("%d", v20);
        printColored("\221\011 and I won't take under ", 99, 99);
        setColor(12, 0, 1);
        cprintf("$%ld", bm);
        printColored("\221\011 for it.\"", 99, 99);
        printColored("\221\016Is it a deal? (Y/N) \020 ", 1, 23);
        setCursor(1);
        do {
            key = toupper(getch());
        } while (key != 'Y' && key != 'N');
        setCursor(0);
        if (key == 'Y') {
            if (g.cash < bm)
                printColored("\221\011\"Sorry Charly, I don't take credit! Maybe when you're a little more solvent!\"", 1, 24);
            else {
                printColored("\221\011\"You bought yourself one hell of a trenchcoat!\"", 16, 24);
                g.cash -= bm;
                g.holdSize = v20;
            }
        } else
            printColored("\221\011\"OK, it's your loss, man.  What a shame!\"", 20, 24);
        pressAnyKey();
        break;
    /* ---- Event 8: gun smuggler Juan.  v20 = gun type offered (0-4),
     * v1e = how many, v1c = price each, v1a = total.  Buying replaces
     * your present guns with the new batch.
     */
    case 8:
        v20 = rand() % 5;
        if (rand() % 10 == 0) v1e = rand() % 6 + 5;
        else v1e = rand() % 5 + 2;
        v1c = v20 * 200 + 200;
        v1a = v1e * v1c;
        printColored("\221\rJuan, one of the most powerful underground gun smugglers in the world,\r\ncomes to you with an offer:", 1, 17);
        printColored("\221\007\"I have a shipment of ", 1, 20);
        setColor(12, 0, 1);
        cprintf("%s's", gunLongName[v20]);
        printColored("\221\007 coming in tonight.  I'm", 99, 99);
        printColored("distributing them in shipments of ", 1, 21);
        setColor(12, 0, 1);
        cprintf("%d ", v1e);
        printColored("\221\007at ", 99, 99);
        setColor(12, 0, 1);
        cprintf("$%d ", v1c);
        printColored("\221\007each, for a total of ", 99, 99);
        setColor(12, 0, 1);
        cprintf("$%d", v1a);
        setColor(7, 0, 0);
        cputs(".\"");
        printColored("\221\rAre you interested in an order? (Y/N) \020 ", 20, 23);
        setCursor(1);
        do {
            key = toupper(getch());
        } while (key != 'Y' && key != 'N');
        setCursor(0);
        if (key == 'Y') {
            if (v1a > g.cash)
                printColored("\221\007\"I'm sorry, sir, but I only accept cash \221\017up-front\221\007.  Business is business.\"", 5, 24);
            else {
                printColored("\221\007\"Very well, they will be delivered by tomorrow.  Good doing business with you.\"", 1, 24);
                g.numGuns = v1e;
                g.gunType = v20 + 1;
                g.cash -= v1a;
            }
        } else
            printColored("\221\007\"As you wish.  Perhaps next time we may do business.\"", 15, 24);
        pressAnyKey();
        break;
    /* ---- Event 9: blackmail.  Only if you have $10000+.  Demands 70..99% of
     * your cash (rounded to thousands).  If you refuse, 80% of the time
     * the cops will come after you (event 0 is re-armed).
     */
    case 9:
        if (g.cash < 10000L) break;
        f = rand() % 30 / 100.0 + 0.7;
        bm = g.cash * f / 1000.0;
        bm = bm * 1000;
        printColored("\221\nYous get a note in de mail from a business associate of yers.  He says dat\r\nif yous don't deposit ", 1, 17);
        setColor(12, 0, 1);
        cprintf("$%ld ", bm);
        printColored("\221\nin his Swiss bank account, he's gonna \221\026rat \221\nto de\r\ncops everything dat he knows about you.", 99, 99);
        printColored("\221\rDo you pay the blackmail? (Y/N) \020 ", 1, 21);
        setCursor(1);
        do {
            key = toupper(getch());
        } while (key != 'Y' && key != 'N');
        setCursor(0);
        if (key == 'Y') {
            printColored("\221\rYou wire de money into his account.  That really \221\034^$&?*@ \221\rsucks!!!", 1, 24);
            g.cash -= bm;
        } else {
            printColored("\221\014Yeah, man, nobody pushes you around! I respect yer integrity!!!", 9, 24);
            if (rand() % 5 != 0) {
                g.copChase = 1;
                g.eventDone[0] = 0;
            }
        }
        pressAnyKey();
        break;
    }
    /* Remember that this event has happened in this city. */
    g.eventDone[kind] = 1;
    if (g.dead == 0) drawStatus();
}

/* Text of the final score screen (shown in a scrolling box by showEndGame).
 * showEndGame() fills the numbers in at the right-hand end of the lines
 * (that is why the strings contain blanks).  \221 = colour escape. */
char endText[18][50] = {
    "           \221\034CONGRATULATIONS!!!         ",
    "                                      ",
    "\221\016Yous has become a \221\035DRUGLORD!!!\221\016  I'm    ",
    "proud ta know you, man!  Anyway, check",
    "dis out --                            ",
    "                                      ",
    "\221\013           Cash in Hand:\221\036              ",
    "\221\013           Cash in Bank:\221\036              ",
    "\221\013          Still in Debt:\221\036              ",
    "\221\013   Number of Good Deals:\221\036              ",
    "\221\013  Number of Total Deals:\221\036              ",
    "\221\013                 DEAL %:\221\036              ",
    "\221\013                        \221\036              ",
    "\221\013             Total Days:\221\036              ",
    "\221\013                        \221\036              ",
    "\221\017------------------------     -------- ",
    "\221\014          OVERALL SCORE:\221\036              ",
    "\221\007                   RANK:\221\036              "
};

/* "Would ya like ta play again? (Y/N)".  Returns 1 for yes, 0 for no. */
int askPlayAgain()
{
    char c;
    register int r;

    setColor(15, 0, 0);
    drawBox("\xc9\xcd\xbb\xba\xbc\xcd\xc8\xba", 29, 10, 51, 15);
    clearBox(30, 11, 50, 14);
    printColored("\x91\014Would ya like ta", 32, 12);
    printColored("play again? (Y/N)", 32, 13);
    setCursor(1);
    gotoxy(40, 14);
    while (1) {
        c = toupper(getch());
        if (c == 'Y') {
            r = 1;
            break;
        }
        if (c == 'N') {
            r = 0;
            break;
        }
    }
    return r;
}

/* You died: show the death picture for a few seconds and wait for a key. */
void showDeathScreen()
{
    register int x, y;

    delay(2000);
    x = 23;
    y = 5;
    puttext(x, y, x + 34, y + 14, deathScreen);
    delay(5000);
    flushKeys();
    getch();
}

/* =========================================================================
 *  END OF THE GAME
 * =========================================================================
 *
 * Reached when your status hits 300.  Plays a victory jingle, draws the
 * money stacks and guns you own as pictures, builds the score table in
 * endText[], computes your score and rank and lets you scroll through it
 * with the arrow keys until you press ESC.
 *
 * Score = (e^(net worth * 6.9315e-8) * 75000, capped at 700000)
 *       + (days-based bonus 104e9 / days^2.12, capped at 500000)
 *       + (percent of good deals)^8.14 * 2.48e-11
 *       + 1500 per gun (gun type * number of guns)
 * and the rank is the number of thresholds in thr[] that it reaches.
 * (6.9315e-8 is ln(2)/10,000,000: net worth doubles the base every 10 million.) */
void showEndGame()
{
    int i, j, k, x, row, n, d, m, pct, rank;
    char *titles[12] = { "Loser!", "Dopehead", "Peddler", "Pusher", "Gangster", "Dealer",
                         "Trafficker", "Racketeer", "Marketeer", "Entrepreneur", "DRUGLORD", "OVERLORD" };
    long thr[11] = { 250000L, 500000L, 600000L, 700000L, 800000L, 850000L,
                     900000L, 950000L, 1000000L, 1100000L, 1250000L };
    char msg[11] = "DRUGLORD!!!";
    int notes[16] = { 100, 150, 500, 400, 100, 75, 25, 500, 1000, 1500, 1000, 750, 500, 200, 100, 50 };
    char bigbuf[1121];
    char key;
    char s[10];
    long score;
    long p2;
    int guns;
    double ex;
    register int si, di;

    delay(1000);
    clearBox(1, 17, 80, 25);
    /* Flash "DRUGLORD!!!" five times in different colours with a little tune.
     */
    for (i = 1; i < 6; i++) {
        clearBox(15, 20, 65, 20);
        setColor(i + 9, 0, i % 2 == 1);
        for (j = 0; j < 22; j++) {
            gotoxy(j / 2 * 5 + 15, 20);
            putch(msg[j / 2]);
            if (j < 16) {
                sound(notes[j]);
                delay(20);
            }
        }
        nosound();
        delay(500);
    }
    /* Draw the end scene, then one gun picture for each gun owned (max 10). */
    puttext(1, 1, 80, 25, endScene);
    if (g.numGuns < 10) guns = g.numGuns;
    else guns = 10;
    for (i = 1; i <= guns; i++) {
        if (i <= 4) {
            k = (i - 1) / 2 * 11 + (i % 2 == 0) * 5 + 55;
            si = (i % 2 == 0) * 3 + 2;
            puttext(k, si, k + 7, si + 2, gunSprite);
        } else {
            k = (i - 5) / 2 * 11 + !(i % 2) * 5 + 1;
            si = (i % 2 == 0) * 3 + 2;
            puttext(k, si, k + 7, si + 2, gunSprite);
        }
    }
    /* Draw stacks of money: each step of $100000 adds a block, each $2000000
     * starts a new layer, accompanied by rising beeps.
     */
    n = (int)((g.cash + g.bank - 1) / 2000000L) + 1;
    for (si = 0; si < 4; si++) {
        row = si + 11;
        for (k = 0; k < 5; k++) {
            x = k * 7 + 18 - si * 2;
            d = 15 - si * 3 - k;
            if (d < 0) d = 0;
            for (m = 0; m < n; m++) {
                if ((long)(si * 5 + k + 1) * 100000L + (long)m * 2000000L > g.cash) break;
                puttext(x, row - m, x + 4, row - m, barSprite);
                sound(m * 500 + 1000);
                delay(d + 5);
                sound(m * 500 + 3000);
                delay(d);
            }
        }
    }
    nosound();
    /* Fill the numbers into the score table (right-aligned, column 0x29). */
    pct = g.goodDeals * 100 / g.totalDeals;
    ltoa(g.cash, s, 10);
    di = strlen(s);
    stpcpy(endText[6] + (0x29 - di), s);
    ltoa(g.bank, s, 10);
    di = strlen(s);
    stpcpy(endText[7] + (0x29 - di), s);
    ltoa(g.debt, s, 10);
    di = strlen(s);
    stpcpy(endText[8] + (0x29 - di), s);
    itoa(g.goodDeals, s, 10);
    di = strlen(s);
    stpcpy(endText[9] + (0x29 - di), s);
    itoa(g.totalDeals, s, 10);
    di = strlen(s);
    stpcpy(endText[10] + (0x29 - di), s);
    itoa(pct, s, 10);
    di = strlen(s);
    stpcpy(endText[11] + (0x28 - di), s);
    strcat(endText[11], "%");
    di = strlen(gunName[g.gunType]);
    stpcpy(endText[12] + (0x17 - di), gunName[g.gunType]);
    stpcpy(endText[12] + 0x17, "'s:\221\036            ");
    itoa(g.numGuns, s, 10);
    di = strlen(s);
    stpcpy(endText[12] + (0x29 - di), s);
    itoa(g.totalDays, s, 10);
    di = strlen(s);
    stpcpy(endText[13] + (0x29 - di), s);
    /* Now the score (see the formula above). */
    ex = exp((double)(g.cash + g.bank - g.debt) * 6.9315e-08);
    score = (long)ex * 75000L;
    if (score > 700000L) score = 700000L;
    p2 = (long)(pow((double)g.totalDays, -2.12) * 104000000000.0);
    if (p2 > 500000L) p2 = 500000L;
    score += p2;
    score += (long)(pow((double)pct, 8.14) * 2.48e-11);
    score += (long)(g.gunType * g.numGuns) * 1500L;
    ltoa(score, s, 10);
    di = strlen(s);
    stpcpy(endText[16] + (0x29 - di), s);
    /* Find the rank title that goes with the score. */
    for (rank = 0; rank < 10; rank++) {
        if (thr[rank] > score) break;
    }
    di = strlen(titles[rank]);
    stpcpy(endText[17] + (0x29 - di), titles[rank]);
    setColor(9, 0, 0);
    drawBox("\311\315\273\272\274\315\310\272", 10, 18, 55, 25);
    printColored(" \221\034\031 ", 50, 25);
    printColored("\221\011[\221\033ESC\221\011] \221\033Finished ", 12, 25);
    setColor(14, 0, 0);
    for (si = 0; si < 5; si++)
        printColored(endText[si], 15, si + 19);
    /* Scroll loop: UP/DOWN arrows (extended keys 0x48/0x50) move the text,
     * ESC leaves.
     */
    si = 0;
    while (1) {
        key = getch();
        setColor(14, 0, 0);
        if (key == 0) {
            key = getch();
            if (key == 0x48 && si > 0) {
                si--;
                gettext(15, 19, 55, 23, bigbuf);
                puttext(15, 20, 55, 24, bigbuf);
                printColored(endText[si], 15, 19);
                playSound(20);
            }
            if (key == 0x50 && si < 12) {
                si++;
                gettext(15, 20, 55, 24, bigbuf);
                puttext(15, 19, 55, 23, bigbuf);
                printColored(endText[si + 5], 15, 24);
                playSound(20);
            }
            if (si > 0) printColored(" \221\014\030 ", 50, 18);
            else printColored("\221\011\315\315\315", 50, 18);
            if (si < 10) printColored(" \221\034\031 ", 50, 25);
            else printColored("\221\011\315\315\315", 50, 25);
        }
        if (key == 0x1b) break;
    }
}

/* Advance the calendar one day.  The loan grows 5% a day and the bank
 * account earns 0.0192% a day.  Years are two digits (wrap at 100). */
void nextDay()
{
    g.loanDays--;
    g.day++;
    g.totalDays++;
    if (daysInMonth[g.month - 1] < g.day) {
        g.day = 1;
        g.month++;
        if (g.month > 12) {
            g.month = 1;
            g.year++;
            if (g.year == 100)
                g.year = 0;
        }
    }
    g.debt = g.debt * 1.05;
    g.bank = g.bank * 1.000192;
}

/* Title screen.  First measures the computer speed by counting how often the
 * title can be drawn in one BIOS clock tick -> g.delayTime, which keeps
 * animations the same speed on fast and slow PCs.  Then the title art is
 * revealed in random order, block by block, until it is done or a key
 * is pressed. */
void showIntro()
{
    char ch;
    int prog;
    int count;
    int step;
    int round;
    long t0;
    int limit;
    int toggle;
    register int x, y;

    round = 0;
    setCursor(0);
    clrscr();
    setColor(14, 0, 0);
    fixForMono(titleArt);
    count = 0;
    t0 = biostime(0, 0L);
    while (biostime(0, 0L) == t0)
        ;
    t0 = biostime(0, 0L);
    while (biostime(0, 0L) == t0) {
        puttext(10, 5, 71, 10, titleArt);
        count++;
    }
    if (count == 0) count = 1;
    g.delayTime = 31 - 55 / count;
    if (g.delayTime < 0) g.delayTime = 0;
    printColored("\221\012Version v2.01", 35, 13);
    printColored("\221\001\020\221\011\020\221\013\020   \221\036UNREGISTERED VERSION   \221\013\021\221\011\021\221\001\021", 24, 16);
    printColored("\221\014Written by Phil Erwin", 30, 19);
    printColored("Copyright 1991-93  FantasyWare Inc.", 23, 20);
    sound(0);
    delay(500);
    nosound();
    limit = 441;
    toggle = 1;
    for (; round < 10; round++) {
        prog = 0;
        toggle = !toggle;
        setColor(rand() % 15, 0, toggle);
        x = 0;
        y = 0;
        step = rand() % 6 * 62 + 13;
        while (prog < limit) {
            x += step;
            if (x >= 62) {
                do {
                    x -= 62;
                    y++;
                    if (y > 6) y = 0;
                } while (x >= 62);
            }
            ch = titleArt[y][x * 2];
            if (ch == (char)0xb1) {
                gotoxy(x + 10, y + 5);
                cputs("\261");
                delay(5);
                prog++;
            }
            if (kbhit()) {
                prog = 0x2bd;
                round = 11;
            }
        }
    }
}

/* Set up a new game: show "Initializing...", clear inventory, roll starting
 * prices, city and date, start with $500 and a 10-item coat. */
void newGame()
{
    register int i;

    setCursor(0);
    clearBox(28, 21, 53, 25);
    setColor(14, 0, 1);
    drawBox("\xc9\xcd\xbb\xba\xbc\xcd\xc8\xba", 30, 22, 51, 24);
    setColor(141, 0, 0);
    printColored("Initializing...", 34, 23);
    for (i = 0; i < 8; i++) {
        g.inventory[i] = 0;
        g.priceBias[i] = 1.0;
        g.avgCost[i] = 0;
    }
    for (i = 0; i < 10; i++)
        g.eventDone[i] = 0;
    generatePrices(&g.price[0], &g.price[1], &g.price[2], &g.price[3], &g.price[4], &g.price[5], &g.price[6], &g.price[7]);
    g.city = rand() % 8;
    g.cash = 500L;
    g.holdSize = 10;
    g.bank = 0L;
    g.debt = 0L;
    g.hasLoan = 0;
    g.numGuns = 0;
    g.gunType = 0;
    g.status = 1;
    g.health = 100;
    g.dead = 0;
    g.sharkAngry = 0;
    g.copChase = 0;
    g.beatings = 0;
    g.totalDays = 0;
    g.month = rand() % 12 + 1;
    g.day = rand() % 28 + 1;
    g.year = rand() % 6 + 86;
    fixForMono(farewellScreen);
    fixForMono(gunSprite);
    fixForMono(barSprite);
    fixForMono(deathScreen);
    fixForMono(endScene);
    delay(1000);
}

/* On a monochrome display, remap every colour attribute byte (the odd bytes)
 * of a text-mode picture to white or grey (keeping the blink bit). */
void fixForMono(s)
char *s;
{
    int len;
    register int i;

    if (g.monochrome == 1) {
        len = strlen(s);
        for (i = 1; i < len; i += 2) {
            if (s[i] <= 7)
                s[i] = (s[i] & 0x80) | 7;
            else
                s[i] = (s[i] & 0x80) | 0xf;
        }
    }
}

/* =========================================================================
 *  MAIN PROGRAM
 * ========================================================================= */
void main()
{
    int v;
    unsigned long lamt;
    char inbuf[81];
    char key;
    int amount;
    int running;
    int days;
    long maxbuy;
    int bonus;
    int oldstatus;
    char bigbuf[1280];
    register int si, di;

    /* Start-up: seed the random generator, detect monochrome (modes 2 and 7)
     * and refuse to run in 40 column modes.
     */
    srand((unsigned)time(0));
    gettextinfo(&g.textInfo);
    if (g.textInfo.currmode == 7 || g.textInfo.currmode == 2) g.monochrome = 1;
    else g.monochrome = 0;
    if (g.textInfo.currmode == 0 || g.textInfo.currmode == 1) {
        cputs("DRUGLORD requires 80 column mode.\r\n");
        exit(0);
    }
    showIntro();
    /* Outer loop: one pass per game; ends when the player does not want to play again.
     */
    running = 1;
    do {
        newGame();
        drawMainScreen();
        drawStatus();
        /* One pass of this loop = one player action, followed by a day passing. */
        while (1) {
            setCursor(0);
            clearBox(1, 17, 80, 25);
            printColored("\221\n(\221\034Q\221\n)uit de game", 67, 17);
            window(15, 20, 80, 25);
            printColored("\221\n(\221\034B\221\n)uy Drugs,                       (\221\034V\221\n)isit de Bank,\r\n(\221\034S\221\n)ell Drugs,                      Go to de (\221\034H\221\n)ospital,\r\n(\221\034J\221\n)et to another city,             See de (\221\034L\221\n)oan Shark", 15, 20);
            window(1, 1, 80, 25);
            printColored("\221\007Here's a list of yer possible plans, dude:", 20, 18);
            printColored("\221\007Make up your mind \221\011\020 ", 30, 25);
            /* 300 status points = you won; leave to the end screen. */
            if (g.status >= 300) break;
            setColor(15, 0, 0);
            setCursor(1);
            do {
                flushKeys();
                key = toupper(getch());
            } while (key != 'B' && key != 'S' && key != 'J' && key != 'V' && key != 'L' && key != 'H' && key != 'Q');
            setCursor(0);
            clearBox(1, 17, 80, 25);
            /* Execute the chosen command. */
            switch (key) {
            /* ---- Jet to another city: pick 1-8, pay the fare, and a "slide" animation
             * scrolls the lower panel while the new prices are rolled.
             */
            case 'J':
                drawCityMenu();
                do {
                    gotoxy(1, 25);
                    clreol();
                    printColored("\221\nWhere ya wanna go? ", 1, 25);
                    g.escPressed = readField(2, 1, inbuf);
                    setCursor(0);
                    if (g.escPressed == 1) break;
                    si = atol(inbuf);
                    if (si < 1 || si > 8) playSound(2);
                    if (si - 1 == g.city) {
                        printColored("\221\rAre you having \221\037flashbacks? \221\rThat's where you are now!", 28, 25);
                        playSound(3);
                        si = 0;
                        delay(500);
                    }
                } while (si < 1 || si > 8);
                setCursor(0);
                if (g.escPressed == 1) continue;
                if ((long)travelCost(g.city, si - 1) > g.cash) {
                    printColored("\221\034Start flappin' yer arms, cause you ain't got de dough!", 25, 25);
                    playSound(3);
                    delay(2000);
                    continue;
                }
                g.cash -= travelCost(g.city, si - 1);
                g.city = si - 1;
                g.marketNews = 0;
                generatePrices(&g.price[0], &g.price[1], &g.price[2], &g.price[3], &g.price[4], &g.price[5], &g.price[6], &g.price[7]);
                gotoxy(30, 25);
                setColor(15, 0, 0);
                for (v = 79; v > 1; v -= 2) {
                    gettext(3, 19, v, 22, bigbuf);
                    puttext(1, 19, v - 2, 22, bigbuf);
                    sound((79 - v) * 20 + 1000);
                    delay(g.delayTime);
                }
                nosound();
                drawStatus();
                for (v = 0; v < 10; v++) g.eventDone[v] = 0;
                break;
            /* ---- Buy drugs.  Average cost per drug (avgCost) is the mean of the old
             * average and today's price; it is used later to score profits.
             */
            case 'B':
                drawDrugMenu();
                do {
                    printColored("\221\004What kinda drugs are\r\nya interested in? ", 1, 19);
                    setColor(14, 0, 0);
                    g.escPressed = readField(1, 0, inbuf);
                    if (g.escPressed == 1) break;
                    di = atol(inbuf);
                    if (di < 1 || di > 8) playSound(2);
                } while (di < 1 || di > 8);
                if (g.escPressed == 1) continue;
                di--;
                maxbuy = g.cash / g.price[di];
                printColored("\221\004How many do ya wanna buy?\r\n(", 1, 22);
                setColor(11, 0, 1);
                cprintf("%ld", maxbuy);
                printColored("\221\004 max.)", 99, 99);
                setColor(14, 0, 0);
                do {
                    gotoxy(27, 22);
                    g.escPressed = readField(5, 0, inbuf);
                    if (g.escPressed == 1) break;
                    amount = atol(inbuf);
                } while (amount < 0 || inbuf[0] == 0);
                setCursor(0);
                if (g.escPressed == 1) continue;
                if ((long)amount > maxbuy) {
                    printColored("\221\035Hey man, if ya don't got enough\r\n  cash, don't come 'round here!", 3, 24);
                    playSound(3);
                    delay(1000);
                    continue;
                }
                if (amount > g.holdFree) {
                    printColored("\221\nDude, you can't hold that much!\r\n  Everyone's got a limit, right?", 3, 24);
                    playSound(3);
                    delay(3000);
                    continue;
                }
                if (amount > 0) {
                    printColored("\221\016* * *   \221\037S O L D ! ! !   \221\016* * *", 1, 25);
                    playSound(4);
                    delay(2000);
                    g.inventory[di] += amount;
                    g.cash -= (long)amount * g.price[di];
                    if (g.avgCost[di] == 0)
                        g.avgCost[di] = g.price[di];
                    else
                        g.avgCost[di] = (g.price[di] + g.avgCost[di]) / 2;
                }
                break;
            /* ---- Sell drugs.  If you sell the whole stack the status changes by the
             * profit ratio ((price/avgCost - 1) * 3 * bonus).  "bonus" is the
             * factor 9, 6 or 3 for the three cheapest drugs, 1 for the rest.
             */
            case 'S':
                drawDrugMenu();
                do {
                    printColored("\221\004What kinda drugs do\r\n ya want to unload? ", 1, 19);
                    setColor(14, 0, 0);
                    g.escPressed = readField(1, 0, inbuf);
                    if (g.escPressed == 1) break;
                    di = atol(inbuf);
                    if (di < 1 || di > 8) playSound(2);
                } while (di < 1 || di > 8);
                setCursor(0);
                if (g.escPressed == 1) continue;
                di--;
                if (g.inventory[di] == 0) {
                    printColored("\221\rGive me a break, man, you ain't got\r\nany of that!", 1, 22);
                    playSound(3);
                    delay(1000);
                    continue;
                }
                do {
                    printColored("\221\004How many do ya wanna sell? ", 1, 22);
                    setColor(14, 0, 0);
                    g.escPressed = readField(5, 0, inbuf);
                    if (g.escPressed == 1) break;
                    amount = atol(inbuf);
                } while (amount < 0 || inbuf[0] == 0);
                setCursor(0);
                if (g.escPressed == 1) continue;
                if (g.inventory[di] < amount) {
                    printColored("\221\025I happen to know that you ain't that\r\nmuch on ya, so quit trying to burn me!", 1, 24);
                    playSound(3);
                    delay(3000);
                    continue;
                }
                if (amount > 0) {
                    printColored("\221\036You got yerself a deal, dude!", 1, 25);
                    playSound(4);
                    delay(2000);
                    if (g.inventory[di] - amount == 0) {
                        if (di < 3) bonus = (3 - di) * 3;
                        else bonus = 1;
                        oldstatus = g.status;
                        g.status += (int)(((double)g.price[di] / (double)(unsigned long)g.avgCost[di] - 1.0) * 3.0 * bonus);
                        g.totalDeals++;
                        if (g.status > oldstatus) g.goodDeals++;
                        g.avgCost[di] = 0;
                    }
                    g.inventory[di] -= amount;
                    g.cash += (long)amount * g.price[di];
                }
                break;
            /* ---- Visit the bank: Deposit or Withdraw.  ESC (27) cancels. */
            case 'V':
                printColored("\221\017You arrive at the First World Bank of America.\r\n\n               \221\nWould ya like to (\221\034D\221\n)eposit  some moola,\r\n                             or (\221\034W\221\n)ithdraw some moola? ", 20, 18);
                setCursor(1);
                do {
                    si = getch();
                    si = toupper(si);
                } while (si != 'D' && si != 'W' && si != 27);
                if (si == 27) continue;
                switch (si) {
                case 'D':
                    printColored("\221\013How much 'ya gonna rathole? ", 5, 23);
                    break;
                case 'W':
                    printColored("\221\013How much 'ya gonna pull out? ", 5, 23);
                    break;
                }
                setColor(15, 0, 0);
                g.escPressed = readField(10, 0, inbuf);
                setCursor(0);
                if (g.escPressed == 1) continue;
                lamt = atol(inbuf);
                if (lamt <= 0) continue;
                switch (si) {
                /* Deposit. */
                case 'D':
                    if (lamt > g.cash) {
                        printColored("\221\035You ain't got that much to deposit!", 25, 25);
                        playSound(3);
                        delay(1000);
                        continue;
                    } else {
                        printColored("\221\036Done!", 37, 25);
                        playSound(2);
                        delay(2000);
                        g.cash -= lamt;
                        g.bank += lamt;
                        drawStatus();
                    }
                    continue;
                /* Withdraw. */
                case 'W':
                    if (lamt > g.bank) {
                        printColored("\221\035You ain't got that much to pull out!", 25, 25);
                        playSound(3);
                        delay(1000);
                    } else {
                        printColored("\221\036Done!", 37, 25);
                        playSound(2);
                        delay(2000);
                        g.cash += lamt;
                        g.bank -= lamt;
                        drawStatus();
                    }
                    continue;
                }
                /* The two bank cases above always `continue`, so the code never runs
                 * through into the loan shark case; the original compiler simply laid the
                 * cases out this way, so we reproduce it.
                 */
                if (g.escPressed == 1) continue;
            case 'L':
                printColored("\221\013Ya set foot in the 'sharks office.  You've been in locker rooms that smelled\r\nbetter than dis place!  His boys are lookin' tough in their nice goon suits,\r\nbut you don't pay dem no never-mind, cause the head-honcho, Guido is talkin...\r\n\n                    \221\nHe say, you wanna (\221\034B\221\n)orrow some cash,\r\n                      or do you wanna (\221\034P\221\n)ay me back? ", 1, 17);
                setCursor(1);
                do {
                    si = getch();
                    si = toupper(si);
                } while (si != 'B' && si != 'P' && si != 27);
                setCursor(0);
                if (si == 27) continue;
                switch (si) {
                /* Borrow: refused if the shark is angry, if you have no status, below
                 * $1000, or if the total debt would be too large for your status
                 * (max debt = status * $10000).  The loan time (days) shrinks as the
                 * debt grows: log(status*10000 / total debt) / log(1.2) + 4.
                 */
                case 'B':
                    if (g.sharkAngry == 1) {
                        printColored("\221\032\"Man, yer pushin' yer luck luck! I ought'ta kill you just for askin'!!\"", 5, 24);
                        delay(2000);
                        break;
                    }
                    if (g.status <= 0) {
                        printColored("\221\n\"Look, whoever you are, you're \221\037NOBODY \221\nto me.  Get outta here!\"", 5, 24);
                        delay(2000);
                        break;
                    }
                    clearBox(1, 21, 80, 25);
                    printColored("\221\nHow much ya wanna borrow? ", 10, 22);
                    g.escPressed = readField(10, 0, inbuf);
                    setCursor(0);
                    if (g.escPressed == 1) continue;
                    lamt = atol(inbuf);
                    if (lamt <= 0) continue;
                    if (lamt < 1000) {
                        printColored("\221\034\"Sorry, bud, but I got a \221\036$1000 \221\034minimum.  Got no time for penny-ante stuff!\"", 1, 25);
                        playSound(3);
                        delay(1000);
                        continue;
                    }
                    if (lamt / 10000 >= g.status) {
                        printColored("\221\034\"No deal, too big a risk for me! I'm a businessman, ya'know!\"", 10, 25);
                        playSound(3);
                        delay(1000);
                        continue;
                    }
                    days = (int)((log((double)g.status * 10000.0) - log((double)(g.debt + lamt))) / 0.182321556794) + 4;
                    if ((lamt + g.debt) / 10000 > g.status || (days >= g.loanDays && g.hasLoan == 1)) {
                        printColored("\221\013\"Look, you already owe me ", 1, 24);
                        setColor(4, 0, 1);
                        cprintf("%ld ", g.debt);
                        setColor(11, 0, 0);
                        cputs("bucks, I'm not givin ya THAT much more!!!");
                        printColored("\221\013Christ, what do I look like, Santi-Claus?\"", 20, 25);
                        playSound(3);
                        delay(2000);
                        continue;
                    } else {
                        printColored("\221\r\"Alright, I'll loan ya de money, but if you welch, my boys will come lookin\r\nfor ya! I expect to see you here to pay me in full in ", 1, 24);
                        g.debt += lamt;
                        g.cash += lamt;
                        g.hasLoan = 1;
                        g.loanDays = days;
                        setColor(4, 0, 1);
                        cprintf("%d", g.loanDays - 1);
                        printColored("\221\r days.\"", 99, 99);
                        playSound(2);
                        delay(2000);
                        break;
                    }
                /* Pay back all or part of the debt; paying it in full clears
                 * the deadline and the shark's anger.
                 */
                case 'P':
                    if (g.debt == 0) {
                        printColored("\221\013\"My books show dat you don't owe me nut'in... would youse like to take out\r\na little loan?\"", 1, 24);
                        playSound(3);
                        delay(2000);
                        continue;
                    }
                    clearBox(1, 21, 80, 25);
                    printColored("\221\nHow much ya wanna pay back? ", 10, 22);
                    g.escPressed = readField(10, 0, inbuf);
                    setCursor(0);
                    if (g.escPressed == 1) continue;
                    lamt = atol(inbuf);
                    if (lamt <= 0) continue;
                    if (lamt > g.cash) {
                        printColored("\221\034'fraid you don't got that in cash, my man.", 20, 25);
                        playSound(3);
                        delay(1000);
                        continue;
                    }
                    if (lamt >= g.debt) g.debt = 0;
                    else g.debt -= lamt;
                    g.cash -= lamt;
                    if (g.debt > 0) {
                        printColored("\221\013\"OK, but you still owe me ", 20, 24);
                        setColor(12, 0, 1);
                        cprintf("%ld ", g.debt);
                        printColored("\221\013buck", 99, 99);
                        if (g.debt > 1) cputs("s!\"");
                        else cputs("!\"");
                        playSound(3);
                        delay(2000);
                        break;
                    } else {
                        printColored("\221\013\"Paid in full? Lucky for you, eh boys? Anytime yous need a loan, just come\r\nto me, I'll take care of ya...\"", 1, 24);
                        g.loanDays = 0;
                        g.hasLoan = 0;
                        g.sharkAngry = 0;
                        g.beatings = 0;
                        playSound(2);
                        delay(2000);
                        break;
                    }
                }
                break;
            /* ---- Hospital: costs $10 per missing hit point and takes 1 day per $15
             * (bonus is reused here as the bill).
             */
            case 'H':
                if (g.health == 100) {
                    printColored("\221\017Ya have de doc check yous out, but aside from yer bunions\r\n              on yer feet, youse is fit as a fiddle!", 15, 19);
                    pressAnyKey();
                    continue;
                }
                bonus = (100 - g.health) * 10;
                printColored("\221\017You drag yer sorry butt into de loc'l Hospital.  The docs check ye\r\n   out, and come to the conclusion that it'll cost ", 4, 19);
                setColor(10, 0, 1);
                cprintf("%d", bonus);
                printColored("\221\017 dollars to fix ya up.", 99, 99);
                printColored("\221\rDo ya sign the consent forms? (Y/N) \020 ", 5, 22);
                setCursor(1);
                do {
                    key = toupper(getch());
                } while (key != 'Y' && key != 'N' && key != 27);
                setCursor(0);
                if (key == 'Y') {
                    if ((long)bonus > g.cash) {
                        printColored("\221\034You don't have enough dinero up front to foot de bill!", 12, 24);
                        playSound(3);
                        pressAnyKey();
                    } else {
                        g.cash -= bonus;
                        printColored("\221\016OK, yer laid up for ", 5, 24);
                        setColor(12, 0, 1);
                        cprintf("%d", bonus / 15);
                        printColored("\221\016 days recoup'ratin', and yous feel good as new!", 99, 99);
                        playSound(2);
                        for (v = 1; v < bonus / 15; v++)
                            nextDay();
                        g.health = 100;
                        pressAnyKey();
                    }
                }
                break;
            /* ---- Quit the game (ask for confirmation). */
            case 'Q':
                printColored("\221\007Are ya sure ya wanna \221\026chicken out \221\007an' \221\024QUIT? \221\007(Y/N) ", 15, 20);
                setCursor(1);
                do {
                    si = toupper(getch());
                } while (si != 'Y' && si != 'N');
                if (si == 'N') continue;
                running = 0;
                break;
            }
            /* Every action ends the day: update the calendar and the display, then
             * see what the day brings.
             */
            nextDay();
            drawStatus();
            if (g.dead || !running) break;
            /* Rare events for high rollers: trenchcoat (7) or gun smuggler (8). */
            if (rand() % 10 == 0 && rand() % (g.status / 20 + 1) > 4) {
                if (rand() % 2 == 0) randomEvent(7);
                else randomEvent(8);
            }
            /* The cops: always after a refused blackmail, otherwise sometimes
             * when your status is 100 or more.
             */
            if (g.copChase == 1 || (g.status >= 100 && rand() % 400 < g.status && rand() % 8 == 0))
                randomEvent(0);
            if (g.dead || !running) break;
            /* One in five days has an ordinary random event. */
            if (rand() % 5 == 0)
                randomEvent(rand() % 10);
            if (g.dead || !running) break;
            /* Loan overdue: the shark's thugs beat you up (25 damage per beating so
             * far, one more each time).  You get a week to pay, or die.
             */
            if (g.loanDays < 0 && g.hasLoan == 1) {
                clearBox(1, 17, 80, 25);
                printColored("\221\013You're ambushed in a dark alley by Guido's thugs, must be 'bout ten guys!\r\nThe boss-man say \221\027\"Oh, man, you should'na missed de deadline, now me and\r\nboys are gonna have to break your legs! Maybe next time you'll learn, eh?\"\221\013\r\nDey take yous from all directions, you ain't gotta chance...\r\nOne steps out with a \221\026baseball bat \221\013and...\r\n", 1, 17);
                delay(3000);
                g.beatings++;
                for (v = 0; v < g.beatings; v++) {
                    clearBox(1, 23, 80, 23);
                    delay(500);
                    printColored("\221\014* * *    \221\024C R A C K ! !   \221\014* * *", 25, 23);
                    g.health -= 25;
                    g.sharkAngry = 1;
                    drawStatus();
                    setCursor(0);
                    playSound(3);
                    delay(1500);
                }
                if (g.health > 0) {
                    printColored("\221\016\"Oh, one more 'ting... you got a week to come up with dinero, or we'll\r\n be back, comprende?\"", 1, 24);
                    delay(3000);
                    g.loanDays = 7;
                } else
                    g.dead = 1;
                pressAnyKey();
            }
            if (g.dead || !running) break;
            drawStatus();
        }
        /* Game over: died, or reached the winning status. */
        if (running) {
            if (g.dead) showDeathScreen();
            else showEndGame();
        }
        running = askPlayAgain();
    } while (running);
    /* Goodbye screen and exit. */
    clrscr();
    puttext(3, 2, 77, 20, farewellScreen);
    printColored("\221\036Thank you for playing DrugLord.", 25, 22);
    printColored("Send For The Registered Version Today!", 22, 23);
    setCursor(1);
    gotoxy(1, 24);
    exit(0);
}

