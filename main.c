/***************************************************************/
/*                                                             */
/*  MAIN.C                                                     */
/*                                                             */
/*  Main program loop, as well as miscellaneous conversion     */
/*  routines, etc.                                             */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1997 by David F. Skoll                  */
/*                                                             */
/***************************************************************/

#include "config.h"
static char const RCSID[] = "$Id: main.c,v 1.5 1997-03-30 19:07:41 dfs Exp $";

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_UNISTD
#include <unistd.h>
#endif
#include <stdio.h>
#include <signal.h>
#include <string.h>
#ifdef HAVE_STDARG
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <ctype.h>
#include <time.h>

#ifdef AMIGA
#include <sys/types.h>
#else
#if defined(__MSDOS__) || defined(__OS2__)
#include <dos.h>
#else
#include <sys/types.h>
#ifndef SYSV
#ifdef QDOS
#include <sys/times.h>
#else
#include <sys/time.h>
#endif /* QDOS */
#endif /* ndef SYSV */
#endif /* if defined(__MSDOS__)... */
#endif /* AMIGA */
#include "types.h"
#include "protos.h"
#include "expr.h"
#include "globals.h"
#include "err.h"

PRIVATE void DoReminders ARGS ((void));

#if defined(NEED_TIMEGM) && !defined(HAVE_MKTIME)
PRIVATE long time_cheat ARGS ((int year, int month));
long timegm ARGS((struct tm *tm));
long timelocal ARGS((struct tm *tm));
#endif

static char TPushBuffer[TOKSIZE+1]; /* Buffer for pushing back a token. */
static char *TokenPushed = NULL;

/* Whooo... the putchar/Putchar/PutChar macros are a mess...
   my apologies... */
#ifdef OS2_POPUP
#define Putchar(c) {if (AsPopUp) PutcPopUp(c); else putchar(c);}
#else
#define Putchar(c) PutChar(c)
#endif

/***************************************************************/
/***************************************************************/
/**                                                           **/
/**  Main Program Loop                                        **/
/**                                                           **/
/***************************************************************/
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int main(int argc, char *argv[])
#else
int main(argc, argv)
int argc;
char *argv[];
#endif
{
#ifdef HAVE_QUEUED
    int pid;
#endif

/* The very first thing to do is to set up ErrFp to be stderr */
    ErrFp = stderr;

/* Set up global vars */
    ArgC = argc;
    ArgV = argv;

    InitRemind(argc, argv);
    if(DoCalendar || DoSimpleCalendar) {
	ProduceCalendar();
	return 0;
    }

    /* Not doing a calendar.  Do the regular remind loop */
    ShouldCache = (Iterations > 1);

    while (Iterations--) {
	DoReminders();

	if (DebugFlag & DB_DUMP_VARS) {
	    DumpVarTable();
	    DumpSysVarByName(NULL);
	}

	if (!Hush) {
	    if (DestroyOmitContexts())
		Eprint("%s", ErrMsg[E_PUSH_NOPOP]);
#ifdef HAVE_QUEUED
	    if (!Daemon && !NextMode && !NumTriggered && !NumQueued) {
		printf("%s\n", ErrMsg[E_NOREMINDERS]);
	    } else if (!Daemon && !NextMode && !NumTriggered) {
		printf(ErrMsg[M_QUEUED], NumQueued);
	    }
#else
	    if (!NextMode && !NumTriggered) {
		printf("%s\n", ErrMsg[E_NOREMINDERS]);
	    }
#endif
	}

	/* If it's MS-DOS, reset the file access date.           */
	/* Note that OS/2 and DOS bound programs have __MSDOS__  */
	/* defined, so this test should probably be modified.    */
#if defined(__MSDOS__)
	if (!UseStdin && (RealToday == JulianToday))
	    SetAccessDate(InitialFile, RealToday);
#endif

	/* If there are sorted reminders, handle them */
	if (SortByDate) IssueSortedReminders();

	/* If there are any background reminders queued up, handle them */
#ifdef HAVE_QUEUED
	if (NumQueued || Daemon) {

	    if (DontFork) {
		HandleQueuedReminders();
		return 0;
	    } else {
		pid = fork();
		if (pid == 0) {
		    HandleQueuedReminders();
		    return 0;
		}
		if (pid == -1) {
		    fprintf(ErrFp, "%s", ErrMsg[E_CANTFORK]);
		    return 1;
		}
	    }
	}
#endif
	if (Iterations) {
	    ClearGlobalOmits();
	    DestroyOmitContexts();
	    DestroyVars(0);
	    NumTriggered = 0;
	    JulianToday++;
	}
    }
    return 0;
}

/***************************************************************/
/*                                                             */
/*  DoReminders                                                */
/*                                                             */
/*  The normal case - we're not doing a calendar.              */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE void DoReminders(void)
#else
static void DoReminders()
#endif
{
    int r;
    Token tok;
    char *s;
    Parser p;

    if (!UseStdin) {
	FileAccessDate = GetAccessDate(InitialFile);
    } else {
	FileAccessDate = JulianToday;
    }

    if (FileAccessDate < 0) {
	fprintf(ErrFp, "%s: `%s'.\n", ErrMsg[E_CANTACCESS], InitialFile);
	exit(1);
    }

    r=OpenFile(InitialFile);
    if (r) {
	fprintf(ErrFp, "%s %s: %s\n", ErrMsg[E_ERR_READING],
		InitialFile, ErrMsg[r]);
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
	    /*** IGNORE THE LINE ***/
	}
	else {
	    /* Create a parser to parse the line */
	    CreateParser(s, &p);
	    switch(tok.type) {

            case T_Empty:
	    case T_Comment:
		break;

	    case T_Rem:     r=DoRem(&p);     break;
	    case T_ErrMsg:  r=DoErrMsg(&p);  break;
	    case T_If:      r=DoIf(&p);      break;
	    case T_IfTrig:  r=DoIfTrig(&p);  break;
	    case T_Else:    r=DoElse(&p);    break;
	    case T_EndIf:   r=DoEndif(&p);   break;
	    case T_Include: r=DoInclude(&p); break;
	    case T_Exit:    DoExit(&p);      break;
	    case T_Flush:   r=DoFlush(&p);   break;
	    case T_Set:     r=DoSet(&p);     break;
	    case T_Fset:    r=DoFset(&p);    break;
	    case T_UnSet:   r=DoUnset(&p);   break;
	    case T_Clr:     r=DoClear(&p);   break;
            case T_Debug:   r=DoDebug(&p);   break;
	    case T_Dumpvars: r=DoDump(&p);   break;
	    case T_Banner:  r=DoBanner(&p);  break;
	    case T_Omit:    r=DoOmit(&p);
		if (r == E_PARSE_AS_REM) {
		    DestroyParser(&p);
		    CreateParser(s, &p);
		    r=DoRem(&p);
		}
		break;
	    case T_Pop:     r=PopOmitContext(&p);     break;
	    case T_Preserve: r=DoPreserve(&p);  break;
	    case T_Push:    r=PushOmitContext(&p);    break;
	    case T_RemType: if (tok.val == RUN_TYPE) {
		r=DoRun(&p);
		break;
	    } else {
		CreateParser(CurLine, &p);
		r=DoRem(&p);
		break;
	    }


	    /* If we don't recognize the command, do a REM by default */
	    /* Note:  Since the parser hasn't been used yet, we don't */
	    /* need to destroy it here. */

	    default: CreateParser(CurLine, &p); r=DoRem(&p); break;

	    }
	    if (r && (!Hush || r != E_RUN_DISABLED)) {
		Eprint("%s", ErrMsg[r]);
	    }

	    /* Destroy the parser - free up resources it may be tying up */
	    DestroyParser(&p);
	}
    }
}

/***************************************************************/
/*                                                             */
/*  Julian                                                     */
/*                                                             */
/*  Given day, month, year, return Julian date in days since   */
/*  1 January 1990.                                            */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int Julian(int year, int month, int day)
#else
int Julian(year, month, day)
int day, month, year;
#endif
{
    int y1 = BASE-1, y2 = year-1;

    int y4 = (y2 / 4) - (y1 / 4);  /* Correct for leap years */
    int y100 = (y2 / 100) - (y1 / 100); /* Don't count multiples of 100... */
    int y400 = (y2 / 400) - (y1 / 400); /* ... but do count multiples of 400 */

    return 365 * (year-BASE) + y4 - y100 + y400 +
	MonthIndex[IsLeapYear(year)][month] + day - 1;
}

/***************************************************************/
/*                                                             */
/*  FromJulian                                                 */
/*                                                             */
/*  Convert a Julian date to year, month, day.                 */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC void FromJulian(int jul, int *y, int *m, int *d)
#else
void FromJulian(jul, y, m, d)
int jul;
int *y, *m, *d;
#endif
{
    int try_yr = (jul / 365) + BASE;
    int try_mon = 0;
    int t;

    /* Inline code for speed... */
    int y1 = BASE-1, y2 = try_yr-1;
    int y4 = (y2 / 4) - (y1 / 4);  /* Correct for leap years */
    int y100 = (y2 / 100) - (y1 / 100); /* Don't count multiples of 100... */
    int y400 = (y2 / 400) - (y1 / 400); /* ... but do count multiples of 400 */

    int try_jul= 365 * (try_yr-BASE) + y4 - y100 + y400;

    while (try_jul > jul) {
	try_yr--;
	try_jul -= DaysInYear(try_yr);
    }
    jul -= try_jul;

    t = DaysInMonth(try_mon, try_yr);
    while (jul >= t) {
	jul -= t;
	try_mon++;
	t = DaysInMonth(try_mon, try_yr);
    }
    *y = try_yr;
    *m = try_mon;
    *d = jul + 1;
    return;
}

/***************************************************************/
/*                                                             */
/*  ParseChar                                                  */
/*                                                             */
/*  Parse a character from a parse pointer.  If peek is non-   */
/*  zero, then just peek ahead; don't advance pointer.         */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int ParseChar(ParsePtr p, int *err, int peek)
#else
int ParseChar(p, err, peek)
ParsePtr p;
int *err;
int peek;
#endif
{
    Value val;
    int r;

    *err = 0;
    if (TokenPushed && *TokenPushed)
	if (peek) return *TokenPushed;
	else      return *TokenPushed++;

    while(1) {
	if (p->isnested) {
	    if (*(p->epos)) {
		if (peek) {
		    return *(p->epos);
		} else {
		    return *(p->epos++);
		}
	    }
	    free(p->etext);  /* End of substituted expression */
	    p->etext = NULL;
	    p->epos = NULL;
	    p->isnested = 0;
	}
	if (!*(p->pos)) {
	    return 0;
	}
	if (*p->pos != BEG_OF_EXPR || !p->allownested) {
	    if (peek) {
		return *(p->pos);
	    } else {
		return *(p->pos++);
	    }
	}
	p->pos++;
	r = EvalExpr(&(p->pos), &val);
	if (r) {
	    *err = r;
	    DestroyParser(p);
	    return 0;
	}
	if (*p->pos != END_OF_EXPR) {
	    *err = E_MISS_END;
	    DestroyParser(p);
	    DestroyValue(val);
	    return 0;
	}
	p->pos++;
	r = DoCoerce(STR_TYPE, &val);
	if (r) { *err = r; return 0; }
	p->etext = val.v.str;
	val.type = ERR_TYPE; /* So it's not accidentally destroyed! */
	p->isnested = 1;
	p->epos = p->etext;
    }
}

/***************************************************************/
/*                                                             */
/*  ParseNonSpaceChar                                          */
/*                                                             */
/*  Parse the next non-space character.                        */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int ParseNonSpaceChar(ParsePtr p, int *err, int peek)
#else
int ParseNonSpaceChar(p, err, peek)
ParsePtr p;
int *err;
int peek;
#endif
{
    int ch;

    ch = ParseChar(p, err, 1);
    if (*err) return 0;

    while (isspace(ch)) {
	ParseChar(p, err, 0);   /* Guaranteed to work */
	ch = ParseChar(p, err, 1);
	if (*err) return 0;
    }
    if (!peek) ch = ParseChar(p, err, 0);  /* Guaranteed to work */
    return ch;
}

/***************************************************************/
/*                                                             */
/*  ParseToken                                                 */
/*                                                             */
/*  Parse a token delimited by whitespace.                     */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int ParseToken(ParsePtr p, char *out)
#else
int ParseToken(p, out)
ParsePtr p;
char *out;
#endif
{
    int c, err;
    int len = 0;

    *out = 0;

    c = ParseChar(p, &err, 0);
    if (err) return err;
    while (c && isspace(c)) {
	c = ParseChar(p, &err, 0);
	if (err) return err;
    }
    if (!c) return OK;
    *out++ = c;
    len++;

    while (c && !isspace(c)) {
	c = ParseChar(p, &err, 0);
	if (err) return err;
	if (len < TOKSIZE && c && !isspace(c)) {
	    *out++ = c;
	    len++;
	}
    }
    *out = 0;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  ParseIdentifier                                            */
/*                                                             */
/*  Parse a valid identifier - ie, alpha or underscore         */
/*  followed by alphanum.  Return E_BAD_ID if identifier is    */
/*  invalid.                                                   */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int ParseIdentifier(ParsePtr p, char *out)
#else
int ParseIdentifier(p, out)
ParsePtr p;
char *out;
#endif
{
    int c, err;
    int len = 0;

    *out = 0;

    c = ParseChar(p, &err, 0);
    if (err) return err;
    while (c && isspace(c)) {
	c = ParseChar(p, &err, 0);
	if (err) return err;
    }
    if (!c) return E_EOLN;
    if (c != '$' && c != '_' && !isalpha(c)) return E_BAD_ID;
    *out++ = c;
    *out = 0;
    len++;

    while (1) {
	c = ParseChar(p, &err, 1);
	if (err) return err;
	if (c != '_' && !isalnum(c)) return OK;

	if (len < TOKSIZE) {
	    c = ParseChar(p, &err, 0);  /* Guaranteed to work */
	    *out++ = c;
	    *out = 0;
	    len++;
	}
    }
}
/***************************************************************/
/*                                                             */
/* EvaluateExpr                                                */
/*                                                             */
/* We are expecting an expression here.  Evaluate it and       */
/* return the value.                                           */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int EvaluateExpr(ParsePtr p, Value *v)
#else
int EvaluateExpr(p, v)
ParsePtr p;
Value *v;
#endif
{

    int bracketed = 0;
    int r;

    if (p->isnested) return E_PARSE_ERR;  /* Can't nest expressions */
    while (isspace(*p->pos)) (p->pos)++;
    if (!p->pos) return E_PARSE_ERR;      /* Missing expression */
    if (*p->pos == BEG_OF_EXPR) {
	(p->pos)++;
	bracketed = 1;
    }
    r = EvalExpr(&(p->pos), v);
    if (r) return r;
    if (bracketed) {
	if (*p->pos != END_OF_EXPR) return E_MISS_END;
	(p->pos)++;
    }
    return OK;
}

/***************************************************************/
/*                                                             */
/*  Eprint - print an error message.                           */
/*                                                             */
/***************************************************************/
#ifdef HAVE_STDARG
#ifdef HAVE_PROTOS
PUBLIC void Eprint(const char *fmt, ...)
#else
void Eprint(fmt)
char *fmt;
#endif
#else
/*VARARGS0*/
void Eprint(va_alist)
va_dcl
#endif
{
    va_list argptr;
#ifndef HAVE_STDARG
    char *fmt;
#endif

    /* Check if more than one error msg. from this line */
    if (!FreshLine && !ShowAllErrors) return;

    if (FreshLine) {
	FreshLine = 0;
	if (strcmp(FileName, "-"))
	    (void) fprintf(ErrFp, "%s(%d): ", FileName, LineNo);
	else
	    (void) fprintf(ErrFp, "-stdin-(%d): ", LineNo);
	if (DebugFlag & DB_PRTLINE) OutputLine(ErrFp);
    } else fprintf(ErrFp, "       ");

#ifdef HAVE_STDARG
    va_start(argptr, fmt);
#else
    va_start(argptr);
    fmt = va_arg(argptr, char *);
#endif
    (void) vfprintf(ErrFp, fmt, argptr);
    (void) fputc('\n', ErrFp);
#ifndef HAVE_STDARG
    va_end(argptr);
#endif
    return;
}

/***************************************************************/
/*                                                             */
/*  OutputLine                                                 */
/*                                                             */
/*  Output a line from memory buffer to a file pointer.  This  */
/*  simply involves escaping newlines.                         */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC void OutputLine(FILE *fp)
#else
void OutputLine(fp)
FILE *fp;
#endif
{
    register char *s = CurLine;
    register char c = 0;

    while (*s) {
	if (*s == '\n') Putc('\\', fp);
	Putc(*s, fp);
	c = *s++;
    }
    if (c != '\n') Putc('\n', fp);
}

/***************************************************************/
/*                                                             */
/*  CreateParser                                               */
/*                                                             */
/*  Create a parser given a string buffer                      */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC void CreateParser(char *s, ParsePtr p)
#else
void CreateParser(s, p)
char *s;
ParsePtr p;
#endif
{
    p->text = s;
    p->pos = s;
    p->isnested = 0;
    p->epos = NULL;
    p->etext = NULL;
    p->allownested = 1;
    TokenPushed = NULL;
}

/***************************************************************/
/*                                                             */
/*  DestroyParser                                              */
/*                                                             */
/*  Destroy a parser, freeing up resources used.               */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC void DestroyParser(ParsePtr p)
#else
void DestroyParser(p)
ParsePtr p;
#endif
{
    if (p->isnested && p->etext) {
	free(p->etext);
	p->etext = NULL;
	p->isnested = 0;
    }
}

/***************************************************************/
/*                                                             */
/*  PushToken - one level of token pushback.  This is          */
/*  GLOBAL, not on a per-parser basis.                         */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC void PushToken(const char *tok)
#else
void PushToken(tok)
char *tok;
#endif
{
    TokenPushed = TPushBuffer;
    strcpy(TPushBuffer, tok);
    strcat(TPushBuffer, " ");  /* Separate the pushed token from the next
				  token */

}

/***************************************************************/
/*                                                             */
/*  SystemTime                                                 */
/*                                                             */
/*  Return the system time in seconds past midnight            */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC long SystemTime(int realtime)
#else
long SystemTime(realtime)
int realtime;
#endif
{
#if defined( __MSDOS__ ) && defined( __TURBOC__ )
/* Get time in Turbo C */

    struct time t;

/* If time was supplied on command line, return it. */
    if (!realtime && (SysTime != -1L)) return SysTime;

    gettime(&t);
    return (long) t.ti_hour * 3600L + (long) t.ti_min * 60L +
	(long) t.ti_sec;
#else
/* Get time in Unix or with MSC */
    time_t tloc;
    struct tm *t;

    if (!realtime && (SysTime != -1L)) return SysTime;

    (void) time(&tloc);
    t = localtime(&tloc);
    return (long) t->tm_hour * 3600L + (long) t->tm_min * 60L +
	(long) t->tm_sec;
#endif
}
/***************************************************************/
/*                                                             */
/*  SystemDate                                                 */
/*                                                             */
/*  Obtains today's date.  Returns Julian date or -1 for       */
/*  failure.  (Failure happens if sys date is before BASE      */
/*  year.)                                                     */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int SystemDate(int *y, int *m, int *d)
#else
int SystemDate(y, m, d)
int *d;
int *m;
int *y;
#endif
{
#if defined( __MSDOS__ ) && defined( __TURBOC__ )
/* Get today's date in Turbo C */
    struct date da;

    getdate(&da);
    *y = da.da_year;
    *m = da.da_mon - 1;
    *d = da.da_day;
#else
/* Get today's date in UNIX or with MSC */
    time_t tloc;
    struct tm *t;

    (void) time(&tloc);
    t = localtime(&tloc);

    *d = t->tm_mday;
    *m = t->tm_mon;
    *y = t->tm_year + 1900;
#endif
    return Julian(*y, *m, *d);
}


/***************************************************************/
/*                                                             */
/*  DoIf - handle the IF command.                              */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int DoIf(ParsePtr p)
#else
int DoIf(p)
ParsePtr p;
#endif
{
    Value v;
    int r;
    unsigned syndrome;

    if (NumIfs >= IF_NEST) return E_NESTED_IF;

    if (ShouldIgnoreLine()) syndrome = IF_TRUE | BEFORE_ELSE;
    else {
	if ( (r = EvaluateExpr(p, &v)) ) {
	    syndrome = IF_TRUE | BEFORE_ELSE;
	    Eprint("%s", ErrMsg[r]);
	} else
	    if ( (v.type != STR_TYPE && v.v.val) ||
		 (v.type == STR_TYPE && strcmp(v.v.str, "")) )
		syndrome = IF_TRUE | BEFORE_ELSE;
	    else
		syndrome = IF_FALSE | BEFORE_ELSE;
    }

    NumIfs++;
    IfFlags &= ~(IF_MASK << (2*NumIfs - 2));
    IfFlags |= syndrome << (2 * NumIfs - 2);
    if (ShouldIgnoreLine()) return OK;
    return VerifyEoln(p);
}


/***************************************************************/
/*                                                             */
/*  DoElse - handle the ELSE command.                          */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int DoElse(ParsePtr p)
#else
int DoElse(p)
ParsePtr p;
#endif
{
    unsigned syndrome;

    if (!NumIfs) return E_ELSE_NO_IF;

    syndrome = IfFlags >> (2 * NumIfs - 2);

    if ((syndrome & IF_ELSE_MASK) == AFTER_ELSE) return E_ELSE_NO_IF;

    IfFlags |= AFTER_ELSE << (2 * NumIfs - 2);
    return VerifyEoln(p);
}

/***************************************************************/
/*                                                             */
/*  DoEndif - handle the Endif command.                        */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int DoEndif(ParsePtr p)
#else
int DoEndif(p)
ParsePtr p;
#endif
{
    if (!NumIfs) return E_ENDIF_NO_IF;
    NumIfs--;
    return VerifyEoln(p);
}

/***************************************************************/
/*                                                             */
/*  DoIfTrig                                                   */
/*                                                             */
/*  Handle the IFTRIG command.                                 */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int DoIfTrig(ParsePtr p)
#else
int DoIfTrig(p)
ParsePtr p;
#endif
{
    int r;
    unsigned syndrome;
    Trigger trig;
    TimeTrig tim;
    int jul;


    if (NumIfs >= IF_NEST) return E_NESTED_IF;
    if (ShouldIgnoreLine()) syndrome = IF_TRUE | BEFORE_ELSE;
    else {
	if ( (r=ParseRem(p, &trig, &tim)) ) return r;
	if (trig.typ != NO_TYPE) return E_PARSE_ERR;
	jul = ComputeTrigger(trig.scanfrom, &trig, &r);
	if (r) syndrome = IF_TRUE | BEFORE_ELSE;
	else {
	    if (ShouldTriggerReminder(&trig, &tim, jul))
		syndrome = IF_TRUE | BEFORE_ELSE;
	    else
		syndrome = IF_FALSE | BEFORE_ELSE;
	}
    }
    NumIfs++;
    IfFlags &= ~(IF_MASK << (2*NumIfs - 2));
    IfFlags |= syndrome << (2 * NumIfs - 2);
    return OK;
}


/***************************************************************/
/*                                                             */
/*  ShouldIgnoreLine - given the current state of the IF       */
/*  stack, should we ignore the current line?                  */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int ShouldIgnoreLine(void)
#else
int ShouldIgnoreLine()
#endif
{
    register int i, syndrome;

/* Algorithm - go from outer to inner, and if any should be ignored, then
   ignore the whole. */

    for (i=0; i<NumIfs; i++) {
	syndrome = (IfFlags >> (i*2)) & IF_MASK;
	if (syndrome == IF_TRUE+AFTER_ELSE ||
	    syndrome == IF_FALSE+BEFORE_ELSE) return 1;
    }
    return 0;
}

/***************************************************************/
/*                                                             */
/*  VerifyEoln                                                 */
/*                                                             */
/*  Verify that current line contains no more tokens.          */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int VerifyEoln(ParsePtr p)
#else
int VerifyEoln(p)
ParsePtr p;
#endif
{
    int r;

    if ( (r = ParseToken(p, TokBuffer)) ) return r;
    if (*TokBuffer && (*TokBuffer != '#') && (*TokBuffer != ';')) {
	Eprint("%s: `%s'", ErrMsg[E_EXPECTING_EOL], TokBuffer);
	return E_EXTRANEOUS_TOKEN;
    }
    return OK;
}

/***************************************************************/
/*                                                             */
/*  DoDebug                                                    */
/*                                                             */
/*  Set the debug options under program control.               */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int DoDebug(ParsePtr p)
#else
int DoDebug(p)
ParsePtr p;
#endif
{
    int err;
    int ch;
    int val=1;

    while(1) {
	ch = ParseChar(p, &err, 0);
	if (err) return err;
	switch(ch) {
	case '#':
	case ';':
	case 0:
	    return OK;

	case ' ':
	case '\t':
	    break;

	case '+':
	    val = 1;
	    break;

	case '-':
	    val = 0;
	    break;

	case 'e':
	case 'E':
	    if (val) DebugFlag |=  DB_ECHO_LINE;
	    else     DebugFlag &= ~DB_ECHO_LINE;
	    break;

	case 'x':
	case 'X':
	    if (val) DebugFlag |=  DB_PRTEXPR;
	    else     DebugFlag &= ~DB_PRTEXPR;
	    break;

	case 't':
	case 'T':
	    if (val) DebugFlag |=  DB_PRTTRIG;
	    else     DebugFlag &= ~DB_PRTTRIG;
	    break;

	case 'v':
	case 'V':
	    if (val) DebugFlag |=  DB_DUMP_VARS;
	    else     DebugFlag &= ~DB_DUMP_VARS;
	    break;

	case 'l':
	case 'L':
	    if (val) DebugFlag |=  DB_PRTLINE;
	    else     DebugFlag &= ~DB_PRTLINE;
	    break;

	}
    }
}

/***************************************************************/
/*                                                             */
/*  DoBanner                                                   */
/*                                                             */
/*  Set the banner to be printed just before the first         */
/*  reminder is issued.                                        */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int DoBanner(ParsePtr p)
#else
int DoBanner(p)
ParsePtr p;
#endif
{
    int err;
    int c;
    char buf[LINELEN];   /* So we don't mess up the banner if an error occurs */
    char *s;

    c = ParseChar(p, &err, 0);
    if (err) return err;
    while (isspace(c)) {
	c = ParseChar(p, &err, 0);
	if (err) return err;
    }
    if (!c) return E_EOLN;
    s = buf;

    while(c) {
	*s++ = c;
	c = ParseChar(p, &err, 0);
	if (err) return err;
    }
    *s++ = 0;
    strcpy(Banner, buf);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  DoRun                                                      */
/*                                                             */
/*  Enable or disable the RUN command under program control    */
/*                                                             */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int DoRun(ParsePtr p)
#else
int DoRun(p)
ParsePtr p;
#endif
{
    int r;

    if ( (r=ParseToken(p, TokBuffer)) ) return r;

/* Only allow RUN ON in top-level script */
    if (! StrCmpi(TokBuffer, "ON")) {
	if (TopLevel()) RunDisabled &= ~RUN_SCRIPT;
    }
/* But allow RUN OFF anywhere */
    else if (! StrCmpi(TokBuffer, "OFF"))
	RunDisabled |= RUN_SCRIPT;
    else return E_PARSE_ERR;

    return VerifyEoln(p);
}

/***************************************************************/
/*                                                             */
/*  DoFlush                                                    */
/*                                                             */
/*  Flush stdout and stderr                                    */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int DoFlush(ParsePtr p)
#else
int DoFlush(p)
ParsePtr p;
#endif
{
    fflush(stdout);
    fflush(stderr);
    return VerifyEoln(p);
}

/***************************************************************/
/*                                                             */
/*  DoExit                                                     */
/*                                                             */
/*  Handle the EXIT command.                                   */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC void DoExit(ParsePtr p)
#else
void DoExit(p)
ParsePtr p;
#endif
{
    int r;
    Value v;

    r = EvaluateExpr(p, &v);
    if (r || v.type != INT_TYPE) exit(99);
    exit(v.v.val);
}

/***************************************************************/
/*                                                             */
/*  DoErrMsg                                                   */
/*                                                             */
/*  Issue an error message under program control.              */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int DoErrMsg(ParsePtr p)
#else
int DoErrMsg(p)
ParsePtr p;
#endif
{
    TimeTrig tt;
    Trigger t;
    int r;
    char *s;

    t.typ = MSG_TYPE;
    tt.ttime = SystemTime(0) / 60;
    if ( (r=DoSubst(p, SubstBuffer, &t, &tt, JulianToday, NORMAL_MODE)) )
	return r;
    s = SubstBuffer;
    while (isspace(*s)) s++;
    fprintf(ErrFp, "%s\n", s);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  CalcMinsFromUTC                                            */
/*                                                             */
/*  Attempt to calculate the minutes from UTC for a specific   */
/*  date.                                                      */
/*                                                             */
/***************************************************************/

/* The array FoldArray[2][7] contains sample years which begin
   on the specified weekday.  For example, FoldArray[0][2] is a
   non-leapyear beginning on Wednesday, and FoldArray[1][5] is a
   leapyear beginning on Saturday.  Used to fold back dates which
   are too high for the standard Unix representation.
   NOTE:  This implies that you cannot set BASE > 2001!!!!! */
static int FoldArray[2][7] = {
    {2001, 2002, 2003, 2009, 2010, 2005, 2006},
    {2024, 2008, 2020, 2004, 2016, 2000, 2012}
};

#ifdef HAVE_PROTOS
PUBLIC int CalcMinsFromUTC(int jul, int tim, int *mins, int *isdst)
#else
int CalcMinsFromUTC(jul, tim, mins, isdst)
int jul, tim, *mins, *isdst;
#endif
{

/* Convert jul and tim to an Unix tm struct */
    int yr, mon, day;
    struct tm local, utc, *temp;
    time_t loc_t, utc_t;

    FromJulian(jul, &yr, &mon, &day);

/* If the year is greater than 2037, some Unix machines have problems.
   Fold it back to a "similar" year and trust that the UTC calculations
   are still valid... */
    if (FoldYear && yr>2037) {
	jul = Julian(yr, 0, 1);
	yr = FoldArray[IsLeapYear(yr)][jul%7];
    }
    local.tm_sec = 0;
    local.tm_min = tim % 60;
    local.tm_hour = tim / 60;
    local.tm_mday = day;
    local.tm_mon = mon;
    local.tm_year = yr-1900;
    local.tm_isdst = -1;  /* We don't know whether or not dst is in effect */

#if !defined(HAVE_MKTIME)
    loc_t = timelocal(&local);
    local.tm_isdst = 0;
    utc_t = timegm(&local);
#else
    loc_t = mktime(&local);
    if (loc_t == -1) return 1;
    temp = gmtime(&loc_t);
    utc = *temp;
    utc.tm_isdst = 0;
    utc_t = mktime(&utc);
    if (utc_t == -1) return 1;
#endif
    temp = localtime(&loc_t);
#ifdef HAVE_MKTIME
    if (mins) *mins = (int)  ( ((temp->tm_isdst) ? 60 : 0) +
			       (loc_t - utc_t) / 60 );  /* Should use difftime */
#else
    if (mins) *mins = (int) ((utc_t - loc_t) / 60);
#endif
    if (isdst) *isdst = temp->tm_isdst;
    return 0;
}

/***************************************************************/
/*                                                             */
/*  FillParagraph                                              */
/*                                                             */
/*  Write a string to standard output, formatting it as a      */
/*  paragraph according to the FirstIndent, FormWidth and      */
/*  SubsIndent variables.  Spaces are gobbled.  Double-spaces  */
/*  are inserted after '.', '?' and '!'.  Newlines in the      */
/*  source are treated as paragraph breaks.                    */
/*                                                             */
/***************************************************************/

/* A macro safe ONLY if used with arg with no side effects! */
#define ISBLANK(c) (isspace(c) && (c) != '\n')

#ifdef HAVE_PROTOS
#ifdef OS2_POPUP
PUBLIC void FillParagraph(char *s, int AsPopUp)
#else
PUBLIC void FillParagraph(char *s)
#endif
#else
#ifdef OS2_POPUP
void FillParagraph(s, AsPopUp)
char *s;
int AsPopUp;
#else
void FillParagraph(s)
char *s;
#endif
#endif
{

    int line = 0;
    int i, j;
    int doublespace = 1;
    int pendspace;
    int len;
    char *t;

    int roomleft;

    if (!s || !*s) return;

    /* Skip leading spaces */
    while(ISBLANK(*s)) s++;

    /* Start formatting */
    while(1) {

	/* If it's a carriage return, output it and start new paragraph */
	if (*s == '\n') {
	    Putchar('\n');
	    s++;
	    line = 0;
	    while(ISBLANK(*s)) s++;
	    continue;
	}
	if (!*s) {
	    return;
	}
	/* Over here, we're at the beginning of a line.  Emit the correct
	   number of spaces */
	j = line ? SubsIndent : FirstIndent;
	for (i=0; i<j; i++) {
	    Putchar(' ');
	}

	/* Calculate the amount of room left on this line */
	roomleft = FormWidth - j;
	pendspace = 0;

	/* Emit words until the next one won't fit */
	while(1) {
	    while(ISBLANK(*s)) s++;
	    if (*s == '\n') break;
	    t = s;
	    while(*s && !isspace(*s)) s++;
	    len = s - t;
	    if (!len) {
		return;
	    }
	    if (!pendspace || len+pendspace <= roomleft) {
		for (i=0; i<pendspace; i++) {
		    Putchar(' ');
		}
		while(t < s) {
		    Putchar(*t);
		    if (strchr(EndSent, *t)) doublespace = 2;
		    else if (!strchr(EndSentIg, *t)) doublespace = 1;
		    t++;
		}
	    } else {
		s = t;
		Putchar('\n');
		line++;
		break;
	    }
	    roomleft -= len+doublespace;
	    pendspace = doublespace;
	}
    }
}

#if defined(NEED_TIMEGM) && !defined(HAVE_MKTIME)
#define		TGM_SEC		(1)
#define		TGM_MIN		(60 * TGM_SEC)
#define		TGM_HR		(60 * TGM_MIN)
#define		TGM_DAY		(24 * TGM_HR)

#ifdef HAVE_PROTOS
PRIVATE long time_cheat(int year, int month)
#else
static long time_cheat (year, month)
int year;
int month;
#endif
{
    long guess = time((long *) NULL);
    struct tm g;
    int diff;

    g = *gmtime (&guess);
    while ((diff = year - g.tm_year) > 0)
    {
	guess += diff * (363 - TGM_DAY);
	g = *gmtime (&guess);
    }
    g.tm_mday--;
    guess -= g.tm_sec * TGM_SEC + g.tm_min * TGM_MIN +
	g.tm_hour * TGM_HR + g.tm_mday * TGM_DAY;
    return (guess);
}

#ifdef HAVE_PROTOS
PUBLIC long timegm (struct tm *tm)
#else
long timegm(tm)
struct tm *tm;
#endif
{
    long clock = time_cheat (tm->tm_year, tm->tm_mon);

    return (clock + tm->tm_sec * TGM_SEC +
	    tm->tm_min * TGM_MIN +
	    tm->tm_hour * TGM_HR +
	    (tm->tm_mday - 1) * TGM_DAY);
}

#ifdef HAVE_PROTOS
PUBLIC long timelocal (struct tm *tm)
#else
long timelocal (tm)
struct tm *tm;
#endif
{
    long zero = 0;
    struct tm epoch;
    int tzmin;
    long clock;
    struct tm test;

    epoch = *localtime (&zero);
    tzmin = epoch.tm_hour * 60 + epoch.tm_min;
    if (tzmin > 0)
    {
	tzmin = 24 * 60 - tzmin;
	if (epoch.tm_year == 70)
	    tzmin -= 24 * 60;
    }
    clock = timegm (tm) + tzmin * TGM_MIN;
    test = *localtime (&clock);

    if (test.tm_hour != tm->tm_hour)
	clock -= TGM_HR;
    return (clock);
}
#endif /* NEED_TIMEGM */

/***************************************************************/
/*                                                             */
/*  LocalToUTC                                                 */
/*                                                             */
/*  Convert a local date/time to a UTC date/time.              */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC void LocalToUTC(int locdate, int loctime, int *utcdate, int *utctime)
#else
void LocalToUTC(locdate, loctime, utcdate, utctime)
int locdate, loctime, *utcdate, *utctime;
#endif
{
    int diff;
    int dummy;

    if (!CalculateUTC || CalcMinsFromUTC(locdate, loctime, &diff, &dummy)) 
	diff=MinsFromUTC;

    loctime -= diff;
    if (loctime < 0) {
	loctime += 1440;
	locdate--;
    } else if (loctime >= 1440) {
	loctime -= 1440;
	locdate++;
    }
    *utcdate = locdate;
    *utctime = loctime;
}

/***************************************************************/
/*                                                             */
/*  UTCToLocal                                                 */
/*                                                             */
/*  Convert a UTC date/time to a local date/time.              */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC void UTCToLocal(int utcdate, int utctime, int *locdate, int *loctime)
#else
void UTCToLocal(utcdate, utctime, locdate, loctime)
int utcdate, utctime, *locdate, *loctime;
#endif
{
    int diff;
    int dummy;

    /* Hack -- not quite right when DST changes.  */
    if (!CalculateUTC || CalcMinsFromUTC(utcdate, utctime, &diff, &dummy))
	diff=MinsFromUTC;

    utctime += diff;
    if (utctime < 0) {
	utctime += 1440;
	utcdate--;
    } else if (utctime >= 1440) {
	utctime -= 1440;
	utcdate++;
    }
    *locdate = utcdate;
    *loctime = utctime;
}

/***************************************************************/
/*							       */
/* SigIntHandler					       */
/*							       */
/* For debugging purposes, when sent a SIGINT, we print the    */
/* contents of the queue.  This does NOT work when the -f      */
/* command-line flag is supplied.			       */
/*							       */
/* For OS/2, this has to be in the main thread. 	       */
/*							       */
/***************************************************************/
#ifdef HAVE_QUEUED

#ifdef __BORLANDC__
void __cdecl SigIntHandler(int d)
#else
#ifdef HAVE_PROTOS
#ifdef SIGHANDLER_INT_ARG
void SigIntHandler(int d)
#else
void SigIntHandler(void)
#endif
#else
void SigIntHandler()
#endif
#endif
{
#ifdef SYSV
    signal(SIGINT, SigIntHandler);
#else
#ifdef __BORLANDC__
    signal(SIGINT, SIG_DFL);
#else
#ifdef __OS2__
    signal(SIGINT, SIG_ACK);
#endif
#endif
#endif
    GotSigInt();

#ifndef UNIX
    exit(0);
#endif
}

#endif /* HAVE_QUEUED */
