# Makefile for REMIND
# $Id: Makefile,v 1.9 1996-05-26 03:17:35 dfs Exp $

#-----------------------------------------------------------------------------
# THINGS FOR YOU TO EDIT START BELOW
#-----------------------------------------------------------------------------

# Uncomment the next line if you are running on a SYSV system
SYSV= -DSYSV

# Uncomment the next line if you are running under UNIX (including SYSV!)
UNIX= -DUNIX

# Uncomment the next lines if you want to use gcc instead of default compiler
# NOTE:  Tempting as it may be, if you use 'cc' for the C compiler, do not
# use 'ld' for the linker.  It will probably work much better if you use
# LD= cc rather than LD= ld.
CC= gcc
LD= gcc

# Put any additional flags for the C compiler or linker here - if you
# are not using gcc, you probably want to remove '-ansi -pedantic -Wall'.
CFLAGS= -O -ansi -pedantic -Wall
CDEFS=
LDFLAGS=

#### INSTALLATION LOCATIONS ####
# Note that I use 'cp' rather than 'install' for improved portability.
#
# BINDIR:  Where should the Remind executable be installed?
BINDIR= /usr/local/bin

# SCRIPTDIR:  Where should the kall and rem shell scripts be installed?
SCRIPTDIR= /usr/local/bin

# MANDIR:  Where should the man pages be installed?
MANDIR= /usr/local/man

# MANSECT:  Which man section should the man pages go into?
MANSECT= 1

# EXEMODE:  What file protection mode should be used for the executables?
EXEMODE= 755

# MANMODE:  What file protection mode should be used for the man pages?
MANMODE= 644

# OWNER, GROUP:  What owner and group to use for executables,
# scripts and man pages?
OWNER=bin
GROUP=bin

#-----------------------------------------------------------------------------
# YOU SHOULDN'T EDIT ANYTHING BELOW HERE.  You may want to change some things
# in config.h; then, you should be able to type 'make'.
#-----------------------------------------------------------------------------
VERSION= 03.00.14
MATHLIB= -lm

HDRS= config.h err.h expr.h globals.h protos.h types.h version.h \
lang.h english.h german.h dutch.h finnish.h french.h norwgian.h \
danish.h polish.h

STDHDRS= config.h types.h protos.h globals.h err.h lang.h

LANGHDRS= english.h german.h dutch.h finnish.h french.h norwgian.h danish.h \
polish.h

SRCS= calendar.c dorem.c dosubst.c expr.c files.c funcs.c globals.c hbcal.c \
init.c main.c moon.c omit.c sort.c queue.c token.c trigger.c userfns.c \
utils.c var.c

MANIFEST=COPYRIGHT Makefile Makefile_QDOS README.AMIGA README.BCC \
README.DOS README.OS2 README.UNIX README_QDOS WHATSNEW.30 \
amiga-SCOPTIONS amiga.c calendar.c config.h danish.h defs.rem dorem.c \
dosubst.c dutch.h english.h err.h expr.c expr.h files.c finnish.h \
french.h funcs.c german.h globals.c globals.h hbcal.c init.c kall \
kall.1 lang.h lnk.bcc lnk.msc lnk.tc main.c makefile.bcc makefile.msc \
makefile.os2 makefile.tc moon.c norwgian.h omit.c os2func.c polish.h \
protos.h queue.c rem rem.1 rem2ps.1 rem2ps.c rem2ps.h remind-all.csh \
remind-all.sh remind.1 remind.def smakefile sort.c test-rem \
test-rem.ami test-rem.bat test-rem.cmd test-rem.rexx test.cmp test.rem \
test1.cmp test2.cmp tkremind tkremind.1 token.c trigger.c tstlang.rem \
types.h userfns.c utils.c var.c version.h

OBJS= $(SRCS:.c=.o)

all: remind rem2ps

.c.o:
	$(CC) $(UNIX) $(SYSV) -c $(CFLAGS) $(CDEFS) $*.c

rem2ps: rem2ps.o
	$(LD) $(LDFLAGS) -o rem2ps rem2ps.o

remind: $(OBJS)
	$(LD) $(LDFLAGS) -o remind $(OBJS) $(MATHLIB)

clean:
	rm -f *.o *~ core *.bak

clobber:
	rm -f *.o *~ remind rem2ps test.out core *.bak

test: remind
	sh test-rem

rem2ps.o: rem2ps.c rem2ps.h lang.h config.h
calendar.o: calendar.c $(STDHDRS) expr.h
dorem.o: dorem.c $(STDHDRS) expr.h
dosubst.o: dosubst.c $(STDHDRS) $(LANGHDRS)
expr.o: expr.c $(STDHDRS) expr.h
files.o: files.c $(STDHDRS)
funcs.o: funcs.c $(STDHDRS) expr.h version.h
globals.o: globals.c config.h types.h globals.h err.h lang.h $(LANGHDRS)
hbcal.o: hbcal.c $(STDHDRS)
init.o: init.c $(STDHDRS) expr.h version.h lang.h $(LANGHDRS)
main.o: main.c $(STDHDRS) expr.h
moon.o: moon.c $(STDHDRS)
omit.o: omit.c $(STDHDRS)
sort.o: sort.c $(STDHDRS)
queue.o: queue.c $(STDHDRS)
token.o: token.c $(STDHDRS)
trigger.o: trigger.c $(STDHDRS) expr.h
userfns.o: userfns.c $(STDHDRS) expr.h
utils.o: utils.c $(STDHDRS)
var.o: var.c $(STDHDRS) expr.h

# Some of these targets are just for my convenience and
# probably won't be too useful to you! -- dfs

tgz:
	tar cvf remind-3.0.14.tar $(MANIFEST)
	gzip -v -9 remind-3.0.14.tar
	mv remind-3.0.14.tar.gz remind-3.0.14.tgz

shar:
	shar -o./Shar -sdfs@doe.carleton.ca -a -c -n"Remind $(VERSION)" -m -l58 -o./Shar $(MANIFEST)

backup:
	tar cvzf ../rbackup.tgz $(MANIFEST)

zip:
	zip ../rbackup.zip $(MANIFEST)

todos:
	rm -rf DOS
	mkdir DOS
	for i in $(MANIFEST) ; do todos < $$i > DOS/$$i; done	

transmit:
	sz -a -e $(MANIFEST)

install:  install-bin install-scripts install-man

install-bin: remind rem2ps
	cp remind $(BINDIR)/remind
	-chmod $(EXEMODE) $(BINDIR)/remind
	-chown $(OWNER) $(BINDIR)/remind
	-chgrp $(GROUP) $(BINDIR)/remind
	cp rem2ps $(BINDIR)/rem2ps
	-chmod $(EXEMODE) $(BINDIR)/rem2ps
	-chown $(OWNER) $(BINDIR)/rem2ps
	-chgrp $(GROUP) $(BINDIR)/rem2ps

install-scripts:
	cp kall $(SCRIPTDIR)/kall
	-chmod $(EXEMODE) $(SCRIPTDIR)/kall
	-chown $(OWNER) $(SCRIPTDIR)/kall
	-chgrp $(GROUP) $(SCRIPTDIR)/kall
	cp rem $(SCRIPTDIR)/rem
	-chmod $(EXEMODE) $(SCRIPTDIR)/rem
	-chown $(OWNER) $(SCRIPTDIR)/rem
	-chgrp $(GROUP) $(SCRIPTDIR)/rem

install-man:
	cp remind.1 $(MANDIR)/man$(MANSECT)/remind.$(MANSECT)
	-chmod $(MANMODE) $(MANDIR)/man$(MANSECT)/remind.$(MANSECT)
	-chown $(OWNER) $(MANDIR)/man$(MANSECT)/remind.$(MANSECT)
	-chgrp $(GROUP) $(MANDIR)/man$(MANSECT)/remind.$(MANSECT)
	cp tkremind.1 $(MANDIR)/man$(MANSECT)/tkremind.$(MANSECT)
	-chmod $(MANMODE) $(MANDIR)/man$(MANSECT)/tkremind.$(MANSECT)
	-chown $(OWNER) $(MANDIR)/man$(MANSECT)/tkremind.$(MANSECT)
	-chgrp $(GROUP) $(MANDIR)/man$(MANSECT)/tkremind.$(MANSECT)
	cp rem.1 $(MANDIR)/man$(MANSECT)/rem.$(MANSECT)
	-chmod $(MANMODE) $(MANDIR)/man$(MANSECT)/rem.$(MANSECT)
	-chown $(OWNER) $(MANDIR)/man$(MANSECT)/rem.$(MANSECT)
	-chgrp $(GROUP) $(MANDIR)/man$(MANSECT)/rem.$(MANSECT)
	cp kall.1 $(MANDIR)/man$(MANSECT)/kall.$(MANSECT)
	-chmod $(MANMODE) $(MANDIR)/man$(MANSECT)/kall.$(MANSECT)
	-chown $(OWNER) $(MANDIR)/man$(MANSECT)/kall.$(MANSECT)
	-chgrp $(GROUP) $(MANDIR)/man$(MANSECT)/kall.$(MANSECT)
	cp rem2ps.1 $(MANDIR)/man$(MANSECT)/rem2ps.$(MANSECT)
	-chmod $(MANMODE) $(MANDIR)/man$(MANSECT)/rem2ps.$(MANSECT)
	-chown $(OWNER) $(MANDIR)/man$(MANSECT)/rem2ps.$(MANSECT)
	-chgrp $(GROUP) $(MANDIR)/man$(MANSECT)/rem2ps.$(MANSECT)

release:
	-mkdir RELEASE
	-rm -f RELEASE/*
	mkpatch ../prev . patch.14 Shar "Remind-3.0/Patch-14/part"
	mv Shar* RELEASE
	rm -f patch.14*
	for i in *.1; do nroff -man $$i | sed -e 's/_//g' > `basename $$i .1`.man; done
	mv *.man RELEASE
	for i in *.1; do groff -man -Tps $$i > `basename $$i .1`.ps; done
	mv *.ps RELEASE
