/***************************************************************/
/*                                                             */
/*  CALENDAR.C                                                 */
/*                                                             */
/*  The code for generating a calendar.                        */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by David F. Skoll                  */
/*  Copyright (C) 1999-2000 by Roaring Penguin Software Inc.   */
/*                                                             */
/***************************************************************/

#include "config.h"
static char const RCSID[] = "$Id: calendar.c,v 1.16 2007-06-29 02:11:02 dfs Exp $";

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <stdlib.h>

#include "types.h"
#include "protos.h"
#include "expr.h"
#include "globals.h"
#include "err.h"

/* Data structures used by the calendar */
typedef struct cal_entry {
    struct cal_entry *next;
    char *text;
    char *pos;
    int time;
    int priority;
    char tag[TAG_LEN+1];
    char passthru[PASSTHRU_LEN+1];
    int duration;
    char *filename;
    int lineno;
} CalEntry;

/* Global variables */
static CalEntry *CalColumn[7];
static CalEntry *CalPs[7];

static int ColSpaces;

static void SortCol (CalEntry **col);
static void DoCalendarOneWeek (void);
static void DoCalendarOneMonth (void);
static int WriteCalendarRow (void);
static void PrintLeft (char *s, int width, char pad);
static void PrintCentered (char *s, int width, char pad);
static int WriteOneCalLine (void);
static int WriteOneColLine (int col);
static void GenerateCalEntries (int col);
static void WriteCalHeader (void);
static void WriteCalTrailer (void);
static int DoCalRem (ParsePtr p, int col);
static void WriteSimpleEntries (int col, int jul);
static void WriteSolidCalLine (void);
static void WriteIntermediateCalLine (void);
static void WriteCalDays (void);

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
	    WriteIntermediateCalLine();
	    WriteCalDays();
	    WriteIntermediateCalLine();
	}

	while (CalWeeks--)
	    DoCalendarOneWeek();
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
static void DoCalendarOneWeek(void)
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
    PutChar('|');
    for (i=0; i<7; i++) {
	FromJulian(OrigJul+i, &y, &m, &d);
	sprintf(buf, "%d %c%c%c ", d, MonthName[m][0], MonthName[m][1],
		MonthName[m][2]);
	if (OrigJul+i == RealToday)
	    PrintLeft(buf, ColSpaces, '*');
	else
	    PrintLeft(buf, ColSpaces, ' ');
	PutChar('|');
    }
    PutChar('\n');
    for (l=0; l<CalPad; l++) {
	PutChar('|');
	for (i=0; i<7; i++) {
	    PrintLeft("", ColSpaces, ' ');
	    PutChar('|');
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
	PutChar('|');
	for (i=0; i<7; i++) {
	    PrintLeft("", ColSpaces, ' ');
	    PutChar('|');
	}
	PutChar('\n');
    }

/* Write the final line */
    WriteIntermediateCalLine();
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
    PutChar('|');
    for (i=0; i<7; i++) {
	if (i < wd || d+i-wd>DaysInMonth(m, y))
	    PrintLeft("", ColSpaces, ' ');
	else {
	    sprintf(buf, "%d", d+i-wd);
	    PrintLeft(buf, ColSpaces, ' ');
	}
	PutChar('|');
    }
    PutChar('\n');
    for (l=0; l<CalPad; l++) {
	PutChar('|');
	for (i=0; i<7; i++) {
	    PrintLeft("", ColSpaces, ' ');
	    PutChar('|');
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
	PutChar('|');
	for (i=0; i<7; i++) {
	    PrintLeft("", ColSpaces, ' ');
	    PutChar('|');
	}
	PutChar('\n');
    }

    WriteIntermediateCalLine();

/* Return non-zero if we have not yet finished */
    return (d+7-wd <= DaysInMonth(m, y));
}

/***************************************************************/
/*                                                             */
/*  PrintLeft                                                  */
/*                                                             */
/*  Left-justify a piece of text.                              */
/*                                                             */
/***************************************************************/
static void PrintLeft(char *s, int width, char pad)
{
    int len = strlen(s);
    printf("%s", s);
    while (len++ < width) PutChar(pad);
}

/***************************************************************/
/*                                                             */
/*  PrintCentered                                              */
/*                                                             */
/*  Center a piec of text                                      */
/*                                                             */
/***************************************************************/
static void PrintCentered(char *s, int width, char pad)
{
    int len = strlen(s);
    int d = (width - len) / 2;
    int i;

    for (i=0; i<d; i++) PutChar(pad);
    for (i=0; i<width; i++) {
	if (*s) PutChar(*s++); else break;
    }
    for (i=d+len; i<width; i++) PutChar(pad);
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

    PutChar('|');
    for (i=0; i<7; i++) {
	if (CalColumn[i]) {
	    if (WriteOneColLine(i)) done = 0;
	} else {
	    PrintCentered("", ColSpaces, ' ');
	}
	PutChar('|');
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
    char *s;
    char *space;
    int numwritten = 0;

/* Print as many characters as possible within the column */
    space = NULL;
    s = e->pos;

/* If we're at the end, and there's another entry, do a blank line and move
   to next entry. */
    if (!*s && e->next) {
	PrintLeft("", ColSpaces, ' ');
	CalColumn[col] = e->next;
	free(e->text);
	free(e->filename);
	free(e);
	return 1;
    }

/* Find the last space char within the column. */
    while (s - e->pos <= ColSpaces) {
	if (!*s) {space = s; break;}
	if (*s == ' ') space = s;
	s++;
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

/* Flesh out the rest of the column */
    while(numwritten++ < ColSpaces) PutChar(' ');

/* Skip any spaces before next word */
    while (*s == ' ') s++;

/* If done, free memory if no next entry. */
    if (!*s && !e->next) {
	CalColumn[col] = e->next;
	free(e->text);
	free(e->filename);
	free(e);
    } else {
	e->pos = s;
    }
    if (CalColumn[col]) return 1; else return 0;
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
    char *s;
    Parser p;

/* Do some initialization first... */
    ClearGlobalOmits();
    DestroyOmitContexts();
    DestroyVars(0);
    NumTriggered = 0;

    r=OpenFile(InitialFile);
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

    WriteSolidCalLine();

    PutChar('|');
    PrintCentered(buf, CalWidth-2, ' ');
    PutChar('|');
    PutChar('\n');

    WriteIntermediateCalLine();
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
    int r;
    int jul;
    CalEntry *CurCol = CalColumn[col];
    CalEntry *CurPs = CalPs[col];
    CalEntry *e;
    char *s, *s2;
    DynamicBuffer buf, obuf;
    Token tok;

    DBufInit(&buf);

    /* Parse the trigger date and time */
    if ( (r=ParseRem(p, &trig, &tim)) ) return r;

/* Don't include timed reminders in calendar if -a option supplied. */
    if (DontIssueAts && tim.ttime != NO_TIME) return OK;
    if (trig.typ == NO_TYPE) return E_EOLN;
    if (trig.typ == SAT_TYPE) {
	r=DoSatRemind(&trig, &tim, p);
	if (r) return r;
	r=ParseToken(p, &buf);
	if (r) return r;
	FindToken(DBufValue(&buf), &tok);
	DBufFree(&buf);
	if (tok.type == T_Empty || tok.type == T_Comment) return OK;
	if (tok.type != T_RemType || tok.val == SAT_TYPE) return E_PARSE_ERR;
	if (tok.val == PASSTHRU_TYPE) {
	    r=ParseToken(p, &buf);
	    if (r) return r;
	    if (!DBufLen(&buf)) {
		DBufFree(&buf);
		return E_EOLN;
	    }
	    StrnCpy(trig.passthru, DBufValue(&buf), PASSTHRU_LEN);
	    DBufFree(&buf);
	}
	trig.typ = tok.val;
	jul = LastTriggerDate;
	if (!LastTrigValid) return OK;
    } else {
	/* Calculate the trigger date */
	jul = ComputeTrigger(trig.scanfrom, &trig, &r);
	if (r) return r;
    }

    /* Convert PS and PSF to PASSTHRU */
    if (trig.typ == PS_TYPE) {
	strcpy(trig.passthru, "PostScript");
	trig.typ = PASSTHRU_TYPE;
    } else if (trig.typ == PSF_TYPE) {
	strcpy(trig.passthru, "PSFile");
	trig.typ = PASSTHRU_TYPE;
    }
    if (!PsCal) {
	if (trig.typ == PASSTHRU_TYPE) {
	    if (strcmp(trig.passthru, "COLOR")) return OK;
	    /* Strip off the three color numbers */
	    DBufFree(&buf);
	    r=ParseToken(p, &buf);
	    DBufFree(&buf);
	    if (r) return r;
	    r=ParseToken(p, &buf);
	    DBufFree(&buf);
	    if (r) return r;
	    r=ParseToken(p, &buf);
	    DBufFree(&buf);
	    if (r) return r;
	}
    }

    /* Remove any "at" times from PS or PSFILE reminders */
    if (trig.typ == PASSTHRU_TYPE && strcmp(trig.passthru, "COLOR")) {
	tim.ttime = NO_TIME;
	tim.duration = NO_TIME;
    }

    /* If trigger date == today, add it to the current entry */
    DBufInit(&obuf);
    if (jul == JulianToday ||
	(DoSimpleCalDelta && trig.delta != NO_DELTA && jul-trig.delta <= JulianToday)) {
	NumTriggered++;
	if (DoSimpleCalendar || tim.ttime != NO_TIME) {
	    if (DBufPuts(&obuf, SimpleTime(tim.ttime)) != OK) {
		DBufFree(&obuf);
		return E_NO_MEM;
	    }
	}
	if (trig.typ != PASSTHRU_TYPE &&
	    UserFuncExists("calprefix")==1) {
	    char evalBuf[64];
	    sprintf(evalBuf, "calprefix(%d)", trig.priority);
	    s2 = evalBuf;
	    r = EvalExpr(&s2, &v);
	    if (!r) {
		if (!DoCoerce(STR_TYPE, &v)) {
		    if (DBufPuts(&obuf, v.v.str) != OK) {
			DestroyValue(v);
			DBufFree(&obuf);
			return E_NO_MEM;
		    }
		}
		DestroyValue(v);
	    }
	}
	oldLen = DBufLen(&obuf);
	if ( (r=DoSubst(p, &obuf, &trig, &tim, jul, CAL_MODE)) ) {
	    DBufFree(&obuf);
	    return r;
	}
	if (DBufLen(&obuf) <= oldLen) {
	    DBufFree(&obuf);
	    return OK;
	}
	if (trig.typ != PASSTHRU_TYPE &&
	    UserFuncExists("calsuffix")==1) {
	    char evalBuf[64];
	    sprintf(evalBuf, "calsuffix(%d)", trig.priority);
	    s2 = evalBuf;
	    r = EvalExpr(&s2, &v);
	    if (!r) {
		if (!DoCoerce(STR_TYPE, &v)) {
		    if (DBufPuts(&obuf, v.v.str) != OK) {
			DestroyValue(v);
			DBufFree(&obuf);
			return E_NO_MEM;
		    }
		}
		DestroyValue(v);
	    }
	}
	s = DBufValue(&obuf);
	if (!DoSimpleCalendar) while (isspace(*s)) s++;
	e = NEW(CalEntry);
	if (!e) {
	    DBufFree(&obuf);
	    return E_NO_MEM;
	}
	e->text = StrDup(s);
	DBufFree(&obuf);
	if (!e->text) {
	    free(e);
	    return E_NO_MEM;
	}
	StrnCpy(e->tag, trig.tag, TAG_LEN);
	if (!e->tag[0]) {
	    strcpy(e->tag, "*");
	}
	e->duration = tim.duration;
	e->priority = trig.priority;
	e->filename = StrDup(FileName);
	if(!e->filename) {
	    free(e);
	    return E_NO_MEM;
	}
	e->lineno = LineNo;

	/* Ugly hack... SPECIAL COLOR and SPECIAL HTML are not treated
	   as "passthru" to preserve ordering and make them behave like
	   a MSG-type reminder */

	if (trig.typ == PASSTHRU_TYPE &&
	    strcmp(trig.passthru, "COLOR") &&
	    strcmp(trig.passthru, "HTML")) {
	    StrnCpy(e->passthru, trig.passthru, PASSTHRU_LEN);
	    e->pos = e->passthru;
	    e->time = NO_TIME;
	    e->next = CurPs;
	    CalPs[col] = e;
	    SortCol(&CalPs[col]);
	} else {
	    if (trig.typ == PASSTHRU_TYPE) {
		StrnCpy(e->passthru, trig.passthru, PASSTHRU_LEN);
	    } else {
		e->passthru[0] = 0;
	    }
	    e->pos = e->text;
	    e->time = tim.ttime;
	    e->next = CurCol;
	    CalColumn[col] = e;
	    SortCol(&CalColumn[col]);
	}
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
    CalEntry *e = CalPs[col];
    CalEntry *n;
    int y, m, d;

/* Do all the PASSTHRU entries first, if any */
    FromJulian(jul, &y, &m, &d);
    while(e) {
	if (DoPrefixLineNo) printf("# fileinfo %d %s\n", e->lineno, e->filename);
	printf("%04d/%02d/%02d ", y, m+1, d);
	printf("%s ", e->passthru);
	printf("%s ", e->tag);
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
	free(e->text);
	free(e->filename);
	n = e->next;
	free(e);
	e = n;
    }
    CalPs[col] = NULL;

    e = CalColumn[col];
    while(e) {
	if (DoPrefixLineNo) printf("# fileinfo %d %s\n", e->lineno, e->filename);
	printf("%04d/%02d/%02d", y, m+1, d);
	if (e->passthru[0]) {
	    printf(" %s", e->passthru);
	} else {
	    printf(" *");
	}
	printf(" %s ", e->tag);
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
	free(e->text);
	free(e->filename);
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
static void WriteSolidCalLine(void)
{
    PutChar('+');
    PrintCentered("", CalWidth-2, '-');
    PutChar('+');
    PutChar('\n');
}

static void WriteIntermediateCalLine(void)
{
    int i;

    PutChar('+');
    for (i=0; i<7; i++) {
	PrintCentered("", ColSpaces, '-');
	PutChar('+');
    }
    PutChar('\n');
}

static void WriteCalDays(void)
{
    int i;
    PutChar('|');
    for (i=0; i<7; i++) {
	if (!MondayFirst)
	    PrintCentered(DayName[(i+6)%7], ColSpaces, ' ');
	else
	    PrintCentered(DayName[i%7], ColSpaces, ' ');
	PutChar('|');
    }
    PutChar('\n');
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
char *SimpleTime(int tim)
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
	    sprintf(buf, "%2d%c%02d%s ", hh, TIMESEP, min, (h>=12) ? L_PM : L_AM);
	}
	break;

    case SC_MIL:
	if (tim != NO_TIME) {
	    h = tim / 60;
	    min = tim % 60;
	    sprintf(buf, "%02d%c%02d ", h, TIMESEP, min);
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
		       SortByDate, SortByTime, SortByPrio) <= 0) {
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
