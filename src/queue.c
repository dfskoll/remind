/***************************************************************/
/*                                                             */
/*  QUEUE.C                                                    */
/*                                                             */
/*  Queue up reminders for subsequent execution.               */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1997 by David F. Skoll                  */
/*                                                             */
/***************************************************************/

#include "config.h"
static char const RCSID[] = "$Id: queue.c,v 1.2 1998-01-17 03:58:31 dfs Exp $";

/* We only want object code generated if we have queued reminders */
#ifdef HAVE_QUEUED
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined(__OS2__) || defined(__MSDOS__)
#include <io.h>
#if defined(__BORLANDC__)
#include <dos.h>
#endif
#include <process.h>
#endif

#include "globals.h"
#include "err.h"
#include "types.h"
#include "protos.h"
#include "expr.h"

/* List structure for holding queued reminders */
typedef struct queuedrem {
    struct queuedrem *next;
    int typ;
    int RunDisabled;
    int ntrig;
    char *text;
    char sched[VAR_NAME_LEN+1];
    TimeTrig tt;
} QueuedRem;

/* Global variables */

static QueuedRem *QueueHead;
static time_t FileModTime;
static struct stat StatBuf;

PRIVATE void CheckInitialFile ARGS ((void));
PRIVATE int CalculateNextTime ARGS ((QueuedRem *q));
PRIVATE QueuedRem *FindNextReminder ARGS ((void));
PRIVATE int CalculateNextTimeUsingSched ARGS ((QueuedRem *q));

/***************************************************************/
/*                                                             */
/*  QueueReminder                                              */
/*                                                             */
/*  Put the reminder on a queue for later, if queueing is      */
/*  enabled.                                                   */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int QueueReminder(ParsePtr p, int typ, TimeTrig *tim, const char *sched)
#else
int QueueReminder(p, typ, tim, sched)
ParsePtr p;
int typ;
TimeTrig *tim;
char *sched;
#endif
{
    QueuedRem *qelem;

    if (DontQueue ||
	tim->ttime == NO_TIME ||
	typ == CAL_TYPE ||
	tim->ttime < SystemTime(0) / 60 ||
	((typ == RUN_TYPE) && RunDisabled)) return OK;

    qelem = NEW(QueuedRem);
    if (!qelem) {
	return E_NO_MEM;
    }
    qelem->text = StrDup(p->pos);  /* Guaranteed that parser is not nested. */
    if (!qelem->text) {
	free(qelem);
	return E_NO_MEM;
    }
    qelem->typ = typ;
    qelem->tt = *tim;
    qelem->next = QueueHead;
    qelem->RunDisabled = RunDisabled;
    qelem->ntrig = 0;
    strcpy(qelem->sched, sched);
    QueueHead = qelem;
    NumQueued++;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  HandleQueuedReminders                                      */
/*                                                             */
/*  Handle the issuing of queued reminders in the background   */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC void HandleQueuedReminders(void)
#else
void HandleQueuedReminders()
#endif
{
    QueuedRem *q = QueueHead;
    long TimeToSleep;
    unsigned SleepTime;
    Parser p;
    Trigger trig;

    /* Suppress the BANNER from being issued */
    NumTriggered = 1;

    /* Turn off sorting -- otherwise, TriggerReminder has no effect! */
    SortByDate = 0;

    /* If we are not connected to a tty, then we must close the
     * standard file descriptors. This is to prevent someone
     * doing:
     *		remind file | <filter> | >log
     * and have <filter> hung because the child (us) is still
     * connected to it. This means the only commands that will be
     * processed correctly are RUN commands, provided they mail
     * the result back or use their own resource (as a window).
     */
    if (!DontFork && (!isatty(1) || !isatty(2))) {
	close(1);
	close(2);
    }

    /* If we're a daemon, get the mod time of initial file */
    if (Daemon) {
	if (stat(InitialFile, &StatBuf)) {
	    fprintf(ErrFp, "Cannot stat %s - not running as daemon!\n",
		    InitialFile);
	    Daemon = 0;
	} else FileModTime = StatBuf.st_mtime;
    }
   
    /* Initialize the queue - initialize all the entries time of issue */
   
    while (q) {
	q->tt.nexttime = (int) (SystemTime(0)/60 - 1);
	q->tt.nexttime = CalculateNextTime(q);
	q = q->next;
    }

#ifdef __BORLANDC__
    signal(SIGINT, SigIntHandler);
#else
    if (!DontFork || Daemon) signal(SIGINT, SigIntHandler);
#endif

    /* Sit in a loop, issuing reminders when necessary */
    while(1) {
	q = FindNextReminder();

	/* If no more reminders to issue, we're done unless we're a daemon. */
	if (!q && !Daemon) break;

	if (Daemon && !q)
	    TimeToSleep = (long) 60*Daemon;
	else
	    TimeToSleep = (long) q->tt.nexttime * 60L - SystemTime(0);

	while (TimeToSleep > 0L) {
	    SleepTime = (unsigned) ((TimeToSleep > 30000L) ? 30000 : TimeToSleep);

	    if (Daemon && SleepTime > 60*Daemon) SleepTime = 60*Daemon;

	    sleep(SleepTime);

	    if (Daemon && SleepTime) CheckInitialFile();

	    if (Daemon && !q)
		TimeToSleep = (long) 60*Daemon;
	    else
		TimeToSleep = (long) q->tt.nexttime * 60L - SystemTime(0);
	}

	/* Trigger the reminder */
	CreateParser(q->text, &p);
	trig.typ = q->typ;
	RunDisabled = q->RunDisabled;
#ifdef OS2_POPUP
	(void) TriggerReminder(&p, &trig, &q->tt, JulianToday, 1);
#else
	(void) TriggerReminder(&p, &trig, &q->tt, JulianToday);
#endif
	fflush(stdout);
      
	/* Calculate the next trigger time */
	q->tt.nexttime = CalculateNextTime(q);
    }
#ifdef __BORLANDC__
    signal(SIGINT, SIG_DFL);
#endif
    exit(0);
}
   

/***************************************************************/
/*                                                             */
/*  CalculateNextTime                                          */
/*                                                             */
/*  Calculate the next time when a reminder should be issued.  */
/*  Return NO_TIME if reminder expired.                        */
/*  Strategy is:  If a sched() function is defined, call it.   */
/*  Otherwise, use AT time with delta and rep.  If sched()     */
/*  fails, revert to AT with delta and rep.                    */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int CalculateNextTime(QueuedRem *q)
#else
static int CalculateNextTime(q)
QueuedRem *q;
#endif
{
    int tim = q->tt.ttime;
    int rep = q->tt.rep;
    int delta = q->tt.delta;
    int curtime = q->tt.nexttime+1;
    int r;

/* Increment number of times this one has been triggered */
    q->ntrig++;
    if (q->sched[0]) {
	r = CalculateNextTimeUsingSched(q);
	if (r != NO_TIME) return r;
    }
    if (delta == NO_DELTA)
	if (tim < curtime) return NO_TIME; else return tim;

    tim -= delta;
    if (rep == NO_REP) rep = delta;
    if (tim < curtime) tim += ((curtime - tim) / rep) * rep;
    if (tim < curtime) tim += rep;
    if (tim > q->tt.ttime) tim = q->tt.ttime;
    if (tim < curtime) return NO_TIME; else return tim;
}

/***************************************************************/
/*                                                             */
/*  FindNextReminder                                           */
/*                                                             */
/*  Find the next reminder to trigger                          */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE QueuedRem *FindNextReminder(void)
#else
static QueuedRem *FindNextReminder()
#endif
{
    QueuedRem *q = QueueHead;
    QueuedRem *ans = NULL;

    while (q) {
	if (q->tt.nexttime != NO_TIME) {
	    if (!ans) ans = q;
	    else if (q->tt.nexttime < ans->tt.nexttime) ans = q;
	}
      
	q = q->next;
    }
    return ans;
}
   

/***************************************************************/
/*                                                             */
/* GotSigInt						       */
/*                                                             */
/* Split out what's done on a SIGINT from the SIGINT Handler.  */
/* This will be necessary for OS/2 multithreaded.	       */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
void GotSigInt(void)
#else
void GotSigInt()
#endif
{
    QueuedRem *q = QueueHead;

    printf("Contents of AT queue:%s", NL);

    while (q) {
	if (q->tt.nexttime != NO_TIME) {
	    printf("Trigger: %02d%c%02d  Activate: %02d%c%02d  Rep: %d  Delta: %d  Sched: %s",
		   q->tt.ttime / 60, TIMESEP, q->tt.ttime % 60,
		   q->tt.nexttime / 60, TIMESEP, q->tt.nexttime % 60,
		   q->tt.rep, q->tt.delta, q->sched);
	    if (*q->sched) printf("(%d)", q->ntrig+1);
	    printf("%s", NL);
	    printf("Text: %s %s%s%s", ((q->typ == MSG_TYPE) ? "MSG" :
				       ((q->typ == MSF_TYPE) ? "MSF" :"RUN")),
		   q->text,
		   NL, NL);
	}
	q = q->next;
    }
    printf(NL);
}

/***************************************************************/
/*                                                             */
/*  CheckInitialFile                                           */
/*                                                             */
/*  If the initial file has been modified, then restart the    */
/*  daemon.                                                    */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE void CheckInitialFile(void)
#else
static void CheckInitialFile()
#endif
{
    /* If date has rolled around, or file has changed, spawn a new version. */
    time_t tim = FileModTime;
    int y, m, d;

    if (stat(InitialFile, &StatBuf) == 0) tim = StatBuf.st_mtime;
    if (tim != FileModTime ||
	RealToday != SystemDate(&y, &m, &d))
	execvp(ArgV[0], ArgV);
}

/***************************************************************/
/*                                                             */
/*  CalculateNextTimeUsingSched                                */
/*                                                             */
/*  Call the scheduling function.                              */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE int CalculateNextTimeUsingSched(QueuedRem *q)
#else
static int CalculateNextTimeUsingSched(q)
QueuedRem *q;
#endif
{
    /* Use LineBuffer for temp. string storage. */
    int r;
    Value v;
    char *s;
    int LastTime = -1;
    int ThisTime;

    if (UserFuncExists(q->sched) != 1) {
	q->sched[0] = 0;
	return NO_TIME;
    }

    RunDisabled = q->RunDisabled;  /* Don't want weird scheduling functions
				     to be a security hole!                */
    while(1) {
	sprintf(LineBuffer, "%s(%d)", q->sched, q->ntrig);
	s = LineBuffer;
	r = EvalExpr(&s, &v);
	if (r) {
	    q->sched[0] = 0;
	    return NO_TIME;
	}
	if (v.type == TIM_TYPE) {
	    ThisTime = v.v.val;
	} else if (v.type == INT_TYPE) {
	    if (v.v.val > 0) 
		ThisTime = q->tt.nexttime + v.v.val;
	    else
		ThisTime = q->tt.ttime + v.v.val;

	} else {
	    DestroyValue(v);
	    q->sched[0] = 0;
	    return NO_TIME;
	}
	if (ThisTime < 0) ThisTime = 0;        /* Can't be less than 00:00 */
	if (ThisTime > 1439) ThisTime = 1439;  /* or greater than 11:59 */
	if (ThisTime > q->tt.nexttime) return ThisTime;
	if (ThisTime <= LastTime) {
	    q->sched[0] = 0;
	    return NO_TIME;
	}
	LastTime = ThisTime;
	q->ntrig++;
    }
}

#endif /* HAVE_QUEUED from way at the top */
