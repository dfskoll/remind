#!/bin/sh
# -*-Mode: TCL;-*-

#--------------------------------------------------------------
#   cm2rem.tcl
#
#   A cheesy Tcl script to convert Sun's "cm" calendar manager
#   files (version 3 only) to Remind format.
#
#   This file is part of REMIND.
#   Copyright (C) 1992-1998 by David F. Skoll
#   Copyright (C) 1999 by Roaring Penguin Software Inc.
#
#--------------------------------------------------------------

# $Id: cm2rem.tcl,v 1.1 1999-08-23 18:13:56 dfs Exp $

# the next line restarts using tclsh \
exec tclsh "$0" "$@"

#***********************************************************************
# %PROCEDURE: convertParens
# %ARGUMENTS:
#  line -- a line read from a cm file
# %RETURNS:
#  A new line with all ( and ) outside quotes converted to { and }.
#  This cheap trick allows us to use Tcl's built-in list manipulation
#  functions to munge the line.
#***********************************************************************
proc convertParens { line } {
    # Convert all ( and ) to { and } unless they are inside a quoted
    # string
    set out ""
    set len [string length $line]
    set inQuotes 0
    for {set i 0} {$i < $len} {incr i} {
	set char [string range $line $i $i]
	if {$char == "\\" && $inQuotes} {
	    append out $char
	    incr i
	    set char [string range $line $i $i]
	    append out $char
	    continue
	}

	if {$char == "(" && !$inQuotes} {
	    set char \{
	}

	if {$char == ")" && !$inQuotes} {
	    set char \}
	}

	if {$char == "\""} {
	    set inQuotes [expr !$inQuotes]
	}

	append out $char
    }
    return $out
}
#***********************************************************************
# %PROCEDURE: processLine
# %ARGUMENTS:
#  line -- a line read from a cm file
# %RETURNS:
#  Nothing
# %DESCRIPTION:
#  Processes a single line from the file, possibly writing a reminder
#  in Remind format to stdout
#***********************************************************************
proc processLine { line } {
    # Only convert lines which start with "(add"
    if {[string range $line 0 3] != "(add"} {
	return
    }
    set line [convertParens $line]

    # Convert it to a list.  CAREFUL:  Potential security problem if
    # $line contains something nasty.

    eval set line $line

    set i 0
    foreach thing $line {
	incr i
	puts "Item $i: $thing"
    }
    
}


while {[gets stdin line] >= 0} {
    processLine $line
}
exit 0
