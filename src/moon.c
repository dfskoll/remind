/***************************************************************/
/*                                                             */
/*  MOON.C                                                     */
/*                                                             */
/*  Calculations for figuring out moon phases.                 */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by David F. Skoll                  */
/*  Copyright (C) 1999-2000 by Roaring Penguin Software Inc.   */
/*                                                             */
/***************************************************************/

#include "config.h"

/* All of these routines were adapted from the program "moontool"
   by John Walker, February 1988.  Here's the blurb from moontool:

   ... The information is generally accurate to within ten
   minutes.

   The algorithms used in this program to calculate the positions Sun and
   Moon as seen from the Earth are given in the book "Practical Astronomy
   With Your Calculator" by Peter Duffett-Smith, Second Edition,
   Cambridge University Press, 1981. Ignore the word "Calculator" in the
   title; this is an essential reference if you're interested in
   developing software which calculates planetary positions, orbits,
   eclipses, and the like. If you're interested in pursuing such
   programming, you should also obtain:

   "Astronomical Formulae for Calculators" by Jean Meeus, Third Edition,
   Willmann-Bell, 1985. A must-have.

   "Planetary Programs and Tables from -4000 to +2800" by Pierre
   Bretagnon and Jean-Louis Simon, Willmann-Bell, 1986. If you want the
   utmost (outside of JPL) accuracy for the planets, it's here.

   "Celestial BASIC" by Eric Burgess, Revised Edition, Sybex, 1985. Very
   cookbook oriented, and many of the algorithms are hard to dig out of
   the turgid BASIC code, but you'll probably want it anyway.

   Many of these references can be obtained from Willmann-Bell, P.O. Box
   35025, Richmond, VA 23235, USA. Phone: (804) 320-7016. In addition
   to their own publications, they stock most of the standard references
   for mathematical and positional astronomy.

   This program was written by:

      John Walker
      Autodesk, Inc.
      2320 Marinship Way
      Sausalito, CA 94965
      (415) 332-2344 Ext. 829

      Usenet: {sun!well}!acad!kelvin

   This program is in the public domain: "Do what thou wilt shall be the
   whole of the law". I'd appreciate receiving any bug fixes and/or
   enhancements, which I'll incorporate in future versions of the
   program. Please leave the original attribution information intact so
   that credit and blame may be properly apportioned.

*/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "types.h"
#include "protos.h"
#include "expr.h"
#include "globals.h"
#include "err.h"

/* Function prototypes */
static long jdate (int y, int mon, int day);
static double jtime (int y, int mon, int day, int hour, int min, int sec);
static void jyear (double td, int *yy, int *mm, int *dd);
static void jhms (double j, int *h, int *m, int *s);
static double meanphase (double sdate, double phase, double *usek);
static double truephase (double k, double phase);
static double kepler (double m, double ecc);
static double phase (double, double *, double *, double *, double *, double *, double *);


/*  Astronomical constants  */

#define epoch	    2444238.5	   /* 1980 January 0.0 */

/*  Constants defining the Sun's apparent orbit  */

#define elonge	    278.833540	   /* Ecliptic longitude of the Sun
				      at epoch 1980.0 */
#define elongp	    282.596403	   /* Ecliptic longitude of the Sun at
				      perigee */
#define eccent      0.016718       /* Eccentricity of Earth's orbit */
#define sunsmax     1.495985e8     /* Semi-major axis of Earth's orbit, km */
#define sunangsiz   0.533128       /* Sun's angular size, degrees, at
				      semi-major axis distance */

/*  Elements of the Moon's orbit, epoch 1980.0  */

#define mmlong      64.975464      /* Moon's mean lonigitude at the epoch */
#define mmlongp     349.383063	   /* Mean longitude of the perigee at the
				      epoch */
#define mlnode	    151.950429	   /* Mean longitude of the node at the
				      epoch */
#define minc        5.145396       /* Inclination of the Moon's orbit */
#define mecc        0.054900       /* Eccentricity of the Moon's orbit */
#define mangsiz     0.5181         /* Moon's angular size at distance a
				      from Earth */
#define msmax       384401.0       /* Semi-major axis of Moon's orbit in km */
#define mparallax   0.9507	   /* Parallax at distance a from Earth */
#define synmonth    29.53058868    /* Synodic month (new Moon to new Moon) */
#define lunatbase   2423436.0      /* Base date for E. W. Brown's numbered
				      series of lunations (1923 January 16) */

/*  Properties of the Earth  */

#define earthrad    6378.16	   /* Radius of Earth in kilometres */
#ifdef PI
#undef PI
#endif

#define PI 3.14159265358979323846

/*  Handy mathematical functions  */

#ifdef sgn
#undef sgn
#endif
#define sgn(x) (((x) < 0) ? -1 : ((x) > 0 ? 1 : 0))	  /* Extract sign */

#ifdef abs
#undef abs
#endif
#define abs(x) ((x) < 0 ? (-(x)) : (x)) 		  /* Absolute val */

#define fixangle(a) ((a) - 360.0 * (floor((a) / 360.0)))  /* Fix angle	  */
#define torad(d) ((d) * (PI / 180.0))			  /* Deg->Rad	  */
#define todeg(d) ((d) * (180.0 / PI))			  /* Rad->Deg	  */
#define dsin(x) (sin(torad((x))))			  /* Sin from deg */
#define dcos(x) (cos(torad((x))))			  /* Cos from deg */

/***************************************************************/
/*                                                             */
/*  jdate                                                      */
/*                                                             */
/*  Convert a date and time to Julian day and fraction.        */
/*                                                             */
/***************************************************************/
static long jdate(int y, int mon, int day)
{
    long c, m;

    m = mon+1;
    if (m>2) {
	m -= 3;
    } else {
	m += 9;
	y--;
    }
    c = y/100L;   /* Century */
    y -= 100L * c;
    return day + (c*146097L)/4 + (y*1461L)/4 + (m*153L+2)/5 + 1721119L;
}

/***************************************************************/
/*                                                             */
/*  jtime                                                      */
/*                                                             */
/*  Convert a GMT date and time to astronomical Julian time,   */
/*  i.e. Julian date plus day fraction, expressed as a double  */
/*                                                             */
/***************************************************************/
static double jtime(int y, int mon, int day, int hour, int min, int sec)
{
    return (jdate(y, mon, day)-0.5) +
	(sec + 60L * (long) min + 3600L * (long) hour) / 86400.0;
}

/***************************************************************/
/*                                                             */
/*  jyear                                                      */
/*                                                             */
/*  Convert a Julian date to year, month, day.                 */
/*                                                             */
/***************************************************************/
static void jyear(double td, int *yy, int *mm, int *dd)
{
    double j, d, y, m;

    td += 0.5;         /* Astronomical to civil */
    j = floor(td);
    j = j - 1721119.0;
    y = floor(((4 * j) - 1) / 146097.0);
    j = (j * 4.0) - (1.0 + (146097.0 * y));
    d = floor(j / 4.0);
    j = floor(((4.0 * d) + 3.0) / 1461.0);
    d = ((4.0 * d) + 3.0) - (1461.0 * j);
    d = floor((d + 4.0) / 4.0);
    m = floor(((5.0 * d) - 3) / 153.0);
    d = (5.0 * d) - (3.0 + (153.0 * m));
    d = floor((d + 5.0) / 5.0);
    y = (100.0 * y) + j;
    if (m < 10.0)
	m = m + 2;
    else {
	m = m - 10;
	y = y + 1;
    }
    *yy = y;
    *mm = m;
    *dd = d;
}

/***************************************************************/
/*                                                             */
/*  jhms                                                       */
/*                                                             */
/*  Convert a Julian time to hour, minutes and seconds.        */
/*                                                             */
/***************************************************************/
static void jhms(double j, int *h, int *m, int *s)
{
    long ij;

    j += 0.5;         /* Astronomical to civil */
    ij = (j - floor(j)) * 86400.0;
    *h = ij / 3600L;
    *m = (ij / 60L) % 60L;
    *s = ij % 60L;
}

/***************************************************************/
/*                                                             */
/*  meanphase                                                  */
/*                                                             */
/*  Calculates mean phase of the Moon for a                    */
/*  given base date and desired phase:			       */
/*     0.0   New Moon					       */
/*     0.25  First quarter				       */
/*     0.5   Full moon					       */
/*     0.75  Last quarter				       */
/*  Beware!!!  This routine returns meaningless		       */
/*  results for any other phase arguments.  Don't	       */
/*  attempt to generalise it without understanding	       */
/*  that the motion of the moon is far more complicated	       */
/*  than this calculation reveals.			       */
/*                                                             */
/***************************************************************/
static double meanphase(double sdate, double phase, double *usek)
{
    double k, t, t2, t3, nt1;

/*** The following was the original code:  It gave roundoff errors
  causing moonphase info to fail for Dec 1994.  ***/
/*    jyear(sdate, &yy, &mm, &dd);
      k = (yy + (mm/12.0) - 1900) * 12.368531; */

/*** The next line is the replacement ***/
    k = (sdate - 2415020.0) / synmonth;

    /* Time in Julian centuries from 1900 January 0.5 */
    t = (sdate - 2415020.0) / 36525.0;
    t2 = t * t;		   /* Square for frequent use */
    t3 = t2 * t;		   /* Cube for frequent use */

    *usek = k = floor(k) + phase;
    nt1 = 2415020.75933 + synmonth * k
	+ 0.0001178 * t2
	- 0.000000155 * t3
	+ 0.00033 * dsin(166.56 + 132.87 * t - 0.009173 * t2);

    return nt1;
}

/***************************************************************/
/*                                                             */
/*  truephase                                                  */
/*                                                             */
/*  Given a K value used to determine the                      */
/*  mean phase of the new moon, and a phase                    */
/*  selector (0.0, 0.25, 0.5, 0.75), obtain                    */
/*  the true, corrected phase time.                            */
/*                                                             */
/***************************************************************/
static double truephase(double k, double phase)
{
    double t, t2, t3, pt, m, mprime, f;
    int apcor = 0;

    k += phase;		   /* Add phase to new moon time */
    t = k / 1236.8531;	   /* Time in Julian centuries from
   			      1900 January 0.5 */
    t2 = t * t;		   /* Square for frequent use */
    t3 = t2 * t;		   /* Cube for frequent use */
    pt = 2415020.75933	   /* Mean time of phase */
        + synmonth * k
        + 0.0001178 * t2
        - 0.000000155 * t3
        + 0.00033 * dsin(166.56 + 132.87 * t - 0.009173 * t2);

    m = 359.2242               /* Sun's mean anomaly */
	+ 29.10535608 * k
	- 0.0000333 * t2
	- 0.00000347 * t3;
    mprime = 306.0253          /* Moon's mean anomaly */
	+ 385.81691806 * k
	+ 0.0107306 * t2
	+ 0.00001236 * t3;
    f = 21.2964                /* Moon's argument of latitude */
	+ 390.67050646 * k
	- 0.0016528 * t2
	- 0.00000239 * t3;
    if ((phase < 0.01) || (abs(phase - 0.5) < 0.01)) {

	/* Corrections for New and Full Moon */

	pt +=     (0.1734 - 0.000393 * t) * dsin(m)
   	    + 0.0021 * dsin(2 * m)
   	    - 0.4068 * dsin(mprime)
   	    + 0.0161 * dsin(2 * mprime)
   	    - 0.0004 * dsin(3 * mprime)
   	    + 0.0104 * dsin(2 * f)
   	    - 0.0051 * dsin(m + mprime)
   	    - 0.0074 * dsin(m - mprime)
   	    + 0.0004 * dsin(2 * f + m)
   	    - 0.0004 * dsin(2 * f - m)
   	    - 0.0006 * dsin(2 * f + mprime)
   	    + 0.0010 * dsin(2 * f - mprime)
   	    + 0.0005 * dsin(m + 2 * mprime);
	apcor = 1;
    } else if ((abs(phase - 0.25) < 0.01 || (abs(phase - 0.75) < 0.01))) {
	pt +=     (0.1721 - 0.0004 * t) * dsin(m)
   	    + 0.0021 * dsin(2 * m)
   	    - 0.6280 * dsin(mprime)
   	    + 0.0089 * dsin(2 * mprime)
   	    - 0.0004 * dsin(3 * mprime)
   	    + 0.0079 * dsin(2 * f)
   	    - 0.0119 * dsin(m + mprime)
   	    - 0.0047 * dsin(m - mprime)
   	    + 0.0003 * dsin(2 * f + m)
   	    - 0.0004 * dsin(2 * f - m)
   	    - 0.0006 * dsin(2 * f + mprime)
   	    + 0.0021 * dsin(2 * f - mprime)
   	    + 0.0003 * dsin(m + 2 * mprime)
   	    + 0.0004 * dsin(m - 2 * mprime)
   	    - 0.0003 * dsin(2 * m + mprime);
	if (phase < 0.5)
	    /* First quarter correction */
	    pt += 0.0028 - 0.0004 * dcos(m) + 0.0003 * dcos(mprime);
	else
	    /* Last quarter correction */
	    pt += -0.0028 + 0.0004 * dcos(m) - 0.0003 * dcos(mprime);
	apcor = 1;
    }
    if (!apcor) return 0.0;
    return pt;
}

/***************************************************************/
/*                                                             */
/*  kepler                                                     */
/*                                                             */
/*  Solve the equation of Kepler.                              */
/*                                                             */
/***************************************************************/
static double kepler(double m, double ecc)
{
    double e, delta;
#define EPSILON 1E-6

    e = m = torad(m);
    do {
	delta = e - ecc * sin(e) - m;
	e -= delta / (1 - ecc * cos(e));
    } while (abs(delta) > EPSILON);
    return e;
}
/***************************************************************/
/*                                                             */
/*  PHASE  --  Calculate phase of moon as a fraction:          */
/*                                                             */
/*   The argument is the time for which the phase is   	       */
/*   Requested, expressed as a Julian date and		       */
/*   fraction.  Returns the terminator phase angle as a	       */
/*   percentage of a full circle (i.e., 0 to 1), and	       */
/*   stores into pointer arguments the illuminated	       */
/*   fraction of the Moon's disc, the Moon's age in	       */
/*   days and fraction, the distance of the Moon from	       */
/*   the centre of the Earth, and the angular diameter	       */
/*   subtended by the Moon as seen by an observer at	       */
/*   the centre of the Earth.				       */
/*                                                             */
/***************************************************************/
static double phase(double pdate,
		    double *pphase,
		    double *mage,
		    double *dist,
		    double *angdia,
		    double *sudist,
		    double *suangdia)
{

    double Day, N, M, Ec, Lambdasun, ml, MM, MN, Ev, Ae, A3, MmP,
	mEc, A4, lP, V, lPP, NP, y, x, Lambdamoon,
	MoonAge, MoonPhase,
	MoonDist, MoonDFrac, MoonAng,
	F, SunDist, SunAng;

    /* Calculation of the Sun's position */

    Day = pdate - epoch;	    /* Date within epoch */
    N = fixangle((360 / 365.2422) * Day); /* Mean anomaly of the Sun */
    M = fixangle(N + elonge - elongp);    /* Convert from perigee
					     co-ordinates to epoch 1980.0 */
    Ec = kepler(M, eccent);     /* Solve equation of Kepler */
    Ec = sqrt((1 + eccent) / (1 - eccent)) * tan(Ec / 2);
    Ec = 2 * todeg(atan(Ec));   /* 1 anomaly */
    Lambdasun = fixangle(Ec + elongp);  /* Sun's geocentric ecliptic
					   longitude */
    /* Orbital distance factor */
    F = ((1 + eccent * cos(torad(Ec))) / (1 - eccent * eccent));
    SunDist = sunsmax / F;	    /* Distance to Sun in km */
    SunAng = F * sunangsiz;     /* Sun's angular size in degrees */


    /* Calculation of the Moon's position */

    /* Moon's mean longitude */
    ml = fixangle(13.1763966 * Day + mmlong);

    /* Moon's mean anomaly */
    MM = fixangle(ml - 0.1114041 * Day - mmlongp);

    /* Moon's ascending node mean longitude */
    MN = fixangle(mlnode - 0.0529539 * Day);

    /* Evection */
    Ev = 1.2739 * sin(torad(2 * (ml - Lambdasun) - MM));

    /* Annual equation */
    Ae = 0.1858 * sin(torad(M));

    /* Correction term */
    A3 = 0.37 * sin(torad(M));

    /* Corrected anomaly */
    MmP = MM + Ev - Ae - A3;

    /* Correction for the equation of the centre */
    mEc = 6.2886 * sin(torad(MmP));

    /* Another correction term */
    A4 = 0.214 * sin(torad(2 * MmP));

    /* Corrected longitude */
    lP = ml + Ev + mEc - Ae + A4;

    /* Variation */
    V = 0.6583 * sin(torad(2 * (lP - Lambdasun)));

    /* 1 longitude */
    lPP = lP + V;

    /* Corrected longitude of the node */
    NP = MN - 0.16 * sin(torad(M));

    /* Y inclination coordinate */
    y = sin(torad(lPP - NP)) * cos(torad(minc));

    /* X inclination coordinate */
    x = cos(torad(lPP - NP));

    /* Ecliptic longitude */
    Lambdamoon = todeg(atan2(y, x));
    Lambdamoon += NP;

    /* Calculation of the phase of the Moon */

    /* Age of the Moon in degrees */
    MoonAge = lPP - Lambdasun;

    /* Phase of the Moon */
    MoonPhase = (1 - cos(torad(MoonAge))) / 2;

    /* Calculate distance of moon from the centre of the Earth */

    MoonDist = (msmax * (1 - mecc * mecc)) /
	(1 + mecc * cos(torad(MmP + mEc)));

    /* Calculate Moon's angular diameter */

    MoonDFrac = MoonDist / msmax;
    MoonAng = mangsiz / MoonDFrac;

    if(pphase)   *pphase = MoonPhase;
    if(mage)     *mage = synmonth * (fixangle(MoonAge) / 360.0);
    if(dist)     *dist = MoonDist;
    if(angdia)   *angdia = MoonAng;
    if(sudist)   *sudist = SunDist;
    if(suangdia) *suangdia = SunAng;
    return fixangle(MoonAge) / 360.0;
}

/***************************************************************/
/*                                                             */
/*  MoonPhase                                                  */
/*                                                             */
/*  Interface routine dealing in Remind representations.       */
/*  Given a local date and time, returns the moon phase at     */
/*  that date and time as a number from 0 to 360.              */
/*                                                             */
/***************************************************************/
int MoonPhase(int date, int time)
{
    int utcd, utct;
    int y, m, d;
    double jd, mp;

    /* Convert from local to UTC */
    LocalToUTC(date, time, &utcd, &utct);

    /* Convert from Remind representation to year/mon/day */
    FromJulian(utcd, &y, &m, &d);

    /* Convert to a true Julian date -- sorry for the name clashes! */
    jd = jtime(y, m, d, (utct / 60), (utct % 60), 0);   

    /* Calculate moon phase */
    mp = 360.0 * phase(jd, NULL, NULL, NULL, NULL, NULL, NULL);
    return (int) mp;
}

/***************************************************************/
/*                                                             */
/*  HuntPhase                                                  */
/*                                                             */
/*  Given a starting date and time and a target phase, find    */
/*  the first date on or after the starting date and time when */
/*  the moon hits the specified phase.  Phase must be from     */
/*  0 to 3 for new, 1stq, full, 3rdq                           */
/*                                                             */
/***************************************************************/
void HuntPhase(int startdate, int starttim, int phas, int *date, int *time)
{
    int utcd, utct;
    int y, m, d;
    int h, min, s;
    int d1, t1;
    double k1, k2, jd, jdorig;
    double nt1, nt2;

    /* Convert from local to UTC */
    LocalToUTC(startdate, starttim, &utcd, &utct);

    /* Convert from Remind representation to year/mon/day */
    FromJulian(utcd, &y, &m, &d);
    /* Convert to a true Julian date -- sorry for the name clashes! */
    jdorig = jtime(y, m, d, (utct / 60), (utct % 60), 0);   
    jd = jdorig - 45.0;
    nt1 = meanphase(jd, 0.0, &k1);
    while(1) {
	jd += synmonth;
	nt2 = meanphase(jd, 0.0, &k2);
	if (nt1 <= jdorig && nt2 > jdorig) break;
	nt1 = nt2;
	k1 = k2;
    }
    jd = truephase(k1, phas/4.0);
    if (jd < jdorig) jd = truephase(k2, phas/4.0);

    /* Convert back to Remind format */
    jyear(jd, &y, &m, &d);
    jhms(jd, &h, &min, &s);

    d1 = Julian(y, m, d);
    t1 = h*60 + min;
    UTCToLocal(d1, t1, date, time);
}
