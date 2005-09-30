/***************************************************************/
/*                                                             */
/*  DYNBUF.H                                                   */
/*                                                             */
/*  Declaration of functions for manipulating dynamic buffers  */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by David F. Skoll                  */
/*  Copyright (C) 1999-2000 by Roaring Penguin Software Inc.   */
/*                                                             */
/***************************************************************/

/* $Id: dynbuf.h,v 1.4 2005-09-30 03:29:32 dfs Exp $ */

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

void DBufInit(DynamicBuffer *dbuf);
int DBufPutc(DynamicBuffer *dbuf, char c);
int DBufPuts(DynamicBuffer *dbuf, char *str);
void DBufFree(DynamicBuffer *dbuf);
int DBufGets(DynamicBuffer *dbuf, FILE *fp);

#define DBufValue(bufPtr) ((bufPtr)->buffer)
#define DBufLen(bufPtr) ((bufPtr)->len)

#endif /* DYNBUF_H */
