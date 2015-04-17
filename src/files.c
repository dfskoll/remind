/***************************************************************/
/*                                                             */
/*  FILES.C                                                    */
/*                                                             */
/*  Controls the opening and closing of files, etc.  Also      */
/*  handles caching of lines and reading of lines from         */
/*  files.                                                     */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by Dianne Skoll                    */
/*  Copyright (C) 1999-2000 by Roaring Penguin Software Inc.   */
/*                                                             */
/***************************************************************/

#include "config.h"

#include <stdio.h>

#include <string.h>
#include <errno.h>
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

#ifdef HAVE_GLOB_H
#include <glob.h>
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
    char const *text;
    int LineNo;
} CachedLine;

typedef struct cheader {
    struct cheader *next;
    char const *filename;
    CachedLine *cache;
    int ownedByMe;
} CachedFile;

/* A linked list of filenames if we INCLUDE /some/directory/  */
typedef struct fname_chain {
    struct fname_chain *next;
    char const *filename;
} FilenameChain;

/* Cache filename chains for directories */
typedef struct directory_fname_chain {
    struct directory_fname_chain *next;
    FilenameChain *chain;
    char const *dirname;
} DirectoryFilenameChain;

/* Define the structures needed by the INCLUDE file system */
typedef struct {
    char const *filename;
    FilenameChain *chain;
    int LineNo;
    unsigned int IfFlags;
    int NumIfs;
    long offset;
    CachedLine *CLine;
    int ownedByMe;
} IncludeStruct;

static CachedFile *CachedFiles = (CachedFile *) NULL;
static CachedLine *CLine = (CachedLine *) NULL;
static DirectoryFilenameChain *CachedDirectoryChains = NULL;

static FILE *fp;

static IncludeStruct IStack[INCLUDE_NEST];
static int IStackPtr = 0;

static int ReadLineFromFile (void);
static int CacheFile (char const *fname);
static void DestroyCache (CachedFile *cf);
static int CheckSafety (void);
static int PopFile (void);

static void OpenPurgeFile(char const *fname, char const *mode)
{
    DynamicBuffer fname_buf;

    if (PurgeFP != NULL && PurgeFP != stdout) {
	fclose(PurgeFP);
    }
    PurgeFP = NULL;

    /* Do not open a purge file if we're below purge
       include depth */
    if (IStackPtr-2 >= PurgeIncludeDepth) {
	PurgeFP = NULL;
	return;
    }

    DBufInit(&fname_buf);
    if (DBufPuts(&fname_buf, fname) != OK) return;
    if (DBufPuts(&fname_buf, ".purged") != OK) return;
    PurgeFP = fopen(DBufValue(&fname_buf), mode);
    if (!PurgeFP) {
	fprintf(ErrFp, "Cannot open `%s' for writing: %s\n", DBufValue(&fname_buf), strerror(errno));
    }
    DBufFree(&fname_buf);
}

static void FreeChainItem(FilenameChain *chain)
{
	if (chain->filename) free((void *) chain->filename);
	free(chain);
}

static void FreeChain(FilenameChain *chain)
{
    FilenameChain *next;
    while(chain) {
	next = chain->next;
	FreeChainItem(chain);
	chain = next;
    }
}

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
    char copy_buffer[4096];
    size_t n;

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
	    if ((DBufLen(&buf) == 0) &&
		(DBufLen(&LineBuffer) == 0) && PurgeMode) {
		if (PurgeFP != NULL && PurgeFP != stdout) fclose(PurgeFP);
		PurgeFP = NULL;
	    }
	}
	l = DBufLen(&buf);
	if (l && (DBufValue(&buf)[l-1] == '\\')) {
	    if (PurgeMode) {
		if (DBufPuts(&LineBuffer, DBufValue(&buf)) != OK) {
		    DBufFree(&buf);
		    DBufFree(&LineBuffer);
		    return E_NO_MEM;
		}
		if (DBufPutc(&LineBuffer, '\n') != OK) {
		    DBufFree(&buf);
		    DBufFree(&LineBuffer);
		    return E_NO_MEM;
		}
	    } else {
		DBufValue(&buf)[l-1] = '\n';
		if (DBufPuts(&LineBuffer, DBufValue(&buf)) != OK) {
		    DBufFree(&buf);
		    DBufFree(&LineBuffer);
		    return E_NO_MEM;
		}
	    }
	    continue;
	}
	if (DBufPuts(&LineBuffer, DBufValue(&buf)) != OK) {
	    DBufFree(&buf);
	    DBufFree(&LineBuffer);
	    return E_NO_MEM;
	}
	DBufFree(&buf);

	/* If the line is: __EOF__ treat it as end-of-file */
	CurLine = DBufValue(&LineBuffer);
	if (!strcmp(CurLine, "__EOF__")) {
	    if (PurgeMode && PurgeFP) {
		PurgeEchoLine("%s\n", "__EOF__");
		while ((n = fread(copy_buffer, 1, sizeof(copy_buffer), fp)) != 0) {
		    fwrite(copy_buffer, 1, n, PurgeFP);
		}
		if (PurgeFP != stdout) fclose(PurgeFP);
		PurgeFP = NULL;
	    }
	    FCLOSE(fp);
	    DBufFree(&LineBuffer);
	    CurLine = DBufValue(&LineBuffer);
	}

	FreshLine = 1;
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
int OpenFile(char const *fname)
{
    CachedFile *h = CachedFiles;
    int r;

    if (PurgeMode) {
	if (PurgeFP != NULL && PurgeFP != stdout) {
	    fclose(PurgeFP);
	}
	PurgeFP = NULL;
    }

/* Assume we own the file for now */
    RunDisabled &= ~RUN_NOTOWNER;

/* If it's in the cache, get it from there. */

    while (h) {
	if (!strcmp(fname, h->filename)) {
	    if (DebugFlag & DB_TRACE_FILES) {
		fprintf(ErrFp, "Reading `%s': Found in cache\n", fname);
	    }
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
	if (PurgeMode) {
	    PurgeFP = stdout;
	}
	if (DebugFlag & DB_TRACE_FILES) {
	    fprintf(ErrFp, "Reading `-': Reading stdin\n");
	}
    } else {
	fp = fopen(fname, "r");
	if (DebugFlag & DB_TRACE_FILES) {
	    fprintf(ErrFp, "Reading `%s': Opening file on disk\n", fname);
	}
	if (PurgeMode) {
	    OpenPurgeFile(fname, "w");
	}
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
		if (PurgeMode) OpenPurgeFile(fname, "w");
	    } else {
		fp = stdin;
		if (PurgeMode) PurgeFP = stdout;
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
static int CacheFile(char const *fname)
{
    int r;
    CachedFile *cf;
    CachedLine *cl;
    char const *s;

    if (DebugFlag & DB_TRACE_FILES) {
	fprintf(ErrFp, "Caching file `%s' in memory\n", fname);
    }
    cl = NULL;
/* Create a file header */
    cf = NEW(CachedFile);
    cf->cache = NULL;
    if (!cf) {
	ShouldCache = 0;
	FCLOSE(fp);
	return E_NO_MEM;
    }
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
	while (isempty(*s)) s++;
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
/*  NextChainedFile - move to the next chained file in a glob  */
/*  list.                                                      */
/*                                                             */
/***************************************************************/
static int NextChainedFile(IncludeStruct *i)
{
    while(i->chain) {
	FilenameChain *cur = i->chain;
	i->chain = i->chain->next;
	if (OpenFile(cur->filename) == OK) {
	    return OK;
	} else {
	    Eprint("%s: %s", ErrMsg[E_CANT_OPEN], cur->filename);
	}
    }
    return E_EOF;
}

/***************************************************************/
/*                                                             */
/*  PopFile - we've reached the end.  Pop up to the previous   */
/*  file, or return E_EOF                                      */
/*                                                             */
/***************************************************************/
static int PopFile(void)
{
    IncludeStruct *i;

    /* Assume we own the file for now */
    RunDisabled &= ~RUN_NOTOWNER;

    if (!Hush && NumIfs) Eprint("%s", ErrMsg[E_MISS_ENDIF]);
    if (!IStackPtr) return E_EOF;
    i = &IStack[IStackPtr-1];

    if (i->chain) {
	int oldRunDisabled = RunDisabled;
	if (NextChainedFile(i) == OK) {
	    return OK;
	}
	RunDisabled = oldRunDisabled;
    }

    if (IStackPtr <= 1) {
	return E_EOF;
    }

    IStackPtr--;

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
	    if (PurgeMode) OpenPurgeFile(i->filename, "a");
	} else {
	    fp = stdin;
	    if (PurgeMode) PurgeFP = stdout;
	}
	if (fp != stdin)
	    (void) fseek(fp, i->offset, 0);  /* Trust that it works... */
    }
    free((char *) i->filename);
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

#ifdef HAVE_GLOB
static int SetupGlobChain(char const *dirname, IncludeStruct *i)
{
    DynamicBuffer pattern;
    char *dir;
    size_t l;
    int r;
    glob_t glob_buf;
    DirectoryFilenameChain *dc = CachedDirectoryChains;

    i->chain = NULL;
    if (!*dirname) return E_CANT_OPEN;

    dir = StrDup(dirname);
    if (!dir) return E_NO_MEM;

    /* Strip trailing slashes off directory */
    l = strlen(dir);
    while(l) {
	if (*(dir+l-1) == '/') {
	    l--;
	    *(dir+l) = 0;
	} else {
	    break;
	}
    }

    /* Repair root directory :-) */
    if (!l) {
	*dir = '/';
    }

    /* Check the cache */
    while(dc) {
	if (!strcmp(dc->dirname, dir)) {
	    if (DebugFlag & DB_TRACE_FILES) {
		fprintf(ErrFp, "Found cached directory listing for `%s'\n",
			dir);
	    }
	    free(dir);
	    i->chain = dc->chain;
	    return OK;
	}
	dc = dc->next;
    }

    if (DebugFlag & DB_TRACE_FILES) {
	fprintf(ErrFp, "Scanning directory `%s' for *.rem files\n", dir);
    }

    if (ShouldCache) {
	dc = malloc(sizeof(DirectoryFilenameChain));
	if (dc) {
	    dc->dirname = StrDup(dir);
	    if (!dc->dirname) {
		free(dc);
		dc = NULL;
	    }
	}
	if (dc) {
	    if (DebugFlag & DB_TRACE_FILES) {
		fprintf(ErrFp, "Caching directory `%s' listing\n", dir);
	    }

	    dc->chain = NULL;
	    dc->next = CachedDirectoryChains;
	    CachedDirectoryChains = dc;
	}
    }

    DBufInit(&pattern);
    DBufPuts(&pattern, dir);
    DBufPuts(&pattern, "/*.rem");
    free(dir);

    r = glob(DBufValue(&pattern), 0, NULL, &glob_buf);
    DBufFree(&pattern);

    if (r == GLOB_NOMATCH) {
	globfree(&glob_buf);
	return OK;
    }

    if (r != 0) {
	globfree(&glob_buf);
	return -1;
    }

    /* Add the files to the chain backwards to preserve sort order */
    for (r=glob_buf.gl_pathc-1; r>=0; r--) {
	FilenameChain *ch = malloc(sizeof(FilenameChain));
	if (!ch) {
	    globfree(&glob_buf);
	    FreeChain(i->chain);
	    i->chain = NULL;
	    return E_NO_MEM;
	}

	/* TODO: stat the file and only add if it's a plain file and
	   readable by us */
	ch->filename = StrDup(glob_buf.gl_pathv[r]);
	if (!ch->filename) {
	    globfree(&glob_buf);
	    FreeChain(i->chain);
	    i->chain = NULL;
	    free(ch);
	    return E_NO_MEM;
	}
	ch->next = i->chain;
	i->chain = ch;
    }
    if (dc) {
	dc->chain = i->chain;
    }

    globfree(&glob_buf);
    return OK;
}
#endif

/***************************************************************/
/*                                                             */
/*  IncludeFile                                                */
/*                                                             */
/*  Process the INCLUDE command - actually do the file         */
/*  inclusion.                                                 */
/*                                                             */
/***************************************************************/
int IncludeFile(char const *fname)
{
    IncludeStruct *i;
    int oldRunDisabled;
    struct stat statbuf;

    FreshLine = 1;
    if (IStackPtr+1 >= INCLUDE_NEST) return E_NESTED_INCLUDE;
    i = &IStack[IStackPtr];

    if (FileName) {
	i->filename = StrDup(FileName);
	if (!i->filename) return E_NO_MEM;
    } else {
	i->filename = NULL;
    }
    i->LineNo = LineNo;
    i->NumIfs = NumIfs;
    i->IfFlags = IfFlags;
    i->CLine = CLine;
    i->offset = -1L;
    i->chain = NULL;
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

#ifdef HAVE_GLOB
    /* If it's a directory, set up the glob chain here. */
    if (stat(fname, &statbuf) == 0) {
	FilenameChain *fc;
	if (S_ISDIR(statbuf.st_mode)) {
	    if (SetupGlobChain(fname, i) == OK) { /* Glob succeeded */
		if (!i->chain) { /* Oops... no matching files */
		    if (!Hush) {
			Eprint("%s: %s", fname, ErrMsg[E_NO_MATCHING_REMS]);
		    }
		    PopFile();
		    return E_NO_MATCHING_REMS;
		}
		while(i->chain) {
		    fc = i->chain;
		    i->chain = i->chain->next;

		    /* Munch first file */
		    oldRunDisabled = RunDisabled;
		    if (!OpenFile(fc->filename)) {
			return OK;
		    }
		    Eprint("%s: %s", ErrMsg[E_CANT_OPEN], fc->filename);
		    RunDisabled = oldRunDisabled;
		}
		/* Couldn't open anything... bail */
		return PopFile();
	    } else {
		if (!Hush) {
		    Eprint("%s: %s", fname, ErrMsg[E_NO_MATCHING_REMS]);
		}
	    }
	    return E_NO_MATCHING_REMS;
	}
    }
#endif

    oldRunDisabled = RunDisabled;
    /* Try to open the new file */
    if (!OpenFile(fname)) {
	return OK;
    }
    RunDisabled = oldRunDisabled;
    Eprint("%s: %s", ErrMsg[E_CANT_OPEN], fname);
    /* Ugh!  We failed!  */
    PopFile();
    return E_CANT_OPEN;
}

/***************************************************************/
/*                                                             */
/* GetAccessDate - get the access date of a file.              */
/*                                                             */
/***************************************************************/
int GetAccessDate(char const *file)
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
    if (cf->filename) free((char *) cf->filename);
    cl = cf->cache;
    while (cl) {
	if (cl->text) free ((char *) cl->text);
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
    return IStackPtr <= 1;
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
