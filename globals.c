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
/*  Copyright (C) 1992-1997 by David F. Skoll                  */
/*                                                             */
/***************************************************************/

static char const RCSID[] = "$Id: globals.c,v 1.2 1997-01-16 04:14:24 dfs Exp $";

#include "config.h"
#include <stdio.h>   /* For defintion of FILE - sigh! */
#include "types.h"
#define MK_GLOBALS
#include "globals.h"
#include "err.h"
