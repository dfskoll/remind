/***************************************************************/
/*                                                             */
/*  CALENDAR.C                                                 */
/*                                                             */
/*  The code for generating a calendar.                        */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by Dianne Skoll                    */
/*  Copyright (C) 1999-2000 by Roaring Penguin Software Inc.   */
/*                                                             */
/***************************************************************/

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <stdlib.h>

#ifdef REM_USE_WCHAR
#include <wctype.h>
#endif

#include "types.h"
#include "protos.h"
#include "expr.h"
#include "globals.h"
#include "err.h"
#include "md5.h"

/* Data structures used by the calendar */
typedef struct cal_entry {
    struct cal_entry *next;
    char const *text;
    char const *pos;
#ifdef REM_USE_WCHAR
    wchar_t const *wc_text;
    wchar_t const *wc_pos;
#endif
    int is_color;
    int r, g, b;
    int time;
    int priority;
    DynamicBuffer tags;
    char passthru[PASSTHRU_LEN+1];
    int duration;
    char const *filename;
    int lineno;
} CalEntry;

/* Line-drawing sequences */
struct line_drawing {
  char const *graphics_on;
  char const *graphics_off;
  char *tlr, *bl, *tbl, *blr, *tblr, *tr, *tb, *br, *tbr, *tl, *lr;
};

static struct line_drawing NormalDrawing = {
  "", "", "+", "+", "+", "+", "+", "+", "|", "+", "+", "+", "-"
};

static struct line_drawing VT100Drawing = {
  "\x1B(0", "\x1B(B",
  "\x76", "\x6b", "\x75", "\x77", "\x6e", "\x6d", "\x78",
  "\x6c", "\x74", "\x6a", "\x71"
};

static struct line_drawing UTF8Drawing = {
    "", "",
    "\xe2\x94\xb4",
    "\xe2\x94\x90",
    "\xe2\x94\xa4",
    "\xe2\x94\xac",
    "\xe2\x94\xbc",
    "\xe2\x94\x94",
    "\xe2\x94\x82",
    "\xe2\x94\x8c",
    "\xe2\x94\x9c",
    "\xe2\x94\x98",
    "\xe2\x94\x80"
};

static char *VT100Colors[2][2][2][2] /* [Br][R][G][B] */ = {
  {
  /*** DIM COLORS ***/
    {
      {
	/* 0, 0, 0 = Black   */ "\x1B[0;30m",
	/* 0, 0, 1 = Blue    */ "\x1B[0;34m"
      },
      {
	/* 0, 1, 0 = Green   */ "\x1B[0;32m",
	/* 0, 1, 1 = Cyan    */ "\x1B[0;36m"
      }
    },
    {
      {
	/* 1, 0, 0 = Red     */ "\x1B[0;31m",
	/* 1, 0, 1 = Magenta */ "\x1B[0;35m"
      },
      {
	/* 1, 1, 0 = Yellow  */ "\x1B[0;33m",
	/* 1, 1, 1 = White   */ "\x1B[0;37m"
      }
    }
  },
  {
  /*** BRIGHT COLORS ***/
    {
      {
	/* 0, 0, 0 = Grey    */ "\x1B[30;1m",
	/* 0, 0, 1 = Blue    */ "\x1B[34;1m"
      },
      {
	/* 0, 1, 0 = Green   */ "\x1B[32;1m",
	/* 0, 1, 1 = Cyan    */ "\x1B[36;1m"
      }
    },
    {
      {
	/* 1, 0, 0 = Red     */ "\x1B[31;1m",
	/* 1, 0, 1 = Magenta */ "\x1B[35;1m"
      },
      {
	/* 1, 1, 0 = Yellow  */ "\x1B[33;1m",
	/* 1, 1, 1 = White   */ "\x1B[37;1m"
      }
    }
  }
};

static struct line_drawing *linestruct;
#define DRAW(x) fputs(linestruct->x, stdout)

/* Global variables */
static CalEntry *CalColumn[7];

static int ColSpaces;

static void Colorize(CalEntry const *e);
static void Decolorize(void);
static void SortCol (CalEntry **col);
static void DoCalendarOneWeek (int nleft);
static void DoCalendarOneMonth (void);
static int WriteCalendarRow (void);
static void WriteWeekHeaderLine (void);
static void WritePostHeaderLine (void);
static void PrintLeft (char const *s, int width, char pad);
static void PrintCentered (char const *s, int width, char *pad);
static int WriteOneCalLine (void);
static int WriteOneColLine (int col);
static void GenerateCalEntries (int col);
static void WriteCalHeader (void);
static void WriteCalTrailer (void);
static int DoCalRem (ParsePtr p, int col);
static void WriteSimpleEntries (int col, int jul);
static void WriteTopCalLine (void);
static void WriteBottomCalLine (void);
static void WriteIntermediateCalLine (void);
static void WriteCalDays (void);

#ifdef REM_USE_WCHAR
static void PutWideChar(wchar_t const wc)
{
    char buf[MB_CUR_MAX+1];
    int len;

    len = wctomb(buf, wc);
    if (len > 0) {
	buf[len] = 0;
	fputs(buf, stdout);
    }
}
#endif

static int make_wchar_versions(CalEntry *e)
{
#ifdef REM_USE_WCHAR
    size_t len;
    wchar_t *buf;
    len = mbstowcs(NULL, e->text, 0);
    if (len == (size_t) -1) return 0;

    buf = calloc(len+1, sizeof(wchar_t));
    if (!buf) return 0;

    (void) mbstowcs(buf, e->text, len+1);
    e->wc_text = buf;
    e->wc_pos = buf;
    return 1;
#else
    return 1;
#endif
}

static void gon(void)
{
  printf("%s", linestruct->graphics_on);
}
static void goff(void)
{
  printf("%s", linestruct->graphics_off);
}

static void Decolorize(void)
{
  printf("%s", "\x1B[0m");
}

static void Colorize(CalEntry const *e)
{
  int bright = 0;
  int r, g, b;
  if (e->r > 128 || e->g > 128 || e->b > 128) {
    bright = 1;
  }
  if (e->r > 64) r = 1;
  else r = 0;
  if (e->g > 64) g = 1;
  else g = 0;
  if (e->b > 64) b = 1;
  else b = 0;

  printf("%s", VT100Colors[bright][r][g][b]);
}

/***************************************************************/
/*                                                             */
/*  ProduceCalendar                                            */
/*                                                             */
/*  Main loop for generating a calendar.                       */
/*                                                             */
/***************************************************************/
void ProduceCalendar(void)
{
    int y, m, d;

    if (UseUTF8Chars) {
	linestruct = &UTF8Drawing;
    } else if (UseVTChars) {
        linestruct = &VT100Drawing;
    } else {
        linestruct = &NormalDrawing;
    }
    ShouldCache = 1;

    ColSpaces = (CalWidth - 9) / 7;
    CalWidth = 7*ColSpaces + 8;

    if (CalMonths) {
	FromJulian(JulianToday, &y, &m, &d);
	JulianToday = Julian(y, m, 1);
	while (CalMonths--)
	    DoCalendarOneMonth();
	return;
    } else {
	if (MondayFirst) JulianToday -= (JulianToday%7);
	else             JulianToday -= ((JulianToday+1)%7);

	if (!DoSimpleCalendar) {
  	    WriteWeekHeaderLine();
	    WriteCalDays();
	    WriteIntermediateCalLine();
	}

	while (CalWeeks--)
	    DoCalendarOneWeek(CalWeeks);
	return;
    }
}

/***************************************************************/
/*                                                             */
/*  DoCalendarOneWeek                                          */
/*                                                             */
/*  Write a calendar for a single week                         */
/*                                                             */
/***************************************************************/
static void DoCalendarOneWeek(nleft)
{
    int y, m, d, done, i, l, wd;
    char buf[81];
    int LinesWritten = 0;
    int OrigJul = JulianToday;

/* Fill in the column entries */
    for (i=0; i<7; i++) {
	GenerateCalEntries(i);
	JulianToday++;
    }

/* Output the entries */

/* If it's "Simple Calendar" format, do it simply... */
    if (DoSimpleCalendar) {
	if (MondayFirst) wd = JulianToday % 7;
	else             wd = (JulianToday + 1) % 7;
	for (i=0; i<7; i++) {
	    WriteSimpleEntries(i, OrigJul+i-wd);
	}
	return;
    }

/* Here come the first few lines... */
    gon();
    DRAW(tb);
    goff();
    for (i=0; i<7; i++) {
	FromJulian(OrigJul+i, &y, &m, &d);
	sprintf(buf, "%d %c%c%c ", d, MonthName[m][0], MonthName[m][1],
		MonthName[m][2]);
	if (OrigJul+i == RealToday)
	    PrintLeft(buf, ColSpaces, '*');
	else
	    PrintLeft(buf, ColSpaces, ' ');
	gon();
	DRAW(tb);
	goff();
    }
    PutChar('\n');
    for (l=0; l<CalPad; l++) {
	gon();
	DRAW(tb);
	goff();
	for (i=0; i<7; i++) {
	    PrintLeft("", ColSpaces, ' ');
	    gon();
	    DRAW(tb);
	    goff();
	}
	PutChar('\n');
    }

/* Write the body lines */
    done = 0;
    while (!done) {
	done = WriteOneCalLine();
	LinesWritten++;
    }

/* Write any blank lines required */
    while (LinesWritten++ < CalLines) {
        gon();
        DRAW(tb);
        goff();
	for (i=0; i<7; i++) {
	    PrintLeft("", ColSpaces, ' ');
	    gon();
	    DRAW(tb);
	    goff();
	}
	PutChar('\n');
    }

/* Write the final line */
    if (nleft) {
        WriteIntermediateCalLine();
    } else {
        WriteBottomCalLine();
    }
}

/***************************************************************/
/*                                                             */
/*  DoCalendarOneMonth                                         */
/*                                                             */
/*  Produce a calendar for the current month.                  */
/*                                                             */
/***************************************************************/
static void DoCalendarOneMonth(void)
{
    int y, m, d, mm, yy;

    if (!DoSimpleCalendar) WriteCalHeader();

    if (PsCal) {
	FromJulian(JulianToday, &y, &m, &d);
	printf("%s\n", PSBEGIN);
	printf("%s %d %d %d %d\n",
	       MonthName[m], y, DaysInMonth(m, y), (JulianToday+1) % 7,
	       MondayFirst);
	printf("%s %s %s %s %s %s %s\n",
	       DayName[6], DayName[0], DayName[1], DayName[2],
	       DayName[3], DayName[4], DayName[5]);
	mm = m-1;
	if (mm<0) {
	    mm = 11; yy = y-1;
	} else yy=y;

	printf("%s %d\n", MonthName[mm], DaysInMonth(mm,yy));
	mm = m+1;
	if (mm>11) {
	    mm = 0; yy = y+1;
	} else yy=y;
	printf("%s %d\n", MonthName[mm], DaysInMonth(mm,yy));
    }
    while (WriteCalendarRow()) continue;

    if (PsCal) printf("%s\n", PSEND);
    if (!DoSimpleCalendar) WriteCalTrailer();
}

/***************************************************************/
/*                                                             */
/*  WriteCalendarRow                                           */
/*                                                             */
/*  Write one row of the calendar                              */
/*                                                             */
/***************************************************************/
static int WriteCalendarRow(void)
{
    int y, m, d, wd, i, l;
    int done;
    char buf[81];
    int OrigJul = JulianToday;
    int LinesWritten = 0;
    int moreleft;

/* Get the date of the first day */
    FromJulian(JulianToday, &y, &m, &d);
    if (!MondayFirst) wd = (JulianToday + 1) % 7;
    else		     wd = JulianToday % 7;

/* Fill in the column entries */
    for (i=wd; i<7; i++) {
	if (d+i-wd > DaysInMonth(m, y)) break;
	GenerateCalEntries(i);
	JulianToday++;
    }

/* Output the entries */

/* If it's "Simple Calendar" format, do it simply... */
    if (DoSimpleCalendar) {
	for (i=wd; i<7 && d+i-wd<=DaysInMonth(m, y); i++) {
	    WriteSimpleEntries(i, OrigJul+i-wd);
	}
	return (d+7-wd <= DaysInMonth(m, y));
    }


/* Here come the first few lines... */
    gon();
    DRAW(tb);
    goff();
    for (i=0; i<7; i++) {
	if (i < wd || d+i-wd>DaysInMonth(m, y))
	    PrintLeft("", ColSpaces, ' ');
	else {
	    sprintf(buf, "%d", d+i-wd);
	    PrintLeft(buf, ColSpaces, ' ');
	}
	gon();
	DRAW(tb);
	goff();
    }
    PutChar('\n');
    for (l=0; l<CalPad; l++) {
        gon();
	DRAW(tb);
	goff();
	for (i=0; i<7; i++) {
	    PrintLeft("", ColSpaces, ' ');
	    gon();
	    DRAW(tb);
	    goff();
	}
	PutChar('\n');
    }

/* Write the body lines */
    done = 0;
    while (!done) {
	done = WriteOneCalLine();
	LinesWritten++;
    }

/* Write any blank lines required */
    while (LinesWritten++ < CalLines) {
        gon();
	DRAW(tb);
	goff();
	for (i=0; i<7; i++) {
	    PrintLeft("", ColSpaces, ' ');
	    gon();
	    DRAW(tb);
	    goff();
	}
	PutChar('\n');
    }

    moreleft = (d+7-wd <= DaysInMonth(m, y));
    if (moreleft) {
        WriteIntermediateCalLine();
    } else {
        WriteBottomCalLine();
    }

/* Return non-zero if we have not yet finished */
    return moreleft;
}

/***************************************************************/
/*                                                             */
/*  PrintLeft                                                  */
/*                                                             */
/*  Left-justify a piece of text.                              */
/*                                                             */
/***************************************************************/
static void PrintLeft(char const *s, int width, char pad)
{
    int len = strlen(s);
    printf("%s", s);
    while (len++ < width) PutChar(pad);
}

/***************************************************************/
/*                                                             */
/*  PrintCentered                                              */
/*                                                             */
/*  Center a piece of text                                      */
/*                                                             */
/***************************************************************/
static void PrintCentered(char const *s, int width, char *pad)
{
#ifndef REM_USE_WCHAR
    int len = strlen(s);
    int d = (width - len) / 2;
    int i;

    for (i=0; i<d; i++) fputs(pad, stdout);
    for (i=0; i<width; i++) {
	if (*s) PutChar(*s++); else break;
    }
    for (i=d+len; i<width; i++) fputs(pad, stdout);
#else
    size_t len = mbstowcs(NULL, s, 0);
    int i;
    wchar_t static_buf[128];
    wchar_t *buf;
    wchar_t *ws;
    int d;

    if (!len) {
	for (i=0; i<width; i++) {
	    fputs(pad, stdout);
	}
	return;
    }
    if (len + 1 <= 128) {
	buf = static_buf;
    } else {
	buf = calloc(len+1, sizeof(wchar_t));
	if (!buf) {
	    /* Uh-oh... cannot recover */
	    fprintf(stderr, "%s\n", ErrMsg[E_NO_MEM]);
	    exit(1);
	}
    }
    (void) mbstowcs(buf, s, len+1);
    d = (width - len) / 2;
    ws = buf;
    for (i=0; i<d; i++) fputs(pad, stdout);
    for (i=0; i<width; i++) {
	if (*ws) PutWideChar(*ws++); else break;
    }
    for (i=d+len; i<width; i++) fputs(pad, stdout);
    if (buf != static_buf) free(buf);
#endif
}

/***************************************************************/
/*                                                             */
/*  WriteOneCalLine                                            */
/*                                                             */
/*  Write a single line.                                       */
/*                                                             */
/***************************************************************/
static int WriteOneCalLine(void)
{
    int done = 1, i;

    gon();
    DRAW(tb);
    goff();
    for (i=0; i<7; i++) {
	if (CalColumn[i]) {
	    if (WriteOneColLine(i)) done = 0;
	} else {
	    PrintCentered("", ColSpaces, " ");
	}
	gon();
	DRAW(tb);
	goff();
    }
    PutChar('\n');

    return done;
}


/***************************************************************/
/*                                                             */
/*  WriteOneColLine                                            */
/*                                                             */
/*  Write a single line for a specified column.  Return 1 if   */
/*  the column still has entries; 0 otherwise.                 */
/*                                                             */
/***************************************************************/
static int WriteOneColLine(int col)
{
    CalEntry *e = CalColumn[col];
    char const *s;
    char const *space;

#ifdef REM_USE_WCHAR
    wchar_t const *ws;
    wchar_t const *wspace;
#endif

    int numwritten = 0;

    /* Print as many characters as possible within the column */
#ifdef REM_USE_WCHAR
    if (e->wc_text) {
	wspace = NULL;
	ws = e->wc_pos;

	/* If we're at the end, and there's another entry, do a blank
	   line and move to next entry. */
	if (!*ws && e->next) {
	    PrintLeft("", ColSpaces, ' ');
	    CalColumn[col] = e->next;
	    free((void *)e->text);
	    free((void *)e->filename);
	    if (e->wc_text) free((void *)e->wc_text);
	    free(e);
	    return 1;
	}

	/* Find the last space char within the column. */
	while (ws - e->wc_pos <= ColSpaces) {
	    if (!*ws) {wspace = ws; break;}
	    if (iswspace(*ws)) wspace = ws;
	    ws++;
	}

	/* Colorize reminder if necessary */
	if (UseVTColors && e->is_color) {
	    Colorize(e);
	}

	/* If we couldn't find a space char, print what we have. */
	if (!wspace) {
	    for (ws = e->wc_pos; ws - e->wc_pos < ColSpaces; ws++) {
		if (!*ws) break;
		numwritten++;
		PutWideChar(*ws);
	    }
	    e->wc_pos = ws;
	} else {
	    /* We found a space - print everything before it. */
	    for (ws = e->wc_pos; ws<wspace; ws++) {
		if (!*ws) break;
		numwritten++;
		PutWideChar(*ws);
	    }
	}

	/* Decolorize reminder if necessary */
	if (UseVTColors && e->is_color) {
	    Decolorize();
	}

	/* Flesh out the rest of the column */
	while(numwritten++ < ColSpaces) PutChar(' ');

	/* Skip any spaces before next word */
	while (iswspace(*ws)) ws++;

	/* If done, free memory if no next entry. */
	if (!*ws && !e->next) {
	    CalColumn[col] = e->next;
	    free((void *)e->text);
	    free((void *)e->filename);
	    if (e->wc_text) free((void *)e->wc_text);
	    free(e);
	} else {
	    e->wc_pos = ws;
	}
	if (CalColumn[col]) return 1; else return 0;
    } else {
#endif
	space = NULL;
	s = e->pos;

	/* If we're at the end, and there's another entry, do a blank
	   line and move to next entry. */
	if (!*s && e->next) {
	    PrintLeft("", ColSpaces, ' ');
	    CalColumn[col] = e->next;
	    free((void *)e->text);
	    free((void *)e->filename);
#ifdef REM_USE_WCHAR
	    if (e->wc_text) free((void *)e->wc_text);
#endif
	    free(e);
	    return 1;
	}

	/* Find the last space char within the column. */
	while (s - e->pos <= ColSpaces) {
	    if (!*s) {space = s; break;}
	    if (*s == ' ') space = s;
	    s++;
	}

	/* Colorize reminder if necessary */
	if (UseVTColors && e->is_color) {
	    Colorize(e);
	}

	/* If we couldn't find a space char, print what we have. */
	if (!space) {
	    for (s = e->pos; s - e->pos < ColSpaces; s++) {
		if (!*s) break;
		numwritten++;
		PutChar(*s);
	    }
	    e->pos = s;
	} else {
	    /* We found a space - print everything before it. */
	    for (s = e->pos; s<space; s++) {
		if (!*s) break;
		numwritten++;
		PutChar(*s);
	    }
	}

	/* Decolorize reminder if necessary */
	if (UseVTColors && e->is_color) {
	    Decolorize();
	}

	/* Flesh out the rest of the column */
	while(numwritten++ < ColSpaces) PutChar(' ');

	/* Skip any spaces before next word */
	while (*s == ' ') s++;

	/* If done, free memory if no next entry. */
	if (!*s && !e->next) {
	    CalColumn[col] = e->next;
	    free((void *)e->text);
	    free((void *)e->filename);
#ifdef REM_USE_WCHAR
	    if (e->wc_text) free((void *)e->wc_text);
#endif
	    free(e);
	} else {
	    e->pos = s;
	}
	if (CalColumn[col]) return 1; else return 0;
#ifdef REM_USE_WCHAR
    }
#endif
}

/***************************************************************/
/*                                                             */
/*  GenerateCalEntries                                         */
/*                                                             */
/*  Generate the calendar entries for the ith column           */
/*                                                             */
/***************************************************************/
static void GenerateCalEntries(int col)
{
    int r;
    Token tok;
    char const *s;
    Parser p;

/* Do some initialization first... */
    ClearGlobalOmits();
    DestroyOmitContexts();
    DestroyVars(0);
    NumTriggered = 0;

    r=IncludeFile(InitialFile);
    if (r) {
	fprintf(ErrFp, "%s %s: %s\n", ErrMsg[E_ERR_READING], InitialFile, ErrMsg[r]);
	exit(1);
    }

    while(1) {
	r = ReadLine();
	if (r == E_EOF) return;
	if (r) {
	    Eprint("%s: %s", ErrMsg[E_ERR_READING], ErrMsg[r]);
	    exit(1);
	}
	s = FindInitialToken(&tok, CurLine);

	/* Should we ignore it? */
	if (NumIfs &&
	    tok.type != T_If &&
	    tok.type != T_Else &&
	    tok.type != T_EndIf &&
	    tok.type != T_IfTrig &&
	    ShouldIgnoreLine())
	{
	    /* DO NOTHING */
	}
	else {
	    /* Create a parser to parse the line */
	    CreateParser(s, &p);

	    switch(tok.type) {

	    case T_Empty:
	    case T_Comment:
		break;

	    case T_ErrMsg:  r=DoErrMsg(&p);  break;
	    case T_Rem:     r=DoCalRem(&p, col); break;
	    case T_If:      r=DoIf(&p);      break;
	    case T_IfTrig:  r=DoIfTrig(&p);  break;
	    case T_Else:    r=DoElse(&p);    break;
	    case T_EndIf:   r=DoEndif(&p);   break;
	    case T_Include: r=DoInclude(&p); break;
	    case T_Exit:    DoExit(&p);	     break;
	    case T_Set:     r=DoSet(&p);     break;
	    case T_Fset:    r=DoFset(&p);    break;
	    case T_UnSet:   r=DoUnset(&p);   break;
	    case T_Clr:     r=DoClear(&p);   break;
	    case T_Flush:   r=DoFlush(&p);   break;
	    case T_Debug:   break;  /* IGNORE DEBUG CMD */
	    case T_Dumpvars: break; /* IGNORE DUMPVARS CMD */
	    case T_Banner:  break;  /* IGNORE BANNER CMD */
	    case T_Omit:    r=DoOmit(&p);
		if (r == E_PARSE_AS_REM) {
		    DestroyParser(&p);
		    CreateParser(s, &p);
		    r=DoCalRem(&p, col);
		}
		break;
	    case T_Pop:     r=PopOmitContext(&p);     break;
	    case T_Push:    r=PushOmitContext(&p);    break;
	    case T_Preserve: r=DoPreserve(&p);        break;
	    case T_RemType: if (tok.val == RUN_TYPE) {
		r=DoRun(&p);
		break;
	    } else {
		CreateParser(CurLine, &p);
		r=DoCalRem(&p, col);
		break;
	    }

	    /* If we don't recognize the command, do a REM by default */
	    /* Note:  Since the parser hasn't been used yet, we don't */
	    /* need to destroy it here. */

	    default:        CreateParser(CurLine, &p);
		r=DoCalRem(&p, col);
		break;
	    }
	    if (r && (!Hush || r != E_RUN_DISABLED)) Eprint("%s", ErrMsg[r]);

	    /* Destroy the parser - free up resources it may be tying up */
	    DestroyParser(&p);
	}
    }
}


/***************************************************************/
/*                                                             */
/*  WriteCalHeader                                             */
/*                                                             */
/***************************************************************/
static void WriteCalHeader(void)
{
    char buf[80];
    int y, m, d;

    FromJulian(JulianToday, &y, &m, &d);
    sprintf(buf, "%s %d", MonthName[m], y);

    WriteTopCalLine();

    gon();
    DRAW(tb);
    goff();
    PrintCentered(buf, CalWidth-2, " ");
    gon();
    DRAW(tb);
    goff();
    PutChar('\n');

    WritePostHeaderLine();
    WriteCalDays();
    WriteIntermediateCalLine();
}

/***************************************************************/
/*                                                             */
/*  WriteCalTrailer                                            */
/*                                                             */
/***************************************************************/
static void WriteCalTrailer(void)
{
    PutChar('\f');
}

/***************************************************************/
/*                                                             */
/*  DoCalRem                                                   */
/*                                                             */
/*  Do the REM command in the context of a calendar.           */
/*                                                             */
/***************************************************************/
static int DoCalRem(ParsePtr p, int col)
{
    int oldLen;
    Trigger trig;
    TimeTrig tim;
    Value v;
    int r, err;
    int jul;
    CalEntry *CurCol = CalColumn[col];
    CalEntry *e;
    char const *s, *s2;
    DynamicBuffer buf, obuf, pre_buf;
    Token tok;

    int is_color, col_r, col_g, col_b;

    is_color = 0;
    DBufInit(&buf);
    DBufInit(&pre_buf);

    /* Parse the trigger date and time */
    if ( (r=ParseRem(p, &trig, &tim, 1)) ) {
	FreeTrig(&trig);
	return r;
    }

/* Don't include timed reminders in calendar if -a option supplied. */
    if (DontIssueAts && tim.ttime != NO_TIME) {
	FreeTrig(&trig);
	return OK;
    }
    if (trig.typ == NO_TYPE) {
	FreeTrig(&trig);
	return E_EOLN;
    }
    if (trig.typ == SAT_TYPE) {
	r=DoSatRemind(&trig, &tim, p);
	if (r) {
	    FreeTrig(&trig);
	    if (r == E_EXPIRED) return OK;
	    return r;
	}
	if (!LastTrigValid) {
	    FreeTrig(&trig);
	    return OK;
	}
	r=ParseToken(p, &buf);
	if (r) {
	    FreeTrig(&trig);
	    return r;
	}
	FindToken(DBufValue(&buf), &tok);
	DBufFree(&buf);
	if (tok.type == T_Empty || tok.type == T_Comment) {
	    FreeTrig(&trig);
	    return OK;
	}
	if (tok.type != T_RemType || tok.val == SAT_TYPE) {
	    FreeTrig(&trig);
	    return E_PARSE_ERR;
	}
	if (tok.val == PASSTHRU_TYPE) {
	    r=ParseToken(p, &buf);
	    if (r) return r;
	    if (!DBufLen(&buf)) {
		DBufFree(&buf);
		FreeTrig(&trig);
		return E_EOLN;
	    }
	    StrnCpy(trig.passthru, DBufValue(&buf), PASSTHRU_LEN);
	    DBufFree(&buf);
	}
	trig.typ = tok.val;
	jul = LastTriggerDate;
	if (!LastTrigValid) {
	    FreeTrig(&trig);
	    return OK;
	}
    } else {
	/* Calculate the trigger date */
	jul = ComputeTrigger(trig.scanfrom, &trig, &r, 1);
	if (r) {
	    FreeTrig(&trig);
	    return r;
	}
    }

    /* Convert PS and PSF to PASSTHRU */
    if (trig.typ == PS_TYPE) {
	strcpy(trig.passthru, "PostScript");
	trig.typ = PASSTHRU_TYPE;
    } else if (trig.typ == PSF_TYPE) {
	strcpy(trig.passthru, "PSFile");
	trig.typ = PASSTHRU_TYPE;
    }
    if (trig.typ == PASSTHRU_TYPE) {
	if (!PsCal && strcmp(trig.passthru, "COLOR") && strcmp(trig.passthru, "COLOUR")) {
	    FreeTrig(&trig);
	    return OK;
	}
	if (!strcmp(trig.passthru, "COLOR") ||
	    !strcmp(trig.passthru, "COLOUR")) {
	    is_color = 1;
	    /* Strip off the three color numbers */
	    DBufFree(&buf);
	    r=ParseToken(p, &buf);
	    DBufPuts(&pre_buf, DBufValue(&buf));
	    DBufPutc(&pre_buf, ' ');
	    DBufFree(&buf);
	    if (r) {
		FreeTrig(&trig);
		return r;
	    }
	    r=ParseToken(p, &buf);
	    DBufPuts(&pre_buf, DBufValue(&buf));
	    DBufPutc(&pre_buf, ' ');
	    DBufFree(&buf);
	    if (r) {
		FreeTrig(&trig);
		return r;
	    }
	    r=ParseToken(p, &buf);
	    DBufPuts(&pre_buf, DBufValue(&buf));
	    DBufPutc(&pre_buf, ' ');
	    DBufFree(&buf);
	    if (r) {
		FreeTrig(&trig);
		return r;
	    }
	    (void) sscanf(DBufValue(&pre_buf), "%d %d %d",
			  &col_r, &col_g, &col_b);
	    if (col_r < 0) col_r = 0;
	    else if (col_r > 255) col_r = 255;
	    if (col_g < 0) col_g = 0;
	    else if (col_g > 255) col_g = 255;
	    if (col_b < 0) col_b = 0;
	    else if (col_b > 255) col_b = 255;
	    if (!PsCal && !DoSimpleCalendar) {
		DBufFree(&pre_buf);
	    }
	}
    }

    /* If trigger date == today, add it to the current entry */
    DBufInit(&obuf);
    if ((jul == JulianToday) ||
	(DoSimpleCalDelta &&
	 ShouldTriggerReminder(&trig, &tim, jul, &err))) {
	NumTriggered++;

	if (DoSimpleCalendar || tim.ttime != NO_TIME) {
	    /* Suppress time if it's not today or if it's a non-COLOR special */
	    if (jul != JulianToday ||
		(trig.typ == PASSTHRU_TYPE &&
		 strcmp(trig.passthru, "COLOUR") &&
		 strcmp(trig.passthru, "COLOR"))) {
		if (DBufPuts(&obuf, SimpleTime(NO_TIME)) != OK) {
		    DBufFree(&obuf);
		    DBufFree(&pre_buf);
		    FreeTrig(&trig);
		    return E_NO_MEM;
		}
	    } else {
		if (DBufPuts(&obuf, CalendarTime(tim.ttime, tim.duration)) != OK) {
		    DBufFree(&obuf);
		    DBufFree(&pre_buf);
		    FreeTrig(&trig);
		    return E_NO_MEM;
		}
	    }
	}
	if (trig.typ != PASSTHRU_TYPE &&
	    UserFuncExists("calprefix")==1) {
	    char evalBuf[64];
	    sprintf(evalBuf, "calprefix(%d)", trig.priority);
	    s2 = evalBuf;
	    r = EvalExpr(&s2, &v, NULL);
	    if (!r) {
		if (!DoCoerce(STR_TYPE, &v)) {
		    if (DBufPuts(&obuf, v.v.str) != OK) {
			DestroyValue(v);
			DBufFree(&obuf);
			DBufFree(&pre_buf);
			FreeTrig(&trig);
			return E_NO_MEM;
		    }
		}
		DestroyValue(v);
	    }
	}
	oldLen = DBufLen(&obuf);

	/* In -sa mode, run in ADVANCE mode if we're triggering
	 * before the actual date */
	if (jul != JulianToday) {
	    r = DoSubst(p, &obuf, &trig, &tim, jul, ADVANCE_MODE);
	} else {
	    r = DoSubst(p, &obuf, &trig, &tim, jul, CAL_MODE);
	}
	if (r) {
	    DBufFree(&pre_buf);
	    DBufFree(&obuf);
	    FreeTrig(&trig);
	    return r;
	}
	if (DBufLen(&obuf) <= oldLen) {
	    DBufFree(&obuf);
	    DBufFree(&pre_buf);
	    FreeTrig(&trig);
	    return OK;
	}
	if (trig.typ != PASSTHRU_TYPE &&
	    UserFuncExists("calsuffix")==1) {
	    char evalBuf[64];
	    sprintf(evalBuf, "calsuffix(%d)", trig.priority);
	    s2 = evalBuf;
	    r = EvalExpr(&s2, &v, NULL);
	    if (!r) {
		if (!DoCoerce(STR_TYPE, &v)) {
		    if (DBufPuts(&obuf, v.v.str) != OK) {
			DestroyValue(v);
			DBufFree(&obuf);
			DBufFree(&pre_buf);
			FreeTrig(&trig);
			return E_NO_MEM;
		    }
		}
		DestroyValue(v);
	    }
	}
	s = DBufValue(&obuf);
	if (!DoSimpleCalendar) while (isempty(*s)) s++;
	DBufPuts(&pre_buf, s);
	s = DBufValue(&pre_buf);
	e = NEW(CalEntry);
	if (!e) {
	    DBufFree(&obuf);
	    DBufFree(&pre_buf);
	    FreeTrig(&trig);
	    return E_NO_MEM;
	}
#ifdef REM_USE_WCHAR
	e->wc_pos = NULL;
	e->wc_text = NULL;
#endif
	e->is_color = is_color;
	e->r = col_r;
	e->g = col_g;
	e->b = col_b;
	e->text = StrDup(s);
	DBufFree(&obuf);
	DBufFree(&pre_buf);
	if (!e->text) {
	    free(e);
	    FreeTrig(&trig);
	    return E_NO_MEM;
	}
	make_wchar_versions(e);
	DBufInit(&(e->tags));
	DBufPuts(&(e->tags), DBufValue(&(trig.tags)));
	if (SynthesizeTags) {
	    AppendTag(&(e->tags), SynthesizeTag());
	}

	/* Don't need tags any more */
	FreeTrig(&trig);
	e->duration = tim.duration;
	e->priority = trig.priority;
	e->filename = StrDup(FileName);
	if(!e->filename) {
	    free(e);
	    return E_NO_MEM;
	}
	e->lineno = LineNo;

	if (trig.typ == PASSTHRU_TYPE) {
	    StrnCpy(e->passthru, trig.passthru, PASSTHRU_LEN);
	} else {
	    e->passthru[0] = 0;
	}
	e->pos = e->text;
	if (jul == JulianToday) {
	    e->time = tim.ttime;
	} else {
	    e->time = NO_TIME;
	}
	e->next = CurCol;
	CalColumn[col] = e;
	SortCol(&CalColumn[col]);
    }
    return OK;
}

/***************************************************************/
/*                                                             */
/*  WriteSimpleEntries                                         */
/*                                                             */
/*  Write entries in 'simple calendar' format.                 */
/*                                                             */
/***************************************************************/
static void WriteSimpleEntries(int col, int jul)
{
    CalEntry *e = CalColumn[col];
    CalEntry *n;
    int y, m, d;

    FromJulian(jul, &y, &m, &d);
    while(e) {
	if (DoPrefixLineNo) printf("# fileinfo %d %s\n", e->lineno, e->filename);
	printf("%04d/%02d/%02d", y, m+1, d);
	if (e->passthru[0]) {
	    printf(" %s", e->passthru);
	} else {
	    printf(" *");
	}
	if (*DBufValue(&(e->tags))) {
	    printf(" %s ", DBufValue(&(e->tags)));
	} else {
	    printf(" * ");
	}
	if (e->duration != NO_TIME) {
	    printf("%d ", e->duration);
	} else {
	    printf("* ");
	}
	if (e->time != NO_TIME) {
	    printf("%d ", e->time);
	} else {
	    printf("* ");
	}
	printf("%s\n", e->text);
	free((void *)e->text);
	free((void *)e->filename);
#ifdef REM_USE_WCHAR
	if (e->wc_text) free((void *)e->wc_text);
#endif
	n = e->next;
	free(e);
	e = n;
    }
    CalColumn[col] = NULL;
}

/***************************************************************/
/*                                                             */
/*  Various functions for writing different types of lines.    */
/*                                                             */
/***************************************************************/
static void WriteTopCalLine(void)
{
    gon();
    DRAW(br);
    PrintCentered("", CalWidth-2, linestruct->lr);
    DRAW(bl);
    goff();
    PutChar('\n');
}

static void WriteBottomCalLine(void)
{
    int i;
    gon();
    DRAW(tr);
    for (i=0; i<7; i++) {
	PrintCentered("", ColSpaces, linestruct->lr);
	if (i != 6) {
	    DRAW(tlr);
	} else {
	    DRAW(tl);
	}
    }
    goff();
    PutChar('\n');
}

static void WritePostHeaderLine(void)
{
    int i;
    gon();
    DRAW(tbr);
    for (i=0; i<7; i++) {
	PrintCentered("", ColSpaces, linestruct->lr);
	if (i != 6) {
	    DRAW(blr);
	} else {
	    DRAW(tbl);
	}
    }
    goff();
    PutChar('\n');
}

static void WriteWeekHeaderLine(void)
{
    int i;
    gon();
    DRAW(br);
    for (i=0; i<7; i++) {
	PrintCentered("", ColSpaces, linestruct->lr);
	if (i != 6) {
	    DRAW(blr);
	} else {
	    DRAW(bl);
	}
    }
    goff();
    PutChar('\n');
}

static void WriteIntermediateCalLine(void)
{
    int i;

    gon();
    DRAW(tbr);
    for (i=0; i<7; i++) {
	PrintCentered("", ColSpaces, linestruct->lr);
	if (i != 6) {
	    DRAW(tblr);
	} else {
	    DRAW(tbl);
	}
    }
    goff();
    PutChar('\n');
}

static void WriteCalDays(void)
{
    int i;
    gon();
    DRAW(tb);
    goff();
    for (i=0; i<7; i++) {
	if (!MondayFirst)
	    PrintCentered(DayName[(i+6)%7], ColSpaces, " ");
	else
	    PrintCentered(DayName[i%7], ColSpaces, " ");
	gon();
	DRAW(tb);
	goff();
    }
    PutChar('\n');
}

/***************************************************************/
/*                                                             */
/*  CalendarTime                                               */
/*                                                             */
/*  Format the time according to simple time format.           */
/*  Answer is returned in a static buffer.                     */
/*  A trailing space is always added.                          */
/*  This takes into account duration                           */
/*                                                             */
/***************************************************************/
char const *
CalendarTime(int tim, int duration)
{
    static char buf[128];
    int h, min, hh;
    int h2, min2, hh2, newtim, days;
    char const *ampm1;
    char const *ampm2;
    char daybuf[64];

    buf[0] = 0;

    if (duration == NO_TIME) {
	/* No duration... just call into SimpleTime */
	return SimpleTime(tim);
    }
    if (tim == NO_TIME) {
	/* No time... nothing to return */
	return buf;
    }
    h = tim/60;
    min = tim % 60;
    if (h == 0)      hh=12;
    else if (h > 12) hh=h-12;
    else             hh = h;

    newtim = tim + duration;

    /* How many days in duration? */
    days = newtim / MINUTES_PER_DAY;
    newtim = newtim % MINUTES_PER_DAY;
    h2 = newtim/60;
    min2 = newtim % 60;
    if (h2 == 0)      hh2=12;
    else if (h2 > 12) hh2=h2-12;
    else              hh2 = h2;

    if (days) {
	sprintf(daybuf, "+%d", days);
    } else {
	daybuf[0] = 0;
    }

    if (h >= 12) {
	ampm1 = L_PM;
    } else {
	ampm1 = L_AM;
    }
    if (h2 >= 12) {
	ampm2 = L_PM;
    } else {
	ampm2 = L_AM;
    }
    if (!days) {
	if (!strcmp(ampm1, ampm2)) {
	    ampm1 = "";
	}
    }

    switch(ScFormat) {
    case SC_AMPM:
	sprintf(buf, "%d%c%02d%s-%d%c%02d%s%s ",
		hh, TimeSep, min, ampm1, hh2, TimeSep, min2, ampm2, daybuf);
	break;

    case SC_MIL:
	sprintf(buf, "%02d%c%02d-%02d%c%02d%s ",
		h, TimeSep, min, h2, TimeSep, min2, daybuf);
	break;
    }
    return buf;
}

/***************************************************************/
/*                                                             */
/*  SimpleTime                                                 */
/*                                                             */
/*  Format the time according to simple time format.           */
/*  Answer is returned in a static buffer.                     */
/*  A trailing space is always added.                          */
/*                                                             */
/***************************************************************/
char const *SimpleTime(int tim)
{
    static char buf[32];
    int h, min, hh;

    buf[0] = 0;

    switch(ScFormat) {

    case SC_AMPM:
	if (tim != NO_TIME) {
	    h = tim / 60;
	    min = tim % 60;
	    if (h == 0) hh=12;
	    else if (h > 12) hh=h-12;
	    else hh=h;
	    sprintf(buf, "%d%c%02d%s ", hh, TimeSep, min, (h>=12) ? L_PM : L_AM);
	}
	break;

    case SC_MIL:
	if (tim != NO_TIME) {
	    h = tim / 60;
	    min = tim % 60;
	    sprintf(buf, "%02d%c%02d ", h, TimeSep, min);
	}
	break;
    }
    return buf;
}

/***************************************************************/
/*                                                             */
/*  SortCol                                                    */
/*                                                             */
/*  Sort the calendar entries in a column by time and priority */
/*                                                             */
/***************************************************************/
static void SortCol(CalEntry **col)
{
    CalEntry *cur, *prev, *next;

    cur = *col;
    prev = NULL;

/* Note that we use <= comparison rather than > comparison to preserve the
   file order of reminders which have the same time and priority */

    while (cur->next &&
	   CompareRems(0, cur->time, cur->priority,
		       0, cur->next->time, cur->next->priority,
		       SortByDate, SortByTime, SortByPrio, UntimedBeforeTimed) <= 0) {
	next = cur->next;
	/* Swap cur and next */
	if (!prev) {
	    *col = next;
	    cur->next = next->next;
	    next->next = cur;
	    prev = next;
	} else {
	    prev->next = next;
	    cur->next = next->next;
	    next->next = cur;
	    prev = next;
	}
    }
}

char const *SynthesizeTag(void)
{
    struct MD5Context ctx;
    unsigned char buf[16];
    static char out[128];
    MD5Init(&ctx);
    MD5Update(&ctx, (unsigned char *) CurLine, strlen(CurLine));
    MD5Final(buf, &ctx);
    sprintf(out, "__syn__%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
	    (unsigned int) buf[0], (unsigned int) buf[1],
	    (unsigned int) buf[2], (unsigned int) buf[3],
	    (unsigned int) buf[4], (unsigned int) buf[5],
	    (unsigned int) buf[6], (unsigned int) buf[7],
	    (unsigned int) buf[8], (unsigned int) buf[9],
	    (unsigned int) buf[10], (unsigned int) buf[11],
	    (unsigned int) buf[12], (unsigned int) buf[13],
	    (unsigned int) buf[14], (unsigned int) buf[15]);
    return out;
}

