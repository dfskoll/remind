/***************************************************************/
/*                                                             */
/*  LANG.H                                                     */
/*                                                             */
/*  Header file for language support for various languages.    */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by Dianne Skoll                    */
/*  Copyright (C) 1999-2000 by Roaring Penguin Software Inc.   */
/*                                                             */
/***************************************************************/

/* I'm chauvinistic and name each language with its English name... */

#define ENGLISH    0 /* original   by Dianne Skoll                   */
#define GERMAN     1 /* translated by Wolfgang Thronicke               */
#define DUTCH      2 /* translated by Willem Kasdorp and Erik-Jan Vens */
#define FINNISH    3 /* translated by Mikko Silvonen                   */
#define FRENCH	   4 /* translated by Laurent Duperval                 */
#define NORWEGIAN  5 /* translated by Trygve Randen                    */
#define DANISH     6 /* translated by Mogens Lynnerup                  */
#define POLISH     7 /* translated by Jerzy Sobczyk                    */
#define BRAZPORT   8 /* Brazilian Portuguese by Marco Paganini         */
#define ITALIAN    9 /* translated by Valerio Aimale                   */
#define ROMANIAN  10 /* translated by Liviu Daia                       */
#define SPANISH   11 /* translated by Rafa Couto                       */
#define ICELANDIC 12 /* translated by Björn Davíðsson                  */

/* Add more languages here - but please e-mail dfs@roaringpenguin.com
   to have your favorite language assigned a number.  If you add a
   language, please send me the header file, and permission to include
   it in future releases of Remind.  Note that you'll get no remuneration
   for this service - just everlasting fame. :-)

   Use the file tstlang.rem to test your new language file. */

/************************************************************************
 *                                                                      *
 *       Define the language you want to use here                       *
 *                                                                      *
 ************************************************************************/
#ifndef LANG  /* Allow for definition on compiler command line */
#define LANG ENGLISH
#endif

/* Pick up the appropriate header file */
#if LANG == ENGLISH
#include "langs/english.h"
#elif LANG == GERMAN
#include "langs/german.h"
#elif LANG == DUTCH
#include "langs/dutch.h"
#elif LANG == FINNISH
#include "langs/finnish.h"
#elif LANG == FRENCH
#include "langs/french.h"
#elif LANG == NORWEGIAN
#include "langs/norwgian.h"
#elif LANG == DANISH
#include "langs/danish.h"
#elif LANG == POLISH
#include "langs/polish.h"
#elif LANG == BRAZPORT
#include "langs/portbr.h"
#elif LANG == ITALIAN
#include "langs/italian.h"
#elif LANG == ROMANIAN
#include "langs/romanian.h"
#elif LANG == SPANISH
#include "langs/spanish.h"
#elif LANG == ICELANDIC
#include "langs/icelandic.h"

/* If no sensible language, choose English.  I intended to use
   the #error directive here, but some C compilers barf. */
#else
#include "langs/english.h"
#endif
