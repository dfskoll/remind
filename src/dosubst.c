/***************************************************************/
/*                                                             */
/*  DOSUBST.C                                                  */
/*                                                             */
/*  This performs all the "%" substitution functions when      */
/*  reminders are triggered.                                   */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-2022 by Dianne Skoll                    */
/*                                                             */
/***************************************************************/

#include "config.h"
#include "expr.h"
#define L_IN_DOSUBST
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <stdlib.h>

#include "types.h"
#include "globals.h"
#include "err.h"
#include "protos.h"

#define UPPER(c) (islower(c) ? toupper(c) : (c))
#define ABS(x) ( (x) < 0 ? -(x) : (x) )
#ifndef NL
#define NL "\n"
#endif

#define SHIP_OUT(s) if(DBufPuts(dbuf, s) != OK) return E_NO_MEM

/***************************************************************/
/*                                                             */
/*  DoSubst                                                    */
/*                                                             */
/*  Process the % escapes in the reminder.  If                 */
/*  mode==NORMAL_MODE, ignore the %" sequence.  If             */
/*  mode==CAL_MODE, process the %" sequence.                   */
/*  If mode==ADVANCE_MODE, ignore %" but don't add newline     */
/*                                                             */
/***************************************************************/
int DoSubst(ParsePtr p, DynamicBuffer *dbuf, Trigger *t, TimeTrig *tt, int jul, int mode)
{
    int diff = jul - JulianToday;
    int curtime = SystemTime(0) / 60;
    int err, done;
    int c;
    int d, m, y;
    int tim = tt->ttime;
    int h, min, hh, ch, cmin, chh;
    char const *pm, *cpm;
    int tdiff, adiff, mdiff, hdiff;
    char const *mplu, *hplu, *when, *plu;
    int has_quote = 0;
    char *ss;
    char const *expr;
    char *os;
    char s[256];
    char uf[32];
    char mypm[64];
    char mycpm[64];
    char myplu[64];
    int origLen = DBufLen(dbuf);
    int altmode;
    int r;
    Value v;

    FromJulian(jul, &y, &m, &d);

    if (tim == NO_TIME) tim = curtime;
    tdiff = tim - curtime;
    adiff = ABS(tdiff);
    mdiff = adiff % 60;
    hdiff = adiff / 60;

#ifdef	L_MPLU_OVER
    L_MPLU_OVER
#else /* L_MPLU_OVER */
    mplu = (mdiff == 1 ? "" : DynamicMplu);
#endif /* L_MPLU_OVER */

#ifdef L_HPLU_OVER
    L_HPLU_OVER
#else /* L_HPLU_OVER */
    hplu = (hdiff == 1 ? "" : DynamicHplu);
#endif /* L_HPLU_OVER */

    when = (tdiff < 0) ? DynamicAgo : DynamicFromnow;

    h = tim / 60;
    min = tim % 60;

#ifdef L_AMPM_OVERRIDE
    L_AMPM_OVERRIDE (pm, h)
#else
    r = -1;
    if (UserFuncExists("subst_ampm") == 1) {
        snprintf(s, sizeof(s), "subst_ampm(%d)", h);
        expr = (char const *) s;
        r = EvalExpr(&expr, &v, NULL);
        if (r == OK) {
            if (!DoCoerce(STR_TYPE, &v)) {
                snprintf(mypm, sizeof(mypm), "%s", v.v.str);
                pm = mypm;
            } else {
                r = -1;
            }
            DestroyValue(v);
        }
    }
    if (r != OK) {
        pm = (h < 12) ? DynamicAm : DynamicPm;
    }
#endif
    hh = (h == 12) ? 12 : h % 12;

    ch = curtime / 60;
    cmin = curtime % 60;

#ifdef L_AMPM_OVERRIDE
    L_AMPM_OVERRIDE (cpm, ch)
#else
    r = -1;
    if (UserFuncExists("subst_ampm") == 1) {
        snprintf(s, sizeof(s), "subst_ampm(%d)", h);
        expr = (char const *) s;
        r = EvalExpr(&expr, &v, NULL);
        if (r == OK) {
            if (!DoCoerce(STR_TYPE, &v)) {
                snprintf(mycpm, sizeof(mycpm), "%s", v.v.str);
                cpm = mycpm;
            } else {
                r = -1;
            }
        }
    }
    if (r != OK) {
        cpm = (h < 12) ? DynamicAm : DynamicPm;
    }
#endif
    chh = (ch == 12) ? 12 : ch % 12;

#ifdef L_ORDINAL_OVERRIDE
    L_ORDINAL_OVERRIDE;
#else
    if (UserFuncExists("subst_ordinal") == 1) {
        snprintf(s, sizeof(s), "subst_ordinal(%d)", d);
        expr = (char const *) s;
        r = EvalExpr(&expr, &v, NULL);
        if (r == OK) {
            if (!DoCoerce(STR_TYPE, &v)) {
                snprintf(myplu, sizeof(myplu), "%s", v.v.str);
                plu = myplu;
            } else {
                r = -1;
            }
        }
    }
    if (r != OK) {
        switch(d) {
        case 1:
        case 21:
        case 31: plu = "st"; break;

        case 2:
        case 22: plu = "nd"; break;

        case 3:
        case 23: plu = "rd"; break;

        default: plu = "th"; break;
        }
    }
#endif

    while(1) {
	c = ParseChar(p, &err, 0);
	if (err) {
	    DBufFree(dbuf);
	    return err;
	}
	if (c == '\n') continue;
	if (!c) {
	    if (mode != CAL_MODE && mode != ADVANCE_MODE &&
		t->typ != RUN_TYPE && !MsgCommand) {
		if (DBufPutc(dbuf, '\n') != OK) return E_NO_MEM;
	    }
	    break;
	}
	if (c != '%') {
	    if (DBufPutc(dbuf, c) != OK) return E_NO_MEM;
	    continue;
	}
	altmode = 0;
	s[0] = 0;
	c = ParseChar(p, &err, 0);
	if (err) {
	    DBufFree(dbuf);
	    return err;
	}
	if (!c) {
	    break;
	}
	if (c == '*') {
	    altmode = c;
	    c = ParseChar(p, &err, 0);
	    if (err) {
		DBufFree(dbuf);
		return err;
	    }
	    if (!c) {
		break;
	    }
	}
	done = 0;
        snprintf(uf, sizeof(uf), "subst_%c", c);
        if (UserFuncExists(uf) == 3) {
            snprintf(s, sizeof(s), "subst_%c(%d, '%04d-%02d-%02d', %02d:%02d)",
                     c, altmode ? 1 : 0, y, m+1, d, h, min);
            expr = (char const *) s;
            r = EvalExpr(&expr, &v, NULL);
            if (r == OK) {
                if (!DoCoerce(STR_TYPE, &v)) {
                    if (DBufPuts(dbuf, v.v.str) != OK) {
                        DestroyValue(v);
                        return E_NO_MEM;
                    }
                }
                DestroyValue(v);
                continue;
            }
        }

	if (diff <= 1) {
	    switch(UPPER(c)) {
#ifndef L_NOTOMORROW_A
	    case 'A':
#endif
#ifndef L_NOTOMORROW_B
	    case 'B':
#endif
#ifndef L_NOTOMORROW_C
	    case 'C':
#endif
#ifndef L_NOTOMORROW_E
	    case 'E':
#endif
#ifndef L_NOTOMORROW_F
	    case 'F':
#endif
#ifndef L_NOTOMORROW_G
	    case 'G':
#endif
#ifndef L_NOTOMORROW_H
	    case 'H':
#endif
#ifndef L_NOTOMORROW_I
	    case 'I':
#endif
#ifndef L_NOTOMORROW_J
	    case 'J':
#endif
#ifndef L_NOTOMORROW_K
	    case 'K':
#endif
#ifndef L_NOTOMORROW_L
	    case 'L':
#endif
#ifndef L_NOTOMORROW_U
	    case 'U':
#endif
#ifndef L_NOTOMORROW_V
	    case 'V':
#endif
		snprintf(s, sizeof(s), "%s", (diff ? DynamicTomorrow: DynamicToday));
		SHIP_OUT(s);
		done = 1;
		break;

	    default: done = 0;
	    }
	}


        if (!done) {
            snprintf(uf, sizeof(uf), "subst_%cx", c);
            if (UserFuncExists(uf) == 3) {
                snprintf(s, sizeof(s), "subst_%cx(%d, '%04d-%02d-%02d', %02d:%02d)",
                         c, altmode ? 1 : 0, y, m+1, d, h, min);
                expr = (char const *) s;
                r = EvalExpr(&expr, &v, NULL);
                if (r == OK) {
                    if (!DoCoerce(STR_TYPE, &v)) {
                        if (DBufPuts(dbuf, v.v.str) != OK) {
                            DestroyValue(v);
                            return E_NO_MEM;
                        }
                    }
                    DestroyValue(v);
                    continue;
                }
            }
        }

	if (!done) switch(UPPER(c)) {
	case 'A':
#ifdef L_A_OVER
	    L_A_OVER
#else
	    if (altmode == '*') {
		snprintf(s, sizeof(s), "%s, %d %s, %d", get_day_name(jul%7), d,
			get_month_name(m), y);
	    } else {
		snprintf(s, sizeof(s), "%s %s, %d %s, %d", DynamicOn, get_day_name(jul%7), d,
			get_month_name(m), y);
	    }
#endif
	    SHIP_OUT(s);
	    break;

	case 'B':
#ifdef L_B_OVER
	    L_B_OVER
#else
	    snprintf(s, sizeof(s), L_INXDAYS, diff);
#endif
	    SHIP_OUT(s);
	    break;

	case 'C':
#ifdef L_C_OVER
	    L_C_OVER
#else
	    if (altmode == '*') {
		snprintf(s, sizeof(s), "%s", get_day_name(jul%7));
	    } else {
		snprintf(s, sizeof(s), "%s %s", DynamicOn, get_day_name(jul%7));
	    }
#endif
	    SHIP_OUT(s);
	    break;

	case 'D':
#ifdef L_D_OVER
	    L_D_OVER
#else
	    snprintf(s, sizeof(s), "%d", d);
#endif
	    SHIP_OUT(s);
	    break;

	case 'E':
#ifdef L_E_OVER
	    L_E_OVER
#else
	    if (altmode == '*') {
		snprintf(s, sizeof(s), "%02d%c%02d%c%04d", d, DateSep,
			m+1, DateSep, y);
	    } else {
		snprintf(s, sizeof(s), "%s %02d%c%02d%c%04d", DynamicOn, d, DateSep,
			m+1, DateSep, y);
	    }
#endif
	    SHIP_OUT(s);
	    break;

	case 'F':
#ifdef L_F_OVER
	    L_F_OVER
#else
	    if (altmode == '*') {
		snprintf(s, sizeof(s), "%02d%c%02d%c%04d", m+1, DateSep, d, DateSep, y);
	    } else {
		snprintf(s, sizeof(s), "%s %02d%c%02d%c%04d", DynamicOn, m+1, DateSep, d, DateSep, y);
	    }
#endif
	    SHIP_OUT(s);
	    break;

	case 'G':
#ifdef L_G_OVER
	    L_G_OVER
#else
	    if (altmode == '*') {
		snprintf(s, sizeof(s), "%s, %d %s", get_day_name(jul%7), d, get_month_name(m));
	    } else {
		snprintf(s, sizeof(s), "%s %s, %d %s", DynamicOn, get_day_name(jul%7), d, get_month_name(m));
	    }
#endif
	    SHIP_OUT(s);
	    break;

	case 'H':
#ifdef L_H_OVER
	    L_H_OVER
#else
	    if (altmode == '*') {
		snprintf(s, sizeof(s), "%02d%c%02d", d, DateSep, m+1);
	    } else {
		snprintf(s, sizeof(s), "%s %02d%c%02d", DynamicOn, d, DateSep, m+1);
	    }
#endif
	    SHIP_OUT(s);
	    break;

	case 'I':
#ifdef L_I_OVER
	    L_I_OVER
#else
	    if (altmode == '*') {
		snprintf(s, sizeof(s), "%02d%c%02d", m+1, DateSep, d);
	    } else {
		snprintf(s, sizeof(s), "%s %02d%c%02d", DynamicOn, m+1, DateSep, d);
	    }
#endif
	    SHIP_OUT(s);
	    break;

	case 'J':
#ifdef L_J_OVER
	    L_J_OVER
#else
	    if (altmode == '*') {
		snprintf(s, sizeof(s), "%s, %s %d%s, %d", get_day_name(jul%7),
			get_month_name(m), d, plu, y);
	    } else {
		snprintf(s, sizeof(s), "%s %s, %s %d%s, %d", DynamicOn, get_day_name(jul%7),
			get_month_name(m), d, plu, y);
	    }
#endif
	    SHIP_OUT(s);
	    break;

	case 'K':
#ifdef L_K_OVER
	    L_K_OVER
#else
	    if (altmode == '*') {
		snprintf(s, sizeof(s), "%s, %s %d%s", get_day_name(jul%7),
			get_month_name(m), d, plu);
	    } else {
		snprintf(s, sizeof(s), "%s %s, %s %d%s", DynamicOn, get_day_name(jul%7),
			get_month_name(m), d, plu);
	    }
#endif
	    SHIP_OUT(s);
	    break;

	case 'L':
#ifdef L_L_OVER
	    L_L_OVER
#else
	    if (altmode == '*') {
		snprintf(s, sizeof(s), "%04d%c%02d%c%02d", y, DateSep, m+1, DateSep, d);
	    } else {
		snprintf(s, sizeof(s), "%s %04d%c%02d%c%02d", DynamicOn, y, DateSep, m+1, DateSep, d);
	    }
#endif
	    SHIP_OUT(s);
	    break;

	case 'M':
#ifdef L_M_OVER
	    L_M_OVER
#else
	    snprintf(s, sizeof(s), "%s", get_month_name(m));
#endif
	    SHIP_OUT(s);
	    break;

	case 'N':
#ifdef L_N_OVER
	    L_N_OVER
#else
	    snprintf(s, sizeof(s), "%d", m+1);
#endif
	    SHIP_OUT(s);
	    break;

	case 'O':
#ifdef L_O_OVER
	    L_O_OVER
#else
            if (RealToday == JulianToday) snprintf(s, sizeof(s), " (%s)", DynamicToday);
	    else *s = 0;
#endif
	    SHIP_OUT(s);
	    break;

	case 'P':
#ifdef L_P_OVER
	    L_P_OVER
#else
	    snprintf(s, sizeof(s), "%s", (diff == 1 ? "" : L_PLURAL));
#endif
	    SHIP_OUT(s);
	    break;

	case 'Q':
#ifdef L_Q_OVER
	    L_Q_OVER
#else
	    snprintf(s, sizeof(s), "%s", (diff == 1 ? "'s" : "s'"));
#endif
	    SHIP_OUT(s);
	    break;

	case 'R':
#ifdef L_R_OVER
	    L_R_OVER
#else
	    snprintf(s, sizeof(s), "%02d", d);
#endif
	    SHIP_OUT(s);
	    break;

	case 'S':
#ifdef L_S_OVER
	    L_S_OVER
#else
	    snprintf(s, sizeof(s), "%s", plu);
#endif
	    SHIP_OUT(s);
	    break;

	case 'T':
#ifdef L_T_OVER
	    L_T_OVER
#else
	    snprintf(s, sizeof(s), "%02d", m+1);
#endif
	    SHIP_OUT(s);
	    break;

	case 'U':
#ifdef L_U_OVER
	    L_U_OVER
#else
	    if (altmode == '*') {
		snprintf(s, sizeof(s), "%s, %d%s %s, %d", get_day_name(jul%7), d,
			plu, get_month_name(m), y);
	    } else {
		snprintf(s, sizeof(s), "%s %s, %d%s %s, %d", DynamicOn, get_day_name(jul%7), d,
			plu, get_month_name(m), y);
	    }
#endif
	    SHIP_OUT(s);
	    break;

	case 'V':
#ifdef L_V_OVER
	    L_V_OVER
#else
	    if (altmode == '*') {
		snprintf(s, sizeof(s), "%s, %d%s %s", get_day_name(jul%7), d, plu,
			get_month_name(m));
	    } else {
		snprintf(s, sizeof(s), "%s %s, %d%s %s", DynamicOn, get_day_name(jul%7), d, plu,
			get_month_name(m));
	    }
#endif
	    SHIP_OUT(s);
	    break;

	case 'W':
#ifdef L_W_OVER
	    L_W_OVER
#else
	    snprintf(s, sizeof(s), "%s", get_day_name(jul%7));
#endif
	    SHIP_OUT(s);
	    break;

	case 'X':
#ifdef L_X_OVER
	    L_X_OVER
#else
	    snprintf(s, sizeof(s), "%d", diff);
#endif
	    SHIP_OUT(s);
	    break;

	case 'Y':
#ifdef L_Y_OVER
	    L_Y_OVER
#else
	    snprintf(s, sizeof(s), "%d", y);
#endif
	    SHIP_OUT(s);
	    break;

	case 'Z':
#ifdef L_Z_OVER
	    L_Z_OVER
#else
	    snprintf(s, sizeof(s), "%d", y % 100);
#endif
	    SHIP_OUT(s);
	    break;

	case '1':
#ifdef L_1_OVER
	    L_1_OVER
#else
	    if (tdiff == 0)
		snprintf(s, sizeof(s), "%s", DynamicNow);
	    else if (hdiff == 0)
		snprintf(s, sizeof(s), "%d %s%s %s", mdiff, DynamicMinute, mplu, when);
	    else if (mdiff == 0)
		snprintf(s, sizeof(s), "%d %s%s %s", hdiff, DynamicHour, hplu, when);
	    else
		snprintf(s, sizeof(s), "%d %s%s %s %d %s%s %s", hdiff, DynamicHour, hplu,
			DynamicAnd, mdiff, DynamicMinute, mplu, when);
#endif
	    SHIP_OUT(s);
	    break;

	case '2':
#ifdef L_2_OVER
	    L_2_OVER
#else
	    if (altmode == '*') {
		snprintf(s, sizeof(s), "%d%c%02d%s", hh, TimeSep, min, pm);
	    } else {
		snprintf(s, sizeof(s), "%s %d%c%02d%s", DynamicAt, hh, TimeSep, min, pm);
	    }
#endif
	    SHIP_OUT(s);
	    break;

	case '3':
#ifdef L_3_OVER
	    L_3_OVER
#else

	    if (altmode == '*') {
		snprintf(s, sizeof(s), "%02d%c%02d", h, TimeSep, min);
	    } else {
		snprintf(s, sizeof(s), "%s %02d%c%02d", DynamicAt, h, TimeSep, min);
	    }
#endif
	    SHIP_OUT(s);
	    break;

	case '4':
#ifdef L_4_OVER
	    L_4_OVER
#else
	    snprintf(s, sizeof(s), "%d", tdiff);
#endif
	    SHIP_OUT(s);
	    break;

	case '5':
#ifdef L_5_OVER
	    L_5_OVER
#else
	    snprintf(s, sizeof(s), "%d", adiff);
#endif
	    SHIP_OUT(s);
	    break;

	case '6':
#ifdef L_6_OVER
	    L_6_OVER
#else
	    snprintf(s, sizeof(s), "%s", when);
#endif
	    SHIP_OUT(s);
	    break;

	case '7':
#ifdef L_7_OVER
	    L_7_OVER
#else
	    snprintf(s, sizeof(s), "%d", hdiff);
#endif
	    SHIP_OUT(s);
	    break;

	case '8':
#ifdef L_8_OVER
	    L_8_OVER
#else
	    snprintf(s, sizeof(s), "%d", mdiff);
#endif
	    SHIP_OUT(s);
	    break;

	case '9':
#ifdef L_9_OVER
	    L_9_OVER
#else
	    snprintf(s, sizeof(s), "%s", mplu);
#endif
	    SHIP_OUT(s);
	    break;

	case '0':
#ifdef L_0_OVER
	    L_0_OVER
#else
	    snprintf(s, sizeof(s), "%s", hplu);
#endif
	    SHIP_OUT(s);
	    break;

	case '!':
#ifdef L_BANG_OVER
	    L_BANG_OVER
#else
            snprintf(s, sizeof(s), "%s", (tdiff >= 0 ? DynamicIs : DynamicWas));
#endif
	    SHIP_OUT(s);
	    break;

	case '@':
#ifdef L_AT_OVER
	    L_AT_OVER
#else
	    snprintf(s, sizeof(s), "%d%c%02d%s", chh, TimeSep, cmin, cpm);
#endif
	    SHIP_OUT(s);
	    break;

	case '#':
#ifdef L_HASH_OVER
	    L_HASH_OVER
#else
	    snprintf(s, sizeof(s), "%02d%c%02d", ch, TimeSep, cmin);
#endif
	    SHIP_OUT(s);
	    break;

	case '_':
	    if (PsCal == PSCAL_LEVEL2 || PsCal == PSCAL_LEVEL3 || (mode != CAL_MODE && mode != ADVANCE_MODE && !MsgCommand)) {
		snprintf(s, sizeof(s), "%s", NL);
	    } else {
		snprintf(s, sizeof(s), " ");
	    }
	    SHIP_OUT(s);
	    break;

	case QUOTE_MARKER:
	    /* Swallow any QUOTE_MARKERs which may somehow creep in... */
	    break;

	case '"':
            if (DontSuppressQuoteMarkers) {
                if (DBufPutc(dbuf, '%') != OK) return E_NO_MEM;
                if (DBufPutc(dbuf, c) != OK) return E_NO_MEM;
            } else {
                if (DBufPutc(dbuf, QUOTE_MARKER) != OK) return E_NO_MEM;
                has_quote = 1;
            }
	    break;

	default:
	    if (DBufPutc(dbuf, c) != OK) return E_NO_MEM;
	}
	if (isupper(c)) {
	    os = DBufValue(dbuf);
	    os += strlen(os) - strlen(s);
	    if (os >= DBufValue(dbuf)) {
		*os = UPPER(*os);
	    }
	}
    }

/* We're outside the big while loop.  The only way to get here is for c to
   be null.  Now we go through and delete %" sequences, if it's the
   NORMAL_MODE, or retain only things within a %" sequence if it's the
   CAL_MODE. */

/* If there are NO quotes, then:  If CAL_MODE && RUN_TYPE, we don't want the
   reminder in the calendar.  Zero the output buffer and quit. */
    if (!has_quote) {
	if ((mode == ADVANCE_MODE || mode == CAL_MODE) && t->typ == RUN_TYPE) {
	    *DBufValue(dbuf) = 0;
	    dbuf->len = 0;
	}
	return OK;
    }

/* There ARE quotes.  If in CAL_MODE, delete everything before first quote
   and after second quote.  If in NORMAL_MODE, delete the %" sequences. */

    ss = DBufValue(dbuf) + origLen;
    os = ss;
    if (mode == NORMAL_MODE || mode == ADVANCE_MODE) {
	while (*ss) {
	    if (*ss != QUOTE_MARKER) *os++ = *ss;
	    ss++;
	}
	*os = 0;
    } else {
/* Skip past the quote marker */
	while (*ss && (*ss != QUOTE_MARKER)) ss++;

/* Security check... actually, *s must == QUOTE_MARKER at this point, but
   it doesn't hurt to make it a bit robust. */
	if (*ss) ss++;

/* Copy the output until the next QUOTE_MARKER */
	while (*ss && (*ss != QUOTE_MARKER)) *os++ = *ss++;
	*os = 0;
    }

    /* Violating encapsulation here!!!! */
    dbuf->len = strlen(dbuf->buffer);

    return OK;
}


/***************************************************************/
/*                                                             */
/*  DoSubstFromString                                          */
/*                                                             */
/*  DoSubst consumes input from a parser.  This function       */
/*  consumes characters from a string.  It also provides       */
/*  default triggers and a mode of NORMAL_MODE.                */
/*                                                             */
/***************************************************************/
int DoSubstFromString(char const *source, DynamicBuffer *dbuf,
			     int jul, int tim)
{
    Trigger tempTrig;
    TimeTrig tempTime;
    Parser tempP;
    int r;

    if (jul == NO_DATE) jul=JulianToday;
    if (tim == NO_TIME) tim=SystemTime(0)/60;
    CreateParser(source, &tempP);
    tempP.allownested = 0;
    tempTrig.typ = MSG_TYPE;
    tempTime.ttime = tim;

    r = DoSubst(&tempP, dbuf, &tempTrig, &tempTime, jul, NORMAL_MODE);
    DestroyParser(&tempP);
    return r;
}
