/***************************************************************/
/*                                                             */
/*  REM2PS.C                                                   */
/*                                                             */
/*  Print a PostScript calendar.                               */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1997 by David F. Skoll                  */
/*                                                             */
/***************************************************************/

#include "config.h"
#include "dynbuf.h"
static char const RCSID[] = "$Id: rem2ps.c,v 1.4 1998-02-07 05:36:03 dfs Exp $";

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef __TURBOC__
#include <io.h>
#endif

#include "rem2ps.h"
#include "version.h"

#ifdef HAVE_PROTOS
#define ARGS(x) x
#else
#define ARGS(x) ()
#endif
#define NEW(type) ((type *) malloc(sizeof(type)))

typedef struct calentry {
    struct calentry *next;
    char *entry;
} CalEntry;

typedef struct {
    char *name;
    int xsize, ysize;
} PageType;

char DayName[7][33];

char *SmallCalLoc[] = {
    "",
    "bt",
    "tb",
    "sbt",
};

#define NUMSMALL (sizeof(SmallCalLoc)/sizeof(SmallCalLoc[0]))
char *SmallLocation;
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
    {"10x14", 720, 1008}
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

char *HeadFont="Helvetica";
char *TitleFont="Helvetica";
char *DayFont="Helvetica-BoldOblique";
char *EntryFont="Helvetica";
char *SmallFont="Helvetica";
char *LineWidth = "1";

char *HeadSize="14";
char *TitleSize="14";
char *DaySize="14";
char *EntrySize="8";
char *BorderSize = "6";

char *UserProlog = NULL;

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

void Init ARGS ((int argc, char *argv[]));
void Usage ARGS ((char *s));
void DoPsCal ARGS ((void));
int DoQueuedPs ARGS ((void));
void DoSmallCal ARGS((char *m, int days, int first, int col, int which));
void WriteProlog ARGS ((void));
void WriteCalEntry ARGS ((void));
void WriteOneEntry ARGS ((char *s));
void GetSmallLocations ARGS ((void));
char *EatToken(char *in, char *out, int maxlen);

/***************************************************************/
/*                                                             */
/*   MAIN PROGRAM                                              */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int main(int argc, char *argv[])
#else
int main(argc, argv)
int argc;
char argv[];
#endif
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
		    fprintf(stderr, "Rem2PS: Version %s Copyright 1992-1997 by David F. Skoll\n\n", VERSION);
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
#ifdef HAVE_PROTOS
void DoPsCal(void)
#else
void DoPsCal()
#endif
{
    char month[40], year[40];
    char prevm[40], nextm[40];
    int days, wkday, prevdays, nextdays;
    int sfirst;
    int i;
    int is_ps;
    int firstcol;
    char *startOfBody;
    char passthru[PASSTHRU_LEN+1];
    DynamicBuffer buf;
    CalEntry *c, *d;

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
	    !strcmp(passthru, "PSFile")) {
	    is_ps = 1;
	}
	c->entry = malloc(strlen(startOfBody) + 1 + is_ps);
	if (!c->entry) {
	    fprintf(stderr, "malloc failed - aborting.\n");
	    exit(1);
	}
	strcpy(c->entry+is_ps, startOfBody);

	if (is_ps) {
/* Save the 'P' or 'F' flag */
	    if (!strcmp(passthru, "PostScript")) {
		*(c->entry) = 'P';
	    } else {
		*(c->entry) = 'F';
	    }
	    if (!PsEntries[DayNum]) PsEntries[DayNum] = c;
	    else {
		d = PsEntries[DayNum];
		while(d->next) d = d->next;
		d->next = c;
	    }
	} else {
/* Put on linked list */
	    if (!CurEntries) CurEntries = c;
	    else {
		d = CurEntries;
		while(d->next) d = d->next;
		d->next = c;
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
#ifdef HAVE_PROTOS
void WriteProlog(void)
#else
void WriteProlog()
#endif
{
    int i;
    int x = CurPage->xsize;
    int y = CurPage->ysize;
    char *isostuff;
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
#ifdef HAVE_PROTOS
void WriteCalEntry(void)
#else
void WriteCalEntry()
#endif
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
	WriteOneEntry(c->entry);
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
#ifdef HAVE_PROTOS
void WriteOneEntry(char *s)
#else
void WriteOneEntry(s)
char *s;
#endif
{
    int c;
    printf("  [");

/* Chew up leading spaces */
    while(isspace((unsigned char) *s)) s++;

    PutChar('(');
    while(*s) {
	/* Use the "unsigned char" cast to fix problem on Solaris 2.5 */
        /* which treated some latin1 characters as white space.       */
	c = (unsigned char) *s++;
	if (c == '\\' || c == '(' || c == ')') PutChar('\\');
	if (!isspace(c)) PutChar(c);
	else {
	    PutChar(')');
	    while(isspace((unsigned char)*s)) s++;
	    if (!*s) {
		printf("]\n");
		return;
	    }
	    PutChar('(');
	}
    }
    printf(")]\n");
}

/***************************************************************/
/*                                                             */
/*  Init - set up parameters                                   */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC void Init(int argc, char *argv[])
#else
void Init(argc, argv)
int argc;
char *argv[];
#endif
{
    char *s, *t;
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
	    for (j=0; j<NUMPAGES; j++)
		if (!strcmp(t, Pages[j].name)) {
		    CurPage = &Pages[j]; 
		    break;
		}

	    if (!CurPage) {
		fprintf(stderr, "\nUnknown media specified.\n");
		fprintf(stderr, "\nAvailable media types:\n");
		for (j=0; j<NUMPAGES; j++)
		    fprintf(stderr, "   %s\n", Pages[j].name);
		fprintf(stderr, "Default media type is %s\n", DefaultPage[0].name);
		exit(1);
            }
	    break;

	case 'o':
	    if (i == argc) Usage("Offset must be supplied");
	    offset = atoi(argv[i++]);
	    if (offset < 36) offset = 36;
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
#ifdef HAVE_PROTOS
PUBLIC void Usage(char *s)
#else
void Usage(s)
char *s;
#endif
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
#ifdef HAVE_PROTOS
void DoSmallCal(char *m, int days, int first, int col, int which)
#else
void DoSmallCal(m, days, first, col, which)
char *m;
int days, first, col;
#endif
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
#ifdef HAVE_PROTOS
PUBLIC int DoQueuedPs(void)
#else
int DoQueuedPs()
#endif
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

/* Now do the user's PostScript code */
	    fnoff = 1;
	    while (isspace(*(e->entry+fnoff))) fnoff++;
	    if (*(e->entry) == 'P') {
		printf("%s\n", e->entry+fnoff);
	    } else {
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
#ifdef HAVE_PROTOS
PUBLIC void GetSmallLocations(void)
#else
void GetSmallLocations()
#endif
{
    char c;
    char *s = SmallLocation;
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
#ifdef HAVE_PROTOS
PUBLIC char  *EatToken(char *in, char *out, int maxlen)
#else
char *EatToken(in, out, maxlen)
char *in, *out;
int maxlen;
#endif
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
