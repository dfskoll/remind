/***************************************************************/
/*                                                             */
/*  DOREM.C                                                    */
/*                                                             */
/*  Contains routines for parsing reminders and evaluating     */
/*  triggers.  Also contains routines for parsing OMIT         */
/*  commands.                                                  */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1997 by David F. Skoll                  */
/*                                                             */
/***************************************************************/

#include "config.h"
static char const RCSID[] = "$Id: dorem.c,v 1.5 1997-03-30 19:07:37 dfs Exp $";

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include "globals.h"
#include "err.h"
#include "types.h"
#include "protos.h"
#include "expr.h"

/* Define the shell characters to escape */
static char const EscapeMe[] =
#ifdef QDOS
"\"'!$%^&*()|<>[]{}\x9F~;?\\";
#else
"\"'!$%^&*()|<>[]{}`~;?\\";
#endif

PRIVATE int ParseTimeTrig ARGS ((ParsePtr s, TimeTrig *tim));
PRIVATE int ParseLocalOmit ARGS ((ParsePtr s, Trigger *t));
PRIVATE int ParseScanFrom ARGS ((ParsePtr s, Trigger *t));
PRIVATE int ParsePriority ARGS ((ParsePtr s, Trigger *t));
PRIVATE int ParseUntil ARGS ((ParsePtr s, Trigger *t));
PRIVATE int ShouldTriggerBasedOnWarn ARGS ((Trigger *t, int jul));

/***************************************************************/
/*                                                             */
/*  DoRem                                                      */
/*                                                             */
/*  Do the REM command.                                        */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int DoRem(ParsePtr p)
#else
int DoRem(p)
ParsePtr p;
#endif
{

    Trigger trig;
    TimeTrig tim;
    int r;
    int jul;
    char buf[TOKSIZE];
    Token tok;

    /* Parse the trigger date and time */
    if ( (r=ParseRem(p, &trig, &tim)) ) return r;

    if (trig.typ == NO_TYPE) return E_EOLN;
    if (trig.typ == SAT_TYPE) {
	r=DoSatRemind(&trig, &tim, p);
	if (r) return r;
	r=ParseToken(p, buf);
	if (r) return r;
	FindToken(buf, &tok);
	if (tok.type == T_Empty || tok.type == T_Comment) return OK;
	if (tok.type != T_RemType || tok.val == SAT_TYPE) return E_PARSE_ERR;
	trig.typ = tok.val;
	jul = LastTriggerDate;
	if (!LastTrigValid) return OK;
    } else {
	/* Calculate the trigger date */
	jul = ComputeTrigger(trig.scanfrom, &trig, &r);
	if (r) return r;
    }
   
/* Queue the reminder, if necessary */
#ifdef HAVE_QUEUED
    if (jul == JulianToday &&
	!(!IgnoreOnce &&
	  trig.once != NO_ONCE &&
	  FileAccessDate == JulianToday))
	QueueReminder(p, trig.typ, &tim, trig.sched);
/* If we're in daemon mode, do nothing over here */
    if (Daemon) return OK;
#endif


    if (ShouldTriggerReminder(&trig, &tim, jul)) {
#ifdef OS2_POPUP
	if ( (r=TriggerReminder(p, &trig, &tim, jul, 0)) ) {
#else
	    if ( (r=TriggerReminder(p, &trig, &tim, jul)) ) {
#endif
		return r;
	    }
	}

	return OK;
    }   

/***************************************************************/
/*                                                             */
/*  ParseRem                                                   */
/*                                                             */
/*  Given a parse pointer, parse line and fill in a            */
/*  trigger structure.                                         */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
    PUBLIC int ParseRem(ParsePtr s, Trigger *trig, TimeTrig *tim)
#else
	int ParseRem(s, trig, tim)
	ParsePtr s;
    Trigger *trig;
    TimeTrig *tim;
#endif
    {
	register int r;
	Token tok;

	trig->y = NO_YR;
	trig->m = NO_MON;
	trig->d = NO_DAY;
	trig->wd = NO_WD;
	trig->back = NO_BACK;
	trig->delta = NO_DELTA;
	trig->until = NO_UNTIL;
	trig->rep  = NO_REP;
	trig->localomit = NO_WD;
	trig->skip = NO_SKIP;
	trig->once = NO_ONCE;
	trig->typ = NO_TYPE;
	trig->scanfrom = NO_DATE;
	trig->priority = DefaultPrio;
	trig->sched[0] = 0;
	trig->warn[0] = 0;
	tim->ttime = NO_TIME;
	tim->delta = NO_DELTA;
	tim->rep   = NO_REP;

	while(1) {
	    /* Read space-delimited string */
	    r = ParseToken(s, TokBuffer);
	    if (r) return r;

	    /* Figure out what we've got */
	    FindToken(TokBuffer, &tok);
	    switch(tok.type) {
	    case T_WkDay:
		if (trig->wd & (1 << tok.val)) return E_WD_TWICE;
		trig->wd |= (1 << tok.val);
		break;

	    case T_Month:
		if (trig->m != NO_MON) return E_MON_TWICE;
		trig->m = tok.val;
		break;

	    case T_Skip:
		if (trig->skip != NO_SKIP) return E_SKIP_ERR;
		trig->skip = tok.val;
		break;

	    case T_Priority:
		r=ParsePriority(s, trig);
		if (r) return r;
		break;

	    case T_At:
		r=ParseTimeTrig(s, tim);
		if (r) return r;
		break;

	    case T_Scanfrom:
		r=ParseScanFrom(s, trig);
		if (r) return r;
		break;

	    case T_RemType:
		trig->typ = tok.val;
		if (s->isnested) return E_CANT_NEST_RTYPE;
		if (trig->scanfrom == NO_DATE) trig->scanfrom = JulianToday;
		return OK;

	    case T_Until:
		r=ParseUntil(s, trig);
		if (r) return r;
		break;

	    case T_Year:
		if (trig->y != NO_YR) return E_YR_TWICE;
		trig->y = tok.val;
		break;

	    case T_Day:
		if (trig->d != NO_DAY) return E_DAY_TWICE;
		trig->d = tok.val;
		break;

	    case T_Rep:
		if (trig->rep != NO_REP) return E_REP_TWICE;
		trig->rep = tok.val;
		break;

	    case T_Delta:
		if (trig->delta != NO_DELTA) return E_DELTA_TWICE;
		trig->delta = tok.val;
		break;

	    case T_Back:
		if (trig->back != NO_BACK) return E_BACK_TWICE;
		trig->back = tok.val;
		break;

	    case T_Once:
		if (trig->once != NO_ONCE) return E_ONCE_TWICE;
		trig->once = ONCE_ONCE;
		break;

	    case T_Omit:
		r = ParseLocalOmit(s, trig);
		if (r) return r;
		break;

	    case T_Empty:
		if (trig->scanfrom == NO_DATE) trig->scanfrom = JulianToday;
		return OK;

	    case T_Warn:
		r=ParseToken(s, TokBuffer);
		if(r) return r;
		StrnCpy(trig->warn, TokBuffer, VAR_NAME_LEN);
		break;

	    case T_Sched:
		r=ParseToken(s, TokBuffer);
		if(r) return r;
		StrnCpy(trig->sched, TokBuffer, VAR_NAME_LEN);
		break;

	    default:
		Eprint("%s: %s", ErrMsg[E_UNKNOWN_TOKEN], TokBuffer);
		return E_UNKNOWN_TOKEN;
	    }
	}
    }

/***************************************************************/
/*                                                             */
/*  ParseTimeTrig - parse the AT part of a timed reminder      */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
    PRIVATE int ParseTimeTrig(ParsePtr s, TimeTrig *tim)
#else
	static int ParseTimeTrig(s, tim)
	ParsePtr s;
    TimeTrig *tim;
#endif
    {
	Token tok;
	int r;

	while(1) {
	    r = ParseToken(s, TokBuffer);
	    if (r) return r;
	    FindToken(TokBuffer, &tok);
	    switch(tok.type) {
	    case T_Time:
		tim->ttime = tok.val;
		break;

	    case T_Delta:
		tim->delta = (tok.val > 0) ? tok.val : -tok.val;
		break;

	    case T_Rep:
		tim->rep = tok.val;
		break;

	    default:
		if (tim->ttime == NO_TIME) return E_EXPECT_TIME;
/* Save in global variable */
		LastTriggerTime = tim->ttime;
		PushToken(TokBuffer);
		return OK;
	    }
	}
    }

/***************************************************************/
/*                                                             */
/*  ParseLocalOmit - parse the local OMIT portion of a         */
/*  reminder.                                                  */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
    PRIVATE int ParseLocalOmit(ParsePtr s, Trigger *t)
#else
	static int ParseLocalOmit(s, t)
	ParsePtr s;
    Trigger *t;
#endif
    {
	Token tok;
	int r;

	while(1) {
	    r = ParseToken(s, TokBuffer);
	    if (r) return r;
	    FindToken(TokBuffer, &tok);
	    switch(tok.type) {
	    case T_WkDay:
		t->localomit |= (1 << tok.val);
		break;

	    default:
		PushToken(TokBuffer);
		return OK;
	    }
	}
    }

/***************************************************************/
/*                                                             */
/*  ParseUntil - parse the UNTIL portion of a reminder         */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
    PRIVATE int ParseUntil(ParsePtr s, Trigger *t)
#else
	static int ParseUntil(s, t)
	ParsePtr s;
    Trigger *t;
#endif
    {
	int y = NO_YR,
	    m = NO_MON,
	    d = NO_DAY;

	Token tok;
	int r;

	if (t->until != NO_UNTIL) return E_UNTIL_TWICE;

	while(1) {
	    r = ParseToken(s, TokBuffer);
	    if (r) return r;
	    FindToken(TokBuffer, &tok);
	    switch(tok.type) {
	    case T_Year:
		if (y != NO_YR) {
		    Eprint("UNTIL: %s", ErrMsg[E_YR_TWICE]);
		    return E_YR_TWICE;
		}
		y = tok.val;
		break;

	    case T_Month:
		if (m != NO_MON) {
		    Eprint("UNTIL: %s", ErrMsg[E_MON_TWICE]);
		    return E_MON_TWICE;
		}
		m = tok.val;
		break;

	    case T_Day:
		if (d != NO_DAY) {
		    Eprint("UNTIL: %s", ErrMsg[E_DAY_TWICE]);
		    return E_DAY_TWICE;
		}
		d = tok.val;
		break;

	    default:
		if (y == NO_YR || m == NO_MON || d == NO_DAY) {
		    Eprint("UNTIL: %s", ErrMsg[E_INCOMPLETE]);
		    return E_INCOMPLETE;
		}
		if (!DateOK(y, m, d)) return E_BAD_DATE;
		t->until = Julian(y, m, d);
		PushToken(TokBuffer);
		return OK;
	    }
	}
    }

/***************************************************************/
/*                                                             */
/*  ParseScanFrom - parse the SCANFROM portion of a reminder   */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
    PRIVATE int ParseScanFrom(ParsePtr s, Trigger *t)
#else
	static int ParseScanFrom(s, t)
	ParsePtr s;
    Trigger *t;
#endif
    {
	int y = NO_YR,
	    m = NO_MON,
	    d = NO_DAY;

	Token tok;
	int r;

	if (t->scanfrom != NO_DATE) return E_SCAN_TWICE;

	while(1) {
	    r = ParseToken(s, TokBuffer);
	    if (r) return r;
	    FindToken(TokBuffer, &tok);
	    switch(tok.type) {
	    case T_Year:
		if (y != NO_YR) {
		    Eprint("SCANFROM: %s", ErrMsg[E_YR_TWICE]);
		    return E_YR_TWICE;
		}
		y = tok.val;
		break;

	    case T_Month:
		if (m != NO_MON) {
		    Eprint("SCANFROM: %s", ErrMsg[E_MON_TWICE]);
		    return E_MON_TWICE;
		}
		m = tok.val;
		break;

	    case T_Day:
		if (d != NO_DAY) {
		    Eprint("SCANFROM: %s", ErrMsg[E_DAY_TWICE]);
		    return E_DAY_TWICE;
		}
		d = tok.val;
		break;

	    default:
		if (y == NO_YR || m == NO_MON || d == NO_DAY) {
		    Eprint("SCANFROM: %s", ErrMsg[E_INCOMPLETE]);
		    return E_INCOMPLETE;
		}
		if (!DateOK(y, m, d)) return E_BAD_DATE;
		t->scanfrom = Julian(y, m, d);
		PushToken(TokBuffer);
		return OK;
	    }
	}
    }
/***************************************************************/
/*                                                             */
/*  TriggerReminder                                            */
/*                                                             */
/*  Trigger the reminder if it's a RUN or MSG type.            */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
#ifdef OS2_POPUP
    PUBLIC int TriggerReminder(ParsePtr p, Trigger *t, TimeTrig *tim, int jul,
			       int AsPopUp)
#else /* ! OS2_POPUP */
	PUBLIC int TriggerReminder(ParsePtr p, Trigger *t, TimeTrig *tim, int jul)
#endif /* OS2_POPUP */
#else /* ! HAVE_PROTOS */
#ifdef OS2_POPUP
	int TriggerReminder(p, t, tim, jul, AsPopUp)
	ParsePtr p;
    Trigger *t;
    TimeTrig *tim;
    int jul;
    int AsPopUp;
#else /* ! OS2_POPUP */
    int TriggerReminder(p, t, tim, jul)
	ParsePtr p;
    Trigger *t;
    TimeTrig *tim;
    int jul;
#endif /* OS2_POPUP */
#endif /* HAVE_PROTOS */
    {
	int r, y, m, d;
	char PrioExpr[25];
	static char buf[LINELEN+TOKSIZE];
	char *s, *s2;
	Value v;

	if (t->typ == RUN_TYPE && RunDisabled) return E_RUN_DISABLED;
	if (t->typ == CAL_TYPE || t->typ == PS_TYPE || t->typ == PSF_TYPE)
	    return OK;

/* If it's a MSG-type reminder, and no -k option was used, issue the banner. */
	if ((t->typ == MSG_TYPE || t->typ == MSF_TYPE) 
	    && !NumTriggered && !NextMode && !MsgCommand) {
	    if (!DoSubstFromString(Banner, SubstBuffer, JulianToday, NO_TIME) && *SubstBuffer)
#ifdef OS2_POPUP
		if (AsPopUp)
		    PutlPopUp(SubstBuffer);
		else
		    printf("%s\n", SubstBuffer);
#else
	    printf("%s\n", SubstBuffer);
#endif
	}

/* If it's NextMode, process as a CAL-type entry, and issue simple-calendar
   format. */
	if (NextMode) {
	    if ( (r=DoSubst(p, SubstBuffer, t, tim, jul, CAL_MODE)) ) return r;
	    if (!*SubstBuffer) return OK;
	    FromJulian(jul, &y, &m, &d);
#ifdef OS2_POPUP
	    if (AsPopUp) {
		sprintf(buf, "%04d%c%02d%c%02d %s", y, DATESEP, m+1, DATESEP, d,
			SimpleTime(tim->ttime, NULL));
		StartPopUp();
		PutsPopUp(buf);
		PutlPopUp(SubstBuffer);
	    }
	    else
		printf("%04d%c%02d%c%02d %s%s\n", y, DATESEP, m+1, DATESEP, d,
		       SimpleTime(tim->ttime, NULL),
		       SubstBuffer);
#else
	    printf("%04d%c%02d%c%02d %s%s\n", y, DATESEP, m+1, DATESEP, d,
		   SimpleTime(tim->ttime, NULL),
		   SubstBuffer);
#endif
	    return OK;
	}

/* Put the substituted string into the SubstBuffer */
	s2 = buf;
	*s2 = 0;
	if (UserFuncExists("msgprefix") == 1) {
	    sprintf(PrioExpr, "msgprefix(%d)", t->priority);
	    s = PrioExpr;
	    r = EvalExpr(&s, &v);
	    if (!r) {
		if (!DoCoerce(STR_TYPE, &v)) {
		    sprintf(s2, "%s", v.v.str);
		    s2 += strlen(s2);
		}
		DestroyValue(v);
	    }
	}
	if ( (r=DoSubst(p, s2, t, tim, jul, NORMAL_MODE)) ) return r;
	s2 += strlen(s2);
	if (UserFuncExists("msgsuffix") == 1) {
	    sprintf(PrioExpr, "msgsuffix(%d)", t->priority);
	    s = PrioExpr;
	    r = EvalExpr(&s, &v);
	    if (!r) {
		if (!DoCoerce(STR_TYPE, &v)) {
		    sprintf(s2, "%s", v.v.str);
		    s2 += strlen(s2);
		}
		DestroyValue(v);
	    }
	}
	if ((!MsgCommand && t->typ == MSG_TYPE) || t->typ == MSF_TYPE) {
	    *s2++ = '\n';
	}
	*s2 = 0;

/* If we are sorting, just queue it up in the sort buffer */
	if (SortByDate) {
	    if (InsertIntoSortBuffer(jul, tim->ttime, buf, t->typ, t->priority) == OK) {
		NumTriggered++;
		return OK;
	    }
	}

/* If we didn't insert the reminder into the sort buffer, issue the
   reminder now. */
	switch(t->typ) {
	case MSG_TYPE:
	    if (MsgCommand) {
		DoMsgCommand(MsgCommand, buf);
	    } else {
#ifdef OS2_POPUP
		if (AsPopUp)
		    PutlPopUp(buf);
		else
		    printf("%s", buf);
#else
		printf("%s", buf);
#endif
	    }
	    break;

	case MSF_TYPE:
#ifdef OS2_POPUP
	    if (AsPopUp) {
		StartPopUp();
		FillParagraph(buf, 1);
		EndPopUp();
	    } else {
		FillParagraph(buf, 0);
	    }
#else
	    FillParagraph(buf);
#endif
	    break;

	case RUN_TYPE:
	    system(buf);
	    break;

	default: /* Unknown/illegal type? */
	    return E_SWERR;
	}

	NumTriggered++;
	return OK;
    }

/***************************************************************/
/*                                                             */
/*  ShouldTriggerReminder                                      */
/*                                                             */
/*  Return 1 if we should trigger a reminder, based on today's */
/*  date and the trigger.  Return 0 if reminder should not be  */
/*  triggered.                                                 */
/*                                                             */
/***************************************************************/
#ifdef __TURBOC__
#pragma argsused
#endif
#ifdef HAVE_PROTOS
PUBLIC int ShouldTriggerReminder(Trigger *t, TimeTrig *tim, int jul)
#else
int ShouldTriggerReminder(t, tim, jul)
Trigger *t;
TimeTrig *tim;
int jul;
#endif
    {
	int r;

	/* Handle the ONCE modifier in the reminder. */
	if (!IgnoreOnce && t->once !=NO_ONCE && FileAccessDate == JulianToday)
	    return 0;
   
	if (jul < JulianToday) return 0;

	/* Don't trigger timed reminders if DontIssueAts is true, and if the
	   reminder is for today */

#ifdef HAVE_QUEUED
	if (jul == JulianToday && DontIssueAts && tim->ttime != NO_TIME) return 0;
#endif

	/* Don't trigger "old" timed reminders */
/*** REMOVED...
  if (jul == JulianToday &&
  tim->ttime != NO_TIME &&
  tim->ttime < SystemTime(0) / 60) return 0;
  *** ...UNTIL HERE */

	/* If "infinite delta" option is chosen, always trigger future reminders */
	if (InfiniteDelta || NextMode) return 1;

	/* If there's a "warn" function, it overrides any deltas */
	if (t->warn[0] != 0) {
	    return ShouldTriggerBasedOnWarn(t, jul);
	}

	/* Move back by delta days, if any */
	if (t->delta != NO_DELTA) {
	    if (t->delta < 0)
		jul = jul + t->delta;
	    else {
		r = t->delta;
		while(r && jul > JulianToday) {
		    jul--;
		    if (!IsOmitted(jul, t->localomit)) r--;
		}
	    }
	}

	/* Should we trigger the reminder? */
	return (jul <= JulianToday);
    }

/***************************************************************/
/*                                                             */
/*  DoSatRemind                                                */
/*                                                             */
/*  Do the "satisfying..." remind calculation.                 */
/*                                                             */
/***************************************************************/
#ifdef __TURBOC__
#pragma argsused
#endif
#ifdef HAVE_PROTOS
    PUBLIC int DoSatRemind(Trigger *trig, TimeTrig *tim, ParsePtr p)
#else
	int DoSatRemind(trig, tim, p)
	Trigger *trig;
    TimeTrig *tim;
    ParsePtr p;
#endif
    {
	int iter, jul, r;
	Value v;
	char *s, *t;

	t = p->pos;
	iter = 0;
	jul = trig->scanfrom;
	while (iter++ < MaxSatIter) {
	    jul = ComputeTrigger(jul, trig, &r);
	    if (r) {
		if (r == E_CANT_TRIG) return OK; else return r;
	    }
	    s = p->pos;
	    r = EvaluateExpr(p, &v);
	    t = p->pos;
	    if (r) return r;
	    if (v.type != INT_TYPE && v.type != STR_TYPE) return E_BAD_TYPE;
	    if (v.type == INT_TYPE && v.v.val) return OK;
	    if (v.type == STR_TYPE && *v.v.str) return OK;
	    p->pos = s;
	    jul++;
	}
	p->pos = t;
	LastTrigValid = 0;
	return OK;
    }

/***************************************************************/
/*                                                             */
/*  ParsePriority - parse the PRIORITY portion of a reminder   */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
    PRIVATE int ParsePriority(ParsePtr s, Trigger *t)
#else
	static int ParsePriority(s, t)
	ParsePtr s;
    Trigger *t;
#endif
    {
	int p, r;
	char *u;

	r = ParseToken(s, TokBuffer);
	if(r) return r;
	u = TokBuffer;

	if (!isdigit(*u)) return E_EXPECTING_NUMBER;
	p = 0;
	while (isdigit(*u)) {
	    p = p*10 + *u - '0';
	    u++;
	}
	if (*u) return E_EXPECTING_NUMBER;

/* Tricky!  The only way p can be < 0 is if overflow occurred; thus,
   E2HIGH is indeed the appropriate error message. */
	if (p<0 || p>9999) return E_2HIGH;
	t->priority = p;
	return OK;
    }

/***************************************************************/
/*                                                             */
/*  DoMsgCommand                                               */
/*                                                             */
/*  Execute the '-k' command, escaping shell chars in message. */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
    PUBLIC void DoMsgCommand(char *cmd, char *msg)
#else
	void DoMsgCommand(cmd, msg)
	char *cmd;
    char *msg;
#endif
    {

#ifdef WANT_SHELL_ESCAPING
	char buf[2*LINELEN+TOKSIZE];
	char *s, *t;
   
	/* Escape shell characters in msg INCLUDING WHITESPACE! */
	for (t=buf, s=msg; *s; s++) {
	    if (isspace(*s) || strchr(EscapeMe, *s)) *t++ = '\\';
	    *t++ = *s;
	}
	*t = 0;
   
	/* Use SubstBuffer -- not too safe, since no check for overflow... */
	sprintf(SubstBuffer, cmd, buf);
#else
	sprintf(SubstBuffer, cmd, msg);
#endif /* WANT_SHELL_ESCAPING */
	system(SubstBuffer);
    }

/***************************************************************/
/*                                                             */
/*  ShouldTriggerBasedOnWarn                                   */
/*                                                             */
/*  Determine whether to trigger a reminder based on its WARN  */
/*  function.                                                  */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int ShouldTriggerBasedOnWarn(Trigger *t, int jul)
#else
static int ShouldTriggerBasedOnWarn(t, jul)
Trigger *t;
int jul;
#endif
{
    char buffer[VAR_NAME_LEN+15];
    int i;
    char *s;
    int r;
    Value v;
    int lastReturnVal = 0; /* Silence compiler warning */

    /* If no proper function exists, barf... */
    if (UserFuncExists(t->warn) != 1) {
	Eprint("%s: `%s'", ErrMsg[M_BAD_WARN_FUNC], t->warn);
	return (jul == JulianToday);
    }
    for (i=1; ; i++) {
	sprintf(buffer, "%s(%d)", t->warn, i);
	s = buffer;
	r = EvalExpr(&s, &v);
	if (r) {
	    Eprint("%s: `%s': %s", ErrMsg[M_BAD_WARN_FUNC],
		   t->warn, ErrMsg[r]);
	    return (jul == JulianToday);
	}
	if (v.type != INT_TYPE) {
	    DestroyValue(v);
	    Eprint("%s: `%s': %s", ErrMsg[M_BAD_WARN_FUNC],
		   t->warn, ErrMsg[E_BAD_TYPE]);
	    return (jul == JulianToday);
	}

	/* If absolute value of return is not monotonically
           decreasing, exit */
	if (i > 1 && abs(v.v.val) >= lastReturnVal) {
	    return (jul == JulianToday);
	}

	lastReturnVal = abs(v.v.val);
	/* Positive values: Just subtract.  Negative values:
           skip omitted days. */
	if (v.v.val >= 0) {
	    if (JulianToday + v.v.val == jul) return 1;
	} else {
	    int j = jul;
	    while (v.v.val) {
		j--;
		if (!IsOmitted(j, t->localomit)) v.v.val++;
	    }
	    if (j == JulianToday) return 1;
	}
    }
}
