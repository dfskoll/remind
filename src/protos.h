/***************************************************************/
/*                                                             */
/*  PROTOS.H                                                   */
/*                                                             */
/*  Function Prototypes.                                       */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1997 by David F. Skoll                  */
/*                                                             */
/***************************************************************/

/* $Id: protos.h,v 1.2 1998-01-17 03:58:31 dfs Exp $ */

#ifdef HAVE_PROTOS
#define ARGS(x) x
#else
#define ARGS(x) ()
#endif

/* Define a string assignment macro - be careful!!! */
#define STRSET(x, str) { if (x) free(x); (x) = StrDup(str); }

/* Define a general malloc routine for creating pointers to objects */
#define NEW(type) ((type *) malloc(sizeof(type)))

#ifndef HAVE_STRSTR
char *strstr ARGS ((char *s1, char *s2));
#endif

int CallUserFunc ARGS ((char *name, int nargs));
int DoFset ARGS ((ParsePtr p));
void ProduceCalendar ARGS ((void));
char *SimpleTime ARGS ((int tim, char *out));
int DoRem ARGS ((ParsePtr p));
int DoFlush ARGS ((ParsePtr p));
void DoExit ARGS ((ParsePtr p));
int ParseRem ARGS ((ParsePtr s, Trigger *trig, TimeTrig *tim));
#ifdef OS2_POPUP
int TriggerReminder ARGS ((ParsePtr p, Trigger *t, TimeTrig *tim, int jul,
			   int AsPopUp));
#else
int TriggerReminder ARGS ((ParsePtr p, Trigger *t, TimeTrig *tim, int jul));
#endif
int ShouldTriggerReminder ARGS ((Trigger *t, TimeTrig *tim, int jul));
int DoSubst ARGS ((ParsePtr p, char *out, Trigger *t, TimeTrig *tt, int jul, int mode));
int DoSubstFromString ARGS ((char *source, char *dest, int jul, int tim));
int EvalExpr ARGS ((char **e, Value *v));
int DoCoerce ARGS ((char type, Value *v));
void PrintValue  ARGS ((Value *v, FILE *fp));
int CopyValue ARGS ((Value *dest, const Value *src));
int ReadLine ARGS ((void));
int OpenFile ARGS ((const char *fname));
int PopFile ARGS ((void));
int DoInclude ARGS ((ParsePtr p));
int IncludeFile ARGS ((const char *fname));
int GetAccessDate ARGS ((char *file));
int SetAccessDate ARGS ((char *fname, int jul));
int TopLevel ARGS ((void));
int CallFunc ARGS ((Operator *f, int nargs));
void InitRemind ARGS ((int argc, char *argv[]));
void Usage ARGS ((void));
int main ARGS ((int argc, char *argv[]));
int Julian ARGS ((int year, int month, int day));
void FromJulian ARGS ((int jul, int *y, int *m, int *d));
int ParseChar ARGS ((ParsePtr p, int *err, int peek));
int ParseToken ARGS ((ParsePtr p, char *out));
int ParseIdentifier ARGS ((ParsePtr p, char *out));
int EvaluateExpr ARGS ((ParsePtr p, Value *v));
int Evaluate ARGS ((char **s, Var *locals));
int FnPopValStack ARGS ((Value *val));
void Eprint ARGS ((const char *fmt, ...));
void OutputLine ARGS ((FILE *fp));
void CreateParser ARGS ((char *s, ParsePtr p));
void DestroyParser ARGS ((ParsePtr p));
void PushToken ARGS ((const char *tok));
long SystemTime ARGS ((int realtime));
int SystemDate ARGS ((int *y, int *m, int *d));
int DoIf ARGS ((ParsePtr p));
int DoElse ARGS ((ParsePtr p));
int DoEndif ARGS ((ParsePtr p));
int DoIfTrig ARGS ((ParsePtr p));
int ShouldIgnoreLine ARGS ((void));
int VerifyEoln ARGS ((ParsePtr p));
int DoDebug ARGS ((ParsePtr p));
int DoBanner ARGS ((ParsePtr p));
int DoRun ARGS ((ParsePtr p));
int DoErrMsg ARGS ((ParsePtr p));
int ClearGlobalOmits ARGS ((void));
int DoClear ARGS ((ParsePtr p));
int DestroyOmitContexts ARGS ((void));
int PushOmitContext ARGS ((ParsePtr p));
int PopOmitContext ARGS ((ParsePtr p));
int IsOmitted ARGS ((int jul, int localomit));
int DoOmit ARGS ((ParsePtr p));
int QueueReminder ARGS ((ParsePtr p, int typ, TimeTrig *tim, const char *sched));
void HandleQueuedReminders ARGS ((void));
char *FindInitialToken ARGS ((Token *tok, char *s));
void FindToken ARGS ((const char *s, Token *tok));
void FindNumericToken ARGS ((const char *s, Token *t));
int ComputeTrigger ARGS ((int today, Trigger *trig, int *err));
char *StrnCpy ARGS ((char *dest, const char *source, int n));
int StrMatch ARGS ((const char *s1, const char *s2, int n));
int StrinCmp ARGS ((const char *s1, const char *s2, int n));
char *StrDup ARGS ((const char *s));
int StrCmpi ARGS ((const char *s1, const char *s2));
Var *FindVar ARGS ((const char *str, int create));
int DeleteVar ARGS ((const char *str));
int SetVar ARGS ((const char *str, Value *val));
int GetVarValue ARGS ((const char *str, Value *val, Var *locals));
int DoSet  ARGS ((Parser *p));
int DoUnset  ARGS ((Parser *p));
int DoDump ARGS ((ParsePtr p));
void DumpVarTable ARGS ((void));
void DestroyVars ARGS ((int all));
int PreserveVar ARGS ((char *name));
int DoPreserve  ARGS ((Parser *p));
int DoSatRemind ARGS ((Trigger *trig, TimeTrig *tim, ParsePtr p));
void DoMsgCommand ARGS ((char *cmd, char *msg));
int ParseNonSpaceChar ARGS ((ParsePtr p, int *err, int peek));
unsigned int HashVal ARGS ((const char *str));
int DateOK ARGS ((int y, int m, int d));
Operator *FindFunc ARGS ((char *name, Operator where[], int num));
int InsertIntoSortBuffer ARGS ((int jul, int tim, char *body, int typ, int prio));
void IssueSortedReminders ARGS ((void));    
int UserFuncExists ARGS ((char *fn));
void JulToHeb ARGS((int jul, int *hy, int *hm, int *hd));
int HebNameToNum ARGS((const char *mname));
char *HebMonthName ARGS((int m, int y));
int RoshHashana ARGS((int i));
long DaysToHebYear ARGS((int y));
int DaysInHebYear ARGS((int y));
char *DaysInHebMonths ARGS((int ylen));
int HebToJul ARGS((int hy, int hm, int hd));
int GetValidHebDate ARGS((int yin, int min, int din, int adarbehave, int *mout, int *dout, int yahr));
int GetNextHebrewDate ARGS((int julstart, int hm, int hd, int yahr, int adarbehave, int *ans));
int ComputeJahr ARGS ((int y, int m, int d, int *ans));
int GetSysVar ARGS ((const char *name, Value *val));
int SetSysVar ARGS ((const char *name, Value *val));
void DumpSysVarByName ARGS ((const char *name));
int CalcMinsFromUTC ARGS ((int jul, int tim, int *mins, int *isdst));
#ifdef OS2_POPUP
void FillParagraph ARGS ((char *s, int AsPopUp));
#else
void FillParagraph ARGS ((char *s));
#endif
void LocalToUTC ARGS ((int locdate, int loctime, int *utcdate, int *utctime));
void UTCToLocal ARGS ((int utcdate, int utctime, int *locdate, int *loctime));
int MoonPhase ARGS ((int date, int time));
void HuntPhase ARGS ((int startdate, int starttim, int phas, int *date, int *time));
int CompareRems ARGS ((int dat1, int tim1, int prio1, int dat2, int tim2, int prio2, int bydate, int bytime, int byprio));
#ifdef __BORLANDC__
void __cdecl SigIntHandler ARGS ((int d));
#else
RETSIGTYPE SigIntHandler ARGS ((int d));
#endif
void GotSigInt ARGS ((void));

#if defined(__OS2__)
int fork ARGS ((void));
#if defined(OS2_POPUP)
void StartPopUp ARGS ((void));
void EndPopUp ARGS ((void));
int PutcPopUp ARGS ((int c));
int PutlPopUp ARGS ((char *s));
int PutsPopUp ARGS ((char *s));
#endif
#endif

#ifdef BROKEN_PUTC
int SafePutChar ARGS ((int ch));
int SafePutc ARGS ((int ch, FILE *fp));
#endif
