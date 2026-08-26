# Makefile.in -- pdp11-as
#
# Process this file with ./configure to produce a Makefile.

PACKAGE		= pdp11-as
VERSION		= 0.1

srcdir		= .
prefix		= /usr/local
exec_prefix	= ${prefix}
bindir		= ${exec_prefix}/bin
libexecdir	= ${exec_prefix}/libexec
datarootdir	= ${prefix}/share
mandir		= ${datarootdir}/man
man1dir		= $(mandir)/man1

CC		= gcc
CFLAGS		= -g -O2 -std=c99
CPPFLAGS	= 
LDFLAGS		= 
LIBS		= 
INSTALL		= /usr/bin/install -c
INSTALL_PROGRAM	= $(INSTALL) -m 755
INSTALL_DATA	= $(INSTALL) -m 644

# Pass 1 execs pass 2 and must know where to find it.  as1.c resolves the
# path at run time ($AS2, then a sibling of argv[0], then this compiled-in
# default); this bakes in the configured install location.
AS2_DEF		= -DAS2_PATH='"$(libexecdir)/as2"'

PROGS		= as as2

# The faithful .s sources, retained as the conformance oracle.  They are not
# built here -- as1.c/as2.c are their C translations -- but ship in dist.
S_FILES		= as11.s as12.s as13.s as14.s as15.s as16.s as17.s as18.s \
		  as19.s as21.s as22.s as23.s as24.s as25.s as26.s as27.s \
		  as28.s as29.s sys.s

DISTFILES	= configure configure.ac Makefile.in install-sh \
		  README COPYING AUTHORS NEWS ChangeLog INSTALL \
		  as1.c as2.c makefile.v7 $(S_FILES) \
		  man/as.1 tests/run-tests.sh tests/t.s tests/t2.s \
		  tests/t.aout tests/t2.aout

all: $(PROGS)

as: as1.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(AS2_DEF) -o $@ as1.c $(LDFLAGS) $(LIBS)

as2: as2.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ as2.c $(LDFLAGS) $(LIBS)

check: $(PROGS)
	@$(srcdir)/tests/run-tests.sh ./as

install: all
	$(INSTALL) -d $(DESTDIR)$(bindir) $(DESTDIR)$(libexecdir)
	$(INSTALL_PROGRAM) as  $(DESTDIR)$(bindir)/as
	$(INSTALL_PROGRAM) as2 $(DESTDIR)$(libexecdir)/as2
	$(INSTALL) -d $(DESTDIR)$(man1dir)
	$(INSTALL_DATA) $(srcdir)/man/as.1 $(DESTDIR)$(man1dir)/as.1

uninstall:
	rm -f $(DESTDIR)$(bindir)/as $(DESTDIR)$(libexecdir)/as2
	rm -f $(DESTDIR)$(man1dir)/as.1

clean:
	rm -f $(PROGS)

distclean: clean
	rm -f Makefile config.status config.log
	rm -rf autom4te.cache

dist: distclean
	rm -rf $(PACKAGE)-$(VERSION)
	mkdir $(PACKAGE)-$(VERSION)
	mkdir $(PACKAGE)-$(VERSION)/man
	mkdir $(PACKAGE)-$(VERSION)/tests
	for f in $(DISTFILES); do cp $$f $(PACKAGE)-$(VERSION)/$$f; done
	tar cf - $(PACKAGE)-$(VERSION) | gzip -9 > $(PACKAGE)-$(VERSION).tar.gz
	rm -rf $(PACKAGE)-$(VERSION)

.PHONY: all check install uninstall clean distclean dist
