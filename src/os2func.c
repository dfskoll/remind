/***************************************************************/
/*                                                             */
/*  OS2FUNC.C                                                  */
/*                                                             */
/*  Functions to support OS/2.                                 */
/*                                                             */
/*  This file is part of REMIND.                               */
/*                                                             */
/*  This file is Copyright (C) 1993 by Russ Herman.            */
/*  REMIND is Copyright (C) 1992-1998 by David F. Skoll        */
/*                                                             */
/***************************************************************/

#include "config.h"
static char const RCSID[] = "$Id: os2func.c,v 1.2 1998-02-10 03:15:53 dfs Exp $";

#ifdef OS2_POPUP
#define INCL_VIO
#define INCL_KBD
#endif

#ifdef _MSC_VER
#define INCL_DOSPROCESS
#endif

#if defined(OS2_POPUP) || defined(_MSC_VER)
#include <os2.h>
#endif

#ifdef OS2_POPUP
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef OS2DBG
#include <dos.h>
#include <stdlib.h>
#endif
#include "globals.h"

/* EMX defines PS_TYPE, so we undefine it here to avoid
   a redefinition warning when we include "types.h" */
#ifdef PS_TYPE
#undef PS_TYPE
#endif

#include "types.h"
#include "protos.h"

#ifdef _MSC_VER
typedef USHORT APIRET;
#endif

static APIRET apiret = 0;
static KBDKEYINFO kbci;
static char *pszPressAny = "\r\nPress any key to continue";
static USHORT pflags = VP_WAIT;  /* | VP_TRANSPARENT; */
static HKBD hkbd = 0;
static char VioSubstBuffer[SHELLSIZE + 1];

void StartPopUp()
{
    if (OS2MODE)
	if (!(DebugFlag & DB_ECHO_LINE))
	    VioPopUp(&pflags, 0);
}

void EndPopUp()
{
    if (DebugFlag & DB_ECHO_LINE)
	return;
    if (OS2MODE) {
	VioWrtTTY(pszPressAny, strlen(pszPressAny), 0);
	KbdCharIn(&kbci, IO_WAIT, hkbd);
	VioEndPopUp(0);
    }
}

int PutsPopUp(char *s)
{
    char c, *os = VioSubstBuffer;

    if (DebugFlag & DB_ECHO_LINE)
	printf("%s", s);
    else {
	do {
	    /* Convert \n to \r\n in auxiliary buffer for VIO */
	    if ((c= *s++) == '\n')
		*os++ = '\r';
	    *os++ = c;
	} while (c > 0);
	VioWrtTTY(VioSubstBuffer, strlen(VioSubstBuffer), 0);
    }
    return(0);
}

int PutlPopUp(char *s)
{
    StartPopUp();
    PutsPopUp(s);
    if (DebugFlag & DB_ECHO_LINE)
	fputc('\n', stdout);
    else
	VioWrtTTY("\r\n", 2, 0);
    EndPopUp();
    return(0);
}


int PutcPopUp(int c)
{
    char *s = " ";

    if (DebugFlag & DB_ECHO_LINE)
	fputc(c, stdout);
    else {
	switch (c) {
	case '\n':
	    VioWrtTTY("\r\n", 2, 0);
	    break;
	default:
	    s[0] = c;
	    VioWrtTTY(s, 1, 0);
	    break;
	}
    }
    return(0);
}

#ifdef OS2DBG
#define DB_ECHO_LINE 16
int DebugFlag = 0;
void main(/* int argc, char **argv */)
{
    int ret;

    ret = os2fputs("Test VIO PopUp Writing");
    if (ret)
	fprintf(stderr, "Test VIO PopUP Writing returned %d %ld",
		ret, apiret);
    exit(ret);
}
#endif
#endif

#ifdef _MSC_VER
unsigned sleep(unsigned sec)
{
    return DosSleep(sec * 1000L);
}
#endif

#ifndef __EMX__ 
int fork()
{
    return(-1);
}
#endif

