/***************************************************************/
/*                                                             */
/*  GLOBALS.C                                                  */
/*                                                             */
/*  This file simply instantiates all of the global variables. */
/*                                                             */
/*  It does this by #defining MK_GLOBALS and #including        */
/*  globals.h and err.h                                        */
/*                                                             */
/*  This file is part of REMIND.                               */
/*  Copyright (C) 1992-1998 by David F. Skoll                  */
/*                                                             */
/***************************************************************/

#include "config.h"
static char const RCSID[] = "$Id: globals.c,v 1.2 1998-02-10 03:15:50 dfs Exp $";

#include <stdio.h>   /* For defintion of FILE - sigh! */
#include "types.h"
#define MK_GLOBALS
#include "globals.h"
#include "err.h"
