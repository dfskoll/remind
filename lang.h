/***************************************************************/
/*                                                             */
/*  LANG.H                                                     */
/*                                                             */
/*  Header file for language support for various languages.    */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1996 by David F. Skoll                  */
/*                                                             */
/***************************************************************/

/* $Id: lang.h,v 1.4 1996-10-12 02:49:33 dfs Exp $ */

/* I'm chauvinistic and name each language with its English name... */

#define ENGLISH    0 /* original   by David F. Skoll                   */
#define GERMAN     1 /* translated by Wolfgang Thronicke               */
#define DUTCH      2 /* translated by Willem Kasdorp and Erik-Jan Vens */
#define FINNISH    3 /* translated by Mikko Silvonen                   */
#define FRENCH	   4 /* translated by Laurent Duperval                 */
#define NORWEGIAN  5 /* translated by Trygve Randen                    */
#define DANISH     6 /* translated by Mogens Lynnerup                  */
#define POLISH     7 /* translated by Jerzy Sobczyk                    */
#define BRAZPORT   8 /* Brazilian Portuguese by Marco Paganini         */
#define ITALIAN    9 /* translated by Valerio Aimale                   */

/* Add more languages here - but please e-mail aa775@freenet.carleton.ca
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
#include "english.h"
#elif LANG == GERMAN
#include "german.h"
#elif LANG == DUTCH
#include "dutch.h"
#elif LANG == FINNISH
#include "finnish.h"
#elif LANG == FRENCH
#include "french.h"
#elif LANG == NORWEGIAN
#include "norwgian.h"
#elif LANG == DANISH
#include "danish.h"
#elif LANG == POLISH
#include "polish.h"
#elif LANG == BRAZPORT
#include "portbr.h"
#elif LANG == ITALIAN
#include "italian.h"

/* If no sensible language, choose English.  I intended to use
   the #error directive here, but some C compilers barf. */
#else
#include "english.h"
#endif
