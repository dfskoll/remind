#!/bin/sh
# Shell script to mail all users reminders.

# This file is part of REMIND
#
# REMIND is Copyright (C) 1992-1998 by David F. Skoll
# Copyright (C) 1999-2007 Roaring Penguin Software Inc.
# This file is Copyright (C) 1990 by Bill Aten

# Thanks to Bill Aten for this script.

# Run it AFTER MIDNIGHT so that date is correct!
# On our system, we have the following in our crontab:
# 02 00 * * * /usr/local/adm/remind-all >/dev/null 2>&1

# This script must be run by root.  The -u option MUST be supplied
# to Remind, or a severe security hole will exist.  Note that Remind
# must be compiled to support the -u option for this script to work.
# Also, the -r and -q options must be used.

# The following line gets a list of users for systems using SUN's
# NIS service:
# USERS=`ypcat passwd | awk -F: '{print $1}'`

# The following line gets a list of users by examining /etc/passwd:
USERS=`awk -F: '{print $1}' /etc/passwd`

# If neither of the above methods works, you must come up with some
# way of getting a list of users on the system

# Set the following variables as appropriate for your system
REMIND=/usr/local/bin/remind
MAIL=/usr/bin/mail
RM="/bin/rm -f"

REMFILE=/tmp/RemFile.$$

# Scan each user's directory for a .reminders file
for i in $USERS
do
HOME=`grep \^$i: /etc/passwd | awk -F: '{print $6}'`
   if [ -r $HOME/.reminders ]; then

#     echo "$i has a .reminders file."     DEBUGGING PURPOSES ONLY

      $REMIND -u$i -h -r -q -iremind_all=1 $HOME/.reminders < /dev/null > $REMFILE
      if [ -s $REMFILE ]; then
#        echo "Sending mail to $i"         DEBUGGING PURPOSES ONLY
         $MAIL -s "Reminders" $i < $REMFILE
      fi
      $RM $REMFILE
   fi
done
