DESTDIR?=
PREFIX?=/usr
BINDIR?=$(PREFIX)/bin
MANDIR?=$(PREFIX)/share/man

default: rem2ics.1

rem2ics.1:
	pod2man -c "" rem2ics > rem2ics.1

install: rem2ics.1
	install -p -D rem2ics $(DESTDIR)$(BINDIR)/rem2ics
	install -p -D -m 0644 rem2ics.1 $(DESTDIR)$(MANDIR)/man1/rem2ics.1
