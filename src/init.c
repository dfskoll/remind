/***************************************************************/
/*                                                             */
/*  INIT.C                                                     */
/*                                                             */
/*  Initialize remind; perform certain tasks between           */
/*  iterations in calendar mode; do certain checks after end   */
/*  in normal mode.                                            */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by Dianne Skoll                    */
/*  Copyright (C) 1999-2011 by Roaring Penguin Software Inc.   */
/*                                                             */
/***************************************************************/

#include "version.h"
#include "config.h"

#define L_IN_INIT 1
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>

#include "types.h"
#include "protos.h"
#include "expr.h"
#include "err.h"
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
 *  -l       = Prefix simple calendar lines with a comment containing
 *             their trigger line numbers and filenames
 *  -v       = Verbose mode
 *  -o       = Ignore ONCE directives
 *  -a       = Don't issue timed reminders which will be queued
 *  -q       = Don't queue timed reminders
 *  -t       = Trigger all reminders (infinite delta)
 *  -h       = Hush mode
 *  -f       = Do not fork
 *  -dchars  = Debugging mode:  Chars are:
 *             f = Trace file openings
 *             e = Echo input lines
 *             x = Display expression evaluation
 *             t = Display trigger dates
 *             v = Dump variables at end
 *             l = Display entire line in error messages
 *  -e       = Send messages normally sent to stderr to stdout instead
 *  -z[n]    = Daemon mode waking up every n (def 1) minutes.
 *  -bn      = Time format for cal (0, 1, or 2)
 *  -xn      = Max. number of iterations for SATISFY
 *  -uname   = Run as user 'name' - only valid when run by root.  If run
 *             by non-root, changes environment but not effective uid.
 *  -kcmd    = Run 'cmd' for MSG-type reminders instead of printing to stdout
 *  -iVAR=EXPR = Initialize and preserve VAR.
 *  -m       = Start calendar with Monday instead of Sunday.
 *  -j[n]    = Purge all junk from reminder files (n = INCLUDE depth)
 *  A minus sign alone indicates to take input from stdin
 *
 **************************************************************/
#if defined(__APPLE__) || defined(__CYGWIN__)
static void rkrphgvba(int x);
#endif

/* For parsing an integer */
#define PARSENUM(var, s)   \
var = 0;                   \
while (isdigit(*(s))) {    \
    var *= 10;             \
    var += *(s) - '0';     \
    s++;                   \
}

static void ChgUser(char const *u);
static void InitializeVar(char const *str);

static char const *BadDate = "Illegal date on command line\n";

static DynamicBuffer default_filename_buf;

/***************************************************************/
/*                                                             */
/*  DefaultFilename                                            */
/*                                                             */
/*  If we're invoked as "rem" rather than "remind", use a      */
/*  default filename.  Use $DOTREMINDERS or $HOME/.reminders   */
/*                                                             */
/***************************************************************/
static char const *DefaultFilename(void)
{
    char const *s;

    DBufInit(&default_filename_buf);

    s = getenv("DOTREMINDERS");
    if (s) {
	return s;
    }

    s = getenv("HOME");
    if (!s) {
	fprintf(stderr, "HOME environment variable not set.  Unable to determine reminder file.\n");
	exit(1);
    }
    DBufPuts(&default_filename_buf, s);
    DBufPuts(&default_filename_buf, "/.reminders");
    return DBufValue(&default_filename_buf);
}

/***************************************************************/
/*                                                             */
/*  InitRemind                                                 */
/*                                                             */
/*  Initialize the system - called only once at beginning!     */
/*                                                             */
/***************************************************************/
void InitRemind(int argc, char const *argv[])
{
    char const *arg;
    int i;
    int y, m, d, rep;
    Token tok;
    int InvokedAsRem = 0;
    char const *s;
    int weeks;

    int jul;

#if defined(__APPLE__)
    rkrphgvba(0);
#elif defined(__CYGWIN__)
    rkrphgvba(1);
#endif

    jul = NO_DATE;

    /* Initialize global dynamic buffers */
    DBufInit(&Banner);
    DBufInit(&LineBuffer);
    DBufInit(&ExprBuf);

    DBufPuts(&Banner, L_BANNER);

    PurgeFP = NULL;

    /* Make sure remind is not installed set-uid or set-gid */
    if (getgid() != getegid() ||
	getuid() != geteuid()) {
	fprintf(ErrFp, "\nRemind should not be installed set-uid or set-gid.\nCHECK YOUR SYSTEM SECURITY.\n");
	exit(1);
    }

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

    /* See if we were invoked as "rem" rather than "remind" */
    if (argv[0]) {
	s = strrchr(argv[0], '/');
	if (!s) {
	    s = argv[0];
	} else {
	    s++;
	}
	if (!strcmp(s, "rem")) {
	    InvokedAsRem = 1;
	}
    }

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

	    case 'j':
	    case 'J':
		PurgeMode = 1;
	        if (*arg) {
		    PARSENUM(PurgeIncludeDepth, arg);
                }
	        break;
            case 'i':
	    case 'I':
		InitializeVar(arg);
		while(*arg) arg++;
		break;

	    case 'n':
	    case 'N':
		NextMode = 1;
		DontQueue = 1;
		Daemon = 0;
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

	    case 'y':
	    case 'Y':
		SynthesizeTags = 1;
		break;

	    case 't':
	    case 'T':
		if (!*arg) {
		    InfiniteDelta = 1;
		} else {
		    PARSENUM(DeltaOffset, arg);
		    if (DeltaOffset < 0) {
			DeltaOffset = 0;
		    }
		}
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
		UntimedBeforeTimed = 0;
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
		if (*arg) {
		    if (*arg == 'D' || *arg == 'd')
			UntimedBeforeTimed = 1;
		    arg++;
		}
		break;

	    case 'u':
	    case 'U':
		ChgUser(arg);
		RunDisabled = RUN_CMDLINE;
		while (*arg) arg++;
		break;
	    case 'z':
	    case 'Z':
		DontFork = 1;
		if (*arg == '0') {
		    PARSENUM(Daemon, arg);
		    if (Daemon == 0) Daemon = -1;
		    else if (Daemon < 1) Daemon = 1;
		    else if (Daemon > 60) Daemon = 60;
		} else {
		    PARSENUM(Daemon, arg);
		    if (Daemon<1) Daemon=1;
		    else if (Daemon>60) Daemon=60;
		}
		break;

	    case 'a':
	    case 'A':
		DontIssueAts++;
		break;

	    case 'q':
	    case 'Q':
		DontQueue = 1;
		break;

	    case 'f':
	    case 'F':
		DontFork = 1;
		break;
	    case 'c':
	    case 'C':
		DoCalendar = 1;
		weeks = 0;
		/* Parse the flags */
		while(*arg) {
		    if (*arg == 'a' ||
		        *arg == 'A') {
		        DoSimpleCalDelta = 1;
			arg++;
			continue;
		    }
		    if (*arg == '+') {
		        weeks = 1;
			arg++;
			continue;
		    }
		    if (*arg == 'l' || *arg == 'L') {
		        UseVTChars = 1;
			arg++;
			continue;
		    }
		    if (*arg == 'u' || *arg == 'U') {
			UseUTF8Chars = 1;
			arg++;
			continue;
		    }
		    if (*arg == 'c' || *arg == 'C') {
		        UseVTColors = 1;
		        arg++;
		        continue;
		    }
		    break;
		}
		if (weeks) {
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
		weeks = 0;
		while(*arg) {
		    if (*arg == 'a' || *arg == 'A') {
			DoSimpleCalDelta = 1;
			arg++;
			continue;
		    }
		    if (*arg == '+') {
		        arg++;
			weeks = 1;
			continue;
		    }
		    break;
		}
		if (weeks) {
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
		if (*arg == 'a' || *arg == 'A') {
		    DoSimpleCalDelta = 1;
		    arg++;
		}
		PARSENUM(CalMonths, arg);
		if (!CalMonths) CalMonths = 1;
		break;

	    case 'l':
	    case 'L':
		DoPrefixLineNo = 1;
		break;

	    case 'w':
	    case 'W':
		if (*arg != ',') {
		    PARSENUM(CalWidth, arg);
		    if (CalWidth < 71) CalWidth = 71;
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
		    case 'e': case 'E': DebugFlag |= DB_ECHO_LINE;   break;
		    case 'x': case 'X': DebugFlag |= DB_PRTEXPR;     break;
		    case 't': case 'T': DebugFlag |= DB_PRTTRIG;     break;
		    case 'v': case 'V': DebugFlag |= DB_DUMP_VARS;   break;
		    case 'l': case 'L': DebugFlag |= DB_PRTLINE;     break;
		    case 'f': case 'F': DebugFlag |= DB_TRACE_FILES; break;
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
    if (!InvokedAsRem) {
	if (i >= argc) {
	    Usage();
	    exit(1);
	}
	InitialFile = argv[i++];
    } else {
	InitialFile = DefaultFilename();
    }

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
		    DontQueue = 1;
		    Daemon = 0;
		}
		break;

	    case T_DateTime:
		if (SysTime != -1L) Usage();
		if (m != NO_MON || d != NO_DAY || y != NO_YR || jul != NO_DATE) Usage();
		SysTime = (tok.val % MINUTES_PER_DAY) * 60;
		DontQueue = 1;
		Daemon = 0;
		jul = tok.val / MINUTES_PER_DAY;
		break;

	    case T_Date:
		if (m != NO_MON || d != NO_DAY || y != NO_YR || jul != NO_DATE) Usage();
		jul = tok.val;
		break;

	    case T_Month:
		if (m != NO_MON || jul != NO_DATE) Usage();
		else m = tok.val;
		break;

	    case T_Day:
		if (d != NO_DAY || jul != NO_DATE) Usage();
		else d = tok.val;
		break;

	    case T_Year:
		if (y != NO_YR || jul != NO_DATE) Usage();
		else y = tok.val;
		break;

	    case T_Rep:
		if (rep != NO_REP) Usage();
		else rep = tok.val;
		break;

	    default:
		Usage();
	    }
	}

	if (rep > 0) {
	    Iterations = rep;
	    DontQueue = 1;
	    Daemon = 0;
	}

	if (jul != NO_DATE) {
	    FromJulian(jul, &y, &m, &d);
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
		fprintf(ErrFp, "%s", BadDate);
		Usage();
	    }
	    JulianToday = Julian(y, m, d);
	    if (JulianToday == -1) {
		fprintf(ErrFp, "%s", BadDate);
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
void Usage(void)
{
    fprintf(ErrFp, "\nREMIND %s (%s version) Copyright 1992-1998 Dianne Skoll\n", VERSION, L_LANGNAME);
    fprintf(ErrFp, "Copyright 1999-2011 Roaring Penguin Software Inc.\n");
#ifdef BETA
    fprintf(ErrFp, ">>>> BETA VERSION <<<<\n");
#endif
    fprintf(ErrFp, "Usage: remind [options] filename [date] [time] [*rep]\n");
    fprintf(ErrFp, "Options:\n");
    fprintf(ErrFp, " -n     Output next occurrence of reminders in simple format\n");
    fprintf(ErrFp, " -r     Disable RUN directives\n");
    fprintf(ErrFp, " -c[a][n] Produce a calendar for n (default 1) months\n");
    fprintf(ErrFp, " -c[a]+[n] Produce a calendar for n (default 1) weeks\n");
    fprintf(ErrFp, " -w[n[,p[,s]]]  Specify width, padding and spacing of calendar\n");
    fprintf(ErrFp, " -s[a][+][n] Produce `simple calendar' for n (1) months (weeks)\n");
    fprintf(ErrFp, " -p[a][n] Same as -s, but input compatible with rem2ps\n");
    fprintf(ErrFp, " -l     Prefix each simple calendar line with line number and filename comment\n");
    fprintf(ErrFp, " -v     Verbose mode\n");
    fprintf(ErrFp, " -o     Ignore ONCE directives\n");
    fprintf(ErrFp, " -t[n]  Trigger all future (or those within `n' days)\n");
    fprintf(ErrFp, " -h     `Hush' mode - be very quiet\n");
    fprintf(ErrFp, " -a     Don't trigger timed reminders immediately - just queue them\n");
    fprintf(ErrFp, " -q     Don't queue timed reminders\n");
    fprintf(ErrFp, " -f     Trigger timed reminders by staying in foreground\n");
    fprintf(ErrFp, " -z[n]  Enter daemon mode, waking every n (1) minutes.\n");
    fprintf(ErrFp, " -d...  Debug: e=echo x=expr-eval t=trig v=dumpvars l=showline f=tracefiles\n");
    fprintf(ErrFp, " -e     Divert messages normally sent to stderr to stdout\n");
    fprintf(ErrFp, " -b[n]  Time format for cal: 0=am/pm, 1=24hr, 2=none\n");
    fprintf(ErrFp, " -x[n]  Iteration limit for SATISFY clause (def=150)\n");
    fprintf(ErrFp, " -kcmd  Run `cmd' for MSG-type reminders\n");
    fprintf(ErrFp, " -g[dddd] Sort reminders by date, time, priority, and 'timedness'\n");
    fprintf(ErrFp, " -ivar=val Initialize var to val and preserve var\n");
    fprintf(ErrFp, " -m     Start calendar with Monday rather than Sunday\n");
    fprintf(ErrFp, " -y     Synthesize tags for tagless reminders\n");
    fprintf(ErrFp, " -j[n]  Run in 'purge' mode.  [n = INCLUDE depth]\n");
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
static void ChgUser(char const *user)
{
    uid_t myuid;

    struct passwd *pwent;
    static char *home;
    static char *shell;
    static char *username;
    static char *logname;

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
	fprintf(ErrFp, "%s", ErrMsg[M_NOMEM_ENV]);
	exit(1);
    }
    sprintf(home, "HOME=%s", pwent->pw_dir);
    putenv(home);

    shell = malloc(strlen(pwent->pw_shell) + 7);
    if (!shell) {
	fprintf(ErrFp, "%s", ErrMsg[M_NOMEM_ENV]);
	exit(1);
    }
    sprintf(shell, "SHELL=%s", pwent->pw_shell);
    putenv(shell);

    if (pwent->pw_uid) {
	username = malloc(strlen(pwent->pw_name) + 6);
	if (!username) {
	    fprintf(ErrFp, "%s", ErrMsg[M_NOMEM_ENV]);
	    exit(1);
	}
	sprintf(username, "USER=%s", pwent->pw_name);
	putenv(username);
	logname= malloc(strlen(pwent->pw_name) + 9);
	if (!logname) {
	    fprintf(ErrFp, "%s", ErrMsg[M_NOMEM_ENV]);
	    exit(1);
	}
	sprintf(logname, "LOGNAME=%s", pwent->pw_name);
	putenv(logname);
    }
}

static void
DefineFunction(char const *str)
{
    Parser p;
    int r;

    CreateParser(str, &p);
    r = DoFset(&p);
    DestroyParser(&p);
    if (r != OK) {
	fprintf(ErrFp, "-i option: %s: %s\n", str, ErrMsg[r]);
    }
}
/***************************************************************/
/*                                                             */
/*  InitializeVar                                              */
/*                                                             */
/*  Initialize and preserve a variable                         */
/*                                                             */
/***************************************************************/
static void InitializeVar(char const *str)
{
    char const *expr;
    char const *ostr = str;
    char varname[VAR_NAME_LEN+1];

    Value val;

    int r;

    /* Scan for an '=' sign */
    r = 0;
    while (*str && *str != '=') {
	if (r < VAR_NAME_LEN) {
	    varname[r++] = *str;
	}
	if (*str == '(') {
	    /* Do a function definition if we see a paren */
	    DefineFunction(ostr);
	    return;
	}
	str++;
    }
    varname[r] = 0;
    if (!*str) {
	fprintf(ErrFp, ErrMsg[M_I_OPTION], ErrMsg[E_MISS_EQ]);
	return;
    }
    if (!*varname) {
	fprintf(ErrFp, ErrMsg[M_I_OPTION], ErrMsg[E_MISS_VAR]);
	return;
    }
    expr = str+1;
    if (!*expr) {
	fprintf(ErrFp, ErrMsg[M_I_OPTION], ErrMsg[E_MISS_EXPR]);
	return;
    }

    r=EvalExpr(&expr, &val, NULL);
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

#if defined(__APPLE__) || defined(__CYGWIN__)
static char const pmsg1[] = {
    0x4c, 0x62, 0x68, 0x20, 0x6e, 0x63, 0x63, 0x72, 0x6e, 0x65, 0x20,
    0x67, 0x62, 0x20, 0x6f, 0x72, 0x20, 0x65, 0x68, 0x61, 0x61, 0x76,
    0x61, 0x74, 0x20, 0x45, 0x72, 0x7a, 0x76, 0x61, 0x71, 0x20, 0x62,
    0x61, 0x20, 0x6e, 0x61, 0x20, 0x4e, 0x63, 0x63, 0x79, 0x72, 0x20,
    0x63, 0x65, 0x62, 0x71, 0x68, 0x70, 0x67, 0x2e, 0x20, 0x20, 0x56,
    0x27, 0x71, 0x20, 0x65, 0x6e, 0x67, 0x75, 0x72, 0x65, 0x20, 0x67,
    0x75, 0x6e, 0x67, 0x0a, 0x6c, 0x62, 0x68, 0x20, 0x71, 0x76, 0x71,
    0x61, 0x27, 0x67, 0x2e, 0x20, 0x20, 0x45, 0x72, 0x7a, 0x76, 0x61,
    0x71, 0x20, 0x72, 0x6b, 0x72, 0x70, 0x68, 0x67, 0x76, 0x62, 0x61,
    0x20, 0x6a, 0x76, 0x79, 0x79, 0x20, 0x70, 0x62, 0x61, 0x67, 0x76,
    0x61, 0x68, 0x72, 0x20, 0x7a, 0x62, 0x7a, 0x72, 0x61, 0x67, 0x6e,
    0x65, 0x76, 0x79, 0x6c, 0x2e, 0x0a, 0x00
};

static char const pmsg2[] = {
    0x4c, 0x62, 0x68, 0x20, 0x6e, 0x63, 0x63, 0x72, 0x6e, 0x65, 0x20,
    0x67, 0x62, 0x20, 0x6f, 0x72, 0x20, 0x65, 0x68, 0x61, 0x61, 0x76,
    0x61, 0x74, 0x20, 0x45, 0x72, 0x7a, 0x76, 0x61, 0x71, 0x20, 0x62,
    0x61, 0x20, 0x6e, 0x20, 0x5a, 0x76, 0x70, 0x65, 0x62, 0x66, 0x62,
    0x73, 0x67, 0x20, 0x66, 0x6c, 0x66, 0x67, 0x72, 0x7a, 0x2e, 0x20,
    0x20, 0x56, 0x27, 0x71, 0x20, 0x65, 0x6e, 0x67, 0x75, 0x72, 0x65,
    0x20, 0x67, 0x75, 0x6e, 0x67, 0x0a, 0x6c, 0x62, 0x68, 0x20, 0x71,
    0x76, 0x71, 0x61, 0x27, 0x67, 0x2e, 0x20, 0x20, 0x45, 0x72, 0x7a,
    0x76, 0x61, 0x71, 0x20, 0x72, 0x6b, 0x72, 0x70, 0x68, 0x67, 0x76,
    0x62, 0x61, 0x20, 0x6a, 0x76, 0x79, 0x79, 0x20, 0x70, 0x62, 0x61,
    0x67, 0x76, 0x61, 0x68, 0x72, 0x20, 0x7a, 0x62, 0x7a, 0x72, 0x61,
    0x67, 0x6e, 0x65, 0x76, 0x79, 0x6c, 0x2e, 0x0a, 0x00
};

static void
rkrphgvba(int x)
{
    char const *s = (x ? pmsg2 : pmsg1);
    while(*s) {
	int c = (int) *s++;
	c=isalpha(c)?tolower(c)<0x6e?c+13:c-13:c;
	putchar(c);
    }
    sleep(5);
}
#endif
