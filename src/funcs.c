/***************************************************************/
/*                                                             */
/*  FUNCS.C                                                    */
/*                                                             */
/*  This file contains the built-in functions used in          */
/*  expressions.                                               */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by David F. Skoll                  */
/*  Copyright (C) 1999-2000 by Roaring Penguin Software Inc.   */
/*                                                             */
/***************************************************************/

#include "version.h"
#include "config.h"

#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <unistd.h>

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#include <sys/types.h>

#include <sys/stat.h>

#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#else
#include <time.h>
#endif

#ifndef R_OK
#define R_OK 4
#define W_OK 2
#define X_OK 1
#endif

#include "types.h"
#include "globals.h"
#include "protos.h"
#include "err.h"
#include "expr.h"

/* Function prototypes */
static int FCurrent (void);
static int FNonomitted (void);
static int FTimepart(void);
static int FDatepart(void);
static int FRealCurrent(void);
static	int	FAbs		(void);
static	int	FAccess		(void);
static int     FArgs		(void);
static	int	FAsc		(void);
static	int	FBaseyr		(void);
static	int	FChar		(void);
static	int	FChoose		(void);
static	int	FCoerce		(void);
static	int	FDate		(void);
static	int	FDateTime	(void);
static	int	FDay		(void);
static	int	FDaysinmon	(void);
static	int	FDefined	(void);
static	int	FDosubst	(void);
static	int	FEasterdate	(void);
static int	FFiledate	(void);
static int	FFiledatetime	(void);
static	int	FFiledir	(void);
static	int	FFilename	(void);
static	int	FGetenv		(void);
static int     FHebdate	(void);
static int     FHebday		(void);
static int     FHebmon		(void);
static int     FHebyear	(void);
static	int	FHour		(void);
static	int	FIif		(void);
static	int	FIndex		(void);
static	int	FIsdst		(void);
static	int	FIsomitted	(void);
static	int	FLanguage	(void);
static	int	FMax		(void);
static	int	FMin		(void);
static	int	FMinute		(void);
static	int	FMinsfromutc	(void);
static	int	FMoondate	(void);
static	int	FMoondatetime	(void);
static	int	FMoonphase	(void);
static	int	FMoontime	(void);
static	int	FMon		(void);
static	int	FMonnum		(void);
static	int	FOrd		(void);
static	int	FOstype 	(void);
static	int	FPlural		(void);
static	int	FSgn		(void);
static int	FPsmoon		(void);
static int	FPsshade	(void);
static	int	FShell		(void);
static	int	FStrlen		(void);
static	int	FSubstr		(void);
static	int	FDawn		(void);
static	int	FDusk	 	(void);
static	int	FSunset		(void);
static	int	FSunrise	(void);
static	int	FTime		(void);
static	int	FTrigdate	(void);
static	int	FTrigdatetime	(void);
static	int	FTrigtime	(void);
static	int	FTrigvalid	(void);
static	int	FTypeof		(void);
static	int	FUpper		(void);
static	int	FValue		(void);
static	int	FVersion	(void);
static	int	FWkday		(void);
static	int	FWkdaynum	(void);
static	int	FYear		(void);
static int	FIsleap         (void);
static int	FLower          (void);
static int	FNow            (void);
static int	FRealnow            (void);
static int	FRealtoday      (void);
static int	FToday          (void);
static int	FTrigger        (void);
static int      FTzconvert      (void);
static int	CheckArgs       (Operator *f, int nargs);
static int	CleanUpAfterFunc (void);
static int	SunStuff	(int rise, double cosz, int jul);

/* "Overload" the struct Operator definition */
#define NO_MAX 127
#define MINARGS prec
#define MAXARGS type

/* Sigh - we use a global var. to hold the number of args supplied to
   function being called */
static int Nargs;

/* Use a global var. to hold function return value */
static Value RetVal;

/* Temp string buffer */
static char Buffer[32];

/* Caches for extracting months, days, years from dates - may
   improve performance slightly. */
static int CacheJul = -1;
static int CacheYear, CacheMon, CacheDay;

static int CacheHebJul = -1;
static int CacheHebYear, CacheHebMon, CacheHebDay;

/* We need access to the value stack */
extern Value ValStack[];
extern int ValStackPtr;

/* Macro for accessing arguments from the value stack - args are numbered
   from 0 to (Nargs - 1) */
#define ARG(x) (ValStack[ValStackPtr - Nargs + (x)])

/* Macro for getting date part of a date or datetime value */
#define DATEPART(x) ((x).type == DATE_TYPE ? (x).v.val : ((x).v.val / MINUTES_PER_DAY))

/* Macro for getting time part of a time or datetime value */
#define TIMEPART(x) ((x).type == TIME_TYPE ? (x).v.val : ((x).v.val % MINUTES_PER_DAY))

#define HASDATE(x) ((x).type == DATE_TYPE || (x).type == DATETIME_TYPE)
#define HASTIME(x) ((x).type == TIME_TYPE || (x).type == DATETIME_TYPE)

/* Macro for copying a value while destroying original copy */
#define DCOPYVAL(x, y) ( (x) = (y), (y).type = ERR_TYPE )

/* Convenience macros */
#define UPPER(c) (islower(c) ? toupper(c) : c)
#define LOWER(c) (isupper(c) ? tolower(c) : c)

/* The array holding the built-in functions. */
Operator Func[] = {
/*	Name		minargs maxargs	func   */

    {   "abs",		1,	1,	FAbs	},
    {   "access",       2,      2,      FAccess },
    {   "args",         1,      1,      FArgs   },
    {   "asc",		1,	1,	FAsc	},
    {   "baseyr",	0,	0,	FBaseyr	},
    {   "char",		1,	NO_MAX,	FChar	},
    {   "choose",	2,	NO_MAX, FChoose },
    {   "coerce",	2,	2,	FCoerce },
    {   "current",      0,      0,      FCurrent },
    {   "date",		3,	3,	FDate	},
    {   "datepart",	1,	1,	FDatepart },
    {   "datetime",	2,	5,	FDateTime },
    {   "dawn",		0,	1,	FDawn},
    {   "day",		1,	1,	FDay	},
    {   "daysinmon",	2,	2,	FDaysinmon },
    {   "defined",	1,	1,	FDefined },
    {   "dosubst",	1,	3,	FDosubst },
    {   "dusk",		0,	1,	FDusk },
    {   "easterdate",	1,	1,	FEasterdate },
    {	"filedate",	1,	1,	FFiledate },
    {	"filedatetime",	1,	1,	FFiledatetime },
    {	"filedir",	0,	0,	FFiledir },
    {   "filename",	0,	0,	FFilename },
    {   "getenv",	1,	1,	FGetenv },
    {   "hebdate",	2,	5,	FHebdate },
    {   "hebday",	1,	1,	FHebday },
    {   "hebmon",	1,	1,	FHebmon },
    {   "hebyear",	1,	1,	FHebyear },
    {   "hour",		1,	1,	FHour	},
    {   "iif",		1,	NO_MAX,	FIif	},
    {   "index",	2,	3,	FIndex 	},
    {   "isdst",	0,	2,	FIsdst },
    {   "isleap",	1,	1,	FIsleap },
    {   "isomitted",	1,	1,	FIsomitted },
    {   "language",     0,      0,      FLanguage },
    {   "lower",	1,	1,	FLower	},
    {   "max",		1,	NO_MAX,	FMax	},
    {   "min",		1,	NO_MAX, FMin	},
    {   "minsfromutc",	0,	2,	FMinsfromutc },
    {   "minute",	1,	1,	FMinute },
    {   "mon",		1,	1,	FMon	},
    {   "monnum",	1,	1,	FMonnum },
    {	"moondate",	1,	3,	FMoondate },
    {	"moondatetime",	1,	3,	FMoondatetime },
    {	"moonphase",	0,	2,	FMoonphase },
    {	"moontime",	1,	3,	FMoontime },
    {   "nonomitted",   2,      NO_MAX, FNonomitted },
    {   "now",		0,	0,	FNow	},
    {   "ord",		1,	1,	FOrd	},
    {   "ostype",       0,      0,      FOstype },
    {   "plural",	1,	3,	FPlural },
    {	"psmoon",	1,	4,	FPsmoon},
    {	"psshade",	1,	3,	FPsshade},
    {   "realcurrent",  0,      0,      FRealCurrent},
    {   "realnow",      0,      0,      FRealnow},
    {   "realtoday",    0,      0,      FRealtoday },
    {   "sgn",		1,	1,	FSgn	},
    {   "shell",	1,	2,	FShell	},
    {   "strlen",	1,	1,	FStrlen	},
    {   "substr",	2,	3,	FSubstr	},
    {   "sunrise",	0,	1,	FSunrise},
    {   "sunset",	0,	1,	FSunset },
    {   "time",		2,	2,	FTime	},
    {   "timepart",	1,	1,	FTimepart },
    {   "today",	0,	0,	FToday	},
    {   "trigdate",	0,	0,	FTrigdate },
    {   "trigdatetime",	0,	0,	FTrigdatetime },
    {   "trigger",	1,	3,	FTrigger },
    {   "trigtime",	0,	0,	FTrigtime },
    {   "trigvalid",	0,	0,	FTrigvalid },
    {   "typeof",       1,      1,      FTypeof },
    {   "tzconvert",    2,      3,      FTzconvert },
    {   "upper",	1,	1,	FUpper	},
    {   "value",	1,	2,	FValue	},
    {   "version",      0,      0,      FVersion },
    {   "wkday",	1,	1,	FWkday	},
    {   "wkdaynum",	1,	1,	FWkdaynum },
    {   "year",		1,	1,	FYear	}
};

/* Need a variable here - Func[] array not really visible to outside. */
int NumFuncs = sizeof(Func) / sizeof(Operator) ;

/***************************************************************/
/*                                                             */
/*  CallFunc                                                   */
/*                                                             */
/*  Call a function given a pointer to it, and the number      */
/*  of arguments supplied.                                     */
/*                                                             */
/***************************************************************/
int CallFunc(Operator *f, int nargs)
{
    register int r = CheckArgs(f, nargs);
    int i;

    Nargs = nargs;

    RetVal.type = ERR_TYPE;
    if (DebugFlag & DB_PRTEXPR) {
	fprintf(ErrFp, "%s(", f->name);
	for (i=0; i<nargs; i++) {
	    PrintValue(&ARG(i), ErrFp);
	    if (i<nargs-1) fprintf(ErrFp, ", ");
	}
	fprintf(ErrFp, ") => ");
	if (r) {
	    fprintf(ErrFp, "%s\n", ErrMsg[r]);
	    return r;
	}
    }
    if (r) {
	Eprint("%s(): %s", f->name, ErrMsg[r]);
	return r;
    }

    r = (*(f->func))();
    if (r) {
	DestroyValue(RetVal);
	if (DebugFlag & DB_PRTEXPR)
	    fprintf(ErrFp, "%s\n", ErrMsg[r]);
	else
	    Eprint("%s(): %s", f->name, ErrMsg[r]);
	return r;
    }
    if (DebugFlag & DB_PRTEXPR) {
	PrintValue(&RetVal, ErrFp);
	fprintf(ErrFp, "\n");
    }
    r = CleanUpAfterFunc();
    return r;
}

/***************************************************************/
/*                                                             */
/*  CheckArgs                                                  */
/*                                                             */
/*  Check that the right number of args have been supplied     */
/*  for a function.                                            */
/*                                                             */
/***************************************************************/
static int CheckArgs(Operator *f, int nargs)
{
    if (nargs < f->MINARGS) return E_2FEW_ARGS;
    if (nargs > f->MAXARGS && f->MAXARGS != NO_MAX) return E_2MANY_ARGS;
    return OK;
}
/***************************************************************/
/*                                                             */
/*  CleanUpAfterFunc                                           */
/*                                                             */
/*  Clean up the stack after a function call - remove          */
/*  args and push the new value.                               */
/*                                                             */
/***************************************************************/
static int CleanUpAfterFunc(void)
{
    Value v;
    int i;

    for (i=0; i<Nargs; i++) {
	PopValStack(v);
	DestroyValue(v);
    }
    PushValStack(RetVal);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  RetStrVal                                                  */
/*                                                             */
/*  Return a string value from a function.                     */
/*                                                             */
/***************************************************************/
static int RetStrVal(char const *s)
{
    RetVal.type = STR_TYPE;
    if (!s) {
	RetVal.v.str = malloc(1);
	if (RetVal.v.str) *RetVal.v.str = 0;
    } else
	RetVal.v.str = StrDup(s);

    if (!RetVal.v.str) {
	RetVal.type = ERR_TYPE;
	return E_NO_MEM;
    }
    return OK;
}


/***************************************************************/
/*                                                             */
/*  FStrlen - string length                                    */
/*                                                             */
/***************************************************************/
static int FStrlen(void)
{
    Value *v = &ARG(0);
    if (v->type != STR_TYPE) return E_BAD_TYPE;
    RetVal.type = INT_TYPE;
    RetVal.v.val = strlen(v->v.str);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FBaseyr - system base year                                 */
/*                                                             */
/***************************************************************/
static int FBaseyr(void)
{
    RetVal.type = INT_TYPE;
    RetVal.v.val = BASE;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FDate - make a date from year, month, day.                 */
/*                                                             */
/***************************************************************/
static int FDate(void)
{
    int y, m, d;
    if (ARG(0).type != INT_TYPE ||
	ARG(1).type != INT_TYPE ||
	ARG(2).type != INT_TYPE) return E_BAD_TYPE;
    y = ARG(0).v.val;
    m = ARG(1).v.val - 1;
    d = ARG(2).v.val;

    if (!DateOK(y, m, d)) return E_BAD_DATE;

    RetVal.type = DATE_TYPE;
    RetVal.v.val = Julian(y, m, d);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FDateTime - make a datetime from one of these combos:      */
/*  DATE, TIME                                                 */
/*  DATE, HOUR, MINUTE                                         */
/*  YEAR, MONTH, DAY, TIME                                     */
/*  YEAR, MONTH, DAY, HOUR, MINUTE                             */
/*                                                             */
/***************************************************************/
static int FDateTime(void)
{
    int y, m, d;

    RetVal.type = DATETIME_TYPE;

    switch(Nargs) {
    case 2:
	if (ARG(0).type != DATE_TYPE ||
	    ARG(1).type != TIME_TYPE) return E_BAD_TYPE;
	RetVal.v.val = (MINUTES_PER_DAY * ARG(0).v.val) + ARG(1).v.val;
	return OK;
    case 3:
	if (ARG(0).type != DATE_TYPE ||
	    ARG(1).type != INT_TYPE ||
	    ARG(2).type != INT_TYPE) return E_BAD_TYPE;
	if (ARG(1).v.val < 0 || ARG(2).v.val < 0) return E_2LOW;
	if (ARG(1).v.val > 23 || ARG(2).v.val > 59) return E_2HIGH;
	RetVal.v.val = (MINUTES_PER_DAY * ARG(0).v.val) + 60 * ARG(1).v.val + ARG(2).v.val;
	return OK;
    case 4:
	if (ARG(0).type != INT_TYPE ||
	    ARG(1).type != INT_TYPE ||
	    ARG(2).type != INT_TYPE ||
	    ARG(3).type != TIME_TYPE) return E_BAD_TYPE;
	y = ARG(0).v.val;
	m = ARG(1).v.val - 1;
	d = ARG(2).v.val;

	if (!DateOK(y, m, d)) return E_BAD_DATE;
	RetVal.v.val = Julian(y, m, d) * MINUTES_PER_DAY + ARG(3).v.val;
	return OK;
    case 5:
	if (ARG(0).type != INT_TYPE ||
	    ARG(1).type != INT_TYPE ||
	    ARG(2).type != INT_TYPE ||
	    ARG(3).type != INT_TYPE ||
	    ARG(4).type != INT_TYPE) return E_BAD_TYPE;

	y = ARG(0).v.val;
	m = ARG(1).v.val - 1;
	d = ARG(2).v.val;
	if (!DateOK(y, m, d)) return E_BAD_DATE;

	if (ARG(3).v.val < 0 || ARG(4).v.val < 0) return E_2LOW;
	if (ARG(3).v.val > 23 || ARG(4).v.val > 59) return E_2HIGH;
	RetVal.v.val = Julian(y, m, d) * MINUTES_PER_DAY + ARG(3).v.val * 60 + ARG(4).v.val;
	return OK;

    default:
	return E_2MANY_ARGS;
    }
}

/***************************************************************/
/*                                                             */
/*  FCoerce - type coercion function.                          */
/*                                                             */
/***************************************************************/
static int FCoerce(void)
{
    char const *s;

    if (ARG(0).type != STR_TYPE) return E_BAD_TYPE;
    s = ARG(0).v.str;

    /* Copy the value of ARG(1) into RetVal, and make ARG(1) invalid so
       it won't be destroyed */
    DCOPYVAL(RetVal, ARG(1));

    if (! StrCmpi(s, "int")) return DoCoerce(INT_TYPE, &RetVal);
    else if (! StrCmpi(s, "date")) return DoCoerce(DATE_TYPE, &RetVal);
    else if (! StrCmpi(s, "time")) return DoCoerce(TIME_TYPE, &RetVal);
    else if (! StrCmpi(s, "string")) return DoCoerce(STR_TYPE, &RetVal);
    else if (! StrCmpi(s, "datetime")) return DoCoerce(DATETIME_TYPE, &RetVal);
    else return E_CANT_COERCE;
}

/***************************************************************/
/*                                                             */
/*  FMax - select maximum from a list of args.                 */
/*                                                             */
/***************************************************************/
static int FMax(void)
{
    Value *maxptr;
    int i;
    char type;

    maxptr = &ARG(0);
    type = maxptr->type;

    for (i=1; i<Nargs; i++) {
	if (ARG(i).type != type) return E_BAD_TYPE;
	if (type != STR_TYPE) {
	    if (ARG(i).v.val > maxptr->v.val) maxptr = &ARG(i);
	} else {
	    if (strcmp(ARG(i).v.str, maxptr->v.str) > 0) maxptr = &ARG(i);
	}
    }
    DCOPYVAL(RetVal, *maxptr);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FMin - select minimum from a list of args.                 */
/*                                                             */
/***************************************************************/
static int FMin(void)
{
    Value *minptr;
    int i;
    char type;

    minptr = &ARG(0);
    type = minptr->type;

    for (i=1; i<Nargs; i++) {
	if (ARG(i).type != type) return E_BAD_TYPE;
	if (type != STR_TYPE) {
	    if (ARG(i).v.val < minptr->v.val) minptr = &ARG(i);
	} else {
	    if (strcmp(ARG(i).v.str, minptr->v.str) < 0) minptr = &ARG(i);
	}
    }
    DCOPYVAL(RetVal, *minptr);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FAsc - ASCII value of first char of string                 */
/*                                                             */
/***************************************************************/
static int FAsc(void)
{
    if (ARG(0).type != STR_TYPE) return E_BAD_TYPE;
    RetVal.type = INT_TYPE;
    RetVal.v.val = *(ARG(0).v.str);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FChar - build a string from ASCII values                   */
/*                                                             */
/***************************************************************/
static int FChar(void)
{

    int i, len;

/* Special case of one arg - if given ascii value 0, create empty string */
    if (Nargs == 1) {
	if (ARG(0).type != INT_TYPE) return E_BAD_TYPE;
	if (ARG(0).v.val < -128) return E_2LOW;
	if (ARG(0).v.val > 255) return E_2HIGH;
	len = ARG(0).v.val ? 2 : 1;
	RetVal.v.str = malloc(len);
	if (!RetVal.v.str) return E_NO_MEM;
	RetVal.type = STR_TYPE;
	*(RetVal.v.str) = ARG(0).v.val;
	if (len>1) *(RetVal.v.str + 1) = 0;
	return OK;
    }

    RetVal.v.str = malloc(Nargs + 1);
    if (!RetVal.v.str) return E_NO_MEM;
    RetVal.type = STR_TYPE;
    for (i=0; i<Nargs; i++) {
	if (ARG(i).type != INT_TYPE) {
	    free(RetVal.v.str);
	    RetVal.type = ERR_TYPE;
	    return E_BAD_TYPE;
	}
	if (ARG(i).v.val < -128 || ARG(i).v.val == 0) {
	    free(RetVal.v.str);
	    RetVal.type = ERR_TYPE;
	    return E_2LOW;
	}
	if (ARG(i).v.val > 255) {
	    free(RetVal.v.str);
	    RetVal.type = ERR_TYPE;
	    return E_2HIGH;
	}
	*(RetVal.v.str + i) = ARG(i).v.val;
    }
    *(RetVal.v.str + Nargs) = 0;
    return OK;
}
/***************************************************************/
/*                                                             */
/*  Functions for extracting the components of a date.         */
/*                                                             */
/*  FDay - get day of month                                    */
/*  FMonnum - get month (1-12)                                 */
/*  FYear - get year                                           */
/*  FWkdaynum - get weekday num (0 = Sun)                      */
/*  FWkday - get weekday (string)                              */
/*  FMon - get month (string)                                  */
/*                                                             */
/***************************************************************/
static int FDay(void)
{
    int y, m, d, v;
    if (!HASDATE(ARG(0))) return E_BAD_TYPE;
    v = DATEPART(ARG(0));

    if (v == CacheJul)
	d = CacheDay;
    else {
	FromJulian(v, &y, &m, &d);
	CacheJul = v;
	CacheYear = y;
	CacheMon = m;
	CacheDay = d;
    }
    RetVal.type = INT_TYPE;
    RetVal.v.val = d;
    return OK;
}

static int FMonnum(void)
{
    int y, m, d, v;
    if (!HASDATE(ARG(0))) return E_BAD_TYPE;
    v = DATEPART(ARG(0));

    if (v == CacheJul)
	m = CacheMon;
    else {
	FromJulian(v, &y, &m, &d);
	CacheJul = v;
	CacheYear = y;
	CacheMon = m;
	CacheDay = d;
    }
    RetVal.type = INT_TYPE;
    RetVal.v.val = m+1;
    return OK;
}

static int FYear(void)
{
    int y, m, d, v;
    if (!HASDATE(ARG(0))) return E_BAD_TYPE;
    v = DATEPART(ARG(0));

    if (v == CacheJul)
	y = CacheYear;
    else {
	FromJulian(v, &y, &m, &d);
	CacheJul = v;
	CacheYear = y;
	CacheMon = m;
	CacheDay = d;
    }
    RetVal.type = INT_TYPE;
    RetVal.v.val = y;
    return OK;
}

static int FWkdaynum(void)
{
    int v;
    if (!HASDATE(ARG(0))) return E_BAD_TYPE;
    v = DATEPART(ARG(0));

    RetVal.type = INT_TYPE;

    /* Correct so that 0 = Sunday */
    RetVal.v.val = (v+1) % 7;
    return OK;
}

static int FWkday(void)
{
    char const *s;

    if (!HASDATE(ARG(0)) && ARG(0).type != INT_TYPE) return E_BAD_TYPE;
    if (ARG(0).type == INT_TYPE) {
	if (ARG(0).v.val < 0) return E_2LOW;
	if (ARG(0).v.val > 6) return E_2HIGH;
	/* Convert 0=Sun to 0=Mon */
	ARG(0).v.val--;
	if (ARG(0).v.val < 0) ARG(0).v.val = 6;
	s = DayName[ARG(0).v.val];
    } else s = DayName[DATEPART(ARG(0)) % 7];
    return RetStrVal(s);
}

static int FMon(void)
{
    char const *s;
    int y, m, d, v;

    if (!HASDATE(ARG(0)) && ARG(0).type != INT_TYPE) return E_BAD_TYPE;

    if (ARG(0).type == INT_TYPE) {
	m = ARG(0).v.val - 1;
	if (m < 0) return E_2LOW;
	if (m > 11) return E_2HIGH;
    } else {
	v = DATEPART(ARG(0));
	if (v == CacheJul)
	    m = CacheMon;
	else {
	    FromJulian(v, &y, &m, &d);
	    CacheJul = v;
	    CacheYear = y;
	    CacheMon = m;
	    CacheDay = d;
	}
    }
    s = MonthName[m];
    return RetStrVal(s);
}

/***************************************************************/
/*                                                             */
/*  FHour - extract hour from a time                           */
/*  FMinute - extract minute from a time                       */
/*  FTime - create a time from hour and minute                 */
/*                                                             */
/***************************************************************/
static int FHour(void)
{
    int v;
    if (!HASTIME(ARG(0))) return E_BAD_TYPE;
    v = TIMEPART(ARG(0));
    RetVal.type = INT_TYPE;
    RetVal.v.val = v / 60;
    return OK;
}

static int FMinute(void)
{
    int v;
    if (!HASTIME(ARG(0))) return E_BAD_TYPE;
    v = TIMEPART(ARG(0));
    RetVal.type = INT_TYPE;
    RetVal.v.val = v % 60;
    return OK;
}

static int FTime(void)
{
    int h, m;

    if (ARG(0).type != INT_TYPE || ARG(1).type != INT_TYPE) return E_BAD_TYPE;

    h = ARG(0).v.val;
    m = ARG(1).v.val;
    if (h<0 || m<0) return E_2LOW;
    if (h>23 || m>59) return E_2HIGH;
    RetVal.type = TIME_TYPE;
    RetVal.v.val = h*60 + m;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FAbs - absolute value                                      */
/*  FSgn - signum function                                     */
/*                                                             */
/***************************************************************/
static int FAbs(void)
{
    int v;

    if (ARG(0).type != INT_TYPE) return E_BAD_TYPE;
    v = ARG(0).v.val;
    RetVal.type = INT_TYPE;
    RetVal.v.val = (v < 0) ? (-v) : v;
    return OK;
}

static int FSgn(void)
{
    int v;

    if (ARG(0).type != INT_TYPE) return E_BAD_TYPE;
    v = ARG(0).v.val;
    RetVal.type = INT_TYPE;
    if (v>0) RetVal.v.val = 1;
    else if (v<0) RetVal.v.val = -1;
    else RetVal.v.val = 0;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FOrd - returns a string containing ordinal number.         */
/*                                                             */
/*  EG - ord(2) == "2nd", etc.                                 */
/*                                                             */
/***************************************************************/
static int FOrd(void)
{
    int t, u, v;
    char const *s;

    if (ARG(0).type != INT_TYPE) return E_BAD_TYPE;

    v = ARG(0).v.val;
    t = v % 100;
    if (t < 0) t = -t;
    u = t % 10;
    s = "th";
    if (u == 1 && t != 11) s = "st";
    if (u == 2 && t != 12) s = "nd";
    if (u == 3 && t != 13) s = "rd";
    sprintf(Buffer, "%d%s", v, s);
    return RetStrVal(Buffer);
}

/***************************************************************/
/*                                                             */
/*  FPlural - pluralization function                           */
/*                                                             */
/*  plural(n) -->  "" or "s"                                   */
/*  plural(n, str) --> "str" or "strs"                         */
/*  plural(n, str1, str2) --> "str1" or "str2"                 */
/*                                                             */
/***************************************************************/
static int FPlural(void)
{
    if (ARG(0).type != INT_TYPE) return E_BAD_TYPE;

    switch(Nargs) {
    case 1:
	if (ARG(0).v.val == 1) return RetStrVal("");
	else return RetStrVal("s");

    case 2:
	if (ARG(1).type != STR_TYPE) return E_BAD_TYPE;
	if (ARG(0).v.val == 1) {
	    DCOPYVAL(RetVal, ARG(1));
	    return OK;
	}
	RetVal.type = STR_TYPE;
	RetVal.v.str = malloc(strlen(ARG(1).v.str)+2);
	if (!RetVal.v.str) {
	    RetVal.type = ERR_TYPE;
	    return E_NO_MEM;
	}
	strcpy(RetVal.v.str, ARG(1).v.str);
	strcat(RetVal.v.str, "s");
	return OK;

    default:
	if (ARG(1).type != STR_TYPE || ARG(2).type != STR_TYPE)
	    return E_BAD_TYPE;
	if (ARG(0).v.val == 1) DCOPYVAL(RetVal, ARG(1));
	else DCOPYVAL(RetVal, ARG(2));
	return OK;
    }
}

/***************************************************************/
/*                                                             */
/*  FChoose                                                    */
/*  Choose the nth value from a list of value.  If n<1, choose */
/*  first.  If n>N, choose Nth value.  Indexes always start    */
/*  from 1.                                                    */
/*                                                             */
/***************************************************************/
static int FChoose(void)
{
    int v;

    if (ARG(0).type != INT_TYPE) return E_BAD_TYPE;
    v = ARG(0).v.val;
    if (v < 1) v = 1;
    if (v > Nargs-1) v = Nargs-1;
    DCOPYVAL(RetVal, ARG(v));
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FVersion - version of Remind                               */
/*                                                             */
/***************************************************************/
static int FVersion(void)
{
    return RetStrVal(VERSION);
}

/***************************************************************/
/*                                                             */
/*  FOstype - the type of operating system                     */
/*  (UNIX, OS/2, or MSDOS)                                     */
/*                                                             */
/***************************************************************/
static int FOstype(void)
{
    return RetStrVal("UNIX");
}

/***************************************************************/
/*                                                             */
/*  FUpper - convert string to upper-case                      */
/*  FLower - convert string to lower-case                      */
/*                                                             */
/***************************************************************/
static int FUpper(void)
{
    char *s;

    if (ARG(0).type != STR_TYPE) return E_BAD_TYPE;
    DCOPYVAL(RetVal, ARG(0));
    s = RetVal.v.str;
    while (*s) {
	*s = UPPER(*s);
	s++;
    }
    return OK;
}

static int FLower(void)
{
    char *s;

    if (ARG(0).type != STR_TYPE) return E_BAD_TYPE;
    DCOPYVAL(RetVal, ARG(0));
    s = RetVal.v.str;
    while (*s) {
	*s = LOWER(*s);
	s++;
    }
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FToday - return the system's notion of "today"             */
/*  Frealtoday - return today's date as read from OS.          */
/*  FNow - return the system time (or time on cmd line.)       */
/*  FRealnow - return the true system time                     */
/*                                                             */
/***************************************************************/
static int FToday(void)
{
    RetVal.type = DATE_TYPE;
    RetVal.v.val = JulianToday;
    return OK;
}

static int FRealtoday(void)
{
    RetVal.type = DATE_TYPE;
    RetVal.v.val = RealToday;
    return OK;
}

static int FNow(void)
{
    RetVal.type = TIME_TYPE;
    RetVal.v.val = (int) ( SystemTime(0) / 60L );
    return OK;
}

static int FRealnow(void)
{
    RetVal.type = TIME_TYPE;
    RetVal.v.val = (int) ( SystemTime(1) / 60L );
    return OK;
}

static int FCurrent(void)
{
    RetVal.type = DATETIME_TYPE;
    RetVal.v.val = JulianToday * MINUTES_PER_DAY + (SystemTime(0) / 60);
    return OK;
}

static int FRealCurrent(void)
{
    RetVal.type = DATETIME_TYPE;
    RetVal.v.val = RealToday * MINUTES_PER_DAY + (SystemTime(1) / 60);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FGetenv - get the value of an environment variable.        */
/*                                                             */
/***************************************************************/
static int FGetenv(void)
{
    if (ARG(0).type != STR_TYPE) return E_BAD_TYPE;
    return RetStrVal(getenv(ARG(0).v.str));
}

/***************************************************************/
/*                                                             */
/*  FValue                                                     */
/*                                                             */
/*  Get the value of a variable.  If a second arg is supplied, */
/*  it is returned if variable is undefined.                   */
/*                                                             */
/***************************************************************/
static int FValue(void)
{
    Var *v;

    if (ARG(0).type != STR_TYPE) return E_BAD_TYPE;
    switch(Nargs) {
    case 1:
	return GetVarValue(ARG(0).v.str, &RetVal, NULL);

    case 2:
	v = FindVar(ARG(0).v.str, 0);
	if (!v) {
	    DCOPYVAL(RetVal, ARG(1));
	    return OK;
	} else {
	    return CopyValue(&RetVal, &v->v);
	}
    }
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FDefined                                                   */
/*                                                             */
/*  Return 1 if a variable is defined, 0 if it is not.         */
/*                                                             */
/***************************************************************/
static int FDefined(void)
{
    if (ARG(0).type != STR_TYPE) return E_BAD_TYPE;

    RetVal.type = INT_TYPE;

    if (FindVar(ARG(0).v.str, 0))
	RetVal.v.val = 1;
    else
	RetVal.v.val = 0;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FTrigdate and FTrigtime                                    */
/*                                                             */
/*  Date and time of last trigger.  These are stored in global */
/*  vars.                                                      */
/*                                                             */
/***************************************************************/
static int FTrigdate(void)
{
    if (LastTrigValid) {
	RetVal.type = DATE_TYPE;
	RetVal.v.val = LastTriggerDate;
    } else {
	RetVal.type = INT_TYPE;
	RetVal.v.val = 0;
    }
    return OK;
}

static int FTrigvalid(void)
{
    RetVal.type = INT_TYPE;
    RetVal.v.val = LastTrigValid;
    return OK;
}

static int FTrigtime(void)
{
    if (LastTriggerTime != NO_TIME) {
	RetVal.type = TIME_TYPE;
	RetVal.v.val = LastTriggerTime;
    } else {
	RetVal.type = INT_TYPE;
	RetVal.v.val = 0;
    }
    return OK;
}

static int FTrigdatetime(void)
{
    if (!LastTrigValid) {
	RetVal.type = INT_TYPE;
	RetVal.v.val = 0;
    } else if (LastTriggerTime != NO_TIME) {
	RetVal.type = DATETIME_TYPE;
	RetVal.v.val = LastTriggerDate * MINUTES_PER_DAY + LastTriggerTime;
    } else {
	RetVal.type = DATE_TYPE;
	RetVal.v.val = LastTriggerDate;
    }
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FDaysinmon                                                 */
/*                                                             */
/*  Returns the number of days in mon,yr                       */
/*                                                             */
/***************************************************************/
static int FDaysinmon(void)
{
    if (ARG(0).type != INT_TYPE || ARG(1).type != INT_TYPE) return E_BAD_TYPE;

    if (ARG(0).v.val > 12 || ARG(0).v.val < 1 ||
	ARG(1).v.val < BASE || ARG(1).v.val > BASE+YR_RANGE)
	return E_DOMAIN_ERR;

    RetVal.type = INT_TYPE;
    RetVal.v.val = DaysInMonth(ARG(0).v.val-1, ARG(1).v.val);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FIsleap                                                    */
/*                                                             */
/*  Return 1 if year is a leap year, zero otherwise.           */
/*                                                             */
/***************************************************************/
static int FIsleap(void)
{
    int y, m, d;

    if (ARG(0).type != INT_TYPE && !HASDATE(ARG(0))) return E_BAD_TYPE;

    /* If it's a date, extract the year */
    if (HASDATE(ARG(0)))
	FromJulian(DATEPART(ARG(0)), &y, &m, &d);
    else
	y = ARG(0).v.val;

    RetVal.type = INT_TYPE;
    RetVal.v.val = IsLeapYear(y);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FTrigger                                                   */
/*                                                             */
/*  Put out a date in a format suitable for triggering.        */
/*                                                             */
/***************************************************************/
static int FTrigger(void)
{
    int y, m, d;
    int date, tim;
    char buf[128];

    tim = NO_TIME;
    if (ARG(0).type != DATE_TYPE &&
	ARG(0).type != DATETIME_TYPE) return E_BAD_TYPE;

    if (ARG(0).type == DATE_TYPE) {
	date = ARG(0).v.val;
    } else {
	date = ARG(0).v.val / MINUTES_PER_DAY;
	tim = ARG(0).v.val % MINUTES_PER_DAY;
    }

    if (ARG(0).type == DATE_TYPE) {
	if (Nargs > 2) {
	    /* Date Time UTCFlag */
	    if (ARG(0).type == DATETIME_TYPE) return E_BAD_TYPE;
	    if (ARG(2).type != INT_TYPE) return E_BAD_TYPE;
	    if (ARG(1).type != TIME_TYPE) return E_BAD_TYPE;
	    tim = ARG(1).v.val;
	    if (ARG(2).v.val) {
		UTCToLocal(date, tim, &date, &tim);
	    }
	} else if (Nargs > 1) {
	    /* Date Time */
	    if (ARG(1).type != TIME_TYPE) return E_BAD_TYPE;
	    tim = ARG(1).v.val;
	}
    } else {
	if (Nargs > 2) {
	    return E_2MANY_ARGS;
	} else if (Nargs > 1) {
	    /* DateTime UTCFlag */
	    if (ARG(1).type != INT_TYPE) return E_BAD_TYPE;
	    if (ARG(1).v.val) {
		UTCToLocal(date, tim, &date, &tim);
	    }
	}
    }

    FromJulian(date, &y, &m, &d);
    if (tim != NO_TIME) {
	sprintf(buf, "%d %s %d AT %02d:%02d", d, EnglishMonthName[m], y,
		tim/60, tim%60);
    } else {
	sprintf(buf, "%d %s %d", d, EnglishMonthName[m], y);
    }
    return RetStrVal(buf);
}

/***************************************************************/
/*                                                             */
/*  FShell                                                     */
/*                                                             */
/*  The shell function.                                        */
/*                                                             */
/*  If run is disabled, will not be executed.                  */
/*                                                             */
/***************************************************************/
static int FShell(void)
{
    DynamicBuffer buf;
    int ch, r;
    FILE *fp;

    /* For compatibility with previous versions of Remind, which
       used a static buffer for reading results from shell() command */
    int maxlen = 511;

    DBufInit(&buf);
    if (RunDisabled) return E_RUN_DISABLED;
    if (ARG(0).type != STR_TYPE) return E_BAD_TYPE;
    if (Nargs >= 2) {
	if (ARG(1).type != INT_TYPE) return E_BAD_TYPE;
	maxlen = ARG(1).v.val;
    }
    fp = popen(ARG(0).v.str, "r");
    if (!fp) return E_IO_ERR;
    while (1) {
	ch = getc(fp);
	if (ch == EOF) {
	    break;
	}
	if (isspace(ch)) ch = ' ';
	if (DBufPutc(&buf, (char) ch) != OK) {
	    pclose(fp);
	    DBufFree(&buf);
	    return E_NO_MEM;
	}
	if (maxlen > 0 && DBufLen(&buf) >= maxlen) {
	    break;
	}
    }

    /* Delete trailing newline (converted to space) */
    if (DBufLen(&buf) && DBufValue(&buf)[DBufLen(&buf)-1] == ' ') {
	DBufValue(&buf)[DBufLen(&buf)-1] = 0;
    }

    pclose(fp);
    r = RetStrVal(DBufValue(&buf));
    DBufFree(&buf);
    return r;
}

/***************************************************************/
/*                                                             */
/*  FIsomitted                                                 */
/*                                                             */
/*  Is a date omitted?                                         */
/*                                                             */
/***************************************************************/
static int FIsomitted(void)
{
    if (!HASDATE(ARG(0))) return E_BAD_TYPE;
    RetVal.type = INT_TYPE;
    RetVal.v.val = IsOmitted(DATEPART(ARG(0)), 0);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FSubstr                                                    */
/*                                                             */
/*  The substr function.  We destroy the value on the stack.   */
/*                                                             */
/***************************************************************/
static int FSubstr(void)
{
    char *s;
    char const *t;
    int start, end;

    if (ARG(0).type != STR_TYPE || ARG(1).type != INT_TYPE) return E_BAD_TYPE;
    if (Nargs == 3 && ARG(2).type != INT_TYPE) return E_BAD_TYPE;

    s = ARG(0).v.str;
    start = 1;
    while (start < ARG(1).v.val) {
	if (!*s) break;
	s++;
	start++;
    }
    if (Nargs == 2 || !*s) return RetStrVal(s);
    end = start;
    t = s;
    while (end <= ARG(2).v.val) {
	if (!*s) break;
	s++;
	end++;
    }
    *s = 0;
    return RetStrVal(t);
}

/***************************************************************/
/*                                                             */
/*  FIndex                                                     */
/*                                                             */
/*  The index of one string embedded in another.               */
/*                                                             */
/***************************************************************/
static int FIndex(void)
{
    char const *s;
    int start;

    if (ARG(0).type != STR_TYPE || ARG(1).type != STR_TYPE ||
	(Nargs == 3 && ARG(2).type != INT_TYPE)) return E_BAD_TYPE;

    s = ARG(0).v.str;

/* If 3 args, bump up the start */
    if (Nargs == 3) {
	start = 1;
	while (start < ARG(2).v.val) {
	    if (!*s) break;
	    s++;
	    start++;
	}
    }

/* Find the string */
    s = strstr(s, ARG(1).v.str);
    RetVal.type = INT_TYPE;
    if (!s) {
	RetVal.v.val = 0;
	return OK;
    }
    RetVal.v.val = (s - ARG(0).v.str) + 1;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FIif                                                       */
/*                                                             */
/*  The IIF function.                                          */
/*                                                             */
/***************************************************************/
static int FIif(void)
{
    int istrue;
    int arg;

    if (!(Nargs % 2)) return E_IIF_ODD;

    for (arg=0; arg<Nargs-1; arg += 2) {
	if (ARG(arg).type != STR_TYPE && ARG(arg).type != INT_TYPE)
	    return E_BAD_TYPE;

	if (ARG(arg).type == INT_TYPE)
	    istrue = ARG(arg).v.val;
	else
	    istrue = *(ARG(arg).v.str);

	if (istrue) {
	    DCOPYVAL(RetVal, ARG(arg+1));
	    return OK;
	}
    }

    DCOPYVAL(RetVal, ARG(Nargs-1));
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FFilename                                                  */
/*                                                             */
/*  Return name of current file                                */
/*                                                             */
/***************************************************************/
static int FFilename(void)
{
    return RetStrVal(FileName);
}

/***************************************************************/
/*                                                             */
/*  FFiledir                                                   */
/*                                                             */
/*  Return directory of current file                           */
/*                                                             */
/***************************************************************/
static int FFiledir(void)
{
    char *s;
    DynamicBuffer buf;
    int r;

    DBufInit(&buf);

    if (DBufPuts(&buf, FileName) != OK) return E_NO_MEM;
    if (DBufLen(&buf) == 0) {
	DBufFree(&buf);
	return RetStrVal(".");
    }

    s = DBufValue(&buf) + DBufLen(&buf) - 1;
    while (s > DBufValue(&buf) && *s != '/') s--;
    if (*s == '/') {
	*s = 0;
	r = RetStrVal(DBufValue(&buf));
    } else r = RetStrVal(".");
    DBufFree(&buf);
    return r;
}
/***************************************************************/
/*                                                             */
/*  FAccess                                                    */
/*                                                             */
/*  The UNIX access() system call.                             */
/*                                                             */
/***************************************************************/
static int FAccess(void)
{
    int amode;
    char const *s;

    if (ARG(0).type != STR_TYPE ||
	(ARG(1).type != INT_TYPE && ARG(1).type != STR_TYPE)) return E_BAD_TYPE;

    if (ARG(1).type == INT_TYPE) amode = ARG(1).v.val;
    else {
	amode = 0;
	s = ARG(1).v.str;
	while (*s) {
	    switch(*s++) {
	    case 'r':
	    case 'R': amode |= R_OK; break;
	    case 'w':
	    case 'W': amode |= W_OK; break;
	    case 'x':
	    case 'X': amode |= X_OK; break;
	    }
	}
    }
    RetVal.type = INT_TYPE;
    RetVal.v.val = access(ARG(0).v.str, amode);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FTypeof                                                    */
/*                                                             */
/*  Implement the typeof() function.                           */
/*                                                             */
/***************************************************************/
static int FTypeof(void)
{
    switch(ARG(0).type) {
    case INT_TYPE:  return RetStrVal("INT");
    case DATE_TYPE: return RetStrVal("DATE");
    case TIME_TYPE:  return RetStrVal("TIME");
    case STR_TYPE:  return RetStrVal("STRING");
    case DATETIME_TYPE: return RetStrVal("DATETIME");
    default:        return RetStrVal("ERR");
    }
}

/***************************************************************/
/*                                                             */
/*  FLanguage                                                  */
/*                                                             */
/*  Implement the language() function.                         */
/*                                                             */
/***************************************************************/
static int FLanguage(void)
{
    return RetStrVal(L_LANGNAME);
}

/***************************************************************/
/*                                                             */
/*  FArgs                                                      */
/*                                                             */
/*  Implement the args() function.                             */
/*                                                             */
/***************************************************************/
static int FArgs(void)
{
    if (ARG(0).type != STR_TYPE) return E_BAD_TYPE;
    RetVal.type = INT_TYPE;
    RetVal.v.val = UserFuncExists(ARG(0).v.str);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FDosubst                                                   */
/*                                                             */
/*  Implement the dosubst() function.                          */
/*                                                             */
/***************************************************************/
static int FDosubst(void)
{
    int jul, tim, r;
    DynamicBuffer buf;

    DBufInit(&buf);

    jul = NO_DATE;
    tim = NO_TIME;
    if (ARG(0).type != STR_TYPE) return E_BAD_TYPE;
    if (Nargs >= 2) {
	if (ARG(1).type == DATETIME_TYPE) {
	    jul = DATEPART(ARG(1));
	    tim = TIMEPART(ARG(1));
	} else {
	    if (ARG(1).type != DATE_TYPE) return E_BAD_TYPE;
	    jul = ARG(1).v.val;
	}
	if (Nargs >= 3) {
	    if (ARG(1).type == DATETIME_TYPE) {
		return E_2MANY_ARGS;
	    }
	    if (ARG(2).type != TIME_TYPE) return E_BAD_TYPE;
	    tim = ARG(2).v.val;
	}
    }

    if ((r=DoSubstFromString(ARG(0).v.str, &buf, jul, tim))) return r;
    r = RetStrVal(DBufValue(&buf));
    DBufFree(&buf);
    return r;
}

/***************************************************************/
/*                                                             */
/*  FHebdate                                                   */
/*  FHebday						       */
/*  FHebmon						       */
/*  FHebyear                                                   */
/*                                                             */
/*  Hebrew calendar support functions                          */
/*                                                             */
/***************************************************************/
static int FHebdate(void)
{
    int year, day, mon, jahr;
    int mout, dout;
    int ans, r;
    int adarbehave;

    if (ARG(0).type != INT_TYPE || ARG(1).type != STR_TYPE) return E_BAD_TYPE;
    day = ARG(0).v.val;
    mon = HebNameToNum(ARG(1).v.str);
    if (mon < 0) return E_BAD_HEBDATE;
    if (Nargs == 2) {
	r = GetNextHebrewDate(JulianToday, mon, day, 0, 0, &ans);
	if (r) return r;
	RetVal.type = DATE_TYPE;
	RetVal.v.val = ans;
	return OK;
    }
    if (Nargs == 5) {
	if (ARG(4).type != INT_TYPE) return E_BAD_TYPE;
	adarbehave = ARG(4).v.val;
	if (adarbehave < 0) return E_2LOW;
	if (adarbehave > 2) return E_2HIGH;
    } else adarbehave = 0;

    if (Nargs >= 4) {
	if (ARG(3).type != INT_TYPE) return E_BAD_TYPE;
	jahr = ARG(3).v.val;
	if (jahr < 0) return E_2LOW;
	if (jahr > 2) {
	    r = ComputeJahr(jahr, mon, day, &jahr);
	    if (r) return r;
	}
    } else jahr = 0;


    if (ARG(2).type == INT_TYPE) {
	year = ARG(2).v.val;
	r = GetValidHebDate(year, mon, day, 0, &mout, &dout, jahr);
	if (r) return r;
	r = HebToJul(year, mout, dout);
	if (r<0) return E_DATE_OVER;
	RetVal.v.val = r;
	RetVal.type = DATE_TYPE;
	return OK;
    } else if (HASDATE(ARG(2))) {
	r = GetNextHebrewDate(DATEPART(ARG(2)), mon, day, jahr, adarbehave, &ans);
	if (r) return r;
	RetVal.v.val = ans;
	RetVal.type = DATE_TYPE;
	return OK;
    } else return E_BAD_TYPE;
}

static int FHebday(void)
{
    int y, m, d, v;

    if (!HASDATE(ARG(0))) return E_BAD_TYPE;
    v = DATEPART(ARG(0));
    if (v == CacheHebJul)
	d = CacheHebDay;
    else {
	JulToHeb(v, &y, &m, &d);
	CacheHebJul = v;
	CacheHebYear = y;
	CacheHebMon = m;
	CacheHebDay = d;
    }
    RetVal.type = INT_TYPE;
    RetVal.v.val = d;
    return OK;
}

static int FHebmon(void)
{
    int y, m, d, v;

    if (!HASDATE(ARG(0))) return E_BAD_TYPE;
    v = DATEPART(ARG(0));

    if (v == CacheHebJul) {
	m = CacheHebMon;
	y = CacheHebYear;
    } else {
	JulToHeb(v, &y, &m, &d);
	CacheHebJul = v;
	CacheHebYear = y;
	CacheHebMon = m;
	CacheHebDay = d;
    }
    return RetStrVal(HebMonthName(m, y));
}

static int FHebyear(void)
{
    int y, m, d, v;

    if (!HASDATE(ARG(0))) return E_BAD_TYPE;
    v = DATEPART(ARG(0));

    if (v == CacheHebJul)
	y = CacheHebYear;
    else {
	JulToHeb(v, &y, &m, &d);
	CacheHebJul = v;
	CacheHebYear = y;
	CacheHebMon = m;
	CacheHebDay = d;
    }
    RetVal.type = INT_TYPE;
    RetVal.v.val = y;
    return OK;
}
/****************************************************************/
/*                                                              */
/*  FEasterdate - calc. easter Sunday from a year.              */
/*                                                              */
/*    from The Art of Computer Programming Vol 1.               */
/*            Fundamental Algorithms                            */
/*    by Donald Knuth.                                          */
/*                                                              */
/* Donated by Michael Salmon - thanks!                          */
/*                                                              */
/* I haven't examined this in detail, but I *think* int         */
/* arithmetic is fine, even on 16-bit machines.                 */
/*                                                              */
/****************************************************************/
static int FEasterdate(void)
{
    int y, m, d;
    int g, c, x, z, e, n;
    if (ARG(0).type == INT_TYPE) {
	y = ARG(0).v.val;
	if (y < BASE) return E_2LOW;
	else if (y > BASE+YR_RANGE) return E_2HIGH;
    } else if (HASDATE(ARG(0))) {
	FromJulian(DATEPART(ARG(0)), &y, &m, &d);  /* We just want the year */
    } else return E_BAD_TYPE;

    do {
	g = (y % 19) + 1;  /* golden number */
	c = (y / 100) + 1; /* century */
	x = (3 * c)/4 - 12;        /* correction for non-leap year centuries */
	z = (8 * c + 5)/25 - 5;    /* special constant for moon sync */
	d = (5 * y)/4 - x - 10;    /* find sunday */
	e = (11 * g + 20 + z - x) % 30;    /* calc epact */
	if ( e < 0 ) e += 30;
	if ( e == 24 || (e == 25 && g > 11)) e++;
	n = 44 - e;                        /* find full moon */
	if ( n < 21 ) n += 30;     /* after 21st */
	d = n + 7 - (d + n)%7;     /* calc sunday after */
	if (d <= 31) m = 2;
	else
	{
	    d = d - 31;
	    m = 3;
	}

	RetVal.type = DATE_TYPE;
	RetVal.v.val = Julian(y, m, d);
	y++; } while (HASDATE(ARG(0)) && RetVal.v.val < DATEPART(ARG(0)));

    return OK;
}
/***************************************************************/
/*                                                             */
/*  FIsdst and FMinsfromutc                                    */
/*                                                             */
/*  Check whether daylight savings time is in effect, and      */
/*  get minutes from UTC.                                      */
/*                                                             */
/***************************************************************/
static int FTimeStuff (int wantmins);
static int FIsdst(void)
{
    return FTimeStuff(0);
}

static int FMinsfromutc(void)
{
    return FTimeStuff(1);
}

static int FTimeStuff(int wantmins)
{
    int jul, tim;
    int mins, dst;

    jul = JulianToday;
    tim = 0;

    if (Nargs >= 1) {
	if (!HASDATE(ARG(0))) return E_BAD_TYPE;
	jul = DATEPART(ARG(0));
	if (HASTIME(ARG(0))) {
	    tim = TIMEPART(ARG(0));
	}
	if (Nargs >= 2) {
	    if (HASTIME(ARG(0))) return E_2MANY_ARGS;
	    if (ARG(1).type != TIME_TYPE) return E_BAD_TYPE;
	    tim = ARG(1).v.val;
	}
    }

    if (CalcMinsFromUTC(jul, tim, &mins, &dst)) return E_MKTIME_PROBLEM;
    RetVal.type = INT_TYPE;
    if (wantmins) RetVal.v.val = mins; else RetVal.v.val = dst;

    return OK;
}

/***************************************************************/
/*                                                             */
/*  Sunrise and sunset functions.                              */
/*                                                             */
/*  Algorithm from "Almanac for computers for the year 1978"   */
/*  by L. E. Doggett, Nautical Almanac Office, USNO.           */
/*                                                             */
/*  This code also uses some ideas found in programs written   */
/*  by Michael Schwartz and Marc T. Kaufman.                   */
/*                                                             */
/***************************************************************/
#ifdef PI
#undef PI
#endif
#define PI 3.14159265358979323846
#define DEGRAD (PI/180.0)
#define RADDEG (180.0/PI)

static int SunStuff(int rise, double cosz, int jul)
{
    int year, mon, day;
    int jan0;
    int mins, hours;
    int dusk_or_dawn;

    double M, L, tanA, sinDelta, cosDelta, a, a_hr, cosH, t, H, T;
    double latitude, longdeg, UT, local;

/* Get offset from UTC */
    if (CalculateUTC) {
	if (CalcMinsFromUTC(jul, 12*60, &mins, NULL)) {
	    Eprint(ErrMsg[E_MKTIME_PROBLEM]);
	    return NO_TIME;
	}
    } else mins = MinsFromUTC;

/* Get latitude and longitude */
    longdeg = (double) LongDeg + (double) LongMin / 60.0
	+ (double) LongSec / 3600.0;

    latitude = DEGRAD * ((double) LatDeg + (double) LatMin / 60.0
		         + (double) LatSec / 3600.0);


    FromJulian(jul, &year, &mon, &day);
    jan0 = jul - Julian(year, 0, 1);

    dusk_or_dawn = rise;
    if (rise > 1)
	rise -= 2;
/* Following formula on page B6 exactly... */
    t = (double) jan0;
    if (rise) t += (6.0 + longdeg/15.0) / 24.0;
    else      t += (18.0 + longdeg/15.0) / 24.0;

/* Mean anomaly of sun for 1978 ... how accurate for other years??? */
    M = 0.985600 * t - 3.251;  /* In degrees */

/* Sun's true longitude */
    L = M + 1.916*sin(DEGRAD*M) + 0.02*sin(2*DEGRAD*M) + 282.565;
    if (dusk_or_dawn == 2) {/* dusk */
	L += 6;
    } else if (dusk_or_dawn == 3) {/* dawn */
	L -= 14;
    }
    if (L > 360.0) L -= 360.0;

/* Tan of sun's right ascension */
    tanA = 0.91746 * tan(DEGRAD*L);
    a = RADDEG * atan(tanA);

/* Move a into same quadrant as L */
    if (0.0 <= L && L < 90.0) {
	if (a < 0.0) a += 180.0;
    } else if (90.0 <= L && L < 180.0) {
	a += 180.0;
    } else if (180.0 <= L && L < 270.0) {
	a += 180.0;
    } else {
	if (a > 0.0) a += 180.0;
    }
/*   if (fabs(a - L) > 90.0)
     a += 180.0; */
    
    if (a > 360.0)
	a -= 360.0;
    a_hr = a / 15.0;

/* Sine of sun's declination */
    sinDelta = 0.39782 * sin(DEGRAD*L);
    cosDelta = sqrt(1 - sinDelta*sinDelta);
    
/* Cosine of sun's local hour angle */
    cosH = (cosz - sinDelta * sin(latitude)) / (cosDelta * cos(latitude));
    
    if (cosH < -1.0) { /* Summer -- permanent daylight */
	if (rise) return NO_TIME;
	else      return -NO_TIME;
    }
    if (cosH > 1.0) { /* Winter -- permanent darkness */
	if (rise) return -NO_TIME;
	else      return NO_TIME;
    }

    H = RADDEG * acos(cosH);
    if (rise) H = 360.0 - H;

    T = H / 15.0 + a_hr - 0.065710*t - 6.620;
    if (T >= 24.0) T -= 24.0;
    else if (T < 0.0) T+= 24.0;
    
    UT = T + longdeg / 15.0;
    

    local = UT + (double) mins / 60.0;
    if (local < 0.0) local += 24.0;
    else if (local >= 24.0) local -= 24.0;

    hours = (int) local;
    mins = (int) ((local - hours) * 60.0);
    
    /* Sometimes, we get roundoff error.  Check for "reasonableness" of
       answer. */
    if (rise) {
	/* Sunrise so close to midnight it wrapped around -- permament light */
	if (hours >= 23) return NO_TIME;
    } else {
	/* Sunset so close to midnight it wrapped around -- permament light */
	if (hours <= 1) return -NO_TIME;
    }
    return hours*60 + mins;
}

/***************************************************************/
/*                                                             */
/*  Sunrise and Sunset functions.                              */
/*                                                             */
/***************************************************************/
static int FSun(int rise)
{
    int jul = JulianToday;
    static double cosz = -0.014543897;  /* for sunrise and sunset */
    int r;

    if (Nargs >= 1) {
	if (!HASDATE(ARG(0))) return E_BAD_TYPE;
	jul = DATEPART(ARG(0));
    }

    r = SunStuff(rise, cosz, jul);
    if (r == NO_TIME) {
	RetVal.v.val = 0;
	RetVal.type = INT_TYPE;
    } else if (r == -NO_TIME) {
	RetVal.v.val = MINUTES_PER_DAY;
	RetVal.type = INT_TYPE;
    } else {
	RetVal.v.val = r;
	RetVal.type = TIME_TYPE;
    }
    return OK;
}

static int FSunrise(void)
{
    return FSun(1);
}
static int FSunset(void)
{
    return FSun(0);
}

static int FDawn(void)
{
    return FSun(3);
}
static int FDusk(void)
{
    return FSun(2);
}

/***************************************************************/
/*                                                             */
/*  FFiledate                                                  */
/*                                                             */
/*  Return modification date of a file                         */
/*                                                             */
/***************************************************************/
static int FFiledate(void)
{
    struct stat statbuf;
    struct tm *t1;

    RetVal.type = DATE_TYPE;

    if (ARG(0).type != STR_TYPE) return E_BAD_TYPE;

    if (stat(ARG(0).v.str, &statbuf)) {
	RetVal.v.val = 0;
	return OK;
    }

    t1 = localtime(&(statbuf.st_mtime));

    if (t1->tm_year + 1900 < BASE)
	RetVal.v.val=0;
    else
	RetVal.v.val=Julian(t1->tm_year+1900, t1->tm_mon, t1->tm_mday);

    return OK;
}

/***************************************************************/
/*                                                             */
/*  FFiledatetime                                              */
/*                                                             */
/*  Return modification datetime of a file                     */
/*                                                             */
/***************************************************************/
static int FFiledatetime(void)
{
    struct stat statbuf;
    struct tm *t1;

    RetVal.type = DATETIME_TYPE;

    if (ARG(0).type != STR_TYPE) return E_BAD_TYPE;

    if (stat(ARG(0).v.str, &statbuf)) {
	RetVal.v.val = 0;
	return OK;
    }

    t1 = localtime(&(statbuf.st_mtime));

    if (t1->tm_year + 1900 < BASE)
	RetVal.v.val=0;
    else
	RetVal.v.val = MINUTES_PER_DAY * Julian(t1->tm_year+1900, t1->tm_mon, t1->tm_mday) + t1->tm_hour * 60 + t1->tm_min;

    return OK;
}

/***************************************************************/
/*                                                             */
/*  FPsshade                                                   */
/*                                                             */
/*  Canned PostScript code for shading a calendar square       */
/*                                                             */
/***************************************************************/
static int psshade_warned = 0;
static int FPsshade(void)
{
    char psbuff[256];
    char *s = psbuff;
    int i;

    /* 1 or 3 args */
    if (Nargs != 1 && Nargs != 3) return E_2MANY_ARGS;

    for (i=0; i<Nargs; i++) {
	if (ARG(i).type != INT_TYPE) return E_BAD_TYPE;
	if (ARG(i).v.val < 0) return E_2LOW;
	if (ARG(i).v.val > 100) return E_2HIGH;
    }

    if (!psshade_warned) {
	psshade_warned = 1;
	Eprint("psshade() is deprecated; use SPECIAL SHADE instead.");
    }

    sprintf(s, "/_A LineWidth 2 div def ");
    s += strlen(s);
    sprintf(s, "_A _A moveto ");
    s += strlen(s);
    sprintf(s, "BoxWidth _A sub _A lineto BoxWidth _A sub BoxHeight _A sub lineto ");
    s += strlen(s);
    if (Nargs == 1) {
	sprintf(s, "_A BoxHeight _A sub lineto closepath %d 100 div setgray fill 0.0 setgray", ARG(0).v.val);
    } else {
	sprintf(s, "_A BoxHeight _A sub lineto closepath %d 100 div %d 100 div %d 100 div setrgbcolor fill 0.0 setgray", ARG(0).v.val, ARG(1).v.val, ARG(2).v.val);
    }
    return RetStrVal(psbuff);
}

/***************************************************************/
/*                                                             */
/*  FPsmoon                                                    */
/*                                                             */
/*  Canned PostScript code for generating moon phases          */
/*                                                             */
/***************************************************************/
static int psmoon_warned = 0;

static int FPsmoon(void)
{
    char psbuff[512];
    char sizebuf[30];
    char fontsizebuf[30];
    char *s = psbuff;
    char const *extra = NULL;
    int size = -1;
    int fontsize = -1;

    if (ARG(0).type != INT_TYPE) return E_BAD_TYPE;
    if (ARG(0).v.val < 0) return E_2LOW;
    if (ARG(0).v.val > 3) return E_2HIGH;
    if (Nargs > 1) {
	if (ARG(1).type != INT_TYPE) return E_BAD_TYPE;
	if (ARG(1).v.val < -1) return E_2LOW;
	size = ARG(1).v.val;
	if (Nargs > 2) {
	    if (ARG(2).type != STR_TYPE) return E_BAD_TYPE;
	    extra = ARG(2).v.str;
	    if (Nargs > 3) {
		if (ARG(3).type != INT_TYPE) return E_BAD_TYPE;
		if (ARG(3).v.val <= 0) return E_2LOW;
		fontsize = ARG(3).v.val;
	    }
	}
    }
    if (!psmoon_warned) {
	psmoon_warned = 1;
	Eprint("psmoon() is deprecated; use SPECIAL MOON instead.");
    }
    if (size > 0) {
	sprintf(sizebuf, "%d", size);
    } else {
	strcpy(sizebuf, "DaySize 2 div");
    }

    if (fontsize > 0) {
	sprintf(fontsizebuf, "%d", fontsize);
    } else {
	strcpy(fontsizebuf, "EntrySize");
    }

    sprintf(s, "gsave 0 setgray newpath Border %s add BoxHeight Border sub %s sub",
	    sizebuf, sizebuf);
    s += strlen(s);
    sprintf(s, " %s 0 360 arc closepath", sizebuf);
    s += strlen(s);
    switch(ARG(0).v.val) {
    case 0:
	sprintf(s, " fill");
	s += strlen(s);
	break;

    case 2:
	sprintf(s, " stroke");
	s += strlen(s);
	break;

    case 1:
	sprintf(s, " stroke");
	s += strlen(s);
	sprintf(s, " newpath Border %s add BoxHeight Border sub %s sub",
		sizebuf, sizebuf);
	s += strlen(s);
	sprintf(s, " %s 90 270 arc closepath fill", sizebuf);
	s += strlen(s);
	break;

    default:
	sprintf(s, " stroke");
	s += strlen(s);
	sprintf(s, " newpath Border %s add BoxHeight Border sub %s sub",
		sizebuf, sizebuf);
	s += strlen(s);
	sprintf(s, " %s 270 90 arc closepath fill", sizebuf);
	s += strlen(s);
	break;
    }
    if (extra) {
	sprintf(s, " Border %s add %s add Border add BoxHeight border sub %s sub %s sub moveto /EntryFont findfont %s scalefont setfont (%s) show",
		sizebuf, sizebuf, sizebuf, sizebuf, fontsizebuf, extra);
	s += strlen(s);
    }

    sprintf(s, " grestore");
    return RetStrVal(psbuff);
}

/***************************************************************/
/*                                                             */
/*  FMoonphase                                                 */
/*                                                             */
/*  Phase of moon for specified date/time.                     */
/*                                                             */
/***************************************************************/
static int FMoonphase(void)
{
    int date, time;

    switch(Nargs) {
    case 0:
	date = JulianToday;
	time = 0;
	break;
    case 1:
	if (!HASDATE(ARG(0))) return E_BAD_TYPE;
	date = DATEPART(ARG(0));
	if (HASTIME(ARG(0))) {
	    time = TIMEPART(ARG(0));
	} else {
	    time = 0;
	}
	break;
    case 2:
	if (ARG(0).type == DATETIME_TYPE) return E_2MANY_ARGS;
	if (ARG(0).type != DATE_TYPE && ARG(1).type != TIME_TYPE) return E_BAD_TYPE;
	date = ARG(0).v.val;
	time = ARG(1).v.val;
	break;

    default: return E_SWERR;
    }

    RetVal.type = INT_TYPE;
    RetVal.v.val = MoonPhase(date, time);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FMoondate                                                  */
/*                                                             */
/*  Hunt for next occurrence of specified moon phase           */
/*                                                             */
/***************************************************************/
static int MoonStuff (int want_time);
static int FMoondate(void)
{
    return MoonStuff(DATE_TYPE);
}

static int FMoontime(void)
{
    return MoonStuff(TIME_TYPE);
}

static int FMoondatetime(void)
{
    return MoonStuff(DATETIME_TYPE);
}

static int MoonStuff(int type_wanted)
{
    int startdate, starttim;
    int d, t;

    startdate = JulianToday;
    starttim = 0;

    if (ARG(0).type != INT_TYPE) return E_BAD_TYPE;
    if (ARG(0).v.val < 0) return E_2LOW;
    if (ARG(0).v.val > 3) return E_2HIGH;
    if (Nargs >= 2) {
	if (!HASDATE(ARG(1))) return E_BAD_TYPE;
	startdate = DATEPART(ARG(1));
	if (HASTIME(ARG(1))) {
		starttim = TIMEPART(ARG(1));
	}

	if (Nargs >= 3) {
	    if (HASTIME(ARG(1))) return E_2MANY_ARGS;
	    if (ARG(2).type != TIME_TYPE) return E_BAD_TYPE;
	    starttim = ARG(2).v.val;
	}
    }

    HuntPhase(startdate, starttim, ARG(0).v.val, &d, &t);
    RetVal.type = type_wanted;
    switch(type_wanted) {
    case TIME_TYPE:
	RetVal.v.val = t;
	break;
    case DATE_TYPE:
	RetVal.v.val = d;
	break;
    case DATETIME_TYPE:
	RetVal.v.val = d * MINUTES_PER_DAY + t;
	break;
    default:
	return E_BAD_TYPE;
    }
    return OK;
}

static int FTimepart(void)
{
    if (ARG(0).type != DATETIME_TYPE) return E_BAD_TYPE;
    RetVal.type = TIME_TYPE;
    RetVal.v.val = TIMEPART(ARG(0));
    return OK;
}

static int FDatepart(void)
{
    if (ARG(0).type != DATETIME_TYPE) return E_BAD_TYPE;
    RetVal.type = DATE_TYPE;
    RetVal.v.val = DATEPART(ARG(0));
    return OK;
}

#ifndef HAVE_SETENV
/* This is NOT a general-purpose replacement for setenv.  It's only
 * used for the timezone stuff! */
static int setenv(char const *varname, char const *val, int overwrite)
{
    static char tzbuf[256];
    if (strcmp(varname, "TZ")) {
	fprintf(stderr, "built-in setenv can only be used with TZ\n");
	abort();
    }
    if (!overwrite) {
	fprintf(stderr, "built-in setenv must have overwrite=1\n");
	abort();
    }

    if (strlen(val) > 250) {
	return -1;
    }
    sprintf(tzbuf, "%s=%s", varname, val);
    return(putenv(tzbuf));
}
#endif
#ifndef HAVE_UNSETENV
/* This is NOT a general-purpose replacement for unsetenv.  It's only
 * used for the timezone stuff! */
static void unsetenv(char const *varname)
{
    static char tzbuf[8];
    if (strcmp(varname, "TZ")) {
	fprintf(stderr, "built-in unsetenv can only be used with TZ\n");
	abort();
    }
    sprintf(tzbuf, "%s", varname);
    putenv(tzbuf);
}
#endif

/***************************************************************/
/*                                                             */
/*  FTz                                                        */
/*                                                             */
/*  Conversion between different timezones.                    */
/*                                                             */
/***************************************************************/
static int tz_set_tz(char const *tz)
{
    int r;
    if (tz == NULL) {
       unsetenv("TZ");
       r = 0;
    } else {
        r = setenv("TZ", tz, 1);
    }
    tzset();
    return r;
}

static int tz_convert(int year, int month, int day,
                      int hour, int minute,
                      char const *src_tz, char const *tgt_tz,
                      struct tm *tm)
{
    int r;
    time_t t;
    struct tm *res;
    char const *old_tz;

    /* init tm struct */
    tm->tm_sec = 0;
    tm->tm_min = minute;
    tm->tm_hour = hour;
    tm->tm_mday = day;
    tm->tm_mon = month;
    tm->tm_year = year - 1900;
    tm->tm_wday = 0; /* ignored by mktime */
    tm->tm_yday = 0; /* ignored by mktime */
    tm->tm_isdst = -1;  /* information not available */

    /* backup old TZ env var */
    old_tz = getenv("TZ");
    if (tgt_tz == NULL) {
        tgt_tz = old_tz;
    }

    /* set source TZ */
    r = tz_set_tz(src_tz);
    if (r == -1) {
        return -1;
    }

    /* create timestamp in UTC */
    t = mktime(tm);

    if (t == (time_t) -1) {
        tz_set_tz(old_tz);
        return -1;
    }

    /* set target TZ */
    r = tz_set_tz(tgt_tz);
    if (r == -1) {
        tz_set_tz(old_tz);
        return -1;
    }

    /* convert to target TZ */
    res = localtime_r(&t, tm);

    /* restore old TZ */
    tz_set_tz(old_tz);

    /* return result */
    if (res == NULL) {
        return -1;
    } else {
        return 1;
    }
}

static int FTzconvert(void)
{
    int year, month, day, hour, minute, r;
    int jul, tim;
    struct tm tm;

    if (ARG(0).type != DATETIME_TYPE ||
	ARG(1).type != STR_TYPE) return E_BAD_TYPE;
    if (Nargs == 3 && ARG(2).type != STR_TYPE) return E_BAD_TYPE;

    FromJulian(DATEPART(ARG(0)), &year, &month, &day);

    r = TIMEPART(ARG(0));
    hour = r / 60;
    minute = r % 60;

    if (Nargs == 2) {
	r = tz_convert(year, month, day, hour, minute,
		       ARG(1).v.str, NULL, &tm);
    } else {
	r = tz_convert(year, month, day, hour, minute,
		       ARG(1).v.str, ARG(2).v.str, &tm);
    }

    if (r == -1) return E_CANT_CONVERT_TZ;

    jul = Julian(tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
    tim = tm.tm_hour * 60 + tm.tm_min;
    RetVal.type = DATETIME_TYPE;
    RetVal.v.val = jul * MINUTES_PER_DAY + tim;
    return OK;
}

static int
FNonomitted(void)
{
    int d1, d2, ans, localomit, i;

    Token tok;

    if (!HASDATE(ARG(0)) ||
	!HASDATE(ARG(1))) {
    }
    d1 = DATEPART(ARG(0));
    d2 = DATEPART(ARG(1));
    if (d2 < d1) return E_2LOW;

    localomit = 0;
    for (i=2; i<Nargs; i++) {
	if (ARG(i).type != STR_TYPE) return E_BAD_TYPE;
	FindToken(ARG(i).v.str, &tok);
	if (tok.type != T_WkDay) return E_UNKNOWN_TOKEN;
	localomit |= (1 << tok.val);
    }

    ans = 0;
    while (d1 < d2) {
	if (!IsOmitted(d1++, localomit)) {
	    ans++;
	}
    }
    RetVal.type = INT_TYPE;
    RetVal.v.val = ans;
    return OK;
}
