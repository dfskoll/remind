/***************************************************************/
/*                                                             */
/*  EXPR.H                                                     */
/*                                                             */
/*  Contains a few definitions used by expression evaluator.   */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by Dianne Skoll                    */
/*  Copyright (C) 1999-2000 by Roaring Penguin Software Inc.   */
/*                                                             */
/***************************************************************/

/* Define the types of values */
#define ERR_TYPE      0
#define INT_TYPE      1
#define TIME_TYPE     2
#define DATE_TYPE     3
#define STR_TYPE      4
#define DATETIME_TYPE 5
#define SPECIAL_TYPE  6 /* Only for system variables */

/* Define stuff for parsing expressions */
#define BEG_OF_EXPR '['
#define END_OF_EXPR ']'
#define COMMA ','

#define UN_OP 0  /* Unary operator */
#define BIN_OP 1 /* Binary Operator */
#define FUNC 2   /* Function */

/* Make the pushing and popping of values and operators in-line code
   for speed.  BEWARE:  These macros invoke return if an error happens ! */

#define PushOpStack(op) \
if (OpStackPtr >= OP_STACK_SIZE) \
return E_OP_STK_OVER; \
else \
OpStack[OpStackPtr++] = (op)

#define PopOpStack(op) \
if (OpStackPtr <= 0) \
return E_OP_STK_UNDER; \
else \
(op) = OpStack[--OpStackPtr]

#define PushValStack(val) \
if (ValStackPtr >= VAL_STACK_SIZE) \
return E_VA_STK_OVER; \
else \
ValStack[ValStackPtr++] = (val)

#define PopValStack(val) \
if (ValStackPtr <= 0) \
return E_VA_STK_UNDER; \
else \
(val) = ValStack[--ValStackPtr]
