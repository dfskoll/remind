# Makefile for installing WWW server calendar scripts
# This file is part of REMIND.
# Copyright (C) 1992-1997 by David F. Skoll

# $Id: Makefile,v 1.1 1997-01-16 04:00:26 dfs Exp $

# The complete name of your www host.  Example: www.mycompany.com
WWWHOST = www.skoll.ca
# WWWHOST = www.mycompany.com
 
# The complete path to where the scripts actually live.
SCRIPTDIR = /var/lib/httpd/cgi-bin
# SCRIPTDIR = /home/dfs/Remind/www/TEST

# Where the scripts live as seen from the outside world.  If
# they live in WWWHOST/cgi-bin, supply /cgi-bin
CGIDIR = /cgi-bin

# Location.  Typically, a city name.  Example: Ottawa
LOCATION = Ottawa
# LOCATION = New York

# Where do Remind and Rem2PS executables live?
REMIND = /usr/local/bin/remind
REM2PS = /usr/local/bin/rem2ps

# Don't change stuff below here.
# --------------------------------------------------------------------
all:
	@echo "Edit the Makefile; then type 'make install' to install"
	@echo "the www server scripts."

# OK, it's abominable.  But it works...
install:
	cp calps hebdate hebps moon sunrise sunset $(SCRIPTDIR)
	chmod 755 $(SCRIPTDIR)/calps $(SCRIPTDIR)/hebdate $(SCRIPTDIR)/hebps $(SCRIPTDIR)/moon $(SCRIPTDIR)/sunrise $(SCRIPTDIR)/sunset
	sed -e 's/Ottawa/$(LOCATION)/g' \
	    -e 's@http://your.machine/your.dir@http://$(WWWHOST)$(CGIDIR)@g' \
	    -e 's@%SCRIPTDIR%@$(SCRIPTDIR)@g' \
	    -e 's@%REMIND%@$(REMIND)@g' \
	    -e 's@%REM2PS%@$(REM2PS)@g' < cal_dispatch-DIST > $(SCRIPTDIR)/cal_dispatch
	chmod 755 $(SCRIPTDIR)/cal_dispatch
	sed -e 's/Ottawa/$(LOCATION)/g' \
	    -e 's@http://your.machine/your.dir@http://$(WWWHOST)$(CGIDIR)@g' \
	    -e 's@%SCRIPTDIR%@$(SCRIPTDIR)@g' \
	    -e 's@%REMIND%@$(REMIND)@g' \
	    -e 's@%REM2PS%@$(REM2PS)@g' < hebdate.rem-DIST > $(SCRIPTDIR)/hebdate.rem
	chmod 644 $(SCRIPTDIR)/hebdate.rem
	sed -e 's/Ottawa/$(LOCATION)/g' \
	    -e 's@http://your.machine/your.dir@http://$(WWWHOST)$(CGIDIR)@g' \
	    -e 's@%SCRIPTDIR%@$(SCRIPTDIR)@g' \
	    -e 's@%REMIND%@$(REMIND)@g' \
	    -e 's@%REM2PS%@$(REM2PS)@g' < moon.rem-DIST > $(SCRIPTDIR)/moon.rem
	chmod 644 $(SCRIPTDIR)/moon.rem
	sed -e 's/Ottawa/$(LOCATION)/g' \
	    -e 's@http://your.machine/your.dir@http://$(WWWHOST)$(CGIDIR)@g' \
	    -e 's@%SCRIPTDIR%@$(SCRIPTDIR)@g' \
	    -e 's@%REMIND%@$(REMIND)@g' \
	    -e 's@%REM2PS%@$(REM2PS)@g' < sunrise.rem-DIST > $(SCRIPTDIR)/sunrise.rem
	chmod 644 $(SCRIPTDIR)/sunrise.rem
	sed -e 's/Ottawa/$(LOCATION)/g' \
	    -e 's@http://your.machine/your.dir@http://$(WWWHOST)$(CGIDIR)@g' \
	    -e 's@%SCRIPTDIR%@$(SCRIPTDIR)@g' \
	    -e 's@%REMIND%@$(REMIND)@g' \
	    -e 's@%REM2PS%@$(REM2PS)@g' < sunset.rem-DIST > $(SCRIPTDIR)/sunset.rem
	chmod 644 $(SCRIPTDIR)/sunset.rem
