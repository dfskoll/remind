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

/* Defines that used to be static variables */
#define Nargs (info->nargs)
#define RetVal (info->retval)

/* Function prototypes */
static int FCurrent (func_info *);
static int FNonomitted (func_info *);
static int FTimepart(func_info *);
static int FDatepart(func_info *);
static int FRealCurrent(func_info *);
static	int	FAbs		(func_info *);
static	int	FAccess		(func_info *);
static int     FArgs		(func_info *);
static	int	FAsc		(func_info *);
static	int	FBaseyr		(func_info *);
static	int	FChar		(func_info *);
static	int	FChoose		(func_info *);
static	int	FCoerce		(func_info *);
static	int	FDate		(func_info *);
static	int	FDateTime	(func_info *);
static	int	FDay		(func_info *);
static	int	FDaysinmon	(func_info *);
static	int	FDefined	(func_info *);
static	int	FDosubst	(func_info *);
static	int	FEasterdate	(func_info *);
static  int     FEvalTrig       (func_info *);
static int	FFiledate	(func_info *);
static int	FFiledatetime	(func_info *);
static	int	FFiledir	(func_info *);
static	int	FFilename	(func_info *);
static	int	FGetenv		(func_info *);
static int     FHebdate	(func_info *);
static int     FHebday		(func_info *);
static int     FHebmon		(func_info *);
static int     FHebyear	(func_info *);
static	int	FHour		(func_info *);
static	int	FIif		(func_info *);
static	int	FIndex		(func_info *);
static	int	FIsdst		(func_info *);
static	int	FIsomitted	(func_info *);
static	int	FSlide  	(func_info *);
static	int	FLanguage	(func_info *);
static	int	FMax		(func_info *);
static	int	FMin		(func_info *);
static	int	FMinute		(func_info *);
static	int	FMinsfromutc	(func_info *);
static	int	FMoondate	(func_info *);
static	int	FMoondatetime	(func_info *);
static	int	FMoonphase	(func_info *);
static	int	FMoontime	(func_info *);
static	int	FMon		(func_info *);
static	int	FMonnum		(func_info *);
static	int	FOrd		(func_info *);
static	int	FOstype 	(func_info *);
static	int	FPlural		(func_info *);
static	int	FSgn		(func_info *);
static int	FPsmoon		(func_info *);
static int	FPsshade	(func_info *);
static	int	FShell		(func_info *);
static	int	FStrlen		(func_info *);
static	int	FSubstr		(func_info *);
static	int	FDawn		(func_info *);
static	int	FDusk	 	(func_info *);
static	int	FSunset		(func_info *);
static	int	FSunrise	(func_info *);
static	int	FTime		(func_info *);
static	int	FTrigdate	(func_info *);
static	int	FTrigdatetime	(func_info *);
static	int	FTrigtime	(func_info *);
static	int	FTrigvalid	(func_info *);
static	int	FTypeof		(func_info *);
static	int	FUpper		(func_info *);
static	int	FValue		(func_info *);
static	int	FVersion	(func_info *);
static	int	FWkday		(func_info *);
static	int	FWkdaynum	(func_info *);
static	int	FYear		(func_info *);
static int	FIsleap         (func_info *);
static int	FLower          (func_info *);
static int	FNow            (func_info *);
static int	FRealnow            (func_info *);
static int	FRealtoday      (func_info *);
static int	FToday          (func_info *);
static int	FTrigger        (func_info *);
static int      FTzconvert      (func_info *);
static int      FWeekno         (func_info *);
static int	CheckArgs       (BuiltinFunc *f, int nargs);
static int	CleanUpAfterFunc (func_info *);
static int	SunStuff	(int rise, double cosz, int jul);

/* "Overload" the struct Operator definition */
#define NO_MAX 127

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

#define ARGV(x) ARG(x).v.val
#define ARGSTR(x) ARG(x).v.str

#define ASSERT_TYPE(x, t) if (ARG(x).type != t) return E_BAD_TYPE

/* Macro for getting date part of a date or datetime value */
#define DATEPART(x) ((x).type == DATE_TYPE ? (x).v.val : ((x).v.val / MINUTES_PER_DAY))

/* Macro for getting time part of a time or datetime value */
#define TIMEPART(x) ((x).type == TIME_TYPE ? (x).v.val : ((x).v.val % MINUTES_PER_DAY))

#define HASDATE(x) ((x).type == DATE_TYPE || (x).type == DATETIME_TYPE)
#define HASTIME(x) ((x).type == TIME_TYPE || (x).type == DATETIME_TYPE)

/* Macro for copying a value while destroying original copy */
#define DCOPYVAL(x, y) ( (x) = (y), (y).type = ERR_TYPE )

/* Get at RetVal.v.val easily */
#define RETVAL info->retval.v.val

/* Convenience macros */
#define UPPER(c) (islower(c) ? toupper(c) : c)
#define LOWER(c) (isupper(c) ? tolower(c) : c)

/* The array holding the built-in functions. */
BuiltinFunc Func[] = {
/*	Name		minargs maxargs	is_constant func   */

    {   "abs",          1,      1,      1,          FAbs },
    {   "access",       2,      2,      0,          FAccess },
    {   "args",         1,      1,      0,          FArgs   },
    {   "asc",          1,      1,      1,          FAsc    },
    {   "baseyr",       0,      0,      1,          FBaseyr },
    {   "char",         1,      NO_MAX, 1,          FChar   },
    {   "choose",       2,      NO_MAX, 1,          FChoose },
    {   "coerce",       2,      2,      1,          FCoerce },
    {   "current",      0,      0,      0,          FCurrent },
    {   "date",         3,      3,      1,          FDate   },
    {   "datepart",     1,      1,      1,          FDatepart },
    {   "datetime",     2,      5,      1,          FDateTime },
    {   "dawn",         0,      1,      0,          FDawn},
    {   "day",          1,      1,      1,          FDay    },
    {   "daysinmon",    2,      2,      1,          FDaysinmon },
    {   "defined",      1,      1,      0,          FDefined },
    {   "dosubst",      1,      3,      0,          FDosubst },
    {   "dusk",         0,      1,      0,          FDusk },
    {   "easterdate",   1,      1,      0,          FEasterdate },
    {   "evaltrig",     1,      2,      0,          FEvalTrig },
    {   "filedate",     1,      1,      0,          FFiledate },
    {   "filedatetime", 1,      1,      0,          FFiledatetime },
    {   "filedir",      0,      0,      0,          FFiledir },
    {   "filename",     0,      0,      0,          FFilename },
    {   "getenv",       1,      1,      0,          FGetenv },
    {   "hebdate",      2,      5,      0,          FHebdate },
    {   "hebday",       1,      1,      0,          FHebday },
    {   "hebmon",       1,      1,      0,          FHebmon },
    {   "hebyear",      1,      1,      0,          FHebyear },
    {   "hour",         1,      1,      1,          FHour   },
    {   "iif",          1,      NO_MAX, 1,          FIif    },
    {   "index",        2,      3,      1,          FIndex  },
    {   "isdst",        0,      2,      0,          FIsdst },
    {   "isleap",       1,      1,      1,          FIsleap },
    {   "isomitted",    1,      1,      0,          FIsomitted },
    {   "language",     0,      0,      1,          FLanguage },
    {   "lower",        1,      1,      1,          FLower  },
    {   "max",          1,      NO_MAX, 1,          FMax    },
    {   "min",          1,      NO_MAX, 1,          FMin    },
    {   "minsfromutc",  0,      2,      0,          FMinsfromutc },
    {   "minute",       1,      1,      1,          FMinute },
    {   "mon",          1,      1,      1,          FMon    },
    {   "monnum",       1,      1,      1,          FMonnum },
    {   "moondate",     1,      3,      0,          FMoondate },
    {   "moondatetime", 1,      3,      0,          FMoondatetime },
    {   "moonphase",    0,      2,      0,          FMoonphase },
    {   "moontime",     1,      3,      0,          FMoontime },
    {   "nonomitted",   2,      NO_MAX, 0,          FNonomitted },
    {   "now",          0,      0,      0,          FNow    },
    {   "ord",          1,      1,      1,          FOrd    },
    {   "ostype",       0,      0,      1,          FOstype },
    {   "plural",       1,      3,      1,          FPlural },
    {   "psmoon",       1,      4,      1,          FPsmoon},
    {   "psshade",      1,      3,      1,          FPsshade},
    {   "realcurrent",  0,      0,      0,          FRealCurrent},
    {   "realnow",      0,      0,      0,          FRealnow},
    {   "realtoday",    0,      0,      0,          FRealtoday },
    {   "sgn",          1,      1,      1,          FSgn    },
    {   "shell",        1,      2,      0,          FShell  },
    {   "slide",        2,      NO_MAX, 0,          FSlide  },
    {   "strlen",       1,      1,      1,          FStrlen },
    {   "substr",       2,      3,      1,          FSubstr },
    {   "sunrise",      0,      1,      0,          FSunrise},
    {   "sunset",       0,      1,      0,          FSunset },
    {   "time",         2,      2,      1,          FTime   },
    {   "timepart",     1,      1,      1,          FTimepart },
    {   "today",        0,      0,      0,          FToday  },
    {   "trigdate",     0,      0,      0,          FTrigdate },
    {   "trigdatetime", 0,      0,      0,          FTrigdatetime },
    {   "trigger",      1,      3,      0,          FTrigger },
    {   "trigtime",     0,      0,      0,          FTrigtime },
    {   "trigvalid",    0,      0,      0,          FTrigvalid },
    {   "typeof",       1,      1,      1,          FTypeof },
    {   "tzconvert",    2,      3,      0,          FTzconvert },
    {   "upper",        1,      1,      1,          FUpper  },
    {   "value",        1,      2,      0,          FValue  },
    {   "version",      0,      0,      1,          FVersion },
    {   "weekno",       0,      3,      1,          FWeekno },
    {   "wkday",        1,      1,      1,          FWkday  },
    {   "wkdaynum",     1,      1,      1,          FWkdaynum },
    {   "year",         1,      1,      1,          FYear   }
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
int CallFunc(BuiltinFunc *f, int nargs)
{
    register int r = CheckArgs(f, nargs);
    int i;

    func_info info_obj;
    func_info *info = &info_obj;

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

    r = (*(f->func))(info);
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
    r = CleanUpAfterFunc(info);
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
static int CheckArgs(BuiltinFunc *f, int nargs)
{
    if (nargs < f->minargs) return E_2FEW_ARGS;
    if (nargs > f->maxargs && f->maxargs != NO_MAX) return E_2MANY_ARGS;
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
static int CleanUpAfterFunc(func_info *info)
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
static int RetStrVal(char const *s, func_info *info)
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
static int FStrlen(func_info *info)
{
    Value *v = &ARG(0);
    if (v->type != STR_TYPE) return E_BAD_TYPE;
    RetVal.type = INT_TYPE;
    RETVAL = strlen(v->v.str);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FBaseyr - system base year                                 */
/*                                                             */
/***************************************************************/
static int FBaseyr(func_info *info)
{
    RetVal.type = INT_TYPE;
    RETVAL = BASE;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FDate - make a date from year, month, day.                 */
/*                                                             */
/***************************************************************/
static int FDate(func_info *info)
{
    int y, m, d;
    int ytemp, mtemp, dtemp;

    /* Any arg can be a date (in which case we use the corresponding
       component) or an integer */
    if (HASDATE(ARG(0))) {
	FromJulian(DATEPART(ARG(0)), &ytemp, &mtemp, &dtemp);
	y = ytemp;
    } else {
	ASSERT_TYPE(0, INT_TYPE);
	y = ARGV(0);
    }

    if (HASDATE(ARG(1))) {
	FromJulian(DATEPART(ARG(1)), &ytemp, &mtemp, &dtemp);
	m = mtemp;
    } else {
	ASSERT_TYPE(1, INT_TYPE);
	m = ARGV(1) - 1;
    }

    if (HASDATE(ARG(2))) {
	FromJulian(DATEPART(ARG(2)), &ytemp, &mtemp, &dtemp);
	d = dtemp;
    } else {
	ASSERT_TYPE(2, INT_TYPE);
	d = ARGV(2);
    }

    if (!DateOK(y, m, d)) {
	return E_BAD_DATE;
    }
    RetVal.type = DATE_TYPE;
    RETVAL = Julian(y, m, d);
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
static int FDateTime(func_info *info)
{
    int y, m, d;

    RetVal.type = DATETIME_TYPE;

    switch(Nargs) {
    case 2:
	if (ARG(0).type != DATE_TYPE ||
	    ARG(1).type != TIME_TYPE) return E_BAD_TYPE;
	RETVAL = (MINUTES_PER_DAY * ARGV(0)) + ARGV(1);
	return OK;
    case 3:
	if (ARG(0).type != DATE_TYPE ||
	    ARG(1).type != INT_TYPE ||
	    ARG(2).type != INT_TYPE) return E_BAD_TYPE;
	if (ARGV(1) < 0 || ARGV(2) < 0) return E_2LOW;
	if (ARGV(1) > 23 || ARGV(2) > 59) return E_2HIGH;
	RETVAL = (MINUTES_PER_DAY * ARGV(0)) + 60 * ARGV(1) + ARGV(2);
	return OK;
    case 4:
	if (ARG(0).type != INT_TYPE ||
	    ARG(1).type != INT_TYPE ||
	    ARG(2).type != INT_TYPE ||
	    ARG(3).type != TIME_TYPE) return E_BAD_TYPE;
	y = ARGV(0);
	m = ARGV(1) - 1;
	d = ARGV(2);

	if (!DateOK(y, m, d)) return E_BAD_DATE;
	RETVAL = Julian(y, m, d) * MINUTES_PER_DAY + ARGV(3);
	return OK;
    case 5:
	if (ARG(0).type != INT_TYPE ||
	    ARG(1).type != INT_TYPE ||
	    ARG(2).type != INT_TYPE ||
	    ARG(3).type != INT_TYPE ||
	    ARG(4).type != INT_TYPE) return E_BAD_TYPE;

	y = ARGV(0);
	m = ARGV(1) - 1;
	d = ARGV(2);
	if (!DateOK(y, m, d)) return E_BAD_DATE;

	if (ARGV(3) < 0 || ARGV(4) < 0) return E_2LOW;
	if (ARGV(3) > 23 || ARGV(4) > 59) return E_2HIGH;
	RETVAL = Julian(y, m, d) * MINUTES_PER_DAY + ARGV(3) * 60 + ARGV(4);
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
static int FCoerce(func_info *info)
{
    char const *s;

    ASSERT_TYPE(0, STR_TYPE);
    s = ARGSTR(0);

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
static int FMax(func_info *info)
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
static int FMin(func_info *info)
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
static int FAsc(func_info *info)
{
    ASSERT_TYPE(0, STR_TYPE);
    RetVal.type = INT_TYPE;
    RETVAL = *(ARGSTR(0));
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FChar - build a string from ASCII values                   */
/*                                                             */
/***************************************************************/
static int FChar(func_info *info)
{

    int i, len;

/* Special case of one arg - if given ascii value 0, create empty string */
    if (Nargs == 1) {
	ASSERT_TYPE(0, INT_TYPE);
	if (ARGV(0) < -128) return E_2LOW;
	if (ARGV(0) > 255) return E_2HIGH;
	len = ARGV(0) ? 2 : 1;
	RetVal.v.str = malloc(len);
	if (!RetVal.v.str) return E_NO_MEM;
	RetVal.type = STR_TYPE;
	*(RetVal.v.str) = ARGV(0);
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
static int FDay(func_info *info)
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
    RETVAL = d;
    return OK;
}

static int FMonnum(func_info *info)
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
    RETVAL = m+1;
    return OK;
}

static int FYear(func_info *info)
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
    RETVAL = y;
    return OK;
}

static int FWkdaynum(func_info *info)
{
    int v;
    if (!HASDATE(ARG(0))) return E_BAD_TYPE;
    v = DATEPART(ARG(0));

    RetVal.type = INT_TYPE;

    /* Correct so that 0 = Sunday */
    RETVAL = (v+1) % 7;
    return OK;
}

static int FWkday(func_info *info)
{
    char const *s;

    if (!HASDATE(ARG(0)) && ARG(0).type != INT_TYPE) return E_BAD_TYPE;
    if (ARG(0).type == INT_TYPE) {
	if (ARGV(0) < 0) return E_2LOW;
	if (ARGV(0) > 6) return E_2HIGH;
	/* Convert 0=Sun to 0=Mon */
	ARGV(0)--;
	if (ARGV(0) < 0) ARGV(0) = 6;
	s = DayName[ARGV(0)];
    } else s = DayName[DATEPART(ARG(0)) % 7];
    return RetStrVal(s, info);
}

static int FMon(func_info *info)
{
    char const *s;
    int y, m, d, v;

    if (!HASDATE(ARG(0)) && ARG(0).type != INT_TYPE) return E_BAD_TYPE;

    if (ARG(0).type == INT_TYPE) {
	m = ARGV(0) - 1;
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
    return RetStrVal(s, info);
}

/***************************************************************/
/*                                                             */
/*  FHour - extract hour from a time                           */
/*  FMinute - extract minute from a time                       */
/*  FTime - create a time from hour and minute                 */
/*                                                             */
/***************************************************************/
static int FHour(func_info *info)
{
    int v;
    if (!HASTIME(ARG(0))) return E_BAD_TYPE;
    v = TIMEPART(ARG(0));
    RetVal.type = INT_TYPE;
    RETVAL = v / 60;
    return OK;
}

static int FMinute(func_info *info)
{
    int v;
    if (!HASTIME(ARG(0))) return E_BAD_TYPE;
    v = TIMEPART(ARG(0));
    RetVal.type = INT_TYPE;
    RETVAL = v % 60;
    return OK;
}

static int FTime(func_info *info)
{
    int h, m;

    if (ARG(0).type != INT_TYPE || ARG(1).type != INT_TYPE) return E_BAD_TYPE;

    h = ARGV(0);
    m = ARGV(1);
    if (h<0 || m<0) return E_2LOW;
    if (h>23 || m>59) return E_2HIGH;
    RetVal.type = TIME_TYPE;
    RETVAL = h*60 + m;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FAbs - absolute value                                      */
/*  FSgn - signum function                                     */
/*                                                             */
/***************************************************************/
static int FAbs(func_info *info)
{
    int v;

    ASSERT_TYPE(0, INT_TYPE);
    v = ARGV(0);
    RetVal.type = INT_TYPE;
    RETVAL = (v < 0) ? (-v) : v;
    return OK;
}

static int FSgn(func_info *info)
{
    int v;

    ASSERT_TYPE(0, INT_TYPE);
    v = ARGV(0);
    RetVal.type = INT_TYPE;
    if (v>0) RETVAL = 1;
    else if (v<0) RETVAL = -1;
    else RETVAL = 0;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FOrd - returns a string containing ordinal number.         */
/*                                                             */
/*  EG - ord(2) == "2nd", etc.                                 */
/*                                                             */
/***************************************************************/
static int FOrd(func_info *info)
{
    int t, u, v;
    char const *s;

    char buf[32];

    ASSERT_TYPE(0, INT_TYPE);

    v = ARGV(0);
    t = v % 100;
    if (t < 0) t = -t;
    u = t % 10;
    s = "th";
    if (u == 1 && t != 11) s = "st";
    if (u == 2 && t != 12) s = "nd";
    if (u == 3 && t != 13) s = "rd";
    sprintf(buf, "%d%s", v, s);
    return RetStrVal(buf, info);
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
static int FPlural(func_info *info)
{
    ASSERT_TYPE(0, INT_TYPE);

    switch(Nargs) {
    case 1:
	if (ARGV(0) == 1) return RetStrVal("", info);
	else return RetStrVal("s", info);

    case 2:
	ASSERT_TYPE(1, STR_TYPE);
	if (ARGV(0) == 1) {
	    DCOPYVAL(RetVal, ARG(1));
	    return OK;
	}
	RetVal.type = STR_TYPE;
	RetVal.v.str = malloc(strlen(ARGSTR(1))+2);
	if (!RetVal.v.str) {
	    RetVal.type = ERR_TYPE;
	    return E_NO_MEM;
	}
	strcpy(RetVal.v.str, ARGSTR(1));
	strcat(RetVal.v.str, "s");
	return OK;

    default:
	if (ARG(1).type != STR_TYPE || ARG(2).type != STR_TYPE)
	    return E_BAD_TYPE;
	if (ARGV(0) == 1) DCOPYVAL(RetVal, ARG(1));
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
static int FChoose(func_info *info)
{
    int v;

    ASSERT_TYPE(0, INT_TYPE);
    v = ARGV(0);
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
static int FVersion(func_info *info)
{
    return RetStrVal(VERSION, info);
}

/***************************************************************/
/*                                                             */
/*  FOstype - the type of operating system                     */
/*  (UNIX, OS/2, or MSDOS)                                     */
/*                                                             */
/***************************************************************/
static int FOstype(func_info *info)
{
    return RetStrVal("UNIX", info);
}

/***************************************************************/
/*                                                             */
/*  FUpper - convert string to upper-case                      */
/*  FLower - convert string to lower-case                      */
/*                                                             */
/***************************************************************/
static int FUpper(func_info *info)
{
    char *s;

    ASSERT_TYPE(0, STR_TYPE);
    DCOPYVAL(RetVal, ARG(0));
    s = RetVal.v.str;
    while (*s) {
	*s = UPPER(*s);
	s++;
    }
    return OK;
}

static int FLower(func_info *info)
{
    char *s;

    ASSERT_TYPE(0, STR_TYPE);
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
static int FToday(func_info *info)
{
    RetVal.type = DATE_TYPE;
    RETVAL = JulianToday;
    return OK;
}

static int FRealtoday(func_info *info)
{
    RetVal.type = DATE_TYPE;
    RETVAL = RealToday;
    return OK;
}

static int FNow(func_info *info)
{
    RetVal.type = TIME_TYPE;
    RETVAL = (int) ( SystemTime(0) / 60L );
    return OK;
}

static int FRealnow(func_info *info)
{
    RetVal.type = TIME_TYPE;
    RETVAL = (int) ( SystemTime(1) / 60L );
    return OK;
}

static int FCurrent(func_info *info)
{
    RetVal.type = DATETIME_TYPE;
    RETVAL = JulianToday * MINUTES_PER_DAY + (SystemTime(0) / 60);
    return OK;
}

static int FRealCurrent(func_info *info)
{
    RetVal.type = DATETIME_TYPE;
    RETVAL = RealToday * MINUTES_PER_DAY + (SystemTime(1) / 60);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FGetenv - get the value of an environment variable.        */
/*                                                             */
/***************************************************************/
static int FGetenv(func_info *info)
{
    ASSERT_TYPE(0, STR_TYPE);
    return RetStrVal(getenv(ARGSTR(0)), info);
}

/***************************************************************/
/*                                                             */
/*  FValue                                                     */
/*                                                             */
/*  Get the value of a variable.  If a second arg is supplied, */
/*  it is returned if variable is undefined.                   */
/*                                                             */
/***************************************************************/
static int FValue(func_info *info)
{
    Var *v;

    ASSERT_TYPE(0, STR_TYPE);
    switch(Nargs) {
    case 1:
	return GetVarValue(ARGSTR(0), &RetVal, NULL, NULL);

    case 2:
	v = FindVar(ARGSTR(0), 0);
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
static int FDefined(func_info *info)
{
    ASSERT_TYPE(0, STR_TYPE);

    RetVal.type = INT_TYPE;

    if (FindVar(ARGSTR(0), 0))
	RETVAL = 1;
    else
	RETVAL = 0;
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
static int FTrigdate(func_info *info)
{
    if (LastTrigValid) {
	RetVal.type = DATE_TYPE;
	RETVAL = LastTriggerDate;
    } else {
	RetVal.type = INT_TYPE;
	RETVAL = 0;
    }
    return OK;
}

static int FTrigvalid(func_info *info)
{
    RetVal.type = INT_TYPE;
    RETVAL = LastTrigValid;
    return OK;
}

static int FTrigtime(func_info *info)
{
    if (LastTriggerTime != NO_TIME) {
	RetVal.type = TIME_TYPE;
	RETVAL = LastTriggerTime;
    } else {
	RetVal.type = INT_TYPE;
	RETVAL = 0;
    }
    return OK;
}

static int FTrigdatetime(func_info *info)
{
    if (!LastTrigValid) {
	RetVal.type = INT_TYPE;
	RETVAL = 0;
    } else if (LastTriggerTime != NO_TIME) {
	RetVal.type = DATETIME_TYPE;
	RETVAL = LastTriggerDate * MINUTES_PER_DAY + LastTriggerTime;
    } else {
	RetVal.type = DATE_TYPE;
	RETVAL = LastTriggerDate;
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
static int FDaysinmon(func_info *info)
{
    if (ARG(0).type != INT_TYPE || ARG(1).type != INT_TYPE) return E_BAD_TYPE;

    if (ARGV(0) > 12 || ARGV(0) < 1 ||
	ARGV(1) < BASE || ARGV(1) > BASE+YR_RANGE)
	return E_DOMAIN_ERR;

    RetVal.type = INT_TYPE;
    RETVAL = DaysInMonth(ARGV(0)-1, ARGV(1));
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FIsleap                                                    */
/*                                                             */
/*  Return 1 if year is a leap year, zero otherwise.           */
/*                                                             */
/***************************************************************/
static int FIsleap(func_info *info)
{
    int y, m, d;

    if (ARG(0).type != INT_TYPE && !HASDATE(ARG(0))) return E_BAD_TYPE;

    /* If it's a date, extract the year */
    if (HASDATE(ARG(0)))
	FromJulian(DATEPART(ARG(0)), &y, &m, &d);
    else
	y = ARGV(0);

    RetVal.type = INT_TYPE;
    RETVAL = IsLeapYear(y);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FTrigger                                                   */
/*                                                             */
/*  Put out a date in a format suitable for triggering.        */
/*                                                             */
/***************************************************************/
static int FTrigger(func_info *info)
{
    int y, m, d;
    int date, tim;
    char buf[128];

    tim = NO_TIME;
    if (ARG(0).type != DATE_TYPE &&
	ARG(0).type != DATETIME_TYPE) return E_BAD_TYPE;

    if (ARG(0).type == DATE_TYPE) {
	date = ARGV(0);
    } else {
	date = ARGV(0) / MINUTES_PER_DAY;
	tim = ARGV(0) % MINUTES_PER_DAY;
    }

    if (ARG(0).type == DATE_TYPE) {
	if (Nargs > 2) {
	    /* Date Time UTCFlag */
	    if (ARG(0).type == DATETIME_TYPE) return E_BAD_TYPE;
	    ASSERT_TYPE(2, INT_TYPE);
	    ASSERT_TYPE(1, TIME_TYPE);
	    tim = ARGV(1);
	    if (ARGV(2)) {
		UTCToLocal(date, tim, &date, &tim);
	    }
	} else if (Nargs > 1) {
	    /* Date Time */
	    ASSERT_TYPE(1, TIME_TYPE);
	    tim = ARGV(1);
	}
    } else {
	if (Nargs > 2) {
	    return E_2MANY_ARGS;
	} else if (Nargs > 1) {
	    /* DateTime UTCFlag */
	    ASSERT_TYPE(1, INT_TYPE);
	    if (ARGV(1)) {
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
    return RetStrVal(buf, info);
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
static int FShell(func_info *info)
{
    DynamicBuffer buf;
    int ch, r;
    FILE *fp;

    /* For compatibility with previous versions of Remind, which
       used a static buffer for reading results from shell() command */
    int maxlen = 511;

    DBufInit(&buf);
    if (RunDisabled) return E_RUN_DISABLED;
    ASSERT_TYPE(0, STR_TYPE);
    if (Nargs >= 2) {
	ASSERT_TYPE(1, INT_TYPE);
	maxlen = ARGV(1);
    }
    fp = popen(ARGSTR(0), "r");
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

    /* XXX Should we consume remaining output from cmd? */

    pclose(fp);
    r = RetStrVal(DBufValue(&buf), info);
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
static int FIsomitted(func_info *info)
{
    int r;
    if (!HASDATE(ARG(0))) return E_BAD_TYPE;

    RetVal.type = INT_TYPE;
    r = IsOmitted(DATEPART(ARG(0)), 0, NULL, &RETVAL);
    return r;
}

/***************************************************************/
/*                                                             */
/*  FSubstr                                                    */
/*                                                             */
/*  The substr function.  We destroy the value on the stack.   */
/*                                                             */
/***************************************************************/
static int FSubstr(func_info *info)
{
    char *s;
    char const *t;
    int start, end;

    if (ARG(0).type != STR_TYPE || ARG(1).type != INT_TYPE) return E_BAD_TYPE;
    if (Nargs == 3 && ARG(2).type != INT_TYPE) return E_BAD_TYPE;

    s = ARGSTR(0);
    start = 1;
    while (start < ARGV(1)) {
	if (!*s) break;
	s++;
	start++;
    }
    if (Nargs == 2 || !*s) return RetStrVal(s, info);
    end = start;
    t = s;
    while (end <= ARGV(2)) {
	if (!*s) break;
	s++;
	end++;
    }
    *s = 0;
    return RetStrVal(t, info);
}

/***************************************************************/
/*                                                             */
/*  FIndex                                                     */
/*                                                             */
/*  The index of one string embedded in another.               */
/*                                                             */
/***************************************************************/
static int FIndex(func_info *info)
{
    char const *s;
    int start;

    if (ARG(0).type != STR_TYPE || ARG(1).type != STR_TYPE ||
	(Nargs == 3 && ARG(2).type != INT_TYPE)) return E_BAD_TYPE;

    s = ARGSTR(0);

/* If 3 args, bump up the start */
    if (Nargs == 3) {
	start = 1;
	while (start < ARGV(2)) {
	    if (!*s) break;
	    s++;
	    start++;
	}
    }

/* Find the string */
    s = strstr(s, ARGSTR(1));
    RetVal.type = INT_TYPE;
    if (!s) {
	RETVAL = 0;
	return OK;
    }
    RETVAL = (s - ARGSTR(0)) + 1;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FIif                                                       */
/*                                                             */
/*  The IIF function.                                          */
/*                                                             */
/***************************************************************/
static int FIif(func_info *info)
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
static int FFilename(func_info *info)
{
    return RetStrVal(FileName, info);
}

/***************************************************************/
/*                                                             */
/*  FFiledir                                                   */
/*                                                             */
/*  Return directory of current file                           */
/*                                                             */
/***************************************************************/
static int FFiledir(func_info *info)
{
    char *s;
    DynamicBuffer buf;
    int r;

    DBufInit(&buf);

    if (DBufPuts(&buf, FileName) != OK) return E_NO_MEM;
    if (DBufLen(&buf) == 0) {
	DBufFree(&buf);
	return RetStrVal(".", info);
    }

    s = DBufValue(&buf) + DBufLen(&buf) - 1;
    while (s > DBufValue(&buf) && *s != '/') s--;
    if (*s == '/') {
	*s = 0;
	r = RetStrVal(DBufValue(&buf), info);
    } else r = RetStrVal(".", info);
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
static int FAccess(func_info *info)
{
    int amode;
    char const *s;

    if (ARG(0).type != STR_TYPE ||
	(ARG(1).type != INT_TYPE && ARG(1).type != STR_TYPE)) return E_BAD_TYPE;

    if (ARG(1).type == INT_TYPE) amode = ARGV(1);
    else {
	amode = 0;
	s = ARGSTR(1);
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
    RETVAL = access(ARGSTR(0), amode);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FTypeof                                                    */
/*                                                             */
/*  Implement the typeof() function.                           */
/*                                                             */
/***************************************************************/
static int FTypeof(func_info *info)
{
    switch(ARG(0).type) {
    case INT_TYPE:  return RetStrVal("INT", info);
    case DATE_TYPE: return RetStrVal("DATE", info);
    case TIME_TYPE:  return RetStrVal("TIME", info);
    case STR_TYPE:  return RetStrVal("STRING", info);
    case DATETIME_TYPE: return RetStrVal("DATETIME", info);
    default:        return RetStrVal("ERR", info);
    }
}

/***************************************************************/
/*                                                             */
/*  FLanguage                                                  */
/*                                                             */
/*  Implement the language() function.                         */
/*                                                             */
/***************************************************************/
static int FLanguage(func_info *info)
{
    return RetStrVal(L_LANGNAME, info);
}

/***************************************************************/
/*                                                             */
/*  FArgs                                                      */
/*                                                             */
/*  Implement the args() function.                             */
/*                                                             */
/***************************************************************/
static int FArgs(func_info *info)
{
    ASSERT_TYPE(0, STR_TYPE);
    RetVal.type = INT_TYPE;
    RETVAL = UserFuncExists(ARGSTR(0));
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FDosubst                                                   */
/*                                                             */
/*  Implement the dosubst() function.                          */
/*                                                             */
/***************************************************************/
static int FDosubst(func_info *info)
{
    int jul, tim, r;
    DynamicBuffer buf;

    DBufInit(&buf);

    jul = NO_DATE;
    tim = NO_TIME;
    ASSERT_TYPE(0, STR_TYPE);
    if (Nargs >= 2) {
	if (ARG(1).type == DATETIME_TYPE) {
	    jul = DATEPART(ARG(1));
	    tim = TIMEPART(ARG(1));
	} else {
	    ASSERT_TYPE(1, DATE_TYPE);
	    jul = ARGV(1);
	}
	if (Nargs >= 3) {
	    if (ARG(1).type == DATETIME_TYPE) {
		return E_2MANY_ARGS;
	    }
	    ASSERT_TYPE(2, TIME_TYPE);
	    tim = ARGV(2);
	}
    }

    if ((r=DoSubstFromString(ARGSTR(0), &buf, jul, tim))) return r;
    r = RetStrVal(DBufValue(&buf), info);
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
static int FHebdate(func_info *info)
{
    int year, day, mon, jahr;
    int mout, dout;
    int ans, r;
    int adarbehave;

    if (ARG(0).type != INT_TYPE || ARG(1).type != STR_TYPE) return E_BAD_TYPE;
    day = ARGV(0);
    mon = HebNameToNum(ARGSTR(1));
    if (mon < 0) return E_BAD_HEBDATE;
    if (Nargs == 2) {
	r = GetNextHebrewDate(JulianToday, mon, day, 0, 0, &ans);
	if (r) return r;
	RetVal.type = DATE_TYPE;
	RETVAL = ans;
	return OK;
    }
    if (Nargs == 5) {
	ASSERT_TYPE(4, INT_TYPE);
	adarbehave = ARGV(4);
	if (adarbehave < 0) return E_2LOW;
	if (adarbehave > 2) return E_2HIGH;
    } else adarbehave = 0;

    if (Nargs >= 4) {
	ASSERT_TYPE(3, INT_TYPE);
	jahr = ARGV(3);
	if (jahr < 0) return E_2LOW;
	if (jahr > 2) {
	    r = ComputeJahr(jahr, mon, day, &jahr);
	    if (r) return r;
	}
    } else jahr = 0;


    if (ARG(2).type == INT_TYPE) {
	year = ARGV(2);
	r = GetValidHebDate(year, mon, day, 0, &mout, &dout, jahr);
	if (r) return r;
	r = HebToJul(year, mout, dout);
	if (r<0) return E_DATE_OVER;
	RETVAL = r;
	RetVal.type = DATE_TYPE;
	return OK;
    } else if (HASDATE(ARG(2))) {
	r = GetNextHebrewDate(DATEPART(ARG(2)), mon, day, jahr, adarbehave, &ans);
	if (r) return r;
	RETVAL = ans;
	RetVal.type = DATE_TYPE;
	return OK;
    } else return E_BAD_TYPE;
}

static int FHebday(func_info *info)
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
    RETVAL = d;
    return OK;
}

static int FHebmon(func_info *info)
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
    return RetStrVal(HebMonthName(m, y), info);
}

static int FHebyear(func_info *info)
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
    RETVAL = y;
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
static int FEasterdate(func_info *info)
{
    int y, m, d;
    int g, c, x, z, e, n;
    if (ARG(0).type == INT_TYPE) {
	y = ARGV(0);
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
	RETVAL = Julian(y, m, d);
	y++; } while (HASDATE(ARG(0)) && RETVAL < DATEPART(ARG(0)));

    return OK;
}
/***************************************************************/
/*                                                             */
/*  FIsdst and FMinsfromutc                                    */
/*                                                             */
/*  Check whether daylight saving time is in effect, and      */
/*  get minutes from UTC.                                      */
/*                                                             */
/***************************************************************/
static int FTimeStuff (int wantmins, func_info *info);
static int FIsdst(func_info *info)
{
    return FTimeStuff(0, info);
}

static int FMinsfromutc(func_info *info)
{
    return FTimeStuff(1, info);
}

static int FTimeStuff(int wantmins, func_info *info)
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
	    ASSERT_TYPE(1, TIME_TYPE);
	    tim = ARGV(1);
	}
    }

    if (CalcMinsFromUTC(jul, tim, &mins, &dst)) return E_MKTIME_PROBLEM;
    RetVal.type = INT_TYPE;
    if (wantmins) RETVAL = mins; else RETVAL = dst;

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
    int mins, hours;
    int year, mon, day;

    double M, L, sinDelta, cosDelta, a, a_hr, cosH, t, H, T;
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

    if (rise > 1)
	rise -= 2;

/* Following formula on page B6 exactly... */
    t = (double) jul;
    if (rise) {
	t += (6.0 + longdeg/15.0) / 24.0;
    } else {
	t += (18.0 + longdeg/15.0) / 24.0;
    }

/* Mean anomaly of sun starting from 1 Jan 1990 */
/* NOTE: This assumes that BASE = 1990!!! */
#if BASE != 1990
#error Sun calculations assume a BASE of 1990!
#endif
    t = 0.9856002585 * t;
    M = t + 357.828757; /* In degrees */

    /* Make sure M is in the range [0, 360) */
    M -= (floor(M/360.0) * 360.0);

/* Sun's true longitude */
    L = M + 1.916*sin(DEGRAD*M) + 0.02*sin(2*DEGRAD*M) + 283.07080214;
    if (L > 360.0) L -= 360.0;

/* Tan of sun's right ascension */
    a = RADDEG * atan2(0.91746*sin(DEGRAD*L), cos(DEGRAD*L));
    if (a<0) {
	a += 360.0;
    }

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

    t -= 360.0*floor(t/360.0);
    T = (H-t) / 15.0 + a_hr - 6.726637276;

    if (T >= 24.0) T -= 24.0;
    else if (T < 0.0) T+= 24.0;
    
    UT = T + longdeg / 15.0;
    

    local = UT + (double) mins / 60.0;
    if (local < 0.0) local += 24.0;
    else if (local >= 24.0) local -= 24.0;

    /* Round off local time to nearest minute */
    local = floor(local * 60.0 + 0.5) / 60.0;

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
static int FSun(int rise, func_info *info)
{
    int jul = JulianToday;
    double cosz = -0.014543897;  /* for sunrise and sunset */
    int r;

    /* Civil twilight: cos(96 degrees) */
    if (rise == 2 || rise == 3) {
	cosz = -0.104528463268;
    }
    if (Nargs >= 1) {
	if (!HASDATE(ARG(0))) return E_BAD_TYPE;
	jul = DATEPART(ARG(0));
    }

    r = SunStuff(rise, cosz, jul);
    if (r == NO_TIME) {
	RETVAL = 0;
	RetVal.type = INT_TYPE;
    } else if (r == -NO_TIME) {
	RETVAL = MINUTES_PER_DAY;
	RetVal.type = INT_TYPE;
    } else {
	RETVAL = r;
	RetVal.type = TIME_TYPE;
    }
    return OK;
}

static int FSunrise(func_info *info)
{
    return FSun(1, info);
}
static int FSunset(func_info *info)
{
    return FSun(0, info);
}

static int FDawn(func_info *info)
{
    return FSun(3, info);
}
static int FDusk(func_info *info)
{
    return FSun(2, info);
}

/***************************************************************/
/*                                                             */
/*  FFiledate                                                  */
/*                                                             */
/*  Return modification date of a file                         */
/*                                                             */
/***************************************************************/
static int FFiledate(func_info *info)
{
    struct stat statbuf;
    struct tm *t1;

    RetVal.type = DATE_TYPE;

    ASSERT_TYPE(0, STR_TYPE);

    if (stat(ARGSTR(0), &statbuf)) {
	RETVAL = 0;
	return OK;
    }

    t1 = localtime(&(statbuf.st_mtime));

    if (t1->tm_year + 1900 < BASE)
	RETVAL=0;
    else
	RETVAL=Julian(t1->tm_year+1900, t1->tm_mon, t1->tm_mday);

    return OK;
}

/***************************************************************/
/*                                                             */
/*  FFiledatetime                                              */
/*                                                             */
/*  Return modification datetime of a file                     */
/*                                                             */
/***************************************************************/
static int FFiledatetime(func_info *info)
{
    struct stat statbuf;
    struct tm *t1;

    RetVal.type = DATETIME_TYPE;

    ASSERT_TYPE(0, STR_TYPE);

    if (stat(ARGSTR(0), &statbuf)) {
	RETVAL = 0;
	return OK;
    }

    t1 = localtime(&(statbuf.st_mtime));

    if (t1->tm_year + 1900 < BASE)
	RETVAL=0;
    else
	RETVAL = MINUTES_PER_DAY * Julian(t1->tm_year+1900, t1->tm_mon, t1->tm_mday) + t1->tm_hour * 60 + t1->tm_min;

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
static int FPsshade(func_info *info)
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
	sprintf(s, "_A BoxHeight _A sub lineto closepath %d 100 div setgray fill 0.0 setgray", ARGV(0));
    } else {
	sprintf(s, "_A BoxHeight _A sub lineto closepath %d 100 div %d 100 div %d 100 div setrgbcolor fill 0.0 setgray", ARGV(0), ARGV(1), ARGV(2));
    }
    return RetStrVal(psbuff, info);
}

/***************************************************************/
/*                                                             */
/*  FPsmoon                                                    */
/*                                                             */
/*  Canned PostScript code for generating moon phases          */
/*                                                             */
/***************************************************************/
static int psmoon_warned = 0;

static int FPsmoon(func_info *info)
{
    char psbuff[512];
    char sizebuf[30];
    char fontsizebuf[30];
    char *s = psbuff;
    char const *extra = NULL;
    int size = -1;
    int fontsize = -1;

    ASSERT_TYPE(0, INT_TYPE);
    if (ARGV(0) < 0) return E_2LOW;
    if (ARGV(0) > 3) return E_2HIGH;
    if (Nargs > 1) {
	ASSERT_TYPE(1, INT_TYPE);
	if (ARGV(1) < -1) return E_2LOW;
	size = ARGV(1);
	if (Nargs > 2) {
	    ASSERT_TYPE(2, STR_TYPE);
	    extra = ARGSTR(2);
	    if (Nargs > 3) {
		ASSERT_TYPE(3, INT_TYPE);
		if (ARGV(3) <= 0) return E_2LOW;
		fontsize = ARGV(3);
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
    switch(ARGV(0)) {
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
    return RetStrVal(psbuff, info);
}

/***************************************************************/
/*                                                             */
/*  FMoonphase                                                 */
/*                                                             */
/*  Phase of moon for specified date/time.                     */
/*                                                             */
/***************************************************************/
static int FMoonphase(func_info *info)
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
	date = ARGV(0);
	time = ARGV(1);
	break;

    default: return E_SWERR;
    }

    RetVal.type = INT_TYPE;
    RETVAL = MoonPhase(date, time);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FMoondate                                                  */
/*                                                             */
/*  Hunt for next occurrence of specified moon phase           */
/*                                                             */
/***************************************************************/
static int MoonStuff (int want_time, func_info *info);
static int FMoondate(func_info *info)
{
    return MoonStuff(DATE_TYPE, info);
}

static int FMoontime(func_info *info)
{
    return MoonStuff(TIME_TYPE, info);
}

static int FMoondatetime(func_info *info)
{
    return MoonStuff(DATETIME_TYPE, info);
}

static int MoonStuff(int type_wanted, func_info *info)
{
    int startdate, starttim;
    int d, t;

    startdate = JulianToday;
    starttim = 0;

    ASSERT_TYPE(0, INT_TYPE);
    if (ARGV(0) < 0) return E_2LOW;
    if (ARGV(0) > 3) return E_2HIGH;
    if (Nargs >= 2) {
	if (!HASDATE(ARG(1))) return E_BAD_TYPE;
	startdate = DATEPART(ARG(1));
	if (HASTIME(ARG(1))) {
		starttim = TIMEPART(ARG(1));
	}

	if (Nargs >= 3) {
	    if (HASTIME(ARG(1))) return E_2MANY_ARGS;
	    ASSERT_TYPE(2, TIME_TYPE);
	    starttim = ARGV(2);
	}
    }

    HuntPhase(startdate, starttim, ARGV(0), &d, &t);
    RetVal.type = type_wanted;
    switch(type_wanted) {
    case TIME_TYPE:
	RETVAL = t;
	break;
    case DATE_TYPE:
	RETVAL = d;
	break;
    case DATETIME_TYPE:
	RETVAL = d * MINUTES_PER_DAY + t;
	break;
    default:
	return E_BAD_TYPE;
    }
    return OK;
}

static int FTimepart(func_info *info)
{
    if (!HASTIME(ARG(0))) return E_BAD_TYPE;
    RetVal.type = TIME_TYPE;
    RETVAL = TIMEPART(ARG(0));
    return OK;
}

static int FDatepart(func_info *info)
{
    if (!HASDATE(ARG(0))) return E_BAD_TYPE;
    RetVal.type = DATE_TYPE;
    RETVAL = DATEPART(ARG(0));
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

static int FTzconvert(func_info *info)
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
		       ARGSTR(1), NULL, &tm);
    } else {
	r = tz_convert(year, month, day, hour, minute,
		       ARGSTR(1), ARGSTR(2), &tm);
    }

    if (r == -1) return E_CANT_CONVERT_TZ;

    jul = Julian(tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
    tim = tm.tm_hour * 60 + tm.tm_min;
    RetVal.type = DATETIME_TYPE;
    RETVAL = jul * MINUTES_PER_DAY + tim;
    return OK;
}

static int
FSlide(func_info *info)
{
    int r, omit, d, i, localomit, amt;
    Token tok;

    if (!HASDATE(ARG(0))) return E_BAD_TYPE;
    ASSERT_TYPE(1, INT_TYPE);

    d = DATEPART(ARG(0));
    amt = ARGV(1);
    if (amt > 1000000) return E_2HIGH;
    if (amt < -1000000) return E_2LOW;

    localomit = 0;
    for (i=2; i<Nargs; i++) {
	if (ARG(i).type != STR_TYPE) return E_BAD_TYPE;
	FindToken(ARG(i).v.str, &tok);
	if (tok.type != T_WkDay) return E_UNKNOWN_TOKEN;
	localomit |= (1 << tok.val);
    }

    /* If ALL weekdays are omitted... barf! */
    if (localomit == 127 && amt != 0) return E_2MANY_LOCALOMIT;
    if (amt > 0) {
	while(amt) {
	    d++;
	    r = IsOmitted(d, localomit, NULL,&omit);
	    if (r) return r;
	    if (!omit) amt--;
	}
    } else {
	while(amt) {
	    d--;
	    r = IsOmitted(d, localomit, NULL,&omit);
	    if (r) return r;
	    if (!omit) amt++;
	}
    }
    RetVal.type = DATE_TYPE;
    RETVAL = d;
    return OK;
}

static int
FNonomitted(func_info *info)
{
    int d1, d2, ans, localomit, i;
    int omit, r;
    Token tok;

    if (!HASDATE(ARG(0)) ||
	!HASDATE(ARG(1))) {
	return E_BAD_TYPE;
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
	r = IsOmitted(d1++, localomit, NULL, &omit);
	if (r) return r;
	if (!omit) {
	    ans++;
	}
    }
    RetVal.type = INT_TYPE;
    RETVAL = ans;
    return OK;
}

static int
FWeekno(func_info *info)
{
    int jul = JulianToday;
    int wkstart = 0; /* Week start on Monday */
    int daystart = 29; /* First week starts on wkstart on or after Dec. 29 */
    int monstart;
    int candidate;

    int y, m, d;

    if (Nargs >= 1) {
	if (!HASDATE(ARG(0))) return E_BAD_TYPE;
	jul = DATEPART(ARG(0));
    }
    if (Nargs >= 2) {
	ASSERT_TYPE(1, INT_TYPE);
	if (ARGV(1) < 0) return E_2LOW;
	if (ARGV(1) > 6) return E_2HIGH;
	wkstart = ARGV(1);
	/* Convert 0=Sun to 0=Mon */
	wkstart--;
	if (wkstart < 0) wkstart = 6;
	if (Nargs >= 3) {
	    ASSERT_TYPE(2, INT_TYPE);
	    if (ARGV(2) < 1) return E_2LOW;
	    if (ARGV(2) > 31) return E_2HIGH;
	    daystart = ARGV(2);
	}
    }

    RetVal.type = INT_TYPE;
    /* If start day is 7, first week starts after Jan,
       otherwise after Dec. */
    if (daystart <= 7) {
	monstart = 0;
    } else {
	monstart = 11;
    }

    FromJulian(jul, &y, &m, &d);

    /* Try this year */
    candidate = Julian(y, monstart, daystart);
    while((candidate % 7) != wkstart) candidate++;

    if (candidate <= jul) {
	RETVAL = ((jul - candidate) / 7) + 1;
	return OK;
    }

    if (y-1 < BASE) return E_DATE_OVER;
    /* Must be last year */
    candidate = Julian(y-1, monstart, daystart);
    while((candidate % 7) != wkstart) candidate++;
    if (candidate <= jul) {
	RETVAL = ((jul - candidate) / 7) + 1;
	return OK;
    }

    if (y-2 < BASE) return E_DATE_OVER;
    /* Holy cow! */
    candidate = Julian(y-2, monstart, daystart);
    while((candidate % 7) != wkstart) candidate++;
    RETVAL = ((jul - candidate) / 7) + 1;
    return OK;
}

static int
FEvalTrig(func_info *info)
{
    Parser p;
    Trigger trig;
    TimeTrig tim;
    int jul, scanfrom;
    int r;

    ASSERT_TYPE(0, STR_TYPE);
    if (Nargs >= 2) {
	if (!HASDATE(ARG(1))) return E_BAD_TYPE;
	scanfrom = DATEPART(ARG(1));
    } else {
	scanfrom = NO_DATE;
    }

    CreateParser(ARGSTR(0), &p);
    p.allownested = 0;
    r = ParseRem(&p, &trig, &tim, 0);
    if (r) return r;
    if (trig.typ != NO_TYPE) {
	FreeTrig(&trig);
	return E_PARSE_ERR;
    }
    if (scanfrom == NO_DATE) {
	jul = ComputeTrigger(trig.scanfrom, &trig, &r, 0);
    } else {
	/* Hokey... */
	if (trig.scanfrom != JulianToday) {
	    Eprint("Warning: SCANFROM is ignored in two-argument form of evaltrig()");
	}
	jul = ComputeTrigger(scanfrom, &trig, &r, 0);
    }
    FreeTrig(&trig);
    if (r) return r;
    if (jul < 0) {
	RetVal.type = INT_TYPE;
	RETVAL = jul;
    } else if (tim.ttime == NO_TIME) {
	RetVal.type = DATE_TYPE;
	RETVAL = jul;
    } else {
	RetVal.type = DATETIME_TYPE;
	RETVAL = (MINUTES_PER_DAY * jul) + tim.ttime;
    }
    return OK;
}
