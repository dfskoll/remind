/***************************************************************/
/*                                                             */
/*  TOKEN.C                                                    */
/*                                                             */
/*  Contains routines for parsing the reminder file and        */
/*  classifying the tokens parsed.                             */
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
#include "types.h"
#include "globals.h"
#include "protos.h"
#include "err.h"

/* The macro PARSENUM parses a char pointer as an integer.  It simply
   executes 'return' if an initial non-numeric char is found. */
#define PARSENUM(var, string) \
if (!isdigit(*(string))) return; \
var = 0; \
while (isdigit(*(string))) { \
    var *= 10; \
    var += *(string) - '0'; \
    string++; \
}

/* The big array holding all recognized (literal) tokens in reminder file.
   Keep this array sorted, or software will not work. */
Token TokArray[] = {
    /* NAME          MINLEN      TYPE           VALUE */

    { "after",		3,	T_Skip,	AFTER_SKIP },
    { "april",		3,	T_Month,	3 },
    { "at",		2,	T_At,		0 },
    { "august",		3,	T_Month,	7 },
    { "banner",		3,	T_Banner,	0 },
    { "before",	3,	T_Skip,	BEFORE_SKIP },
    { "cal",		3,	T_RemType,	CAL_TYPE },
    { "clear-omit-context", 5,   T_Clr,         0 },
    { "debug",          5,      T_Debug,       0 },
    { "december",	3,	T_Month,       11 },
    { "dumpvars",       4,      T_Dumpvars,    0 },
    { "duration",       3,      T_Duration,     0 },
    { "else",		4,	T_Else,	0 },
    { "endif",		5,	T_EndIf,	0 },
    { "errmsg",         6,      T_ErrMsg,      0 },
    { "exit",		4,	T_Exit,		0 },
    { "february",	3,	T_Month,	1 },
    { "flush",		5,	T_Flush,	0 },
    { "friday",		3,	T_WkDay,	4 },
    { "from",		4,	T_Scanfrom,	FROM_TYPE },
    { "fset",		4,	T_Fset,		0 },
    { "if",		2,	T_If,		0 },
    { "iftrig",		6,	T_IfTrig,	0 },
    { "include",	3,	T_Include,	0 },
    { "january",	3,	T_Month,	0 },
    { "july",		3,	T_Month,	6 },
    { "june",		3,	T_Month,	5 },
    { "march",		3,	T_Month,	2 },
    { "may",		3,	T_Month,	4 },
    { "monday",		3,	T_WkDay,	0 },
    { "msf",		3,	T_RemType,	MSF_TYPE },
    { "msg",		3,	T_RemType,	MSG_TYPE },
    { "november",	3,	T_Month,	10 },
    { "october",	3,	T_Month,	9 },
    { "omit",		3,	T_Omit,		0 },
    { "omitfunc",       8,      T_OmitFunc,     0 },
    { "once",		3,	T_Once,		0 },
    { "pop-omit-context", 3,	T_Pop,		0 },
    { "preserve",       8,      T_Preserve,     0 },
    { "priority",	8,	T_Priority,	0 },
    { "ps",		2,	T_RemType,	PS_TYPE },
    { "psfile",	6,	T_RemType,	PSF_TYPE },
    { "push-omit-context", 4,	T_Push,		0 },
    { "rem",		3,	T_Rem,		0 },
    { "run",		3,	T_RemType,	RUN_TYPE },
    { "satisfy",	7,	T_RemType,      SAT_TYPE },
    { "saturday",	3,	T_WkDay,	5 },
    { "scanfrom",	4,	T_Scanfrom,	SCANFROM_TYPE },
    { "sched",		5,	T_Sched,	0 },
    { "september",	3,	T_Month,	8 },
    { "set",		3,	T_Set,		0 },
    { "skip",		3,	T_Skip,	SKIP_SKIP },
    { "special",        7,      T_RemType,      PASSTHRU_TYPE },
    { "sunday",		3,	T_WkDay,	6 },
    { "tag",            3,      T_Tag,          0 },
    { "through",        7,      T_Through,      0 },
    { "thursday",	3,	T_WkDay,	3 },
    { "tuesday",	3,	T_WkDay,	1 },
    { "unset",		5,	T_UnSet,	0 },
    { "until",		3,	T_Until,	0 },
    { "warn",           4,      T_Warn,         0 },
    { "wednesday",	3,	T_WkDay,	2 }
};

/* If language != English, we must also search the following... */
#if LANG != ENGLISH
Token NonEnglishToks[] = {
    /* NAME          MINLEN      TYPE           VALUE */

    { L_MONDAY,	       3,       T_WkDay,        0 },
    { L_TUESDAY,       3,       T_WkDay,        1 },
    { L_WEDNESDAY,     3,       T_WkDay,        2 },
    { L_THURSDAY,      3,       T_WkDay,        3 },
    { L_FRIDAY,	       3,       T_WkDay,        4 },
    { L_SATURDAY,      3,       T_WkDay,        5 },
    { L_SUNDAY,	       3,       T_WkDay,        6 },
    { L_JAN,           3,       T_Month,        0 },
    { L_FEB,           3,       T_Month,        1 },
    { L_MAR,           3,       T_Month,        2 },
    { L_APR,           3,       T_Month,        3 },
    { L_MAY,           3,       T_Month,        4 },
    { L_JUN,           3,       T_Month,        5 },
    { L_JUL,           3,       T_Month,        6 },
    { L_AUG,           3,       T_Month,        7 },
    { L_SEP,           3,       T_Month,        8 },
    { L_OCT,           3,       T_Month,        9 },
    { L_NOV,           3,       T_Month,        10 },
    { L_DEC,           3,       T_Month,        11 }
};
#endif

static int TokStrCmp (Token const *t, char const *s);

/***************************************************************/
/*                                                             */
/*  FindInitialToken                                           */
/*                                                             */
/*  Find the initial token on the command line.  If it's a     */
/*  left square bracket, return a T_Illegal type.              */
/*                                                             */
/***************************************************************/
char const *FindInitialToken(Token *tok, char const *s)
{
    DynamicBuffer buf;
    DBufInit(&buf);

    tok->type = T_Illegal;

    while (isempty(*s)) s++;

    while (*s && !isempty(*s)) {
	if (DBufPutc(&buf, *s++) != OK) return s;
    }

    FindToken(DBufValue(&buf), tok);
    DBufFree(&buf);

    return s;
}


/***************************************************************/
/*                                                             */
/*  FindToken                                                  */
/*                                                             */
/*  Given a string, which token is it?                         */
/*                                                             */
/***************************************************************/
void FindToken(char const *s, Token *tok)
{
    int top, bot, mid, r, max;
    int l;

    tok->type = T_Illegal;
    if (! *s) {
	tok->type = T_Empty;
	return;
    }

    if (*s == '#' || *s == ';') {
	tok->type = T_Comment;
	return;
    }

    /* Quickly give up the search if first char not a letter */
    if ( ! isalpha(*s)) {
	FindNumericToken(s, tok);
	return;
    }

    l = strlen(s);

    /* Ignore trailing commas */
    if (l > 0 && s[l-1] == ',') {
	l--;
    }
    bot = 0;
    top = sizeof(TokArray) / sizeof(TokArray[0]) - 1;
    max = sizeof(TokArray) / sizeof(TokArray[0]);

    while(top >= bot) {
	mid = (top + bot) / 2;
	r = TokStrCmp(&TokArray[mid], s);
	if (!r) {
	    if (l >= TokArray[mid].MinLen) {
		tok->type = TokArray[mid].type;
		tok->val  = TokArray[mid].val;
		return;
	    } else {
		while (mid && !TokStrCmp(&TokArray[mid-1],s)) mid--;
		while (mid < max &&
		       !TokStrCmp(&TokArray[mid], s) &&
		       l < TokArray[mid].MinLen) {
		    mid++;
		}
		if (mid < max &&
		    !TokStrCmp(&TokArray[mid], s)) {
		    tok->type = TokArray[mid].type;
		    tok->val = TokArray[mid].val;
		    return;
		}
	    }
	    break;
	}
	if (r > 0) top = mid-1; else bot=mid+1;
    }

/* If language is other than English, search the DayNames[] and MonthNames[]
   array. */
#if LANG != ENGLISH
    for (r=0; r<(sizeof(NonEnglishToks) / sizeof(Token)); r++) {
	if (l >= NonEnglishToks[r].MinLen &&
	    !TokStrCmp(&NonEnglishToks[r], s)) {
	    tok->type = NonEnglishToks[r].type;
	    tok->val = NonEnglishToks[r].val;
	    return;
	}
    }
#endif

    return;
}

/***************************************************************/
/*                                                             */
/*  FindNumericToken                                           */
/*                                                             */
/*  Parse a numeric token:                                     */
/*  Year - number between 1990 and 2085, or 90-99.             */
/*  Day - number between 1 and 31                              */
/*  Delta - +[+]n                                              */
/*  Back - -[-]n                                               */
/*  Rep - *n                                                   */
/*                                                             */
/***************************************************************/
void FindNumericToken(char const *s, Token *t)
{
    int mult = 1, hour, min;
    char const *s_orig = s;

    t->type = T_Illegal;
    t->val = 0;
    if (isdigit(*s)) {
	PARSENUM(t->val, s);

	/* If we hit a '-' or a '/', we may have a date or a datetime */
	if (*s == '-' || *s == '/') {
	    char const *p = s_orig;
	    int jul, tim;
	    if (ParseLiteralDate(&p, &jul, &tim) == OK) {
		if (*p) return;
		if (tim == NO_TIME) {
		    t->type = T_Date;
		    t->val = jul;
		    return;
		}
		t->type = T_DateTime;
		t->val = MINUTES_PER_DAY * jul + tim;
	    }
	    return;
	}

	/* If we hit a comma, swallow it.  This allows stuff
	   like Jan 6, 1998 */
	if (*s == ',') {
	    s++;
	    /* Special hack - convert years between 90 and
	       99 to 1990 and 1999 */
	    if (t->val >= 90 && t->val <= 99) t->val += 1900;

	    /* Classify the number we've got */
	    if (t->val >= BASE && t->val <= BASE+YR_RANGE) t->type = T_Year;
	    else if (t->val >= 1 && t->val <= 31) t->type = T_Day;
	    else t->type = T_Number;
	    return;
	}
	/* If we hit a colon or a period, we've probably got a time hr:min */
	if (*s == ':' || *s == '.' || *s == TimeSep) {
	    s++;
	    hour = t->val;
	    PARSENUM(min, s);
	    if (*s || min > 59) return;  /* Illegal time */
	    t->val = hour*60 + min;  /* Convert to minutes past midnight */
	    if (hour <= 23) {
		t->type = T_Time;
	    } else {
		t->type = T_LongTime;
	    }
	    return;
	}

	/* If we hit a non-digit, error! */
	if (*s) return;

	/* Special hack - convert years between 90 and 99 to 1990 and 1999 */
	if (t->val >= 90 && t->val <= 99) t->val += 1900;

	/* Classify the number we've got */
	if (t->val >= BASE && t->val <= BASE+YR_RANGE) t->type = T_Year;
	else if (t->val >= 1 && t->val <= 31) t->type = T_Day;
	else t->type = T_Number;
	return;
    } else if (*s == '*') {
	s++;
	PARSENUM(t->val, s);
	if (*s) return;  /* Illegal token if followed by non-numeric char */
	t->type = T_Rep;
	return;
    } else if (*s == '+') {
	s++;
	if (*s == '+') { mult = -1; s++; }
	PARSENUM(t->val, s);
	if (*s) return;  /* Illegal token if followed by non-numeric char */
	t->type = T_Delta;
	t->val *= mult;
	return;
    } else if (*s == '-') {
	s++;
	if (*s == '-') { mult = -1; s++; }
	PARSENUM(t->val, s);
	if (*s) return;  /* Illegal token if followed by non-numeric char */
	t->type = T_Back;
	t->val *= mult;
	return;
    }
    return;  /* Unknown token type */
}


/***************************************************************/
/*                                                             */
/*  TokStrCmp                                                  */
/*                                                             */
/*  Compare a token to a string.                               */
/*                                                             */
/***************************************************************/
static int TokStrCmp(Token const *t, char const *s)
{
    register int r;
    char const *tk = t->name;
    while(*tk && *s && !(*s == ',' && *(s+1) == 0)) {
	r = tolower(*tk) - tolower(*s);
	tk++;
	s++;
	if (r) return r;
    }
    /* Ignore trailing commas on s */

    if (!*s || (*s == ',' && !*(s+1))) return 0;
    return (tolower(*tk) - tolower(*s));
}
