/***************************************************************/
/*                                                             */
/*  HBCAL.C                                                    */
/*                                                             */
/*  Support for the Hebrew calendar                            */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by Dianne Skoll                    */
/*  Copyright (C) 1999-2000 by Roaring Penguin Software Inc.   */
/*                                                             */
/*  Derived from code written by Amos Shapir in 1978; revised  */
/*  1985.                                                      */
/*                                                             */
/***************************************************************/

#include "config.h"

#include <stdio.h>  /* For FILE used by protos.h - sigh. */
#include "types.h"
#include "protos.h"
#include "globals.h"
#include "err.h"
#define HOUR 1080L
#define DAY  (24L*HOUR)
#define WEEK (7L*DAY)
#define M(h,p) ((long)(h*HOUR+p))
#define MONTH (DAY+M(12,793))

/* Correction to convert base reference to 1990.  NOTE:  If you change
   the value of BASE in config.h, this will NOT WORK!  You'll have to
   add the appropriate number of days to CORRECTION. */

#define CORRECTION 732774L

#define TISHREY 0
#define HESHVAN 1
#define KISLEV  2
#define TEVET   3
#define SHVAT   4
#define ADARA   5
#define ADARB   6
#define NISAN   7
#define IYAR    8
#define SIVAN   9
#define TAMUZ  10
#define AV     11
#define ELUL   12
#define ADAR   13

#define JAHR_NONE     0
#define JAHR_FORWARD  1
#define JAHR_BACKWARD 2

#define ADAR2ADARB 0
#define ADAR2ADARA 1
#define ADAR2BOTH  2

static char const *HebMonthNames[] = {
    "Tishrey", "Heshvan", "Kislev", "Tevet", "Shvat", "Adar A", "Adar B",
    "Nisan", "Iyar", "Sivan", "Tamuz", "Av", "Elul", "Adar"};

static char MaxMonLen[] = {
    30, 30, 30, 29, 30, 30, 29, 30, 29, 30, 29, 30, 29, 29};

static char HebIsLeap[] = {0,0,1,0,0,1,0,1,0,0,1,0,0,1,0,0,1,0,1};

/***************************************************************/
/*                                                             */
/*  RoshHashana                                                */
/*                                                             */
/*  Return the Julian date for Rosh Hashana of specified       */
/*  Hebrew year.  (ie, 5751, not 1990)                         */
/*                                                             */
/***************************************************************/
int RoshHashana(int i)
{
    long j;
    j = DaysToHebYear(i-3744) - CORRECTION;
    return (int) j; /* No overflow check... very trusting! */
}
 
/***************************************************************/
/*                                                             */
/*  DaysToHebYear                                              */
/*                                                             */
/*  Return the number of days to RH of specified Hebrew year   */
/*  from new moon before Tishrey 1 5701.                       */
/*                                                             */
/***************************************************************/
long DaysToHebYear(int y)
{
    long m, nm, dw, s, l;

    l = y*7+1;	  /* no. of leap months */
    m = y*12+l/19;  /* total no. of months */
    nm = m*MONTH+M(1,779); /* molad at 197 cycles */
    s = m*28+nm/DAY-2;

    nm %= WEEK;
    l %= 19L;
    dw = nm/DAY;
    nm %= DAY;

    /* special cases of Molad Zaken */
    if (nm >= 18*HOUR ||
	(l < 12 && dw==3 && nm>=M(9,204)) ||
	(l <  7 && dw==2 && nm>=M(15,589)))
	s++,dw++;
    /* ADU */
    if(dw == 1 || dw == 4 || dw == 6)
	s++;
    return s;
}

/***************************************************************/
/*                                                             */
/*  DaysInHebYear                                              */
/*                                                             */
/*  Return the number of days in the Hebrew year.              */
/*                                                             */
/*                                                             */
/***************************************************************/
int DaysInHebYear(int y)
{
    long thisyear, nextyear;

    thisyear = DaysToHebYear(y-3744);
    nextyear = DaysToHebYear(y-3743);
    return (int) (nextyear - thisyear);
}

/***************************************************************/
/*                                                             */
/*  DaysInHebMonths                                            */
/*                                                             */
/*  Return a pointer to an array giving lengths of months      */
/*  given the LENGTH of the Hebrew year.                       */
/*                                                             */
/***************************************************************/
char const *DaysInHebMonths(int ylen)
{
    static char monlen[13] =
    {30, 29, 30, 29, 30, 0, 29, 30, 29, 30, 29, 30, 29};


    if (ylen > 355) {
	monlen[ADARA] = 30;
	ylen -= 30;
    } else monlen[ADARA] = 0;

    if (ylen == 353) monlen[KISLEV] = 29; else monlen[KISLEV] = 30;
    if (ylen == 355) monlen[HESHVAN] = 30; else monlen[HESHVAN] = 29;

    return monlen;
}

/***************************************************************/
/*                                                             */
/*  HebToJul                                                   */
/*                                                             */
/*  Convert a Hebrew date to Julian.                           */
/*  Hebrew months range from 0-12, but Adar A has 0 length in  */
/*  non-leap-years.                                            */
/*                                                             */
/***************************************************************/
int HebToJul(int hy, int hm, int hd)
{
    int ylen;
    char const *monlens;
    int rh;
    int m;

    /* Do some range checking */
    if (hy - 3761 < BASE || hy - 3760 > BASE+YR_RANGE) return -1;

    ylen = DaysInHebYear(hy);
    monlens = DaysInHebMonths(ylen);

    /* Get the Rosh Hashana of the year */
    rh = RoshHashana(hy);

    /* Bump up to the appropriate month */
    for (m=0; m<hm; m++) rh += monlens[m];

    /* Add in appropriate number of days */
    rh += hd - 1;
    return rh;
}

/***************************************************************/
/*                                                             */
/*  JulToHeb                                                   */
/*                                                             */
/*  Convert a Julian date to Hebrew.                           */
/*  Hebrew months range from 0-12, but Adar A has 0 length in  */
/*  non-leap-years.                                            */
/*                                                             */
/***************************************************************/
void JulToHeb(int jul, int *hy, int *hm, int *hd)
{
    int y, m, d;
    int rh;
    int ylen;
    char const *monlen;
    /* Get the common year */
    FromJulian(jul, &y, &m, &d);
    y += 3763; /* Over-estimate a bit to be on the safe side below... */

    /* Find the RH just before desired date */
    while ((rh=RoshHashana(y))>jul) y--;

    /* Got the year - now find the month */
    jul -= rh;
    ylen = DaysInHebYear(y);
    monlen = DaysInHebMonths(ylen);
    m = 0;
    while((jul >= monlen[m]) || !monlen[m]) {
	jul -= monlen[m];
	m++;
    }

    *hy = y;
    *hm = m;
    *hd = jul+1;
}

/***************************************************************/
/*                                                             */
/*  HebNameToNum                                               */
/*                                                             */
/*  Convert a Hebrew month's name to its number, given the     */
/*  year.                                                      */
/*                                                             */
/***************************************************************/
int HebNameToNum(char const *mname)
{
    int i;
    int m=-1;

    for (i=0; i<14; i++)
	if (!StrCmpi(mname, HebMonthNames[i])) {
	    m = i;
	    break;
	}

    return m;
}   

/***************************************************************/
/*                                                             */
/*  HebMonthname                                               */
/*                                                             */
/*  Convert a Hebrew month's number to its name, given the     */
/*  year.                                                      */
/*                                                             */
/***************************************************************/
char const *HebMonthName(int m, int y)
{
    if (m != ADARA && m != ADARB) return HebMonthNames[m];

    if (!HebIsLeap[(y-1)%19]) return HebMonthNames[ADAR];
    else return HebMonthNames[m];
}

/***************************************************************/
/*                                                             */
/* GetValidHebDate                                             */
/*                                                             */
/* Given the day of a month, a Hebrew month number, and a      */
/* year, return a valid year number, month number, and day     */
/* number.  Returns 0 for success, non-0 for failure.          */
/* If *dout is set to -1, then date is completely invalid.     */
/* Otherwise, date is only invalid in specified year.          */
/*                                                             */
/* Algorithm:                                                  */
/* - Convert references to Adar to Adar B.                     */
/* If jahr == 0 then                                           */ 
/*     - If no such date in current Hebrew year, return        */
/*       failure.                                              */
/* else follow jahrzeit rules:                                 */
/*     - If jahr == 1: Convert 30 Kislev to 1 Tevet and        */
/*                     30 Heshvan to 1 Kislev if chaser.       */
/*                     Convert 30 Adar A to 1 Nisan in nonleap */
/*                     This rule is NOT appropriate for a      */
/*                     jahrzeit on 30 Adar A.  Use rule 2 for  */
/*                     that.  However, I believe it is correct */
/*                     for smachot.                            */
/*     - If jahr == 2: Convert 30 Kislev to 29 Kislev and      */
/*                     30 Heshvan to 29 Heshvan if chaser.     */
/*                     Change 30 Adar A to 30 Shvat in nonleap */
/*                                                             */
/***************************************************************/
int GetValidHebDate(int yin, int min, int din, int adarbehave,
                           int *mout, int *dout, int jahr)
{
    char const *monlen;
    int ylen;

    *mout = min;
    *dout = din;

    /* Do some error checking */
    if (din < 1 || din > MaxMonLen[min] || min < 0 || min > 13) {
	*dout = -1;
	return E_BAD_HEBDATE;
    }

    ylen = DaysInHebYear(yin);
    monlen = DaysInHebMonths(ylen);

    /* Convert ADAR as necessary */
    if (min == ADAR) {
	switch(adarbehave) {
	case ADAR2ADARA: if (monlen[ADARA]) *mout = min = ADARA;
	else		     *mout = min = ADARB;
	break;

	case ADAR2ADARB: *mout = min = ADARB; break;

	default:
	    Eprint("GetValidHebDate: Bad adarbehave value %d", adarbehave);
	    return E_SWERR;
	}
    }

    if (din <= monlen[min]) return OK;

    switch(jahr) {
    case JAHR_NONE: return E_BAD_DATE;

    case JAHR_FORWARD:
	if (min == KISLEV) {
            *mout = TEVET;
            *dout = 1;
            return OK;
	} else if (min == HESHVAN) {
            *mout = KISLEV;
            *dout = 1;
            return OK;
	} else if (min == ADARA) {
            if (din > 29) {
		*dout = 1;
		*mout = NISAN;
            } else {
		*dout = din;
		*mout = ADARB;
            }
	    return OK;
	}

	Eprint("GetValidHebDate: (1) software error! %d", jahr);
	return E_SWERR;

    case JAHR_BACKWARD:
	if (min == KISLEV) {
            *mout = KISLEV;
            *dout = 29;
            return OK;
	} else if (min == HESHVAN) {
            *mout = HESHVAN;
            *dout = 29;
            return OK;
	} else if (min == ADARA) {
	    if (din > 29) {
		*dout = 30;
		*mout = SHVAT;
            } else {
		*mout = ADARB;
		*dout = din;
            }
	    return OK;
	}

	Eprint("GetValidHebDate: (2) software error! %d", jahr);
	return E_SWERR;

    default:
	Eprint("GetValidHebDate: (3) software error! %d", jahr);
	return E_SWERR;
    }
}


/***************************************************************/
/*                                                             */
/*  GetNextHebrewDate                                          */
/*                                                             */
/*  Get the next Hebrew date on or after specified date.       */
/*                                                             */
/*  Returns 0 for success, non-zero for failure.               */
/*                                                             */
/***************************************************************/
int GetNextHebrewDate(int julstart, int hm, int hd,
			     int jahr, int adarbehave, int *ans)
{
    int r, yout, mout, dout, jul=1;
    int adarflag = adarbehave;

    /* I initialize jul above to stop gcc from complaining about
       possible use of uninitialized variable.  You can take it
       out if the small inefficiency really bothers you. */

    /* If adarbehave == ADAR2BOTH, set adarflag to ADAR2ADARA for now */
    if (adarbehave == ADAR2BOTH) adarflag = ADAR2ADARA;

    JulToHeb(julstart, &yout, &mout, &dout);

    r = 1;
    while(r) {
	r = GetValidHebDate(yout, hm, hd, adarflag, &mout, &dout, jahr);
	if (dout == -1) return r;
	if (r) {
	    if (adarbehave == ADAR2BOTH && hm == ADAR) {
		if (adarflag == ADAR2ADARA) {
		    adarflag = ADAR2ADARB;
		} else {
		    adarflag = ADAR2ADARA;
		    yout++;
		}
	    } else yout++;
	    continue;
	}
	jul = HebToJul(yout, mout, dout);
	if (jul < 0) return E_DATE_OVER;
	if (jul >= julstart) break;
	else {
	    if (adarbehave == ADAR2BOTH && hm == ADAR) {
		if (adarflag == ADAR2ADARA) {
		    adarflag = ADAR2ADARB;
		} else {
		    adarflag = ADAR2ADARA;
		    yout++;
		}
	    } else yout++;
	    r=1;  /* Force loop to continue */
	}
    }
    *ans = jul;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  ComputeJahr                                                */
/*                                                             */
/*  Given a date of death, compute the value to use for jahr.  */
/*                                                             */
/***************************************************************/
int ComputeJahr(int y, int m, int d, int *ans)
{
    char const *monlen;
    int len;

    *ans = JAHR_NONE;

    len = DaysInHebYear(y);
    monlen = DaysInHebMonths(len);

/* Check for Adar A */
    if (m == ADARA && monlen[m] == 0) {
	Eprint("No Adar A in %d", y);
	return E_BAD_HEBDATE;
    }


    if (d < 1 || d > MaxMonLen[m] || m < 0 || m > 13) {
	return E_BAD_HEBDATE;
    }

    if (d > monlen[m]) {
	Eprint("%d %s %d: %s", d, HebMonthNames[m], y, ErrMsg[E_BAD_HEBDATE]);
	return E_BAD_HEBDATE;
    }

/* If the jahrzeit was in Adar A, we always use JAHR_BACKWARD */
    if (m == ADARA) {
	*ans = JAHR_BACKWARD;
	return OK;
    }

/* Get lengths of months in year following jahrzeit */
    len = DaysInHebYear(y+1);
    monlen = DaysInHebMonths(len);

    if (d > monlen[m]) *ans = JAHR_FORWARD;
    else               *ans = JAHR_BACKWARD;

    return OK;
}
