/***************************************************************/
/*                                                             */
/*  FUNCS.C                                                    */
/*                                                             */
/*  This file contains the built-in functions used in          */
/*  expressions.                                               */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1996 by David F. Skoll                  */
/*                                                             */
/***************************************************************/

static char const RCSID[] = "$Id: funcs.c,v 1.3 1996-03-31 04:01:56 dfs Exp $";

#include "config.h"
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <string.h>
#include <ctype.h>
#include <math.h>
#ifdef UNIX
#ifdef HAVE_UNISTD
#include <unistd.h>
#else
#include <sys/file.h>
#endif
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#if defined(__MSDOS__) || defined(__OS2__)
#include <io.h>
#define R_OK 4
#define W_OK 2
#define X_OK 1
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
#include "version.h"

/* Function prototypes */
PRIVATE	int	FAbs		ARGS ((void));
PRIVATE	int	FAccess		ARGS ((void));
PRIVATE int     FArgs		ARGS ((void));
PRIVATE	int	FAsc		ARGS ((void));
PRIVATE	int	FBaseyr		ARGS ((void));
PRIVATE	int	FChar		ARGS ((void));
PRIVATE	int	FChoose		ARGS ((void));
PRIVATE	int	FCoerce		ARGS ((void));
PRIVATE	int	FDate		ARGS ((void));
PRIVATE	int	FDay		ARGS ((void));
PRIVATE	int	FDaysinmon	ARGS ((void));
PRIVATE	int	FDefined	ARGS ((void));
PRIVATE	int	FDosubst	ARGS ((void));
PRIVATE	int	FEasterdate	ARGS ((void));
PRIVATE int	FFiledate	ARGS ((void));
PRIVATE	int	FFiledir	ARGS ((void));
PRIVATE	int	FFilename	ARGS ((void));
PRIVATE	int	FGetenv		ARGS ((void));
PRIVATE int     FHebdate	ARGS ((void));
PRIVATE int     FHebday		ARGS ((void));
PRIVATE int     FHebmon		ARGS ((void));
PRIVATE int     FHebyear	ARGS ((void));
PRIVATE	int	FHour		ARGS ((void));
PRIVATE	int	FIif		ARGS ((void));
PRIVATE	int	FIndex		ARGS ((void));
PRIVATE	int	FIsdst		ARGS ((void));
PRIVATE	int	FIsomitted	ARGS ((void));
PRIVATE	int	FLanguage	ARGS ((void));
PRIVATE	int	FMax		ARGS ((void));
PRIVATE	int	FMin		ARGS ((void));
PRIVATE	int	FMinute		ARGS ((void));
PRIVATE	int	FMinsfromutc	ARGS ((void));
PRIVATE	int	FMoondate	ARGS ((void));
PRIVATE	int	FMoonphase	ARGS ((void));
PRIVATE	int	FMoontime	ARGS ((void));
PRIVATE	int	FMon		ARGS ((void));
PRIVATE	int	FMonnum		ARGS ((void));
PRIVATE	int	FOrd		ARGS ((void));
PRIVATE	int	FOstype 	ARGS ((void));
PRIVATE	int	FPlural		ARGS ((void));
PRIVATE	int	FSgn		ARGS ((void));
PRIVATE int	FPsmoon		ARGS ((void));
PRIVATE int	FPsshade	ARGS ((void));
PRIVATE	int	FShell		ARGS ((void));
PRIVATE	int	FStrlen		ARGS ((void));
PRIVATE	int	FSubstr		ARGS ((void));
PRIVATE	int	FSunrise	ARGS ((void));
PRIVATE	int	FSunset		ARGS ((void));
PRIVATE	int	FTime		ARGS ((void));
PRIVATE	int	FTrigdate	ARGS ((void));
PRIVATE	int	FTrigtime	ARGS ((void));
PRIVATE	int	FTrigvalid	ARGS ((void));
PRIVATE	int	FTypeof		ARGS ((void));
PRIVATE	int	FUpper		ARGS ((void));
PRIVATE	int	FValue		ARGS ((void));
PRIVATE	int	FVersion	ARGS ((void));
PRIVATE	int	FWkday		ARGS ((void));
PRIVATE	int	FWkdaynum	ARGS ((void));
PRIVATE	int	FYear		ARGS ((void));
PRIVATE int	FIsleap         ARGS ((void));
PRIVATE int	FLower          ARGS ((void));
PRIVATE int	FNow            ARGS ((void));
PRIVATE int	FRealnow            ARGS ((void));
PRIVATE int	FRealtoday      ARGS ((void));
PRIVATE int	FToday          ARGS ((void));
PRIVATE int	FTrigger        ARGS ((void));
PRIVATE int	CheckArgs       ARGS ((Operator *f, int nargs));
PRIVATE int	CleanUpAfterFunc ARGS ((void));
PRIVATE int	SunStuff	ARGS ((int rise, double cosz, int jul));

#if defined(__MSDOS__) || defined(__BORLANDC__) || defined(AMIGA)
PRIVATE FILE *os_popen  ARGS((char *cmd, char *mode));
PRIVATE int   os_pclose ARGS((FILE *fp));
#define POPEN os_popen
#define PCLOSE os_pclose

#if defined(_MSC_VER)
#define popen _popen
#define pclose _pclose
#endif

#elif defined(_MSC_VER)
#define POPEN _popen
#define PCLOSE _pclose

#else
#define POPEN popen
#define PCLOSE pclose
#endif

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
    {   "date",		3,	3,	FDate	},
    {   "day",		1,	1,	FDay	},
    {   "daysinmon",	2,	2,	FDaysinmon },
    {   "defined",	1,	1,	FDefined },
    {   "dosubst",	1,	3,	FDosubst },
    {   "easterdate",	1,	1,	FEasterdate },
    {	"filedate",	1,	1,	FFiledate },
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
    {	"moonphase",	0,	2,	FMoonphase },
    {	"moontime",	1,	3,	FMoontime },
    {   "now",		0,	0,	FNow	},
    {   "ord",		1,	1,	FOrd	},
    {   "ostype",       0,      0,      FOstype },
    {   "plural",	1,	3,	FPlural },
    {	"psmoon",	1,	4,	FPsmoon},
    {	"psshade",	1,	1,	FPsshade},
    {   "realnow",      0,      0,      FRealnow},
    {   "realtoday",    0,      0,      FRealtoday },
    {   "sgn",		1,	1,	FSgn	},
    {   "shell",	1,	1,	FShell	},
    {   "strlen",	1,	1,	FStrlen	},
    {   "substr",	2,	3,	FSubstr	},
    {   "sunrise",	0,	1,	FSunrise},
    {   "sunset",	0,	1,	FSunset },
    {   "time",		2,	2,	FTime	},
    {   "today",	0,	0,	FToday	},
    {   "trigdate",	0,	0,	FTrigdate },
    {   "trigger",	1,	3,	FTrigger },
    {   "trigtime",	0,	0,	FTrigtime },
    {   "trigvalid",	0,	0,	FTrigvalid },
    {   "typeof",       1,      1,      FTypeof },
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
#ifdef HAVE_PROTOS
PUBLIC int CallFunc(Operator *f, int nargs)
#else
int CallFunc(f, nargs)
Operator *f;
int nargs;
#endif
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
#ifdef HAVE_PROTOS
PRIVATE int CheckArgs(Operator *f, int nargs)
#else
static int CheckArgs(f, nargs)
Operator *f;
int nargs;
#endif
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
#ifdef HAVE_PROTOS
PRIVATE int CleanUpAfterFunc(void)
#else
static int CleanUpAfterFunc()
#endif
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
#ifdef HAVE_PROTOS
PRIVATE int RetStrVal(const char *s)
#else
static int RetStrVal(s)
char *s;
#endif
{
    RetVal.type = STR_TYPE;
    if (!s) {
	RetVal.v.str = (char *) malloc(1);
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
#ifdef HAVE_PROTOS
PRIVATE int FStrlen(void)
#else
static int FStrlen()
#endif
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
#ifdef HAVE_PROTOS
PRIVATE int FBaseyr(void)
#else
static int FBaseyr()
#endif
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
#ifdef HAVE_PROTOS
PRIVATE int FDate(void)
#else
static int FDate()
#endif
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
/*  FCoerce - type coercion function.                          */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int FCoerce(void)
#else
static int FCoerce()
#endif
{
    char *s;

    if (ARG(0).type != STR_TYPE) return E_BAD_TYPE;
    s = ARG(0).v.str;

    /* Copy the value of ARG(1) into RetVal, and make ARG(1) invalid so
       it won't be destroyed */
    DCOPYVAL(RetVal, ARG(1));

    if (! StrCmpi(s, "int")) return DoCoerce(INT_TYPE, &RetVal);
    else if (! StrCmpi(s, "date")) return DoCoerce(DATE_TYPE, &RetVal);
    else if (! StrCmpi(s, "time")) return DoCoerce(TIM_TYPE, &RetVal);
    else if (! StrCmpi(s, "string")) return DoCoerce(STR_TYPE, &RetVal);
    else return E_CANT_COERCE;
}

/***************************************************************/
/*                                                             */
/*  FMax - select maximum from a list of args.                 */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int FMax(void)
#else
static int FMax()
#endif
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
#ifdef HAVE_PROTOS
PRIVATE int FMin(void)
#else
static int FMin()
#endif
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
#ifdef HAVE_PROTOS
PRIVATE int FAsc(void)
#else
static int FAsc()
#endif
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
#ifdef HAVE_PROTOS
PRIVATE int FChar(void)
#else
static int FChar()
#endif
{

    int i, len;

/* Special case of one arg - if given ascii value 0, create empty string */
    if (Nargs == 1) {
	if (ARG(0).type != INT_TYPE) return E_BAD_TYPE;
	if (ARG(0).v.val < -128) return E_2LOW;
	if (ARG(0).v.val > 255) return E_2HIGH;
	len = ARG(0).v.val ? 2 : 1;
	RetVal.v.str = (char *) malloc(len);
	if (!RetVal.v.str) return E_NO_MEM;
	RetVal.type = STR_TYPE;
	*(RetVal.v.str) = ARG(0).v.val;
	if (len>1) *(RetVal.v.str + 1) = 0;
	return OK;
    }

    RetVal.v.str = (char *) malloc(Nargs + 1);
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
#ifdef HAVE_PROTOS
PRIVATE int FDay(void)
#else
static int FDay()
#endif
{
    int y, m, d;
    if (ARG(0).type != DATE_TYPE) return E_BAD_TYPE;
    if (ARG(0).v.val == CacheJul)
	d = CacheDay;
    else {
	FromJulian(ARG(0).v.val, &y, &m, &d);
	CacheJul = ARG(0).v.val;
	CacheYear = y;
	CacheMon = m;
	CacheDay = d;
    }
    RetVal.type = INT_TYPE;
    RetVal.v.val = d;
    return OK;
}

#ifdef HAVE_PROTOS
PRIVATE int FMonnum(void)
#else
static int FMonnum()
#endif
{
    int y, m, d;
    if (ARG(0).type != DATE_TYPE) return E_BAD_TYPE;
    if (ARG(0).v.val == CacheJul)
	m = CacheMon;
    else {
	FromJulian(ARG(0).v.val, &y, &m, &d);
	CacheJul = ARG(0).v.val;
	CacheYear = y;
	CacheMon = m;
	CacheDay = d;
    }
    RetVal.type = INT_TYPE;
    RetVal.v.val = m+1;
    return OK;
}

#ifdef HAVE_PROTOS
PRIVATE int FYear(void)
#else
static int FYear()
#endif
{
    int y, m, d;
    if (ARG(0).type != DATE_TYPE) return E_BAD_TYPE;
    if (ARG(0).v.val == CacheJul)
	y = CacheYear;
    else {
	FromJulian(ARG(0).v.val, &y, &m, &d);
	CacheJul = ARG(0).v.val;
	CacheYear = y;
	CacheMon = m;
	CacheDay = d;
    }
    RetVal.type = INT_TYPE;
    RetVal.v.val = y;
    return OK;
}

#ifdef HAVE_PROTOS
PRIVATE int FWkdaynum(void)
#else
static int FWkdaynum()
#endif
{
    if (ARG(0).type != DATE_TYPE) return E_BAD_TYPE;
    RetVal.type = INT_TYPE;

    /* Correct so that 0 = Sunday */
    RetVal.v.val = (ARG(0).v.val+1) % 7;
    return OK;
}

#ifdef HAVE_PROTOS
PRIVATE int FWkday(void)
#else
static int FWkday()
#endif
{
    char *s;

    if (ARG(0).type != DATE_TYPE && ARG(0).type != INT_TYPE) return E_BAD_TYPE;
    if (ARG(0).type == INT_TYPE) {
	if (ARG(0).v.val < 0) return E_2LOW;
	if (ARG(0).v.val > 6) return E_2HIGH;
	/* Convert 0=Sun to 0=Mon */
	ARG(0).v.val--;
	if (ARG(0).v.val < 0) ARG(0).v.val = 6;
	s = DayName[ARG(0).v.val];
    } else s = DayName[ARG(0).v.val % 7];
    return RetStrVal(s);
}

#ifdef HAVE_PROTOS
PRIVATE int FMon(void)
#else
static int FMon()
#endif
{
    char *s;
    int y, m, d;

    if (ARG(0).type != DATE_TYPE && ARG(0).type != INT_TYPE)
	return E_BAD_TYPE;
    if (ARG(0).type == INT_TYPE) {
	m = ARG(0).v.val - 1;
	if (m < 0) return E_2LOW;
	if (m > 11) return E_2HIGH;
    } else {
	if (ARG(0).v.val == CacheJul)
	    m = CacheMon;
	else {
	    FromJulian(ARG(0).v.val, &y, &m, &d);
	    CacheJul = ARG(0).v.val;
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
#ifdef HAVE_PROTOS
PRIVATE int FHour(void)
#else
static int FHour()
#endif
{
    if (ARG(0).type != TIM_TYPE) return E_BAD_TYPE;
    RetVal.type = INT_TYPE;
    RetVal.v.val = ARG(0).v.val / 60;
    return OK;
}

#ifdef HAVE_PROTOS
PRIVATE int FMinute(void)
#else
static int FMinute()
#endif
{
    if (ARG(0).type != TIM_TYPE) return E_BAD_TYPE;
    RetVal.type = INT_TYPE;
    RetVal.v.val = ARG(0).v.val % 60;
    return OK;
}

#ifdef HAVE_PROTOS
PRIVATE int FTime(void)
#else
static int FTime()
#endif
{
    int h, m;

    if (ARG(0).type != INT_TYPE || ARG(1).type != INT_TYPE) return E_BAD_TYPE;

    h = ARG(0).v.val;
    m = ARG(1).v.val;
    if (h<0 || m<0) return E_2LOW;
    if (h>23 || m>59) return E_2HIGH;
    RetVal.type = TIM_TYPE;
    RetVal.v.val = h*60 + m;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FAbs - absolute value                                      */
/*  FSgn - signum function                                     */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int FAbs(void)
#else
static int FAbs()
#endif
{
    int v;

    if (ARG(0).type != INT_TYPE) return E_BAD_TYPE;
    v = ARG(0).v.val;
    RetVal.type = INT_TYPE;
    RetVal.v.val = (v < 0) ? (-v) : v;
    return OK;
}

#ifdef HAVE_PROTOS
PRIVATE int FSgn(void)
#else
static int FSgn()
#endif
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
#ifdef HAVE_PROTOS
PRIVATE int FOrd(void)
#else
static int FOrd()
#endif
{
    int t, u, v;
    char *s;

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
#ifdef HAVE_PROTOS
PRIVATE int FPlural(void)
#else
static int FPlural()
#endif
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
	RetVal.v.str = (char *) malloc(strlen(ARG(1).v.str)+2);
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
#ifdef HAVE_PROTOS
PRIVATE int FChoose(void)
#else
static int FChoose()
#endif
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
#ifdef HAVE_PROTOS
PRIVATE int FVersion(void)
#else
static int FVersion()
#endif
{
    return RetStrVal(VERSION);
}

/***************************************************************/
/*                                                             */
/*  FOstype - the type of operating system                     */
/*  (UNIX, OS/2, or MSDOS)                                     */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int FOstype(void)
#else
static int FOstype()
#endif
{
#ifdef UNIX
    return RetStrVal("UNIX");
#else
#ifdef __OS2__
    return RetStrVal(OS2MODE ? "OS/2" : "MSDOS");
#else
#ifdef QDOS
    return RetStrVal("QDOS / SMSQ");
#else
#ifdef AMIGA
    return RetStrVal("AmigaDOS");
#else
    return RetStrVal("MSDOS");
#endif
#endif
#endif
#endif
}

/***************************************************************/
/*                                                             */
/*  FUpper - convert string to upper-case                      */
/*  FLower - convert string to lower-case                      */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int FUpper(void)
#else
static int FUpper()
#endif
{
    char *s;

    if (ARG(0).type != STR_TYPE) return E_BAD_TYPE;
    DCOPYVAL(RetVal, ARG(0));
    s = RetVal.v.str;
    while (*s) { *s = UPPER(*s); s++; }
    return OK;
}

#ifdef HAVE_PROTOS
PRIVATE int FLower(void)
#else
static int FLower()
#endif
{
    char *s;

    if (ARG(0).type != STR_TYPE) return E_BAD_TYPE;
    DCOPYVAL(RetVal, ARG(0));
    s = RetVal.v.str;
    while (*s) { *s = LOWER(*s); s++; }
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
#ifdef HAVE_PROTOS
PRIVATE int FToday(void)
#else
static int FToday()
#endif
{
    RetVal.type = DATE_TYPE;
    RetVal.v.val = JulianToday;
    return OK;
}

#ifdef HAVE_PROTOS
PRIVATE int FRealtoday(void)
#else
static int FRealtoday()
#endif
{
    RetVal.type = DATE_TYPE;
    RetVal.v.val = RealToday;
    return OK;
}

#ifdef HAVE_PROTOS
PRIVATE int FNow(void)
#else
static int FNow()
#endif
{
    RetVal.type = TIM_TYPE;
    RetVal.v.val = (int) ( SystemTime(0) / 60L );
    return OK;
}

#ifdef HAVE_PROTOS
PRIVATE int FRealnow(void)
#else
static int FRealnow()
#endif
{
    RetVal.type = TIM_TYPE;
    RetVal.v.val = (int) ( SystemTime(1) / 60L );
    return OK;
}
/***************************************************************/
/*                                                             */
/*  FGetenv - get the value of an environment variable.        */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int FGetenv(void)
#else
static int FGetenv()
#endif
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
#ifdef HAVE_PROTOS
PRIVATE int FValue(void)
#else
static int FValue()
#endif
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
#ifdef HAVE_PROTOS
PRIVATE int FDefined(void)
#else
static int FDefined()
#endif
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
#ifdef HAVE_PROTOS
PRIVATE int FTrigdate(void)
#else
static int FTrigdate()
#endif
{
    RetVal.type = DATE_TYPE;
    RetVal.v.val = LastTriggerDate;
    return OK;
}

#ifdef HAVE_PROTOS
PRIVATE int FTrigvalid(void)
#else
static int FTrigvalid()
#endif
{
    RetVal.type = INT_TYPE;
    RetVal.v.val = LastTrigValid;
    return OK;
}

#ifdef HAVE_PROTOS
PRIVATE int FTrigtime(void)
#else
static int FTrigtime()
#endif
{
    RetVal.type = TIM_TYPE;
    RetVal.v.val = LastTriggerTime;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FDaysinmon                                                 */
/*                                                             */
/*  Returns the number of days in mon,yr                       */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int FDaysinmon(void)
#else
static int FDaysinmon()
#endif
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
#ifdef HAVE_PROTOS
PRIVATE int FIsleap(void)
#else
static int FIsleap()
#endif
{
    int y, m, d;

    if (ARG(0).type != INT_TYPE && ARG(0).type != DATE_TYPE) return E_BAD_TYPE;

    /* If it's a date, extract the year */
    if (ARG(0).type == DATE_TYPE)
	FromJulian(ARG(0).v.val, &y, &m, &d);
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
#ifdef HAVE_PROTOS
PRIVATE int FTrigger(void)
#else
static int FTrigger()
#endif
{
    int y, m, d;
    int date, time;
    char buf[40];

    if (ARG(0).type != DATE_TYPE) return E_BAD_TYPE;
    date = ARG(0).v.val;
    if (Nargs > 2) {
	if (ARG(2).type != INT_TYPE) return E_BAD_TYPE;
	if (ARG(1).type != TIM_TYPE) return E_BAD_TYPE;
	if (ARG(2).v.val) {
	    UTCToLocal(ARG(0).v.val, ARG(1).v.val, &date, &time);
	} else {
	    date = ARG(0).v.val;
	    time = ARG(1).v.val;
	}
    }
    FromJulian(date, &y, &m, &d);
    if (Nargs > 1) {
	if (ARG(1).type != TIM_TYPE) return E_BAD_TYPE;
	if (Nargs == 2) time = ARG(1).v.val;
	sprintf(buf, "%d %s %d AT %02d:%02d", d, EnglishMonthName[m], y,
		time/60, time%60);
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
#ifdef HAVE_PROTOS
PRIVATE int FShell(void)
#else
static int FShell()
#endif
{
    char buf[SHELLSIZE+1];
    int ch, len;
    FILE *fp;
    char *s;

    if (RunDisabled) return E_RUN_DISABLED;
    if (ARG(0).type != STR_TYPE) return E_BAD_TYPE;
    s = buf;
    len = 0;
    fp = POPEN(ARG(0).v.str, "r");
    if (!fp) return E_IO_ERR;
    while (len < SHELLSIZE) {
	ch = getc(fp);
	if (ch == EOF) {
	    break;
	}
	if (isspace(ch)) *s++ = ' ';
	else            *s++ = ch;
	len++;
    }
    *s = 0;

    /* Delete trailing newline (converted to space) */
    if (s > buf && *(s-1) == ' ') *(s-1) = 0;
#if defined(__MSDOS__) || defined(__OS2__)
    if (s-1 > buf && *(s-2) == ' ') *(s-2) = 0;
#endif
    PCLOSE(fp);
    return RetStrVal(buf);
}

/***************************************************************/
/*                                                             */
/*  FIsomitted                                                 */
/*                                                             */
/*  Is a date omitted?                                         */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int FIsomitted(void)
#else
static int FIsomitted()
#endif
{
    if (ARG(0).type != DATE_TYPE) return E_BAD_TYPE;
    RetVal.type = INT_TYPE;
    RetVal.v.val = IsOmitted(ARG(0).v.val, 0);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FSubstr                                                    */
/*                                                             */
/*  The substr function.  We destroy the value on the stack.   */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int FSubstr(void)
#else
static int FSubstr()
#endif
{
    char *s, *t;
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
#ifdef HAVE_PROTOS
PRIVATE int FIndex(void)
#else
static int FIndex()
#endif
{
    char *s;
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
#ifdef HAVE_PROTOS
PRIVATE int FIif(void)
#else
static int FIif()
#endif
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
#ifdef HAVE_PROTOS
PRIVATE int FFilename(void)
#else
static int FFilename()
#endif
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
#ifdef HAVE_PROTOS
PRIVATE int FFiledir(void)
#else
static int FFiledir()
#endif
{
    char TmpBuf[LINELEN];  /* Should be _POSIX_PATH_MAX ? */
    char *s;

    strcpy(TmpBuf, FileName);
    s = TmpBuf + strlen(TmpBuf) - 1;
    if (s < TmpBuf) return RetStrVal(".");
#if defined(__OS2__) || defined(__MSDOS__)
    /* Both '\\' and '/' can be part of path; handle drive letters. */
    while (s > TmpBuf && !strchr("\\/:", *s)) s--;
    if (*s == ':') { s[1] = '.'; s += 2; }
    if (s > TmpBuf) *s = '/';
#else
    while (s > TmpBuf && *s != '/') s--;
#endif
    if (*s == '/') {
	*s = 0;
	return RetStrVal(TmpBuf);
    } else return RetStrVal(".");
}
/***************************************************************/
/*                                                             */
/*  FAccess                                                    */
/*                                                             */
/*  The UNIX access() system call.                             */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int FAccess(void)
#else
static int FAccess()
#endif
{
    int amode;
    char *s;

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

#if defined(__MSDOS__) || defined(__BORLANDC__) || defined(AMIGA)
/***************************************************************/
/*                                                             */
/*  popen and pclose                                           */
/*                                                             */
/*  These are some rather brain-dead kludges for MSDOS.        */
/*  They are just sufficient for the shell() function, and     */
/*  should NOT be viewed as general-purpose replacements       */
/*  for the UNIX system calls.                                 */
/*                                                             */
/***************************************************************/
#ifdef __TURBOC__
#pragma argsused
#endif

static char *TmpFile;
#ifdef HAVE_PROTOS
PRIVATE FILE *os_popen(char *cmd, char *mode)
#else
static FILE *os_popen(cmd, mode)
char *cmd, *mode;
#endif
{
    char *s;

#if defined(__OS2__) && !defined(__BORLANDC__)
    if (OS2MODE)
	return(popen(cmd, mode));
#endif

    TmpFile = tmpnam(NULL);
    if (!TmpFile) return NULL;
    s = (char *) malloc(strlen(cmd) + 3 + strlen(TmpFile) + 1);
    if (!s) return NULL;
    strcpy(s, cmd);
    strcat(s, " > ");
    strcat(s, TmpFile);
    system(s);
    free(s);
    return fopen(TmpFile, "r");
}

#ifdef HAVE_PROTOS
PRIVATE int os_pclose(FILE *fp)
#else
static int os_pclose(fp)
FILE *fp;
#endif
{
#if defined(__OS2__) && !defined(__BORLANDC__)
    if (OS2MODE)
	return(pclose(fp));
#endif

    unlink(TmpFile);
#ifdef AMIGA
    free(TmpFile);
#endif
    return fclose(fp);
}

#endif

/***************************************************************/
/*                                                             */
/*  FTypeof                                                    */
/*                                                             */
/*  Implement the typeof() function.                           */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int FTypeof(void)
#else
static int FTypeof()
#endif
{
    switch(ARG(0).type) {
    case INT_TYPE:  return RetStrVal("INT");
    case DATE_TYPE: return RetStrVal("DATE");
    case TIM_TYPE:  return RetStrVal("TIME");
    case STR_TYPE:  return RetStrVal("STRING");
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
#ifdef HAVE_PROTOS
PRIVATE int FLanguage(void)
#else
static int FLanguage()
#endif
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
#ifdef HAVE_PROTOS
PRIVATE int FArgs(void)
#else
static int FArgs()
#endif
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
#ifdef HAVE_PROTOS
PRIVATE int FDosubst(void)
#else
static int FDosubst()
#endif
{
    int jul, tim, r;
    char TmpBuf[LINELEN];

    jul = NO_DATE;
    tim = NO_TIME;
    if (ARG(0).type != STR_TYPE) return E_BAD_TYPE;
    if (Nargs >= 2) {
	if (ARG(1).type != DATE_TYPE) return E_BAD_TYPE;
	jul = ARG(1).v.val;
	if (Nargs >= 3) {
	    if (ARG(2).type != TIM_TYPE) return E_BAD_TYPE;
	    tim = ARG(2).v.val;
	}
    }

    if ((r=DoSubstFromString(ARG(0).v.str, TmpBuf, jul, tim))) return r;
    return RetStrVal(TmpBuf);
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
#ifdef HAVE_PROTOS
PRIVATE int FHebdate(void)
#else
static int FHebdate()
#endif
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
    } else if (ARG(2).type == DATE_TYPE) {
	r = GetNextHebrewDate(ARG(2).v.val, mon, day, jahr, adarbehave, &ans);
	if (r) return r;
	RetVal.v.val = ans;
	RetVal.type = DATE_TYPE;
	return OK;
    } else return E_BAD_TYPE;
}

#ifdef HAVE_PROTOS
PRIVATE int FHebday(void)
#else
static int FHebday()
#endif
{
    int y, m, d;

    if (ARG(0).type != DATE_TYPE) return E_BAD_TYPE;
    if (ARG(0).v.val == CacheHebJul)
	d = CacheHebDay;
    else {
	JulToHeb(ARG(0).v.val, &y, &m, &d);
	CacheHebJul = ARG(0).v.val;
	CacheHebYear = y;
	CacheHebMon = m;
	CacheHebDay = d;
    }
    RetVal.type = INT_TYPE;
    RetVal.v.val = d;
    return OK;
}

#ifdef HAVE_PROTOS
PRIVATE int FHebmon(void)
#else
static int FHebmon()
#endif
{
    int y, m, d;

    if (ARG(0).type != DATE_TYPE) return E_BAD_TYPE;
    if (ARG(0).v.val == CacheHebJul) {
	m = CacheHebMon;
	y = CacheHebYear;
    } else {
	JulToHeb(ARG(0).v.val, &y, &m, &d);
	CacheHebJul = ARG(0).v.val;
	CacheHebYear = y;
	CacheHebMon = m;
	CacheHebDay = d;
    }
    return RetStrVal(HebMonthName(m, y));
}

#ifdef HAVE_PROTOS
PRIVATE int FHebyear(void)
#else
static int FHebyear()
#endif
{
    int y, m, d;

    if (ARG(0).type != DATE_TYPE) return E_BAD_TYPE;
    if (ARG(0).v.val == CacheHebJul)
	y = CacheHebYear;
    else {
	JulToHeb(ARG(0).v.val, &y, &m, &d);
	CacheHebJul = ARG(0).v.val;
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
#ifdef HAVE_PROTOS
PRIVATE int FEasterdate(void)
#else
static int FEasterdate()
#endif
{
    int y, m, d;
    int g, c, x, z, e, n;
    if (ARG(0).type == INT_TYPE) {
	y = ARG(0).v.val;
	if (y < BASE) return E_2LOW;
	else if (y > BASE+YR_RANGE) return E_2HIGH;
    } else if (ARG(0).type == DATE_TYPE) {
	FromJulian(ARG(0).v.val, &y, &m, &d);  /* We just want the year */
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
	y++; } while (ARG(0).type == DATE_TYPE && RetVal.v.val < ARG(0).v.val);

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
PRIVATE int FTimeStuff ARGS ((int wantmins));
#ifdef HAVE_PROTOS
PRIVATE int FIsdst(void)
#else
static int FIsdst()
#endif
{
    return FTimeStuff(0);
}

#ifdef HAVE_PROTOS
PRIVATE int FMinsfromutc(void)
#else
static int FMinsfromutc()
#endif
{
    return FTimeStuff(1);
}

#ifdef HAVE_PROTOS
PRIVATE int FTimeStuff(int wantmins)
#else
static int FTimeStuff(wantmins)
int wantmins;
#endif
{
    int jul, tim;
    int mins, dst;

    jul = JulianToday;
    tim = 0;

    if (Nargs >= 1) {
	if (ARG(0).type != DATE_TYPE) return E_BAD_TYPE;
	jul = ARG(0).v.val;
	if (Nargs >= 2) {
	    if (ARG(1).type != TIM_TYPE) return E_BAD_TYPE;
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

#ifdef HAVE_PROTOS
PRIVATE int SunStuff(int rise, double cosz, int jul)
#else
static int SunStuff(rise, cosz, jul)
int rise;
double cosz;
int jul;
#endif
{
    int year, mon, day;
    int jan0;
    int mins, hours;

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

/* Following formula on page B6 exactly... */
    t = (double) jan0;
    if (rise) t += (6.0 + longdeg/15.0) / 24.0;
    else      t += (18.0 + longdeg/15.0) / 24.0;

/* Mean anomaly of sun for 1978 ... how accurate for other years??? */
    M = 0.985600 * t - 3.251;  /* In degrees */

/* Sun's true longitude */
    L = M + 1.916*sin(DEGRAD*M) + 0.02*sin(2*DEGRAD*M) + 282.565;
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

    if (cosH > 1.0 || cosH < -1.0) return NO_TIME;

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

    return hours*60 + mins;
}

/***************************************************************/
/*                                                             */
/*  Sunrise and Sunset functions.                              */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int FSun(int rise)
#else
static int FSun(rise)
int rise;
#endif
{
    int jul = JulianToday;
    static double cosz = -0.014543897;  /* for sunrise and sunset */
    int r;

    if (Nargs >= 1) {
	if (ARG(0).type != DATE_TYPE) return E_BAD_TYPE;
	jul = ARG(0).v.val;
    }
   
    r = SunStuff(rise, cosz, jul);
    if (r == NO_TIME) {
	RetVal.v.val = 0;
	RetVal.type = INT_TYPE;
    } else {
	RetVal.v.val = r;
	RetVal.type = TIM_TYPE;
    }
    return OK;
}
      
#ifdef HAVE_PROTOS
PRIVATE int FSunrise(void)
#else
static int FSunrise()
#endif
{
    return FSun(1);
}
#ifdef HAVE_PROTOS
PRIVATE int FSunset(void)
#else
static int FSunset()
#endif
{
    return FSun(0);
}

/***************************************************************/
/*                                                             */
/*  FFiledate                                                  */
/*                                                             */
/*  Return modification date of a file                         */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int FFiledate(void)
#else
static int FFiledate()
#endif
{
    struct stat statbuf;
    struct tm *t1;

    RetVal.type = DATE_TYPE;

    if (ARG(0).type != STR_TYPE) return E_BAD_TYPE;

    if (stat(ARG(0).v.str, &statbuf)) {
	RetVal.v.val = 0;
	return OK;
    }

#ifdef __TURBOC__
    t1 = localtime( (time_t *) &(statbuf.st_mtime) );
#else
    t1 = localtime(&(statbuf.st_mtime));
#endif

    if (t1->tm_year + 1900 < BASE)
	RetVal.v.val=0;
    else
	RetVal.v.val=Julian(t1->tm_year+1900, t1->tm_mon, t1->tm_mday);

    return OK;
}

/***************************************************************/
/*                                                             */
/*  FPsshade                                                   */
/*                                                             */
/*  Canned PostScript code for shading a calendar square       */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int FPsshade(void)
#else
static int FPsshade()
#endif
{
    char psbuff[256];
    char *s = psbuff;
    if (ARG(0).type != INT_TYPE) return E_BAD_TYPE;
    if (ARG(0).v.val < 0) return E_2LOW;
    if (ARG(0).v.val > 100) return E_2HIGH;

    sprintf(s, "/_A LineWidth 2 div def ");
    s += strlen(s);
    sprintf(s, "_A _A moveto ");
    s += strlen(s);
    sprintf(s, "BoxWidth _A sub _A lineto BoxWidth _A sub BoxHeight _A sub lineto ");
    s += strlen(s);
    sprintf(s, "_A BoxHeight _A sub lineto closepath %d 100 div setgray fill 0.0 setgray", ARG(0).v.val);
    return RetStrVal(psbuff);
}

/***************************************************************/
/*                                                             */
/*  FPsmoon                                                    */
/*                                                             */
/*  Canned PostScript code for generating moon phases          */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int FPsmoon(void)
#else
static int FPsmoon()
#endif
{
    char psbuff[512];
    char sizebuf[30];
    char fontsizebuf[30];
    char *s = psbuff;
    char *extra = NULL;
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
#ifdef HAVE_PROTOS
PRIVATE int FMoonphase(void)
#else
static int FMoonphase()
#endif
{
    int date, time;

    switch(Nargs) {
    case 0:
	date = JulianToday;
	time = 0;
	break;
    case 1:
	if (ARG(0).type != DATE_TYPE) return E_BAD_TYPE;
	date = ARG(0).v.val;
	time = 0;
	break;
    case 2:
	if (ARG(0).type != DATE_TYPE && ARG(1).type != TIM_TYPE) return E_BAD_TYPE;
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
PRIVATE int MoonStuff ARGS ((int want_time));
#ifdef HAVE_PROTOS
PRIVATE int FMoondate(void)
#else
static int FMoondate()
#endif
{
    return MoonStuff(0);
}

#ifdef HAVE_PROTOS
PRIVATE int FMoontime(void)
#else
static int FMoontime()
#endif
{
    return MoonStuff(1);
}

#ifdef HAVE_PROTOS
PRIVATE int MoonStuff(int want_time)
#else
static int MoonStuff(want_time)
int want_time;
#endif
{
    int startdate, starttim;
    int d, t;

    startdate = JulianToday;
    starttim = 0;

    if (ARG(0).type != INT_TYPE) return E_BAD_TYPE;
    if (ARG(0).v.val < 0) return E_2LOW;
    if (ARG(0).v.val > 3) return E_2HIGH;
    if (Nargs >= 2) {
	if (ARG(1).type != DATE_TYPE) return E_BAD_TYPE;
	startdate = ARG(1).v.val;
	if (Nargs >= 3) {
	    if (ARG(2).type != TIM_TYPE) return E_BAD_TYPE;
	    starttim = ARG(2).v.val;
	}
    }

    HuntPhase(startdate, starttim, ARG(0).v.val, &d, &t);
    if (want_time) {
	RetVal.type = TIM_TYPE;
	RetVal.v.val = t;
    } else {
	RetVal.type = DATE_TYPE;
	RetVal.v.val = d;
    }
    return OK;
}


