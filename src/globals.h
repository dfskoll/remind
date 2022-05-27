/***************************************************************/
/*                                                             */
/*  GLOBALS.H                                                  */
/*                                                             */
/*  This function contains declarations of global variables.   */
/*  They are instantiated in main.c by defining                */
/*  MK_GLOBALS.  Also contains useful macro definitions.       */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-2021 by Dianne Skoll                    */
/*                                                             */
/***************************************************************/


#ifdef MK_GLOBALS
#undef EXTERN
#define EXTERN
#define INIT(var, val) var = val
#else
#undef EXTERN
#define EXTERN extern
#define INIT(var, val) var
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
EXTERN  FILE *ErrFp;

#include "dynbuf.h"
#include "lang.h"

#define MAX_TRUSTED_USERS 20

#define MINUTES_PER_DAY 1440

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
EXTERN  uid_t   TrustedUsers[MAX_TRUSTED_USERS];

EXTERN  INIT(   int     NumTrustedUsers, 0);
EXTERN  INIT(   char    const *MsgCommand, NULL);
EXTERN  INIT(	int     ShowAllErrors, 0);
EXTERN  INIT(	int     DebugFlag, 0);
EXTERN  INIT(   int	DoCalendar, 0);
EXTERN  INIT(   int     DoSimpleCalendar, 0);
EXTERN  INIT(   int     DoSimpleCalDelta, 0);
EXTERN  INIT(   int     DoPrefixLineNo, 0);
EXTERN	INIT(	int	MondayFirst, 0);
EXTERN	INIT(	int	Iterations, 1);
EXTERN  INIT(   int     PsCal, 0);
EXTERN  INIT(   int     CalWidth, -1);
EXTERN  INIT(   int     CalWeeks, 0);
EXTERN  INIT(   int     CalMonths, 0);
EXTERN  INIT(	int 	Hush, 0);
EXTERN  INIT(	int 	NextMode, 0);
EXTERN  INIT(	int 	InfiniteDelta, 0);
EXTERN  INIT(   int     DefaultTDelta, 0);
EXTERN  INIT(   int     DeltaOffset, 0);
EXTERN  INIT(   int     RunDisabled, 0);
EXTERN  INIT(   int     IgnoreOnce, 0);
EXTERN  INIT(   int     SortByTime, 0);
EXTERN  INIT(   int     SortByDate, 0);
EXTERN	INIT(	int	SortByPrio, 0);
EXTERN  INIT(   int     UntimedBeforeTimed, 0);
EXTERN	INIT(	int	DefaultPrio, NO_PRIORITY);
EXTERN  INIT(   long    SysTime, -1L);

EXTERN	char	const *InitialFile;
EXTERN	int	FileAccessDate;

EXTERN  INIT(   int     DontSuppressQuoteMarkers, 0);
EXTERN  INIT(	int 	DontFork, 0);
EXTERN  INIT(	int 	DontQueue, 0);
EXTERN  INIT(   int     NumQueued, 0);
EXTERN  INIT(   int     DontIssueAts, 0);
EXTERN  INIT(   int     Daemon, 0);
EXTERN  INIT(   char    DateSep, DATESEP);
EXTERN  INIT(   char    TimeSep, TIMESEP);
EXTERN  INIT(   char    DateTimeSep, DATETIMESEP);
EXTERN  INIT(   int     DefaultColorR, -1);
EXTERN  INIT(   int     DefaultColorB, -1);
EXTERN  INIT(   int     DefaultColorG, -1);
EXTERN  INIT(   int     SynthesizeTags, 0);
EXTERN  INIT(   int     ScFormat, SC_AMPM);
EXTERN  INIT(   int     MaxSatIter, 1000);
EXTERN  INIT(   int     MaxStringLen, MAX_STR_LEN);
EXTERN  INIT(	char	*FileName, NULL);
EXTERN	INIT(	int	UseStdin, 0);
EXTERN  INIT(   int     PurgeMode, 0);
EXTERN  INIT(   int     PurgeIncludeDepth, 0);
EXTERN  INIT(   FILE    *PurgeFP,  NULL);
EXTERN  INIT(   int     NumIfs,    0);
EXTERN  INIT(   unsigned int IfFlags,   0);
EXTERN  INIT(   int     LastTrigValid, 0);
EXTERN  Trigger  LastTrigger;
EXTERN  TimeTrig LastTimeTrig;
EXTERN  INIT(   int     LastTriggerDate, 0);
EXTERN  INIT(   int     LastTriggerTime, 0);
EXTERN  INIT(   int     ShouldCache, 0);
EXTERN  char const   *CurLine;
EXTERN  INIT(   int     NumTriggered, 0);
EXTERN  int ArgC;
EXTERN  char const **ArgV;
EXTERN  INIT(   int     CalLines, CAL_LINES);
EXTERN  INIT(   int     CalPad, 1);
EXTERN  INIT(   int     UseVTChars, 0);
EXTERN  INIT(   int     UseUTF8Chars, 0);
EXTERN  INIT(   int     UseVTColors, 0);
EXTERN  INIT(   int     Use256Colors, 0);
EXTERN  INIT(   int     UseTrueColors, 0);
EXTERN  INIT(   int     TerminalBackground, TERMINAL_BACKGROUND_UNKNOWN);

/* Latitude and longitude */
EXTERN  INIT(	int	  LatDeg, 0);
EXTERN  INIT(	int	  LatMin, 0);
EXTERN  INIT(	int	  LatSec, 0);
EXTERN  INIT(	int	  LongDeg, 0);
EXTERN  INIT(	int	  LongMin, 0);
EXTERN  INIT(	int	  LongSec, 0);
EXTERN  INIT(   double    Longitude, DEFAULT_LONGITUDE);
EXTERN  INIT(   double    Latitude, DEFAULT_LATITUDE);

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

EXTERN DynamicBuffer Banner;
EXTERN DynamicBuffer LineBuffer;
EXTERN DynamicBuffer ExprBuf;
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

EXTERN char *DynamicMonthName[]
#ifdef MK_GLOBALS
#if LANG == ENGLISH
= {"January", "February", "March", "April", "May", "June",
   "July", "August", "September", "October", "November", "December"}
#else
= {L_JAN, L_FEB, L_MAR, L_APR, L_MAY, L_JUN,
   L_JUL, L_AUG, L_SEP, L_OCT, L_NOV, L_DEC}
#endif
#endif
;
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

EXTERN char *DynamicDayName []
#ifdef MK_GLOBALS
#if LANG == ENGLISH
= {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
   "Saturday", "Sunday"}
#else
= {L_MONDAY, L_TUESDAY, L_WEDNESDAY, L_THURSDAY, L_FRIDAY,
   L_SATURDAY, L_SUNDAY}
#endif
#endif
;

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

EXTERN char *DynamicAgo
#ifdef MK_GLOBALS
= L_AGO
#endif
;
EXTERN char *DynamicAm
#ifdef MK_GLOBALS
= L_AM
#endif
;
EXTERN char *DynamicAnd
#ifdef MK_GLOBALS
= L_AND
#endif
;
EXTERN char *DynamicAt
#ifdef MK_GLOBALS
= L_AT
#endif
;
EXTERN char *DynamicFromnow
#ifdef MK_GLOBALS
= L_FROMNOW
#endif
;
EXTERN char *DynamicHour
#ifdef MK_GLOBALS
= L_HOUR
#endif
;
EXTERN char *DynamicHplu
#ifdef MK_GLOBALS
= L_HPLU
#endif
;
EXTERN char *DynamicIs
#ifdef MK_GLOBALS
= L_IS
#endif
;
EXTERN char *DynamicMinute
#ifdef MK_GLOBALS
= L_MINUTE
#endif
;
EXTERN char *DynamicMplu
#ifdef MK_GLOBALS
= L_MPLU
#endif
;
EXTERN char *DynamicNow
#ifdef MK_GLOBALS
= L_NOW
#endif
;
EXTERN char *DynamicOn
#ifdef MK_GLOBALS
= L_ON
#endif
;
EXTERN char *DynamicPm
#ifdef MK_GLOBALS
= L_PM
#endif
;
EXTERN char *DynamicToday
#ifdef MK_GLOBALS
= L_TODAY
#endif
;
EXTERN char *DynamicTomorrow
#ifdef MK_GLOBALS
= L_TOMORROW
#endif
;
EXTERN char *DynamicWas
#ifdef MK_GLOBALS
= L_WAS
#endif
;

#define XSTR(x) #x
#define STRSYSDIR(x) XSTR(x)

EXTERN char *SysDir
#ifdef MK_GLOBALS
= STRSYSDIR(SYSDIR)
#endif
;

EXTERN int SuppressLRM
#ifdef MK_GLOBALS
= 0
#endif
;
