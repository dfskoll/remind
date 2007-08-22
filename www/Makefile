# Makefile for installing WWW server calendar scripts
# This file is part of REMIND.
# Copyright (C) 1992-1997 by David F. Skoll
# Copyright (C) 1999 by Roaring Penguin Software Inc.

# The complete name of your www host.  Example: www.mycompany.com
WWWHOST = magnesium.roaringpenguin.com
# WWWHOST = www.mycompany.com
 
# The complete path to where the scripts actually live, as seen by
# the UNIX operating system.
SCRIPTDIR = /var/www/cgi-bin
# SCRIPTDIR = /home/dfs/Remind/www/TEST

# Where the scripts live as seen by the web browser.  If
# they live in WWWHOST/cgi-bin, supply /cgi-bin
CGIDIR = /cgi-bin

# The complete path to the directory containing the HTML file "calendar.html".
#  This is a sample file containing links to all the scripts.  This path
# should be the path as seen by the UNIX operating system
HTMLDIR = /var/www/html/remind

# Where you stick images, as seen by UNIX
IMAGEDIR = /var/www/html/remind

# Where images are, as seen by web browers
IMAGEBASE = /remind

# Location.  Typically, the name of your city or town.  Example: Ottawa
LOCATION = Ottawa
# LOCATION = New York

# Where do Remind and Rem2PS executables live?
REMIND = /usr/local/bin/remind
REM2PS = /usr/local/bin/rem2ps

# If your Web server requires CGI programs to have a .cgi suffix, use
# the next line.  Otherwise, comment it out
CGISUFFIX=.cgi

# Don't change stuff below here.
# --------------------------------------------------------------------

# Construct a rotten mean nasty sed script to do the dirty work
SEDSCRIPT = -e 's/Ottawa/$(LOCATION)/g' \
	    -e 's@http://your.machine/your.dir@http://$(WWWHOST)$(CGIDIR)@g' \
	    -e 's@%SCRIPTDIR%@$(SCRIPTDIR)@g' \
	    -e 's@%REMIND%@$(REMIND)@g' \
	    -e 's@%IMAGEBASE%@$(IMAGEBASE)@g' \
	    -e 's@%REM2PS%@$(REM2PS)@g' \
	    -e 's@cal_dispatch@cal_dispatch$(CGISUFFIX)@g' \
	    -e 's@rem2html@rem2html$(CGISUFFIX)@g'

SEDSCRIPT2 = -e 's/Ottawa/$(LOCATION)/g' \
	    -e 's@http://your.machine/your.dir@http://$(WWWHOST)$(CGIDIR)@g' \
	    -e 's@%SCRIPTDIR%@$(SCRIPTDIR)@g' \
	    -e 's@%REMIND%@$(REMIND)@g' \
	    -e 's@%IMAGEBASE%@$(IMAGEBASE)@g' \
	    -e 's@%REM2PS%@$(REM2PS)@g'

all:
	@echo "Edit the Makefile; then type 'make install' to install"
	@echo "the www server scripts."

# OK, it's abominable.  But it works...
install:
	-mkdir $(SCRIPTDIR)
	cp calps hebdate hebps hebhtml moon sunrise sunset $(SCRIPTDIR)
	sed $(SEDSCRIPT) < cal_dispatch-DIST  > $(SCRIPTDIR)/cal_dispatch$(CGISUFFIX)
	sed $(SEDSCRIPT)  < hebdate.rem-DIST   > $(SCRIPTDIR)/hebdate.rem
	sed $(SEDSCRIPT)  < moon.rem-DIST      > $(SCRIPTDIR)/moon.rem
	sed $(SEDSCRIPT)  < sunrise.rem-DIST   > $(SCRIPTDIR)/sunrise.rem
	sed $(SEDSCRIPT)  < sunset.rem-DIST    > $(SCRIPTDIR)/sunset.rem
	sed $(SEDSCRIPT)  < calendar.html-DIST > $(HTMLDIR)/calendar.html
	sed $(SEDSCRIPT)  < hebhtml            > $(SCRIPTDIR)/hebhtml
	sed $(SEDSCRIPT2) < rem2html           > $(SCRIPTDIR)/rem2html$(CGISUFFIX)
	chmod 644 $(SCRIPTDIR)/sunrise.rem
	chmod 644 $(SCRIPTDIR)/moon.rem
	chmod 644 $(SCRIPTDIR)/hebdate.rem
	chmod 644 $(SCRIPTDIR)/sunset.rem
	chmod 644 $(HTMLDIR)/calendar.html
	chmod 755 $(SCRIPTDIR)/cal_dispatch$(CGISUFFIX)
	chmod 755 $(SCRIPTDIR)/rem2html$(CGISUFFIX)
	chmod 755 $(SCRIPTDIR)/calps $(SCRIPTDIR)/hebdate \
	$(SCRIPTDIR)/hebps $(SCRIPTDIR)/moon $(SCRIPTDIR)/sunrise \
	$(SCRIPTDIR)/sunset $(SCRIPTDIR)/hebhtml $(SCRIPTDIR)/rem2html$(CGISUFFIX)

	-mkdir $(IMAGEDIR)
	cp firstquarter.png fullmoon.png lastquarter.png newmoon.png $(IMAGEDIR)
	chmod 644 $(IMAGEDIR)/firstquarter.png $(IMAGEDIR)/fullmoon.png $(IMAGEDIR)/lastquarter.png $(IMAGEDIR)/newmoon.png




