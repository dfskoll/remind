# Top-level Makefile for Remind.

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
# DO NOT DELETE
