/***************************************************************/
/*                                                             */
/*  UTILS.C                                                    */
/*                                                             */
/*  Useful utility functions.                                  */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by David F. Skoll                  */
/*  Copyright (C) 1999 by Roaring Penguin Software Inc.        */
/*                                                             */
/***************************************************************/

#include "config.h"
static char const RCSID[] = "$Id: utils.c,v 1.4 1999-04-05 17:34:59 dfs Exp $";

#include <string.h>
#include <stdio.h>
#include <ctype.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#include "types.h"
#include "globals.h"
#include "protos.h"

#define UPPER(c) (islower(c) ? toupper(c) : c)

/***************************************************************/
/*                                                             */
/*  StrnCpy                                                    */
/*                                                             */
/*  Just like strncpy EXCEPT we ALWAYS copy the trailing 0.    */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC char *StrnCpy(char *dest, const char *source, int n)
#else
char *StrnCpy(dest, source, n)
char *dest, *source;
int n;
#endif
{
    register char *odest = dest;

    while (n-- && (*dest++ = *source++)) ;
    if (*(dest-1)) *dest = 0;
    return odest;
}

/***************************************************************/
/*                                                             */
/*  StrMatch                                                   */
/*                                                             */
/*  Checks that two strings match (case-insensitive) to at     */
/*  least the specified number of characters, or the length    */
/*  of the first string, whichever is greater.                 */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int StrMatch(const char *s1, const char *s2, int n)
#else
int StrMatch(s1, s2, n)
char *s1, *s2;
int n;
#endif
{
    int l;
    if ((l = strlen(s1)) < n) return 0;
    return !StrinCmp(s1, s2, l);
}

/***************************************************************/
/*                                                             */
/*  StrinCmp - compare strings, case-insensitive               */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int StrinCmp(const char *s1, const char *s2, int n)
#else
int StrinCmp(s1, s2, n)
char *s1, *s2;
int n;
#endif
{
    register int r;
    while (n && *s1 && *s2) {
	n--;
	r = UPPER(*s1) - UPPER(*s2);
	if (r) return r;
	s1++;
	s2++;
    }
    if (n) return (UPPER(*s1) - UPPER(*s2)); else return 0;
}

/***************************************************************/
/*                                                             */
/*  StrDup                                                     */
/*                                                             */
/*  Like ANSI strdup                                           */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC char *StrDup(const char *s)
#else
char *StrDup(s)
char *s;
#endif
{
    char *ret = (char *) malloc(strlen(s)+1);
    if (!ret) return (char *) NULL;
    strcpy(ret, s);
    return ret;
}

/***************************************************************/
/*                                                             */
/*  StrCmpi                                                    */
/*                                                             */
/*  Compare strings, case insensitive.                         */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int StrCmpi(const char *s1, const char *s2)
#else
int StrCmpi(s1, s2)
char *s1, *s2;
#endif
{
    int r;
    while (*s1 && *s2) {
	r = UPPER(*s1) - UPPER(*s2);
	if (r) return r;
	s1++;
	s2++;
    }
    return UPPER(*s1) - UPPER(*s2);
}

#ifndef HAVE_STRSTR
#ifdef HAVE_PROTOS
PUBLIC char *strstr(char *s1, char *s2)
#else
char *strstr(s1, s2)
char *s1, *s2;
#endif
{
    char *s = s1;
    int len2 = strlen(s2);
    int len1 = strlen(s1);

    while (s-s1 <= len1-len2) {
	if (!strncmp(s, s2, len2)) return s;
	s++;
    }
    return NULL;
}
#endif

/***************************************************************/
/*                                                             */
/*  DateOK                                                     */
/*                                                             */
/*  Return 1 if the date is OK, 0 otherwise.                   */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int DateOK(int y, int m, int d)
#else
int DateOK(y, m, d)
int y, m, d;
#endif
{
    if (d < 1                 ||
	m < 0                 ||
	y < BASE              ||
	m > 11                ||
	y > BASE + YR_RANGE   ||
	d > DaysInMonth(m, y) ) return 0;
    else return 1;
}

#ifdef BROKEN_PUTC
/***************************************************************/
/*                                                             */
/*  Safe versions of putc and putchar                          */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int SafePutChar(int c)
#else
int SafePutChar(c)
int c;
#endif
{
    return putchar(c);
}

#ifdef HAVE_PROTOS
PUBLIC int SafePutc(int c, FILE *fp)
#else
int SafePutc(c, fp)
int c;
FILE *fp;
#endif
{
    return putc(c, fp);
}
#endif
