#!/bin/csh -f

# Shell script to mail all users reminders.

# $Id: remind-all.csh,v 1.1 1998-01-15 02:50:21 dfs Exp $

# Run it AFTER MIDNIGHT so that date is correct!
# On our system, we have the following in our crontab:
# 05 5 * * * /usr/share/lib/remind/remind-all > /dev/null 2>&1

# This script must be run by root.  The -u option MUST be supplied
# to Remind, or a severe security hole will exist.  Note that Remind
# must be compiled to support the -u option for this script to work.
# Also, the -r and -q options must be used.

# The following line gets a list of users for systems using SUN's
# NIS service:
set USERS  = `ypcat passwd | awk -F: '{print $1}'`

# The following line gets a list of users by examining /etc/passwd:
# set USERS = `awk -F: '{print $1}' /etc/passwd`

# If neither of the above methods works, you must come up with some
# way of getting a list of users on the system

# Set the following variables as appropriate for your system
set REMIND = /usr/local/bin/remind
set MAIL   = /usr/ucb/mail
set RM     = "/usr/bin/rm -f"

set REMFILE   = /tmp/RemFile.$$

# Scan each user's directory for a .reminders file
foreach i ($USERS)
   if (-r ~$i/.reminders) then
#     echo "$i has a .reminders file."     DEBUGGING PURPOSES ONLY

      $REMIND -u$i -h -r -q -iremind_all=1 ~$i/.reminders < /dev/null > $REMFILE
      if (! -z $REMFILE) then
#        echo "Sending mail to $i"         DEBUGGING PURPOSES ONLY

         $MAIL -s "Reminders" $i < $REMFILE
      endif
      $RM $REMFILE
   endif
end
