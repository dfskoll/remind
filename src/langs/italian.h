/***************************************************************/
/*                                                             */
/*  ITALIAN.H                                                  */
/*                                                             */
/*  Support for the Italian language.                          */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  It is Copyright (C) 1996 by Valerio Aimale                 */
/*                                                             */
/*  Remind is copyright (C) 1992-1998 by David F. Skoll        */
/*  Copyright 1999-2000 by Roaring Penguin Software Inc.       */
/*                                                             */
/***************************************************************/

/* The very first define in a language support file must be L_LANGNAME: */
#define L_LANGNAME "Italian"

/* Day names */
#if ISOLATIN1
#define L_SUNDAY "Domenica"
#define L_MONDAY "Luned\354"
#define L_TUESDAY "Marted\354"
#define L_WEDNESDAY "Mercoled\354"
#define L_THURSDAY "Gioved\354"
#define L_FRIDAY "Venerd\354"
#define L_SATURDAY "Sabato"
#else /* ISOLATIN1 */
#define L_SUNDAY "Domenica"
#define L_MONDAY "Lunedi`"
#define L_TUESDAY "Martedi`"
#define L_WEDNESDAY "Mercoledi`"
#define L_THURSDAY "Giovedi`"
#define L_FRIDAY "Venerdi`"
#define L_SATURDAY "Sabato"
#endif /* ISOLATIN */

/* Month names */
#define L_JAN "Gennaio"
#define L_FEB "Febbraio"
#define L_MAR "Marzo"
#define L_APR "Aprile"
#define L_MAY "Maggio"
#define L_JUN "Giugno"
#define L_JUL "Luglio"
#define L_AUG "Agosto"
#define L_SEP "Settembre"
#define L_OCT "Ottobre"
#define L_NOV "Novembre"
#define L_DEC "Dicembre"

/* Today and tomorrow */
#define L_TODAY "oggi"

#define L_TOMORROW "domani"

/* The default banner */
#define L_BANNER "Promemoria per %w, %d %m %y%o:"

/* "am" and "pm" */
#define L_AM "am"
#define L_PM "pm"

/*** The following are only used in dosubst.c ***/
#ifdef L_IN_DOSUBST

/* Ago and from now */
#define L_AGO "fa"
#define L_FROMNOW "da oggi"

/* "in %d days' time" */
#define L_INXDAYS "fra %d giorni"

/* "on" as in "on date..." */
#define L_ON ""

/* Pluralizing - this is a problem for many languages and may require
   a more drastic fix */

/* Minutes, hours, at, etc */
#define L_NOW "ora"  
#define L_AT "alle"
#define L_MINUTE "minut"
#define L_HOUR "or"
#if ISOLATIN1 
#define L_IS "\350"
#else /* ISOLATIN1 */
#define L_IS "e`"
#endif/* ISOLATIN1 */
#define L_WAS "era"
#define L_AND "e"
/* What to add to make "hour" plural */
#define L_HPLU "s"  
/* What to add to make "minute" plural */
#define L_MPLU "s"

/* Define any overrides here, such as L_ORDINAL_OVERRIDE, L_A_OVER, etc.
   See the file dosubst.c for more info. */

#define L_P_OVER sprintf(s, (diff == 1 ? "o" : "i")); 
#define L_Q_OVER sprintf(s, (diff == 1 ? "a" : "e")); 

#define L_HPLU_OVER	hplu = (hdiff == 1 ? "a" : "e");
#define L_MPLU_OVER	mplu = (mdiff == 1 ? "o" : "i");

#define L_A_OVER	sprintf(s, "%s, %d %s %d", DayName[jul%7], d,\
		    		MonthName[m], y);	
#define L_C_OVER	sprintf(s, "%s", DayName[jul%7]);	

#define L_E_OVER	sprintf(s, "%02d%c%02d%c%04d", d, DateSep,\
				 m+1, DateSep, y);	

#define L_F_OVER	sprintf(s, "%02d%c%02d%c%04d", m+1, DateSep, d, DateSep, y);	

#define L_G_OVER	sprintf(s, "%s, %d %s", DayName[jul%7], d, MonthName[m]);	

#define L_H_OVER	sprintf(s, "%02d%c%02d", d, DateSep, m+1);	

#define L_I_OVER	sprintf(s, "%02d%c%02d", m+1, DateSep, d);	

#define L_J_OVER	sprintf(s, "%s, %d %s %d", DayName[jul%7], d, \
				MonthName[m], y);

#define L_K_OVER	sprintf(s, "%s, %d %s", DayName[jul%7], d, \
				MonthName[m]);
#define L_L_OVER	sprintf(s, "%04d%c%02d%c%02d", y, DateSep, m+1, DateSep, d);	

#define L_U_OVER	sprintf(s, "%s, %d %s %d", DayName[jul%7], d, \
				MonthName[m], y);

#define L_V_OVER	sprintf(s, "%s, %d %s", DayName[jul%7], d, \
				MonthName[m]);

 
#endif /* L_IN_DOSUBST */
