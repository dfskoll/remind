!/***************************************************************/
/*                                                             */
/*  ROMANIAN.H                                                 */
/*                                                             */
/*  Support for the Romanian language.                         */
/*                                                             */
/*  Contributed by Liviu Daia  <daia@stoilow.imar.ro>          */
/*                                                             */
/*  This file is part of REMIND.                               */
/*                                                             */
/*  REMIND is Copyright (C) 1992-1998 by Dianne Skoll          */
/*       Copyright 1999-2000 by Roaring Penguin Software Inc.  */
/*  This file is Copyright (C) 1996-1998 by Liviu Daia         */
/*                                                             */
/***************************************************************/

/* The very first define in a language support file must be L_LANGNAME: */
#define L_LANGNAME "Romanian"

/* Day names */
#if ISOLATIN1
#  define L_SUNDAY "Duminica"
#  define L_MONDAY "Luni"
#  define L_TUESDAY "Marti"
#  define L_WEDNESDAY "Miercuri"
#  define L_THURSDAY "Joi"
#  define L_FRIDAY "Vineri"
#  define L_SATURDAY "S\342mbata"
#elif ISOLATIN2
#  define L_SUNDAY "Duminic\343"
#  define L_MONDAY "Luni"
#  define L_TUESDAY "Mar\376i"
#  define L_WEDNESDAY "Miercuri"
#  define L_THURSDAY "Joi"
#  define L_FRIDAY "Vineri"
#  define L_SATURDAY "S\342mb\343t\343"
#elif IBM852
#  define L_SUNDAY "Duminic\307"
#  define L_MONDAY "Luni"
#  define L_TUESDAY "Mar\316i"
#  define L_WEDNESDAY "Miercuri"
#  define L_THURSDAY "Joi"
#  define L_FRIDAY "Vineri"
#  define L_SATURDAY "S\203mb\307t\307"
#else
#  define L_SUNDAY "Duminica"
#  define L_MONDAY "Luni"
#  define L_TUESDAY "Marti"
#  define L_WEDNESDAY "Miercuri"
#  define L_THURSDAY "Joi"
#  define L_FRIDAY "Vineri"
#  define L_SATURDAY "Sambata"
#endif

/* Month names */
#define L_JAN "Ianuarie"
#define L_FEB "Februarie"
#define L_MAR "Martie"
#define L_APR "Aprilie"
#define L_MAY "Mai"
#define L_JUN "Iunie"
#define L_JUL "Iulie"
#define L_AUG "August"
#define L_SEP "Septembrie"
#define L_OCT "Octombrie"
#define L_NOV "Noiembrie"
#define L_DEC "Decembrie"

/* Today and tomorrow */
#if ISOLATIN1
#  define L_TODAY "astazi"
#  define L_TOMORROW "m\342ine"
#elif ISOLATIN2
#  define L_TODAY "ast\343zi"
#  define L_TOMORROW "m\342ine"
#elif IBM852
#  define L_TODAY "ast\307zi"
#  define L_TOMORROW "m\203ine"
#else
#  define L_TODAY "astazi"
#  define L_TOMORROW "maine"
#endif

/* The default banner */
#define L_BANNER "Reamintiri pentru %w, %d %m %y%o:"

/* "am" and "pm" */
#define L_AM "am"
#define L_PM "pm"

#ifdef L_IN_DOSUBST
/*** The following are only used in dosubst.c ***/

/* Ago and from now */
#define L_AGO "acum"
#define L_FROMNOW "peste"

/* "in %d days' time" */
#define L_INXDAYS "peste %d zile"

/* "on" as in "on date..." */
#define L_ON "pe"

/* Pluralizing - this is a problem for many languages and may require
   a more drastic fix */
#define L_PLURAL "le"

/* Minutes, hours, at, etc */
#define L_NOW "acum"
#define L_AT "la ora"
#define L_MINUTE "minut"
#define L_HOUR "or"
#define L_IS "este"
#define L_WAS "a fost"
/* What to add to make "minute" plural */
#define L_MPLU "e"

#if ISOLATIN2
   /* What to add to make "hour" plural */
#  define L_HPLU_OVER hplu = (hdiff == 1 ? "\343" : "e");
#  define L_AND "\272i"
#elif IBM852
   /* What to add to make "hour" plural */
#  define L_HPLU_OVER hplu = (hdiff == 1 ? "\307" : "e");
#  define L_AND "\255i"
#else
   /* What to add to make "hour" plural */
#  define L_HPLU_OVER hplu = (hdiff == 1 ? "a" : "e");
#  define L_AND "si"
#endif

/* Define any overrides here, such as L_ORDINAL_OVERRIDE, L_A_OVER, etc.
   See the file dosubst.c for more info. */
#if ISOLATIN2
#  define L_AMPM_OVERRIDE(ampm, hour)	ampm = (hour < 12) ? (hour<4) ? " noaptea" : " diminea\376a" : (hour > 17) ? " seara" : " dup\343-amiaza";
#elif IBM852
#  define L_AMPM_OVERRIDE(ampm, hour)	ampm = (hour < 12) ? (hour<4) ? " noaptea" : " diminea\316a" : (hour > 17) ? " seara" : " dup\307-amiaza";
#else
#  define L_AMPM_OVERRIDE(ampm, hour)	ampm = (hour < 12) ? (hour<4) ? " noaptea" : " dimineata" : (hour > 17) ? " seara" : " dupa-amiaza";
#endif

#define L_ORDINAL_OVERRIDE		plu = "";

#define L_A_OVER sprintf(s, "%s, %d %s %d", DayName[jul%7], d, MonthName[m], y);
#define L_C_OVER sprintf(s, "%s", DayName[jul%7]);
#define L_G_OVER sprintf(s, "%s, %d %s", DayName[jul%7], d, MonthName[m]);
#define L_J_OVER sprintf(s, "%s, %s %d, %d", DayName[jul%7], MonthName[m], d, y);
#define L_K_OVER sprintf(s, "%s, %s %d", DayName[jul%7], MonthName[m], d);
#define L_S_OVER
#define L_U_OVER sprintf(s, "%s, %d %s %d", DayName[jul%7], d, MonthName[m], y);
#define L_V_OVER sprintf(s, "%s, %d %s", DayName[jul%7], d, MonthName[m]);
#define L_1_OVER							\
            if (tdiff == 0)						\
                sprintf(s, L_NOW);					\
            else if (hdiff == 0)					\
                sprintf(s, "%s %d %s%s", when, mdiff, L_MINUTE, mplu);	\
            else if (mdiff == 0)					\
                sprintf(s, "%s %d %s%s", when, hdiff, L_HOUR, hplu);	\
            else							\
                sprintf(s, "%s %d %s%s %s %d %s%s", when, hdiff, 	\
			L_HOUR, hplu, L_AND, mdiff, L_MINUTE, mplu);

#endif /* L_IN_DOSUBST */
