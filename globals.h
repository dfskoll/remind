/***************************************************************/
/*                                                             */
/*  GLOBALS.H                                                  */
/*                                                             */
/*  This function contains declarations of global variables.   */
/*  They are instantiated in main.c by defining                */
/*  MK_GLOBALS.  Also contains useful macro definitions.       */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1996 by David F. Skoll                  */
/*                                                             */
/***************************************************************/

/* $Id: globals.h,v 1.1 1996-03-27 03:25:57 dfs Exp $ */

#ifdef MK_GLOBALS
#undef EXTERN
#define EXTERN
#define INIT(var, val) var = val
#else
#undef EXTERN
#define EXTERN extern
#define INIT(var, val) var
#endif

#define DaysInYear(y) (((y) % 4) ? 365 : ((!((y) % 100) && ((y) % 400)) ? 365 : 366 ))
#define IsLeapYear(y) (((y) % 4) ? 0 : ((!((y) % 100) && ((y) % 400)) ? 0 : 1 ))
#define DaysInMonth(m, y) ((m) != 1 ? MonthDays[m] : 28 + IsLeapYear(y))

#define DestroyValue(x) (void) (((x).type == STR_TYPE && (x).v.str) ? (free((x).v.str),(x).type = ERR_TYPE) : 0)

EXTERN	int	JulianToday;
EXTERN	int	RealToday;
EXTERN	int	CurDay;
EXTERN	int	CurMon;
EXTERN	int	CurYear;
EXTERN  int	LineNo;
EXTERN  int     FreshLine;
EXTERN  char    LineBuffer[LINELEN];
EXTERN  char    SubstBuffer[LINELEN];
EXTERN  char    TokBuffer[TOKSIZE+1];
EXTERN  INIT(   char    *MsgCommand, NULL);
EXTERN  INIT(	int     ShowAllErrors, 0);
EXTERN  INIT(	int     DebugFlag, 0);
EXTERN  INIT(   int	DoCalendar, 0);
EXTERN  INIT(   int     DoSimpleCalendar, 0);
EXTERN	INIT(	int	MondayFirst, 0);
EXTERN	INIT(	int	Iterations, 1);
EXTERN  INIT(   int     PsCal, 0);
EXTERN  INIT(   int     CalWidth, 80);
EXTERN  INIT(   int     CalWeeks, 0);
EXTERN  INIT(   int     CalMonths, 0);
EXTERN  INIT(	int 	Hush, 0);
EXTERN  INIT(	int 	NextMode, 0);
EXTERN  INIT(	int 	InfiniteDelta, 0);
EXTERN  INIT(   int     RunDisabled, 0);
EXTERN  INIT(   int     IgnoreOnce, 0);
EXTERN  INIT(   int     SortByTime, 0);
EXTERN  INIT(   int     SortByDate, 0);
EXTERN	INIT(	int	SortByPrio, 0);
EXTERN	INIT(	int	DefaultPrio, NO_PRIORITY);
EXTERN  INIT(   long    SysTime, -1L);

EXTERN	char	*InitialFile;
EXTERN	int	FileAccessDate;

EXTERN  INIT(	int 	DontFork, 0);
EXTERN  INIT(	int 	DontQueue, 0);
EXTERN  INIT(   int     NumQueued, 0);
EXTERN  INIT(   int     DontIssueAts, 0);
EXTERN  INIT(   int     Daemon, 0);


EXTERN  INIT(   int     ScFormat, SC_AMPM);
EXTERN  INIT(   int     MaxSatIter, 150);
EXTERN  INIT(	char	*FileName, NULL);
EXTERN	INIT(	int	UseStdin, 0);
EXTERN  FILE *ErrFp;
EXTERN  INIT(   int     NumIfs,    0);
EXTERN  INIT(   unsigned int IfFlags,   0);
EXTERN  INIT(   int     LastTriggerDate, 0);
EXTERN  INIT(   int     LastTrigValid, 0);
EXTERN  INIT(   int     LastTriggerTime, 0);
EXTERN  INIT(   int     ShouldCache, 0);
EXTERN  char    *CurLine;
EXTERN  INIT(   int     NumTriggered, 0);
EXTERN  int ArgC;
EXTERN  char **ArgV;
EXTERN  INIT(   int     CalLines, CAL_LINES);
EXTERN  INIT(   int     CalPad, 1);

/* Latitude and longitude */
EXTERN  INIT(	int	  LatDeg, LAT_DEG);
EXTERN  INIT(	int	  LatMin, LAT_MIN);
EXTERN  INIT(	int	  LatSec, LAT_SEC);
EXTERN  INIT(	int	  LongDeg, LON_DEG);
EXTERN  INIT(	int	  LongMin, LON_MIN);
EXTERN  INIT(	int	  LongSec, LON_SEC);
EXTERN	INIT(	char	  *Location, LOCATION);

/* UTC calculation stuff */
EXTERN  INIT(	int	  MinsFromUTC, 0);
EXTERN	INIT(	int	  CalculateUTC, 1);
EXTERN  INIT(   int	  FoldYear, 0);

/* Parameters for formatting MSGF reminders */
EXTERN  INIT(   int	  FormWidth, 72);
EXTERN	INIT(	int	  FirstIndent, 0);
EXTERN	INIT(	int	  SubsIndent, 0);
EXTERN	INIT(	char	  *EndSent, ".?!");
EXTERN	INIT(	char	  *EndSentIg, "\"')]}>");

/* We need the language stuff here... */

#include "lang.h"

EXTERN  INIT(   char    Banner[LINELEN], L_BANNER);

/* List of months */
EXTERN  char    *EnglishMonthName[]
#ifdef MK_GLOBALS
= {"January", "February", "March", "April", "May", "June",
   "July", "August", "September", "October", "November", "December"}
#endif
;

#if LANG == ENGLISH
#define MonthName EnglishMonthName
#else
EXTERN	char	*MonthName[]
#ifdef MK_GLOBALS
= {L_JAN, L_FEB, L_MAR, L_APR, L_MAY, L_JUN,
   L_JUL, L_AUG, L_SEP, L_OCT, L_NOV, L_DEC}
#endif
;
#endif

EXTERN  char	*EnglishDayName[]
#ifdef MK_GLOBALS
= {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
   "Saturday", "Sunday"}
#endif
;

#if LANG == ENGLISH
#define DayName EnglishDayName
#else
EXTERN	char	*DayName[]
#ifdef MK_GLOBALS
= {L_MONDAY, L_TUESDAY, L_WEDNESDAY, L_THURSDAY, L_FRIDAY,
   L_SATURDAY, L_SUNDAY}
#endif
;
#endif

EXTERN	int	MonthDays[]
#ifdef MK_GLOBALS
= {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
#endif
;

/* The first day of each month expressed as number of days after Jan 1.
   Second row is for leap years. */

EXTERN	int	MonthIndex[2][12]
#ifdef MK_GLOBALS
= {
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 }
}
#endif
;

#if defined(__OS2__)
#if defined(_MSC_VER) || defined(__EMX__)
#define OS2MODE (_osmode == OS2_MODE)
#define DOSMODE (_osmode == DOS_MODE)
#else
#define OS2MODE 1
#define DOSMODE 0
#endif
#endif
