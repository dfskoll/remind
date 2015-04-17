/***************************************************************/
/*                                                             */
/*  REM2PS.C                                                   */
/*                                                             */
/*  Print a PostScript calendar.                               */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by Dianne Skoll                    */
/*  Copyright (C) 1999-2000 by Roaring Penguin Software Inc.   */
/*                                                             */
/***************************************************************/

#include "version.h"
#include "config.h"
#include "dynbuf.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>
#include <stdlib.h>
#include "rem2ps.h"

#define NEW(type) (malloc(sizeof(type)))

#define SPECIAL_NORMAL     0
#define SPECIAL_POSTSCRIPT 1
#define SPECIAL_PSFILE     2
#define SPECIAL_MOON       3
#define SPECIAL_COLOR      4
#define SPECIAL_WEEK       5
#define SPECIAL_SHADE      6

/* Array holding how specials sort */
static int SpecialSortOrder[] = {
    0, /* NORMAL */
    1, /* POSTSCRIPT */
    1, /* PSFILE */
    2, /* MOON */
    0, /* COLOR */
    4, /* WEEK */
    5  /* SHADE */
};

typedef struct calentry {
    struct calentry *next;
    int special;
    char *entry;
} CalEntry;

typedef struct {
    char const *name;
    int xsize, ysize;
} PageType;

char DayName[7][33];

char const *SmallCalLoc[] = {
    "",
    "bt",
    "tb",
    "sbt",
};

#define NUMSMALL (sizeof(SmallCalLoc)/sizeof(SmallCalLoc[0]))
char const *SmallLocation;
int SmallCol1, SmallCol2;

PageType Pages[] =
{
    {"Letter", 612, 792},     /* 8.5 x 11 in. */
    {"Tabloid", 792, 1224},   /* 11 x 17 in. */
    {"Ledger", 1224, 792},    /* 17 x 11 in. */
    {"Legal", 612, 1008},     /* 8.5 x 14 in. */
    {"Statement", 396, 612},  /* 5.5 x 8.5 in. */
    {"Executive", 540, 720},  /* 7.5 x 10 in. */
    {"A3", 842, 1190},
    {"A4", 595, 842},
    {"A5", 420, 595},
    {"B4", 729, 1032},
    {"B5", 519, 729},
    {"Folio", 612, 936},
    {"Quarto", 612, 780},
    {"10x14", 720, 1008},
    {"-custom-", 0, 0}
};

PageType DefaultPage[1] =
{
    DEFAULT_PAGE
};

#define NUMPAGES (sizeof(Pages)/sizeof(Pages[0]))

CalEntry *CurEntries = NULL;
CalEntry *PsEntries[32];
PageType *CurPage;
char PortraitMode;
char NoSmallCal;
char UseISO;

char const *HeadFont="Helvetica";
char const *TitleFont="Helvetica";
char const *DayFont="Helvetica-BoldOblique";
char const *EntryFont="Helvetica";
char const *SmallFont="Helvetica";
char const *LineWidth = "1";

char const *HeadSize="14";
char const *TitleSize="14";
char const *DaySize="14";
char const *EntrySize="8";
char const *BorderSize = "6";

char const *UserProlog = NULL;

int validfile = 0;

int CurDay;
int MaxDay;
int DayNum;
int WkDayNum;
int FirstWkDay;
int MondayFirst;
int LeftMarg, RightMarg, TopMarg, BotMarg;
int FillPage;
int Verbose = 0;

void Init (int argc, char *argv[]);
void Usage (char const *s);
void DoPsCal (void);
int DoQueuedPs (void);
void DoSmallCal (char const *m, int days, int first, int col, int which);
void WriteProlog (void);
void WriteCalEntry (void);
void WriteOneEntry (CalEntry *c);
void GetSmallLocations (void);
char const *EatToken(char const *in, char *out, int maxlen);

/***************************************************************/
/*                                                             */
/*   MAIN PROGRAM                                              */
/*                                                             */
/***************************************************************/
int main(int argc, char *argv[])
{
    /* If stdin is a tty - probably wrong. */

    DynamicBuffer buf;
    DBufInit(&buf);
    Init(argc, argv);

    if (isatty(0)) {
	Usage("Input should not come from a terminal");
    }

    /* Search for a valid input file */
    while (!feof(stdin)) {
	DBufGets(&buf, stdin);
	if (!strcmp(DBufValue(&buf), PSBEGIN)) {
	    if (!validfile) {
		if (Verbose) {
		    fprintf(stderr, "Rem2PS: Version %s Copyright 1992-1998 by Dianne Skoll\n\n", VERSION);
		    fprintf(stderr, "Generating PostScript calendar\n");
		}
	    }
	    validfile++;
	    DoPsCal();
	}
    }
    if (!validfile) {
	fprintf(stderr, "Rem2PS: Couldn't find any calendar data - are you\n");
	fprintf(stderr, "        sure you fed me input produced by remind -p ...?\n");
	exit(1);
    }
    printf("%%%%Trailer\n");
    printf("%%%%Pages: %d\n", validfile);
    if (Verbose) fprintf(stderr, "Rem2PS: Done\n");
    return 0;
}

/***************************************************************/
/*                                                             */
/*  DoPsCal - emit PostScript for the calendar.                */
/*                                                             */
/***************************************************************/
void DoPsCal(void)
{
    char month[40], year[40];
    char prevm[40], nextm[40];
    int days, wkday, prevdays, nextdays;
    int sfirst;
    int i;
    int is_ps;
    int firstcol;
    char const *startOfBody;
    char passthru[PASSTHRU_LEN+1];
    DynamicBuffer buf;
    CalEntry *c, *d, *p;

/* Read the month and year name, followed by # days in month and 1st day of
   month */
    DBufInit(&buf);
    DBufGets(&buf, stdin);
    sscanf(DBufValue(&buf), "%s %s %d %d %d", month, year, &days, &wkday,
	   &MondayFirst);

    /* Get day names */
    DBufGets(&buf, stdin);
    sscanf(DBufValue(&buf), "%32s %32s %32s %32s %32s %32s %32s",
	   DayName[0], DayName[1], DayName[2], DayName[3],
	   DayName[4], DayName[5], DayName[6]);

    /* We write the prolog here because it's only at this point that
       MondayFirst is set correctly. */
    if (validfile == 1) {
	WriteProlog();
    }

    DBufGets(&buf, stdin);
    sscanf(DBufValue(&buf), "%s %d", prevm, &prevdays);
    DBufGets(&buf, stdin);
    sscanf(DBufValue(&buf), "%s %d", nextm, &nextdays);
    DBufFree(&buf);
    MaxDay = days;
    FirstWkDay = wkday;

/* Print a message for the user */
    if (Verbose) fprintf(stderr, "        %s %s\n", month, year);

    printf("%%%%Page: %c%c%c%c%c %d\n", month[0], month[1], month[2],
	   year[2], year[3], validfile);

/* Emit PostScript to do the heading */
    if (!PortraitMode) printf("90 rotate 0 XSIZE neg translate\n");
    printf("/SAVESTATE save def (%s) (%s) PreCal SAVESTATE restore\n", month, year);
    printf("(%s %s) doheading\n", month, year);

/* Figure out the column of the first day in the calendar */

    if (MondayFirst) {
	firstcol = wkday-1;
	if (firstcol < 0) firstcol = 6;
    } else {
	firstcol = wkday;
    }

/* Calculate the minimum box size */
    if (!FillPage) {
	printf("/MinBoxSize ytop MinY sub 7 div def\n");
    } else {
	if ((days == 31 && firstcol >= 5) || (days == 30 && firstcol == 6))
	    printf("/MinBoxSize ytop MinY sub 6 div def\n");
	else if (days == 28 && firstcol == 0 && NoSmallCal)
	    printf("/MinBoxSize ytop MinY sub 4 div def\n");
	else
	    printf("/MinBoxSize ytop MinY sub 5 div def\n");
    }

    printf("/ysmalltop ytop def\n");

/* Do each entry */

    CurEntries = NULL;
    CurDay = 1;
    WkDayNum = wkday;

    while(1) {
	if (feof(stdin)) {
	    fprintf(stderr, "Input from REMIND is corrupt!\n");
	    exit(1);
	}

	DBufGets(&buf, stdin);
	if (!strcmp(DBufValue(&buf), PSEND)) {
	    DBufFree(&buf);
	    break;
	}

	/* Ignore lines beginning with '#' */
	if (DBufValue(&buf)[0] == '#') {
	    continue;
	}
	/* Read the day number - a bit of a hack! */
	DayNum = (DBufValue(&buf)[8] - '0') * 10 + DBufValue(&buf)[9] - '0';
	if (DayNum != CurDay) {
	    for(; CurDay<DayNum; CurDay++) {
		WriteCalEntry();
		WkDayNum = (WkDayNum + 1) % 7;
	    }
	}
	/* Add the text */
	c = NEW(CalEntry);
	if (!c) {
	    fprintf(stderr, "malloc failed - aborting.\n");
	    exit(1);
	}
	c->next = NULL;
	c->special = SPECIAL_NORMAL;

	/* Skip the tag, duration and time */
	startOfBody = DBufValue(&buf)+10;

	/* Eat the passthru */
	startOfBody = EatToken(startOfBody, passthru, PASSTHRU_LEN);

	/* Eat the tag */
	startOfBody = EatToken(startOfBody, NULL, 0);

	/* Eat the duration */
	startOfBody = EatToken(startOfBody, NULL, 0);

	/* Eat the time */
	startOfBody = EatToken(startOfBody, NULL, 0);

	is_ps = 0;
	if (!strcmp(passthru, "PostScript") ||
	    !strcmp(passthru, "PSFile") ||
	    !strcmp(passthru, "MOON") ||
	    !strcmp(passthru, "WEEK") ||
	    !strcmp(passthru, "SHADE")) {
	    is_ps = 1;
	}
	c->entry = malloc(strlen(startOfBody) + 1);
	if (!c->entry) {
	    fprintf(stderr, "malloc failed - aborting.\n");
	    exit(1);
	}
	strcpy(c->entry, startOfBody);

	if (is_ps) {
	    /* Save the type of SPECIAL */
	    if (!strcmp(passthru, "PostScript")) {
		c->special = SPECIAL_POSTSCRIPT;
	    } else if (!strcmp(passthru, "SHADE")) {
		c->special = SPECIAL_SHADE;
	    } else if (!strcmp(passthru, "MOON")) {
		c->special = SPECIAL_MOON;
	    } else if (!strcmp(passthru, "WEEK")) {
		c->special = SPECIAL_WEEK;
	    } else {
		c->special = SPECIAL_PSFILE;
	    }

	    if (!PsEntries[DayNum]) {
		PsEntries[DayNum] = c;
	    } else {
		d = PsEntries[DayNum];
		p = NULL;
		/* Slot it into the right place */
		while (d->next && (SpecialSortOrder[c->special] <= SpecialSortOrder[d->special])) {
		    p = d;
		    d = d->next;
		}
		if (SpecialSortOrder[c->special] <= SpecialSortOrder[d->special]) {
		    c->next = d->next;
		    d->next = c;
		} else {
		    if (p) {
			p->next = c;
		    } else {
			PsEntries[DayNum] = c;
		    }
		    c->next = d;
		}
	    }
	} else if (!strcmp(passthru, "*") ||
		   !strcmp(passthru, "COLOUR") ||
	           !strcmp(passthru, "COLOR")) {
	    /* Put on linked list */
	    if (!CurEntries) {
		CurEntries = c;
	    } else {
		d = CurEntries;
		while(d->next) d = d->next;
		d->next = c;
	    }
	    if (!strcmp(passthru, "COLOR") ||
		!strcmp(passthru, "COLOUR")) {
		c->special = SPECIAL_COLOR;
	    }
	}
    }
    for(; CurDay<=days; CurDay++) {
	WriteCalEntry();
	WkDayNum = (WkDayNum + 1) % 7;
    }

/* If wkday < 2, set ysmall.  If necessary (only for feb) increase cal size. */
    printf("/ysmallbot ylast def\n");

/* Now draw the vertical lines */
    GetSmallLocations();
    for (i=0; i<=7; i++) {
	printf("%d xincr mul MinX add ymin %d xincr mul MinX add topy L\n",
	       i, i);
    }

/* print the small calendars */
    if (!NoSmallCal) {
	sfirst = wkday - (prevdays % 7);
	if (sfirst < 0) sfirst += 7;
	DoSmallCal(prevm, prevdays, sfirst, SmallCol1, 1);
	sfirst = wkday + (days % 7);
	if (sfirst >6) sfirst -= 7;
	DoSmallCal(nextm, nextdays, sfirst, SmallCol2, 2);
    }
/* Do it! */
    printf("showpage\n");
}

/***************************************************************/
/*                                                             */
/*  WriteProlog - write the PostScript prologue                */
/*                                                             */
/***************************************************************/
void WriteProlog(void)
{
    int i;
    int x = CurPage->xsize;
    int y = CurPage->ysize;
    char const *isostuff;
    FILE *fp;
    int nread;
    char buffer[512];

    if (!PortraitMode) {
	i = x; x = y; y = i;
    }

    if (UseISO)
	isostuff = "reencodeISO";
    else
	isostuff = "copyFont";

/* Write the document structuring stuff */
    printf("%%!PS-Adobe-2.0\n");
    printf("%%%%DocumentFonts: %s", HeadFont);
    if (strcmp(TitleFont, HeadFont)) printf(" %s", TitleFont);
    if (strcmp(TitleFont, DayFont) &&
	strcmp(HeadFont, DayFont)) printf(" %s", DayFont);
    if (strcmp(EntryFont, HeadFont) &&
	strcmp(TitleFont, EntryFont) &&
	strcmp(EntryFont, DayFont)) printf(" %s", EntryFont);
    if (!NoSmallCal && strcmp(SmallFont, HeadFont) &&
	strcmp(SmallFont, DayFont)  &&
	strcmp(TitleFont, SmallFont) &&
	strcmp(SmallFont, EntryFont)) printf(" %s", SmallFont);
    PutChar('\n');
    printf("%%%%Creator: Rem2PS\n");
    printf("%%%%Pages: (atend)\n");
    printf("%%%%Orientation: %s\n", PortraitMode ? "Portrait" : "Landscape");
    printf("%%%%EndComments\n");

    for (i=0; PSProlog1[i]; i++) puts(PSProlog1[i]);
    if (!MondayFirst)
	printf("[(%s) (%s) (%s) (%s) (%s) (%s) (%s)]\n",
	       DayName[0], DayName[1], DayName[2], DayName[3],
	       DayName[4], DayName[5], DayName[6]);
    else
	printf("[(%s) (%s) (%s) (%s) (%s) (%s) (%s)]\n",
	       DayName[1], DayName[2], DayName[3],
	       DayName[4], DayName[5], DayName[6], DayName[0]);
    for (i=0; PSProlog2[i]; i++) puts(PSProlog2[i]);

    printf("/HeadFont /%s %s\n", HeadFont, isostuff);
    if (!NoSmallCal) printf("/SmallFont /%s %s\n", SmallFont, isostuff);
    printf("/DayFont /%s %s\n", DayFont, isostuff);
    printf("/EntryFont /%s %s\n", EntryFont, isostuff);
    printf("/TitleFont /%s %s\n", TitleFont, isostuff);
    printf("/HeadSize %s def\n", HeadSize);
    printf("/DaySize %s def\n", DaySize);
    printf("/EntrySize %s def\n", EntrySize);
    printf("/TitleSize %s def\n", TitleSize);
    printf("/XSIZE %d def\n", CurPage->xsize);
    printf("/MinX %d def\n", LeftMarg);
    printf("/MinY %d def\n", BotMarg);
    printf("/MaxX %d def\n", x-RightMarg);
    printf("/MaxY %d def\n", y-TopMarg);
    printf("/Border %s def\n", BorderSize);
    printf("/LineWidth %s def\n", LineWidth);
    printf("%s setlinewidth\n", LineWidth);

/* Check if smallfont is fixed pitch */
    if (!NoSmallCal) {
	printf("/SmallFont findfont /FontInfo get /isFixedPitch get\n");

/* Define SmallString used to set smallfont size */
	printf("{/SmallString (WW ) def}\n");
	printf("{/SmallString (WW) def}\nifelse\n");
    }

/* Do the user-supplied prolog file, if any */
    if (UserProlog) {
	fp = fopen(UserProlog, "r");
	if (!fp) {
	    fprintf(stderr, "Could not open prologue file `%s'\n", UserProlog);
	} else {
	    while(1) {
		nread = fread(buffer, sizeof(char), 512, fp);
		if (!nread) break;
		fwrite(buffer, sizeof(char), nread, stdout);
	    }
	    fclose(fp);
	}
    }

    printf("%%%%EndProlog\n");


}

/***************************************************************/
/*                                                             */
/*  WriteCalEntry - write all entries for one day              */
/*                                                             */
/***************************************************************/
void WriteCalEntry(void)
{
    CalEntry *c = CurEntries;
    CalEntry *d;
    int begin, end, i, HadQPS;

/* Move to appropriate location */
    printf("/CAL%d {\n", CurDay);
    if (!MondayFirst)
	printf("Border ytop %d xincr mul MinX add xincr\n", WkDayNum);
    else
	printf("Border ytop %d xincr mul MinX add xincr\n", (WkDayNum ? WkDayNum-1 : 6));

/* Set up the text array */
    printf("[\n");

    CurEntries = NULL;

    while(c) {
	WriteOneEntry(c);
	free(c->entry);
	d = c->next;
	free(c);
	c = d;
    }
    printf("]\n");

/* Print the day number */
    printf("(%d)\n", CurDay);
/* Do it! */
    printf("DoCalBox\n");

/* Update ymin */
    printf("/y exch def y ymin lt {/ymin y def} if\n");
    printf("} def\n");

/* If WkDayNum is a Sunday or Monday, depending on MondayFirst,
   move to next row.  Also handle the queued PS and PSFILE reminders */
    if ((!MondayFirst && WkDayNum == 6) ||
	(MondayFirst && WkDayNum == 0) || CurDay == MaxDay) {
	HadQPS = 0;
	if (MondayFirst) begin =  CurDay - (WkDayNum ? WkDayNum-1 : 6);
	else             begin = CurDay - WkDayNum;
	if (begin < 1) begin = 1;
	end = CurDay;
	for (i=begin; i<=end; i++) {
	    if (PsEntries[i]) {
		HadQPS = 1;
		break;
	    }
	}
	/* Avoid problems with blotching if PS printer has roundoff errors */
	if (HadQPS) printf("1 setgray\n");
	for (i=begin; i<=end; i++) {
	    printf("CAL%d\n", i);
	}
	if (HadQPS) printf("0 setgray\n");
	printf("/y ytop MinBoxSize sub def y ymin lt {/ymin y def} if\n");

/* Draw the line at the bottom of the row */
	printf("MinX ymin MaxX ymin L\n");

/* Update ytop */
	printf("/ylast ytop def\n");
	printf("/ytop ymin def\n");

	(void) DoQueuedPs();

/* Re-do the calendar stuff if there was any included PS code */
	if (HadQPS) {
	    printf("/ytop ylast def\n");
	    for (i=begin; i<=end; i++) {
		printf("CAL%d\n", i);
	    }
	    printf("/y ytop MinBoxSize sub def y ymin lt {/ymin y def} if\n");
	    printf("MinX ymin MaxX ymin L\n");
	    printf("/ylast ytop def\n");
	    printf("/ytop ymin def\n");
	}
    }
}

/***************************************************************/
/*                                                             */
/*  WriteOneEntry - write an entry for one day                 */
/*                                                             */
/***************************************************************/
void WriteOneEntry(CalEntry *c)
{
    int ch, i;
    char const *s = c->entry;

    printf("  [");

    /* Chew up leading spaces */
    while(isspace((unsigned char) *s)) s++;

    /* Skip three decimal numbers for COLOR special */
    if (c->special == SPECIAL_COLOR) {
	for (i=0; i<3; i++) {
	    while(*s && !isspace(*s)) s++;
	    while(*s && isspace(*s)) s++;
	}
    }

    PutChar('(');
    while(*s) {
	/* Use the "unsigned char" cast to fix problem on Solaris 2.5 */
	/* which treated some latin1 characters as white space.       */
	ch = (unsigned char) *s++;
	if (ch == '\\' || ch == '(' || ch == ')') PutChar('\\');
	if (!isspace(ch)) PutChar(ch);
	else {
	    PutChar(')');
	    while(isspace((unsigned char)*s)) s++;
	    if (!*s) {
		goto finish;
	    }
	    PutChar('(');
	}
    }
    printf(")\n");
  finish:
    if (c->special == SPECIAL_COLOR) {
	int r, g, b;
	if (sscanf(c->entry, "%d %d %d", &r, &g, &b) == 3) {
	    if (r < 0) r = 0;
	    else if (r > 255) r = 255;
	    if (g < 0) g = 0;
	    else if (g > 255) g = 255;
	    if (b < 0) b = 0;
	    else if (b > 255) b = 255;
	    printf("(gsave %f %f %f setrgbcolor)(grestore)",
		   r / 255.0, g / 255.0, b / 255.0);
	} else {
	    /* Punt... unrecognized color is black */
	    printf("()()");
	}
    } else {
	printf("()()");
    }
    printf("]\n");
}

/***************************************************************/
/*                                                             */
/*  Init - set up parameters                                   */
/*                                                             */
/***************************************************************/
void Init(int argc, char *argv[])
{
    char const *s;
    char const *t;
    int i=1;
    int j;
    int offset;

    PortraitMode = 1;
    NoSmallCal = 0;
    LeftMarg = 36;
    RightMarg = 36;
    TopMarg = 36;
    BotMarg = 36;
    UseISO = 0;
    FillPage = 0;
    MondayFirst = 0;
    SmallLocation = "bt";

    for(j=0; j<32; j++) PsEntries[i] = NULL;

    CurPage = DefaultPage;  /* Letter size by default */

    while (i < argc) {
	s = argv[i];
	i++;

	if (*s++ != '-') Usage("Options must begin with `-'");

	switch(*s++) {

	case 'p':
	    if (i == argc) Usage("Prologue filename must be supplied");
	    UserProlog = argv[i++];
	    break;

	case 's':
	    if (i == argc) Usage("Size must be supplied");
	    t = argv[i++];
	    while(*s) {
		switch(*s++) {
		case 'h': HeadSize = t; break;
		case 'e': EntrySize = t; break;
		case 'd': DaySize = t; break;
		case 't': TitleSize = t; break;
		default: Usage("Size must specify h, t, e, or d");
		}
	    }
	    break;

	case 'f':
	    if (i == argc) Usage("Font must be supplied");
	    t = argv[i++];
	    while(*s) {
		switch(*s++) {
		case 'h': HeadFont = t; break;
		case 'e': EntryFont = t; break;
		case 'd': DayFont = t; break;
		case 's': SmallFont = t; break;
		case 't': TitleFont = t; break;
		default: Usage("Font must specify s, h, t, e, or d");
		}
	    }
	    break;

	case 'v':
	    Verbose = 1;
	    break;

	case 'm':
	    if (i == argc) Usage("Media must be supplied");
	    t = argv[i++];
	    CurPage = NULL;
	    for (j=0; j<NUMPAGES-1; j++)
		if (!strcmp(t, Pages[j].name)) {
		    CurPage = &Pages[j];
		    break;
		}

	    if (!CurPage) {
		double w, h;
		if (sscanf(t, "%lfx%lfin", &w, &h) == 2) {
		    CurPage = &Pages[NUMPAGES-1];
		    CurPage->xsize = (int) (w * 72.0);
		    CurPage->ysize = (int) (h * 72.0);
		} else if (sscanf(t, "%lfx%lfcm", &w, &h) == 2) {
		    CurPage = &Pages[NUMPAGES-1];
		    CurPage->xsize = (int) ((double) w * 28.346457);
		    CurPage->ysize = (int) ((double) w * 28.346457);
		}
	    }
	    if (!CurPage) {
		fprintf(stderr, "\nUnknown media specified.\n");
		fprintf(stderr, "\nAvailable media types:\n");
		for (j=0; j<NUMPAGES-1; j++) {
		    fprintf(stderr, "   %s\n", Pages[j].name);
		}
		fprintf(stderr, "   WxHin  Specify size in inches (W and H are decimal numbers)\n");
		fprintf(stderr, "   WxHcm  Specify size in centimetres (W and H are decimal numbers)\n");
		fprintf(stderr, "Default media type is %s\n", DefaultPage[0].name);
		exit(1);
	    }
	    break;

	case 'o':
	    if (i == argc) Usage("Offset must be supplied");
	    offset = atoi(argv[i++]);
	    if (offset < 0) offset = 0;
	    if (!*s) Usage("Offset must specify l, r, t or b");
	    while(*s) {
		switch(*s++) {
		case 'l': LeftMarg = offset; break;
		case 'r': RightMarg = offset ; break;
		case 't': TopMarg = offset; break;
		case 'b': BotMarg = offset; break;
		default: Usage("Offset must specify l, r, t or b");
		}
	    }
	    break;

	case 'b':
	    if (i == argc) Usage("Border must be supplied");
	    BorderSize = argv[i++];
	    break;

	case 't':
	    if (i == argc) Usage("Line thickness must be supplied");
	    LineWidth = argv[i++];
	    break;

	case 'l': PortraitMode = 0; break;

	case 'i': UseISO = 1; break;

	case 'c': j=(*s);
	    if (!j) {
		SmallLocation = SmallCalLoc[0];
	    } else {
		j -= '0';
		if (j>=0 && j<NUMSMALL) {
		    SmallLocation = SmallCalLoc[j];
		} else {
		    SmallLocation = SmallCalLoc[0];
		}
	    }
	    break;

	case 'e': FillPage = 1; break;

	default: Usage("Unrecognized option");
	}
    }
}

/***************************************************************/
/*                                                             */
/*  Usage - print usage information                            */
/*                                                             */
/***************************************************************/
void Usage(char const *s)
{
    if (s) fprintf(stderr, "Rem2PS: %s\n\n", s);

    fprintf(stderr, "Rem2PS: Produce a PostScript calendar from output of Remind.\n\n");
    fprintf(stderr, "Usage: rem2ps [options]\n\n");
    fprintf(stderr, "Options:\n\n");
    fprintf(stderr, "-v            Print progress messages to standard error\n");
    fprintf(stderr, "-p file       Include user-supplied PostScript code in prologue\n");
    fprintf(stderr, "-l            Do calendar in landscape mode\n");
    fprintf(stderr, "-c[n]         Control small calendars: 0=none; 1=bt; 2=tb; 3=sbt\n");
    fprintf(stderr, "-i            Use ISO 8859-1 encoding in PostScript output\n");
    fprintf(stderr, "-m media      Set page size (eg, Letter, Legal, A4.)  Case sensitive!\n");
    fprintf(stderr, "              (Default page size is %s)\n", DefaultPage[0].name);
    fprintf(stderr, "-f[shted] font Set font for small cal, hdr, title, cal entries, day numbers\n");
    fprintf(stderr, "-s[hted] size Set size for header, title, calendar entries and/or day numbers\n");
    fprintf(stderr, "-b size       Set border size for calendar entries\n");
    fprintf(stderr, "-t size       Set line thickness\n");
    fprintf(stderr, "-e            Make calendar fill entire page\n");
    fprintf(stderr, "-o[lrtb] marg Specify left, right, top and bottom margins\n");
    exit(1);
}

/***************************************************************/
/*                                                             */
/*  DoSmallCal - do the small calendar for previous or next    */
/*  month.                                                     */
/*                                                             */
/***************************************************************/
void DoSmallCal(char const *m, int days, int first, int col, int which)
{
    /* Do the small calendar */
    int i, j;
    int row = 2;

    if (MondayFirst) {
	first--;
	if (first < 0) first = 6;
    }
    /* Figure out the font size */

    printf("/SmallFontSize MinBoxSize Border sub Border sub 8 div 2 sub def\n");
    printf("/SmallFont findfont setfont\n");
    printf("SmallString stringwidth pop /SmallWidth exch def\n");
    printf("SmallWidth 7 mul xincr Border sub Border sub exch div /tmp exch def\n");
    printf("tmp SmallFontSize lt {/SmallFontSize tmp def} if\n");
    printf("/SmallFont findfont SmallFontSize scalefont setfont\n");

   /* Recalculate SmallWidth */
    printf("SmallString stringwidth pop /SmallWidth exch def\n");

    /* Save graphics state */
    printf("gsave\n");

    /* Move origin to upper-left hand corner of appropriate box */
    printf("%d xincr mul MinX add ysmall%d translate\n", col, which);

    /* Print the month */
    printf("SmallWidth 7 mul (%s) stringwidth pop sub 2 div Border add Border neg SmallFontSize sub moveto (%s) show\n", m, m);

    /* Print the days of the week */
    for (i=0; i<7; i++) {
	if (MondayFirst) j=(i+1)%7;
	else             j=i;
	printf("Border %d SmallWidth mul add Border neg SmallFontSize sub SmallFontSize sub 2 sub moveto (%c) show\n", i, DayName[j][0]);
    }

    /* Now do the days of the month */
    for (i=1; i<=days; i++) {
	printf("Border %d SmallWidth mul add Border neg SmallFontSize sub SmallFontSize 2 add %d mul sub moveto (%d) show\n", first, row, i);
	first++;
	if (first == 7) { first = 0; row++; }
    }

    /* restore graphics state */
    printf("grestore\n");
}

/***************************************************************/
/*                                                             */
/*  DoQueuedPs - do the queued PS and PSFILE reminders.        */
/*                                                             */
/***************************************************************/
int DoQueuedPs(void)
{
    int i;
    int HadPS = 0;
    int wd;
    int begin, end;
    int nread;
    CalEntry *e, *n;
    FILE *fp;
    int fnoff;
    char buffer[512];
    char const *size, *extra;
    char const *s;
    int num, r, g, b, phase, fontsize, moonsize;
    unsigned char c;

    if (!MondayFirst) begin = CurDay - WkDayNum;
    else		     begin = CurDay - (WkDayNum ? WkDayNum-1 : 6);
    wd = 0;
    while (begin < 1) begin++, wd++;
    end = CurDay;
    for (i=begin; i<=end; i++, wd++) {
	e = PsEntries[i];

	if (e) {
	    HadPS = 1;
	    printf("/SAVESTATE save def\n");

	    /* Translate coordinates to bottom of calendar box */
	    printf("%d xincr mul MinX add ytop translate\n", wd);

	    /* Set up convenient variables */
	    printf("/BoxWidth xincr def\n/BoxHeight ylast ytop sub def\n");
	    printf("/InBoxHeight BoxHeight border sub DaySize sub DaySize sub 2 add EntrySize add def \n");
	}

	while (e) {

	    /* Now do the PostScript SPECIAL */
	    fnoff = 0;
	    while (isspace(*(e->entry+fnoff))) fnoff++;
	    switch(e->special) {
	    case SPECIAL_POSTSCRIPT:		/* Send PostScript through */
		printf("%s\n", e->entry+fnoff);
		break;
	    case SPECIAL_PSFILE:		/* PostScript from a file */
		fp = fopen(e->entry+fnoff, "r");
		if (!fp) {
		    fprintf(stderr, "Could not open PostScript file `%s'\n", e->entry+1);
		} else {
		    while(1) {
			nread = fread(buffer, sizeof(char), 512, fp);
			if (!nread) break;
			fwrite(buffer, sizeof(char), nread, stdout);
		    }
		    fclose(fp);
		}
		break;
	    case SPECIAL_SHADE:		/* Shading */
		num = sscanf(e->entry+fnoff, "%d %d %d", &r, &g, &b);
		if (num == 1) {
		    g = r;
		    b = r;
		} else if (num != 3) {
		    fprintf(stderr, "Rem2PS: Malformed SHADE special\n");
		    break;
		}
		if (r < 0 || r > 255 ||
		    g < 0 || g > 255 ||
		    b < 0 || b > 255) {
		    fprintf(stderr, "Rem2PS: Illegal values for SHADE\n");
		    break;
		}
		printf("/_A LineWidth 2 div def _A _A moveto\n");
		printf("BoxWidth _A sub _A lineto BoxWidth _A sub BoxHeight _A sub lineto\n");
		printf("_A BoxHeight _A sub lineto closepath\n");
		printf("%g %g %g setrgbcolor fill 0.0 setgray\n",
		       r/255.0, g/255.0, b/255.0);
		break;

	    case SPECIAL_WEEK:          /* Week number */
		printf("gsave Border Border 2 div moveto /EntryFont findfont EntrySize 1.2 div scalefont setfont (");
		s = e->entry+fnoff;
		while(*s && isspace(*s)) {
		    s++;
		}
		while(*s) {
		    if (*s == '\\' || *s == '(' || *s == ')') {
			PutChar('\\');
		    }
		    PutChar(*s);
		    s++;
		}
		printf(") show grestore\n");
		break;

	    case SPECIAL_MOON:		/* Moon phase */
		num = sscanf(e->entry+fnoff, "%d %d %d", &phase, &moonsize,
			     &fontsize);
		if (num == 1) {
		    moonsize = -1;
		    fontsize = -1;
		} else if (num == 2) {
		    fontsize = -1;
		} else if (num != 3) {
		    fprintf(stderr, "Rem2PS: Badly formed MOON special\n");
		    break;
		}
		if (phase < 0 || phase > 3) {
		    fprintf(stderr, "Rem2PS: Illegal MOON phase %d\n",
			    phase);
		    break;
		}
		if (moonsize < 0) {
		    size = "DaySize 2 div";
		} else {
		    sprintf(buffer, "%d", moonsize);
		    size = buffer;
		}

		printf("gsave 0 setgray newpath Border %s add BoxHeight Border sub %s sub\n", size, size);
		printf(" %s 0 360 arc closepath\n", size);
		switch(phase) {
		case 0:
		    printf("fill\n");
		    break;
		case 2:
		    printf("stroke\n");
		    break;

		case 1:
		    printf("stroke\n");
		    printf("newpath Border %s add BoxHeight Border sub %s sub\n",
			   size, size);
		    printf("%s 90 270 arc closepath fill\n", size);
		    break;
		default:
		    printf("stroke\n");
		    printf("newpath Border %s add BoxHeight Border sub %s sub\n",
		size, size);
		    printf("%s 270 90 arc closepath fill\n", size);
		    break;
		}
		/* See if we have extra stuff */
		extra = e->entry+fnoff;

		/* Skip phase */
		while(*extra && !isspace(*extra)) extra++;
		while(*extra && isspace(*extra)) extra++;

		/* Skip moon size */
		while(*extra && !isspace(*extra)) extra++;
		while(*extra && isspace(*extra)) extra++;

		/* Skip font size */
		while(*extra && !isspace(*extra)) extra++;
		while(*extra && isspace(*extra)) extra++;

		/* Anything left? */
		if (*extra) {
		    printf("Border %s add %s add Border add BoxHeight border sub %s sub %s sub moveto\n", size, size, size, size);
		    if (fontsize < 0) {
			size = "EntrySize";
		    } else {
			sprintf(buffer, "%d", fontsize);
			size = buffer;
		    }
		    printf("/EntryFont findfont %s scalefont setfont (",
			   size);
		    while(*extra) {
			c = (unsigned char) *extra++;
			if (c == '\\' || c == '(' || c == ')') PutChar('\\');
			PutChar(c);
		    }
		    printf(") show\n");

		}
		printf("grestore\n");
		break;
	    }

/* Free the entry */
	    free(e->entry);
	    n = e->next;
	    free(e);
	    e = n;
	}
	if (PsEntries[i]) printf("\n SAVESTATE restore\n");
	PsEntries[i] = NULL;
    }
    return HadPS;
}

/***************************************************************/
/*                                                             */
/* GetSmallLocations                                           */
/*                                                             */
/* Set up the locations for the small calendars.               */
/*                                                             */
/***************************************************************/
void GetSmallLocations(void)
{
    char c;
    char const *s = SmallLocation;
    int colfirst, collast;

/* Figure out the first and last columns */
    colfirst = FirstWkDay;
    collast = (FirstWkDay+MaxDay-1) % 7;
    if (MondayFirst) {
	colfirst = colfirst ? colfirst - 1 : 6;
	collast = collast ? collast - 1 : 6;
    }
    NoSmallCal = 0;

    while((c = *s++) != 0) {
	switch(c) {
	case 'b':
	    /* Adjust Feb. if we want it on the bottom */
	    if (MaxDay == 28 && colfirst == 0) {
		printf("/ysmallbot ymin def /ymin ysmallbot MinBoxSize sub def\n");
		printf("MinX ymin MaxX ymin L\n");
		printf("/ysmall1 ysmallbot def /ysmall2 ysmallbot def\n");
		SmallCol1 = 5;
		SmallCol2 = 6;
		return;
	    }
	    if (collast <= 4) {
		printf("/ysmall1 ysmallbot def /ysmall2 ysmallbot def\n");
		SmallCol1 = 5;
		SmallCol2 = 6;
		return;
	    }
	    break;

	case 't':
	    if (colfirst >= 2) {
		printf("/ysmall1 ysmalltop def /ysmall2 ysmalltop def\n");
		SmallCol1 = 0;
		SmallCol2 = 1;
		return;
	    }
	    break;

	case 's':
	    if (colfirst >= 1 && collast<=5) {
		printf("/ysmall1 ysmalltop def /ysmall2 ysmallbot def\n");
		SmallCol1 = 0;
		SmallCol2 = 6;
		return;
	    }
	    break;
	}
    }
    NoSmallCal = 1;
    return;
}

/***************************************************************/
/*                                                             */
/* EatToken                                                    */
/*                                                             */
/* Read a space-delimited token into an output buffer.         */
/*                                                             */
/***************************************************************/
char const *EatToken(char const *in, char *out, int maxlen)
{
    int i = 0;

    /* Skip space before token */
    while(*in && isspace(*in)) in++;

    /* Eat the token */
    while(*in && !isspace(*in)) {
	if (i < maxlen) {
	    if (out) *out++ = *in;
	    i++;
	}
	in++;
    }
    if (out) *out = 0;
    return in;
}
