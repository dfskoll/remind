/***************************************************************/
/*                                                             */
/*  VAR.C                                                      */
/*                                                             */
/*  This file contains routines, structures, etc for           */
/*  user- and system-defined variables.                        */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by David F. Skoll                  */
/*                                                             */
/***************************************************************/

#include "config.h"
static char const RCSID[] = "$Id: var.c,v 1.4 1998-02-10 03:15:57 dfs Exp $";

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

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

/***************************************************************/
/*                                                             */
/*  HashVal                                                    */
/*  Given a string, compute the hash value.                    */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC unsigned int HashVal(const char *str)
#else
unsigned int HashVal(str)
char *str;
#endif
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
#ifdef HAVE_PROTOS
PUBLIC Var *FindVar(const char *str, int create)
#else
Var *FindVar(str, create)
char *str;
int create;
#endif
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
#ifdef HAVE_PROTOS
PUBLIC int DeleteVar(const char *str)
#else
int DeleteVar(str)
char *str;
#endif
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
#ifdef HAVE_PROTOS
PUBLIC int SetVar(const char *str, Value *val)
#else
int SetVar(str, val)
char *str;
Value *val;
#endif
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
#ifdef HAVE_PROTOS
PUBLIC int GetVarValue(const char *str, Value *val, Var *locals)
#else
int GetVarValue(str, val, locals)
char *str;
Value *val;
Var *locals;
#endif
{
    Var *v;

    /* Try searching local variables first */
    v = locals;
    while (v) {
	if (! StrinCmp(str, v->name, VAR_NAME_LEN))
	    return CopyValue(val, &v->v);
	v = v->next;
    }

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
#ifdef HAVE_PROTOS
PUBLIC int DoSet (Parser *p)
#else
int DoSet (p)
Parser *p;
#endif
{
    Value v;
    int r;

    DynamicBuffer buf;
    DBufInit(&buf);

    r = ParseIdentifier(p, &buf);
    if (r) return r;

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
#ifdef HAVE_PROTOS
PUBLIC int DoUnset (Parser *p)
#else
int DoUnset (p)
Parser *p;
#endif
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
#ifdef HAVE_PROTOS
PUBLIC int DoDump(ParsePtr p)
#else
int DoDump(p)
ParsePtr p;
#endif
{
    int r;
    Var *v;
    DynamicBuffer buf;

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
#ifdef HAVE_PROTOS
PUBLIC void DumpVarTable(void)
#else
void DumpVarTable()
#endif
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
#ifdef HAVE_PROTOS
PUBLIC void DestroyVars(int all)
#else
void DestroyVars(all)
int all;
#endif
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
#ifdef HAVE_PROTOS
PUBLIC int PreserveVar(char *name)
#else
int PreserveVar(name)
char *name;
#endif
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
#ifdef HAVE_PROTOS
PUBLIC int DoPreserve (Parser *p)
#else
int DoPreserve (p)
Parser *p;
#endif
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
    char *name;
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
#define ANY 4532
/* All of the system variables sorted alphabetically */
static SysVar SysVarArr[] = {
    /* name		  mod	type		value		min/mal	max */
    {   "CalcUTC",	  1,	INT_TYPE,	&CalculateUTC,	0,	1   },
    {   "CalMode",	  0,	INT_TYPE,	&DoCalendar,	0,	0   },
    {   "Daemon",	  0,	INT_TYPE,	&Daemon,	0,	0   },
    {   "DefaultPrio",	  1,	INT_TYPE,	&DefaultPrio,	0,	9999 },
    {   "DontFork",	  0,	INT_TYPE,	&DontFork,	0,	0   },
    {   "DontQueue",	  0,	INT_TYPE,	&DontQueue,	0,	0   },
    {   "DontTrigAts",	  0,	INT_TYPE,	&DontIssueAts,	0,	0   },
    {   "EndSent",	  1,	STR_TYPE,	&EndSent,	0,	0   },
    {   "EndSentIg",	  1,	STR_TYPE,	&EndSentIg,	0,	0   },
    {   "FirstIndent",	  1,	INT_TYPE,	&FirstIndent,	0,	132 },
    {   "FoldYear",	  1,	INT_TYPE,	&FoldYear,	0,	1   },
    {   "FormWidth",	  1,	INT_TYPE,	&FormWidth,	20,	132 },
    {   "HushMode",	  0,	INT_TYPE,	&Hush,		0,	0   },
    {   "IgnoreOnce",	  0,	INT_TYPE,	&IgnoreOnce,	0,	0   },
    {   "InfDelta",	  0,	INT_TYPE,	&InfiniteDelta,	0,	0   },
    {   "LatDeg",	  1,	INT_TYPE,	&LatDeg,	-90,	90  },
    {   "LatMin",	  1,	INT_TYPE,	&LatMin,	-59,	59  },
    {   "LatSec",	  1,	INT_TYPE,	&LatSec,	-59,	59  },
    {   "Location",	  1,	STR_TYPE,	&Location,	0,	0   },
    {   "LongDeg",	  1,	INT_TYPE,	&LongDeg,	-180,	180  },
    {   "LongMin",	  1,	INT_TYPE,	&LongMin,	-59,	59  },
    {   "LongSec",	  1,	INT_TYPE,	&LongSec,	-59,	59  },
    {   "MaxSatIter",	  1,	INT_TYPE,	&MaxSatIter,	10,	ANY },
    {   "MinsFromUTC",	  1,	INT_TYPE,	&MinsFromUTC,	-13*60,	13*60 },
    {   "NextMode",	  0,	INT_TYPE,	&NextMode,	0,	0   },
    {   "NumQueued",	  0,	INT_TYPE,	&NumQueued,	0,	0   },
    {   "NumTrig",	  0,	INT_TYPE,	&NumTriggered,	0,	0   },
    {   "PSCal",		  0,	INT_TYPE,	&PsCal,		0,	0   },
    {   "RunOff",	  0,	INT_TYPE,	&RunDisabled,	0,	0   },
    {   "SimpleCal",	  0,	INT_TYPE,	&DoSimpleCalendar,	0,  0 },
    {   "SortByDate",	  0,	INT_TYPE,	&SortByDate,	0,	0},
    {   "SortByPrio",	  0,	INT_TYPE,	&SortByPrio,	0,	0},
    {   "SortByTime",	  0,	INT_TYPE,	&SortByTime,	0,	0},
    {   "SubsIndent",	  1,	INT_TYPE,	&SubsIndent,	0,	132}
};

#define NUMSYSVARS ( sizeof(SysVarArr) / sizeof(SysVar) )
PRIVATE SysVar *FindSysVar ARGS((const char *name));
PRIVATE void DumpSysVar ARGS((const char *name, const SysVar *v));
/***************************************************************/
/*                                                             */
/*  SetSysVar                                                  */
/*                                                             */
/*  Set a system variable to the indicated value.              */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int SetSysVar(const char *name, Value *value)
#else
int SetSysVar(name, value)
char *name;
Value *value;
#endif
{
    SysVar *v = FindSysVar(name);
    if (!v) return E_NOSUCH_VAR;
    if (v->type != value->type) return E_BAD_TYPE;
    if (!v->modifiable) {
	Eprint("%s: `$%s'", ErrMsg[E_CANT_MODIFY], name);
	return E_CANT_MODIFY;
    }

/* If it's a string variable, special measures must be taken */
    if (v->type == STR_TYPE) {
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
#ifdef HAVE_PROTOS
PUBLIC int GetSysVar(const char *name, Value *val)
#else
int GetSysVar(name, val)
char *name;
Value *val;
#endif
{
    SysVar *v = FindSysVar(name);

    val->type = ERR_TYPE;
    if (!v) return E_NOSUCH_VAR;
    if (v->type == STR_TYPE) {
	val->v.str = StrDup(*((char **) v->value));
	if (!val->v.str) return E_NO_MEM;
    } else {
	val->v.val = *((int *) v->value);
    }
    val->type = v->type;
    return OK;
}

/***************************************************************/
/*                                                             */
/* FindSysVar                                                  */
/*                                                             */
/* Find a system var with specified name.                      */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE SysVar *FindSysVar(const char *name)
#else
static SysVar *FindSysVar(name)
char *name;
#endif
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
#ifdef HAVE_PROTOS
PUBLIC void DumpSysVarByName(const char *name)
#else
void DumpSysVarByName(name)
char *name;
#endif
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
#ifdef HAVE_PROTOS
PRIVATE void DumpSysVar(const char *name, const SysVar *v)
#else
static void DumpSysVar(name, v)
char *name;
SysVar *v;
#endif
{
    char buffer[VAR_NAME_LEN+10];

    if (name && !*name) name=NULL;
    if (!v && !name) return;  /* Shouldn't happen... */
   
    buffer[0]='$'; buffer[1] = 0;
    if (name) strcat(buffer, name); else strcat(buffer, v->name);
    fprintf(ErrFp, "%*s  ", VAR_NAME_LEN, buffer);
    if (v) {
	if (v->type == STR_TYPE) {
	    char *s = *((char **)v->value);
	    int y;
	    Putc('"', ErrFp);
	    for (y=0; y<MAX_PRT_LEN && *s; y++) Putc(*s++, ErrFp);
	    Putc('"', ErrFp);
	    if (*s) fprintf(ErrFp, "...");
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

