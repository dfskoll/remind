# Top-level Makefile for Remind.

all: src/Makefile
	@echo ""
	@echo "*******************"
	@echo "*                 *"
	@echo "* Building REMIND *"
	@echo "*                 *"
	@echo "*******************"
	@echo ""
	@cd src && $(MAKE) all LANGDEF=$(LANGDEF)
	@$(MAKE) -C rem2pdf -f Makefile.top
install:
	@echo ""
	@echo "*********************"
	@echo "*                   *"
	@echo "* Installing REMIND *"
	@echo "*                   *"
	@echo "*********************"
	@echo ""
	@$(MAKE) -C src install
	@$(MAKE) -C rem2html install
	@$(MAKE) -C rem2pdf -f Makefile.top install
clean:
	find . -name '*~' -exec rm {} \;
	-$(MAKE) -C src clean
	-$(MAKE) -C rem2pdf clean

test:
	@$(MAKE) -C src -s test

distclean: clean
	rm -f config.cache config.log config.status src/Makefile src/config.h tests/test.out www/Makefile rem2pdf/Makefile.top rem2pdf/Makefile.old rem2pdf/Makefile rem2pdf/Makefile.PL rem2pdf/bin/rem2pdf

src/Makefile: src/Makefile.in
	./configure

# DO NOT DELETE

