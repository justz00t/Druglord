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
 * Game idea (a "Dope Wars" clone): you are a street dealer with $500.
 * Buy drugs where they are cheap, sell where they are expensive, travel
 * between 8 cities, borrow from the loan shark, dodge the cops and muggers,
 * and keep going until you have 300 status points ("status", your reputation
 * as a dealer).  Then the game ends and you are ranked by a score.
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

/* ---- Colour codes inside strings -----------------------------------------
 * printColored() understands an escape inside a string: the byte 0x91
 * followed by a colour number.  Numbers 16-31 mean "colour minus 16, and
 * blinking".  These macros hide the raw bytes, so  C_LRED "Hello"  prints
 * "Hello" in light red and  C_BLINK_YELLOW "Hi"  prints it blinking yellow.
 * (They expand to ordinary string pieces that the compiler joins together,
 * so the program is unchanged.)
 */
#define C_BLUE             "\221\001"
#define C_RED              "\221\004"
#define C_MAGENTA          "\221\005"
#define C_BROWN            "\221\006"
#define C_LGRAY            "\221\007"
#define C_LBLUE            "\221\011"
#define C_LGREEN           "\221\012"
#define C_LCYAN            "\221\013"
#define C_LRED             "\221\014"
#define C_LMAGENTA         "\221\015"
#define C_YELLOW           "\221\016"
#define C_WHITE            "\221\017"
#define C_BLINK_RED        "\221\024"
#define C_BLINK_MAGENTA    "\221\025"
#define C_BLINK_BROWN      "\221\026"
#define C_BLINK_LGRAY      "\221\027"
#define C_BLINK_LBLUE      "\221\031"
#define C_BLINK_LGREEN     "\221\032"
#define C_BLINK_LCYAN      "\221\033"
#define C_BLINK_LRED       "\221\034"
#define C_BLINK_LMAGENTA   "\221\035"
#define C_BLINK_YELLOW     "\221\036"
#define C_BLINK_WHITE      "\221\037"

/* Third argument of setColor(): on a monochrome screen BOLD selects high
 * intensity text.  On a colour screen it has no effect. */
#define NORMAL 0
#define BOLD   1

/* ---- Game constants ------------------------------------------------------ */
#define NUM_DRUGS    8       /* number of different drugs */
#define NUM_CITIES   8       /* number of cities */
#define NUM_EVENTS   10      /* number of random events (see EV_ below) */
#define MAX_HEALTH   100     /* full health */
#define WIN_STATUS   300     /* status points that end the game */
#define MAX_COAT     300     /* biggest trenchcoat that can be bought */
#define START_CASH   500L    /* money at the start of a game */
#define START_COAT   10      /* starting trenchcoat capacity */

/* Drug numbers (index into drugName[], inventory[], price[] ...) */
enum { DRUG_COCAINE, DRUG_CRACK, DRUG_HEROIN, DRUG_ACID, DRUG_CRYSTAL,
       DRUG_GRASS, DRUG_SPEED, DRUG_LUDES };

/* Random events (argument of randomEvent()) */
enum { EV_COPS, EV_COPS_SELL, EV_CHEAP_DRUGS, EV_BUST, EV_FACTORY,
       EV_BROWNIES, EV_MUGGER, EV_TRENCHCOAT, EV_GUN_DEALER, EV_BLACKMAIL };

/* Sound effect numbers (argument of playSound()) */
#define SND_BEEP         2   /* short blip: "Done!" or an invalid entry */
#define SND_DENIED       3   /* falling two-tone: refused / not allowed */
#define SND_DEAL         4   /* rising chirps: a deal was made */
#define SND_SIREN        5   /* police siren */
#define SND_NEWS         6   /* rising sweep: news flash */
#define SND_EXTRA        7   /* warbling "EXTRA!" newspaper sound */
#define SND_HIT          8   /* a bullet hits */
#define SND_SHOT_BASE    10  /* + gun type (1-5): the bang of that gun */
#define SND_MUGGER_SHOT  13  /* the mugger's gun */
#define SND_SCROLL       20  /* click when scrolling the final score list */

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
char *drugName[NUM_DRUGS] = {
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
char *cityName[NUM_CITIES] = {
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
char cityCoords[NUM_CITIES][2] = {
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
float cityPriceMult[NUM_CITIES][NUM_DRUGS] = {
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
struct range priceRange[NUM_DRUGS] = {
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
 * original address order.  So that the rest of the code can use plain names
 * (cash, health, ...) like the original, each field is given a #define below.
 */
struct GameState {
    int  marketNews;     /* 1 = a "street news" price event already happened in this city */
    int  beatings;       /* how many times the loan shark's thugs have beaten you up */
    float priceBias[NUM_DRUGS];  /* per-drug price bias added in generatePrices() (1.0 at the start) */
    unsigned avgCost[NUM_DRUGS]; /* average price you paid per drug (0 = none held); used to score profits */
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
    int  inventory[NUM_DRUGS];   /* how much of each drug you carry */
    int  loanDays;       /* days left to repay the loan before the thugs come */
    int  goodDeals;      /* number of sales that made a profit (for the final score) */
    int  picked[NUM_DRUGS];      /* scratch: which drugs a market event already picked */
    char eventDone[NUM_EVENTS];  /* 1 = random event number n already happened in this city */
    int  copChase;       /* 1 = the cops are after you (you refused them earlier) */
    int  delayTime;      /* animation speed, measured at start-up from the BIOS clock */
    int  sharkAngry;     /* 1 = the loan shark won't lend to you any more */
    int  totalDeals;     /* number of times you sold a whole stack of a drug */
    struct text_info textInfo; /* video mode info from gettextinfo() */
    int  totalDays;      /* days played so far */
    long price[NUM_DRUGS];       /* today's street price of each drug */
    int  numGuns;        /* how many guns you own (all of gunType) */
};
struct GameState g;

/* The fields of g, usable like ordinary global variables.  (These macros
 * only stand for "g.<name>"; they do not change the compiled program.) */
#define marketNews   g.marketNews
#define beatings     g.beatings
#define priceBias    g.priceBias
#define avgCost      g.avgCost
#define escPressed   g.escPressed
#define dead         g.dead
#define holdFree     g.holdFree
#define cash         g.cash
#define day          g.day
#define monochrome   g.monochrome
#define month        g.month
#define year         g.year
#define health       g.health
#define bank         g.bank
#define gunType      g.gunType
#define holdSize     g.holdSize
#define status       g.status
#define debt         g.debt
#define city         g.city
#define hasLoan      g.hasLoan
#define lineBuf      g.lineBuf
#define inventory    g.inventory
#define loanDays     g.loanDays
#define goodDeals    g.goodDeals
#define picked       g.picked
#define eventDone    g.eventDone
#define copChase     g.copChase
#define delayTime    g.delayTime
#define sharkAngry   g.sharkAngry
#define totalDeals   g.totalDeals
#define textInfo     g.textInfo
#define totalDays    g.totalDays
#define price        g.price
#define numGuns      g.numGuns

/* =========================================================================
 *  SCREEN AND SOUND HELPERS
 * ========================================================================= */

/* Select the text colour.  fg/bg are the usual 16 PC text colours (the
 * conio.h names such as YELLOW; "+ BLINK" makes the foreground blink).
 * "blink" is really a bold flag and only has an effect on a monochrome
 * display: there everything is white, a non-zero background becomes grey, and
 * a non-zero third argument selects high intensity. */
void setColor(fg, bg, blink)
int fg;
int bg;
char blink;
{
    if (monochrome == 1) {
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
    register int pos, start;

    pos = 0;
    strcpy(buf, s);
    if (x != 99)
        gotoxy(x, y);
    while (buf[pos] != 0) {
        if (buf[pos] == (char)0x91) {
            if (buf[pos + 1] > 15) {
                buf[pos + 1] -= 16;
                blink = 1;
            } else
                blink = 0;
            setColor(buf[pos + 1], BLACK, blink);
            pos += 2;
        }
        start = pos;
        while (buf[pos] != 0 && buf[pos] != (char)0x91)
            pos++;
        save = buf[pos];
        buf[pos] = 0;
        cputs(buf + start);
        buf[pos] = save;
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
    else if (textInfo.currmode == 7)
        _CX = 0x90a;
    else
        _CX = 0x607;
    _AH = 1;
    geninterrupt(0x10);
}

/* Sound effects through the PC speaker.  See the SND_ names near the top.
 * An unknown id just silences the speaker. */
void playSound(id)
int id;
{
    register int i, j;

    switch (id) {
    case SND_BEEP:
        sound(2000); delay(10);
        sound(5000); delay(20);
        sound(500);  delay(30);
        sound(1000); delay(40);
        break;
    case SND_DENIED:
        for (i = 500; i > 200; i -= 100) {
            sound(i);       delay(100);
            sound(i - 200); delay(100);
            nosound();      delay(50);
        }
        break;
    case SND_DEAL:
        for (i = 500; i < 1000; i += 100) {
            sound(i);        delay(10);
            sound(i + 2000); delay(20);
            nosound();       delay(10);
        }
        break;
    case SND_SIREN:
        for (j = 0; j < 6; j++) {
            for (i = 7; i < 15; i++) {
                sound(i * 100); delay(25);
            }
            for (i = 15; i > 7; i--) {
                sound(i * 100); delay(25);
            }
        }
        break;
    case SND_NEWS:
        for (i = 0; i < 5; i++)
            for (j = 0; j < 10; j++) {
                sound(i * 1000 + j * 1000); delay(40);
            }
        break;
    case SND_EXTRA:
        for (i = 100; i > 0; i--) {
            sound(5000 - i * 50); delay(5);
            sound(i * 50 + 5000); delay(5);
        }
        break;
    case SND_HIT:
        sound(2000); delay(30);
        sound(500);  delay(25);
        sound(70);   delay(40);
        break;
    case SND_SHOT_BASE + 1:
        sound(1000); delay(50);
        break;
    case SND_SHOT_BASE + 2:
        sound(500); delay(30);
        sound(200); delay(20);
        break;
    case SND_SHOT_BASE + 3:
        sound(100); delay(30);
        sound(50);  delay(30);
        break;
    case SND_SHOT_BASE + 4:
        for (i = 1; i < 2; i++) {
            sound(150); delay(30);
            sound(100); delay(50);
        }
        break;
    case SND_SHOT_BASE + 5:
        for (i = 1; i < 3; i++) {
            sound(100); delay(20);
            sound(50);  delay(60);
        }
        break;
    case SND_SCROLL:
        sound(800); delay(100);
        break;
    }
    nosound();
}

/* Draw a frame from (x1,y1) to (x2,y2).  chars[] holds the 8 border
 * characters in this order: [0] top-left corner, [1] top edge, [2] top-right
 * corner, [3] left edge, [4] bottom-right corner, [5] bottom edge,
 * [6] bottom-left corner, [7] right edge.  The game uses the double-line set
 * "\xc9\xcd\xbb\xba\xbc\xcd\xc8\xba". */
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
    fillChars(chars[1], x2 - x1 - 1, lineBuf);
    cputs(lineBuf);
    gotoxy(x1 + 1, y2);
    fillChars(chars[5], x2 - x1 - 1, lineBuf);
    cputs(lineBuf);
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

/* Read a line of text from the keyboard at the cursor into buf, in a field
 * "width" characters wide.  ENTER ends the entry (returns 0), ESC cancels it
 * (returns 1; callers store that in escPressed), BACKSPACE deletes.
 * flag == 0: the entry ends by itself when the field is full (used for the
 *            one-digit menu choices).
 * flag != 0: when the field is full, further typing replaces the last
 *            character with a beep; only ENTER ends it. */
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
    fillChars(' ', width, lineBuf);
    cputs(lineBuf);
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
                playSound(SND_BEEP);
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
    gettextinfo(&textInfo);
    window(x1, y1, x2, y2);
    clrscr();
    textmode(textInfo.currmode);
}

/* =========================================================================
 *  MAIN SCREEN DRAWING
 * ========================================================================= */

/* Print the eight drug names in both lists (inventory on the left, street
 * prices in the middle). */
void drawDrugNames()
{
    int i;

    setColor(WHITE, BLACK, NORMAL);
    for (i = 0; i < NUM_DRUGS; i++) {
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

    holdFree = 0;
    setCursor(0);
    drawDrugNames();
    for (i = 0; i < NUM_DRUGS; i++) {
        gotoxy(16, i + 7);
        if (inventory[i] > 0) setColor(YELLOW, BLACK, BOLD);
        else setColor(LIGHTGRAY, BLACK, NORMAL);
        cprintf("%6d", inventory[i]);
        gotoxy(38, i + 7);
        setColor(LIGHTRED, BLACK, BOLD);
        cprintf("%10ld", price[i]);
        holdFree += inventory[i];
    }
    setColor(YELLOW, BLACK, BOLD);
    holdFree = holdSize - holdFree;
    gotoxy(63, 4);
    cprintf("%-15s", cityName[city]);
    gotoxy(63, 5);
    cprintf("%-6d (%d max)", holdFree, holdSize);
    gotoxy(63, 7);
    cprintf("$%-10ld", cash);
    gotoxy(63, 8);
    cprintf("$%-10ld", bank);
    gotoxy(63, 9);
    cprintf("$%-10ld", debt);
    gotoxy(74, 9);
    if (hasLoan == 1) {
        cputs("(   )");
        if (loanDays <= 2) setColor(LIGHTRED + BLINK, BLACK, BOLD);
        gotoxy(75, 9);
        cprintf("%3d", loanDays);
        setColor(YELLOW, BLACK, BOLD);
    } else
        cputs("     ");
    gotoxy(63, 11);
    cprintf("%-6d", numGuns);
    gotoxy(63, 12);
    cprintf("%-13s", gunName[gunType]);
    gotoxy(63, 14);
    cprintf("%-6d", status);
    gotoxy(63, 15);
    if (health <= 25) setColor(RED + BLINK, BLACK, BOLD);
    cprintf("%-4d", health);
    setColor(YELLOW, BLACK, BOLD);
    gotoxy(12, 3);
    cprintf("%#02d/%#02d/%#02d", month, day, year);
}

/* Draw the static part of the main screen: clear, copy the 80x16 template
 * (mainScreenTop) to the screen and add the coloured labels on top of it. */
void drawMainScreen()
{
    setCursor(0);
    setColor(LIGHTGRAY, BLACK, NORMAL);
    clrscr();
    puttext(1, 1, 80, 16, mainScreenTop);
    setColor(YELLOW, BLACK, NORMAL);
    printColored(label[0], 24, 2);
    printColored(label[0], 52, 2);
    setColor(LIGHTBLUE, BLACK, BOLD);
    printColored(label[1], 32, 2);
    setColor(LIGHTCYAN, BLACK, NORMAL);
    printColored(label[2], 6, 3);
    setColor(BROWN, BLACK, BOLD);
    printColored(label[3], 6, 5);
    setColor(RED, BLACK, BOLD);
    printColored(label[4], 30, 5);
    setColor(BROWN, BLACK, NORMAL);
    printColored(label[5], 53, 4);
    printColored(label[6], 57, 5);
    setColor(LIGHTGREEN, BLACK, NORMAL);
    printColored(label[7], 57, 7);
    printColored(label[8], 54, 8);
    printColored(label[9], 54, 9);
    setColor(LIGHTMAGENTA, BLACK, NORMAL);
    printColored(label[10], 52, 11);
    printColored(label[11], 53, 12);
    setColor(LIGHTCYAN, BLACK, NORMAL);
    printColored(label[12], 51, 14);
    printColored(label[13], 55, 15);
    setColor(LIGHTCYAN, BLACK, NORMAL);
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
    int col;
    register int cityNo, row;

    setCursor(0);
    clearBox(1, 17, 80, 25);
    printColored(C_MAGENTA "Drug Cities:", 34, 17);
    for (row = 0; row < 4; row++) {
        for (col = 0; col <= 1; col++) {
            cityNo = col * 4 + row;
            if (cityNo > NUM_CITIES) break;
            gotoxy(col * 35 + 10, row + 19);
            setColor(LIGHTRED, BLACK, BOLD);
            cprintf("%2d. ", cityNo + 1);
            setColor(LIGHTCYAN, BLACK, NORMAL);
            cprintf("%-20s", cityName[cityNo]);
            if (cityNo == city) continue;
            cputs("(");
            setColor(GREEN, BLACK, NORMAL);
            cprintf("$%d", travelCost(city, cityNo));
            printColored(C_LCYAN ")", 11, 0);
        }
    }
}

/* Roll new street prices for all eight drugs.  Each price is a random value in
 * the drug's range, plus priceBias, times the current city's multiplier.
 * (The original passes the eight price slots as eight pointers -- an odd
 * choice, but it has to stay for the bytes to match.) */
void generatePrices(long *p0, long *p1, long *p2, long *p3, long *p4, long *p5, long *p6, long *p7)
{
    *p0 = (long)(((double)(rand() % (priceRange[0].hi - priceRange[0].lo) + priceRange[0].lo) + priceBias[0]) * cityPriceMult[city][0]);
    *p1 = (long)(((double)(rand() % (priceRange[1].hi - priceRange[1].lo) + priceRange[1].lo) + priceBias[1]) * cityPriceMult[city][1]);
    *p2 = (long)(((double)(rand() % (priceRange[2].hi - priceRange[2].lo) + priceRange[2].lo) + priceBias[2]) * cityPriceMult[city][2]);
    *p3 = (long)(((double)(rand() % (priceRange[3].hi - priceRange[3].lo) + priceRange[3].lo) + priceBias[3]) * cityPriceMult[city][3]);
    *p4 = (long)(((double)(rand() % (priceRange[4].hi - priceRange[4].lo) + priceRange[4].lo) + priceBias[4]) * cityPriceMult[city][4]);
    *p5 = (long)(((double)(rand() % (priceRange[5].hi - priceRange[5].lo) + priceRange[5].lo) + priceBias[5]) * cityPriceMult[city][5]);
    *p6 = (long)(((double)(rand() % (priceRange[6].hi - priceRange[6].lo) + priceRange[6].lo) + priceBias[6]) * cityPriceMult[city][6]);
    *p7 = (long)(((double)(rand() % (priceRange[7].hi - priceRange[7].lo) + priceRange[7].lo) + priceBias[7]) * cityPriceMult[city][7]);
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
    printColored(C_LMAGENTA "* * *  " C_BLINK_LBLUE "D R U G S  " C_LMAGENTA "* * *", 45, 18);
    fillChars(0xc4, 40, lineBuf);
    setColor(YELLOW, BLACK, NORMAL);
    printColored(lineBuf, 36, 19);
    for (i = 0; i < 4; i++) {
        gotoxy(43, i + 21);
        setColor(LIGHTRED, BLACK, BOLD);
        cprintf("%1d. ", i + 1);
        setColor(LIGHTGREEN, BLACK, NORMAL);
        cprintf("%-15s", drugName[i]);
        gotoxy(60, i + 21);
        setColor(LIGHTRED, BLACK, BOLD);
        cprintf("%1d. ", i + 5);
        setColor(LIGHTGREEN, BLACK, NORMAL);
        cprintf("%-15s", drugName[i + 4]);
    }
    setCursor(1);
}

/* Fight screen: show the hit points of one fighter.  side 0 = you, otherwise
 * the opponent.  Shows "Alive" or "Dead". */
void drawHitPoints(hp, row, side)
int hp, row, side;
{
    register char *word;

    if (side == 0)
        gotoxy(12, row + 10);
    else
        gotoxy(52, row + 11);
    if (hp > 0) {
        word = "Alive";
        setColor(WHITE, BLACK, BOLD);
    } else {
        word = "Dead";
        setColor(LIGHTGRAY, BLACK, NORMAL);
    }
    cprintf("%4d            %-12s", hp, word);
}

/* A "street news" price event: pick 1..count different drugs, announce them
 * ("X, Y, and Z") and multiply their price by (random 0..rng/100) + mul.
 * Used for both price spikes (mul large) and busts (mul small). */
void marketEvent(int count, int rng, float mul)
{
    int numDrugs;
    char *sep;
    register int i, drug;

    numDrugs = rand() % count + 1;
    for (i = 0; i < NUM_DRUGS; i++)
        picked[i] = 0;
    sep = "";
    "";
    for (i = 0; i < numDrugs; i++) {
        do {
            drug = rand() % NUM_DRUGS;
        } while (picked[drug] == 1);
        cprintf("%s%s", sep, drugName[drug]);
        if (numDrugs == 2)
            sep = " and ";
        else
            sep = ", ";
        picked[drug] = 1;
        price[drug] = price[drug] * ((double)(rand() % rng / 100) + mul);
        if (numDrugs > 2 && i + 2 == numDrugs)
            sep = ", and ";
    }
    marketNews = 1;
}

/* Show "Press any key to continue" with a spinning marker and wait for a key. */
void pressAnyKey()
{
    char spin[3] = { 0xf9, 0x09, 0x4f };
    register int i;

    setCursor(0);
    flushKeys();
    printColored(C_BLINK_LGREEN "Press any key to continue", 27, 25);
    setColor(YELLOW, BLACK, NORMAL);
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
 * Each event can happen only once per city visit (eventDone[kind]).
 *
 * The first case is by far the longest: it is the cop fight.  The unusual
 * local names (v20, v1e, ...) are the original compiler's stack slots reused
 * by several cases; their meaning is explained in each case. */
void randomEvent(kind)
int kind;
{
    int choice;
    int numHelpers;
    int myHp[11];
    int copHp[11];
    int copGunType;
    int copTarget;
    int copsAlive;
    int teamAlive;
    int myTarget;
    int gunSaleMoney;
    int fightState;
    int maxShots;
    int shotsFired;
    char unused1;
    char key;
    int v20;
    int v1e;
    int v1c;
    int v1a;
    long bm;
    float blackmailFactor;
    char *statLabel[2] = { "Health", "Status" };
    char buf[12];
    register int i, numCops;

    setCursor(0);
    if (eventDone[kind] == 1) return;
    clearBox(1, 17, 80, 25);
    switch (kind) {
    /* ---- Event 0: the cops fight --------------------------------
     * Fighters on your side: hp[0] is you, hp[1..nfr] are friends/posse.
     * Cops: chp[0..di-1] (di = number of cops, ctype = their gun type).
     * state: 0 = normal, 1 = escaped, 2 = tried to run and failed (cops shoot first).
     */
    case EV_COPS:
#define sellGuns       v20
        copChase = 0;
        fightState = 0;
        if (gunType < 4) maxShots = 1;
        else maxShots = 3;
        printColored(C_RED "The " C_LBLUE "cops " C_RED "are on your tail!!!", 27, 17);
        /* Ask how you want to face them: alone, with a friend (costs 1 status
         * point) or with a posse (costs 5 status points).
         */
        playSound(SND_SIREN);
        printColored(C_LGREEN "Do you wanna   " C_BLINK_LRED "1" C_LGREEN ") Go at it alone", 15, 19);
        printColored(C_BLINK_LRED "2" C_LGREEN ") Call a friend (1 status pt.)", 30, 20);
        printColored(C_BLINK_LRED "3" C_LGREEN ") Call a posse (5 status pts.)", 30, 21);
        choice = 0;
        while (1) {
            do {
                clearBox(1, 23, 80, 25);
                printColored(C_YELLOW "Call it \020 ", 20, 23);
                setCursor(1);
                escPressed = readField(1, 0, lineBuf);
                setCursor(0);
                choice = atol(lineBuf);
            } while (choice < 1 || choice > 3);
            if ((choice == 2 && status == 0) || (choice == 3 && status < 5)) {
                printColored(C_LRED "You don't have enough points!", 25, 25);
                playSound(SND_DENIED);
                delay(2000);
                clearBox(25, 25, 80, 25);
            } else
                break;
        }
        /* nfr = number of helpers who fight next to you. */
        switch (choice) {
        case 1:
            numHelpers = 0;
            break;
        case 2:
            numHelpers = 1;
            status--;
            break;
        case 3:
            numHelpers = rand() % 4 + status / 100 + 4;
            if (numHelpers > 8) numHelpers = 8;
            status -= 5;
            break;
        }
        /* Draw the fight screen and its captions. */
        clearBox(1, 1, 80, 25);
        puttext(1, 1, 80, 22, fightScreen);
        setColor(YELLOW, BLACK, NORMAL);
        printColored(label[0], 24, 2);
        printColored(label[0], 52, 2);
        setColor(LIGHTRED, BLACK, BOLD);
        printColored(label[1], 32, 2);
        setColor(LIGHTGREEN, BLACK, BOLD);
        printColored(statLabel[0], 9, 8);
        printColored(statLabel[0], 49, 8);
        printColored(statLabel[1], 28, 8);
        printColored(statLabel[1], 68, 8);
        setColor(YELLOW, BLACK, BOLD);
        printColored(label[11], 10, 6);
        printColored(label[11], 50, 6);
        printColored(label[10], 9, 5);
        setColor(LIGHTRED, BLACK, NORMAL);
        gotoxy(20, 5);
        cprintf("%-10d", numGuns);
        setColor(BROWN, BLACK, NORMAL);
        gotoxy(20, 6);
        cprintf("%-15s", gunName[gunType]);
        /* Hit points: yours, then every helper (25..34 plus bonus dice from status).
         */
        myHp[0] = health;
        setColor(BROWN, BLACK, NORMAL);
        for (i = 1; i <= numHelpers; i++) {
            myHp[i] = rand() % 10 + rollDice(10, status / 50) + 25;
            gotoxy(1, i + 10);
            if (choice == 2) cprintf("%10s:", "Friend");
            if (choice == 3) cprintf("%6s #%2d:", "Posse", i);
        }
/* Choose the cops: how many (di) and how strong (ctype is the gun type,
 * limited to 2 or 3, tougher with more status).  Re-roll until valid.
 * Once in 20 times a bigger squad appears.
 */
again:
        numCops = rand() % 8 + 3;
        copGunType = status / (numCops * 2);
        if (rand() % 20 == 0) {
            if (copGunType < 2) {
                numCops = rand() % 4 + 1;
                copGunType = status / numCops;
                if (copGunType > 3) copGunType = 3;
                if (copGunType < 2) copGunType = 2;
            } else {
                numCops = rand() % 3 + 8;
                copGunType = 3;
            }
        } else {
            if (copGunType < 2) goto again;
            if (copGunType > 3) goto again;
        }
        gotoxy(60, 6);
        setColor(LIGHTBLUE, BLACK, NORMAL);
        cprintf("%-15s", gunName[copGunType]);
        setColor(LIGHTBLUE, BLACK, NORMAL);
        for (i = 0; i < numCops; i++) {
            gotoxy(42, i + 11);
            cprintf("%6s #%2d", "Cop", i + 1);
            copHp[i] = rollDice(15, 2) + 15;
        }
        /* Cop hit points: two 15-sided dice + 15 each. */
        copsAlive = numCops;
        teamAlive = numHelpers + 1;
        for (i = 0; i <= numHelpers; i++)
            drawHitPoints(myHp[i], i, 0);
        for (i = 0; i < numCops; i++)
            drawHitPoints(copHp[i], i, 1);
        /* Fight round loop: ask Fight or Run, then play the round. */
        do {
            clearBox(1, 24, 80, 25);
            printColored(C_WHITE "Do ya wanna (" C_BLINK_LRED "F" C_WHITE ")ight or (" C_BLINK_LRED "R" C_WHITE ")un? ", 5, 24);
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
                    if (fightState != 2) {
                        for (i = 0; i <= numHelpers; i++) {
                            if (myHp[i] == 0) continue;
                            if (i < numGuns) printColored(C_LRED "Firing", 28, i + 10);
                            else printColored(C_LRED "Punching", 27, i + 10);
                            shotsFired = 0;
                            do {
                                do {
                                    copTarget = rand() % numCops;
                                } while (copHp[copTarget] == 0);
                                playSound(SND_SHOT_BASE + gunType);
                                if (rand() % 2 == 0) {
                                    printColored(C_BLINK_RED "*  Hit!  *", 67, copTarget + 11);
                                    playSound(SND_HIT);
                                    delay(100);
                                    if (i < numGuns) copHp[copTarget] -= rollDice(6, gunPower[gunType]);
                                    else copHp[copTarget] -= rollDice(4, 1);
                                    if (copHp[copTarget] <= 0) {
                                        copHp[copTarget] = 0;
                                        copsAlive--;
                                    }
                                } else
                                    printColored(C_YELLOW "- Missed! -", 67, copTarget + 11);
                                delay(200);
                                drawHitPoints(copHp[copTarget], copTarget, 1);
                                shotsFired++;
                                if (myHp[0] == 0 || copsAlive == 0) break;
                            } while (gunType > 3 && shotsFired < maxShots && copsAlive > 0 && i < numGuns);
                            drawHitPoints(myHp[i], i, 0);
                            if (copsAlive == 0) break;
                        }
                    }
                    /* The surviving cops shoot back at random living members of your side.
                     * Every one of your helpers who dies costs you a status point.
                     */
                    if (copsAlive == 0) break;
                    fightState = 0;
                    for (i = 0; i < numCops; i++) {
                        if (copHp[i] == 0) continue;
                        printColored(C_LRED "Firing", 68, i + 11);
                        shotsFired = 0;
                        do {
                            myTarget = rand() % (numHelpers + 1);
                        } while (myHp[myTarget] == 0);
                        playSound(SND_SHOT_BASE + copGunType);
                        if (rand() % 2 == 0) {
                            printColored(C_BLINK_RED "*  Hit!  *", 26, myTarget + 10);
                            playSound(SND_HIT);
                            delay(100);
                            myHp[myTarget] -= rollDice(6, gunPower[copGunType]);
                            if (myHp[myTarget] <= 0) {
                                status--;
                                myHp[myTarget] = 0;
                                teamAlive--;
                            }
                        } else
                            printColored(C_YELLOW "- Missed! -", 26, myTarget + 10);
                        delay(200);
                        drawHitPoints(myHp[myTarget], myTarget, 0);
                        shotsFired++;
                        drawHitPoints(copHp[i], i, 1);
                        if (myHp[0] == 0 || teamAlive == 0) break;
                    }
                    break;
                /* Run away: succeeds when a random roll (+cops/3) is lower than yours (+helpers/3).
                 */
                case 'R':
                    printColored(C_LCYAN "You take off running... ", 20, 25);
                    delay(3000);
                    if (rand() % 10 + numCops / 3 < rand() % 10 + numHelpers / 3) {
                        printColored(C_BLINK_YELLOW "and ya got away!!!", 99, 99);
                        health = myHp[0];
                        fightState = 1;
                    } else {
                        printColored(C_LCYAN "and ya didn't get away!!!", 99, 99);
                        fightState = 2;
                        key = 'F';
                        continue;
                    }
                    break;
                }
                break;
            }
        } while (copsAlive > 0 && teamAlive > 0 && myHp[0] > 0 && fightState != 1);
        /* You got away. */
        if (fightState == 1) {
            delay(3000);
            drawMainScreen();
            break;
        }
        /* Fight over.  If you died the game ends; otherwise the cops are dead. */
        health = myHp[0];
        if (health == 0) {
            dead = 1;
            break;
        }
        clearBox(1, 23, 80, 25);
        printColored(C_LGREEN "Yous wasted 'em!!! Way ta go, man!!!", 23, 23);
        delay(1000);
        /* v20 = 1 means "sell the guns" (default); 0 if you took them instead. */
        sellGuns = 1;
        /* If you already own better and enough guns, don't even ask. */
        if (copGunType < gunType && numCops <= numGuns) goto skipask;
        setColor(LIGHTGRAY, BLACK, NORMAL);
        gotoxy(21, 24);
        if (numCops == 1) strcpy(buf, "his gun");
        else strcpy(buf, "their guns");
        cprintf("Do you wanna take %s, man? (Y/N) ", buf);
        setCursor(1);
        do {
            key = toupper(getch());
        } while (key != 'Y' && key != 'N');
        setCursor(0);
        /* Taking their guns: add to your collection or swap to the better type. */
        if (key == 'Y') {
            printColored(C_YELLOW "You " C_LRED "want" C_YELLOW " 'em, you " C_LRED "got " C_YELLOW "'em!  What a bunch'a " C_BROWN "chumps!", 15, 25);
            sellGuns = 0;
            if (gunType == copGunType) numGuns += numCops;
            else {
                gunType = copGunType;
                numGuns = numCops;
            }
        } else
            printColored(C_LBLUE "OK, man, it's yer call.", 29, 25);
        delay(2000);
skipask:
        /* Otherwise sell the guns: 200 dollars per gun per gun-type level, plus status.
         */
        if (sellGuns == 1) {
            printColored(C_LCYAN "You sell the ", 5, 24);
            gunSaleMoney = numCops * copGunType * 200;
            cprintf("%d guns that were on the pigs you wasted for %d bucks!", numCops, gunSaleMoney);
            printColored(C_LCYAN "This'll teach 'em not to mess with ya, huh?", 20, 25);
            cash += gunSaleMoney;
            status += numCops;
            delay(4000);
        }
        if (dead == 0) drawMainScreen();
        break;
    /* ---- Event 1: police sell confiscated drugs.  1-3 drugs drop to
     * 70..99% of their price.  Skipped if a price event already ran.
     */
#undef sellGuns
    case EV_COPS_SELL:
        if (marketNews == 1) break;
        printColored(C_LCYAN "\"Hey man, check dis out!  Word has it on de street dat deres des cops, see?\r\nThey musta decided that workin' for 25 G's a year ain't enough, so de've\r\ndecided ta sell some of de take drugs confiscated on de side! De market is\r\nflooded with ", 1, 18);
        setColor(BROWN, BLACK, BOLD);
        marketEvent(3, 30, 0.7f);
        setColor(LIGHTCYAN, BLACK, NORMAL);
        cprintf("!\"");
        playSound(SND_NEWS);
        pressAnyKey();
        break;
    /* ---- Event 2: cheap homemade stuff.  1-3 drugs drop to 60..89%. */
    case EV_CHEAP_DRUGS:
        printColored(C_LGRAY "\"Hey, I heard it through de grapevine dat deres some cheap homemade stuff\r\navailable from some of the homeboys downtown.  Real primo stuff, dey say!\r\nAll I hear on de streets is 'Cheap ", 1, 18);
        setColor(BROWN, BLACK, BOLD);
        marketEvent(3, 40, 0.6f);
        setColor(LIGHTGRAY, BLACK, NORMAL);
        cprintf("!'\"");
        pressAnyKey();
        break;
    /* ---- Event 3: "EXTRA!" newspaper - a drug bust.  Every price rises 5..29%.
     * The newspaper is a boxed text drawn with line-drawing characters.
     */
    case EV_BUST:
        window(15, 17, 80, 25);
        printColored(C_LGRAY "\332\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\277\r\n\263  * * *   " C_BLINK_WHITE "E X T R A ! !   E X T R A ! !" C_LGRAY "   * * *  \263\r\n\263               " C_BLINK_WHITE "Read all about it!!" C_LGRAY "               \263\r\n\303\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\264\r\n\263  After six months of undercover investigation,  \263\r\n\263  police close in and make a major drug bust!    \263\r\n\263  This will no doubt affect street drug prices.  \263\r\n\300\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\304\331", 15, 17);
        window(1, 1, 80, 25);
        for (i = 0; i < NUM_DRUGS; i++)
            price[i] = price[i] * ((double)(rand() % 25) / 100.0 + 1.05);
        playSound(SND_EXTRA);
        pressAnyKey();
        break;
    /* ---- Event 4: a supplier's factory is destroyed.  One drug costs
     * 120..159% of the normal price.
     */
    case EV_FACTORY:
        printColored(C_LMAGENTA "\"Dude, I got some news for ya... a factory of one of de cities biggest suppliersof ", 1, 18);
        setColor(BROWN, BLACK, BOLD);
        marketEvent(1, 40, 1.2f);
        setColor(LIGHTMAGENTA, BLACK, NORMAL);
        if (rand() % 2 == 1) cputs(" burned down");
        else cputs(" blew up");
        printColored("last night.  I don't have to tell ya what this'll on de\r\nstreet...\"", 99, 99);
        pressAnyKey();
        break;
    /* ---- Event 5: your girlfriend turns all your weed (drug #5) into brownies.
     * Only happens if you actually carry some.
     */
    case EV_BROWNIES:
        if (inventory[DRUG_GRASS] == 0) break;
        printColored(C_BROWN "Ya come back to yer place tonight and find dat yer sexy main squeeze tought\r\ndat it wouldst be a good idea to use some of yer weed to make brownies!!!\r\nAll tings considered, yous take it pretty well.  After all, if ya were mad,\r\nyou would've " C_LGRAY "chopped " C_BROWN "up her body " C_LRED "before " C_BROWN "dumping it in de river!", 1, 17);
        inventory[DRUG_GRASS] = 0;
        pressAnyKey();
        break;
    /* ---- Event 6: a mugger.  With 10+ status you may talk your way out
     * (costs 10 status).  Otherwise he takes ALL your cash; if you had less
     * than $50 he shoots you instead (2..12 damage).
     */
    case EV_MUGGER:
        printColored(C_LRED "Yer walking along de street at night, when from outta nowhere jumps dis goon!\r\nHe waves a 9mm Beretta at yous and demands yer fork over all yer money.", 1, 17);
        while (1) {
            if (status >= 10 && rand() % 20 != 0) {
                printColored(C_YELLOW "Do ya wanna use yer status as a major drug dealer to get outta this? (Y/N) ", 1, 20);
                setCursor(1);
                do {
                    key = toupper(getch());
                } while (key != 'Y' && key != 'N');
                setCursor(0);
                if (key == 'Y') {
                    printColored(C_LRED "Yous tell him whos he's dealin' wit.  He looks nervous, asks fer yer pardon,den runs off down an alley!", 1, 22);
                    status -= 10;
                    break;
                }
            }
            if (cash < 50) {
                printColored(C_LCYAN "\"This is all you got? &%&*! this, man!\"", 20, 21);
                printColored("He looks at ya like a crazed animal, and pulls de trigger!", 10, 22);
                printColored(C_YELLOW "* * *   " C_BLINK_LRED "B L A M   " C_YELLOW "* * *", 28, 23);
                playSound(SND_MUGGER_SHOT);
                health -= rand() % 6 + rand() % 6 + 2;
            } else
                printColored(C_LMAGENTA "He laughs merrily as he takes yer cash and runs off down de street!", 5, 22);
            cash = 0;
            break;
        }
        pressAnyKey();
        break;
    /* ---- Event 7: trenchcoat salesman.  Offers a bigger coat (capacity in
     * v20, a multiple of 10, up to 50 more than yours).  The price bm grows
     * with the capacity: capacity^3.34 * 0.0634, times a random 1-200%.
     */
    case EV_TRENCHCOAT:
#define coatSize       v20
#define coatPrice      bm
        if (holdSize >= MAX_COAT) break;
        do {
            coatSize = rand() % 10 + 1;
            if (rand() % 5 == 0) {
                coatSize += rand() % 10 + 1;
                if (rand() % 5 == 0)
                    coatSize += rand() % 10 + 1;
            }
            coatSize *= 10;
        } while (holdSize + 50 < coatSize || coatSize <= holdSize);
        coatPrice = pow((double)coatSize, 3.34) * 0.0634;
        coatPrice = (long)((double)((rand() % 200 + 1) * coatPrice) / 10000.0) * 100;
        printColored(C_LGREEN "You are approached by a shady-lookin guy in a long dark trenchcoat.  He says:", 1, 17);
        printColored(C_LBLUE "\"Hey, bud, I got a " C_BLINK_YELLOW "special " C_LBLUE "trenchcoat for ya, just your style!", 10, 19);
        printColored("It's got a capacity of ", 11, 20);
        setColor(LIGHTRED, BLACK, BOLD);
        cprintf("%d", coatSize);
        printColored(C_LBLUE " and I won't take under ", 99, 99);
        setColor(LIGHTRED, BLACK, BOLD);
        cprintf("$%ld", coatPrice);
        printColored(C_LBLUE " for it.\"", 99, 99);
        printColored(C_YELLOW "Is it a deal? (Y/N) \020 ", 1, 23);
        setCursor(1);
        do {
            key = toupper(getch());
        } while (key != 'Y' && key != 'N');
        setCursor(0);
        if (key == 'Y') {
            if (cash < coatPrice)
                printColored(C_LBLUE "\"Sorry Charly, I don't take credit! Maybe when you're a little more solvent!\"", 1, 24);
            else {
                printColored(C_LBLUE "\"You bought yourself one hell of a trenchcoat!\"", 16, 24);
                cash -= coatPrice;
                holdSize = coatSize;
            }
        } else
            printColored(C_LBLUE "\"OK, it's your loss, man.  What a shame!\"", 20, 24);
        pressAnyKey();
        break;
    /* ---- Event 8: gun smuggler Juan.  v20 = gun type offered (0-4),
     * v1e = how many, v1c = price each, v1a = total.  Buying replaces
     * your present guns with the new batch.
     */
#undef coatSize
#undef coatPrice
    case EV_GUN_DEALER:
#define gunKind        v20
#define gunCount       v1e
#define gunPrice       v1c
#define gunTotal       v1a
        gunKind = rand() % 5;
        if (rand() % 10 == 0) gunCount = rand() % 6 + 5;
        else gunCount = rand() % 5 + 2;
        gunPrice = gunKind * 200 + 200;
        gunTotal = gunCount * gunPrice;
        printColored(C_LMAGENTA "Juan, one of the most powerful underground gun smugglers in the world,\r\ncomes to you with an offer:", 1, 17);
        printColored(C_LGRAY "\"I have a shipment of ", 1, 20);
        setColor(LIGHTRED, BLACK, BOLD);
        cprintf("%s's", gunLongName[gunKind]);
        printColored(C_LGRAY " coming in tonight.  I'm", 99, 99);
        printColored("distributing them in shipments of ", 1, 21);
        setColor(LIGHTRED, BLACK, BOLD);
        cprintf("%d ", gunCount);
        printColored(C_LGRAY "at ", 99, 99);
        setColor(LIGHTRED, BLACK, BOLD);
        cprintf("$%d ", gunPrice);
        printColored(C_LGRAY "each, for a total of ", 99, 99);
        setColor(LIGHTRED, BLACK, BOLD);
        cprintf("$%d", gunTotal);
        setColor(LIGHTGRAY, BLACK, NORMAL);
        cputs(".\"");
        printColored(C_LMAGENTA "Are you interested in an order? (Y/N) \020 ", 20, 23);
        setCursor(1);
        do {
            key = toupper(getch());
        } while (key != 'Y' && key != 'N');
        setCursor(0);
        if (key == 'Y') {
            if (gunTotal > cash)
                printColored(C_LGRAY "\"I'm sorry, sir, but I only accept cash " C_WHITE "up-front" C_LGRAY ".  Business is business.\"", 5, 24);
            else {
                printColored(C_LGRAY "\"Very well, they will be delivered by tomorrow.  Good doing business with you.\"", 1, 24);
                numGuns = gunCount;
                gunType = gunKind + 1;
                cash -= gunTotal;
            }
        } else
            printColored(C_LGRAY "\"As you wish.  Perhaps next time we may do business.\"", 15, 24);
        pressAnyKey();
        break;
    /* ---- Event 9: blackmail.  Only if you have $10000+.  Demands 70..99% of
     * your cash (rounded to thousands).  If you refuse, 80% of the time
     * the cops will come after you (event 0 is re-armed).
     */
#undef gunKind
#undef gunCount
#undef gunPrice
#undef gunTotal
    case EV_BLACKMAIL:
#define blackmail      bm
        if (cash < 10000L) break;
        blackmailFactor = rand() % 30 / 100.0 + 0.7;
        blackmail = cash * blackmailFactor / 1000.0;
        blackmail = blackmail * 1000;
        printColored(C_LGREEN "Yous get a note in de mail from a business associate of yers.  He says dat\r\nif yous don't deposit ", 1, 17);
        setColor(LIGHTRED, BLACK, BOLD);
        cprintf("$%ld ", blackmail);
        printColored(C_LGREEN "in his Swiss bank account, he's gonna " C_BLINK_BROWN "rat " C_LGREEN "to de\r\ncops everything dat he knows about you.", 99, 99);
        printColored(C_LMAGENTA "Do you pay the blackmail? (Y/N) \020 ", 1, 21);
        setCursor(1);
        do {
            key = toupper(getch());
        } while (key != 'Y' && key != 'N');
        setCursor(0);
        if (key == 'Y') {
            printColored(C_LMAGENTA "You wire de money into his account.  That really " C_BLINK_LRED "^$&?*@ " C_LMAGENTA "sucks!!!", 1, 24);
            cash -= blackmail;
        } else {
            printColored(C_LRED "Yeah, man, nobody pushes you around! I respect yer integrity!!!", 9, 24);
            if (rand() % 5 != 0) {
                copChase = 1;
                eventDone[EV_COPS] = 0;
            }
        }
        pressAnyKey();
        break;
#undef blackmail
    }
    /* Remember that this event has happened in this city. */
    eventDone[kind] = 1;
    if (dead == 0) drawStatus();
}

/* Text of the final score screen (shown in a scrolling box by showEndGame).
 * showEndGame() fills the numbers in at the right-hand end of the lines
 * (that is why the strings contain blanks).  \221 = colour escape. */
char endText[18][50] = {
    "           " C_BLINK_LRED "CONGRATULATIONS!!!         ",
    "                                      ",
    C_YELLOW "Yous has become a " C_BLINK_LMAGENTA "DRUGLORD!!!" C_YELLOW "  I'm    ",
    "proud ta know you, man!  Anyway, check",
    "dis out --                            ",
    "                                      ",
    C_LCYAN "           Cash in Hand:" C_BLINK_YELLOW "              ",
    C_LCYAN "           Cash in Bank:" C_BLINK_YELLOW "              ",
    C_LCYAN "          Still in Debt:" C_BLINK_YELLOW "              ",
    C_LCYAN "   Number of Good Deals:" C_BLINK_YELLOW "              ",
    C_LCYAN "  Number of Total Deals:" C_BLINK_YELLOW "              ",
    C_LCYAN "                 DEAL %:" C_BLINK_YELLOW "              ",
    C_LCYAN "                        " C_BLINK_YELLOW "              ",
    C_LCYAN "             Total Days:" C_BLINK_YELLOW "              ",
    C_LCYAN "                        " C_BLINK_YELLOW "              ",
    C_WHITE "------------------------     -------- ",
    C_LRED "          OVERALL SCORE:" C_BLINK_YELLOW "              ",
    C_LGRAY "                   RANK:" C_BLINK_YELLOW "              "
};

/* "Would ya like ta play again? (Y/N)".  Returns 1 for yes, 0 for no. */
int askPlayAgain()
{
    char c;
    register int r;

    setColor(WHITE, BLACK, NORMAL);
    drawBox("\xc9\xcd\xbb\xba\xbc\xcd\xc8\xba", 29, 10, 51, 15);
    clearBox(30, 11, 50, 14);
    printColored(C_LRED "Would ya like ta", 32, 12);
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
    int i, j, k, x, row, numLayers, stepDelay, layer, pct, rank;
    char *titles[12] = { "Loser!", "Dopehead", "Peddler", "Pusher", "Gangster", "Dealer",
                         "Trafficker", "Racketeer", "Marketeer", "Entrepreneur", "DRUGLORD", "OVERLORD" };
    long rankLimit[11] = { 250000L, 500000L, 600000L, 700000L, 800000L, 850000L,
                     900000L, 950000L, 1000000L, 1100000L, 1250000L };
    char msg[11] = "DRUGLORD!!!";
    int jingle[16] = { 100, 150, 500, 400, 100, 75, 25, 500, 1000, 1500, 1000, 750, 500, 200, 100, 50 };
    char scrollBuf[1121];
    char key;
    char numStr[10];
    long score;
    long speedBonus;
    int guns;
    double wealthFactor;
    register int idx, len;

    delay(1000);
    clearBox(1, 17, 80, 25);
    /* Flash "DRUGLORD!!!" five times in different colours with a little tune.
     */
    for (i = 1; i < 6; i++) {
        clearBox(15, 20, 65, 20);
        setColor(i + 9, BLACK, i % 2 == 1);
        for (j = 0; j < 22; j++) {
            gotoxy(j / 2 * 5 + 15, 20);
            putch(msg[j / 2]);
            if (j < 16) {
                sound(jingle[j]);
                delay(20);
            }
        }
        nosound();
        delay(500);
    }
    /* Draw the end scene, then one gun picture for each gun owned (max 10). */
    puttext(1, 1, 80, 25, endScene);
    if (numGuns < 10) guns = numGuns;
    else guns = 10;
    for (i = 1; i <= guns; i++) {
        if (i <= 4) {
            k = (i - 1) / 2 * 11 + (i % 2 == 0) * 5 + 55;
            idx = (i % 2 == 0) * 3 + 2;
            puttext(k, idx, k + 7, idx + 2, gunSprite);
        } else {
            k = (i - 5) / 2 * 11 + !(i % 2) * 5 + 1;
            idx = (i % 2 == 0) * 3 + 2;
            puttext(k, idx, k + 7, idx + 2, gunSprite);
        }
    }
    /* Draw stacks of money: each step of $100000 adds a block, each $2000000
     * starts a new layer, accompanied by rising beeps.
     */
    numLayers = (int)((cash + bank - 1) / 2000000L) + 1;
    for (idx = 0; idx < 4; idx++) {
        row = idx + 11;
        for (k = 0; k < 5; k++) {
            x = k * 7 + 18 - idx * 2;
            stepDelay = 15 - idx * 3 - k;
            if (stepDelay < 0) stepDelay = 0;
            for (layer = 0; layer < numLayers; layer++) {
                if ((long)(idx * 5 + k + 1) * 100000L + (long)layer * 2000000L > cash) break;
                puttext(x, row - layer, x + 4, row - layer, barSprite);
                sound(layer * 500 + 1000);
                delay(stepDelay + 5);
                sound(layer * 500 + 3000);
                delay(stepDelay);
            }
        }
    }
    nosound();
    /* Fill the numbers into the score table (right-aligned, column 0x29). */
    pct = goodDeals * 100 / totalDeals;
    ltoa(cash, numStr, 10);
    len = strlen(numStr);
    stpcpy(endText[6] + (0x29 - len), numStr);
    ltoa(bank, numStr, 10);
    len = strlen(numStr);
    stpcpy(endText[7] + (0x29 - len), numStr);
    ltoa(debt, numStr, 10);
    len = strlen(numStr);
    stpcpy(endText[8] + (0x29 - len), numStr);
    itoa(goodDeals, numStr, 10);
    len = strlen(numStr);
    stpcpy(endText[9] + (0x29 - len), numStr);
    itoa(totalDeals, numStr, 10);
    len = strlen(numStr);
    stpcpy(endText[10] + (0x29 - len), numStr);
    itoa(pct, numStr, 10);
    len = strlen(numStr);
    stpcpy(endText[11] + (0x28 - len), numStr);
    strcat(endText[11], "%");
    len = strlen(gunName[gunType]);
    stpcpy(endText[12] + (0x17 - len), gunName[gunType]);
    stpcpy(endText[12] + 0x17, "'s:" C_BLINK_YELLOW "            ");
    itoa(numGuns, numStr, 10);
    len = strlen(numStr);
    stpcpy(endText[12] + (0x29 - len), numStr);
    itoa(totalDays, numStr, 10);
    len = strlen(numStr);
    stpcpy(endText[13] + (0x29 - len), numStr);
    /* Now the score (see the formula above). */
    wealthFactor = exp((double)(cash + bank - debt) * 6.9315e-08);
    score = (long)wealthFactor * 75000L;
    if (score > 700000L) score = 700000L;
    speedBonus = (long)(pow((double)totalDays, -2.12) * 104000000000.0);
    if (speedBonus > 500000L) speedBonus = 500000L;
    score += speedBonus;
    score += (long)(pow((double)pct, 8.14) * 2.48e-11);
    score += (long)(gunType * numGuns) * 1500L;
    ltoa(score, numStr, 10);
    len = strlen(numStr);
    stpcpy(endText[16] + (0x29 - len), numStr);
    /* Find the rank title that goes with the score. */
    for (rank = 0; rank < 10; rank++) {
        if (rankLimit[rank] > score) break;
    }
    len = strlen(titles[rank]);
    stpcpy(endText[17] + (0x29 - len), titles[rank]);
    setColor(LIGHTBLUE, BLACK, NORMAL);
    drawBox("\311\315\273\272\274\315\310\272", 10, 18, 55, 25);
    printColored(" " C_BLINK_LRED "\031 ", 50, 25);
    printColored(C_LBLUE "[" C_BLINK_LCYAN "ESC" C_LBLUE "] " C_BLINK_LCYAN "Finished ", 12, 25);
    setColor(YELLOW, BLACK, NORMAL);
    for (idx = 0; idx < 5; idx++)
        printColored(endText[idx], 15, idx + 19);
    /* Scroll loop: UP/DOWN arrows (extended keys 0x48/0x50) move the text,
     * ESC leaves.
     */
    idx = 0;
    while (1) {
        key = getch();
        setColor(YELLOW, BLACK, NORMAL);
        if (key == 0) {
            key = getch();
            if (key == 0x48 && idx > 0) {
                idx--;
                gettext(15, 19, 55, 23, scrollBuf);
                puttext(15, 20, 55, 24, scrollBuf);
                printColored(endText[idx], 15, 19);
                playSound(SND_SCROLL);
            }
            if (key == 0x50 && idx < 12) {
                idx++;
                gettext(15, 20, 55, 24, scrollBuf);
                puttext(15, 19, 55, 23, scrollBuf);
                printColored(endText[idx + 5], 15, 24);
                playSound(SND_SCROLL);
            }
            if (idx > 0) printColored(" " C_LRED "\030 ", 50, 18);
            else printColored(C_LBLUE "\315\315\315", 50, 18);
            if (idx < 10) printColored(" " C_BLINK_LRED "\031 ", 50, 25);
            else printColored(C_LBLUE "\315\315\315", 50, 25);
        }
        if (key == 0x1b) break;
    }
}

/* Advance the calendar one day.  The loan grows 5% a day and the bank
 * account earns 0.0192% a day.  Years are two digits (wrap at 100). */
void nextDay()
{
    loanDays--;
    day++;
    totalDays++;
    if (daysInMonth[month - 1] < day) {
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

/* Title screen.  First measures the computer speed by counting how often the
 * title can be drawn in one BIOS clock tick -> delayTime, which keeps
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
    setColor(YELLOW, BLACK, NORMAL);
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
    delayTime = 31 - 55 / count;
    if (delayTime < 0) delayTime = 0;
    printColored(C_LGREEN "Version v2.01", 35, 13);
    printColored(C_BLUE "\020" C_LBLUE "\020" C_LCYAN "\020   " C_BLINK_YELLOW "UNREGISTERED VERSION   " C_LCYAN "\021" C_LBLUE "\021" C_BLUE "\021", 24, 16);
    printColored(C_LRED "Written by Phil Erwin", 30, 19);
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
    setColor(YELLOW, BLACK, BOLD);
    drawBox("\xc9\xcd\xbb\xba\xbc\xcd\xc8\xba", 30, 22, 51, 24);
    setColor(LIGHTMAGENTA + BLINK, BLACK, NORMAL);
    printColored("Initializing...", 34, 23);
    for (i = 0; i < NUM_DRUGS; i++) {
        inventory[i] = 0;
        priceBias[i] = 1.0;
        avgCost[i] = 0;
    }
    for (i = 0; i < NUM_EVENTS; i++)
        eventDone[i] = 0;
    generatePrices(&price[0], &price[1], &price[2], &price[3], &price[4], &price[5], &price[6], &price[7]);
    city = rand() % NUM_CITIES;
    cash = START_CASH;
    holdSize = START_COAT;
    bank = 0L;
    debt = 0L;
    hasLoan = 0;
    numGuns = 0;
    gunType = 0;
    status = 1;
    health = MAX_HEALTH;
    dead = 0;
    sharkAngry = 0;
    copChase = 0;
    beatings = 0;
    totalDays = 0;
    month = rand() % 12 + 1;
    day = rand() % 28 + 1;
    year = rand() % 6 + 86;
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

    if (monochrome == 1) {
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
    int i;
    unsigned long moneyAmount;
    char inputBuf[81];
    char key;
    int amount;
    int running;
    int loanTerm;
    long maxAfford;
    int bonus;
    int statusBefore;
    char slideBuf[1280];
    register int choice, drug;

    /* Start-up: seed the random generator, detect monochrome (modes 2 and 7)
     * and refuse to run in 40 column modes.
     */
    srand((unsigned)time(0));
    gettextinfo(&textInfo);
    if (textInfo.currmode == 7 || textInfo.currmode == 2) monochrome = 1;
    else monochrome = 0;
    if (textInfo.currmode == 0 || textInfo.currmode == 1) {
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
            printColored(C_LGREEN "(" C_BLINK_LRED "Q" C_LGREEN ")uit de game", 67, 17);
            window(15, 20, 80, 25);
            printColored(C_LGREEN "(" C_BLINK_LRED "B" C_LGREEN ")uy Drugs,                       (" C_BLINK_LRED "V" C_LGREEN ")isit de Bank,\r\n(" C_BLINK_LRED "S" C_LGREEN ")ell Drugs,                      Go to de (" C_BLINK_LRED "H" C_LGREEN ")ospital,\r\n(" C_BLINK_LRED "J" C_LGREEN ")et to another city,             See de (" C_BLINK_LRED "L" C_LGREEN ")oan Shark", 15, 20);
            window(1, 1, 80, 25);
            printColored(C_LGRAY "Here's a list of yer possible plans, dude:", 20, 18);
            printColored(C_LGRAY "Make up your mind " C_LBLUE "\020 ", 30, 25);
            /* 300 status points = you won; leave to the end screen. */
            if (status >= WIN_STATUS) break;
            setColor(WHITE, BLACK, NORMAL);
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
                    printColored(C_LGREEN "Where ya wanna go? ", 1, 25);
                    escPressed = readField(2, 1, inputBuf);
                    setCursor(0);
                    if (escPressed == 1) break;
                    choice = atol(inputBuf);
                    if (choice < 1 || choice > NUM_CITIES) playSound(SND_BEEP);
                    if (choice - 1 == city) {
                        printColored(C_LMAGENTA "Are you having " C_BLINK_WHITE "flashbacks? " C_LMAGENTA "That's where you are now!", 28, 25);
                        playSound(SND_DENIED);
                        choice = 0;
                        delay(500);
                    }
                } while (choice < 1 || choice > NUM_CITIES);
                setCursor(0);
                if (escPressed == 1) continue;
                if ((long)travelCost(city, choice - 1) > cash) {
                    printColored(C_BLINK_LRED "Start flappin' yer arms, cause you ain't got de dough!", 25, 25);
                    playSound(SND_DENIED);
                    delay(2000);
                    continue;
                }
                cash -= travelCost(city, choice - 1);
                city = choice - 1;
                marketNews = 0;
                generatePrices(&price[0], &price[1], &price[2], &price[3], &price[4], &price[5], &price[6], &price[7]);
                gotoxy(30, 25);
                setColor(WHITE, BLACK, NORMAL);
                for (i = 79; i > 1; i -= 2) {
                    gettext(3, 19, i, 22, slideBuf);
                    puttext(1, 19, i - 2, 22, slideBuf);
                    sound((79 - i) * 20 + 1000);
                    delay(delayTime);
                }
                nosound();
                drawStatus();
                for (i = 0; i < NUM_EVENTS; i++) eventDone[i] = 0;
                break;
            /* ---- Buy drugs.  Average cost per drug (avgCost) is the mean of the old
             * average and today's price; it is used later to score profits.
             */
            case 'B':
                drawDrugMenu();
                do {
                    printColored(C_RED "What kinda drugs are\r\nya interested in? ", 1, 19);
                    setColor(YELLOW, BLACK, NORMAL);
                    escPressed = readField(1, 0, inputBuf);
                    if (escPressed == 1) break;
                    drug = atol(inputBuf);
                    if (drug < 1 || drug > NUM_DRUGS) playSound(SND_BEEP);
                } while (drug < 1 || drug > NUM_DRUGS);
                if (escPressed == 1) continue;
                drug--;
                maxAfford = cash / price[drug];
                printColored(C_RED "How many do ya wanna buy?\r\n(", 1, 22);
                setColor(LIGHTCYAN, BLACK, BOLD);
                cprintf("%ld", maxAfford);
                printColored(C_RED " max.)", 99, 99);
                setColor(YELLOW, BLACK, NORMAL);
                do {
                    gotoxy(27, 22);
                    escPressed = readField(5, 0, inputBuf);
                    if (escPressed == 1) break;
                    amount = atol(inputBuf);
                } while (amount < 0 || inputBuf[0] == 0);
                setCursor(0);
                if (escPressed == 1) continue;
                if ((long)amount > maxAfford) {
                    printColored(C_BLINK_LMAGENTA "Hey man, if ya don't got enough\r\n  cash, don't come 'round here!", 3, 24);
                    playSound(SND_DENIED);
                    delay(1000);
                    continue;
                }
                if (amount > holdFree) {
                    printColored(C_LGREEN "Dude, you can't hold that much!\r\n  Everyone's got a limit, right?", 3, 24);
                    playSound(SND_DENIED);
                    delay(3000);
                    continue;
                }
                if (amount > 0) {
                    printColored(C_YELLOW "* * *   " C_BLINK_WHITE "S O L D ! ! !   " C_YELLOW "* * *", 1, 25);
                    playSound(SND_DEAL);
                    delay(2000);
                    inventory[drug] += amount;
                    cash -= (long)amount * price[drug];
                    if (avgCost[drug] == 0)
                        avgCost[drug] = price[drug];
                    else
                        avgCost[drug] = (price[drug] + avgCost[drug]) / 2;
                }
                break;
            /* ---- Sell drugs.  If you sell the whole stack the status changes by the
             * profit ratio ((price/avgCost - 1) * 3 * bonus).  "bonus" is the
             * factor 9, 6 or 3 for the three most expensive drugs (cocaine,
crack, heroin) and 1 for the rest.
             */
            case 'S':
#define profitBonus    bonus
                drawDrugMenu();
                do {
                    printColored(C_RED "What kinda drugs do\r\n ya want to unload? ", 1, 19);
                    setColor(YELLOW, BLACK, NORMAL);
                    escPressed = readField(1, 0, inputBuf);
                    if (escPressed == 1) break;
                    drug = atol(inputBuf);
                    if (drug < 1 || drug > NUM_DRUGS) playSound(SND_BEEP);
                } while (drug < 1 || drug > NUM_DRUGS);
                setCursor(0);
                if (escPressed == 1) continue;
                drug--;
                if (inventory[drug] == 0) {
                    printColored(C_LMAGENTA "Give me a break, man, you ain't got\r\nany of that!", 1, 22);
                    playSound(SND_DENIED);
                    delay(1000);
                    continue;
                }
                do {
                    printColored(C_RED "How many do ya wanna sell? ", 1, 22);
                    setColor(YELLOW, BLACK, NORMAL);
                    escPressed = readField(5, 0, inputBuf);
                    if (escPressed == 1) break;
                    amount = atol(inputBuf);
                } while (amount < 0 || inputBuf[0] == 0);
                setCursor(0);
                if (escPressed == 1) continue;
                if (inventory[drug] < amount) {
                    printColored(C_BLINK_MAGENTA "I happen to know that you ain't that\r\nmuch on ya, so quit trying to burn me!", 1, 24);
                    playSound(SND_DENIED);
                    delay(3000);
                    continue;
                }
                if (amount > 0) {
                    printColored(C_BLINK_YELLOW "You got yerself a deal, dude!", 1, 25);
                    playSound(SND_DEAL);
                    delay(2000);
                    if (inventory[drug] - amount == 0) {
                        if (drug < 3) profitBonus = (3 - drug) * 3;
                        else profitBonus = 1;
                        statusBefore = status;
                        status += (int)(((double)price[drug] / (double)(unsigned long)avgCost[drug] - 1.0) * 3.0 * profitBonus);
                        totalDeals++;
                        if (status > statusBefore) goodDeals++;
                        avgCost[drug] = 0;
                    }
                    inventory[drug] -= amount;
                    cash += (long)amount * price[drug];
                }
                break;
            /* ---- Visit the bank: Deposit or Withdraw.  ESC (27) cancels. */
#undef profitBonus
            case 'V':
                printColored(C_WHITE "You arrive at the First World Bank of America.\r\n\n               " C_LGREEN "Would ya like to (" C_BLINK_LRED "D" C_LGREEN ")eposit  some moola,\r\n                             or (" C_BLINK_LRED "W" C_LGREEN ")ithdraw some moola? ", 20, 18);
                setCursor(1);
                do {
                    choice = getch();
                    choice = toupper(choice);
                } while (choice != 'D' && choice != 'W' && choice != 27);
                if (choice == 27) continue;
                switch (choice) {
                case 'D':
                    printColored(C_LCYAN "How much 'ya gonna rathole? ", 5, 23);
                    break;
                case 'W':
                    printColored(C_LCYAN "How much 'ya gonna pull out? ", 5, 23);
                    break;
                }
                setColor(WHITE, BLACK, NORMAL);
                escPressed = readField(10, 0, inputBuf);
                setCursor(0);
                if (escPressed == 1) continue;
                moneyAmount = atol(inputBuf);
                if (moneyAmount <= 0) continue;
                switch (choice) {
                /* Deposit. */
                case 'D':
                    if (moneyAmount > cash) {
                        printColored(C_BLINK_LMAGENTA "You ain't got that much to deposit!", 25, 25);
                        playSound(SND_DENIED);
                        delay(1000);
                        continue;
                    } else {
                        printColored(C_BLINK_YELLOW "Done!", 37, 25);
                        playSound(SND_BEEP);
                        delay(2000);
                        cash -= moneyAmount;
                        bank += moneyAmount;
                        drawStatus();
                    }
                    continue;
                /* Withdraw. */
                case 'W':
                    if (moneyAmount > bank) {
                        printColored(C_BLINK_LMAGENTA "You ain't got that much to pull out!", 25, 25);
                        playSound(SND_DENIED);
                        delay(1000);
                    } else {
                        printColored(C_BLINK_YELLOW "Done!", 37, 25);
                        playSound(SND_BEEP);
                        delay(2000);
                        cash += moneyAmount;
                        bank -= moneyAmount;
                        drawStatus();
                    }
                    continue;
                }
                /* The two bank cases above always `continue`, so the code never runs
                 * through into the loan shark case; the original compiler simply laid the
                 * cases out this way, so we reproduce it.
                 */
                if (escPressed == 1) continue;
            case 'L':
                printColored(C_LCYAN "Ya set foot in the 'sharks office.  You've been in locker rooms that smelled\r\nbetter than dis place!  His boys are lookin' tough in their nice goon suits,\r\nbut you don't pay dem no never-mind, cause the head-honcho, Guido is talkin...\r\n\n                    " C_LGREEN "He say, you wanna (" C_BLINK_LRED "B" C_LGREEN ")orrow some cash,\r\n                      or do you wanna (" C_BLINK_LRED "P" C_LGREEN ")ay me back? ", 1, 17);
                setCursor(1);
                do {
                    choice = getch();
                    choice = toupper(choice);
                } while (choice != 'B' && choice != 'P' && choice != 27);
                setCursor(0);
                if (choice == 27) continue;
                switch (choice) {
                /* Borrow: refused if the shark is angry, if you have no status, below
                 * $1000, or if the total debt would be too large for your status
                 * (max debt = status * $10000).  The loan time (days) shrinks as the
                 * debt grows: log(status*10000 / total debt) / log(1.2) + 4.
                 */
                case 'B':
                    if (sharkAngry == 1) {
                        printColored(C_BLINK_LGREEN "\"Man, yer pushin' yer luck luck! I ought'ta kill you just for askin'!!\"", 5, 24);
                        delay(2000);
                        break;
                    }
                    if (status <= 0) {
                        printColored(C_LGREEN "\"Look, whoever you are, you're " C_BLINK_WHITE "NOBODY " C_LGREEN "to me.  Get outta here!\"", 5, 24);
                        delay(2000);
                        break;
                    }
                    clearBox(1, 21, 80, 25);
                    printColored(C_LGREEN "How much ya wanna borrow? ", 10, 22);
                    escPressed = readField(10, 0, inputBuf);
                    setCursor(0);
                    if (escPressed == 1) continue;
                    moneyAmount = atol(inputBuf);
                    if (moneyAmount <= 0) continue;
                    if (moneyAmount < 1000) {
                        printColored(C_BLINK_LRED "\"Sorry, bud, but I got a " C_BLINK_YELLOW "$1000 " C_BLINK_LRED "minimum.  Got no time for penny-ante stuff!\"", 1, 25);
                        playSound(SND_DENIED);
                        delay(1000);
                        continue;
                    }
                    if (moneyAmount / 10000 >= status) {
                        printColored(C_BLINK_LRED "\"No deal, too big a risk for me! I'm a businessman, ya'know!\"", 10, 25);
                        playSound(SND_DENIED);
                        delay(1000);
                        continue;
                    }
                    loanTerm = (int)((log((double)status * 10000.0) - log((double)(debt + moneyAmount))) / 0.182321556794) + 4;
                    if ((moneyAmount + debt) / 10000 > status || (loanTerm >= loanDays && hasLoan == 1)) {
                        printColored(C_LCYAN "\"Look, you already owe me ", 1, 24);
                        setColor(RED, BLACK, BOLD);
                        cprintf("%ld ", debt);
                        setColor(LIGHTCYAN, BLACK, NORMAL);
                        cputs("bucks, I'm not givin ya THAT much more!!!");
                        printColored(C_LCYAN "Christ, what do I look like, Santi-Claus?\"", 20, 25);
                        playSound(SND_DENIED);
                        delay(2000);
                        continue;
                    } else {
                        printColored(C_LMAGENTA "\"Alright, I'll loan ya de money, but if you welch, my boys will come lookin\r\nfor ya! I expect to see you here to pay me in full in ", 1, 24);
                        debt += moneyAmount;
                        cash += moneyAmount;
                        hasLoan = 1;
                        loanDays = loanTerm;
                        setColor(RED, BLACK, BOLD);
                        cprintf("%d", loanDays - 1);
                        printColored(C_LMAGENTA " days.\"", 99, 99);
                        playSound(SND_BEEP);
                        delay(2000);
                        break;
                    }
                /* Pay back all or part of the debt; paying it in full clears
                 * the deadline and the shark's anger.
                 */
                case 'P':
                    if (debt == 0) {
                        printColored(C_LCYAN "\"My books show dat you don't owe me nut'in... would youse like to take out\r\na little loan?\"", 1, 24);
                        playSound(SND_DENIED);
                        delay(2000);
                        continue;
                    }
                    clearBox(1, 21, 80, 25);
                    printColored(C_LGREEN "How much ya wanna pay back? ", 10, 22);
                    escPressed = readField(10, 0, inputBuf);
                    setCursor(0);
                    if (escPressed == 1) continue;
                    moneyAmount = atol(inputBuf);
                    if (moneyAmount <= 0) continue;
                    if (moneyAmount > cash) {
                        printColored(C_BLINK_LRED "'fraid you don't got that in cash, my man.", 20, 25);
                        playSound(SND_DENIED);
                        delay(1000);
                        continue;
                    }
                    if (moneyAmount >= debt) debt = 0;
                    else debt -= moneyAmount;
                    cash -= moneyAmount;
                    if (debt > 0) {
                        printColored(C_LCYAN "\"OK, but you still owe me ", 20, 24);
                        setColor(LIGHTRED, BLACK, BOLD);
                        cprintf("%ld ", debt);
                        printColored(C_LCYAN "buck", 99, 99);
                        if (debt > 1) cputs("s!\"");
                        else cputs("!\"");
                        playSound(SND_DENIED);
                        delay(2000);
                        break;
                    } else {
                        printColored(C_LCYAN "\"Paid in full? Lucky for you, eh boys? Anytime yous need a loan, just come\r\nto me, I'll take care of ya...\"", 1, 24);
                        loanDays = 0;
                        hasLoan = 0;
                        sharkAngry = 0;
                        beatings = 0;
                        playSound(SND_BEEP);
                        delay(2000);
                        break;
                    }
                }
                break;
            /* ---- Hospital: costs $10 per missing hit point and takes 1 day per $15
             * (bonus is reused here as the bill).
             */
            case 'H':
#define hospitalBill   bonus
                if (health == MAX_HEALTH) {
                    printColored(C_WHITE "Ya have de doc check yous out, but aside from yer bunions\r\n              on yer feet, youse is fit as a fiddle!", 15, 19);
                    pressAnyKey();
                    continue;
                }
                hospitalBill = (MAX_HEALTH - health) * 10;
                printColored(C_WHITE "You drag yer sorry butt into de loc'l Hospital.  The docs check ye\r\n   out, and come to the conclusion that it'll cost ", 4, 19);
                setColor(LIGHTGREEN, BLACK, BOLD);
                cprintf("%d", hospitalBill);
                printColored(C_WHITE " dollars to fix ya up.", 99, 99);
                printColored(C_LMAGENTA "Do ya sign the consent forms? (Y/N) \020 ", 5, 22);
                setCursor(1);
                do {
                    key = toupper(getch());
                } while (key != 'Y' && key != 'N' && key != 27);
                setCursor(0);
                if (key == 'Y') {
                    if ((long)hospitalBill > cash) {
                        printColored(C_BLINK_LRED "You don't have enough dinero up front to foot de bill!", 12, 24);
                        playSound(SND_DENIED);
                        pressAnyKey();
                    } else {
                        cash -= hospitalBill;
                        printColored(C_YELLOW "OK, yer laid up for ", 5, 24);
                        setColor(LIGHTRED, BLACK, BOLD);
                        cprintf("%d", hospitalBill / 15);
                        printColored(C_YELLOW " days recoup'ratin', and yous feel good as new!", 99, 99);
                        playSound(SND_BEEP);
                        for (i = 1; i < hospitalBill / 15; i++)
                            nextDay();
                        health = MAX_HEALTH;
                        pressAnyKey();
                    }
                }
                break;
            /* ---- Quit the game (ask for confirmation). */
#undef hospitalBill
            case 'Q':
                printColored(C_LGRAY "Are ya sure ya wanna " C_BLINK_BROWN "chicken out " C_LGRAY "an' " C_BLINK_RED "QUIT? " C_LGRAY "(Y/N) ", 15, 20);
                setCursor(1);
                do {
                    choice = toupper(getch());
                } while (choice != 'Y' && choice != 'N');
                if (choice == 'N') continue;
                running = 0;
                break;
            }
            /* Every action ends the day: update the calendar and the display, then
             * see what the day brings.
             */
            nextDay();
            drawStatus();
            if (dead || !running) break;
            /* Rare events for high rollers: trenchcoat (7) or gun smuggler (8). */
            if (rand() % 10 == 0 && rand() % (status / 20 + 1) > 4) {
                if (rand() % 2 == 0) randomEvent(EV_TRENCHCOAT);
                else randomEvent(EV_GUN_DEALER);
            }
            /* The cops: always after a refused blackmail, otherwise sometimes
             * when your status is 100 or more.
             */
            if (copChase == 1 || (status >= 100 && rand() % 400 < status && rand() % 8 == 0))
                randomEvent(EV_COPS);
            if (dead || !running) break;
            /* One in five days has an ordinary random event. */
            if (rand() % 5 == 0)
                randomEvent(rand() % NUM_EVENTS);
            if (dead || !running) break;
            /* Loan overdue: the shark's thugs beat you up (25 damage per beating so
             * far, one more each time).  You get a week to pay, or die.
             */
            if (loanDays < 0 && hasLoan == 1) {
                clearBox(1, 17, 80, 25);
                printColored(C_LCYAN "You're ambushed in a dark alley by Guido's thugs, must be 'bout ten guys!\r\nThe boss-man say " C_BLINK_LGRAY "\"Oh, man, you should'na missed de deadline, now me and\r\nboys are gonna have to break your legs! Maybe next time you'll learn, eh?\"" C_LCYAN "\r\nDey take yous from all directions, you ain't gotta chance...\r\nOne steps out with a " C_BLINK_BROWN "baseball bat " C_LCYAN "and...\r\n", 1, 17);
                delay(3000);
                beatings++;
                for (i = 0; i < beatings; i++) {
                    clearBox(1, 23, 80, 23);
                    delay(500);
                    printColored(C_LRED "* * *    " C_BLINK_RED "C R A C K ! !   " C_LRED "* * *", 25, 23);
                    health -= 25;
                    sharkAngry = 1;
                    drawStatus();
                    setCursor(0);
                    playSound(SND_DENIED);
                    delay(1500);
                }
                if (health > 0) {
                    printColored(C_YELLOW "\"Oh, one more 'ting... you got a week to come up with dinero, or we'll\r\n be back, comprende?\"", 1, 24);
                    delay(3000);
                    loanDays = 7;
                } else
                    dead = 1;
                pressAnyKey();
            }
            if (dead || !running) break;
            drawStatus();
        }
        /* Game over: died, or reached the winning status. */
        if (running) {
            if (dead) showDeathScreen();
            else showEndGame();
        }
        running = askPlayAgain();
    } while (running);
    /* Goodbye screen and exit. */
    clrscr();
    puttext(3, 2, 77, 20, farewellScreen);
    printColored(C_BLINK_YELLOW "Thank you for playing DrugLord.", 25, 22);
    printColored("Send For The Registered Version Today!", 22, 23);
    setCursor(1);
    gotoxy(1, 24);
    exit(0);
}

