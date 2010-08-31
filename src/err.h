/***************************************************************/
/*                                                             */
/*  ERR.H                                                      */
/*                                                             */
/*  Error definitions.                                         */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by David F. Skoll                  */
/*  Copyright (C) 1999-2000 by Roaring Penguin Software Inc.   */
/*                                                             */
/***************************************************************/

/* Note that not all of the "errors" are really errors - some are just
   messages for information purposes.  Constants beginning with M_ should
   never be returned as error indicators - they should only be used to
   index the ErrMsg array. */

#define OK                    0
#define E_MISS_END            1
#define E_MISS_QUOTE          2
#define E_OP_STK_OVER         3
#define E_VA_STK_OVER         4
#define E_MISS_RIGHT_PAREN    5
#define E_UNDEF_FUNC          6
#define E_ILLEGAL_CHAR        7
#define E_EXPECTING_BINOP     8
#define E_NO_MEM              9
#define E_BAD_NUMBER         10
#define E_OP_STK_UNDER       11
#define E_VA_STK_UNDER       12
#define E_CANT_COERCE        13
#define E_BAD_TYPE           14
#define E_DATE_OVER          15
#define E_STACK_ERR          16
#define E_DIV_ZERO           17
#define E_NOSUCH_VAR         18
#define E_EOLN		     19
#define E_EOF		     20
#define E_IO_ERR             21
#define E_LINE_2_LONG        22
#define E_SWERR		     23
#define E_BAD_DATE           24
#define E_2FEW_ARGS	     25
#define E_2MANY_ARGS	     26
#define E_BAD_TIME	     27
#define E_2HIGH		     28
#define E_2LOW 		     29
#define E_CANT_OPEN          30
#define E_NESTED_INCLUDE     31
#define E_PARSE_ERR          32
#define E_CANT_TRIG          33
#define E_NESTED_IF          34
#define E_ELSE_NO_IF         35
#define E_ENDIF_NO_IF        36
#define E_2MANY_LOCALOMIT    37
#define E_EXTRANEOUS_TOKEN   38
#define E_POP_NO_PUSH        39
#define E_RUN_DISABLED       40
#define E_DOMAIN_ERR         41
#define E_BAD_ID             42
#define E_RECURSIVE          43
#define E_PARSE_AS_REM       44 /* Not really an error - just returned by
                                   DoOmit to indicate line should be executed
                                   as a REM statement, also. */
#define E_CANT_MODIFY        45
#define E_MKTIME_PROBLEM     46
#define E_REDEF_FUNC	     47
#define E_CANTNEST_FDEF      48
#define E_REP_FULSPEC        49
#define E_YR_TWICE	     50
#define E_MON_TWICE	     51
#define E_DAY_TWICE	     52
#define E_UNKNOWN_TOKEN	     53
#define E_SPEC_MON_DAY	     54
#define E_2MANY_PART	     55
#define E_2MANY_FULL	     56
#define E_PUSH_NOPOP	     57
#define E_ERR_READING	     58
#define E_EXPECTING_EOL	     59
#define E_BAD_HEBDATE	     60
#define E_IIF_ODD	     61
#define E_MISS_ENDIF	     62
#define E_EXPECT_COMMA	     63
#define E_WD_TWICE	     64
#define E_SKIP_ERR	     65
#define E_CANT_NEST_RTYPE    66
#define E_REP_TWICE	     67
#define E_DELTA_TWICE	     68
#define E_BACK_TWICE	     69
#define E_ONCE_TWICE	     70
#define E_EXPECT_TIME	     71
#define E_UNTIL_TWICE	     72
#define E_INCOMPLETE	     73
#define E_SCAN_TWICE	     74
#define E_VAR		     75
#define E_VAL		     76
#define E_UNDEF		     77
#define E_ENTER_FUN	     78
#define E_LEAVE_FUN	     79
#define E_EXPIRED	     80
#define E_CANTFORK	     81
#define E_CANTACCESS	     82
#define M_BAD_SYS_DATE	     83
#define M_BAD_DB_FLAG	     84
#define M_BAD_OPTION	     85
#define M_BAD_USER	     86
#define M_NO_CHG_GID	     87
#define M_NO_CHG_UID	     88
#define M_NOMEM_ENV	     89
#define E_MISS_EQ	     90
#define E_MISS_VAR	     91
#define E_MISS_EXPR	     92
#define M_CANTSET_ACCESS     93
#define M_I_OPTION	     94
#define E_NOREMINDERS	     95
#define M_QUEUED	     96
#define E_EXPECTING_NUMBER   97
#define M_BAD_WARN_FUNC      98
#define E_CANT_CONVERT_TZ    99
#define E_NO_MATCHING_REMS  100
#define E_STRING_TOO_LONG   101

#ifdef MK_GLOBALS
#undef EXTERN
#define EXTERN
#else
#undef EXTERN
#define EXTERN extern
#endif

#ifndef L_ERR_OVERRIDE
EXTERN char *ErrMsg[]

#ifdef MK_GLOBALS
= {
    "Ok",
    "Missing ']'",
    "Missing quote",
    "Expression too complex - too many operators",
    "Expression too complex - too many operands",
    "Missing ')'",
    "Undefined function",
    "Illegal character",
    "Expecting binary operator",
    "Out of memory",
    "Ill-formed number",
    "Op stack underflow - internal error",
    "Va stack underflow - internal error",
    "Can't coerce",
    "Type mismatch",
    "Date overflow",
    "Stack error - internal error",
    "Division by zero",
    "Undefined variable",
    "Unexpected end of line",
    "Unexpected end of file",
    "I/O error",
    "Line too long",
    "Internal error",
    "Bad date specification",
    "Not enough arguments",
    "Too many arguments",
    "Ill-formed time",
    "Number too high",
    "Number too low",
    "Can't open file",
    "INCLUDE nested too deeply",
    "Parse error",
    "Can't compute trigger",
    "Too many nested IFs",
    "ELSE with no matching IF",
    "ENDIF with no matching IF",
    "Can't OMIT every weekday",
    "Extraneous token(s) on line",
    "POP-OMIT-CONTEXT without matching PUSH-OMIT-CONTEXT",
    "RUN disabled",
    "Domain error",
    "Invalid identifier",
    "Recursive function call detected",
    "",
    "Cannot modify system variable",
    "C library function can't represent date/time",
    "Attempt to redefine built-in function",
    "Can't nest function definition in expression",
    "Must fully specify date to use repeat factor",
    "Year specified twice",
    "Month specified twice",
    "Day specified twice",
    "Unknown token",
    "Must specify month and day in OMIT command",
    "Too many partial OMITs",
    "Too many full OMITs",
    "Warning: PUSH-OMIT-CONTEXT without matching POP-OMIT-CONTEXT",
    "Error reading",
    "Expecting end-of-line",
    "Invalid Hebrew date",
    "IIF needs odd number of arguments",
    "Warning: Missing ENDIF",
    "Expecting comma",
    "Weekday specified twice",
    "Only use one of BEFORE, AFTER or SKIP",
    "Can't nest MSG, MSF, RUN, etc. in expression",
    "Repeat value specified twice",
    "Delta value specified twice",
    "Back value specified twice",
    "ONCE keyword used twice. (Hah.)",
    "Expecting time after AT",
    "THROUGH/UNTIL keyword used twice",
    "Incomplete date specification",
    "FROM/SCANFROM keyword used twice",
    "Variable",
    "Value",
    "*UNDEFINED*",
    "Entering UserFN",
    "Leaving UserFN",
    "Expired",
    "fork() failed - can't do queued reminders",
    "Can't access file",
    "Illegal system date: Year is less than %d\n",
    "Unknown debug flag '%c'\n",
    "Unknown option '%c'\n",
    "Unknown user '%s'\n",
    "Could not change gid to %d\n",
    "Could not change uid to %d\n",
    "Out of memory for environment\n",
    "Missing '=' sign",
    "Missing variable name",
    "Missing expression",
    "Can't reset access date of %s\n",
    "Remind: '-i' option: %s\n",
    "No reminders.",
    "%d reminder(s) queued for later today.\n",
    "Expecting number",
    "Bad function in WARN clause",
    "Can't convert between time zones",
    "No files matching *.rem",
    "String too long"
}
#endif /* MK_GLOBALS */
;
#endif /* L_ERR_OVERRIDE */
