# Top-level Makefile for Remind.

# $Id: Makefile,v 1.29 2007-03-26 19:38:07 dfs Exp $

all: src/Makefile
	@echo ""
	@echo "*******************"
	@echo "*                 *"
	@echo "* Building REMIND *"
	@echo "*                 *"
	@echo "*******************"
	@echo ""
	@cd src; $(MAKE) all LANGDEF=$(LANGDEF)

install:
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

distclean: clean
	rm -f config.cache config.log config.status src/Makefile src/config.h tests/test.out

src/Makefile: src/Makefile.in
	./configure
