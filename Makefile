# Top-level Makefile for Remind.

# $Id: Makefile,v 1.26 1998-01-19 03:07:09 dfs Exp $

all: src/Makefile
	@echo ""
	@echo "*******************"
	@echo "*                 *"
	@echo "* Building REMIND *"
	@echo "*                 *"
	@echo "*******************"
	@echo ""
	@cd src; $(MAKE) all LANGDEF=$(LANGDEF)

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


src/Makefile: src/Makefile.in
	./configure
