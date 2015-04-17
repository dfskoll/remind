#!/bin/sh
# -*-Mode: TCL;-*-

#--------------------------------------------------------------
#   cm2rem.tcl
#
#   A cheesy Tcl script to convert Sun's "cm" calendar manager
#   files (version 3 only) to Remind format.
#
#   This file is part of REMIND.
#   Copyright (C) 1992-1998 by Dianne Skoll
#   Copyright (C) 1999-2000 by Roaring Penguin Software Inc.
#
#--------------------------------------------------------------

# the next line restarts using tclsh \
exec tclsh "$0" "$@"

set i 0
foreach month {January February March April May June
    July August September October November December} {
    incr i
    set MonthNum($month) $i
    set FullMonth([string range $month 0 2]) $month
}

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
    global Attributes
    global FullMonth

    catch {unset Attributes}

    # Only convert lines which start with "(add"
    if {[string range $line 0 3] != "(add"} {
	return
    }
    set line [convertParens $line]
    # Convert it to a list.  CAREFUL:  Potential security problem if
    # $line contains something nasty.

    eval set line $line

    set Attributes(body) ""
    foreach {key val} $line {
	switch -exact -- $key {
	    "add" {
		set Attributes(date) $val
	    }
	    "what:" {
		append Attributes(body) $val
	    }
	    "details:" {
		append Attributes(body) $val
	    }
	    "duration:" {
		set Attributes(duration) $val
	    }
	    "period:" {
		set Attributes(period) $val
	    }
	    "ntimes:" {
		set Attributes(ntimes) $val
	    }
	    "attributes:" {
		set Attributes(action) $val
	    }
		
	}
    }

    if {[info exists Attributes(action)]} {
	# Nuke quotes and commas in action
	regsub -all {[,\"]} $Attributes(action) { } Attributes(action)

	# Add spaces to pairs
	regsub -all \}\{  $Attributes(action) \}\ \{ Attributes(action)

	# Add another pair of brackets to make a proper list
	set Attributes(action) "{$Attributes(action)}"

	# Convert to a real Tcl list
	eval set Attributes(action) $Attributes(action)
    }
    # Split out date into month, day, year, time parts
    scan $Attributes(date) "%s%s%s%s%s" wkday month day time year
    set time [string range $time 0 4]
    set Attributes(wkday) $wkday
    set Attributes(month) $FullMonth($month)
    set Attributes(day) $day
    set Attributes(time) $time
    set Attributes(year) $year

    # Convert newlines in body to spaces
    set body $Attributes(body)
    regsub -all "\n" $body " " body

    # TODO: Escape BODY to get rid of [] chars.
    set Attributes(body) $body

    # Convert to Reminder format
    convertReminder
}

#***********************************************************************
# %PROCEDURE: convertReminder
# %ARGUMENTS:
#  None -- uses global Attributes variable which must be filled in
# %RETURNS:
#  Nothing
# %DESCRIPTION:
#  Converts a reminder to Remind format.
#***********************************************************************
proc convertReminder {} {
    global Attributes
    switch -exact $Attributes(period) {
	single { convertSingleReminder }
	daily  { convertDailyReminder  }
	weekly { convertWeeklyReminder }
	monthly { convertMonthlyReminder }
	yearly { convertYearlyReminder }
	default {
	    puts "\# Unable to convert reminder with period $Attributes(period)"
	    puts "\# Body is: $Attributes(body)"
	}
    }
}

#***********************************************************************
# %PROCEDURE: convertSingleReminder
# %ARGUMENTS:
#  None -- uses global Attributes variable which must be filled in
# %RETURNS:
#  Nothing
# %DESCRIPTION:
#  Converts a reminder with "single" period to Remind format.
#***********************************************************************
proc convertSingleReminder {} {
    global Attributes
    puts "REM $Attributes(day) $Attributes(month) $Attributes(year) [at][duration]MSG $Attributes(body)"
}

#***********************************************************************
# %PROCEDURE: convertDailyReminder
# %ARGUMENTS:
#  None -- uses global Attributes variable which must be filled in
# %RETURNS:
#  Nothing
# %DESCRIPTION:
#  Converts a reminder with "daily" period to Remind format.
#***********************************************************************
proc convertDailyReminder {} {
    global Attributes
    set ntimes [expr $Attributes(ntimes) - 1]
    if {$ntimes <= 1} {
	convertSingleReminder
	return
    }
    set until [getUntilDate $Attributes(day) $Attributes(month) $Attributes(year) $ntimes]

    puts "REM $Attributes(day) $Attributes(month) $Attributes(year) *1 [at][duration]UNTIL $until MSG $Attributes(body)"
}

#***********************************************************************
# %PROCEDURE: convertWeeklyReminder
# %ARGUMENTS:
#  None -- uses global Attributes variable which must be filled in
# %RETURNS:
#  Nothing
# %DESCRIPTION:
#  Converts a reminder with "daily" period to Remind format.
#***********************************************************************
proc convertWeeklyReminder {} {
    global Attributes
    set ntimes [expr $Attributes(ntimes) - 1]
    if {$ntimes <= 1} {
	convertSingleReminder
	return
    }
    set until [getUntilDate $Attributes(day) $Attributes(month) $Attributes(year) [expr $ntimes * 7]]

    puts "REM $Attributes(day) $Attributes(month) $Attributes(year) *7 [at][duration]UNTIL $until MSG $Attributes(body)"
}

#***********************************************************************
# %PROCEDURE: convertMonthlyReminder
# %ARGUMENTS:
#  None -- uses global Attributes variable which must be filled in
# %RETURNS:
#  Nothing
# %DESCRIPTION:
#  Converts a reminder with "monthly" period to Remind format.
#***********************************************************************
proc convertMonthlyReminder {} {
    global Attributes
    set ntimes [expr $Attributes(ntimes) - 1]
    if {$ntimes <= 1} {
	convertSingleReminder
	return
    }

    # If repetition > 1000, it's infinite
    if {$ntimes > 1000} {
	puts "REM $Attributes(day) [at][duration]MSG $Attributes(body)"
	return
    }

    ### UNTIL date is fudged!
    set until [getUntilDate $Attributes(day) $Attributes(month) $Attributes(year) [expr $ntimes * 30]]

    puts "REM $Attributes(day) [at][duration]UNTIL $until MSG $Attributes(body)"
}

#***********************************************************************
# %PROCEDURE: convertYearlyReminder
# %ARGUMENTS:
#  None -- uses global Attributes variable which must be filled in
# %RETURNS:
#  Nothing
# %DESCRIPTION:
#  Converts a reminder with "yearly" period to Remind format.
#***********************************************************************
proc convertYearlyReminder {} {
    global Attributes

    # No special handling of ntimes et al.
    puts "REM $Attributes(day) $Attributes(month) [at][duration]MSG $Attributes(body)"

}

#***********************************************************************
# %PROCEDURE: at
# %ARGUMENTS:
#  None -- uses Attributes global variable
# %RETURNS:
#  A string providing the correct AT clause for a timed reminder.
#***********************************************************************
proc at {} {
    global Attributes
    if {![info exists Attributes(time)]} {
	return ""
    }
    if {"$Attributes(time)" == ""} {
	return ""
    }

    return "AT $Attributes(time) "
}

#***********************************************************************
# %PROCEDURE: duration
# %ARGUMENTS:
#  None -- uses Attributes global variable
# %RETURNS:
#  A string providing the correct DURATION clause for a timed reminder.
#***********************************************************************
proc duration {} {
    global Attributes
    if {![info exists Attributes(duration)]} {
	return ""
    }
    if {"$Attributes(duration)" == ""} {
	return ""
    }
    set h [expr $Attributes(duration) / 3600]
    set remainder [expr $Attributes(duration) - $h*3600]
    set m [expr $remainder / 60]
    return "DURATION [format "%d:%02d " $h $m]"
}

#***********************************************************************
# %PROCEDURE: getUntilDate
# %ARGUMENTS:
#  day, month, year -- a date
#  days -- number of days to add to date
# %RETURNS:
#  The date which is "days" later than supplied date in a correct UNTIL
#  format.
#***********************************************************************
proc getUntilDate { day month year days } {
    global RemindPipe
    global MonthNum
    set date "'$year/$MonthNum($month)/$day'"
    puts $RemindPipe "MSG \[trigger($date + $days)\]%"
    puts $RemindPipe "flush"
    flush $RemindPipe
    gets $RemindPipe line
    return $line
}

catch {wm withdraw .}
# Start a Remind process to issue reminders
if {[catch {set RemindPipe [open "|remind -" "r+"]} err]} {
    puts stderr "Error: Cannot run Remind: $err"
    exit 1
}

puts $RemindPipe "banner %"
flush $RemindPipe

# Write some blurb
puts "\# Reminder file converted from \"cm\" data by cm2rem.tcl"
puts ""

while {[gets stdin line] >= 0} {
    processLine $line
}
exit 0
