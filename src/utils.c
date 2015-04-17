/***************************************************************/
/*                                                             */
/*  UTILS.C                                                    */
/*                                                             */
/*  Useful utility functions.                                  */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by Dianne Skoll                    */
/*  Copyright (C) 1999-2000 by Roaring Penguin Software Inc.   */
/*                                                             */
/***************************************************************/

#include "config.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <stdlib.h>
#include "types.h"
#include "globals.h"
#include "protos.h"

#define UPPER(c) toupper(c)

/***************************************************************/
/*                                                             */
/*  StrnCpy                                                    */
/*                                                             */
/*  Just like strncpy EXCEPT we ALWAYS copy the trailing 0.    */
/*                                                             */
/***************************************************************/
char *StrnCpy(char *dest, char const *source, int n)
{
    char *odest = dest;

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
int StrMatch(char const *s1, char const *s2, int n)
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
int StrinCmp(char const *s1, char const *s2, int n)
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
char *StrDup(char const *s)
{
    char *ret = malloc(strlen(s)+1);
    if (!ret) return NULL;
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
int StrCmpi(char const *s1, char const *s2)
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

/***************************************************************/
/*                                                             */
/*  DateOK                                                     */
/*                                                             */
/*  Return 1 if the date is OK, 0 otherwise.                   */
/*                                                             */
/***************************************************************/
int DateOK(int y, int m, int d)
{
    if (d < 1                 ||
	m < 0                 ||
	y < BASE              ||
	m > 11                ||
	y > BASE + YR_RANGE   ||
	d > DaysInMonth(m, y) ) return 0;
    else return 1;
}
