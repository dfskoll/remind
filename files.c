/***************************************************************/
/*                                                             */
/*  FILES.C                                                    */
/*                                                             */
/*  Controls the opening and closing of files, etc.  Also      */
/*  handles caching of lines and reading of lines from         */
/*  files.                                                     */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1997 by David F. Skoll                  */
/*                                                             */
/***************************************************************/

#include "config.h"
static char const RCSID[] = "$Id: files.c,v 1.5 1997-03-30 19:07:39 dfs Exp $";

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#if defined(__MSDOS__)
#include <io.h>
#endif

#ifdef __MSC__
#include <dos.h>
#endif

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
} CachedFile;

/* Define the structures needed by the INCLUDE file system */
typedef struct {
    char *filename;
    int LineNo;
    unsigned int IfFlags;
    int NumIfs;
    long offset;
    CachedLine *CLine;
} IncludeStruct;

static CachedFile *CachedFiles = (CachedFile *) NULL;
static CachedLine *CLine = (CachedLine *) NULL;

static FILE *fp;

static IncludeStruct IStack[INCLUDE_NEST];
static int IStackPtr = 0;

PRIVATE int ReadLineFromFile ARGS ((void));
PRIVATE int CacheFile ARGS ((const char *fname));
PRIVATE void DestroyCache ARGS ((CachedFile *cf));

/***************************************************************/
/*                                                             */
/*  ReadLine                                                   */
/*                                                             */
/*  Read a line from the file or cache.                        */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PUBLIC int ReadLine(void)
#else
int ReadLine()
#endif
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
#ifdef HAVE_PROTOS
PRIVATE int ReadLineFromFile(void)
#else
static ReadLineFromFile()
#endif
{
    int l;
    char *ptr;
    char *tmp;

    CurLine = LineBuffer;
    *LineBuffer = (char) 0;
    l = 0;
    ptr = LineBuffer;
    while(fp) {
	tmp=fgets(ptr, LINELEN-l, fp);
	LineNo++;
	if (ferror(fp)) return E_IO_ERR;
	if (feof(fp) || !tmp) {
	    FCLOSE(fp);
	}
	l = strlen(LineBuffer);
	if (l && (LineBuffer[l-1] == '\n')) LineBuffer[--l] = '\0';
	if (l && (LineBuffer[l-1] == '\\')) {
	    l--;
	    ptr = LineBuffer+l;
	    if (l >= LINELEN-1) return E_LINE_2_LONG;
	    continue;
	}
	FreshLine = 1;
	if (DebugFlag & DB_ECHO_LINE) OutputLine(ErrFp);
	return OK;
    }
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
#ifdef HAVE_PROTOS
PUBLIC int OpenFile(const char *fname)
#else
int OpenFile(fname)
char *fname;
#endif
{
    CachedFile *h = CachedFiles;
    int r;

/* If it's in the cache, get it from there. */

    while (h) {
	if (!strcmp(fname, h->filename)) {
	    CLine = h->cache;
	    STRSET(FileName, fname);
	    LineNo = 0;
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
    if (!fp) return E_CANT_OPEN;
    CLine = NULL;
    if (ShouldCache) {
	LineNo = 0;
	r = CacheFile(fname);
	if (r == OK) {
	    fp = NULL;
	    CLine = CachedFiles->cache;
	} else {
	    if (strcmp(fname, "-"))
		fp = fopen(fname, "r");
	    else
		fp = stdin;
	    if (!fp) return E_CANT_OPEN;
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
#ifdef HAVE_PROTOS
PRIVATE int CacheFile(const char *fname)
#else
static int CacheFile(fname)
char *fname;
#endif
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
	s = LineBuffer;
	while (isspace(*s)) s++;
	if (*s && *s!=';' && *s!='#') {
/* Add the line to the cache */
	    if (!cl) {
		cf->cache = NEW(CachedLine);
		if (!cf->cache) {
		    DestroyCache(cf);
		    ShouldCache = 0;
		    FCLOSE(fp);
		    return E_NO_MEM;
		}
		cl = cf->cache;
	    } else {
		cl->next = NEW(CachedLine);
		if (!cl->next) {
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
#ifdef HAVE_PROTOS
PUBLIC int PopFile(void)
#else
int PopFile()
#endif
{
    IncludeStruct *i;

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
    if (!CLine && (i->offset != -1L)) {
	/* We must open the file, then seek to specified position */
	if (strcmp(i->filename, "-"))
	    fp = fopen(i->filename, "r");
	else
	    fp = stdin;
	if (!fp) return E_CANT_OPEN;
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
#ifdef HAVE_PROTOS
PUBLIC int DoInclude(ParsePtr p)
#else
int DoInclude(p)
ParsePtr p;
#endif
{     
    char tok[TOKSIZE];
    int r, e;

    if ( (r=ParseToken(p, tok)) ) return r;
    e = VerifyEoln(p); 
    if (e) Eprint("%s", ErrMsg[e]);
    if ( (r=IncludeFile(tok)) ) return r;
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
#ifdef HAVE_PROTOS
PUBLIC int IncludeFile(const char *fname)
#else
int IncludeFile(fname)
char *fname;
#endif
{
    IncludeStruct *i;
    int r;

    if (IStackPtr+1 >= INCLUDE_NEST) return E_NESTED_INCLUDE;
    i = &IStack[IStackPtr];

    i->filename = StrDup(FileName);
    if (!i->filename) return E_NO_MEM;
    i->LineNo = LineNo;
    i->NumIfs = NumIfs;
    i->IfFlags = IfFlags;
    i->CLine = CLine;
    i->offset = -1L;
    if (fp) {
	i->offset = ftell(fp);
	FCLOSE(fp);
    }

    IStackPtr++;

    /* Try to open the new file */
    if (!OpenFile(fname)) {
	return OK;
    }
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
#ifdef HAVE_PROTOS
PUBLIC int GetAccessDate(char *file)
#else
int GetAccessDate(file)
char *file;
#endif
{
    struct stat statbuf;
    struct tm *t1;

    if (stat(file, &statbuf)) return -1;
#ifdef __TURBOC__
    t1 = localtime( (time_t *) &(statbuf.st_atime) );
#else
    t1 = localtime(&(statbuf.st_atime));
#endif

    if (t1->tm_year + 1900 < BASE)
	return 0;
    else
	return Julian(t1->tm_year+1900, t1->tm_mon, t1->tm_mday);
}

/***************************************************************/
/*                                                             */
/*  SetAccessDate                                              */
/*                                                             */
/*  Used only by DOS to set access date after we close the     */
/*  file.  Not needed for UNIX.                                */
/*                                                             */
/***************************************************************/
#if defined(__MSDOS__)
/*
 * WARNING WARNING WARNING WARNING
 * In the version of Turbo C which I have, there is a bug in the
 * stdio.h file.  The following lines correct the bug.  YOU MAY
 * HAVE TO REMOVE THESE LINES FOR LATER VERSIONS OF TURBOC
 */
#ifdef __TURBOC__
#ifndef fileno
#define fileno(f) ((f)->fd)
#endif
#endif

#ifdef HAVE_PROTOS
PUBLIC int SetAccessDate(char *fname, int jul)
#else
int SetAccessDate(fname, jul)
char *fname;
int jul;
#endif
{

#ifdef __TURBOC__   
    int y, m, d;
    struct ftime ft;
    FILE *f;

    FromJulian(jul, &y, &m, &d);
    ft.ft_tsec = 0;
    ft.ft_min = 0;
    ft.ft_hour = 12;  /* Arbitrarily set time to noon. */
    ft.ft_day = (unsigned int) d;
    ft.ft_month = (unsigned int) m+1;
    ft.ft_year = (unsigned int) (y - 1980);

    f = fopen(fname, "r"); 
    if (!f || setftime(fileno(f) , &ft)) {

#else /* Must be MSC */
    if (utime(fname, (struct utimbuf *) NULL)) {
#endif   	
	fprintf(ErrFp, ErrMsg[M_CANTSET_ACCESS], fname);

#ifdef __TURBOC__
	if (f) FCLOSE(f);
#endif
	return -1;
    }

#ifdef __TURBOC__
    FCLOSE(f);
#endif

    return 0;
}
#endif /* __MSDOS__ */

/***************************************************************/
/*                                                             */
/*  DestroyCache                                               */
/*                                                             */
/*  Free all the memory used by a cached file.                 */
/*                                                             */
/***************************************************************/
#ifdef HAVE_PROTOS
PRIVATE void DestroyCache(CachedFile *cf)
#else
static void DestroyCache(cf)
CachedFile *cf;
#endif
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
#ifdef HAVE_PROTOS
PUBLIC int TopLevel(void)
#else
int TopLevel()
#endif
{
    return !IStackPtr;
}
