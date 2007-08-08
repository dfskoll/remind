/***************************************************************/
/*                                                             */
/*  FILES.C                                                    */
/*                                                             */
/*  Controls the opening and closing of files, etc.  Also      */
/*  handles caching of lines and reading of lines from         */
/*  files.                                                     */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by David F. Skoll                  */
/*  Copyright (C) 1999-2000 by Roaring Penguin Software Inc.   */
/*                                                             */
/***************************************************************/

#include "config.h"

#include <stdio.h>

#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#else
#include <time.h>
#endif

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

#include "types.h"
#include "protos.h"
#include "globals.h"
#include "err.h"


/* Convenient macro for closing files */
#define FCLOSE(fp) (((fp)&&((fp)!=stdin)) ? (fclose(fp),(fp)=NULL) : ((fp)=NULL))

/* Define the structures needed by the file caching system */
typedef struct cache {
    struct cache *next;
    char *text;
    int LineNo;
} CachedLine;

typedef struct cheader {
    struct cheader *next;
    char *filename;
    CachedLine *cache;
    int ownedByMe;
} CachedFile;

/* Define the structures needed by the INCLUDE file system */
typedef struct {
    char *filename;
    int LineNo;
    unsigned int IfFlags;
    int NumIfs;
    long offset;
    CachedLine *CLine;
    int ownedByMe;
} IncludeStruct;

static CachedFile *CachedFiles = (CachedFile *) NULL;
static CachedLine *CLine = (CachedLine *) NULL;

static FILE *fp;

static IncludeStruct IStack[INCLUDE_NEST];
static int IStackPtr = 0;

static int ReadLineFromFile (void);
static int CacheFile (const char *fname);
static void DestroyCache (CachedFile *cf);
static int CheckSafety (void);

/***************************************************************/
/*                                                             */
/*  ReadLine                                                   */
/*                                                             */
/*  Read a line from the file or cache.                        */
/*                                                             */
/***************************************************************/
int ReadLine(void)
{
    int r;

/* If we're at the end of a file, pop */
    while (!CLine && !fp) {
	r = PopFile();
	if (r) return r;
    }

/* If it's cached, read line from the cache */
    if (CLine) {
	CurLine = CLine->text;
	LineNo = CLine->LineNo;
	CLine = CLine->next;
	FreshLine = 1;
	if (DebugFlag & DB_ECHO_LINE) OutputLine(ErrFp);
	return OK;
    }

/* Not cached.  Read from the file. */
    return ReadLineFromFile();
}

/***************************************************************/
/*                                                             */
/*  ReadLineFromFile                                           */
/*                                                             */
/*  Read a line from the file pointed to by fp.                */
/*                                                             */
/***************************************************************/
static int ReadLineFromFile(void)
{
    int l;

    DynamicBuffer buf;

    DBufInit(&buf);
    DBufFree(&LineBuffer);

    while(fp) {
	if (DBufGets(&buf, fp) != OK) {
	    DBufFree(&LineBuffer);
	    return E_NO_MEM;
	}
	LineNo++;
	if (ferror(fp)) {
	    DBufFree(&buf);
	    DBufFree(&LineBuffer);
	    return E_IO_ERR;
	}
	if (feof(fp)) {
	    FCLOSE(fp);
	}
	l = DBufLen(&buf);
	if (l && (DBufValue(&buf)[l-1] == '\\')) {
	    DBufValue(&buf)[l-1] = 0;
	    if (DBufPuts(&LineBuffer, DBufValue(&buf)) != OK) {
		DBufFree(&buf);
		DBufFree(&LineBuffer);
		return E_NO_MEM;
	    }
	    continue;
	}
	if (DBufPuts(&LineBuffer, DBufValue(&buf)) != OK) {
	    DBufFree(&buf);
	    DBufFree(&LineBuffer);
	    return E_NO_MEM;
	}
	FreshLine = 1;
	CurLine = DBufValue(&LineBuffer);
	if (DebugFlag & DB_ECHO_LINE) OutputLine(ErrFp);
	return OK;
    }
    CurLine = DBufValue(&LineBuffer);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  OpenFile                                                   */
/*                                                             */
/*  Open a file for reading.  If it's in the cache, set        */
/*  CLine.  Otherwise, open it on disk and set fp.  If         */
/*  ShouldCache is 1, cache the file                           */
/*                                                             */
/***************************************************************/
int OpenFile(const char *fname)
{
    CachedFile *h = CachedFiles;
    int r;

/* Assume we own the file for now */
    RunDisabled &= ~RUN_NOTOWNER;

/* If it's in the cache, get it from there. */

    while (h) {
	if (!strcmp(fname, h->filename)) {
	    CLine = h->cache;
	    STRSET(FileName, fname);
	    LineNo = 0;
	    if (!h->ownedByMe) {
		RunDisabled |= RUN_NOTOWNER;
	    }
	    if (FileName) return OK; else return E_NO_MEM;
	}
	h = h->next;
    }

/* If it's a dash, then it's stdin */
    if (!strcmp(fname, "-")) {
	fp = stdin;
    } else {
	fp = fopen(fname, "r");
    }
    if (!fp || !CheckSafety()) return E_CANT_OPEN;
    CLine = NULL;
    if (ShouldCache) {
	LineNo = 0;
	r = CacheFile(fname);
	if (r == OK) {
	    fp = NULL;
	    CLine = CachedFiles->cache;
	} else {
	    if (strcmp(fname, "-")) {
		fp = fopen(fname, "r");
		if (!fp || !CheckSafety()) return E_CANT_OPEN;
	    } else {
		fp = stdin;
	    }
	}
    }
    STRSET(FileName, fname);
    LineNo = 0;
    if (FileName) return OK; else return E_NO_MEM;
}

/***************************************************************/
/*                                                             */
/*  CacheFile                                                  */
/*                                                             */
/*  Cache a file in memory.  If we fail, set ShouldCache to 0  */
/*  Returns an indication of success or failure.               */
/*                                                             */
/***************************************************************/
static int CacheFile(const char *fname)
{
    int r;
    CachedFile *cf;
    CachedLine *cl;
    char *s;

    cl = NULL;
/* Create a file header */
    cf = NEW(CachedFile);
    cf->cache = NULL;
    if (!cf) { ShouldCache = 0; FCLOSE(fp); return E_NO_MEM; }
    cf->filename = StrDup(fname);
    if (!cf->filename) {
	ShouldCache = 0;
	FCLOSE(fp);
	free(cf);
	return E_NO_MEM;
    }

    if (RunDisabled & RUN_NOTOWNER) {
	cf->ownedByMe = 0;
    } else {
	cf->ownedByMe = 1;
    }

/* Read the file */
    while(fp) {
	r = ReadLineFromFile();
	if (r) {
	    DestroyCache(cf);
	    ShouldCache = 0;
	    FCLOSE(fp);
	    return r;
	}
/* Skip blank chars */
	s = DBufValue(&LineBuffer);
	while (isspace(*s)) s++;
	if (*s && *s!=';' && *s!='#') {
/* Add the line to the cache */
	    if (!cl) {
		cf->cache = NEW(CachedLine);
		if (!cf->cache) {
		    DBufFree(&LineBuffer);
		    DestroyCache(cf);
		    ShouldCache = 0;
		    FCLOSE(fp);
		    return E_NO_MEM;
		}
		cl = cf->cache;
	    } else {
		cl->next = NEW(CachedLine);
		if (!cl->next) {
		    DBufFree(&LineBuffer);
		    DestroyCache(cf);
		    ShouldCache = 0;
		    FCLOSE(fp);
		    return E_NO_MEM;
		}
		cl = cl->next;
	    }
	    cl->next = NULL;
	    cl->LineNo = LineNo;
	    cl->text = StrDup(s);
	    DBufFree(&LineBuffer);
	    if (!cl->text) {
		DestroyCache(cf);
		ShouldCache = 0;
		FCLOSE(fp);
		return E_NO_MEM;
	    }
	}
    }

/* Put the cached file at the head of the queue */
    cf->next = CachedFiles;
    CachedFiles = cf;

    return OK;
}

/***************************************************************/
/*                                                             */
/*  PopFile - we've reached the end.  Pop up to the previous   */
/*  file, or return E_EOF                                      */
/*                                                             */
/***************************************************************/
int PopFile(void)
{
    IncludeStruct *i;

    /* Assume we own the file for now */
    RunDisabled &= ~RUN_NOTOWNER;

    if (!Hush && NumIfs) Eprint("%s", ErrMsg[E_MISS_ENDIF]);
    if (!IStackPtr) return E_EOF;
    IStackPtr--;
    i = &IStack[IStackPtr];

    LineNo = i->LineNo;
    IfFlags = i->IfFlags;
    NumIfs = i->NumIfs;
    CLine = i->CLine;
    fp = NULL;
    STRSET(FileName, i->filename);
    if (!i->ownedByMe) {
	RunDisabled |= RUN_NOTOWNER;
    }
    if (!CLine && (i->offset != -1L)) {
	/* We must open the file, then seek to specified position */
	if (strcmp(i->filename, "-")) {
	    fp = fopen(i->filename, "r");
	    if (!fp || !CheckSafety()) return E_CANT_OPEN;
	} else {
	    fp = stdin;
	}
	if (fp != stdin)
	    (void) fseek(fp, i->offset, 0);  /* Trust that it works... */
    }
    free(i->filename);
    return OK;
}

/***************************************************************/
/*                                                             */
/*  DoInclude                                                  */
/*                                                             */
/*  The INCLUDE command.                                       */
/*                                                             */
/***************************************************************/
int DoInclude(ParsePtr p)
{     
    DynamicBuffer buf;
    int r, e;

    DBufInit(&buf);
    if ( (r=ParseToken(p, &buf)) ) return r;
    e = VerifyEoln(p); 
    if (e) Eprint("%s", ErrMsg[e]);
    if ( (r=IncludeFile(DBufValue(&buf))) ) {
	DBufFree(&buf);
	return r;
    }
    DBufFree(&buf);
    NumIfs = 0;
    IfFlags = 0;
    return OK;
}

/***************************************************************/
/*                                                             */
/*  IncludeFile                                                */
/*                                                             */
/*  Process the INCLUDE command - actually do the file         */
/*  inclusion.                                                 */
/*                                                             */
/***************************************************************/
int IncludeFile(const char *fname)
{
    IncludeStruct *i;
    int r;
    int oldRunDisabled;

    if (IStackPtr+1 >= INCLUDE_NEST) return E_NESTED_INCLUDE;
    i = &IStack[IStackPtr];

    i->filename = StrDup(FileName);
    if (!i->filename) return E_NO_MEM;
    i->LineNo = LineNo;
    i->NumIfs = NumIfs;
    i->IfFlags = IfFlags;
    i->CLine = CLine;
    i->offset = -1L;
    if (RunDisabled & RUN_NOTOWNER) {
	i->ownedByMe = 0;
    } else {
	i->ownedByMe = 1;
    }
    if (fp) {
	i->offset = ftell(fp);
	FCLOSE(fp);
    }

    IStackPtr++;

    oldRunDisabled = RunDisabled;
    /* Try to open the new file */
    if (!OpenFile(fname)) {
	return OK;
    }
    RunDisabled = oldRunDisabled;
    /* Ugh!  We failed!  */
    if ( (r=PopFile()) ) return r;
    Eprint("%s: %s", ErrMsg[E_CANT_OPEN], fname);
    return E_CANT_OPEN;
}

/***************************************************************/
/*                                                             */
/* GetAccessDate - get the access date of a file.              */
/*                                                             */
/***************************************************************/
int GetAccessDate(char *file)
{
    struct stat statbuf;
    struct tm *t1;

    if (stat(file, &statbuf)) return -1;
    t1 = localtime(&(statbuf.st_atime));

    if (t1->tm_year + 1900 < BASE)
	return 0;
    else
	return Julian(t1->tm_year+1900, t1->tm_mon, t1->tm_mday);
}

/***************************************************************/
/*                                                             */
/*  DestroyCache                                               */
/*                                                             */
/*  Free all the memory used by a cached file.                 */
/*                                                             */
/***************************************************************/
static void DestroyCache(CachedFile *cf)
{
    CachedLine *cl, *cnext;
    CachedFile *temp;
    if (cf->filename) free(cf->filename);
    cl = cf->cache;
    while (cl) {
	if (cl->text) free (cl->text);
	cnext = cl->next;
	free(cl);
	cl = cnext;
    }
    if (CachedFiles == cf) CachedFiles = cf->next;
    else {
	temp = CachedFiles;
	while(temp) {
	    if (temp->next == cf) {
		temp->next = cf->next;
		break;
	    }
	    temp = temp->next;
	}
    }
    free(cf);
}

/***************************************************************/
/*                                                             */
/*  TopLevel                                                   */
/*                                                             */
/*  Returns 1 if current file is top level, 0 otherwise.       */
/*                                                             */
/***************************************************************/
int TopLevel(void)
{
    return !IStackPtr;
}

/***************************************************************/
/*                                                             */
/*  CheckSafety                                                */
/*                                                             */
/*  Returns 1 if current file is safe to read; 0 otherwise.    */
/*  Currently only meaningful for UNIX.  If we are running as  */
/*  root, we refuse to open files not owned by root.           */
/*  We also reject world-writable files, no matter             */
/*  who we're running as.                                      */
/*  As a side effect, if we don't own the file, we disable RUN */
/***************************************************************/
static int CheckSafety(void)
{
    struct stat statbuf;

    if (fp == stdin) {
	return 1;
    }

    if (fstat(fileno(fp), &statbuf)) {
	fclose(fp);
	fp = NULL;
	return 0;
    }

    /* Under UNIX, take extra precautions if running as root */
    if (!geteuid()) {
	/* Reject files not owned by root or group/world writable */
	if (statbuf.st_uid != 0) {
	    fprintf(ErrFp, "SECURITY: Won't read non-root-owned file when running as root!\n");
	    fclose(fp);
	    fp = NULL;
	    return 0;
	}
    }
    /* Sigh... /dev/null is usually world-writable, so ignore devices,
       FIFOs, sockets, etc. */
    if (!S_ISREG(statbuf.st_mode)) {
	return 1;
    }
    if ((statbuf.st_mode & S_IWOTH)) {
	fprintf(ErrFp, "SECURITY: Won't read world-writable file!\n");
	fclose(fp);
	fp = NULL;
	return 0;
    }

/* If file is not owned by me, disable RUN command */
    if (statbuf.st_uid != geteuid()) {
	RunDisabled |= RUN_NOTOWNER;
    }

    return 1;
}
