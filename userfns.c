/***************************************************************/
/*                                                             */
/*  USERFNS.C                                                  */
/*                                                             */
/*  This file contains the routines to support user-defined    */
/*  functions.                                                 */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1997 by David F. Skoll                  */
/*                                                             */
/***************************************************************/

#include "config.h"
static char const RCSID[] = "$Id: userfns.c,v 1.4 1997-03-30 19:07:51 dfs Exp $";

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <ctype.h>
#include "types.h"
#include "globals.h"
#include "protos.h"
#include "err.h"
#include "expr.h"

#define FUNC_HASH_SIZE 32   /* Size of User-defined function hash table */

/* Define the data structure used to hold a user-defined function */
typedef struct udf_struct {
    struct udf_struct *next;
    char name[VAR_NAME_LEN+1];
    char *text;
    Var *locals;
    char IsCached;
    char IsActive;
    int nargs;
} UserFunc;

/* The hash table */
static UserFunc *FuncHash[FUNC_HASH_SIZE];

/* Access to built-in functions */
extern int NumFuncs;
extern Operator Func[];

/* We need access to the expression evaluation stack */
extern Value ValStack[];
extern int ValStackPtr;

PRIVATE void DestroyUserFunc ARGS ((UserFunc *f));
PRIVATE void FUnset ARGS ((char *name));
PRIVATE void FSet ARGS ((UserFunc *f));
PRIVATE int SetUpLocalVars ARGS ((UserFunc *f));
PRIVATE void DestroyLocalVals ARGS ((UserFunc *f));

/***************************************************************/
/*                                                             */
/*  DoFset                                                     */
/*                                                             */
/*  Define a user-defined function - the FSET command.         */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int DoFset(ParsePtr p)
#else
int DoFset(p)
ParsePtr p;
#endif
{
    int r;
    int c;
    UserFunc *func;
    Var *v;

    /* Get the function name */
    if ( (r=ParseIdentifier(p, TokBuffer)) ) return r;
    if (*TokBuffer == '$') return E_BAD_ID;

    /* Should be followed by '(' */
    c = ParseNonSpaceChar(p, &r, 0);
    if (r) return r;
    if (c != '(') return E_PARSE_ERR;

    func = NEW(UserFunc);
    if (!func) return E_NO_MEM;
    StrnCpy(func->name, TokBuffer, VAR_NAME_LEN);
    if (!Hush) {
	if (FindFunc(TokBuffer, Func, NumFuncs)) {
	    Eprint("%s: `%s'", ErrMsg[E_REDEF_FUNC],
		   TokBuffer);
	}
    }
    func->locals = NULL;
    func->text = NULL;
    func->IsCached = 1;
    func->IsActive = 0;
    func->nargs = 0;

    /* Get the local variables - we insert the local variables in REVERSE
       order, but that's OK, because we pop them off the stack in reverse
       order, too, so everything works out just fine. */

    c=ParseNonSpaceChar(p, &r, 1);
    if (r) return r;
    if (c == ')') {
	(void) ParseNonSpaceChar(p, &r, 0);
    }
    else {
	while(1) {
	    if ( (r=ParseIdentifier(p, TokBuffer)) ) return r;
	    if (*TokBuffer == '$') return E_BAD_ID;
	    v = NEW(Var);
	    func->nargs++;
	    v->v.type = ERR_TYPE;
	    if (!v) {
		DestroyUserFunc(func);
		return E_NO_MEM;
	    }
	    StrnCpy(v->name, TokBuffer, VAR_NAME_LEN);
	    v->next = func->locals;
	    func->locals = v;
	    c = ParseNonSpaceChar(p, &r, 0);
	    if (c == ')') break;
	    else if (c != ',') {
		DestroyUserFunc(func);
		return E_PARSE_ERR;
	    }
	}
    }

    /* Copy the text over */
    if (p->isnested) {
	Eprint("%s", ErrMsg[E_CANTNEST_FDEF]);
	DestroyUserFunc(func);
	return E_PARSE_ERR;
    }

    /* A bit of trickery here - if the definition is already cached,
       no point in copying it. */
    if (CurLine != LineBuffer) {
	func->IsCached = 1;
	func->text = p->pos;
    } else {
	func->IsCached = 0;
	func->text = StrDup(p->pos);
	if (!func->text) {
	    DestroyUserFunc(func);
	    return E_NO_MEM;
	}
    }

    /* If an old definition of this function exists, destroy it */
    FUnset(func->name);

    /* Add the function definition */
    FSet(func);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  DestroyUserFunc                                            */
/*                                                             */
/*  Free up all the resources used by a user-defined function. */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE void DestroyUserFunc(UserFunc *f)
#else
static void DestroyUserFunc(f)
UserFunc *f;
#endif
{
    Var *v, *prev;

    /* Free the local variables first */
    v = f->locals;
    while(v) {
	DestroyValue(v->v);
	prev = v;
	v = v->next;
	free(prev);
    }

    /* Free the function definition */
    if (f->text && !f->IsCached) free(f->text);

    /* Free the data structure itself */
    free(f);
}

/***************************************************************/
/*                                                             */
/*  FUnset                                                     */
/*                                                             */
/*  Delete the function definition with the given name, if     */
/*  it exists.                                                 */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE void FUnset(char *name)
#else
static void FUnset(name)
char *name;
#endif
{
    UserFunc *cur, *prev;
    int h;

    h = HashVal(name) % FUNC_HASH_SIZE;

    cur = FuncHash[h];
    prev = NULL;
    while(cur) {
	if (! StrinCmp(name, cur->name, VAR_NAME_LEN)) break;
	prev = cur;
	cur = cur->next;
    }
    if (!cur) return;
    if (prev) prev->next = cur->next; else FuncHash[h] = cur->next;
    DestroyUserFunc(cur);
}

/***************************************************************/
/*                                                             */
/*  FSet                                                       */
/*                                                             */
/*  Insert a user-defined function into the hash table.        */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE void FSet(UserFunc *f)
#else
static void FSet(f)
UserFunc *f;
#endif
{
    int h = HashVal(f->name) % FUNC_HASH_SIZE;
    f->next = FuncHash[h];
    FuncHash[h] = f;
}

/***************************************************************/
/*                                                             */
/*  CallUserFunc                                               */
/*                                                             */
/*  Call a user-defined function.                              */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int CallUserFunc(char *name, int nargs)
#else
int CallUserFunc(name, nargs)
char *name;
int nargs;
#endif
{
    UserFunc *f;
    int h = HashVal(name) % FUNC_HASH_SIZE;
    int i;
    char *s;

    /* Search for the function */
    f = FuncHash[h];
    while (f && StrinCmp(name, f->name, VAR_NAME_LEN)) f = f->next;
    if (!f) {
	Eprint("%s: `%s'", ErrMsg[E_UNDEF_FUNC], name);
	return E_UNDEF_FUNC;
    }
    /* Debugging stuff */
    if (DebugFlag & DB_PRTEXPR) {
	fprintf(ErrFp, "%s %s(", ErrMsg[E_ENTER_FUN], f->name);
	for (i=0; i<nargs; i++) {
	    PrintValue(&ValStack[ValStackPtr - nargs + i], ErrFp);
	    if (i<nargs-1) fprintf(ErrFp, ", ");
	}
	fprintf(ErrFp, ")\n");
    }
    /* Detect illegal recursive call */
    if (f->IsActive) {
	if (DebugFlag &DB_PRTEXPR) {
	    fprintf(ErrFp, "%s %s() => ", ErrMsg[E_LEAVE_FUN], name);
	    fprintf(ErrFp, "%s\n", ErrMsg[E_RECURSIVE]);
	}
	return E_RECURSIVE;
    }
   
    /* Check number of args */
    if (nargs != f->nargs) {
	if (DebugFlag &DB_PRTEXPR) {
	    fprintf(ErrFp, "%s %s() => ", ErrMsg[E_LEAVE_FUN], name);
	    fprintf(ErrFp, "%s\n",
		    ErrMsg[(nargs < f->nargs) ? E_2FEW_ARGS : E_2MANY_ARGS]);
	}
	return (nargs < f->nargs) ? E_2FEW_ARGS : E_2MANY_ARGS;
    }
    /* Found the function - set up a local variable frame */
    h = SetUpLocalVars(f);
    if (h) {
	if (DebugFlag &DB_PRTEXPR) {
	    fprintf(ErrFp, "%s %s() => ", ErrMsg[E_LEAVE_FUN], name);
	    fprintf(ErrFp, "%s\n", ErrMsg[h]);
	}
	return h;
    }

    /* Evaluate the expression */
    f->IsActive = 1;
    s = f->text;

    /* Skip the opening bracket, if there's one */
    while (isspace(*s)) s++;
    if (*s == BEG_OF_EXPR) s++;
    h = Evaluate(&s, f->locals);
    f->IsActive = 0;
    DestroyLocalVals(f);
    if (DebugFlag &DB_PRTEXPR) {
	fprintf(ErrFp, "%s %s() => ", ErrMsg[E_LEAVE_FUN], name);
	if (h) fprintf(ErrFp, "%s\n", ErrMsg[h]);
	else {
	    PrintValue(&ValStack[ValStackPtr-1], ErrFp);
	    fprintf(ErrFp, "\n");
	}
    }
    return h;
}

/***************************************************************/
/*                                                             */
/*  SetUpLocalVars                                             */
/*                                                             */
/*  Set up the local variables from the stack frame.           */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int SetUpLocalVars(UserFunc *f)
#else
static int SetUpLocalVars(f)
UserFunc *f;
#endif
{
    int i, r;
    Var *var;

    for (i=0, var=f->locals; var && i<f->nargs; var=var->next, i++) {
	if ( (r=FnPopValStack(&(var->v))) ) {
	    DestroyLocalVals(f);
	    return r;
	}
    }
    return OK;
}

/***************************************************************/
/*                                                             */
/*  DestroyLocalVals                                           */
/*                                                             */
/*  Destroy the values of all local variables after evaluating */
/*  the function.                                              */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE void DestroyLocalVals(UserFunc *f)
#else
static void DestroyLocalVals(f)
UserFunc *f;
#endif
{
    Var *v = f->locals;

    while(v) {
	DestroyValue(v->v);
	v = v->next;
    }
}
/***************************************************************/
/*                                                             */
/*  UserFuncExists                                             */
/*                                                             */
/*  Return the number of arguments accepted by the function if */
/*  it is defined, or -1 if it is not defined.                 */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int UserFuncExists(char *fn)
#else
int UserFuncExists(fn)
char *fn;
#endif
{
    UserFunc *f;
    int h = HashVal(fn) % FUNC_HASH_SIZE;

    f = FuncHash[h];
    while (f && StrinCmp(fn, f->name, VAR_NAME_LEN)) f = f->next;
    if (!f) return -1;
    else return f->nargs;
}

