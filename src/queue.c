/***************************************************************/
/*                                                             */
/*  QUEUE.C                                                    */
/*                                                             */
/*  Queue up reminders for subsequent execution.               */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-2022 by Dianne Skoll                    */
/*                                                             */
/***************************************************************/

#include "config.h"

/* Solaris needs this to get select() prototype */
#ifdef __sun__
#define __EXTENSIONS__ 1
#endif

/* We only want object code generated if we have queued reminders */
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/select.h>
#include <stdlib.h>
#include <unistd.h>

#include "types.h"
#include "globals.h"
#include "err.h"
#include "protos.h"
#include "expr.h"

/* List structure for holding queued reminders */
typedef struct queuedrem {
    struct queuedrem *next;
    int typ;
    int RunDisabled;
    int ntrig;
    char const *text;
    char passthru[PASSTHRU_LEN+1];
    char sched[VAR_NAME_LEN+1];
    DynamicBuffer tags;
    Trigger t;
    TimeTrig tt;
} QueuedRem;

/* Global variables */

static QueuedRem *QueueHead;
static time_t FileModTime;
static struct stat StatBuf;

static void CheckInitialFile (void);
static int CalculateNextTime (QueuedRem *q);
static QueuedRem *FindNextReminder (void);
static int CalculateNextTimeUsingSched (QueuedRem *q);
static void DaemonWait (struct timeval *sleep_tv);
static void reread (void);

/***************************************************************/
/*                                                             */
/*  QueueReminder                                              */
/*                                                             */
/*  Put the reminder on a queue for later, if queueing is      */
/*  enabled.                                                   */
/*                                                             */
/***************************************************************/
int QueueReminder(ParsePtr p, Trigger *trig,
			 TimeTrig *tim, char const *sched)
{
    QueuedRem *qelem;

    if (DontQueue ||
	tim->ttime == NO_TIME ||
	trig->typ == CAL_TYPE ||
	tim->ttime < SystemTime(0) / 60 ||
	((trig->typ == RUN_TYPE) && RunDisabled)) return OK;

    qelem = NEW(QueuedRem);
    if (!qelem) {
	return E_NO_MEM;
    }
    qelem->text = StrDup(p->pos);  /* Guaranteed that parser is not nested. */
    if (!qelem->text) {
	free(qelem);
	return E_NO_MEM;
    }
    NumQueued++;
    qelem->typ = trig->typ;
    strcpy(qelem->passthru, trig->passthru);
    qelem->tt = *tim;
    qelem->t = *trig;
    DBufInit(&(qelem->t.tags));
    qelem->next = QueueHead;
    qelem->RunDisabled = RunDisabled;
    qelem->ntrig = 0;
    strcpy(qelem->sched, sched);
    DBufInit(&(qelem->tags));
    DBufPuts(&(qelem->tags), DBufValue(&(trig->tags)));
    if (SynthesizeTags) {
	AppendTag(&(qelem->tags), SynthesizeTag());
    }
    QueueHead = qelem;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  HandleQueuedReminders                                      */
/*                                                             */
/*  Handle the issuing of queued reminders in the background   */
/*                                                             */
/***************************************************************/
void HandleQueuedReminders(void)
{
    QueuedRem *q = QueueHead;
    int TimeToSleep;
    unsigned SleepTime;
    Parser p;
    Trigger trig;
    struct timeval tv;
    struct timeval sleep_tv;

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
    if (Daemon > 0) {
	if (stat(InitialFile, &StatBuf)) {
	    fprintf(ErrFp, "Cannot stat %s - not running as daemon!\n",
		    InitialFile);
	    Daemon = 0;
	} else FileModTime = StatBuf.st_mtime;
    }

    /* Initialize the queue - initialize all the entries time of issue */

    while (q) {
	q->tt.nexttime = (int) (SystemTime(1)/60 - 1);
	q->tt.nexttime = CalculateNextTime(q);
	q = q->next;
    }

    if (!DontFork || Daemon) signal(SIGINT, SigIntHandler);

    /* Sit in a loop, issuing reminders when necessary */
    while(1) {
	q = FindNextReminder();

	/* If no more reminders to issue, we're done unless we're a daemon. */
	if (!q && !Daemon) break;

	if (Daemon && !q) {
	    if (Daemon < 0) {
		/* Sleep until midnight */
		TimeToSleep = MINUTES_PER_DAY*60 - SystemTime(1);
	    } else {
		TimeToSleep = 60*Daemon;
	    }
	} else {
	    TimeToSleep = q->tt.nexttime * 60L - SystemTime(1);
	}

	while (TimeToSleep > 0L) {
	    SleepTime = TimeToSleep;

	    if (Daemon > 0 && SleepTime > (unsigned int) 60*Daemon) {
                SleepTime = 60*Daemon;
            }

	    /* Wake up once a minute to recalibrate sleep time in
	       case of laptop hibernation */
	    if (Daemon < 0) {
		/* Wake up on the next exact minute */
                gettimeofday(&tv, NULL);
                sleep_tv.tv_sec = 60 - (tv.tv_sec % 60);
                if (tv.tv_usec != 0 && sleep_tv.tv_sec != 0) {
                    sleep_tv.tv_sec--;
                    sleep_tv.tv_usec = 1000000 - tv.tv_usec;
                } else {
                    sleep_tv.tv_usec = 0;
                }
		DaemonWait(&sleep_tv);
	    } else {
		sleep(SleepTime);
            }

	    /* If not in daemon mode and day has rolled around,
	       exit -- not much we can do. */
	    if (!Daemon) {
		int y, m, d;
		if (RealToday != SystemDate(&y, &m, &d)) {
			exit(0);
		}
	    }

	    if (Daemon > 0 && SleepTime) CheckInitialFile();

	    if (Daemon && !q) {
		if (Daemon < 0) {
		    /* Sleep until midnight */
		    TimeToSleep = MINUTES_PER_DAY*60 - SystemTime(1);
		} else {
		    TimeToSleep = 60*Daemon;
		}
	    } else {
		TimeToSleep = q->tt.nexttime * 60L - SystemTime(1);
	    }

	}

	/* Do NOT trigger the reminder if tt.nexttime is more than a
	   minute in the past.  This can happen if the clock is
	   changed or a laptop awakes from hibernation.
	   However, DO trigger if tt.nexttime == tt.ttime so all
	   queued reminders are triggered at least once. */
	if ((SystemTime(1) - (q->tt.nexttime * 60) <= 60) ||
	    (q->tt.nexttime == q->tt.ttime)) {
	    /* Trigger the reminder */
	    CreateParser(q->text, &p);
	    trig.typ = q->typ;
	    strcpy(trig.passthru, q->passthru);
	    RunDisabled = q->RunDisabled;
	    if (Daemon < 0) {
		printf("NOTE reminder %s",
		       SimpleTime(q->tt.ttime));
		printf("%s", SimpleTime(SystemTime(1)/60));
		if (!*DBufValue(&q->tags)) {
		    printf("*\n");
		} else {
		    printf("%s\n", DBufValue(&(q->tags)));
		}
	    }

	    /* Set up global variables so some functions like trigdate()
	       and trigtime() work correctly                             */
	    SaveAllTriggerInfo(&(q->t), &(q->tt), JulianToday, q->tt.ttime, 1);
	    (void) TriggerReminder(&p, &trig, &q->tt, JulianToday);
	    if (Daemon < 0) {
		printf("NOTE endreminder\n");
	    }
	    fflush(stdout);
	    DestroyParser(&p);
	}

	/* Calculate the next trigger time */
	q->tt.nexttime = CalculateNextTime(q);
    }
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
static int CalculateNextTime(QueuedRem *q)
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
    if (delta == NO_DELTA) {
	if (tim < curtime) {
	    return NO_TIME;
	} else {
	    return tim;
	}
    }

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
static QueuedRem *FindNextReminder(void)
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
void GotSigInt(void)
{
    QueuedRem *q = QueueHead;

    printf("Contents of AT queue:%s", NL);

    while (q) {
	if (q->tt.nexttime != NO_TIME) {
	    printf("Trigger: %02d%c%02d  Activate: %02d%c%02d  Rep: %d  Delta: %d  Sched: %s",
		   q->tt.ttime / 60, TimeSep, q->tt.ttime % 60,
		   q->tt.nexttime / 60, TimeSep, q->tt.nexttime % 60,
		   q->tt.rep, q->tt.delta, q->sched);
	    if (*q->sched) printf("(%d)", q->ntrig+1);
	    printf("%s", NL);
	    printf("Text: %s %s%s%s%s%s", ((q->typ == MSG_TYPE) ? "MSG" :
				       ((q->typ == MSF_TYPE) ? "MSF" : 
					((q->typ == RUN_TYPE) ? "RUN" : "SPECIAL"))),
		   q->passthru,
		   (*(q->passthru)) ? " " : "",
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
static void CheckInitialFile(void)
{
    /* If date has rolled around, or file has changed, spawn a new version. */
    time_t tim = FileModTime;
    int y, m, d;

    if (stat(InitialFile, &StatBuf) == 0) tim = StatBuf.st_mtime;
    if (tim != FileModTime ||
	RealToday != SystemDate(&y, &m, &d)) {
	reread();
    }
}

/***************************************************************/
/*                                                             */
/*  CalculateNextTimeUsingSched                                */
/*                                                             */
/*  Call the scheduling function.                              */
/*                                                             */
/***************************************************************/
static int CalculateNextTimeUsingSched(QueuedRem *q)
{
    /* Use LineBuffer for temp. string storage. */
    int r;
    Value v;
    char const *s;
    int LastTime = -1;
    int ThisTime;

    if (UserFuncExists(q->sched) != 1) {
	q->sched[0] = 0;
	return NO_TIME;
    }

    RunDisabled = q->RunDisabled;  /* Don't want weird scheduling functions
				     to be a security hole!                */
    while(1) {
	char exprBuf[VAR_NAME_LEN+32];
	sprintf(exprBuf, "%s(%d)", q->sched, q->ntrig);
	s = exprBuf;
	r = EvalExpr(&s, &v, NULL);
	if (r) {
	    q->sched[0] = 0;
	    return NO_TIME;
	}
	if (v.type == TIME_TYPE) {
	    ThisTime = v.v.val;
	} else if (v.type == INT_TYPE) {
	    if (v.v.val > 0)
		if (LastTime >= 0) {
		    ThisTime = LastTime + v.v.val;
		} else {
		    ThisTime = q->tt.nexttime + v.v.val;
		}
	    else
		ThisTime = q->tt.ttime + v.v.val;

	} else {
	    DestroyValue(v);
	    q->sched[0] = 0;
	    return NO_TIME;
	}
	if (ThisTime < 0) ThisTime = 0;        /* Can't be less than 00:00 */
	if (ThisTime > (MINUTES_PER_DAY-1)) ThisTime = (MINUTES_PER_DAY-1);  /* or greater than 11:59 */
	if (DebugFlag & DB_PRTEXPR) {
	    fprintf(ErrFp, "SCHED: Considering %02d%c%02d\n",
		    ThisTime / 60, TimeSep, ThisTime % 60);
	}
	if (ThisTime > q->tt.nexttime) return ThisTime;
	if (ThisTime <= LastTime) {
	    q->sched[0] = 0;
	    return NO_TIME;
	}
	LastTime = ThisTime;
	q->ntrig++;
    }
}

/* Dump the queue in JSON format */
static void
json_queue(QueuedRem const *q)
{
    int done = 0;
    printf("[");
    while(q) {
	if (q->tt.nexttime == NO_TIME) {
	    q = q->next;
	    continue;
	}
	if (done) {
	    printf(",");
	}
	done = 1;
	printf("{");
	switch(q->typ) {
	case NO_TYPE: PrintJSONKeyPairString("type", "NO_TYPE"); break;
	case MSG_TYPE: PrintJSONKeyPairString("type", "MSG_TYPE"); break;
	case RUN_TYPE: PrintJSONKeyPairString("type", "RUN_TYPE"); break;
	case CAL_TYPE: PrintJSONKeyPairString("type", "CAL_TYPE"); break;
	case SAT_TYPE: PrintJSONKeyPairString("type", "SAT_TYPE"); break;
	case PS_TYPE: PrintJSONKeyPairString("type", "PS_TYPE"); break;
	case PSF_TYPE: PrintJSONKeyPairString("type", "PSF_TYPE"); break;
	case MSF_TYPE: PrintJSONKeyPairString("type", "MSF_TYPE"); break;
	case PASSTHRU_TYPE: PrintJSONKeyPairString("type", "PASSTHRU_TYPE"); break;
	default: PrintJSONKeyPairString("type", "?"); break;
	}
	PrintJSONKeyPairInt("rundisabled", q->RunDisabled);
	PrintJSONKeyPairInt("ntrig", q->ntrig);
	PrintJSONKeyPairTime("ttime", q->tt.ttime);
	PrintJSONKeyPairTime("nextttime", q->tt.nexttime);
	PrintJSONKeyPairInt("delta", q->tt.delta);
	if (q->tt.rep != NO_TIME) {
	    PrintJSONKeyPairInt("rep", q->tt.rep);
	}
	if (q->tt.duration != NO_TIME) {
	    PrintJSONKeyPairInt("duration", q->tt.duration);
	}
	if (q->passthru[0]) {
	    PrintJSONKeyPairString("passthru", q->passthru);
	}
	if (q->sched[0]) {
	    PrintJSONKeyPairString("sched", q->sched);
	}
	if (DBufLen(&(q->tags))) {
	    PrintJSONKeyPairString("tags", DBufValue(&(q->tags)));
	}

	/* Last one is a special case - no trailing comma */
	printf("\"");
	PrintJSONString("body");
	printf("\":\"");
	if (q->text) {
	    PrintJSONString(q->text);
	} else {
	    PrintJSONString("");
	}
	printf("\"}");
	q = q->next;
    }
    printf("]\n");
}

/***************************************************************/
/*                                                             */
/*  DaemonWait                                                 */
/*                                                             */
/*  Sleep or read command from stdin in "daemon -1" mode       */
/*                                                             */
/***************************************************************/
static void DaemonWait(struct timeval *sleep_tv)
{
    fd_set readSet;
    int retval;
    int y, m, d;
    char cmdLine[256];

    FD_ZERO(&readSet);
    FD_SET(0, &readSet);
    retval = select(1, &readSet, NULL, NULL, sleep_tv);

    /* If date has rolled around, restart */
    if (RealToday != SystemDate(&y, &m, &d)) {
	printf("NOTE newdate\nNOTE reread\n");
	fflush(stdout);
	reread();
    }

    /* If nothing readable or interrupted system call, return */
    if (retval <= 0) return;

    /* If stdin not readable, return */
    if (!FD_ISSET(0, &readSet)) return;

    /* If EOF on stdin, exit */
    if (feof(stdin)) {
	exit(0);
    }

    /* Read a line from stdin and interpret it */
    if (!fgets(cmdLine, sizeof(cmdLine), stdin)) {
	exit(0);
    }

    if (!strcmp(cmdLine, "EXIT\n")) {
	exit(0);
    } else if (!strcmp(cmdLine, "STATUS\n")) {
	int nqueued = 0;
	QueuedRem *q = QueueHead;
	while(q) {
	    if (q->tt.nexttime != NO_TIME) {
		nqueued++;
	    }
	    q = q->next;
	}
	printf("NOTE queued %d\n", nqueued);
	fflush(stdout);
    } else if (!strcmp(cmdLine, "QUEUE\n")) {
	printf("NOTE queue\n");
	QueuedRem *q = QueueHead;
	while (q) {
	    if (q->tt.nexttime != NO_TIME) {
		switch (q->typ) {
		case NO_TYPE: printf("NO_TYPE "); break;
		case MSG_TYPE: printf("MSG_TYPE "); break;
		case RUN_TYPE: printf("RUN_TYPE "); break;
		case CAL_TYPE: printf("CAL_TYPE "); break;
		case SAT_TYPE: printf("SAT_TYPE "); break;
		case PS_TYPE: printf("PS_TYPE "); break;
		case PSF_TYPE: printf("PSF_TYPE "); break;
		case MSF_TYPE: printf("MSF_TYPE "); break;
		case PASSTHRU_TYPE: printf("PASSTHRU_TYPE "); break;
		default: printf("? "); break;
		}
		printf("RunDisabled=%d ntrig=%d ttime=%02d:%02d nexttime=%02d:%02d delta=%d rep=%d duration=%d ", q->RunDisabled, q->ntrig, q->tt.ttime/60, q->tt.ttime % 60, q->tt.nexttime / 60, q->tt.nexttime % 60, q->tt.delta, (q->tt.rep != NO_TIME ? q->tt.rep : -1), (q->tt.duration != NO_TIME ? q->tt.duration : -1));
		printf("%s %s %s\n",
		       (q->passthru[0] ? q->passthru : "*"),
		       (q->sched[0] ? q->sched : "*"),
		       q->text ? q->text : "NULL");
	    }
	    q = q->next;
	}
	printf("NOTE endqueue\n");
	fflush(stdout);
    } else if (!strcmp(cmdLine, "JSONQUEUE\n")) {
	printf("NOTE JSONQUEUE\n");
	json_queue(QueueHead);
	printf("NOTE ENDJSONQUEUE\n");
	fflush(stdout);
    } else if (!strcmp(cmdLine, "REREAD\n")) {
	printf("NOTE reread\n");
	fflush(stdout);
	reread();
    } else {
	printf("ERR Invalid daemon command: %s", cmdLine);
	fflush(stdout);
    }
}

/***************************************************************/
/*                                                             */
/*  reread                                                     */
/*                                                             */
/*  Restarts Remind if date rolls over or REREAD cmd received  */
/*                                                             */
/***************************************************************/
static void reread(void)
{
    execvp(ArgV[0], (char **) ArgV);
}

