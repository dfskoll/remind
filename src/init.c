/***************************************************************/
/*                                                             */
/*  INIT.C                                                     */
/*                                                             */
/*  Initialize remind; perform certain tasks between           */
/*  iterations in calendar mode; do certain checks after end   */
/*  in normal mode.                                            */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by David F. Skoll                  */
/*                                                             */
/***************************************************************/

#include "config.h"
static char const RCSID[] = "$Id: init.c,v 1.4 1998-02-10 03:15:51 dfs Exp $";

#define L_IN_INIT 1
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "types.h"
#include "protos.h"
#include "expr.h"
#include "err.h"
#include "version.h"
#include "globals.h"

/***************************************************************
 *
 *  Command line options recognized:
 *
 *  -n       = Output next trigger date of each reminder in
 *             simple calendar format.
 *  -r       = Disallow RUN mode
 *  -c[n]    = Produce a calendar for n months (default = 1)
 *  -w[n,n,n] = Specify output device width, padding and spacing
 *  -s[n]    = Produce calendar in "simple calendar" format
 *  -p[n]    = Produce calendar in format compatible with rem2ps
 *  -v       = Verbose mode
 *  -o       = Ignore ONCE directives
 *  -a       = Don't issue timed reminders which will be queued
 *  -q       = Don't queue timed reminders
 *  -t       = Trigger all reminders (infinite delta)
 *  -h       = Hush mode
 *  -f       = Do not fork
 *  -dchars  = Debugging mode:  Chars are:
 *             e = Echo input lines
 *             x = Display expression evaluation
 *             t = Display trigger dates
 *             v = Dump variables at end
 *             l = Display entire line in error messages
 *  -e       = Send messages normally sent to stderr to stdout instead
 *  -z[n]    = Daemon mode waking up every n (def 5) minutes.
 *  -bn      = Time format for cal (0, 1, or 2)
 *  -xn      = Max. number of iterations for SATISFY
 *  -uname   = Run as user 'name' - only valid when run by root.  If run
 *             by non-root, changes environment but not effective uid.
 *  -kcmd    = Run 'cmd' for MSG-type reminders instead of printing to stdout
 *  -iVAR=EXPR = Initialize and preserve VAR.
 *  -m       = Start calendar with Monday instead of Sunday.
 *  A minus sign alone indicates to take input from stdin
 *
 **************************************************************/

/* For parsing an integer */
#define PARSENUM(var, s)   \
var = 0;                   \
while (isdigit(*(s))) {    \
    var *= 10;             \
    var += *(s) - '0';     \
    s++;                   \
}

#ifdef UNIX
PRIVATE void ChgUser ARGS((char *uname));
#endif

PRIVATE void InitializeVar ARGS ((char *str));

static char *BadDate = "Illegal date on command line\n";

/***************************************************************/
/*                                                             */
/*  InitRemind                                                 */
/*                                                             */
/*  Initialize the system - called only once at beginning!     */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC void InitRemind(int argc, char *argv[])
#else
void InitRemind(argc, argv)
int argc;
char *argv[];
#endif
{
    char *arg;
    int i;
    int y, m, d, rep;
    Token tok;

    /* Initialize global dynamic buffers */
    DBufInit(&Banner);
    DBufInit(&LineBuffer);
    DBufInit(&TPushBuffer);

    DBufPuts(&Banner, L_BANNER);

    /* Make sure remind is not installed set-uid or set-gid */
#ifdef UNIX
    if (getgid() != getegid() ||
	getuid() != geteuid()) {
	fprintf(ErrFp, "\nRemind should not be installed set-uid or set-gid.\nCHECK YOUR SYSTEM SECURITY.\n");
	exit(1);
    }
#endif

    y = NO_YR;
    m = NO_MON;
    d = NO_DAY;
    rep = NO_REP;

    RealToday = SystemDate(&CurYear, &CurMon, &CurDay);
    if (RealToday < 0) {
	fprintf(ErrFp, ErrMsg[M_BAD_SYS_DATE], BASE);
	exit(1);
    }
    JulianToday = RealToday;
    FromJulian(JulianToday, &CurYear, &CurMon, &CurDay);

#if !defined(HAVE_QUEUED)
    DontFork = 1;
    DontQueue = 1;
    NumQueued = 0;
    DontIssueAts = 0;
    Daemon = 0;
#elif defined(_MSC_VER) || defined(__BORLANDC__)
    DontFork = 1;
#elif defined(__OS2__) && defined (__MSDOS__)
    if (DOSMODE)
	DontFork = 1;
#endif

    /* Parse the command-line options */
    i = 1;
    while (i < argc) {
	arg = argv[i];
	if (*arg != '-') break; /* Exit the loop if it's not an option */
	i++;
	arg++;
	if (!*arg) {
	    UseStdin = 1;
	    IgnoreOnce = 1;
	    i--;
	    break;
	}
	while (*arg) {
	    switch(*arg++) {

            case 'i':
	    case 'I':
		InitializeVar(arg);
		while(*arg) arg++;
		break;
	   
	    case 'n':
	    case 'N':
		NextMode = 1;
#ifdef HAVE_QUEUED
		DontQueue = 1;
		Daemon = 0;
#endif
		break;

	    case 'r':
	    case 'R':
		RunDisabled = RUN_CMDLINE;
		break;

	    case 'm':
	    case 'M':
		MondayFirst = 1;
		break;

	    case 'o':
	    case 'O':
		IgnoreOnce = 1;
		break;

	    case 't':
	    case 'T':
		InfiniteDelta = 1;
		break;

	    case 'e':
	    case 'E':
		ErrFp = stdout;
		break;

	    case 'h':
	    case 'H':
		Hush = 1;
		break;

	    case 'g':
	    case 'G':
		SortByDate = SORT_ASCEND;
		SortByTime = SORT_ASCEND;
		SortByPrio = SORT_ASCEND;
		if (*arg) {
		    if (*arg == 'D' || *arg == 'd')
			SortByDate = SORT_DESCEND;
		    arg++;
		}
		if (*arg) {
		    if (*arg == 'D' || *arg == 'd')
			SortByTime = SORT_DESCEND;
		    arg++;
		}
		if (*arg) {
		    if (*arg == 'D' || *arg == 'd')
			SortByPrio = SORT_DESCEND;
		    arg++;
		}
		break;

#if defined(UNIX) && defined(WANT_U_OPTION)
	    case 'u':
	    case 'U':
		ChgUser(arg);
		RunDisabled = RUN_CMDLINE;
		while (*arg) arg++;
		break;
#endif	       

#ifdef HAVE_QUEUED
	    case 'z':
	    case 'Z':
		DontFork = 1;
		PARSENUM(Daemon, arg);
		if (Daemon<5) Daemon=5;
		else if (Daemon>60) Daemon=60;
		break;

	    case 'a':
	    case 'A':
		DontIssueAts = 1;
		break;

	    case 'q':
	    case 'Q':
		DontQueue = 1;
		break;

	    case 'f':
	    case 'F':
		DontFork = 1;
		break;
#endif
	    case 'c':
	    case 'C':
		DoCalendar = 1;
		if (*arg == '+') {
		    arg++;
		    PARSENUM(CalWeeks, arg);
		    if (!CalWeeks) CalWeeks = 1;
		} else {
		    PARSENUM(CalMonths, arg);
		    if (!CalMonths) CalMonths = 1;
		}
		break;

	    case 's':
	    case 'S':
		DoSimpleCalendar = 1;
		if (*arg == '+') {
		    arg++;
		    PARSENUM(CalWeeks, arg);
		    if (!CalWeeks) CalWeeks = 1;
		} else {
		    PARSENUM(CalMonths, arg);
		    if (!CalMonths) CalMonths = 1;
		}
		break;

	    case 'p':
	    case 'P':
		DoSimpleCalendar = 1;
		PsCal = 1;
		PARSENUM(CalMonths, arg);
		if (!CalMonths) CalMonths = 1;
		break;

	    case 'w':
	    case 'W':
		if (*arg != ',') {
		    PARSENUM(CalWidth, arg);
		    if (CalWidth < 80) CalWidth = 80;
		}
		if (*arg == ',') {
		    arg++;
		    if (*arg != ',') {
			PARSENUM(CalLines, arg);
			if (CalLines > 20) CalLines = 20;
		    }
		    if (*arg == ',') {
			arg++;
			PARSENUM(CalPad, arg);
			if (CalPad > 20) CalPad = 20;
		    }
		}
		break;

	    case 'd':
	    case 'D':
		while (*arg) {
		    switch(*arg++) {
		    case 'e': case 'E': DebugFlag |= DB_ECHO_LINE; break;
		    case 'x': case 'X': DebugFlag |= DB_PRTEXPR;   break;
		    case 't': case 'T': DebugFlag |= DB_PRTTRIG;   break;
		    case 'v': case 'V': DebugFlag |= DB_DUMP_VARS; break;
		    case 'l': case 'L': DebugFlag |= DB_PRTLINE;   break;
		    default:
		        fprintf(ErrFp, ErrMsg[M_BAD_DB_FLAG], *(arg-1));
		    }
		}
		break;

	    case 'v':
	    case 'V':
		DebugFlag |= DB_PRTLINE;
		ShowAllErrors = 1;
		break;

	    case 'b':
	    case 'B':
		PARSENUM(ScFormat, arg);
		if (ScFormat<0 || ScFormat>2) ScFormat=SC_AMPM;
		break;

	    case 'x':
	    case 'X':
		PARSENUM(MaxSatIter, arg);
		if (MaxSatIter < 10) MaxSatIter=10;
		break;

	    case 'k':
	    case 'K':
		MsgCommand = arg;
		while (*arg) arg++;  /* Chew up remaining chars in this arg */
		break;

	    default:
		fprintf(ErrFp, ErrMsg[M_BAD_OPTION], *(arg-1));
	    }

	}
    }

    /* Get the filename. */
    if (i >= argc) {
	Usage();
	exit(1);
    }
    InitialFile = argv[i++];

    /* Get the date, if any */
    if (i < argc) {
	while (i < argc) {
	    arg = argv[i++];
	    FindToken(arg, &tok);
	    switch (tok.type) {
	    case T_Time:
		if (SysTime != -1L) Usage();
		else {
		    SysTime = (long) tok.val * 60L;
#ifdef HAVE_QUEUED
		    DontQueue = 1;
		    Daemon = 0;
#endif
		}
		break;

	    case T_Month:
		if (m != NO_MON) Usage();
		else m = tok.val;
		break;

	    case T_Day:
		if (d != NO_DAY) Usage();
		else d = tok.val;
		break;

	    case T_Year:
		if (y != NO_YR) Usage();
		else y = tok.val;
		break;

	    case T_Rep:
		if (rep != NO_REP) Usage();
		else rep = tok.val;
		break;

	    default: Usage();
	    }
	}

	if (rep > 0) {
	    Iterations = rep;
	    DontQueue = 1;
	    Daemon = 0;
	}

/* Must supply date in the form:  day, mon, yr OR mon, yr */
	if (m != NO_MON || y != NO_YR || d != NO_DAY) {
	    if (m == NO_MON || y == NO_YR) {
		if (rep == NO_REP) Usage();
		else if (m != NO_MON || y != NO_YR) Usage();
		else {
		    m = CurMon;
		    y = CurYear;
		    if (d == NO_DAY) d = CurDay;
		}
	    }
	    if (d == NO_DAY) d=1;
	    if (d > DaysInMonth(m, y)) {
		fprintf(ErrFp, BadDate);
		Usage();
	    }
	    JulianToday = Julian(y, m, d);
	    if (JulianToday == -1) {
		fprintf(ErrFp, BadDate);
		Usage();
	    }
	    CurYear = y;
	    CurMon = m;
	    CurDay = d;
	    if (JulianToday != RealToday) IgnoreOnce = 1;
	}

    }
/* Figure out the offset from UTC */
    if (CalculateUTC)
	(void) CalcMinsFromUTC(JulianToday, SystemTime(1)/60,
			       &MinsFromUTC, NULL);
}

/***************************************************************/
/*                                                             */
/*  Usage                                                      */
/*                                                             */
/*  Print the usage info.                                      */
/*                                                             */
/***************************************************************/
#ifndef L_USAGE_OVERRIDE
#ifdef HAVE_PROTOS
PUBLIC void Usage(void)
#else
void Usage()
#endif /* HAVE_PROTOS */
{
    fprintf(ErrFp, "\nREMIND %s (%s version) Copyright 1992-1997 by David F. Skoll\n", VERSION, L_LANGNAME);
#ifdef BETA
    fprintf(ErrFp, ">>>> BETA VERSION <<<<\n");
#endif
    fprintf(ErrFp, "Usage: remind [options] filename [date] [time] [*rep]\n");
    fprintf(ErrFp, "Options:\n");
    fprintf(ErrFp, " -n     Output next occurrence of reminders in simple format\n");
    fprintf(ErrFp, " -r     Disable RUN directives\n");
    fprintf(ErrFp, " -c[n]  Produce a calendar for n (default 1) months\n");
    fprintf(ErrFp, " -c+[n] Produce a calendar for n (default 1) weeks\n");
    fprintf(ErrFp, " -w[n[,p[,s]]]  Specify width, padding and spacing of calendar\n");
    fprintf(ErrFp, " -s[+][n] Produce `simple calendar' for n (1) months (weeks)\n");
    fprintf(ErrFp, " -p[n]  Same as -s, but input compatible with rem2ps\n");
    fprintf(ErrFp, " -v     Verbose mode\n");
    fprintf(ErrFp, " -o     Ignore ONCE directives\n");
    fprintf(ErrFp, " -t     Trigger all future reminders regardless of delta\n");
    fprintf(ErrFp, " -h     `Hush' mode - be very quiet\n");
#ifdef HAVE_QUEUED
    fprintf(ErrFp, " -a     Don't trigger timed reminders immediately - just queue them\n");
    fprintf(ErrFp, " -q     Don't queue timed reminders\n");
    fprintf(ErrFp, " -f     Trigger timed reminders by staying in foreground\n");
    fprintf(ErrFp, " -z[n]  Enter daemon mode, waking every n (5) minutes.\n");
#endif
    fprintf(ErrFp, " -d...  Debug: e=echo x=expr-eval t=trig v=dumpvars l=showline\n");
    fprintf(ErrFp, " -e     Divert messages normally sent to stderr to stdout\n");
    fprintf(ErrFp, " -b[n]  Time format for cal: 0=am/pm, 1=24hr, 2=none\n");
    fprintf(ErrFp, " -x[n]  Iteration limit for SATISFY clause (def=150)\n");
    fprintf(ErrFp, " -kcmd  Run `cmd' for MSG-type reminders\n");
    fprintf(ErrFp, " -g[ddd] Sort reminders by date, time and priority before issuing\n");
    fprintf(ErrFp, " -ivar=val Initialize var to val and preserve var\n");
    fprintf(ErrFp, " -m     Start calendar with Monday rather than Sunday\n");
    exit(1);
}
#endif /* L_USAGE_OVERRIDE */
/***************************************************************/
/*                                                             */
/*  ChgUser                                                    */
/*                                                             */
/*  Run as a specified user.  Can only be used if Remind is    */
/*  started by root.  This changes the real and effective uid, */
/*  the real and effective gid, and sets the HOME, SHELL and   */
/*  USER environment variables.                                */
/*                                                             */
/***************************************************************/
#if defined(UNIX) && defined(WANT_U_OPTION)
#ifdef HAVE_PROTOS
PRIVATE void ChgUser(char *user)
#else
static void ChgUser(user)
char *user;
#endif /* HAVE_PROTOS */
{
    uid_t myuid;

    struct passwd *pwent;
    static char *home, *shell, *username, *logname;

    myuid = getuid();

    pwent = getpwnam(user);

    if (!pwent) {
	fprintf(ErrFp, ErrMsg[M_BAD_USER], user);
	exit(1);
    }

    if (!myuid && setgid(pwent->pw_gid)) {
	fprintf(ErrFp, ErrMsg[M_NO_CHG_GID], pwent->pw_gid);
	exit(1);
    }

    if (!myuid && setuid(pwent->pw_uid)) {
	fprintf(ErrFp, ErrMsg[M_NO_CHG_UID], pwent->pw_uid);
	exit(1);
    }

    home = malloc(strlen(pwent->pw_dir) + 6);
    if (!home) {
	fprintf(ErrFp, ErrMsg[M_NOMEM_ENV]);
	exit(1);
    }
    sprintf(home, "HOME=%s", pwent->pw_dir);
    putenv(home);

    shell = malloc(strlen(pwent->pw_shell) + 7);
    if (!shell) {
	fprintf(ErrFp, ErrMsg[M_NOMEM_ENV]);
	exit(1);
    }
    sprintf(shell, "SHELL=%s", pwent->pw_shell);
    putenv(shell);

    if (pwent->pw_uid) {
	username = malloc(strlen(pwent->pw_name) + 6);
	if (!username) {
	    fprintf(ErrFp, ErrMsg[M_NOMEM_ENV]);
	    exit(1);
	}
	sprintf(username, "USER=%s", pwent->pw_name);
	putenv(username);
	logname= malloc(strlen(pwent->pw_name) + 9);
	if (!logname) {
	    fprintf(ErrFp, ErrMsg[M_NOMEM_ENV]);
	    exit(1);
	}
	sprintf(logname, "LOGNAME=%s", pwent->pw_name);
	putenv(logname);
    }
}
#endif /* UNIX && WANT_U_OPTION */
   
/***************************************************************/
/*                                                             */
/*  InitializeVar                                              */
/*                                                             */
/*  Initialize and preserve a variable                         */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE void InitializeVar(char *str)
#else
static void InitializeVar(str)
char *str;
#endif
{
    char *varname, *expr;

    Value val;

    int r;

    /* Scan for an '=' sign */
    varname = str;
    while (*str && *str != '=') str++;
    if (!*str) {
	fprintf(ErrFp, ErrMsg[M_I_OPTION], ErrMsg[E_MISS_EQ]);
	return;
    }
    *str = 0;
    if (!*varname) {
	fprintf(ErrFp, ErrMsg[M_I_OPTION], ErrMsg[E_MISS_VAR]);
	return;
    }
    expr = str+1;
    if (!*expr) {
	fprintf(ErrFp, ErrMsg[M_I_OPTION], ErrMsg[E_MISS_EXPR]);
	return;
    }

    r=EvalExpr(&expr, &val);
    if (r) {
	fprintf(ErrFp, ErrMsg[M_I_OPTION], ErrMsg[r]);
	return;
    }

    if (*varname == '$') {
	r=SetSysVar(varname+1, &val);
	if (r) fprintf(ErrFp, ErrMsg[M_I_OPTION], ErrMsg[r]);
	return;
    }

    r=SetVar(varname, &val);
    if (r) {
	fprintf(ErrFp, ErrMsg[M_I_OPTION], ErrMsg[r]);
	return;
    }
    r=PreserveVar(varname);
    if (r) fprintf(ErrFp, ErrMsg[M_I_OPTION], ErrMsg[r]);
    return;
}

