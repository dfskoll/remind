/***************************************************************/
/*                                                             */
/*  DYNBUF.H                                                   */
/*                                                             */
/*  Declaration of functions for manipulating dynamic buffers  */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by David F. Skoll                  */
/*                                                             */
/***************************************************************/

/* $Id: dynbuf.h,v 1.1 1998-02-07 05:35:57 dfs Exp $ */

#ifndef DYNBUF_H
#define DYNBUF_H

#include <stdio.h>  /* For FILE */

#define DBUF_STATIC_SIZE 128
typedef struct {
    char *buffer;
    int len;
    int allocatedLen;
    char staticBuf[DBUF_STATIC_SIZE];
} DynamicBuffer;

#ifndef ARGS
#ifdef HAVE_PROTOS
#define ARGS(x) x
#else
#define ARGS(x) ()
#endif
#endif

void DBufInit(DynamicBuffer *dbuf);
int DBufPutc(DynamicBuffer *dbuf, char c);
int DBufPuts(DynamicBuffer *dbuf, char *str);
void DBufFree(DynamicBuffer *dbuf);
int DBufGets(DynamicBuffer *dbuf, FILE *fp);

#define DBufValue(bufPtr) ((bufPtr)->buffer)
#define DBufLen(bufPtr) ((bufPtr)->len)

#endif /* DYNBUF_H */
