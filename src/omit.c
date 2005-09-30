/***************************************************************/
/*                                                             */
/*  OMIT.C                                                     */
/*                                                             */
/*  This file handles all global OMIT commands, and maintains  */
/*  the data structures for OMITted dates.                     */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by David F. Skoll                  */
/*  Copyright (C) 1999-2000 by Roaring Penguin Software Inc.   */
/*                                                             */
/***************************************************************/

#include "config.h"
static char const RCSID[] = "$Id: omit.c,v 1.7 2005-09-30 03:29:32 dfs Exp $";

#include <stdio.h>

#include <stdlib.h>
#include "types.h"
#include "protos.h"
#include "globals.h"
#include "err.h"

static int BexistsIntArray (int array[], int num, int key);
static void InsertIntoSortedArray (int *array, int num, int key);

/* Arrays for the global omits */
static int FullOmitArray[MAX_FULL_OMITS];
static int PartialOmitArray[MAX_PARTIAL_OMITS];

/* How many of each omit types do we have? */
static int NumFullOmits, NumPartialOmits;

/* The structure for saving and restoring OMIT contexts */
typedef struct omitcontext {
    struct omitcontext *next;
    int numfull, numpart;
    int *fullsave;
    int *partsave;
} OmitContext;

/* The stack of saved omit contexts */
static OmitContext *SavedOmitContexts = NULL;

/***************************************************************/
/*                                                             */
/*  ClearGlobalOmits                                           */
/*                                                             */
/*  Clear all the global OMIT context.                         */
/*                                                             */
/***************************************************************/
int ClearGlobalOmits(void)
{
    NumFullOmits = NumPartialOmits = 0;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  DoClear                                                    */
/*                                                             */
/*  The command-line function CLEAR-OMIT-CONTEXT               */
/*                                                             */
/***************************************************************/
int DoClear(ParsePtr p)
{
    ClearGlobalOmits();
    return VerifyEoln(p);
}

/***************************************************************/
/*                                                             */
/*  DestroyOmitContexts                                        */
/*                                                             */
/*  Free all the memory used by saved OMIT contexts.           */
/*  As a side effect, return the number of OMIT contexts       */
/*  destroyed.                                                 */
/*                                                             */
/***************************************************************/
int DestroyOmitContexts(void)
{
    OmitContext *c = SavedOmitContexts;
    OmitContext *d;
    int num = 0;

    while (c) {
	num++;
	if (c->fullsave) free(c->fullsave);
	if (c->partsave) free(c->partsave);
	d = c->next;
	free(c);
	c = d;
    }
    SavedOmitContexts = NULL;
    return num;
}

/***************************************************************/
/*                                                             */
/*  PushOmitContext                                            */
/*                                                             */
/*  Push the OMIT context on to the stack.                     */
/*                                                             */
/***************************************************************/
int PushOmitContext(ParsePtr p)
{
    register int i;
    OmitContext *context;

/* Create the saved context */
    context = NEW(OmitContext);
    if (!context) return E_NO_MEM;

    context->numfull = NumFullOmits;
    context->numpart = NumPartialOmits;
    context->fullsave = (int *) malloc(NumFullOmits * sizeof(int));
    if (NumFullOmits && !context->fullsave) {
	free(context);
	return E_NO_MEM;
    }
    context->partsave = (int *) malloc(NumPartialOmits * sizeof(int));
    if (NumPartialOmits && !context->partsave) {
	free(context->fullsave);
	free(context);
	return E_NO_MEM;
    }
      
/* Copy the context over */
    for (i=0; i<NumFullOmits; i++)
	*(context->fullsave + i) = FullOmitArray[i];

    for (i=0; i<NumPartialOmits; i++)
	*(context->partsave + i) = PartialOmitArray[i];

/* Add the context to the stack */
    context->next = SavedOmitContexts;
    SavedOmitContexts = context;
    return VerifyEoln(p);
}

/***************************************************************/
/*                                                             */
/*  PopOmitContext                                             */
/*                                                             */
/*  Pop the OMIT context off of the stack.                     */
/*                                                             */
/***************************************************************/
int PopOmitContext(ParsePtr p)
{

    register int i;
    OmitContext *c = SavedOmitContexts;

    if (!c) return E_POP_NO_PUSH;
    NumFullOmits = c->numfull;
    NumPartialOmits = c->numpart;

/* Copy the context over */
    for (i=0; i<NumFullOmits; i++)
	FullOmitArray[i] = *(c->fullsave + i);

    for (i=0; i<NumPartialOmits; i++)
	PartialOmitArray[i] = *(c->partsave + i);

/* Remove the context from the stack */
    SavedOmitContexts = c->next;

/* Free memory used by the saved context */
    if (c->partsave) free(c->partsave);
    if (c->fullsave) free(c->fullsave);
    free(c);

    return VerifyEoln(p);
}

/***************************************************************/
/*                                                             */
/*  IsOmitted                                                  */
/*                                                             */
/*  Return non-zero if date is OMITted, zero if it is not.     */
/*                                                             */
/***************************************************************/
int IsOmitted(int jul, int localomit)
{
    int y, m, d;

    /* Is it omitted because of local omits? */
    if (localomit & (1 << (jul % 7))) return 1;

    /* Is it omitted because of fully-specified omits? */
    if (BexistsIntArray(FullOmitArray, NumFullOmits, jul)) return 1;

    /* Get the syndrome */
    FromJulian(jul, &y, &m, &d);
    if (BexistsIntArray(PartialOmitArray, NumPartialOmits, (m << 5) + d))
	return 1;

    /* Not omitted */
    return 0;
}

/***************************************************************/
/*                                                             */
/*  BexistsIntArray                                            */
/*                                                             */
/*  Perform a binary search on an integer array.  Return 1 if  */
/*  element is found, 0 otherwise.                             */
/*                                                             */
/***************************************************************/
static int BexistsIntArray(int array[], int num, int key)
{
    int top=num-1, bot=0, mid;

    while (top >= bot) {
	mid = (top+bot)/2;
	if (array[mid] == key) return 1;
	else if (array[mid] > key) top = mid-1;
	else bot=mid+1;
    }
    return 0;
}

/***************************************************************/
/*                                                             */
/*  InsertIntoSortedArray                                      */
/*                                                             */
/*  Insert a key into a sorted array.  We assume that there is */
/*  room in the array for it.                                  */
/*                                                             */
/***************************************************************/
static void InsertIntoSortedArray(int *array, int num, int key)
{
    /* num is number of elements CURRENTLY in the array. */
    int *cur = array+num;

    while (cur > array && *(cur-1) > key) {
	*cur = *(cur-1);
	cur--;
    }
    *cur = key;
}

/***************************************************************/
/*                                                             */
/*  DoOmit                                                     */
/*                                                             */
/*  Do a global OMIT command.                                  */
/*                                                             */
/***************************************************************/
int DoOmit(ParsePtr p)
{
    int y = NO_YR, m = NO_MON, d = NO_DAY, r;
    Token tok;
    int parsing=1;
    int syndrome;

    DynamicBuffer buf;
    DBufInit(&buf);

/* Parse the OMIT.  We need a month and day; year is optional. */
    while(parsing) {
	if ( (r=ParseToken(p, &buf)) ) return r;
	FindToken(DBufValue(&buf), &tok);
	switch (tok.type) {
	case T_Year:
	    DBufFree(&buf);
	    if (y != NO_YR) return E_YR_TWICE;
	    y = tok.val;
	    break;

	case T_Month:
	    DBufFree(&buf);
	    if (m != NO_MON) return E_MON_TWICE;
	    m = tok.val;
	    break;

	case T_Day:
	    DBufFree(&buf);
	    if (d != NO_DAY) return E_DAY_TWICE;
	    d = tok.val;
	    break;
	 
	case T_Delta:
	    DBufFree(&buf);
	    break;

	case T_Empty:
	case T_Comment:
	case T_RemType:
	case T_Priority:
	case T_Tag:
	case T_Duration:
	    DBufFree(&buf);
	    parsing = 0;
	    break;

	default:
	    Eprint("%s: `%s' (OMIT)", ErrMsg[E_UNKNOWN_TOKEN],
		   DBufValue(&buf));
	    DBufFree(&buf);
	    return E_UNKNOWN_TOKEN;
	}
    }
    if (m == NO_MON || d == NO_DAY) return E_SPEC_MON_DAY;

    if (y == NO_YR) {
	if (NumPartialOmits == MAX_PARTIAL_OMITS) return E_2MANY_PART;

	if (d > MonthDays[m]) return E_BAD_DATE;
	syndrome = (m<<5) + d;
	if (!BexistsIntArray(PartialOmitArray, NumPartialOmits, syndrome)) {
	    InsertIntoSortedArray(PartialOmitArray, NumPartialOmits, syndrome);
	    NumPartialOmits++;
	}
    } else {
	if (NumFullOmits == MAX_FULL_OMITS) return E_2MANY_FULL;

	if (d > DaysInMonth(m, y)) return E_BAD_DATE;
	syndrome = Julian(y, m, d);
	if (!BexistsIntArray(FullOmitArray, NumFullOmits, syndrome)) {
	    InsertIntoSortedArray(FullOmitArray, NumFullOmits, syndrome);
	    NumFullOmits++;
	}
    }
    if (tok.type == T_RemType || tok.type == T_Priority) return E_PARSE_AS_REM;
    return OK;

}
