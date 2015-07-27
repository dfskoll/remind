/***************************************************************/
/*                                                             */
/*  VAR.C                                                      */
/*                                                             */
/*  This file contains routines, structures, etc for           */
/*  user- and system-defined variables.                        */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by Dianne Skoll                    */
/*  Copyright (C) 1999-2007 by Roaring Penguin Software Inc.   */
/*                                                             */
/***************************************************************/

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <stdlib.h>
#include "types.h"
#include "expr.h"
#include "globals.h"
#include "protos.h"
#include "err.h"

#define UPPER(c) (islower(c) ? toupper(c) : c)

/* The variable hash table */
#define VAR_HASH_SIZE 64
#define VARIABLE ErrMsg[E_VAR]
#define VALUE    ErrMsg[E_VAL]
#define UNDEF	 ErrMsg[E_UNDEF]

static Var *VHashTbl[VAR_HASH_SIZE];

typedef int (*SysVarFunc)(int, Value *);

static int trig_date_func(int do_set, Value *val)
{
    val->type = DATE_TYPE;
    if (!LastTrigValid) {
	val->v.val = 0;
    } else {
	val->v.val = LastTriggerDate;
    }
    return OK;
}
static int trig_day_func(int do_set, Value *val)
{
    int y, m, d;
    val->type = INT_TYPE;
    if (!LastTrigValid) {
	val->v.val = -1;
	return OK;
    }

    FromJulian(LastTriggerDate, &y, &m, &d);
    val->v.val = d;
    return OK;
}

static int trig_mon_func(int do_set, Value *val)
{
    int y, m, d;
    val->type = INT_TYPE;
    if (!LastTrigValid) {
	val->v.val = -1;
	return OK;
    }

    FromJulian(LastTriggerDate, &y, &m, &d);
    val->v.val = m+1;
    return OK;
}

static int trig_year_func(int do_set, Value *val)
{
    int y, m, d;
    val->type = INT_TYPE;
    if (!LastTrigValid) {
	val->v.val = -1;
	return OK;
    }

    FromJulian(LastTriggerDate, &y, &m, &d);
    val->v.val = y;
    return OK;
}

static int trig_wday_func(int do_set, Value *val)
{
    val->type = INT_TYPE;
    if (!LastTrigValid) {
	val->v.val = -1;
	return OK;
    }

    val->v.val = (LastTriggerDate + 1) % 7;
    return OK;
}

static int today_date_func(int do_set, Value *val)
{
    val->type = DATE_TYPE;
    val->v.val = JulianToday;
    return OK;
}
static int today_day_func(int do_set, Value *val)
{
    int y, m, d;
    val->type = INT_TYPE;
    FromJulian(JulianToday, &y, &m, &d);
    val->v.val = d;
    return OK;
}

static int today_mon_func(int do_set, Value *val)
{
    int y, m, d;
    val->type = INT_TYPE;
    FromJulian(JulianToday, &y, &m, &d);
    val->v.val = m+1;
    return OK;
}

static int today_year_func(int do_set, Value *val)
{
    int y, m, d;
    val->type = INT_TYPE;
    FromJulian(JulianToday, &y, &m, &d);
    val->v.val = y;
    return OK;
}

static int today_wday_func(int do_set, Value *val)
{
    val->type = INT_TYPE;
    val->v.val = (JulianToday + 1) % 7;
    return OK;
}

static int date_sep_func(int do_set, Value *val)
{
    if (!do_set) {
	val->v.str = malloc(2);
	if (!val->v.str) return E_NO_MEM;
	val->v.str[0] = DateSep;
	val->v.str[1] = 0;
	val->type = STR_TYPE;
	return OK;
    }
    if (val->type != STR_TYPE) return E_BAD_TYPE;
    if (strcmp(val->v.str, "/") &&
	strcmp(val->v.str, "-")) {
	return E_BAD_TYPE;
    }
    DateSep = val->v.str[0];
    return OK;
}

static int time_sep_func(int do_set, Value *val)
{
    if (!do_set) {
	val->v.str = malloc(2);
	if (!val->v.str) return E_NO_MEM;
	val->v.str[0] = TimeSep;
	val->v.str[1] = 0;
	val->type = STR_TYPE;
	return OK;
    }
    if (val->type != STR_TYPE) return E_BAD_TYPE;
    if (strcmp(val->v.str, ":") &&
	strcmp(val->v.str, ".")) {
	return E_BAD_TYPE;
    }
    TimeSep = val->v.str[0];
    return OK;
}

/***************************************************************/
/*                                                             */
/*  HashVal                                                    */
/*  Given a string, compute the hash value.                    */
/*                                                             */
/***************************************************************/
unsigned int HashVal(char const *str)
{
    register unsigned int i=0;
    register unsigned int j=1;
    register unsigned int len=0;

    while(*str && len < VAR_NAME_LEN) {
	i += j * (unsigned int) UPPER(*str);
	str++;
	len++;
	j = 3-j;
    }
    return i;
}

/***************************************************************/
/*                                                             */
/*  FindVar                                                    */
/*  Given a string, find the variable whose name is that       */
/*  string.  If create is 1, create the variable.              */
/*                                                             */
/***************************************************************/
Var *FindVar(char const *str, int create)
{
    register int h;
    register Var *v;
    register Var *prev;

    h = HashVal(str) % VAR_HASH_SIZE;
    v = VHashTbl[h];
    prev = NULL;

    while(v) {
	if (! StrinCmp(str, v->name, VAR_NAME_LEN)) return v;
	prev = v;
	v = v-> next;
    }
    if (!create) return v;

/* Create the variable */
    v = NEW(Var);
    if (!v) return v;
    v->next = NULL;
    v->v.type = INT_TYPE;
    v->v.v.val = 0;
    v->preserve = 0;
    StrnCpy(v->name, str, VAR_NAME_LEN);

    if (prev) prev->next = v; else VHashTbl[h] = v;
    return v;
}

/***************************************************************/
/*                                                             */
/*  DeleteVar                                                  */
/*  Given a string, find the variable whose name is that       */
/*  string and delete it.                                      */
/*                                                             */
/***************************************************************/
int DeleteVar(char const *str)
{
    register int h;
    register Var *v;
    register Var *prev;

    h = HashVal(str) % VAR_HASH_SIZE;
    v = VHashTbl[h];
    prev = NULL;

    while(v) {
	if (! StrinCmp(str, v->name, VAR_NAME_LEN)) break;
	prev = v;
	v = v-> next;
    }
    if (!v) return E_NOSUCH_VAR;
    DestroyValue(v->v);
    if (prev) prev->next = v->next; else VHashTbl[h] = v->next;
    free(v);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  SetVar                                                     */
/*                                                             */
/*  Set the indicate variable to the specified value.          */
/*                                                             */
/***************************************************************/
int SetVar(char const *str, Value *val)
{
    Var *v = FindVar(str, 1);

    if (!v) return E_NO_MEM;  /* Only way FindVar can fail */

    DestroyValue(v->v);
    v->v = *val;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  GetVarValue                                                */
/*                                                             */
/*  Get a copy of the value of the variable.                   */
/*                                                             */
/***************************************************************/
int GetVarValue(char const *str, Value *val, Var *locals, ParsePtr p)
{
    Var *v;

    /* Try searching local variables first */
    v = locals;
    while (v) {
	if (! StrinCmp(str, v->name, VAR_NAME_LEN))
	    return CopyValue(val, &v->v);
	v = v->next;
    }

    /* Global variable... mark expression as non-constant */
    if (p) p->nonconst_expr = 1;
    v=FindVar(str, 0);

    if (!v) {
	Eprint("%s: %s", ErrMsg[E_NOSUCH_VAR], str);
	return E_NOSUCH_VAR;
    }
    return CopyValue(val, &v->v);
}

/***************************************************************/
/*                                                             */
/*  DoSet - set a variable.                                    */
/*                                                             */
/***************************************************************/
int DoSet (Parser *p)
{
    Value v;
    int r;

    DynamicBuffer buf;
    DBufInit(&buf);

    r = ParseIdentifier(p, &buf);
    if (r) return r;

    /* Allow optional equals-sign:  SET var = value */
    if (ParseNonSpaceChar(p, &r, 1) == '=') {
	ParseNonSpaceChar(p, &r, 0);
    }

    r = EvaluateExpr(p, &v);
    if (r) {
	DBufFree(&buf);
	return r;
    }

    if (*DBufValue(&buf) == '$') r = SetSysVar(DBufValue(&buf)+1, &v);
    else r = SetVar(DBufValue(&buf), &v);
    DBufFree(&buf);
    return r;
}

/***************************************************************/
/*                                                             */
/*  DoUnset - delete a bunch of variables.                     */
/*                                                             */
/***************************************************************/
int DoUnset (Parser *p)
{
    int r;

    DynamicBuffer buf;
    DBufInit(&buf);

    r = ParseToken(p, &buf);
    if (r) return r;
    if (!DBufLen(&buf)) {
	DBufFree(&buf);
	return E_EOLN;
    }

    (void) DeleteVar(DBufValue(&buf));  /* Ignore error - nosuchvar */

/* Keep going... */
    while(1) {
	r = ParseToken(p, &buf);
	if (r) return r;
	if (!DBufLen(&buf)) {
	    DBufFree(&buf);
	    return OK;
	}
	(void) DeleteVar(DBufValue(&buf));
    }
}

/***************************************************************/
/*                                                             */
/*  DoDump                                                     */
/*                                                             */
/*  Command file command to dump variable table.               */
/*                                                             */
/***************************************************************/
int DoDump(ParsePtr p)
{
    int r;
    Var *v;
    DynamicBuffer buf;

    if (PurgeMode) return OK;

    DBufInit(&buf);
    r = ParseToken(p, &buf);
    if (r) return r;
    if (!*DBufValue(&buf) ||
	*DBufValue(&buf) == '#' ||
	*DBufValue(&buf) == ';') {
	DBufFree(&buf);
	DumpVarTable();
	return OK;
    }
    fprintf(ErrFp, "%*s  %s\n\n", VAR_NAME_LEN, VARIABLE, VALUE);
    while(1) {
	if (*DBufValue(&buf) == '$') {
	    DumpSysVarByName(DBufValue(&buf)+1);
	} else {
	    v = FindVar(DBufValue(&buf), 0);
	    DBufValue(&buf)[VAR_NAME_LEN] = 0;
	    if (!v) fprintf(ErrFp, "%*s  %s\n", VAR_NAME_LEN,
			    DBufValue(&buf), UNDEF);
	    else {
		fprintf(ErrFp, "%*s  ", VAR_NAME_LEN, v->name);
		PrintValue(&(v->v), ErrFp);
		fprintf(ErrFp, "\n");
	    }
	}
	r = ParseToken(p, &buf);
	if (r) return r;
	if (!*DBufValue(&buf) ||
	    *DBufValue(&buf) == '#' ||
	    *DBufValue(&buf) == ';') {
	    DBufFree(&buf);
	    return OK;
	}
    }
}

/***************************************************************/
/*                                                             */
/*  DumpVarTable                                               */
/*                                                             */
/*  Dump the variable table to stderr.                         */
/*                                                             */
/***************************************************************/
void DumpVarTable(void)
{
    register Var *v;
    register int i;

    fprintf(ErrFp, "%*s  %s\n\n", VAR_NAME_LEN, VARIABLE, VALUE);

    for (i=0; i<VAR_HASH_SIZE; i++) {
	v = VHashTbl[i];
	while(v) {
	    fprintf(ErrFp, "%*s  ", VAR_NAME_LEN, v->name);
	    PrintValue(&(v->v), ErrFp);
	    fprintf(ErrFp, "\n");
	    v = v->next;
	}
    }
}

/***************************************************************/
/*                                                             */
/*  DestroyVars                                                */
/*                                                             */
/*  Free all the memory used by variables, but don't delete    */
/*  preserved variables unless ALL is non-zero.                */
/*                                                             */
/***************************************************************/
void DestroyVars(int all)
{
    int i;
    Var *v, *next, *prev;

    for (i=0; i<VAR_HASH_SIZE; i++) {
	v = VHashTbl[i];
	VHashTbl[i] = NULL;
	prev = NULL;
	while(v) {
	    if (all || !v->preserve) {
		DestroyValue(v->v);
		next = v->next;
		free(v);
	    } else {
		if (prev) prev->next = v;
		else VHashTbl[i] = v;
		prev = v;
		next = v->next;
		v->next = NULL;
	    }
	    v = next;
	}
    }
}

/***************************************************************/
/*                                                             */
/*  PreserveVar                                                */
/*                                                             */
/*  Given the name of a variable, "preserve" it.               */
/*                                                             */
/***************************************************************/
int PreserveVar(char const *name)
{
    Var *v;

    v = FindVar(name, 1);
    if (!v) return E_NO_MEM;
    v->preserve = 1;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  DoPreserve - preserve a bunch of variables.                */
/*                                                             */
/***************************************************************/
int DoPreserve (Parser *p)
{
    int r;

    DynamicBuffer buf;
    DBufInit(&buf);

    r = ParseToken(p, &buf);
    if (r) return r;
    if (!DBufLen(&buf)) {
	DBufFree(&buf);
	return E_EOLN;
    }

    r = PreserveVar(DBufValue(&buf));
    DBufFree(&buf);
    if (r) return r;

/* Keep going... */
    while(1) {
	r = ParseToken(p, &buf);
	if (r) return r;
	if (!DBufLen(&buf)) {
	    DBufFree(&buf);
	    return OK;
	}
	r = PreserveVar(DBufValue(&buf));
	DBufFree(&buf);
	if (r) return r;
    }
}

/***************************************************************/
/*                                                             */
/*  SYSTEM VARIABLES                                           */
/*                                                             */
/*  Interface for modifying and reading system variables.      */
/*                                                             */
/***************************************************************/

/* The structure of a system variable */
typedef struct {
    char const *name;
    char modifiable;
    int type;
    void *value;
    int min;
    int max;
} SysVar;

/* If the type of a sys variable is STR_TYPE, then min is redefined
   to be a flag indicating whether or not the value has been malloc'd. */
#define been_malloced min

/* Flag for no min/max constraint */
#define ANY -31415926

/* All of the system variables sorted alphabetically */
static SysVar SysVarArr[] = {
    /*  name          mod  type              value         min/mal   max */
    {"CalcUTC",        1,  INT_TYPE,     &CalculateUTC,       0,      1   },
    {"CalMode",        0,  INT_TYPE,     &DoCalendar,         0,      0   },
    {"Daemon",         0,  INT_TYPE,     &Daemon,             0,      0   },
    {"DateSep",        1,  SPECIAL_TYPE, date_sep_func,       0,      0   },
    {"DefaultPrio",    1,  INT_TYPE,     &DefaultPrio,        0,      9999},
    {"DeltaOffset",    0,  INT_TYPE,     &DeltaOffset,        0,      0   },
    {"DontFork",       0,  INT_TYPE,     &DontFork,           0,      0   },
    {"DontQueue",      0,  INT_TYPE,     &DontQueue,          0,      0   },
    {"DontTrigAts",    0,  INT_TYPE,     &DontIssueAts,       0,      0   },
    {"EndSent",        1,  STR_TYPE,     &EndSent,            0,      0   },
    {"EndSentIg",      1,  STR_TYPE,     &EndSentIg,          0,      0   },
    {"FirstIndent",    1,  INT_TYPE,     &FirstIndent,        0,      132 },
    {"FoldYear",       1,  INT_TYPE,     &FoldYear,           0,      1   },
    {"FormWidth",      1,  INT_TYPE,     &FormWidth,          20,     132 },
    {"HushMode",       0,  INT_TYPE,     &Hush,               0,      0   },
    {"IgnoreOnce",     0,  INT_TYPE,     &IgnoreOnce,         0,      0   },
    {"InfDelta",       0,  INT_TYPE,     &InfiniteDelta,      0,      0   },
    {"LatDeg",         1,  INT_TYPE,     &LatDeg,             -90,    90  },
    {"LatMin",         1,  INT_TYPE,     &LatMin,             -59,    59  },
    {"LatSec",         1,  INT_TYPE,     &LatSec,             -59,    59  },
    {"Location",       1,  STR_TYPE,     &Location,           0,      0   },
    {"LongDeg",        1,  INT_TYPE,     &LongDeg,            -180,   180 },
    {"LongMin",        1,  INT_TYPE,     &LongMin,            -59,    59  },
    {"LongSec",        1,  INT_TYPE,     &LongSec,            -59,    59  },
    {"MaxSatIter",     1,  INT_TYPE,     &MaxSatIter,         10,     ANY },
    {"MaxStringLen",   1,  INT_TYPE,     &MaxStringLen,       -1,     ANY },
    {"MinsFromUTC",    1,  INT_TYPE,     &MinsFromUTC,        -780,   780 },
    {"NextMode",       0,  INT_TYPE,     &NextMode,           0,      0   },
    {"NumQueued",      0,  INT_TYPE,     &NumQueued,          0,      0   },
    {"NumTrig",        0,  INT_TYPE,     &NumTriggered,       0,      0   },
    {"PrefixLineNo",   0,  INT_TYPE,     &DoPrefixLineNo,     0,      0   },
    {"PSCal",          0,  INT_TYPE,     &PsCal,              0,      0   },
    {"RunOff",         0,  INT_TYPE,     &RunDisabled,        0,      0   },
    {"SimpleCal",      0,  INT_TYPE,     &DoSimpleCalendar,   0,      0   },
    {"SortByDate",     0,  INT_TYPE,     &SortByDate,         0,      0   },
    {"SortByPrio",     0,  INT_TYPE,     &SortByPrio,         0,      0   },
    {"SortByTime",     0,  INT_TYPE,     &SortByTime,         0,      0   },
    {"SubsIndent",     1,  INT_TYPE,     &SubsIndent,         0,      132 },
    {"T",              0,  SPECIAL_TYPE, trig_date_func,      0,      0   },
    {"Td",             0,  SPECIAL_TYPE, trig_day_func,       0,      0   },
    {"TimeSep",        1,  SPECIAL_TYPE, time_sep_func,       0,      0   },
    {"Tm",             0,  SPECIAL_TYPE, trig_mon_func,       0,      0   },
    {"Tw",             0,  SPECIAL_TYPE, trig_wday_func,      0,      0   },
    {"Ty",             0,  SPECIAL_TYPE, trig_year_func,      0,      0   },
    {"U",              0,  SPECIAL_TYPE, today_date_func,     0,      0   },
    {"Ud",             0,  SPECIAL_TYPE, today_day_func,      0,      0   },
    {"Um",             0,  SPECIAL_TYPE, today_mon_func,      0,      0   },
    {"UntimedFirst",   0,  INT_TYPE,     &UntimedBeforeTimed, 0,      0   },
    {"Uw",             0,  SPECIAL_TYPE, today_wday_func,     0,      0   },
    {"Uy",             0,  SPECIAL_TYPE, today_year_func,     0,      0   }
};

#define NUMSYSVARS ( sizeof(SysVarArr) / sizeof(SysVar) )
static SysVar *FindSysVar (char const *name);
static void DumpSysVar (char const *name, const SysVar *v);
/***************************************************************/
/*                                                             */
/*  SetSysVar                                                  */
/*                                                             */
/*  Set a system variable to the indicated value.              */
/*                                                             */
/***************************************************************/
int SetSysVar(char const *name, Value *value)
{
    SysVar *v = FindSysVar(name);
    if (!v) return E_NOSUCH_VAR;
    if (v->type != SPECIAL_TYPE &&
	v->type != value->type) return E_BAD_TYPE;
    if (!v->modifiable) {
	Eprint("%s: `$%s'", ErrMsg[E_CANT_MODIFY], name);
	return E_CANT_MODIFY;
    }

    if (v->type == SPECIAL_TYPE) {
	SysVarFunc f = (SysVarFunc) v->value;
	return f(1, value);
    } else if (v->type == STR_TYPE) {
        /* If it's a string variable, special measures must be taken */
	if (v->been_malloced) free(*((char **)(v->value)));
	v->been_malloced = 1;
	*((char **) v->value) = value->v.str;
	value->type = ERR_TYPE;  /* So that it's not accidentally freed */
    } else {
	if (v->max != ANY && value->v.val > v->max) return E_2HIGH;
	if (v->min != ANY && value->v.val < v->min) return E_2LOW;
	*((int *)v->value) = value->v.val;
    }
    return OK;
}

/***************************************************************/
/*                                                             */
/*  GetSysVar                                                  */
/*                                                             */
/*  Get the value of a system variable                         */
/*                                                             */
/***************************************************************/
int GetSysVar(char const *name, Value *val)
{
    SysVar *v = FindSysVar(name);

    val->type = ERR_TYPE;
    if (!v) return E_NOSUCH_VAR;
    if (v->type == SPECIAL_TYPE) {
	SysVarFunc f = (SysVarFunc) v->value;
	return f(0, val);
    } else if (v->type == STR_TYPE) {
	val->v.str = StrDup(*((char **) v->value));
	if (!val->v.str) return E_NO_MEM;
    } else {
	val->v.val = *((int *) v->value);
    }
    val->type = v->type;

    /* In "verbose" mode, print attempts to test $RunOff */
    if (DebugFlag & DB_PRTLINE) {
	if (v->value == (void *) &RunDisabled) {
	    Eprint("(Security note: $RunOff variable tested.)\n");
	    /* Allow further messages from this line */
	    FreshLine = 1;
	}
    }
    return OK;
}

/***************************************************************/
/*                                                             */
/* FindSysVar                                                  */
/*                                                             */
/* Find a system var with specified name.                      */
/*                                                             */
/***************************************************************/
static SysVar *FindSysVar(char const *name)
{
    int top=NUMSYSVARS-1, bottom=0;
    int mid=(top + bottom) / 2;
    int r;

    while (top >= bottom) {
	r = StrCmpi(name, SysVarArr[mid].name);
	if (!r) return &SysVarArr[mid];
	else if (r>0) bottom = mid+1;
	else	    top = mid-1;
	mid = (top+bottom) / 2;
    }
    return NULL;
}

/***************************************************************/
/*                                                             */
/*  DumpSysVarByName                                           */
/*                                                             */
/*  Given the name of a system variable, display it.           */
/*  If name is "", dump all system variables.                  */
/*                                                             */
/***************************************************************/
void DumpSysVarByName(char const *name)
{
    int i;
    SysVar *v;

    if (!name || !*name) {
	for (i=0; i<NUMSYSVARS; i++) DumpSysVar(name, SysVarArr + i);
	return;
    }

    v = FindSysVar(name);
    DumpSysVar(name, v);
    return;
}

/***************************************************************/
/*                                                             */
/*  DumpSysVar                                                 */
/*                                                             */
/*  Dump the system variable.                                  */
/*                                                             */
/***************************************************************/
static void DumpSysVar(char const *name, const SysVar *v)
{
    char buffer[VAR_NAME_LEN+10];

    if (name && !*name) name=NULL;
    if (!v && !name) return;  /* Shouldn't happen... */

    buffer[0]='$'; buffer[1] = 0;
    if (name && strlen(name) > VAR_NAME_LEN) {
	fprintf(ErrFp, "$%s: Name too long\n", name);
	return;
    }
    if (name) strcat(buffer, name); else strcat(buffer, v->name);
    fprintf(ErrFp, "%*s  ", VAR_NAME_LEN+1, buffer);
    if (v) {
	if (v->type == SPECIAL_TYPE) {
	    Value val;
	    SysVarFunc f = (SysVarFunc) v->value;
	    f(0, &val);
	    PrintValue(&val, ErrFp);
	    Putc('\n', ErrFp);
	    DestroyValue(val);
	} else if (v->type == STR_TYPE) {
	    char const *s = *((char **)v->value);
	    int y;
	    Putc('"', ErrFp);
	    for (y=0; y<MAX_PRT_LEN && *s; y++) {
		if (*s == '"') {
		    fprintf(ErrFp, "\" + char(34) + \"");
		    s++;
		} else {
		    Putc(*s++, ErrFp);
		}
	    }
	    Putc('"', ErrFp);
	    if (*s) fprintf(ErrFp, "...");
	    Putc('\n', ErrFp);
	} else if (v->type == DATE_TYPE) {
	    Value val;
	    val.type = DATE_TYPE;
	    val.v.val = * (int *) v->value;
	    PrintValue(&val, ErrFp);
	    Putc('\n', ErrFp);
	} else {
	    if (!v->modifiable) fprintf(ErrFp, "%d\n", *((int *)v->value));
	    else {
		fprintf(ErrFp, "%-10d  ", *((int *)v->value));
		if (v->min == ANY) fprintf(ErrFp, "(-Inf, ");
		else                         fprintf(ErrFp, "[%d, ", v->min);
		if (v->max == ANY) fprintf(ErrFp, "Inf)\n");
		else                         fprintf(ErrFp, "%d]\n", v->max);
	    }
	}
    } else   fprintf(ErrFp, "%s\n", UNDEF);

    return;
}
