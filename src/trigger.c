/***************************************************************/
/*                                                             */
/*  TRIGGER.C                                                  */
/*                                                             */
/*  Routines for figuring out the trigger date of a reminder   */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by David F. Skoll                  */
/*  Copyright (C) 1999-2000 by Roaring Penguin Software Inc.   */
/*                                                             */
/***************************************************************/

#include "config.h"
#include <stdio.h>

#include <stdlib.h>
#include "types.h"
#include "expr.h"
#include "protos.h"
#include "globals.h"
#include "err.h"

#define GOT_DAY 1
#define GOT_MON 2
#define GOT_YR 4
#define GOT_WD 8

static int JYear(int jul);
static int JMonth(int jul);
static int NextSimpleTrig(int startdate, Trigger *trig, int *err);
static int GetNextTriggerDate(Trigger *trig, int start, int *err, int *nextstart);

/***************************************************************/
/*                                                             */
/*  NextSimpleTrig                                             */
/*                                                             */
/*  Compute the "simple" trigger date, taking into account     */
/*  ONLY the day of week, day, month and year components.      */
/*  Normally, returns -1 if the trigger has expired.  As a     */
/*  special case, if D, M, Y [WD] are specified, returns the   */
/*  Julian date, regardless of whether it's expired.  This is  */
/*  so that dates with a REP can be handled properly.          */
/*                                                             */
/***************************************************************/
static int NextSimpleTrig(int startdate, Trigger *trig, int *err)
{
    int typ = 0;
    int d, m, y, j, d2, m2, y2;

    *err = 0;
    FromJulian(startdate, &y, &m, &d);
    d2 = d;
    m2 = m;
    y2 = y;

    if (trig->d != NO_DAY) typ |= GOT_DAY;
    if (trig->m != NO_MON) typ |= GOT_MON;
    if (trig->y != NO_YR) typ |= GOT_YR;
    if (trig->wd != NO_WD) typ |= GOT_WD;
    switch(typ) {
    case 0:
    case GOT_WD:
	if (trig->wd != NO_WD)
	    while(! (trig->wd & (1 << (startdate%7)))) startdate++;
	return startdate;

    case GOT_DAY:
	if (d > trig->d) {
	    m++;
	    if (m == 12) { m = 0; y++; }
	}
	while (trig->d > DaysInMonth(m, trig->y)) m++;
	j = Julian(y, m, trig->d);
	return j;

    case GOT_MON:
	if (m == trig->m) return startdate;
	else if (m > trig->m) return Julian(y+1, trig->m, 1);
	else return Julian(y, trig->m, 1);

    case GOT_YR:
	if (y == trig->y) return startdate;
	else if (y < trig->y) return Julian(trig->y, 0, 1);
	else return -1;

    case GOT_DAY+GOT_MON:
	if (m > trig->m || (m == trig->m && d > trig->d)) y++;
	if (trig->d > MonthDays[trig->m]) {
	    *err = E_BAD_DATE;
	    return -1;
	}

	/* Take care of Feb. 29 */
	while (trig->d > DaysInMonth(trig->m, y)) y++;
	return Julian(y, trig->m, trig->d);

    case GOT_DAY+GOT_YR:
	if (y < trig->y) return Julian(trig->y, 0, trig->d);
	else if (y > trig->y) return -1;

	if (d > trig->d) {
	    m++;
	    if (m == 12) return -1;
	}
	while (trig->d > DaysInMonth(m, trig->y)) m++;
	return Julian(trig->y, m, trig->d);

    case GOT_MON+GOT_YR:
	if (y > trig->y || (y == trig->y && m > trig->m)) return -1;
	if (y < trig->y) return Julian(trig->y, trig->m, 1);
	if (m == trig->m) return startdate;
	return Julian(trig->y, trig->m, 1);

    case GOT_DAY+GOT_MON+GOT_YR:
	if (trig->d > DaysInMonth(trig->m, trig->y)) {
	    *err = E_BAD_DATE;
	    return -1;
	}
	return Julian(trig->y, trig->m, trig->d);

    case GOT_YR+GOT_WD:
	if (y > trig->y) return -1;
	if (y < trig->y) j = Julian(trig->y, 0, 1);
	else j = startdate;
	while(! (trig->wd & (1 << (j%7)))) j++;
	if (JYear(j) > trig->y) return -1;
	return j;

    case GOT_MON+GOT_WD:
	if (m == trig->m) {
	    j = startdate;
	    while(! (trig->wd & (1 << (j%7)))) j++;
	    if (JMonth(j) == trig->m) return j;
	}
	if (m >= trig->m) j = Julian(y+1, trig->m, 1);
	else j = Julian(y, trig->m, 1);
	while(! (trig->wd & (1 << (j%7)))) j++;
	return j; /* Guaranteed to be within the month */

    case GOT_DAY+GOT_WD:
	if (m !=0 || y > BASE) {
	    m2 = m-1;
	    if (m2 < 0) { y2 = y-1; m2 = 11; }

	    /* If there are fewer days in previous month, no match */
	    if (trig->d <= DaysInMonth(m2, y2)) {
		j = Julian(y2, m2, trig->d);
		while(! (trig->wd & (1 << (j%7)))) j++;
		if (j >= startdate) return j;

	    }
	}

	/* Try this month */
	if (trig->d <= DaysInMonth(m, y)) {
	    j = Julian(y, m, trig->d);
	    while(! (trig->wd & (1 << (j%7)))) j++;
	    if (j >= startdate) return j;
	}

	/* Argh!  Try next avail. month */
	m2 = m+1;
	if (m2 > 11) { m2 = 0; y++; }
	while (trig->d > DaysInMonth(m2, y)) m2++;
	j = Julian(y, m2, trig->d);
	while(! (trig->wd & (1 << (j%7)))) j++;
	return j;

    case GOT_WD+GOT_YR+GOT_DAY:
	if (y > trig->y+1 || (y > trig->y && m>0)) return -1;
	if (y > trig->y) {
	    j = Julian(trig->y, 11, trig->d);
	    while(! (trig->wd & (1 << (j%7)))) j++;
	    if (j >= startdate) return j;
	} else if (y < trig->y) {
	    j = Julian(trig->y, 0, trig->d);
	    while(! (trig->wd & (1 << (j%7)))) j++;
	    return j;
	} else {
	    /* Try last month */
	    if (m > 0) {
		m2 = m-1;
		while (trig->d > DaysInMonth(m2, trig->y)) m2--;
		j = Julian(trig->y, m2, trig->d);
		while(! (trig->wd & (1 << (j%7)))) j++;
		if (j >= startdate) return j;
	    }
	}
	/* Try this month */
	if (trig->d <= DaysInMonth(m, trig->y)) {
	    j = Julian(trig->y, m, trig->d);
	    while(! (trig->wd & (1 << (j%7)))) j++;
	    if (j >= startdate) return j;
	}

	/* Must be next month */
	if (m == 11) return -1;
	m++;
	while (trig->d > DaysInMonth(m, trig->d)) m++;
	j = Julian(trig->y, m, trig->d);
	while(! (trig->wd & (1 << (j%7)))) j++;
	return j;

    case GOT_DAY+GOT_MON+GOT_WD:
	if (trig->d > MonthDays[trig->m]) {
	    *err = E_BAD_DATE;
	    return -1;
	}
	/* Back up a year in case we'll cross a year boundary*/
	if (y > BASE) {
	    y--;
	}

	/* Move up to the first valid year */
	while (trig->d > DaysInMonth(trig->m, y)) y++;

	/* Try last year */
	j = Julian(y, trig->m, trig->d);
	while(! (trig->wd & (1 << (j%7)))) j++;
	if (j >= startdate) return j;

	/* Try this year */
	y++;
	j = Julian(y, trig->m, trig->d);
	while(! (trig->wd & (1 << (j%7)))) j++;
	if (j >= startdate) return j;

	/* Must be next year */
	y++;
	while (trig->d > DaysInMonth(trig->m, y)) y++;
	j = Julian(y, trig->m, trig->d);
	while(! (trig->wd & (1 << (j%7)))) j++;
	return j;

    case GOT_WD+GOT_MON+GOT_YR:
	if (y > trig->y || (y == trig->y && m > trig->m)) return -1;
	if (trig->y > y || (trig->y == y && trig->m > m)) {
	    j = Julian(trig->y, trig->m, 1);
	    while(! (trig->wd & (1 << (j%7)))) j++;
	    return j;
	} else {
	    j = startdate;
	    while(! (trig->wd & (1 << (j%7)))) j++;
	    FromJulian(j, &y2, &m2, &d2);
	    if (m2 == trig->m) return j; else return -1;
	}

    case GOT_WD+GOT_DAY+GOT_MON+GOT_YR:
	if (trig->d > DaysInMonth(trig->m, trig->y)) {
	    *err = E_BAD_DATE;
	    return -1;
	}
	j = Julian(trig->y, trig->m, trig->d);
	while(! (trig->wd & (1 << (j%7)))) j++;
	return j;

    default:
	Eprint("NextSimpleTrig %s %d", ErrMsg[E_SWERR], typ);
	*err = E_SWERR;
	return -1;
    }
}

/***************************************************************/
/*                                                             */
/*  JMonth - Given a Julian date, what's the month?            */
/*                                                             */
/***************************************************************/
static int JMonth(int jul)
{
    int y, m, d;
    FromJulian(jul, &y, &m, &d);
    return m;
}

/***************************************************************/
/*                                                             */
/*  JYear - Given a Julian date, what's the year?              */
/*                                                             */
/***************************************************************/
static int JYear(int jul)
{
    int y, m, d;
    FromJulian(jul, &y, &m, &d);
    return y;
}

/***************************************************************/
/*                                                             */
/*  GetNextTriggerDate                                         */
/*                                                             */
/*  Given a trigger, compute the next trigger date.            */
/*                                                             */
/*  Returns the Julian date of next trigger, -1 if             */
/*  expired, -2 if can't compute trigger date.                 */
/*                                                             */
/***************************************************************/
static int GetNextTriggerDate(Trigger *trig, int start, int *err, int *nextstart)
{
    int simple, mod, omit;

/* First:  Have we passed the UNTIL date? */
    if (trig->until != NO_UNTIL &&
	trig->until < start) {
	trig->expired = 1;
	return -1; /* expired */
    }

/* Next: If it's an "AFTER"-type skip, back up
   until we're at the start of a block of holidays */
    if (trig->skip == AFTER_SKIP) {
	int iter = 0;
	while (iter++ <= MaxSatIter) {
	    *err = IsOmitted(start-1, trig->localomit, trig->omitfunc, &omit);
	    if (*err) return -2;
	    if (!omit) {
		break;
	    }
	    start--;
	}
	if (iter > MaxSatIter) {
	    /* omitfunc must have returned "true" too often */
	    *err = E_CANT_TRIG;
	    return -2;
	}
    }

/* Find the next simple trigger */
    simple = NextSimpleTrig(start, trig, err);

/* Problems? */
    if (*err || (simple == -1)) return -1;

/* Suggested starting point for next attempt */
    *nextstart = simple+1;

/* If there's a BACK, back up... */
    if (trig->back != NO_BACK) {
	mod = trig->back;
	if (mod < 0) {
	    simple += mod;
	}
	else {
	    int iter = 0;
	    int max = MaxSatIter;
	    if (max < mod*2) {
		max = mod*2;
	    }
	    while(iter++ <= max) {
		if (!mod) {
		    break;
		}
		simple--;
		*err = IsOmitted(simple, trig->localomit, trig->omitfunc, &omit);
		if (*err) return -2;
		if (!omit) mod--;
	    }
	    if (iter > max) {
	        *err = E_CANT_TRIG;
		return -2;
	    }
	}
    }

/* If there's a REP, calculate the next occurrence */
    if (trig->rep != NO_REP) {
	if (simple < start) {
	    mod = (start - simple) / trig->rep;
	    simple = simple + mod * trig->rep;
	    if (simple < start) simple += trig->rep;
	}
    }

/* If it's a "BEFORE"-type skip, back up */
    if (trig->skip == BEFORE_SKIP) {
	int iter = 0;
	while(iter++ <= MaxSatIter) {
	    *err = IsOmitted(simple, trig->localomit, trig->omitfunc, &omit);
	    if (*err) return -2;
	    if (!omit) {
		break;
	    }
	    simple--;
	}
	if (iter > MaxSatIter) {
	    *err = E_CANT_TRIG;
	    return -2;
	}
    }

/* If it's an "AFTER"-type skip, jump ahead */
    if (trig->skip == AFTER_SKIP) {
	int iter = 0;
	while (iter++ <= MaxSatIter) {
	    *err = IsOmitted(simple, trig->localomit, trig->omitfunc, &omit);
	    if (*err) return -2;
	    if (!omit) {
		break;
	    }
	    simple++;
	}
	if (iter > MaxSatIter) {
	    *err = E_CANT_TRIG;
	    return -2;
	}
    }

/* Return the date */
    return simple;
}

/***************************************************************/
/*                                                             */
/*  ComputeTrigger                                             */
/*                                                             */
/*  The main function.  Compute the next trigger date given    */
/*  today's date.                                              */
/*                                                             */
/***************************************************************/
int ComputeTrigger(int today, Trigger *trig, int *err, int save_in_globals)
{
    int nattempts = 0,
	start = today,
	nextstart = 0,
	y, m, d, omit,
	result;

    trig->expired = 0;
    if (save_in_globals) {
	LastTrigValid = 0;
    }

/* Assume everything works */
    *err = OK;

/* But check for obvious problems... */
    if (trig->localomit == 1 + 2 + 4 + 8 + 16 + 32 + 64) {
	*err = E_2MANY_LOCALOMIT;
	return -1;
    }

    if (trig->rep != NO_REP &&
	(trig->d == NO_DAY ||
	 trig->m == NO_MON ||
	 trig->y == NO_YR)) {
	Eprint("%s", ErrMsg[E_REP_FULSPEC]);
	*err = E_REP_FULSPEC;
	return -1;
    }


    while (nattempts++ < TRIG_ATTEMPTS) {
	result = GetNextTriggerDate(trig, start, err, &nextstart);

	/* If there's an error, die immediately */
	if (*err) return -1;
	if (result == -1) {
	    trig->expired = 1;
	    if (DebugFlag & DB_PRTTRIG) {
		fprintf(ErrFp, "%s(%d): %s\n",
			FileName, LineNo, ErrMsg[E_EXPIRED]);
	    }
	    return -1;
	}

	/* If result is >= today, great! */
	if (trig->skip == SKIP_SKIP) {
	    *err = IsOmitted(result, trig->localomit, trig->omitfunc, &omit);
	    if (*err) return -1;
	} else {
	    omit = 0;
	}
	if (result >= today &&
	    (trig->skip != SKIP_SKIP || !omit)) {
	    if (save_in_globals) {
		LastTriggerDate = result;  /* Save in global var */
		LastTrigValid = 1;
	    }
	    if (DebugFlag & DB_PRTTRIG) {
		FromJulian(result, &y, &m, &d);
		fprintf(ErrFp, "%s(%d): Trig = %s, %d %s, %d\n",
			FileName, LineNo,
			DayName[result % 7],
			d,
			MonthName[m],
			y);
	    }
	    return result;
	}

	/* If it's a simple trigger, no point in rescanning */
	if (trig->back == NO_BACK &&
	    trig->skip == NO_SKIP &&
	    trig->rep == NO_REP) {
	    trig->expired = 1;
	    if (DebugFlag & DB_PRTTRIG) {
		fprintf(ErrFp, "%s(%d): %s\n",
			FileName, LineNo, ErrMsg[E_EXPIRED]);
	    }
	    if (result != -1) {
		if (save_in_globals) {
		    LastTriggerDate = result;
		    LastTrigValid = 1;
		}
	    }
	    return -1;
	}

	if (trig->skip == SKIP_SKIP &&
	    omit &&
	    nextstart <= start &&
	    result >= start) {
	    nextstart = result + 1;
	}

	/* Keep scanning... unless there's no point in doing it.*/
	if (nextstart <= start) {
	    if (result != -1) {
		if (save_in_globals) {
		    LastTriggerDate = result;
		    LastTrigValid = 1;
		}
	    }
	    trig->expired = 1;
	    if (DebugFlag & DB_PRTTRIG) {
		fprintf(ErrFp, "%s(%d): %s\n",
			FileName, LineNo, ErrMsg[E_EXPIRED]);
	    }
	    return -1;
	}
	else start = nextstart;

    }

    /* We failed - too many attempts or trigger has expired*/
    *err = E_CANT_TRIG;
    return -1;
}
