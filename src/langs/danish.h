/***************************************************************/
/*                                                             */
/*  DANISH.H                                                   */
/*                                                             */
/*  Support for the Danish language.                           */
/*                                                             */
/*  This file is part of REMIND.                               */
/*                                                             */
/*  REMIND is Copyright (C) 1992-1998 by Dianne Skoll          */
/*  Copyright 1999-2000 by Roaring Penguin Software Inc.       */
/*  This file is Copyright (C) 1993 by Mogens Lynnerup.        */
/*                                                             */
/***************************************************************/

/* The very first define in a language support file must be L_LANGNAME: */
#define L_LANGNAME "Danish"

/* Day names */
#if ISOLATIN1
#  define L_SUNDAY "S\370ndag"
#else
#  define L_SUNDAY "Soendag"
#endif
#define L_MONDAY "Mandag"
#define L_TUESDAY "Tirsdag"
#define L_WEDNESDAY "Onsdag"
#define L_THURSDAY "Torsdag"
#define L_FRIDAY "Fredag"
#if ISOLATIN1
#  define L_SATURDAY "L\370rdag"
#else
#  define L_SATURDAY "Loerdag"
#endif

/* Month names */
#define L_JAN "Januar"
#define L_FEB "Februar"
#define L_MAR "Marts"
#define L_APR "April"
#define L_MAY "Maj"
#define L_JUN "Juni"
#define L_JUL "Juli"
#define L_AUG "August"
#define L_SEP "September"
#define L_OCT "Oktober"
#define L_NOV "November"
#define L_DEC "December"

/* Today and tomorrow */
#define L_TODAY "i dag"
#define L_TOMORROW "i morgen"

/* The default banner */
#if ISOLATIN1
#  define L_BANNER "P\345mindelse for %w, %d. %m, %y%o:"
#else
#  define L_BANNER "Paamindelse for %w, %d. %m, %y%o:"
#endif

/* "am" and "pm" */
#define L_AM "am"
#define L_PM "pm"

/*** The following are only used in dosubst.c ***/
#ifdef L_IN_DOSUBST

/* Ago and from now */
#define L_AGO "siden"
#define L_FROMNOW "fra nu"

/* "in %d days' time" */
#define L_INXDAYS "om %d dage"

/* "on" as in "on date..." */
#if ISOLATIN1
#  define L_ON "p\345"
#else
#  define L_ON "paa"
#endif 

/* Pluralizing - this is a problem for many languages and may require
   a more drastic fix */
#define L_PLURAL "e"

/* Minutes, hours, at, etc */
#define L_NOW "nu"
#define L_AT "kl."
#define L_MINUTE "minut"
#define L_HOUR "time"
#define L_IS "er"
#define L_WAS "var"
#define L_AND "og"
/* What to add to make "hour" plural */
#define L_HPLU "r"  
/* What to add to make "minute" plural */
#define L_MPLU "ter"

/* Define any overrides here, such as L_ORDINAL_OVERRIDE, L_A_OVER, etc.
   See the file dosubst.c for more info. */

#define L_AMPM_OVERRIDE(ampm, hour)	ampm = (hour < 12) ? (hour<5) ? " om natten" : " om formiddagen" : (hour > 17) ? " om aftenen" : " om eftermiddagen";
#define L_ORDINAL_OVERRIDE		plu = ".";
#define L_A_OVER                        if (altmode == '*') { sprintf(s, "%s, den %d. %s %d", DayName[jul%7], d, MonthName[m], y); } else { sprintf(s, "%s %s, den %d. %s %d", L_ON, DayName[jul%7], d, MonthName[m], y); }
#define L_E_OVER                        sprintf(s, "den %02d%c%02d%c%04d", d, DateSep, m+1, DateSep, y);
#define L_F_OVER                        sprintf(s, "den %02d%c%02d%c%04d", m+1, DateSep, d, DateSep, y);
#define	L_G_OVER			if (altmode == '*') { sprintf(s, "%s, den %d. %s", DayName[jul%7], d, MonthName[m]); } else { sprintf(s, "%s %s, den %d. %s", L_ON, DayName[jul%7], d, MonthName[m]); }
#define L_H_OVER                        sprintf(s, "den %02d%c%02d", d, DateSep, m+1);
#define L_I_OVER                        sprintf(s, "den %02d%c%02d", m+1, DateSep, d);
#define L_U_OVER			L_A_OVER
#define L_V_OVER			L_G_OVER

#endif /* L_IN_DOSUBST */
