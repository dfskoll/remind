/***************************************************************/
/*                                                             */
/*  DOREM.C                                                    */
/*                                                             */
/*  Contains routines for parsing reminders and evaluating     */
/*  triggers.  Also contains routines for parsing OMIT         */
/*  commands.                                                  */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by Dianne Skoll                    */
/*  Copyright (C) 1999-2000 by Roaring Penguin Software Inc.   */
/*                                                             */
/***************************************************************/

#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <stdlib.h>

#include "globals.h"
#include "err.h"
#include "types.h"
#include "protos.h"
#include "expr.h"

/* Define the shell characters not to escape */
static char const DontEscapeMe[] =
"1234567890_-=+abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@.,";

static int ParseTimeTrig (ParsePtr s, TimeTrig *tim, int save_in_globals);
static int ParseLocalOmit (ParsePtr s, Trigger *t);
static int ParseScanFrom (ParsePtr s, Trigger *t, int type);
static int ParsePriority (ParsePtr s, Trigger *t);
static int ParseUntil (ParsePtr s, Trigger *t);
static int ShouldTriggerBasedOnWarn (Trigger *t, int jul, int *err);

/***************************************************************/
/*                                                             */
/*  DoRem                                                      */
/*                                                             */
/*  Do the REM command.                                        */
/*                                                             */
/***************************************************************/
int DoRem(ParsePtr p)
{

    Trigger trig;
    TimeTrig tim;
    int r, err;
    int jul;
    DynamicBuffer buf;
    Token tok;

    DBufInit(&buf);

    /* Parse the trigger date and time */
    if ( (r=ParseRem(p, &trig, &tim, 1)) ) {
	FreeTrig(&trig);
	return r;
    }

    if (trig.typ == NO_TYPE) {
	PurgeEchoLine("%s\n%s\n", "#!P! Cannot parse next line", CurLine);
	FreeTrig(&trig);
	return E_EOLN;
    }
    if (trig.typ == SAT_TYPE) {
	PurgeEchoLine("%s\n", "#!P: Cannot purge SATISFY-type reminders");
	PurgeEchoLine("%s\n", CurLine);
	r=DoSatRemind(&trig, &tim, p);
	if (r) {
	    FreeTrig(&trig);
	    if (r == E_EXPIRED) return OK;
	    return r;
	}
	if (!LastTrigValid) {
	    FreeTrig(&trig);
	    return OK;
	}
	r=ParseToken(p, &buf);
	if (r) {
	    FreeTrig(&trig);
	    return r;
	}
	FindToken(DBufValue(&buf), &tok);
	DBufFree(&buf);
	if (tok.type == T_Empty || tok.type == T_Comment) {
	    DBufFree(&buf);
	    FreeTrig(&trig);
	    return OK;
	}
	if (tok.type != T_RemType || tok.val == SAT_TYPE) {
	    DBufFree(&buf);
	    FreeTrig(&trig);
	    return E_PARSE_ERR;
	}
	if (tok.val == PASSTHRU_TYPE) {
	    r=ParseToken(p, &buf);
	    if (r) {
		FreeTrig(&trig);
		return r;
	    }
	    if (!DBufLen(&buf)) {
		FreeTrig(&trig);
		DBufFree(&buf);
		return E_EOLN;
	    }
	    StrnCpy(trig.passthru, DBufValue(&buf), PASSTHRU_LEN);
	    DBufFree(&buf);
	}
	trig.typ = tok.val;
	jul = LastTriggerDate;
	if (!LastTrigValid || PurgeMode) {
	    FreeTrig(&trig);
	    return OK;
	}
    } else {
	/* Calculate the trigger date */
	jul = ComputeTrigger(trig.scanfrom, &trig, &r, 1);
	if (r) {
	    if (PurgeMode) {
		PurgeEchoLine("%s: %s\n", "#!P! Problem calculating trigger date", ErrMsg[r]);
		PurgeEchoLine("%s\n", CurLine);
	    }
	    FreeTrig(&trig);
	    return r;
	}
    }

    if (PurgeMode) {
	if (trig.expired || jul < JulianToday) {
	    if (p->expr_happened) {
		if (p->nonconst_expr) {
		    PurgeEchoLine("%s\n", "#!P: Next line may have expired, but contains non-constant expression");
		    PurgeEchoLine("%s\n", CurLine);
		} else {
		    PurgeEchoLine("%s\n", "#!P: Next line has expired, but contains expression...  please verify");
		    PurgeEchoLine("#!P: Expired: %s\n", CurLine);
		}
	    } else {
		PurgeEchoLine("#!P: Expired: %s\n", CurLine);
	    }
	} else {
	    PurgeEchoLine("%s\n", CurLine);
	}
	FreeTrig(&trig);
	return OK;
    }
/* Queue the reminder, if necessary */
    if (jul == JulianToday &&
	!(!IgnoreOnce &&
	  trig.once != NO_ONCE &&
	  FileAccessDate == JulianToday))
	QueueReminder(p, &trig, &tim, trig.sched);
/* If we're in daemon mode, do nothing over here */
    if (Daemon) {
	FreeTrig(&trig);
	return OK;
    }

    if (ShouldTriggerReminder(&trig, &tim, jul, &err)) {
	if ( (r=TriggerReminder(p, &trig, &tim, jul)) ) {
	    FreeTrig(&trig);
	    return r;
	}
    }

    FreeTrig(&trig);
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
int ParseRem(ParsePtr s, Trigger *trig, TimeTrig *tim, int save_in_globals)
{
    register int r;
    DynamicBuffer buf;
    Token tok;
    int y, m, d;

    DBufInit(&buf);

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
    trig->omitfunc[0] = 0;
    DBufInit(&(trig->tags));
    trig->passthru[0] = 0;
    tim->ttime = NO_TIME;
    tim->delta = NO_DELTA;
    tim->rep   = NO_REP;
    tim->duration = NO_TIME;
    if (save_in_globals) {
	LastTriggerTime = NO_TIME;
    }

    while(1) {
	/* Read space-delimited string */
	r = ParseToken(s, &buf);
	if (r) return r;

	/* Figure out what we've got */
	FindToken(DBufValue(&buf), &tok);
	switch(tok.type) {
	case T_Date:
	    DBufFree(&buf);
	    if (trig->d != NO_DAY) return E_DAY_TWICE;
	    if (trig->m != NO_MON) return E_MON_TWICE;
	    if (trig->y != NO_YR)  return E_YR_TWICE;
	    FromJulian(tok.val, &y, &m, &d);
	    trig->y = y;
	    trig->m = m;
	    trig->d = d;
	    break;

	case T_DateTime:
	    DBufFree(&buf);
	    if (trig->d != NO_DAY) return E_DAY_TWICE;
	    if (trig->m != NO_MON) return E_MON_TWICE;
	    if (trig->y != NO_YR)  return E_YR_TWICE;
	    FromJulian(tok.val / MINUTES_PER_DAY, &y, &m, &d);
	    trig->y = y;
	    trig->m = m;
	    trig->d = d;
	    tim->ttime = (tok.val % MINUTES_PER_DAY);
	    if (save_in_globals) {
		LastTriggerTime = tim->ttime;
	    }
	    break;

	case T_WkDay:
	    DBufFree(&buf);
	    if (trig->wd & (1 << tok.val)) return E_WD_TWICE;
	    trig->wd |= (1 << tok.val);
	    break;

	case T_Month:
	    DBufFree(&buf);
	    if (trig->m != NO_MON) return E_MON_TWICE;
	    trig->m = tok.val;
	    break;

	case T_Skip:
	    DBufFree(&buf);
	    if (trig->skip != NO_SKIP) return E_SKIP_ERR;
	    trig->skip = tok.val;
	    break;

	case T_Priority:
	    DBufFree(&buf);
	    r=ParsePriority(s, trig);
	    if (r) return r;
	    break;

	case T_At:
	    DBufFree(&buf);
	    r=ParseTimeTrig(s, tim, save_in_globals);
	    if (r) return r;
	    break;

	case T_Scanfrom:
	    DBufFree(&buf);
	    r=ParseScanFrom(s, trig, tok.val);
	    if (r) return r;
	    break;

	case T_RemType:
	    DBufFree(&buf);
	    trig->typ = tok.val;
	    if (s->isnested) return E_CANT_NEST_RTYPE;
	    if (trig->scanfrom == NO_DATE) trig->scanfrom = JulianToday;
	    if (trig->typ == PASSTHRU_TYPE) {
		r = ParseToken(s, &buf);
		if (r) return r;
		if (!DBufLen(&buf)) {
		    DBufFree(&buf);
		    return E_EOLN;
		}
		StrnCpy(trig->passthru, DBufValue(&buf), PASSTHRU_LEN);
	    }
	    return OK;

	case T_Through:
	    DBufFree(&buf);
	    if (trig->rep != NO_REP) return E_REP_TWICE;
	    trig->rep = 1;
	    r = ParseUntil(s, trig);
	    if (r) return r;
	    break;

	case T_Until:
	    DBufFree(&buf);
	    r=ParseUntil(s, trig);
	    if (r) return r;
	    break;

	case T_Year:
	    DBufFree(&buf);
	    if (trig->y != NO_YR) return E_YR_TWICE;
	    trig->y = tok.val;
	    break;

	case T_Day:
	    DBufFree(&buf);
	    if (trig->d != NO_DAY) return E_DAY_TWICE;
	    trig->d = tok.val;
	    break;

	case T_Rep:
	    DBufFree(&buf);
	    if (trig->rep != NO_REP) return E_REP_TWICE;
	    trig->rep = tok.val;
	    break;

	case T_Delta:
	    DBufFree(&buf);
	    if (trig->delta != NO_DELTA) return E_DELTA_TWICE;
	    trig->delta = tok.val;
	    break;

	case T_Back:
	    DBufFree(&buf);
	    if (trig->back != NO_BACK) return E_BACK_TWICE;
	    trig->back = tok.val;
	    break;

	case T_Once:
	    DBufFree(&buf);
	    if (trig->once != NO_ONCE) return E_ONCE_TWICE;
	    trig->once = ONCE_ONCE;
	    break;

	case T_Omit:
	    DBufFree(&buf);
	    if (trig->omitfunc[0]) {
		Eprint("Warning: OMIT is ignored if you use OMITFUNC");
	    }

	    r = ParseLocalOmit(s, trig);
	    if (r) return r;
	    break;

	case T_Empty:
	    DBufFree(&buf);
	    if (trig->scanfrom == NO_DATE) trig->scanfrom = JulianToday;
	    return OK;

	case T_OmitFunc:
	    if (trig->localomit) {
		Eprint("Warning: OMIT is ignored if you use OMITFUNC");
	    }
	    r=ParseToken(s, &buf);
	    if (r) return r;
	    StrnCpy(trig->omitfunc, DBufValue(&buf), VAR_NAME_LEN);
	    DBufFree(&buf);
	    break;

	case T_Warn:
	    r=ParseToken(s, &buf);
	    if(r) return r;
	    StrnCpy(trig->warn, DBufValue(&buf), VAR_NAME_LEN);
	    DBufFree(&buf);
	    break;

	case T_Tag:
	    r = ParseToken(s, &buf);
	    if (r) return r;
	    AppendTag(&(trig->tags), DBufValue(&buf));
	    break;

	case T_Duration:
	    r = ParseToken(s, &buf);
	    if (r) return r;
	    FindToken(DBufValue(&buf), &tok);
	    DBufFree(&buf);
	    switch(tok.type) {
	    case T_Time:
	    case T_LongTime:
		tim->duration = tok.val;
		break;
	    default:
		return E_BAD_TIME;
	    }
	    break;

	case T_Sched:
	    r=ParseToken(s, &buf);
	    if(r) return r;
	    StrnCpy(trig->sched, DBufValue(&buf), VAR_NAME_LEN);
	    DBufFree(&buf);
	    break;

	case T_LongTime:
	    DBufFree(&buf);
	    return E_BAD_TIME;
	    break;

	default:
	    PushToken(DBufValue(&buf), s);
	    DBufFree(&buf);
	    trig->typ = MSG_TYPE;
	    if (s->isnested) return E_CANT_NEST_RTYPE;
	    if (trig->scanfrom == NO_DATE) trig->scanfrom = JulianToday;
	    return OK;
	}
    }
}

/***************************************************************/
/*                                                             */
/*  ParseTimeTrig - parse the AT part of a timed reminder      */
/*                                                             */
/***************************************************************/
static int ParseTimeTrig(ParsePtr s, TimeTrig *tim, int save_in_globals)
{
    Token tok;
    int r;

    DynamicBuffer buf;
    DBufInit(&buf);

    while(1) {
	r = ParseToken(s, &buf);
	if (r) return r;
	FindToken(DBufValue(&buf), &tok);
	switch(tok.type) {
	case T_Time:
	    DBufFree(&buf);
	    if (tim->ttime != NO_TIME) return E_TIME_TWICE;
	    tim->ttime = tok.val;
	    break;

	case T_Delta:
	    DBufFree(&buf);
	    if (tim->delta != NO_DELTA) return E_DELTA_TWICE;
	    tim->delta = (tok.val > 0) ? tok.val : -tok.val;
	    break;

	case T_Rep:
	    DBufFree(&buf);
	    if (tim->rep != NO_REP) return E_REP_TWICE;
	    tim->rep = tok.val;
	    break;

	default:
	    if (tim->ttime == NO_TIME) return E_EXPECT_TIME;

	    /* Save trigger time in global variable */
	    if (save_in_globals) {
		LastTriggerTime = tim->ttime;
	    }
	    PushToken(DBufValue(&buf), s);
	    DBufFree(&buf);
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
static int ParseLocalOmit(ParsePtr s, Trigger *t)
{
    Token tok;
    int r;
    DynamicBuffer buf;
    DBufInit(&buf);

    while(1) {
	r = ParseToken(s, &buf);
	if (r) return r;
	FindToken(DBufValue(&buf), &tok);
	switch(tok.type) {
	case T_WkDay:
	    DBufFree(&buf);
	    t->localomit |= (1 << tok.val);
	    break;

	default:
	    PushToken(DBufValue(&buf), s);
	    DBufFree(&buf);
	    return OK;
	}
    }
}

/***************************************************************/
/*                                                             */
/*  ParseUntil - parse the UNTIL portion of a reminder         */
/*                                                             */
/***************************************************************/
static int ParseUntil(ParsePtr s, Trigger *t)
{
    int y = NO_YR,
	m = NO_MON,
	d = NO_DAY;

    Token tok;
    int r;
    DynamicBuffer buf;
    DBufInit(&buf);

    if (t->until != NO_UNTIL) return E_UNTIL_TWICE;

    while(1) {
	r = ParseToken(s, &buf);
	if (r) return r;
	FindToken(DBufValue(&buf), &tok);
	switch(tok.type) {
	case T_Year:
	    DBufFree(&buf);
	    if (y != NO_YR) {
		Eprint("UNTIL: %s", ErrMsg[E_YR_TWICE]);
		return E_YR_TWICE;
	    }
	    y = tok.val;
	    break;

	case T_Month:
	    DBufFree(&buf);
	    if (m != NO_MON) {
		Eprint("UNTIL: %s", ErrMsg[E_MON_TWICE]);
		return E_MON_TWICE;
	    }
	    m = tok.val;
	    break;

	case T_Day:
	    DBufFree(&buf);
	    if (d != NO_DAY) {
		Eprint("UNTIL: %s", ErrMsg[E_DAY_TWICE]);
		return E_DAY_TWICE;
	    }
	    d = tok.val;
	    break;

	case T_Date:
	    DBufFree(&buf);
	    if (y != NO_YR) {
		Eprint("UNTIL: %s", ErrMsg[E_YR_TWICE]);
		return E_YR_TWICE;
	    }
	    if (m != NO_MON) {
		Eprint("UNTIL: %s", ErrMsg[E_MON_TWICE]);
		return E_MON_TWICE;
	    }
	    if (d != NO_DAY) {
		Eprint("UNTIL: %s", ErrMsg[E_DAY_TWICE]);
		return E_DAY_TWICE;
	    }
	    FromJulian(tok.val, &y, &m, &d);
	    break;

	default:
	    if (y == NO_YR || m == NO_MON || d == NO_DAY) {
		Eprint("UNTIL: %s", ErrMsg[E_INCOMPLETE]);
		DBufFree(&buf);
		return E_INCOMPLETE;
	    }
	    if (!DateOK(y, m, d)) {
		DBufFree(&buf);
		return E_BAD_DATE;
	    }
	    t->until = Julian(y, m, d);
	    PushToken(DBufValue(&buf), s);
	    DBufFree(&buf);
	    return OK;
	}
    }
}

/***************************************************************/
/*                                                             */
/*  ParseScanFrom - parse the FROM/SCANFROM portion            */
/*                                                             */
/***************************************************************/
static int ParseScanFrom(ParsePtr s, Trigger *t, int type)
{
    int y = NO_YR,
	m = NO_MON,
	d = NO_DAY;

    Token tok;
    int r;
    DynamicBuffer buf;
    char const *word;

    DBufInit(&buf);
    if (type == SCANFROM_TYPE) {
	word = "SCANFROM";
    } else {
	word = "FROM";
    }

    if (t->scanfrom != NO_DATE) return E_SCAN_TWICE;

    while(1) {
	r = ParseToken(s, &buf);
	if (r) return r;
	FindToken(DBufValue(&buf), &tok);
	switch(tok.type) {
	case T_Year:
	    DBufFree(&buf);
	    if (y != NO_YR) {
		Eprint("%s: %s", word, ErrMsg[E_YR_TWICE]);
		return E_YR_TWICE;
	    }
	    y = tok.val;
	    break;

	case T_Month:
	    DBufFree(&buf);
	    if (m != NO_MON) {
		Eprint("%s: %s", word, ErrMsg[E_MON_TWICE]);
		return E_MON_TWICE;
	    }
	    m = tok.val;
	    break;

	case T_Day:
	    DBufFree(&buf);
	    if (d != NO_DAY) {
		Eprint("%s: %s", word, ErrMsg[E_DAY_TWICE]);
		return E_DAY_TWICE;
	    }
	    d = tok.val;
	    break;

	case T_Date:
	    DBufFree(&buf);
	    if (y != NO_YR) {
		Eprint("%s: %s", word, ErrMsg[E_YR_TWICE]);
		return E_YR_TWICE;
	    }
	    if (m != NO_MON) {
		Eprint("%s: %s", word, ErrMsg[E_MON_TWICE]);
		return E_MON_TWICE;
	    }
	    if (d != NO_DAY) {
		Eprint("%s: %s", word, ErrMsg[E_DAY_TWICE]);
		return E_DAY_TWICE;
	    }
	    FromJulian(tok.val, &y, &m, &d);
	    break;

	default:
	    if (y == NO_YR || m == NO_MON || d == NO_DAY) {
		Eprint("%s: %s", word, ErrMsg[E_INCOMPLETE]);
		DBufFree(&buf);
		return E_INCOMPLETE;
	    }
	    if (!DateOK(y, m, d)) {
		DBufFree(&buf);
		return E_BAD_DATE;
	    }
	    t->scanfrom = Julian(y, m, d);
	    if (type == FROM_TYPE) {
		if (t->scanfrom < JulianToday) {
		    t->scanfrom = JulianToday;
		}
	    }

	    PushToken(DBufValue(&buf), s);
	    DBufFree(&buf);
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
int TriggerReminder(ParsePtr p, Trigger *t, TimeTrig *tim, int jul)
{
    int r, y, m, d;
    char PrioExpr[VAR_NAME_LEN+25];
    char tmpBuf[64];
    DynamicBuffer buf, calRow;
    DynamicBuffer pre_buf;
    char const *s;
    Value v;

    DBufInit(&buf);
    DBufInit(&calRow);
    DBufInit(&pre_buf);
    if (t->typ == RUN_TYPE && RunDisabled) return E_RUN_DISABLED;
    if ((t->typ == PASSTHRU_TYPE && strcmp(t->passthru, "COLOR") && strcmp(t->passthru, "COLOUR")) ||
	t->typ == CAL_TYPE ||
	t->typ == PS_TYPE ||
	t->typ == PSF_TYPE)
	return OK;

    /* Handle COLOR types */
    if (t->typ == PASSTHRU_TYPE && (!strcmp(t->passthru, "COLOR") || !strcmp(t->passthru, "COLOUR"))) {
	/* Strip off three tokens */
	r = ParseToken(p, &buf);
	if (!NextMode) {
	    DBufPuts(&pre_buf, DBufValue(&buf));
	    DBufPutc(&pre_buf, ' ');
	}
	DBufFree(&buf);
	if (r) return r;
	r = ParseToken(p, &buf);
	if (!NextMode) {
	    DBufPuts(&pre_buf, DBufValue(&buf));
	    DBufPutc(&pre_buf, ' ');
	}
	DBufFree(&buf);
	if (r) return r;
	r = ParseToken(p, &buf);
	if (!NextMode) {
	    DBufPuts(&pre_buf, DBufValue(&buf));
	    DBufPutc(&pre_buf, ' ');
	}
	DBufFree(&buf);
	if (r) return r;
	t->typ = MSG_TYPE;
    }
/* If it's a MSG-type reminder, and no -k option was used, issue the banner. */
    if ((t->typ == MSG_TYPE || t->typ == MSF_TYPE) 
	&& !NumTriggered && !NextMode && !MsgCommand) {
	if (!DoSubstFromString(DBufValue(&Banner), &buf,
			       JulianToday, NO_TIME) &&
	    DBufLen(&buf)) {
	printf("%s\n", DBufValue(&buf));
	}
	DBufFree(&buf);
    }

/* If it's NextMode, process as a ADVANCE_MODE-type entry, and issue
   simple-calendar format. */
    if (NextMode) {
	if ( (r=DoSubst(p, &buf, t, tim, jul, ADVANCE_MODE)) ) return r;
	if (!DBufLen(&buf)) {
	    DBufFree(&buf);
	    DBufFree(&pre_buf);
	    return OK;
	}
	FromJulian(jul, &y, &m, &d);
 	sprintf(tmpBuf, "%04d/%02d/%02d ", y, m+1, d);
 	if (DBufPuts(&calRow, tmpBuf) != OK) {
 	    DBufFree(&calRow);
	    DBufFree(&pre_buf);
 	    return E_NO_MEM;
 	}
 	/* If DoSimpleCalendar==1, output *all* simple calendar fields */
 	if (DoSimpleCalendar) {
 	    /* ignore passthru field when in NextMode */
 	    if (DBufPuts(&calRow, "* ") != OK) {
 		DBufFree(&calRow);
		DBufFree(&pre_buf);
 		return E_NO_MEM;
 	    }
	    if (*DBufValue(&(t->tags))) {
		DBufPuts(&calRow, DBufValue(&(t->tags)));
		DBufPutc(&calRow, ' ');
	    } else {
		DBufPuts(&calRow, "* ");
	    }
 	    if (tim->duration != NO_TIME) {
 		sprintf(tmpBuf, "%d ", tim->duration);
 	    } else {
 		sprintf(tmpBuf, "* ");
 	    }
 	    if (DBufPuts(&calRow, tmpBuf) != OK) {
 		DBufFree(&calRow);
		DBufFree(&pre_buf);
 		return E_NO_MEM;
 	    }
 	    if (tim->ttime != NO_TIME) {
 		sprintf(tmpBuf, "%d ", tim->ttime);
 	    } else {
 		sprintf(tmpBuf, "* ");
 	    }
 	    if (DBufPuts(&calRow, tmpBuf) != OK) {
 		DBufFree(&calRow);
		DBufFree(&pre_buf);
 		return E_NO_MEM;
 	    }
 	}
 	if (DBufPuts(&calRow, SimpleTime(tim->ttime)) != OK) {
 	    DBufFree(&calRow);
	    DBufFree(&pre_buf);
 	    return E_NO_MEM;
 	}

 	printf("%s%s%s\n", DBufValue(&calRow), DBufValue(&pre_buf), DBufValue(&buf));
	DBufFree(&buf);
	DBufFree(&pre_buf);
	DBufFree(&calRow);
	return OK;
    }

    /* Put the substituted string into the substitution buffer */

    /* Don't use msgprefix() on RUN-type reminders */
    if (t->typ != RUN_TYPE) {
	if (UserFuncExists("msgprefix") == 1) {
	    sprintf(PrioExpr, "msgprefix(%d)", t->priority);
	    s = PrioExpr;
	    r = EvalExpr(&s, &v, NULL);
	    if (!r) {
		if (!DoCoerce(STR_TYPE, &v)) {
		    if (DBufPuts(&buf, v.v.str) != OK) {
			DBufFree(&buf);
			DestroyValue(v);
			return E_NO_MEM;
		    }
		}
		DestroyValue(v);
	    }
	}
    }

    if ( (r=DoSubst(p, &buf, t, tim, jul, NORMAL_MODE)) ) return r;
    if (t->typ != RUN_TYPE) {
	if (UserFuncExists("msgsuffix") == 1) {
	    sprintf(PrioExpr, "msgsuffix(%d)", t->priority);
	    s = PrioExpr;
	    r = EvalExpr(&s, &v, NULL);
	    if (!r) {
		if (!DoCoerce(STR_TYPE, &v)) {
		    if (DBufPuts(&buf, v.v.str) != OK) {
			DBufFree(&buf);
			DestroyValue(v);
			return E_NO_MEM;
		    }
		}
		DestroyValue(v);
	    }
	}
    }

    if ((!MsgCommand && t->typ == MSG_TYPE) || t->typ == MSF_TYPE) {
	if (DBufPutc(&buf, '\n') != OK) {
	    DBufFree(&buf);
	    return E_NO_MEM;
	}
    }

/* If we are sorting, just queue it up in the sort buffer */
    if (SortByDate) {
	if (InsertIntoSortBuffer(jul, tim->ttime, DBufValue(&buf),
				 t->typ, t->priority) == OK) {
	    DBufFree(&buf);
	    NumTriggered++;
	    return OK;
	}
    }

/* If we didn't insert the reminder into the sort buffer, issue the
   reminder now. */
    switch(t->typ) {
    case MSG_TYPE:
    case PASSTHRU_TYPE:
	if (MsgCommand) {
	    DoMsgCommand(MsgCommand, DBufValue(&buf));
	} else {
	    printf("%s", DBufValue(&buf));
	}
	break;

    case MSF_TYPE:
	FillParagraph(DBufValue(&buf));
	break;

    case RUN_TYPE:
	system(DBufValue(&buf));
	break;

    default: /* Unknown/illegal type? */
	DBufFree(&buf);
	return E_SWERR;
    }

    DBufFree(&buf);
    NumTriggered++;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  ShouldTriggerReminder                                      */
/*                                                             */
/*  Return 1 if we should trigger a reminder, based on today's */
/*  date and the trigger.  Return 0 if reminder should not be  */
/*  triggered.  Sets *err non-zero in event of an error.       */
/*                                                             */
/***************************************************************/
int ShouldTriggerReminder(Trigger *t, TimeTrig *tim, int jul, int *err)
{
    int r, omit;
    *err = 0;

    /* Handle the ONCE modifier in the reminder. */
    if (!IgnoreOnce && t->once !=NO_ONCE && FileAccessDate == JulianToday)
	return 0;

    if (jul < JulianToday) return 0;

    /* Don't trigger timed reminders if DontIssueAts is true, and if the
       reminder is for today */
    if (jul == JulianToday && DontIssueAts && tim->ttime != NO_TIME) {
	if (DontIssueAts > 1) {
	    /* If two or more -a options, then *DO* issue ats that are in the
	       future */
	    if (tim->ttime < SystemTime(0) / 60) {
		return 0;
	    }
	} else {
	    return 0;
	}
    }

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
	if (DeltaOffset) {
	    if (jul <= JulianToday + DeltaOffset) {
		return 1;
	    }
	}
	return ShouldTriggerBasedOnWarn(t, jul, err);
    }

    /* Move back by delta days, if any */
    if (t->delta != NO_DELTA) {
	if (t->delta < 0)
	    jul = jul + t->delta;
	else {
	    int iter = 0;
	    int max = MaxSatIter;
	    r = t->delta;
	    if (max < r*2) max = r*2;
	    while(iter++ < max) {
		if (!r || (jul <= JulianToday)) {
		    break;
		}
		jul--;
		*err = IsOmitted(jul, t->localomit, t->omitfunc, &omit);
		if (*err) return 0;
		if (!omit) r--;
	    }
	    if (iter > max) {
		*err = E_CANT_TRIG;
	        Eprint("Delta: Bad OMITFUNC? %s", ErrMsg[E_CANT_TRIG]);
		return 0;
	    }
	}
    }

    /* Should we trigger the reminder? */
    return (jul <= JulianToday + DeltaOffset);
}

/***************************************************************/
/*                                                             */
/*  DoSatRemind                                                */
/*                                                             */
/*  Do the "satisfying..." remind calculation.                 */
/*                                                             */
/***************************************************************/
int DoSatRemind(Trigger *trig, TimeTrig *tim, ParsePtr p)
{
    int iter, jul, r;
    Value v;
    char const *s;
    char const *t;

    t = p->pos;
    iter = 0;
    jul = trig->scanfrom;
    while (iter++ < MaxSatIter) {
	jul = ComputeTrigger(jul, trig, &r, 1);
	if (r) {
	    if (r == E_CANT_TRIG) return OK; else return r;
	}
	if (jul == -1) {
	    return E_EXPIRED;
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
static int ParsePriority(ParsePtr s, Trigger *t)
{
    int p, r;
    char const *u;
    DynamicBuffer buf;
    DBufInit(&buf);

    r = ParseToken(s, &buf);
    if(r) return r;
    u = DBufValue(&buf);

    if (!isdigit(*u)) {
	DBufFree(&buf);
	return E_EXPECTING_NUMBER;
    }
    p = 0;
    while (isdigit(*u)) {
	p = p*10 + *u - '0';
	u++;
    }
    if (*u) {
	DBufFree(&buf);
	return E_EXPECTING_NUMBER;
    }

    DBufFree(&buf);

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
int DoMsgCommand(char const *cmd, char const *msg)
{
    int r;
    int i, l;
    DynamicBuffer execBuffer;

    DynamicBuffer buf;
    char const *s;

    DBufInit(&buf);
    DBufInit(&execBuffer);

    /* Escape shell characters in msg INCLUDING WHITESPACE! */
    for (s=msg; *s; s++) {
	if (isspace(*s) || !strchr(DontEscapeMe, *s)) {
	    if (DBufPutc(&buf, '\\') != OK) {
		r = E_NO_MEM;
		goto finished;
	    }
	}
	if (DBufPutc(&buf, *s) != OK) {
	    r = E_NO_MEM;
	    goto finished;
	}
    }
    msg = DBufValue(&buf);

    /* Do "%s" substitution */
    l = strlen(cmd);
    for (i=0; i<l; i++) {
	if (cmd[i] == '%' && cmd[i+1] == 's') {
	    ++i;
	    if (DBufPuts(&execBuffer, msg) != OK) {
		r = E_NO_MEM;
		goto finished;
	    }
	} else {
	    if (DBufPutc(&execBuffer, cmd[i]) != OK) {
		r = E_NO_MEM;
		goto finished;
	    }
	}
    }
    r = OK;

    system(DBufValue(&execBuffer));

finished:
    DBufFree(&buf);
    DBufFree(&execBuffer);
    return r;
}

/***************************************************************/
/*                                                             */
/*  ShouldTriggerBasedOnWarn                                   */
/*                                                             */
/*  Determine whether to trigger a reminder based on its WARN  */
/*  function.                                                  */
/*                                                             */
/***************************************************************/
static int ShouldTriggerBasedOnWarn(Trigger *t, int jul, int *err)
{
    char buffer[VAR_NAME_LEN+32];
    int i;
    char const *s;
    int r, omit;
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
	r = EvalExpr(&s, &v, NULL);
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
	    int iter = 0;
	    int max = MaxSatIter;
	    if (max < v.v.val * 2) max = v.v.val*2;
	    while(iter++ <= max) {
		j--;
		*err = IsOmitted(j, t->localomit, t->omitfunc, &omit);
		if (*err) return 0;
		if (!omit) v.v.val++;
		if (!v.v.val) {
		    break;
		}
	    }
	    if (iter > max) {
	        Eprint("Delta: Bad OMITFUNC? %s", ErrMsg[E_CANT_TRIG]);
	        return 0;
	    }
	    if (j == JulianToday) return 1;
	}
    }
}
