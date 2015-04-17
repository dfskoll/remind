/***************************************************************/
/*                                                             */
/*  MAIN.C                                                     */
/*                                                             */
/*  Main program loop, as well as miscellaneous conversion     */
/*  routines, etc.                                             */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by Dianne Skoll                    */
/*  Copyright (C) 1999-2011 by Roaring Penguin Software Inc.   */
/*                                                             */
/***************************************************************/

#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include <ctype.h>
#ifdef TIME_WITH_SYS_TIME
#include <time.h>
#include <sys/time.h>
#else
#if defined(HAVE_SYS_TIME_H) || defined (TIME_WITH_SYS_TIME)
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#include <sys/types.h>

#include "types.h"
#include "protos.h"
#include "expr.h"
#include "globals.h"
#include "err.h"

static void DoReminders(void);

/* Whooo... the putchar/Putchar/PutChar macros are a mess...
   my apologies... */
#define Putchar(c) PutChar(c)

/***************************************************************/
/***************************************************************/
/**                                                           **/
/**  Main Program Loop                                        **/
/**                                                           **/
/***************************************************************/
/***************************************************************/
int main(int argc, char *argv[])
{
    int pid;

#ifdef HAVE_SETLOCALE
    setlocale(LC_ALL, "");
#endif

/* The very first thing to do is to set up ErrFp to be stderr */
    ErrFp = stderr;

/* Set up global vars */
    ArgC = argc;
    ArgV = (char const **) argv;

    InitRemind(argc, (char const **) argv);
    if (DoCalendar || (DoSimpleCalendar && (!NextMode || PsCal))) {
	ProduceCalendar();
	return 0;
    }

    /* Are we purging old reminders?  Then just run through the loop once! */
    if (PurgeMode) {
	DoReminders();
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
	    if (!Daemon && !NextMode && !NumTriggered && !NumQueued) {
		printf("%s\n", ErrMsg[E_NOREMINDERS]);
	    } else if (!Daemon && !NextMode && !NumTriggered) {
		printf(ErrMsg[M_QUEUED], NumQueued);
	    }
	}

	/* If there are sorted reminders, handle them */
	if (SortByDate) IssueSortedReminders();

	/* If there are any background reminders queued up, handle them */
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

void PurgeEchoLine(char const *fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    if (PurgeFP != NULL) {
	(void) vfprintf(PurgeFP, fmt, argptr);
    }
    va_end(argptr);

}

/***************************************************************/
/*                                                             */
/*  DoReminders                                                */
/*                                                             */
/*  The normal case - we're not doing a calendar.              */
/*                                                             */
/***************************************************************/
static void DoReminders(void)
{
    int r;
    Token tok;
    char const *s;
    Parser p;
    int purge_handled;

    if (!UseStdin) {
	FileAccessDate = GetAccessDate(InitialFile);
    } else {
	FileAccessDate = JulianToday;
    }

    if (FileAccessDate < 0) {
	fprintf(ErrFp, "%s: `%s'.\n", ErrMsg[E_CANTACCESS], InitialFile);
	exit(1);
    }

    r=IncludeFile(InitialFile);
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
	    if (PurgeMode) {
		if (strncmp(CurLine, "#!P", 3)) {
		    PurgeEchoLine("%s\n", CurLine);
		}
	    }
	}
	else {
	    purge_handled = 0;
	    /* Create a parser to parse the line */
	    CreateParser(s, &p);
	    switch(tok.type) {

            case T_Empty:
	    case T_Comment:
		if (!strncmp(CurLine, "#!P", 3)) {
		    purge_handled = 1;
		}
		break;

	    case T_Rem:     r=DoRem(&p); purge_handled = 1; break;
	    case T_ErrMsg:  r=DoErrMsg(&p);  break;
	    case T_If:      r=DoIf(&p);      break;
	    case T_IfTrig:  r=DoIfTrig(&p);  break;
	    case T_Else:    r=DoElse(&p);    break;
	    case T_EndIf:   r=DoEndif(&p);   break;
	    case T_Include:
		/* In purge mode, include closes file, so we
		   need to echo it here! */
		if (PurgeMode) {
		    PurgeEchoLine("%s\n", CurLine);
		}
		r=DoInclude(&p);
		purge_handled = 1;
		break;
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
		    purge_handled = 1;
		}
		break;
	    case T_Pop:     r=PopOmitContext(&p);     break;
	    case T_Preserve: r=DoPreserve(&p);  break;
	    case T_Push:    r=PushOmitContext(&p);    break;
	    case T_RemType: if (tok.val == RUN_TYPE) {
		    r=DoRun(&p);
		} else {
		    CreateParser(CurLine, &p);
		    r=DoRem(&p);
		    purge_handled = 1;
		}
		break;


	    /* If we don't recognize the command, do a REM by default */
	    /* Note:  Since the parser hasn't been used yet, we don't */
	    /* need to destroy it here. */

	    default: CreateParser(CurLine, &p); purge_handled = 1; r=DoRem(&p); break;

	    }
	    if (r && (!Hush || r != E_RUN_DISABLED)) {
		Eprint("%s", ErrMsg[r]);
	    }
	    if (PurgeMode) {
		if (!purge_handled) {
		    PurgeEchoLine("%s\n", CurLine);
		} else {
		    if (r) {
			PurgeEchoLine("#!P! Could not parse next line: %s\n", ErrMsg[r]);
			PurgeEchoLine("%s\n", CurLine);
		    }
		}
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
int Julian(int year, int month, int day)
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
void FromJulian(int jul, int *y, int *m, int *d)
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
int ParseChar(ParsePtr p, int *err, int peek)
{
    Value val;
    int r;

    *err = 0;
    if (p->tokenPushed && *p->tokenPushed) {
	if (peek) return *p->tokenPushed;
	else {
	    r = *p->tokenPushed++;
	    if (!r) {
		DBufFree(&p->pushedToken);
		p->tokenPushed = NULL;
	    }
	    return r;
	}
    }

    while(1) {
	if (p->isnested) {
	    if (*(p->epos)) {
		if (peek) {
		    return *(p->epos);
		} else {
		    return *(p->epos++);
		}
	    }
	    free((void *) p->etext);  /* End of substituted expression */
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
	p->expr_happened = 1;
	p->pos++;
	r = EvalExpr(&(p->pos), &val, p);
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
int ParseNonSpaceChar(ParsePtr p, int *err, int peek)
{
    int ch;

    ch = ParseChar(p, err, 1);
    if (*err) return 0;

    while (isempty(ch)) {
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
int ParseToken(ParsePtr p, DynamicBuffer *dbuf)
{
    int c, err;

    DBufFree(dbuf);

    c = ParseChar(p, &err, 0);
    if (err) return err;
    while (c && isempty(c)) {
	c = ParseChar(p, &err, 0);
	if (err) return err;
    }
    if (!c) return OK;
    while (c && !isempty(c)) {
	if (DBufPutc(dbuf, c) != OK) {
	    DBufFree(dbuf);
	    return E_NO_MEM;
	}
	c = ParseChar(p, &err, 0);
	if (err) {
	    DBufFree(dbuf);
	    return err;
	}
    }
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
int ParseIdentifier(ParsePtr p, DynamicBuffer *dbuf)
{
    int c, err;

    DBufFree(dbuf);

    c = ParseChar(p, &err, 0);
    if (err) return err;
    while (c && isempty(c)) {
	c = ParseChar(p, &err, 0);
	if (err) return err;
    }
    if (!c) return E_EOLN;
    if (c != '$' && c != '_' && !isalpha(c)) return E_BAD_ID;
    if (DBufPutc(dbuf, c) != OK) {
	DBufFree(dbuf);
	return E_NO_MEM;
    }

    while (1) {
	c = ParseChar(p, &err, 1);
	if (err) {
	    DBufFree(dbuf);
	    return err;
	}
	if (c != '_' && !isalnum(c)) return OK;
	c = ParseChar(p, &err, 0);  /* Guaranteed to work */
	if (DBufPutc(dbuf, c) != OK) {
	    DBufFree(dbuf);
	    return E_NO_MEM;
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
int EvaluateExpr(ParsePtr p, Value *v)
{

    int bracketed = 0;
    int r;

    if (p->isnested) return E_PARSE_ERR;  /* Can't nest expressions */
    while (isempty(*p->pos)) (p->pos)++;
    if (!p->pos) return E_PARSE_ERR;      /* Missing expression */
    if (*p->pos == BEG_OF_EXPR) {
	(p->pos)++;
	bracketed = 1;
    }
    r = EvalExpr(&(p->pos), v, p);
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
void Eprint(char const *fmt, ...)
{
    va_list argptr;

    /* Check if more than one error msg. from this line */
    if (!FreshLine && !ShowAllErrors) return;

    if (FreshLine && FileName) {
	FreshLine = 0;
	if (strcmp(FileName, "-"))
	    (void) fprintf(ErrFp, "%s(%d): ", FileName, LineNo);
	else
	    (void) fprintf(ErrFp, "-stdin-(%d): ", LineNo);
	if (DebugFlag & DB_PRTLINE) OutputLine(ErrFp);
    } else if (FileName) {
	fprintf(ErrFp, "       ");
    }

    va_start(argptr, fmt);
    (void) vfprintf(ErrFp, fmt, argptr);
    (void) fputc('\n', ErrFp);
    va_end(argptr);
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
void OutputLine(FILE *fp)
{
    char const *s = CurLine;
    char c = 0;

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
void CreateParser(char const *s, ParsePtr p)
{
    p->text = s;
    p->pos = s;
    p->isnested = 0;
    p->epos = NULL;
    p->etext = NULL;
    p->allownested = 1;
    p->tokenPushed = NULL;
    p->expr_happened = 0;
    p->nonconst_expr = 0;
    DBufInit(&p->pushedToken);
}

/***************************************************************/
/*                                                             */
/*  DestroyParser                                              */
/*                                                             */
/*  Destroy a parser, freeing up resources used.               */
/*                                                             */
/***************************************************************/
void DestroyParser(ParsePtr p)
{
    if (p->isnested && p->etext) {
	free((void *) p->etext);
	p->etext = NULL;
	p->isnested = 0;
    }
    DBufFree(&p->pushedToken);
}

/***************************************************************/
/*                                                             */
/*  PushToken - one level of token pushback.  This is          */
/*  on a per-parser basis.                                     */
/*                                                             */
/***************************************************************/
int PushToken(char const *tok, ParsePtr p)
{
    DBufFree(&p->pushedToken);
    if (DBufPuts(&p->pushedToken, tok) != OK ||
	DBufPutc(&p->pushedToken, ' ') != OK) {
	DBufFree(&p->pushedToken);
	return E_NO_MEM;
    }
    p->tokenPushed = DBufValue(&p->pushedToken);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  SystemTime                                                 */
/*                                                             */
/*  Return the system time in seconds past midnight            */
/*                                                             */
/***************************************************************/
long SystemTime(int realtime)
{
    time_t tloc;
    struct tm *t;

    if (!realtime && (SysTime != -1L)) return SysTime;

    (void) time(&tloc);
    t = localtime(&tloc);
    return (long) t->tm_hour * 3600L + (long) t->tm_min * 60L +
	(long) t->tm_sec;
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
int SystemDate(int *y, int *m, int *d)
{
    time_t tloc;
    struct tm *t;

    (void) time(&tloc);
    t = localtime(&tloc);

    *d = t->tm_mday;
    *m = t->tm_mon;
    *y = t->tm_year + 1900;

    return Julian(*y, *m, *d);
}


/***************************************************************/
/*                                                             */
/*  DoIf - handle the IF command.                              */
/*                                                             */
/***************************************************************/
int DoIf(ParsePtr p)
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
		 (v.type == STR_TYPE && strcmp(v.v.str, "")) ) {
		syndrome = IF_TRUE | BEFORE_ELSE;
	    } else {
		syndrome = IF_FALSE | BEFORE_ELSE;
		if (PurgeMode) {
		    PurgeEchoLine("%s\n", "#!P: The next IF evaluated false...");
		    PurgeEchoLine("%s\n", "#!P: REM statements in IF block not checked for purging.");
		}
	    }
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
int DoElse(ParsePtr p)
{
    unsigned syndrome;

    int was_ignoring = ShouldIgnoreLine();

    if (!NumIfs) return E_ELSE_NO_IF;

    syndrome = IfFlags >> (2 * NumIfs - 2);

    if ((syndrome & IF_ELSE_MASK) == AFTER_ELSE) return E_ELSE_NO_IF;

    IfFlags |= AFTER_ELSE << (2 * NumIfs - 2);
    if (PurgeMode && ShouldIgnoreLine() && !was_ignoring) {
	PurgeEchoLine("%s\n", "#!P: The previous IF evaluated true.");
	PurgeEchoLine("%s\n", "#!P: REM statements in ELSE block not checked for purging");
    }
    return VerifyEoln(p);
}

/***************************************************************/
/*                                                             */
/*  DoEndif - handle the Endif command.                        */
/*                                                             */
/***************************************************************/
int DoEndif(ParsePtr p)
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
int DoIfTrig(ParsePtr p)
{
    int r, err;
    unsigned syndrome;
    Trigger trig;
    TimeTrig tim;
    int jul;


    if (NumIfs >= IF_NEST) return E_NESTED_IF;
    if (ShouldIgnoreLine()) syndrome = IF_TRUE | BEFORE_ELSE;
    else {
	if ( (r=ParseRem(p, &trig, &tim, 1)) ) return r;
	if (trig.typ != NO_TYPE) return E_PARSE_ERR;
	jul = ComputeTrigger(trig.scanfrom, &trig, &r, 1);
	if (r) syndrome = IF_TRUE | BEFORE_ELSE;
	else {
	    if (ShouldTriggerReminder(&trig, &tim, jul, &err)) {
		syndrome = IF_TRUE | BEFORE_ELSE;
	    } else {
		syndrome = IF_FALSE | BEFORE_ELSE;
		if (PurgeMode) {
		    PurgeEchoLine("%s\n", "#!P: The next IFTRIG did not trigger.");
		    PurgeEchoLine("%s\n", "#!P: REM statements in IFTRIG block not checked for purging.");
		}
	    }
	}
	FreeTrig(&trig);
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
int ShouldIgnoreLine(void)
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
int VerifyEoln(ParsePtr p)
{
    int r;

    DynamicBuffer buf;
    DBufInit(&buf);

    if ( (r = ParseToken(p, &buf)) ) return r;
    if (*DBufValue(&buf) &&
	(*DBufValue(&buf) != '#') &&
	(*DBufValue(&buf) != ';')) {
	Eprint("%s: `%s'", ErrMsg[E_EXPECTING_EOL], DBufValue(&buf));
	DBufFree(&buf);
	return E_EXTRANEOUS_TOKEN;
    }
    DBufFree(&buf);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  DoDebug                                                    */
/*                                                             */
/*  Set the debug options under program control.               */
/*                                                             */
/***************************************************************/
int DoDebug(ParsePtr p)
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

	case 'f':
	case 'F':
	    if (val) DebugFlag |= DB_TRACE_FILES;
	    else     DebugFlag &= ~DB_TRACE_FILES;
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
int DoBanner(ParsePtr p)
{
    int err;
    int c;
    DynamicBuffer buf;

    DBufInit(&buf);
    c = ParseChar(p, &err, 0);
    if (err) return err;
    while (isempty(c)) {
	c = ParseChar(p, &err, 0);
	if (err) return err;
    }
    if (!c) return E_EOLN;

    while(c) {
	if (DBufPutc(&buf, c) != OK) return E_NO_MEM;
	c = ParseChar(p, &err, 0);
	if (err) {
	    DBufFree(&buf);
	    return err;
	}
    }
    DBufFree(&Banner);
    
    err = DBufPuts(&Banner, DBufValue(&buf));
    DBufFree(&buf);
    return err;
}

/***************************************************************/
/*                                                             */
/*  DoRun                                                      */
/*                                                             */
/*  Enable or disable the RUN command under program control    */
/*                                                             */
/*                                                             */
/***************************************************************/
int DoRun(ParsePtr p)
{
    int r;

    DynamicBuffer buf;
    DBufInit(&buf);

    if ( (r=ParseToken(p, &buf)) ) return r;

/* Only allow RUN ON in top-level script */
    if (! StrCmpi(DBufValue(&buf), "ON")) {
	if (TopLevel()) RunDisabled &= ~RUN_SCRIPT;
    }
/* But allow RUN OFF anywhere */
    else if (! StrCmpi(DBufValue(&buf), "OFF"))
	RunDisabled |= RUN_SCRIPT;
    else {
	DBufFree(&buf);
	return E_PARSE_ERR;
    }
    DBufFree(&buf);

    return VerifyEoln(p);
}

/***************************************************************/
/*                                                             */
/*  DoFlush                                                    */
/*                                                             */
/*  Flush stdout and stderr                                    */
/*                                                             */
/***************************************************************/
int DoFlush(ParsePtr p)
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
void DoExit(ParsePtr p)
{
    int r;
    Value v;

    if (PurgeMode) return;

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
int DoErrMsg(ParsePtr p)
{
    TimeTrig tt;
    Trigger t;
    int r;
    char const *s;

    DynamicBuffer buf;

    if (PurgeMode) return OK;

    DBufInit(&buf);
    t.typ = MSG_TYPE;
    tt.ttime = SystemTime(0) / 60;
    if ( (r=DoSubst(p, &buf, &t, &tt, JulianToday, NORMAL_MODE)) ) {
	return r;
    }
    s = DBufValue(&buf);
    while (isempty(*s)) s++;
    fprintf(ErrFp, "%s\n", s);
    DBufFree(&buf);
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

int CalcMinsFromUTC(int jul, int tim, int *mins, int *isdst)
{

/* Convert jul and tim to an Unix tm struct */
    int yr, mon, day;
    int tdiff;
    struct tm local, utc, *temp;
    time_t loc_t, utc_t;
    int isdst_tmp;

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


    /* Horrible contortions to get minutes from UTC portably */
    loc_t = mktime(&local);
    if (loc_t == -1) return 1;
    isdst_tmp = local.tm_isdst;
    local.tm_isdst = 0;
    loc_t = mktime(&local);
    if (loc_t == -1) return 1;
    temp = gmtime(&loc_t);
    utc = *temp;
    utc.tm_isdst = 0;
    utc_t = mktime(&utc);
    if (utc_t == -1) return 1;
    /* Compute difference between local time and UTC in seconds.
       Be careful, since time_t might be unsigned. */

    tdiff = (int) difftime(loc_t, utc_t);
    if (isdst_tmp) tdiff += 60*60;
    if (mins) *mins = (int)(tdiff / 60);
    if (isdst) *isdst = isdst_tmp;
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

void FillParagraph(char const *s)
{

    int line = 0;
    int i, j;
    int doublespace = 1;
    int pendspace;
    int len;
    char const *t;

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

/***************************************************************/
/*                                                             */
/*  LocalToUTC                                                 */
/*                                                             */
/*  Convert a local date/time to a UTC date/time.              */
/*                                                             */
/***************************************************************/
void LocalToUTC(int locdate, int loctime, int *utcdate, int *utctime)
{
    int diff;
    int dummy;

    if (!CalculateUTC || CalcMinsFromUTC(locdate, loctime, &diff, &dummy)) 
	diff=MinsFromUTC;

    loctime -= diff;
    if (loctime < 0) {
	loctime += MINUTES_PER_DAY;
	locdate--;
    } else if (loctime >= MINUTES_PER_DAY) {
	loctime -= MINUTES_PER_DAY;
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
void UTCToLocal(int utcdate, int utctime, int *locdate, int *loctime)
{
    int diff;
    int dummy;

    /* Hack -- not quite right when DST changes.  */
    if (!CalculateUTC || CalcMinsFromUTC(utcdate, utctime, &diff, &dummy))
	diff=MinsFromUTC;

    utctime += diff;
    if (utctime < 0) {
	utctime += MINUTES_PER_DAY;
	utcdate--;
    } else if (utctime >= MINUTES_PER_DAY) {
	utctime -= MINUTES_PER_DAY;
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
/***************************************************************/

void SigIntHandler(int d)
{
    signal(SIGINT, SigIntHandler);
    GotSigInt();
    exit(0);
}

void
AppendTag(DynamicBuffer *buf, char const *s)
{
    if (*(DBufValue(buf))) {
	DBufPutc(buf, ',');
    }
    DBufPuts(buf, s);
}

void
FreeTrig(Trigger *t)
{
    DBufFree(&(t->tags));
}
