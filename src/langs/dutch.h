/***************************************************************/
/*                                                             */
/*  DUTCH.H                                                    */
/*                                                             */
/*  Support for the DUTCH language.                            */
/*                                                             */
/*  Author: Willem Kasdorp                                     */
/*                                                             */
/*  Modified slightly by Dianne Skoll                          */
/*                                                             */
/*  Further corrections by Erik-Jan Vens                       */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by Dianne Skoll                    */
/*  Copyright 1999-2000 by Roaring Penguin Software Inc.       */
/*                                                             */
/***************************************************************/

/* The very first define in a language support file must be L_LANGNAME: */
#define L_LANGNAME "Dutch"

/* Day names */
#define L_SUNDAY "zondag"
#define L_MONDAY "maandag"
#define L_TUESDAY "dinsdag"
#define L_WEDNESDAY "woensdag"
#define L_THURSDAY "donderdag"
#define L_FRIDAY "vrijdag"
#define L_SATURDAY "zaterdag"

/* Month names */
#define L_JAN "januari"
#define L_FEB "februari"
#define L_MAR "maart"
#define L_APR "april"
#define L_MAY "mei"
#define L_JUN "juni"
#define L_JUL "juli"
#define L_AUG "augustus"
#define L_SEP "september"
#define L_OCT "oktober"
#define L_NOV "november"
#define L_DEC "december"

/* Today and tomorrow */
#define L_TODAY "vandaag"
#define L_TOMORROW "morgen"

/* The default banner */
#define L_BANNER "Herinneringen voor %w, %d%s %m, %y%o:"

/* "am" and "pm" */
#define L_AM "am"
#define L_PM "pm"

/*** The following are only used in dosubst.c ***/
#ifdef L_IN_DOSUBST

/* Ago and from now */
#define L_AGO "geleden"
#define L_FROMNOW "vanaf nu"

/* "in %d days' time" */
#define L_INXDAYS "over %d dagen"

/* "on" as in "on date..." */
#define L_ON "op"

/* Pluralizing - this is a problem for many languages and may require
   a more drastic fix. (Indeed..., wkasdo) */
#define L_PLURAL "s"

/* Minutes, hours, at, etc */
#define L_NOW "nu"
#define L_AT "op"
#define L_MINUTE "minuut"
#define L_HOUR "uur"
#define L_IS "is"
#define L_WAS "was"
#define L_AND "en"
/* What to add to make "hour" plural (should result in uren, not uuren (wkasdo) */
#define L_HPLU "en"  
/* What to add to make "minute" plural (should be minuten, not minuuten) */
#define L_MPLU "en"

/* Define any overrides here, such as L_ORDINAL_OVERRIDE, L_A_OVER, etc.
   See the file dosubst.c for more info. */

/* Willem - I fixed the uren/uuren problem here */
#define L_1_OVER \
if (tdiff == 0) \
sprintf(s, L_NOW); \
else if (hdiff == 0) \
sprintf(s, "%d %s %s", mdiff, \
	(mdiff == 1 ? "minuut" : "minuten"), when); \
else if (mdiff == 0) \
sprintf(s, "%d %s %s", hdiff, \
	(mdiff == 1 ? "uur" : "uren"), when); \
	else sprintf(s, "%d %s %s %d %s %s", hdiff, \
		     (hdiff == 1 ? "uur" : "uren"), \
		     L_AND, mdiff, \
		     (mdiff == 1 ? "minuut" : "minuten"), \
		     when);

#endif /* L_IN_DOSUBST */

