/***************************************************************/
/*                                                             */
/*  VAR.C                                                      */
/*                                                             */
/*  This file contains routines, structures, etc for           */
/*  user- and system-defined variables.                        */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-2022 by Dianne Skoll                    */
/*                                                             */
/***************************************************************/

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <locale.h>
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

static int IntMin = INT_MIN;
static int IntMax = INT_MAX;

static Var *VHashTbl[VAR_HASH_SIZE];

typedef int (*SysVarFunc)(int, Value *);

static double
strtod_in_c_locale(char const *str, char **endptr)
{
    /* Get current locale */
    char const *loc = setlocale(LC_NUMERIC, NULL);
    double x;

    /* If it failed, punt */
    if (!loc) {
        return strtod(str, endptr);
    }

    /* Set locale to C */
    setlocale(LC_NUMERIC, "C");

    x = strtod(str, endptr);

    /* Back to original locale */
    setlocale(LC_NUMERIC, loc);

    /* If we got an error, try in original locale, but issue a warning */
    if (**endptr) {
        x = strtod(str, endptr);
        if (!**endptr) {
            Wprint("Accepting \"%s\" for $Latitude/$Longitude, but you should use the \"C\" locale decimal separator \".\" instead", str);
        }
    }
    return x;
}

static void deprecated_var(char const *var, char const *instead)
{
    if (DebugFlag & DB_PRTLINE) {
        Wprint("%s is deprecated; use %s instead", var, instead);
    }
}

static int latlong_component_func(int do_set, Value *val, int *var, int min, int max, char const *varname, char const *newvarname)
{
    if (!do_set) {
        val->type = INT_TYPE;
        val->v.val = *var;
        return OK;
    }
    deprecated_var(varname, newvarname);
    if (val->type != INT_TYPE) return E_BAD_TYPE;
    if (val->v.val < min) return E_2LOW;
    if (val->v.val > max) return E_2HIGH;
    *var = val->v.val;
    set_lat_and_long_from_components();
    return OK;
}
static int latdeg_func(int do_set, Value *val)
{
    return latlong_component_func(do_set, val, &LatDeg, -90, 90, "$LatDeg", "$Latitude");
}

static int latmin_func(int do_set, Value *val)
{
    return latlong_component_func(do_set, val, &LatMin, -59, 59, "$LatMin", "$Latitude");
}

static int latsec_func(int do_set, Value *val)
{
    return latlong_component_func(do_set, val, &LatSec, -59, 59, "$LatSec", "$Latitude");
}

static int longdeg_func(int do_set, Value *val)
{
    return latlong_component_func(do_set, val, &LongDeg, -180, 180, "$LongDeg", "$Longitude");
}

static int longmin_func(int do_set, Value *val)
{
    return latlong_component_func(do_set, val, &LongMin, -59, 59, "$LongMin", "$Longitude");
}

static int longsec_func(int do_set, Value *val)
{
    return latlong_component_func(do_set, val, &LongSec, -59, 59, "$LongSec", "$Longitude");
}

static int latitude_longitude_func(int do_set, Value *val, double *var, double min, double max) {
    char buf[64];
    double x;
    char *endptr;
    char const *loc = setlocale(LC_NUMERIC, NULL);

    if (!do_set) {
        if (loc) {
            setlocale(LC_NUMERIC, "C");
        }
        snprintf(buf, sizeof(buf), "%f", *var);
        if (loc) {
            setlocale(LC_NUMERIC, loc);
        }
        val->v.str = malloc(strlen(buf)+1);
        if (!val->v.str) return E_NO_MEM;
        strcpy(val->v.str, buf);
        val->type = STR_TYPE;
        return OK;
    }
    if (val->type == INT_TYPE) {
        x = (double) val->v.val;
    } else {
        if (val->type != STR_TYPE) return E_BAD_TYPE;
        x = strtod_in_c_locale(val->v.str, &endptr);
        if (*endptr) return E_BAD_TYPE;
    }
    if (x < min) return E_2LOW;
    if (x > max) return E_2HIGH;
    *var = x;
    set_components_from_lat_and_long();
    return OK;
}

static int longitude_func(int do_set, Value *val)
{
    return latitude_longitude_func(do_set, val, &Longitude, -180.0, 180.0);
}

static int latitude_func(int do_set, Value *val)
{
    return latitude_longitude_func(do_set, val, &Latitude, -90.0, 90.0);
}


static int trig_date_func(int do_set, Value *val)
{
    UNUSED(do_set);
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
    UNUSED(do_set);
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
    UNUSED(do_set);
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
    UNUSED(do_set);
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
    UNUSED(do_set);
    if (!LastTrigValid) {
	val->v.val = -1;
	return OK;
    }

    val->v.val = (LastTriggerDate + 1) % 7;
    return OK;
}

static int today_date_func(int do_set, Value *val)
{
    UNUSED(do_set);
    val->type = DATE_TYPE;
    val->v.val = JulianToday;
    return OK;
}
static int today_day_func(int do_set, Value *val)
{
    int y, m, d;
    UNUSED(do_set);
    val->type = INT_TYPE;
    FromJulian(JulianToday, &y, &m, &d);
    val->v.val = d;
    return OK;
}

static int today_mon_func(int do_set, Value *val)
{
    int y, m, d;
    UNUSED(do_set);
    val->type = INT_TYPE;
    FromJulian(JulianToday, &y, &m, &d);
    val->v.val = m+1;
    return OK;
}

static int today_year_func(int do_set, Value *val)
{
    int y, m, d;
    UNUSED(do_set);
    val->type = INT_TYPE;
    FromJulian(JulianToday, &y, &m, &d);
    val->v.val = y;
    return OK;
}

static int today_wday_func(int do_set, Value *val)
{
    UNUSED(do_set);
    val->type = INT_TYPE;
    val->v.val = (JulianToday + 1) % 7;
    return OK;
}

static int datetime_sep_func(int do_set, Value *val)
{
    if (!do_set) {
	val->v.str = malloc(2);
	if (!val->v.str) return E_NO_MEM;
	val->v.str[0] = DateTimeSep;
	val->v.str[1] = 0;
	val->type = STR_TYPE;
	return OK;
    }
    if (val->type != STR_TYPE) return E_BAD_TYPE;
    if (strcmp(val->v.str, "T") &&
	strcmp(val->v.str, "@")) {
	return E_BAD_TYPE;
    }
    DateTimeSep = val->v.str[0];
    return OK;
}

static int default_color_func(int do_set, Value *val)
{
    int col_r, col_g, col_b;
    if (!do_set) {
    /* 12 = strlen("255 255 255\0") */
        val->v.str = malloc(12);
        if (!val->v.str) return E_NO_MEM;
        snprintf(val->v.str, 12, "%d %d %d",
                 DefaultColorR,
                 DefaultColorG,
                 DefaultColorB
            );
        val->type = STR_TYPE;
        return OK;
    }
    if (val->type != STR_TYPE) return E_BAD_TYPE;
    if (sscanf(val->v.str, "%d %d %d", &col_r, &col_g, &col_b) != 3) {
        return E_BAD_TYPE;
    }
    /* They either all have to be -1, or all between 0 and 255 */
    if (col_r == -1 && col_g == -1 && col_b == -1) {
	DefaultColorR = -1;
	DefaultColorG = -1;
	DefaultColorB = -1;
	return OK;
    }
    if (col_r < 0) return E_2LOW;
    if (col_r > 255) return E_2HIGH;
    if (col_g < 0) return E_2LOW;
    if (col_g > 255) return E_2HIGH;
    if (col_b < 0) return E_2LOW;
    if (col_b > 255) return E_2HIGH;

    DefaultColorR = col_r;
    DefaultColorG = col_g;
    DefaultColorB = col_b;
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
    if (buf.len > VAR_NAME_LEN) {
	Wprint("Warning: Variable name `%.*s...' truncated to `%.*s'",
	       VAR_NAME_LEN, DBufValue(&buf), VAR_NAME_LEN, DBufValue(&buf));
    }
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
    fprintf(ErrFp, "%s  %s\n\n", VARIABLE, VALUE);
    while(1) {
	if (*DBufValue(&buf) == '$') {
	    DumpSysVarByName(DBufValue(&buf)+1);
	} else {
	    v = FindVar(DBufValue(&buf), 0);
	    DBufValue(&buf)[VAR_NAME_LEN] = 0;
	    if (!v) fprintf(ErrFp, "%s  %s\n",
			    DBufValue(&buf), UNDEF);
	    else {
		fprintf(ErrFp, "%s  ", v->name);
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

    fprintf(ErrFp, "%s  %s\n\n", VARIABLE, VALUE);

    for (i=0; i<VAR_HASH_SIZE; i++) {
	v = VHashTbl[i];
	while(v) {
	    fprintf(ErrFp, "%s  ", v->name);
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
    int (*validate)(void const *newvalue);
} SysVar;

/* If the type of a sys variable is STR_TYPE, then min is redefined
   to be a flag indicating whether or not the value has been malloc'd. */
#define been_malloced min

/* Flag for no min/max constraint */
#define ANY -31415926

/* All of the system variables sorted alphabetically */
static SysVar SysVarArr[] = {
    /*  name          mod  type              value          min/mal   max  validate*/
    {"Am",             1,  STR_TYPE,     &DynamicAm,           0,      0,    NULL },
    {"And",            1,  STR_TYPE,     &DynamicAnd,          0,      0,    NULL },
    {"April",          1,  STR_TYPE,     &DynamicMonthName[3], 0,      0,    NULL },
    {"At",             1,  STR_TYPE,     &DynamicAt,           0,      0,    NULL },
    {"August",         1,  STR_TYPE,     &DynamicMonthName[7], 0,      0,    NULL },
    {"CalcUTC",        1,  INT_TYPE,     &CalculateUTC,        0,      1,    NULL },
    {"CalMode",        0,  INT_TYPE,     &DoCalendar,          0,      0,    NULL },
    {"Daemon",         0,  INT_TYPE,     &Daemon,              0,      0,    NULL },
    {"DateSep",        1,  SPECIAL_TYPE, date_sep_func,        0,      0,    NULL },
    {"DateTimeSep",    1,  SPECIAL_TYPE, datetime_sep_func,    0,      0,    NULL },
    {"December",       1,  STR_TYPE,     &DynamicMonthName[11],0,      0,    NULL },
    {"DefaultColor",   1,  SPECIAL_TYPE, default_color_func,   0,      0,    NULL },
    {"DefaultPrio",    1,  INT_TYPE,     &DefaultPrio,         0,      9999, NULL },
    {"DefaultTDelta",  1,  INT_TYPE,     &DefaultTDelta,       0,      1440, NULL },
    {"DeltaOffset",    0,  INT_TYPE,     &DeltaOffset,         0,      0,    NULL },
    {"DontFork",       0,  INT_TYPE,     &DontFork,            0,      0,    NULL },
    {"DontQueue",      0,  INT_TYPE,     &DontQueue,           0,      0,    NULL },
    {"DontTrigAts",    0,  INT_TYPE,     &DontIssueAts,        0,      0,    NULL },
    {"EndSent",        1,  STR_TYPE,     &EndSent,             0,      0,    NULL },
    {"EndSentIg",      1,  STR_TYPE,     &EndSentIg,           0,      0,    NULL },
    {"February",       1,  STR_TYPE,     &DynamicMonthName[1], 0,      0,    NULL },
    {"FirstIndent",    1,  INT_TYPE,     &FirstIndent,         0,      132,  NULL },
    {"FoldYear",       1,  INT_TYPE,     &FoldYear,            0,      1,    NULL },
    {"FormWidth",      1,  INT_TYPE,     &FormWidth,           20,     500,  NULL },
    {"Friday",         1,  STR_TYPE,     &DynamicDayName[4],   0,      0,    NULL },
    {"Hour",           1,  STR_TYPE,     &DynamicHour,         0,      0,    NULL },
    {"Hplu",           1,  STR_TYPE,     &DynamicHplu,         0,      0,    NULL },
    {"HushMode",       0,  INT_TYPE,     &Hush,                0,      0,    NULL },
    {"IgnoreOnce",     0,  INT_TYPE,     &IgnoreOnce,          0,      0,    NULL },
    {"InfDelta",       0,  INT_TYPE,     &InfiniteDelta,       0,      0,    NULL },
    {"IntMax",         0,  INT_TYPE,     &IntMax,              0,      0,    NULL },
    {"IntMin",         0,  INT_TYPE,     &IntMin,              0,      0,    NULL },
    {"Is",             1,  STR_TYPE,     &DynamicIs,           0,      0,    NULL },
    {"January",        1,  STR_TYPE,     &DynamicMonthName[0], 0,      0,    NULL },
    {"July",           1,  STR_TYPE,     &DynamicMonthName[6], 0,      0,    NULL },
    {"June",           1,  STR_TYPE,     &DynamicMonthName[5], 0,      0,    NULL },
    {"LatDeg",         1,  SPECIAL_TYPE, latdeg_func,          0,      0,    NULL },
    {"Latitude",       1,  SPECIAL_TYPE, latitude_func,        0,      0,    NULL },
    {"LatMin",         1,  SPECIAL_TYPE, latmin_func,          0,      0,    NULL },
    {"LatSec",         1,  SPECIAL_TYPE, latsec_func,          0,      0,    NULL },
    {"Location",       1,  STR_TYPE,     &Location,            0,      0,    NULL },
    {"LongDeg",        1,  SPECIAL_TYPE, longdeg_func,         0,      0,    NULL },
    {"Longitude",      1,  SPECIAL_TYPE, longitude_func,       0,      0,    NULL },
    {"LongMin",        1,  SPECIAL_TYPE, longmin_func,         0,      0,    NULL },
    {"LongSec",        1,  SPECIAL_TYPE, longsec_func,         0,      0,    NULL },
    {"March",          1,  STR_TYPE,     &DynamicMonthName[2], 0,      0,    NULL },
    {"MaxSatIter",     1,  INT_TYPE,     &MaxSatIter,          10,     ANY,  NULL },
    {"MaxStringLen",   1,  INT_TYPE,     &MaxStringLen,        -1,     ANY,  NULL },
    {"May",            1,  STR_TYPE,     &DynamicMonthName[4], 0,      0,    NULL },
    {"MinsFromUTC",    1,  INT_TYPE,     &MinsFromUTC,         -780,   780,  NULL },
    {"Minute",         1,  STR_TYPE,     &DynamicMinute,       0,      0,    NULL },
    {"Monday",         1,  STR_TYPE,     &DynamicDayName[0],   0,      0,    NULL },
    {"Mplu",           1,  STR_TYPE,     &DynamicMplu,         0,      0,    NULL },
    {"NextMode",       0,  INT_TYPE,     &NextMode,            0,      0,    NULL },
    {"November",       1,  STR_TYPE,     &DynamicMonthName[10],0,      0,    NULL },
    {"Now",            1,  STR_TYPE,     &DynamicNow,          0,      0,    NULL },
    {"NumQueued",      0,  INT_TYPE,     &NumQueued,           0,      0,    NULL },
    {"NumTrig",        0,  INT_TYPE,     &NumTriggered,        0,      0,    NULL },
    {"October",        1,  STR_TYPE,     &DynamicMonthName[9], 0,      0,    NULL },
    {"On",             1,  STR_TYPE,     &DynamicOn,           0,      0,    NULL },
    {"Pm",             1,  STR_TYPE,     &DynamicPm,           0,      0,    NULL },
    {"PrefixLineNo",   0,  INT_TYPE,     &DoPrefixLineNo,      0,      0,    NULL },
    {"PSCal",          0,  INT_TYPE,     &PsCal,               0,      0,    NULL },
    {"RunOff",         0,  INT_TYPE,     &RunDisabled,         0,      0,    NULL },
    {"Saturday",       1,  STR_TYPE,     &DynamicDayName[5],   0,      0,    NULL },
    {"September",      1,  STR_TYPE,     &DynamicMonthName[8], 0,      0,    NULL },
    {"SimpleCal",      0,  INT_TYPE,     &DoSimpleCalendar,    0,      0,    NULL },
    {"SortByDate",     0,  INT_TYPE,     &SortByDate,          0,      0,    NULL },
    {"SortByPrio",     0,  INT_TYPE,     &SortByPrio,          0,      0,    NULL },
    {"SortByTime",     0,  INT_TYPE,     &SortByTime,          0,      0,    NULL },
    {"SubsIndent",     1,  INT_TYPE,     &SubsIndent,          0,      132,  NULL },
    {"Sunday",         1,  STR_TYPE,     &DynamicDayName[6],   0,      0,    NULL },
    {"SysInclude",     0,  STR_TYPE,     &SysDir,              0,      0,    NULL },
    {"T",              0,  SPECIAL_TYPE, trig_date_func,       0,      0,    NULL },
    {"Td",             0,  SPECIAL_TYPE, trig_day_func,        0,      0,    NULL },
    {"Thursday",       1,  STR_TYPE,     &DynamicDayName[3],   0,      0,    NULL },
    {"TimeSep",        1,  SPECIAL_TYPE, time_sep_func,        0,      0,    NULL },
    {"Tm",             0,  SPECIAL_TYPE, trig_mon_func,        0,      0,    NULL },
    {"Today",          1,  STR_TYPE,     &DynamicToday,        0,      0,    NULL },
    {"Tomorrow",       1,  STR_TYPE,     &DynamicTomorrow,     0,      0,    NULL },
    {"Tuesday",        1,  STR_TYPE,     &DynamicDayName[1],   0,      0,    NULL },
    {"Tw",             0,  SPECIAL_TYPE, trig_wday_func,       0,      0,    NULL },
    {"Ty",             0,  SPECIAL_TYPE, trig_year_func,       0,      0,    NULL },
    {"U",              0,  SPECIAL_TYPE, today_date_func,      0,      0,    NULL },
    {"Ud",             0,  SPECIAL_TYPE, today_day_func,       0,      0,    NULL },
    {"Um",             0,  SPECIAL_TYPE, today_mon_func,       0,      0,    NULL },
    {"UntimedFirst",   0,  INT_TYPE,     &UntimedBeforeTimed,  0,      0,    NULL },
    {"Uw",             0,  SPECIAL_TYPE, today_wday_func,      0,      0,    NULL },
    {"Uy",             0,  SPECIAL_TYPE, today_year_func,      0,      0,    NULL },
    {"Was",            1,  STR_TYPE,     &DynamicWas,          0,      0,    NULL },
    {"Wednesday",      1,  STR_TYPE,     &DynamicDayName[2],   0,      0,    NULL }
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
    int r;
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
	r = f(1, value);
        DestroyValue(*value);
        return r;
    }
    if (v->validate) {
        if (v->type == STR_TYPE) {
            r = (v->validate)((void *) value->v.str);
        } else {
            r = (v->validate)((void *) &(value->v.val));
        }
        if (r != OK) {
            return r;
        }
    }
    if (v->type == STR_TYPE) {
        /* If it's already the same, don't bother doing anything */
        if (!strcmp(value->v.str, (char const *) v->value)) {
            DestroyValue(*value);
            return OK;
        }

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
        if (! * (char **) v->value) {
            val->v.str = StrDup("");
        } else {
            val->v.str = StrDup(*((char **) v->value));
        }
	if (!val->v.str) return E_NO_MEM;
    } else {
	val->v.val = *((int *) v->value);
    }
    val->type = v->type;

    /* In "verbose" mode, print attempts to test $RunOff */
    if (DebugFlag & DB_PRTLINE) {
	if (v->value == (void *) &RunDisabled) {
	    Wprint("(Security note: $RunOff variable tested.)");
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
    size_t i;
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
    fprintf(ErrFp, "%16s  ", buffer);
    if (v) {
	if (v->type == SPECIAL_TYPE) {
	    Value val;
	    SysVarFunc f = (SysVarFunc) v->value;
	    f(0, &val);
	    PrintValue(&val, ErrFp);
	    putc('\n', ErrFp);
	    DestroyValue(val);
	} else if (v->type == STR_TYPE) {
	    char const *s = *((char **)v->value);
	    int y;
	    putc('"', ErrFp);
	    for (y=0; y<MAX_PRT_LEN && *s; y++) {
		if (*s == '"') {
		    fprintf(ErrFp, "\" + char(34) + \"");
		    s++;
		} else {
		    putc(*s++, ErrFp);
		}
	    }
	    putc('"', ErrFp);
	    if (*s) fprintf(ErrFp, "...");
	    putc('\n', ErrFp);
	} else if (v->type == DATE_TYPE) {
	    Value val;
	    val.type = DATE_TYPE;
	    val.v.val = * (int *) v->value;
	    PrintValue(&val, ErrFp);
	    putc('\n', ErrFp);
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

void
set_lat_and_long_from_components(void)
{
    Latitude = (double) LatDeg + ((double) LatMin) / 60.0 + ((double) LatSec) / 3600.0;
    Longitude = - ( (double) LongDeg + ((double) LongMin) / 60.0 + ((double) LongSec) / 3600.0);
}

void
set_components_from_lat_and_long(void)
{
    double x;

    x = (Latitude < 0.0 ? -Latitude : Latitude);
    LatDeg = (int) x;
    x -= (double) LatDeg;
    x *= 60;
    LatMin = (int) x;
    x -= (double) LatMin;
    x *= 60;
    LatSec = (int) x;
    if (Latitude < 0.0) {
        LatDeg = -LatDeg;
        LatMin = -LatMin;
        LatSec = -LatSec;
    }

    x = (Longitude < 0.0 ? -Longitude : Longitude);
    LongDeg = (int) x;
    x -= (double) LongDeg;
    x *= 60;
    LongMin = (int) x;
    x -= (double) LongMin;
    x *= 60;
    LongSec = (int) x;

    /* Use STANDARD sign for $Longitude even if $LongDeg, $LongMin and
     * $LongSec are messed up */
    if (Longitude > 0.0) {
        LongDeg = -LongDeg;
        LongMin = -LongMin;
        LongSec = -LongSec;
    }
}

