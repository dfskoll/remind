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
/*  Copyright (C) 1992-2022 by Dianne Skoll                    */
/*                                                             */
/***************************************************************/

#include "config.h"

#include <stdio.h>   /* For definition of FILE - sigh! */
#include "types.h"
#include "custom.h"
#include "lang.h"
#define MK_GLOBALS 1
#include "globals.h"
#include "err.h"
