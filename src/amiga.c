/***************************************************************/
/*                                                             */
/*  AMIGA.C                                                    */
/*                                                             */
/*  Support functions for AmigaDOS                             */
/*                                                             */
/*  This file is Copyright (C) 1995 by Martin Hohl             */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Remind is Copyright (C) 1992-1998 by David F. Skoll        */
/*                                                             */
/***************************************************************/
#include "config.h"
static char const RCSID[] = "$Id: amiga.c,v 1.2 1998-02-10 03:15:46 dfs Exp $";

#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include <proto/dos.h>

#include "version.h"    /* Hopefully this will define VERSION as a string */

#ifdef __SASC_60
/* AmigaDOS 2.04 compatible version string for "version" utility */
const static char ver_string[] = "$VER: remind "VERSION" "__AMIGADATE__;
#endif

void sleep(int dt)
{
    register long Ticks;

    if (dt != 0) {
	Ticks = 50L * dt;
	Delay(Ticks);
    }
}

void execvp(char *name, char **argvec)
{
    char *cmdline;
    int i,l;

    l = strlen(name)+2;
    i=1;
    while (argvec[i] != 0L) {
	l += strlen(argvec[i])+1;
	i++;
    };
    cmdline = malloc(l+1);
    if (cmdline == 0L) return;
    strcpy(cmdline,name);
    i=1;
    while (argvec[i] != 0L) {
	strcat(cmdline," ");
	strcat(cmdline,argvec[i]);
	i++;
    };
    system(cmdline);
    free(cmdline);
}
