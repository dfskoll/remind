/***************************************************************/
/*                                                             */
/*  EXPR.C                                                     */
/*                                                             */
/*  This file contains routines to parse and evaluate          */
/*  expressions.                                               */
/*                                                             */
/*  Copyright 1992-1996 by David F. Skoll                      */
/*                                                             */
/***************************************************************/

static char const RCSID[] = "$Id: expr.c,v 1.3 1996-04-28 02:01:55 dfs Exp $";

#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include "err.h"
#include "types.h"
#include "expr.h"
#include "protos.h"
#include "globals.h"

#define ISID(c) (isalnum(c) || (c) == '_')
#define EQ 0
#define GT 1
#define LT 2
#define GE 3
#define LE 4
#define NE 5

static char ExprBuf[TOKSIZE+1];
static char CoerceBuf[TOKSIZE+1];
extern int NumFuncs;

#ifdef HAVE_PROTOS
PRIVATE int Multiply(void), Divide(void), Mod(void), Add(void),
    Subtract(void), GreaterThan(void), LessThan(void),
    EqualTo(void), NotEqual(void), LessOrEqual(void),
    GreaterOrEqual(void), LogAND(void), LogOR(void),
    UnMinus(void), LogNot(void),
    Compare(int);
#else
PRIVATE int Multiply(), Divide(), Mod(), Add(),
    Subtract(), GreaterThan(), LessThan(),
    EqualTo(), NotEqual(), LessOrEqual(),
    GreaterOrEqual(), LogAND(), LogOR(),
    UnMinus(), LogNot(), Compare();
#endif

PRIVATE int MakeValue ARGS ((char *s, Value *v, Var *locals));
PRIVATE int ParseLiteralDate ARGS ((char **s, int *jul));

/* Binary operators - all left-associative */

/* Make SURE they are sorted lexically... this may die on an EBCDIC
   system... */

Operator BinOp[] = {
    { "!=", 15, BIN_OP, NotEqual },
    { "%", 20, BIN_OP, Mod },
    { "&&", 14, BIN_OP, LogAND },
    { "*", 20, BIN_OP, Multiply },
    { "+", 18, BIN_OP, Add },
    { "-", 18, BIN_OP, Subtract },
    { "/", 20, BIN_OP, Divide },
    { "<", 16, BIN_OP, LessThan },
    { "<=", 16, BIN_OP, LessOrEqual },
    { "==", 15, BIN_OP, EqualTo },
    { ">", 16, BIN_OP, GreaterThan },
    { ">=", 16, BIN_OP, GreaterOrEqual },
    { "||", 12, BIN_OP, LogOR },
};
#define NUM_BIN_OPS (sizeof(BinOp) / sizeof(Operator))

/* These ones must be sorted too. */
Operator UnOp[] = {
    { "!", 22, UN_OP, LogNot },
    { "-", 22, UN_OP, UnMinus },
};
#define NUM_UN_OPS (sizeof(UnOp) / sizeof(Operator))

/* Functions have the same definitions as operators, except the prec field
   is used to indicate how many arguments are needed. */
extern Operator Func[];

Operator OpStack[OP_STACK_SIZE];
Value    ValStack[VAL_STACK_SIZE];
int OpStackPtr, ValStackPtr;

/***************************************************************/
/*                                                             */
/*  DebugPerform                                               */
/*                                                             */
/*  Execute an operator or function with debugging.            */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int DebugPerform(Operator *op)
#else
static int DebugPerform(op)
Operator *op;
#endif
{
    int r;

    if (op->type == UN_OP) {
	fprintf(ErrFp, "%s ", op->name);
	PrintValue(&ValStack[ValStackPtr-1], ErrFp);
    } else { /* Must be binary operator */
	PrintValue(&ValStack[ValStackPtr-2], ErrFp);
	fprintf(ErrFp, " %s ", op->name);
	PrintValue(&ValStack[ValStackPtr-1], ErrFp);
    }

    r = (op->func)();
    fprintf(ErrFp, " => ");
    if (!r) {
	PrintValue(&ValStack[ValStackPtr-1], ErrFp);
	Putc('\n', ErrFp);
    } else {
	fprintf(ErrFp, "%s\n", ErrMsg[r]);
    }
    return r;
}

/***************************************************************/
/*                                                             */
/*  CleanStack                                                 */
/*                                                             */
/*  Clean the stack after an error occurs.                     */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE void CleanStack(void)
#else
static void CleanStack()
#endif
{
    int i;

    for (i=0; i<ValStackPtr; i++) DestroyValue(ValStack[i]);
    ValStackPtr = 0;
}

/***************************************************************/
/*                                                             */
/*  PeekChar - peek ahead to next char.                        */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE char PeekChar(char **s)
#else
static char PeekChar(s)
char **s;
#endif
{
    char *t = *s;
    while (*t && isspace(*t)) t++;
    return *t;
}

/***************************************************************/
/*                                                             */
/*  ParseExprToken                                             */
/*                                                             */
/*  Read a token.                                              */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int ParseExprToken(char *out, char **in)
#else
static int ParseExprToken(out, in)
char *out;
char **in;
#endif
{

    char c;
   
    *out = 0;
/* Skip white space */
    while (**in && isspace(**in)) (*in)++;
   
    if (!**in) return OK;

    *out++ = c = *(*in)++;
    *out = 0;

    switch(c) {
    case COMMA:
    case END_OF_EXPR:
    case '+':
    case '-':
    case '*':
    case '/':
    case '(':
    case ')':
    case '%': return OK;

    case '&':
    case '|':
    case '=': if (**in == c) {
	*out++ = c;
	*out = 0;
	(*in)++;
    }
    return OK;
      	   
    case '!':
    case '>':
    case '<': if (**in == '=') {
	*out++ = '=';
	*out = 0;
	(*in)++;
    }
    return OK;
    }           

    /* Handle the parsing of quoted strings */
    if (c == '\"') {
	if (!**in) return E_MISS_QUOTE;
	while (**in) if ((c = *out++ = *(*in)++) == '\"') break;
	*out = 0;
	if (c == '\"') return OK ; else return E_MISS_QUOTE;
    }

    /* Dates can be specified with single-quotes */
    if (c == '\'') {
	if (!**in) return E_MISS_QUOTE;
	while (**in) if ((c = *out++ = *(*in)++) == '\'') break;
	*out = 0;
	if (c == '\'') return OK ; else return E_MISS_QUOTE;
    }

    if (!ISID(c) && c != '$') {
	Eprint("%s `%c'", ErrMsg[E_ILLEGAL_CHAR], c);
	return E_ILLEGAL_CHAR;
    }

    /* Parse a constant, variable name or function */
    while (ISID(**in) || **in == ':' || **in == '.' || **in == TIMESEP)
	*out++ = *(*in)++;

    /* Chew up any remaining white space */
    while (**in && isspace(**in)) (*in)++;

    /* Peek ahead - is it '('?  Then we have a function call */
    if (**in == '(') *out++ = *(*in)++;

    *out = 0;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  EvalExpr                                                   */
/*  Evaluate an expression.  Return 0 if OK, non-zero if error */
/*  Put the result into value pointed to by v.                 */
/*                                                             */
/***************************************************************/
#ifdef HaveProtos
PUBLIC int EvalExpr(char **e, Value *v)
#else
int EvalExpr(e, v)
char **e;
Value *v;
#endif
{
    int r;

    OpStackPtr = 0;
    ValStackPtr = 0;
    r = Evaluate(e, NULL);

    /* Put last character parsed back onto input stream */
    if (*ExprBuf) (*e)--;

    if (r) {
	CleanStack();
	return r;
    }
    *v = *ValStack;
    ValStack[0].type = ERR_TYPE;
    return r;
}

/* Evaluate - do the actual work of evaluation. */
#ifdef HAVE_PROTOS
PUBLIC int Evaluate(char **s, Var *locals)
#else
int Evaluate(s, locals)
char **s;
Var *locals;
#endif
{
    int OpBase, ValBase;
    int r;
    Operator *f;
    int args; /* Number of function arguments */
    Operator op, op2;
    Value va;
    char *ufname = NULL; /* Stop GCC from complaining about use of uninit var */
   
    OpBase = OpStackPtr;
    ValBase = ValStackPtr;
   
    while(1) {
/* Looking for a value.  Accept: value, unary op, func. call or left paren */
	r = ParseExprToken(ExprBuf, s);
	if (r) return r;
	if (!*ExprBuf) return E_EOLN;

	if (*ExprBuf == '(') { /* Parenthesized expression */
	    r = Evaluate(s, locals);  /* Leaves the last parsed token in ExprBuf */
	    if (r) return r;
	    if (*ExprBuf != ')') return E_MISS_RIGHT_PAREN;
	} else if (*ExprBuf == '+') continue; /* Ignore unary + */
	else if (*(ExprBuf + strlen(ExprBuf) -1) == '(') { /* Function Call */
	    *(ExprBuf + strlen(ExprBuf) - 1) = 0;
	    f = FindFunc(ExprBuf, Func, NumFuncs);
	    if (!f) {
		ufname = StrDup(ExprBuf);
		if (!ufname) return E_NO_MEM;
	    }
	    args = 0;
	    if (PeekChar(s) == ')') { /* Function has no arguments */
		if (f) r = CallFunc(f, 0);
		else {
		    r = CallUserFunc(ufname, 0);
		    free(ufname);
		}
		if (r) return r;
		(void) ParseExprToken(ExprBuf, s); /* Guaranteed to be right paren. */
	    } else { /* Function has some arguments */
		while(1) {
		    args++;
		    r = Evaluate(s, locals);
		    if (r) {
			if (!f) free(ufname);
			return r;
		    }
		    if (*ExprBuf == ')') break;
		    else if (*ExprBuf != ',') {
			if (!f) free(ufname);
			Eprint("%s: `%c'", ErrMsg[E_EXPECT_COMMA], *ExprBuf);
			return E_EXPECT_COMMA;
		    }
		}
		if (f) r = CallFunc(f, args);
		else {
		    r = CallUserFunc(ufname, args);
		    free(ufname);
		}
		if (r) return r;
	    }
	} else { /* Unary operator */
	    f = FindFunc(ExprBuf, UnOp, NUM_UN_OPS);
	    if (f) {
		PushOpStack(*f);
		continue;  /* Still looking for an atomic vlue */
	    } else if (!ISID(*ExprBuf) && *ExprBuf != '$' 
		       && *ExprBuf != '"' && *ExprBuf != '\'') {
		Eprint("%s `%c'", ErrMsg[E_ILLEGAL_CHAR], *ExprBuf);
		return E_ILLEGAL_CHAR;
	    } else { /* Must be a literal value */
		r = MakeValue(ExprBuf, &va, locals);
		if (r) return r;
		PushValStack(va);
	    }
	}
/* OK, we've got a literal value; now, we're looking for the end of the
   expression, or a binary operator. */
	r = ParseExprToken(ExprBuf, s);
	if (r) return r;
	if (*ExprBuf == 0 || *ExprBuf == ',' || *ExprBuf == ']' || *ExprBuf == ')') {
	    /* We've hit the end of the expression.  Pop off and evaluate until
	       OpStackPtr = OpBase and ValStackPtr = ValBase+1 */
	    while (OpStackPtr > OpBase) {
		PopOpStack(op);
		if (DebugFlag & DB_PRTEXPR)
		    r=DebugPerform(&op);
		else
		    r=(op.func)();
		if (r) {
		    Eprint("`%s': %s", op.name, ErrMsg[r]);
		    return r;
		}
	    }
	    if (ValStackPtr != ValBase+1) return E_STACK_ERR; else return OK;
	}
	/* Must be a binary operator */
	f = FindFunc(ExprBuf, BinOp, NUM_BIN_OPS);
	if (!f) return E_EXPECTING_BINOP;

	/* While operators of higher or equal precedence are on the stack,
	   pop them off and evaluate */
	while (OpStackPtr > OpBase && OpStack[OpStackPtr-1].prec >= f->prec) {
	    PopOpStack(op2);
	    if (r) return r;
	    if (DebugFlag & DB_PRTEXPR)
		r=DebugPerform(&op2);
	    else
		r=(op2.func)();
	    if (r) {
		Eprint("`%s': %s", op2.name, ErrMsg[r]);
		return r;
	    }
	}
	PushOpStack(*f);
    }
}
   
/***************************************************************/
/*                                                             */
/*  MakeValue                                                  */
/*  Generate a literal value.  It's either a string, a number, */
/*  a date or the value of a symbol.                           */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int MakeValue(char *s, Value *v, Var *locals)
#else
static int MakeValue(s, v, locals)
char *s;
Value *v;
Var *locals;
#endif
{
    int len;
    int h, m, r;

    if (*s == '\"') { /* It's a literal string */
	len = strlen(s)-1;
	v->type = STR_TYPE;
	v->v.str = (char *) malloc(len);
	if (! v->v.str) {
	    v->type = ERR_TYPE;
	    return E_NO_MEM;
	}
	strncpy(v->v.str, s+1, len-1);
	*(v->v.str+len-1) = 0;
	return OK;
    } else if (*s == '\'') { /* It's a literal date */
	s++;
	if ((r=ParseLiteralDate(&s, &h))) return r;
	if (*s != '\'') return E_BAD_DATE;
	v->type = DATE_TYPE;
	v->v.val = h;
	return OK;
    } else if (isdigit(*s)) { /* It's a number - use len to hold it.*/
	len = 0;
	while (*s && isdigit(*s)) {
	    len *= 10;
	    len += (*s++ - '0');
	}
	if (*s == ':' || *s == '.' || *s == TIMESEP) { /* Must be a literal time */
	    s++;
	    if (!isdigit(*s)) return E_BAD_TIME;
	    h = len;
	    m = 0;
	    while (isdigit(*s)) {
		m *= 10;
		m += *s - '0';
		s++;
	    }
	    if (*s || h>23 || m>59) return E_BAD_TIME;
	    v->type = TIM_TYPE;
	    v->v.val = h*60 + m;
	    return OK;
	}
	/* Not a time - must be a number */
	if (*s) return E_BAD_NUMBER;
	v->type = INT_TYPE;
	v->v.val = len;
	return OK;
    } else if (*s == '$') { /* A system variable */
	if (DebugFlag & DB_PRTEXPR)
	    fprintf(ErrFp, "%s => ", s);
	r = GetSysVar(s+1, v);
   
	if (! (DebugFlag & DB_PRTEXPR)) return r;
	if (r == OK) {
	    PrintValue(v, ErrFp);
	    Putc('\n', ErrFp);
	}
	return r;
    } else /* Must be a symbol */
	if (DebugFlag & DB_PRTEXPR)
	    fprintf(ErrFp, "%s => ", s);
    r = GetVarValue(s, v, locals);
    if (! (DebugFlag & DB_PRTEXPR)) return r;
    if (r == OK) {
        PrintValue(v, ErrFp);
	Putc('\n', ErrFp);
    }
    return r;
}

/***************************************************************/
/*                                                             */
/*  DoCoerce - actually coerce a value to the specified type.  */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int DoCoerce(char type, Value *v)
#else
int DoCoerce(type, v)
char type;
Value *v;
#endif
{
    int h, d, m, y, i;
    char *s;
   
    /* Do nothing if value is already the right type */
    if (type == v->type) return OK;
   
    switch(type) {
    case STR_TYPE:
	switch(v->type) {
	case INT_TYPE: sprintf(CoerceBuf, "%d", v->v.val); break;
	case TIM_TYPE: sprintf(CoerceBuf, "%02d%c%02d", v->v.val / 60, 
			       TIMESEP, v->v.val % 60);
	break;
	case DATE_TYPE: FromJulian(v->v.val, &y, &m, &d);
	    sprintf(CoerceBuf, "%04d%c%02d%c%02d",
		    y, DATESEP, m+1, DATESEP, d);
	    break;
	default: return E_CANT_COERCE;
	}
	v->type = STR_TYPE;
	v->v.str = StrDup(CoerceBuf);
	if (!v->v.str) {
	    v->type = ERR_TYPE;
	    return E_NO_MEM;
	}
	return OK;

    case INT_TYPE:
	i = 0;
	m = 1;
	switch(v->type) {
	case STR_TYPE:
	    s = v->v.str;
	    if (*s == '-') {
		m = -1;
		s++;
	    }
	    while(*s && isdigit(*s)) {
		i *= 10;
		i += (*s++) - '0';
	    }
	    if (*s) {
		free (v->v.str);
		v->type = ERR_TYPE;
		return E_CANT_COERCE;
	    }
	    free(v->v.str);
	    v->type = INT_TYPE;
	    v->v.val = i * m;
	    return OK;

	case DATE_TYPE:
	case TIM_TYPE:
	    v->type = INT_TYPE;
	    return OK;

	default: return E_CANT_COERCE;
	}

    case DATE_TYPE:
	switch(v->type) {
	case INT_TYPE:
	    if(v->v.val >= 0) {
		v->type = DATE_TYPE;
		return OK;
	    } else return E_2LOW;

	case STR_TYPE:
	    s = v->v.str;
	    if (ParseLiteralDate(&s, &i)) return E_CANT_COERCE;
	    if (*s) return E_CANT_COERCE;
	    v->type = DATE_TYPE;
	    free(v->v.str);
	    v->v.val = i;
	    return OK;

	default: return E_CANT_COERCE;
	}

    case TIM_TYPE:
	switch(v->type) {
	case INT_TYPE:
	    v->type = TIM_TYPE;
	    v->v.val %= 1440;
	    if (v->v.val < 0) v->v.val += 1440;
	    return OK;

	case STR_TYPE:
	    h = 0;
	    m = 0;
	    s = v->v.str;
	    if (!isdigit(*s)) return E_CANT_COERCE;
	    while (isdigit(*s)) {
		h *= 10;
		h += *s++ - '0';
	    }
	    if (*s != ':' && *s != '.' && *s != TIMESEP)
		return E_CANT_COERCE;
	    s++;
	    if (!isdigit(*s)) return E_CANT_COERCE;
	    while (isdigit(*s)) {
		m *= 10;
		m += *s++ - '0';
	    }
	    if (*s || h>23 || m>59) return E_CANT_COERCE;
	    v->type = TIM_TYPE;
	    free(v->v.str);
	    v->v.val = h*60+m;
	    return OK;

	default: return E_CANT_COERCE;
	}
    default: return E_CANT_COERCE;
    }
}

/***************************************************************/
/*                                                             */
/*  Add                                                        */
/*                                                             */
/*  Perform addition.                                          */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int Add(void)
#else
static int Add()
#endif
{
    Value v1, v2, v3;
    int r;
   
    PopValStack(v2);
    if ( (r = FnPopValStack(&v1)) ) {
	DestroyValue(v2);
	return r;
    }
   
/* If both are ints, just add 'em */
    if (v2.type == INT_TYPE && v1.type == INT_TYPE) {
	v2.v.val += v1.v.val;
	PushValStack(v2);
	return OK;
    }

/* If it's a date plus an int, add 'em */
    if ((v1.type == DATE_TYPE && v2.type == INT_TYPE) ||
	(v1.type == INT_TYPE && v2.type == DATE_TYPE)) {
	v1.v.val += v2.v.val;
	if (v1.v.val < 0) return E_DATE_OVER;
	v1.type = DATE_TYPE;
	PushValStack(v1);
	return OK;
    }
   
/* If it's a time plus an int, add 'em mod 1440 */
    if ((v1.type == TIM_TYPE && v2.type == INT_TYPE) ||
	(v1.type == INT_TYPE && v2.type == TIM_TYPE)) {
	v1.v.val = (v1.v.val + v2.v.val) % 1440;
	if (v1.v.val < 0) v1.v.val += 1440;
	v1.type = TIM_TYPE;
	PushValStack(v1);
	return OK;
    }   	

/* If either is a string, coerce them both to strings and concatenate */
    if (v1.type == STR_TYPE || v2.type == STR_TYPE) {
	if ( (r = DoCoerce(STR_TYPE, &v1)) ) {
	    DestroyValue(v1); DestroyValue(v2);
	    return r;
	}
	if ( (r = DoCoerce(STR_TYPE, &v2)) ) {
	    DestroyValue(v1); DestroyValue(v2);
	    return r;
	}
	v3.type = STR_TYPE;
	v3.v.str = (char *) malloc(strlen(v1.v.str) + strlen(v2.v.str) + 1);
	if (!v3.v.str) {
	    DestroyValue(v1); DestroyValue(v2);
	    return E_NO_MEM;
	}
	strcpy(v3.v.str, v1.v.str);
	strcat(v3.v.str, v2.v.str);
	DestroyValue(v1); DestroyValue(v2);
	PushValStack(v3);
	return OK;
    }

    /* Don't handle other types yet */
    return E_BAD_TYPE;
}
      
/***************************************************************/
/*                                                             */
/*  Subtract                                                   */
/*                                                             */
/*  Perform subtraction.                                       */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int Subtract(void)
#else
static int Subtract()
#endif
{
    Value v1, v2;
    int r;
   
    PopValStack(v2);
    if ( (r = FnPopValStack(&v1)) ) {
	DestroyValue(v2);
	return r;
    }

    /* If they're both INTs, do subtraction */
    if (v1.type == INT_TYPE && v2.type == INT_TYPE) {
	v1.v.val -= v2.v.val;
	PushValStack(v1);
	return OK;
    }

    /* If it's a date minus an int, do subtraction, checking for underflow */
    if (v1.type == DATE_TYPE && v2.type == INT_TYPE) {
	v1.v.val -= v2.v.val;
	if (v1.v.val < 0) return E_DATE_OVER;
	PushValStack(v1);
	return OK;
    }

    /* If it's a time minus an int, do subtraction mod 1440 */
    if (v1.type == TIM_TYPE && v2.type == INT_TYPE) {
	v1.v.val = (v1.v.val - v2.v.val) % 1440;
	if (v1.v.val < 0) v1.v.val += 1440;
	PushValStack(v1);
	return OK;
    }

    /* If it's a time minus a time or a date minus a date, do it */
    if ((v1.type == TIM_TYPE && v2.type == TIM_TYPE) ||
	(v1.type == DATE_TYPE && v2.type == DATE_TYPE)) {
	v1.v.val -= v2.v.val;
	v1.type = INT_TYPE;
	PushValStack(v1);
	return OK;
    }

    /* Must be types illegal for subtraction */
    DestroyValue(v1); DestroyValue(v2);
    return E_BAD_TYPE;
}

/***************************************************************/
/*                                                             */
/*  Multiply                                                   */
/*                                                             */
/*  Perform multiplication.                                    */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int Multiply(void)
#else
static int Multiply()
#endif
{
    Value v1, v2;
    int r;

    PopValStack(v2);
    if ( (r = FnPopValStack(&v1)) ) {
	DestroyValue(v2);
	return r;
    }

    if (v1.type == INT_TYPE && v2.type == INT_TYPE) {
	v1.v.val *= v2.v.val;
	PushValStack(v1);
	return OK;
    }
    DestroyValue(v1); DestroyValue(v2);
    return E_BAD_TYPE;
}

/***************************************************************/
/*                                                             */
/*  Divide                                                     */
/*                                                             */
/*  Perform division.                                          */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int Divide(void)
#else
static int Divide()
#endif
{
    Value v1, v2;
    int r;

    PopValStack(v2);
    if ( (r = FnPopValStack(&v1)) ) {
	DestroyValue(v2);
	return r;
    }

    if (v1.type == INT_TYPE && v2.type == INT_TYPE) {
	if (v2.v.val == 0) return E_DIV_ZERO;
	v1.v.val /= v2.v.val;
	PushValStack(v1);
	return OK;
    }
    DestroyValue(v1); DestroyValue(v2);
    return E_BAD_TYPE;
}

/***************************************************************/
/*                                                             */
/*  Mod                                                        */
/*                                                             */
/*  Perform modulus function.                                  */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int Mod(void)
#else
static int Mod()
#endif
{
    Value v1, v2;
    int r;

    PopValStack(v2);
    if ( (r = FnPopValStack(&v1)) ) {
	DestroyValue(v2);
	return r;
    }

    if (v1.type == INT_TYPE && v2.type == INT_TYPE) {
	if (v2.v.val == 0) return E_DIV_ZERO;
	v1.v.val %= v2.v.val;
	PushValStack(v1);
	return OK;
    }
    DestroyValue(v1); DestroyValue(v2);
    return E_BAD_TYPE;
}


/***************************************************************/
/*                                                             */
/*  GreaterThan, LessThan, EqualTo, NotEqual, LessOrEqual,     */
/*  GreaterOrEqual                                             */
/*                                                             */
/*  All the comparison functions.                              */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int GreaterThan(void) {return Compare(GT);}
PRIVATE int LessThan(void) {return Compare(LT);}
PRIVATE int EqualTo(void) {return Compare(EQ);}
PRIVATE int NotEqual(void) {return Compare(NE);}
PRIVATE int LessOrEqual(void) {return Compare(LE);}
PRIVATE int GreaterOrEqual(void) {return Compare(GE);}
#else
static int GreaterThan() {return Compare(GT);}
static int LessThan() {return Compare(LT);}
static int EqualTo() {return Compare(EQ);}
static int NotEqual() {return Compare(NE);}
static int LessOrEqual() {return Compare(LE);}
static int GreaterOrEqual() {return Compare(GE);}
#endif

/***************************************************************/
/*                                                             */
/*  Compare                                                    */
/*  Do the actual work of comparison.                          */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int Compare(int how)
#else
static int Compare(how)
int how;
#endif
{
    Value v1, v2, v3;
    int r;

    PopValStack(v2);
    if ( (r = FnPopValStack(&v1)) ) {
	DestroyValue(v2);
	return r;
    }

/* Special case for EQ and NE */

    v3.type = INT_TYPE;
    if (v1.type != v2.type) {
	DestroyValue(v1); DestroyValue(v2);
	if (how == EQ) {
	    v3.v.val = 0;
	    PushValStack(v3);
	    return OK;
	} else if (how == NE) {
	    v3.v.val = 1;
	    PushValStack(v3);
	    return OK;
	} else return E_BAD_TYPE;
    }

    if (v1.type == STR_TYPE) {
	switch(how) {
	case EQ: v3.v.val = (strcmp(v1.v.str, v2.v.str) == 0); break;
	case NE: v3.v.val = (strcmp(v1.v.str, v2.v.str) != 0); break;
	case LT: v3.v.val = (strcmp(v1.v.str, v2.v.str) < 0); break;
	case GT: v3.v.val = (strcmp(v1.v.str, v2.v.str) > 0); break;
	case LE: v3.v.val = (strcmp(v1.v.str, v2.v.str) <= 0); break;
	case GE: v3.v.val = (strcmp(v1.v.str, v2.v.str) >= 0); break;
	}
    } else {
	switch(how) {
	case EQ: v3.v.val = (v1.v.val == v2.v.val); break;
	case NE: v3.v.val = (v1.v.val != v2.v.val); break;
	case LT: v3.v.val = (v1.v.val < v2.v.val); break;
	case GT: v3.v.val = (v1.v.val > v2.v.val); break;
	case LE: v3.v.val = (v1.v.val <= v2.v.val); break;
	case GE: v3.v.val = (v1.v.val >= v2.v.val); break;
	}
    }
    DestroyValue(v1); DestroyValue(v2);
    PushValStack(v3);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  LogOR                                                      */
/*                                                             */
/*  Do logical OR                                              */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int LogOR(void)
#else
static int LogOR()
#endif
{
    Value v1, v2;
    int r;

    PopValStack(v2);
    if ( (r = FnPopValStack(&v1)) ) {
	DestroyValue(v2);
	return r;
    }

    if (v1.type == INT_TYPE && v2.type == INT_TYPE) {
	v1.v.val = (v1.v.val || v2.v.val) ? 1 : 0;
	PushValStack(v1);
	return OK;
    }
    DestroyValue(v1); DestroyValue(v2);
    return E_BAD_TYPE;
}

/***************************************************************/
/*                                                             */
/*  LogAND                                                     */
/*                                                             */
/*  Do logical AND                                             */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int LogAND(void)
#else
static int LogAND()
#endif
{
    Value v1, v2;
    int r;

    PopValStack(v2);
    if ( (r = FnPopValStack(&v1)) ) {
	DestroyValue(v2);
	return r;
    }

    if (v1.type == INT_TYPE && v2.type == INT_TYPE) {
	v1.v.val = (v1.v.val && v2.v.val) ? 1 : 0;
	PushValStack(v1);
	return OK;
    }
    DestroyValue(v1); DestroyValue(v2);
    return E_BAD_TYPE;
}

/***************************************************************/
/*                                                             */
/*  UnMinus                                                    */
/*                                                             */
/*  Unary Minus                                                */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int UnMinus(void)
#else
static int UnMinus()
#endif
{
    Value *v = &ValStack[ValStackPtr-1];
    if (v->type != INT_TYPE) return E_BAD_TYPE;
    v->v.val = -v->v.val;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  LogNot                                                     */
/*                                                             */
/*  Logical NOT                                                */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int LogNot(void)
#else
static int LogNot()
#endif
{
    Value *v = &ValStack[ValStackPtr-1];
    if (v->type != INT_TYPE) return E_BAD_TYPE;
    if (v->v.val) v->v.val = 0; else v->v.val = 1;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  FindFunc                                                   */
/*                                                             */
/*  Find a function.                                           */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
Operator *FindFunc(char *name, Operator where[], int num)
#else
Operator *FindFunc(name, where, num)
char *name;
Operator where[];
int num;
#endif
{
    int top=num-1, bot=0;
    int mid, r;
    while (top >= bot) {
	mid = (top + bot) / 2;
	r = StrCmpi(name, where[mid].name);
	if (!r) return &where[mid];
	else if (r > 0) bot = mid+1;
	else top = mid-1;
    }
    return NULL;
}
	
/***************************************************************/
/*                                                             */
/*  PrintValue                                                 */
/*                                                             */
/*  Print a value to stdout for debugging purposes.            */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC void PrintValue (Value *v, FILE *fp)
#else
void PrintValue(v, fp)
Value *v;
FILE *fp;
#endif
{
    int y, m, d;
    char *s;

    if (v->type == STR_TYPE) {
	s=v->v.str;
	Putc('"', fp);
	for (y=0; y<MAX_PRT_LEN && *s; y++) Putc(*s++, fp);
	Putc('"',fp);
	if (*s) fprintf(fp, "...");
    }      
    else if (v->type == INT_TYPE) fprintf(fp, "%d", v->v.val);
    else if (v->type == TIM_TYPE) fprintf(fp, "%02d%c%02d", v->v.val / 60, 
					  TIMESEP, v->v.val % 60);
    else if (v->type == DATE_TYPE) {
	FromJulian(v->v.val, &y, &m, &d);
	fprintf(fp, "%04d%c%02d%c%02d", y, DATESEP, m+1, DATESEP, d);
    }
    else fprintf(fp, "ERR");
}

/***************************************************************/
/*                                                             */
/*  CopyValue                                                  */
/*                                                             */
/*  Copy a value.                                              */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int CopyValue(Value *dest, const Value *src)
#else
int CopyValue(dest, src)
Value *dest, *src;
#endif
{
    dest->type = ERR_TYPE;
    if (src->type == STR_TYPE) {
	dest->v.str = StrDup(src->v.str);
	if (!dest->v.str) return E_NO_MEM;
    } else {
	dest->v.val = src->v.val;
    }
    dest->type = src->type;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  ParseLiteralDate                                           */
/*                                                             */
/*  Parse a literal date.  Return result in jul, update s.     */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int ParseLiteralDate(char **s, int *jul)
#else
static int ParseLiteralDate(s, jul)
char **s;
int *jul;
#endif
{
    int y, m, d;

    y=0; m=0; d=0;

    if (!isdigit(**s)) return E_BAD_DATE;
    while (isdigit(**s)) {
	y *= 10;
	y += *(*s)++ - '0';
    }
    if (**s != '/' && **s != '-' && **s != DATESEP) return E_BAD_DATE;
    (*s)++;
    if (!isdigit(**s)) return E_BAD_DATE;
    while (isdigit(**s)) {
	m *= 10;
	m += *(*s)++ - '0';
    }
    m--;
    if (**s != '/' && **s != '-' && **s != DATESEP) return E_BAD_DATE;
    (*s)++;
    if (!isdigit(**s)) return E_BAD_DATE;
    while (isdigit(**s)) {
	d *= 10;
	d += *(*s)++ - '0';
    }
    if (!DateOK(y, m, d)) return E_BAD_DATE;
   
    *jul = Julian(y, m, d);

    return OK;
}

/***************************************************************/
/*                                                             */
/*  FnPopValStack                                              */
/*                                                             */
/*  Pop a value from the value stack - implemented as a        */
/*  function for situations where we don't want an immediate   */
/*  return upon failure.                                       */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int FnPopValStack(Value *val)
#else
int FnPopValStack(val)
Value *val;
#endif
{
    if (ValStackPtr <= 0)
	return E_VA_STK_UNDER;
    else {
	*val = ValStack[--ValStackPtr];
	return OK;
    }
}

