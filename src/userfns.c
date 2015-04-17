/***************************************************************/
/*                                                             */
/*  USERFNS.C                                                  */
/*                                                             */
/*  This file contains the routines to support user-defined    */
/*  functions.                                                 */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by Dianne Skoll                    */
/*  Copyright (C) 1999-2000 by Roaring Penguin Software Inc.   */
/*                                                             */
/***************************************************************/

#include "config.h"

#include <stdio.h>
#include <ctype.h>

#include <stdlib.h>
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
    char const *text;
    Var *locals;
    char IsActive;
    int nargs;
} UserFunc;

/* The hash table */
static UserFunc *FuncHash[FUNC_HASH_SIZE];

/* Access to built-in functions */
extern int NumFuncs;
extern BuiltinFunc Func[];

/* We need access to the expression evaluation stack */
extern Value ValStack[];
extern int ValStackPtr;

static void DestroyUserFunc (UserFunc *f);
static void FUnset (char const *name);
static void FSet (UserFunc *f);
static int SetUpLocalVars (UserFunc *f);
static void DestroyLocalVals (UserFunc *f);

/***************************************************************/
/*                                                             */
/*  DoFset                                                     */
/*                                                             */
/*  Define a user-defined function - the FSET command.         */
/*                                                             */
/***************************************************************/
int DoFset(ParsePtr p)
{
    int r;
    int c;
    UserFunc *func;
    Var *v;

    DynamicBuffer buf;
    DBufInit(&buf);

    /* Get the function name */
    if ( (r=ParseIdentifier(p, &buf)) ) return r;
    if (*DBufValue(&buf) == '$') {
	DBufFree(&buf);
	return E_BAD_ID;
    }

    /* Should be followed by '(' */
    c = ParseNonSpaceChar(p, &r, 0);
    if (r) {
	DBufFree(&buf);
	return r;
    }
    if (c != '(') {
	DBufFree(&buf);
	return E_PARSE_ERR;
    }

    func = NEW(UserFunc);
    if (!func) {
	DBufFree(&buf);
	return E_NO_MEM;
    }
    StrnCpy(func->name, DBufValue(&buf), VAR_NAME_LEN);
    DBufFree(&buf);
    if (!Hush) {
	if (FindFunc(DBufValue(&buf), Func, NumFuncs)) {
	    Eprint("%s: `%s'", ErrMsg[E_REDEF_FUNC],
		   DBufValue(&buf));
	}
    }
    func->locals = NULL;
    func->text = NULL;
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
	    if ( (r=ParseIdentifier(p, &buf)) ) return r;
	    if (*DBufValue(&buf) == '$') {
		DBufFree(&buf);
		return E_BAD_ID;
	    }
	    v = NEW(Var);
	    if (!v) {
		DBufFree(&buf);
		DestroyUserFunc(func);
		return E_NO_MEM;
	    }
	    func->nargs++;
	    v->v.type = ERR_TYPE;
	    StrnCpy(v->name, DBufValue(&buf), VAR_NAME_LEN);
	    DBufFree(&buf);
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

    /* Allow an optional = sign: FSET f(x) = x*x */
    c = ParseNonSpaceChar(p, &r, 1);
    if (c == '=') {
	c = ParseNonSpaceChar(p, &r, 0);
    }
    /* Copy the text over */
    if (p->isnested) {
	Eprint("%s", ErrMsg[E_CANTNEST_FDEF]);
	DestroyUserFunc(func);
	return E_PARSE_ERR;
    }

    func->text = StrDup(p->pos);
    if (!func->text) {
	DestroyUserFunc(func);
	return E_NO_MEM;
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
static void DestroyUserFunc(UserFunc *f)
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
    if (f->text) free( (char *) f->text);

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
static void FUnset(char const *name)
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
static void FSet(UserFunc *f)
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
int CallUserFunc(char const *name, int nargs, ParsePtr p)
{
    UserFunc *f;
    int h = HashVal(name) % FUNC_HASH_SIZE;
    int i;
    char const *s;

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
    while (isempty(*s)) s++;
    if (*s == BEG_OF_EXPR) s++;
    h = Evaluate(&s, f->locals, p);
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
static int SetUpLocalVars(UserFunc *f)
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
static void DestroyLocalVals(UserFunc *f)
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
int UserFuncExists(char const *fn)
{
    UserFunc *f;
    int h = HashVal(fn) % FUNC_HASH_SIZE;

    f = FuncHash[h];
    while (f && StrinCmp(fn, f->name, VAR_NAME_LEN)) f = f->next;
    if (!f) return -1;
    else return f->nargs;
}

