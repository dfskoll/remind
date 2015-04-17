/***************************************************************/
/*                                                             */
/*  DYNBUF.H                                                   */
/*                                                             */
/*  Declaration of functions for manipulating dynamic buffers  */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by Dianne Skoll                    */
/*  Copyright (C) 1999-2000 by Roaring Penguin Software Inc.   */
/*                                                             */
/***************************************************************/

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
int DBufPutcFN(DynamicBuffer *dbuf, char c);
int DBufPuts(DynamicBuffer *dbuf, char const *str);
void DBufFree(DynamicBuffer *dbuf);
int DBufGets(DynamicBuffer *dbuf, FILE *fp);

#define DBufValue(bufPtr) ((bufPtr)->buffer)
#define DBufLen(bufPtr) ((bufPtr)->len)

#define DBufPutc(dbuf, c) ( (dbuf)->allocatedLen < (dbuf)->len+1 ) ? (dbuf)->buffer[(dbuf)->len++] = c, (dbuf)->buffer[(dbuf)->len] = 0, OK : DBufPutcFN((dbuf), c)

#endif /* DYNBUF_H */
