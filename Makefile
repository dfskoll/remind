# Top-level Makefile for Remind.

# $Id: Makefile,v 1.24 1998-01-17 04:51:52 dfs Exp $

all:
	@if test ! -f src/Makefile ; then \
		./configure ; \
	fi
	@echo ""
	@echo "*******************"
	@echo "*                 *"
	@echo "* Building REMIND *"
	@echo "*                 *"
	@echo "*******************"
	@echo ""
	@cd src; $(MAKE) all

install: all
	@echo ""
	@echo "*********************"
	@echo "*                   *"
	@echo "* Installing REMIND *"
	@echo "*                   *"
	@echo "*********************"
	@echo ""
	cd src; $(MAKE) install

clean:
	find . -name '*~' -exec rm {} \;
	cd src; $(MAKE) clean
