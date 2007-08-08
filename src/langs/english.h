/***************************************************************/
/*                                                             */
/*  ENGLISH.H                                                  */
/*                                                             */
/*  Support for the English language.                          */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by David F. Skoll                  */
/*  Copyright 1999-2000 by Roaring Penguin Software Inc.       */
/*                                                             */
/***************************************************************/

/* The very first define in a language support file must be L_LANGNAME: */
#define L_LANGNAME "English"

/* Day names */
#define L_SUNDAY "Sunday"
#define L_MONDAY "Monday"
#define L_TUESDAY "Tuesday"
#define L_WEDNESDAY "Wednesday"
#define L_THURSDAY "Thursday"
#define L_FRIDAY "Friday"
#define L_SATURDAY "Saturday"

/* Month names */
#define L_JAN "January"
#define L_FEB "February"
#define L_MAR "March"
#define L_APR "April"
#define L_MAY "May"
#define L_JUN "June"
#define L_JUL "July"
#define L_AUG "August"
#define L_SEP "September"
#define L_OCT "October"
#define L_NOV "November"
#define L_DEC "December"

/* Today and tomorrow */
#define L_TODAY "today"
#define L_TOMORROW "tomorrow"

/* The default banner */
#define L_BANNER "Reminders for %w, %d%s %m, %y%o:"

/* "am" and "pm" */
#define L_AM "am"
#define L_PM "pm"

/*** The following are only used in dosubst.c ***/
#ifdef L_IN_DOSUBST

/* Ago and from now */
#define L_AGO "ago"
#define L_FROMNOW "from now"

/* "in %d days' time" */
#define L_INXDAYS "in %d days' time"

/* "on" as in "on date..." */
#define L_ON "on"

/* Pluralizing - this is a problem for many languages and may require
   a more drastic fix */
#define L_PLURAL "s"

/* Minutes, hours, at, etc */
#define L_NOW "now"
#define L_AT "at"
#define L_MINUTE "minute"
#define L_HOUR "hour"
#define L_IS "is"
#define L_WAS "was"
#define L_AND "and"
/* What to add to make "hour" plural */
#define L_HPLU "s"  
/* What to add to make "minute" plural */
#define L_MPLU "s"

/* Define any overrides here, such as L_ORDINAL_OVERRIDE, L_A_OVER, etc.
   See the file dosubst.c for more info. */

#endif /* L_IN_DOSUBST */
