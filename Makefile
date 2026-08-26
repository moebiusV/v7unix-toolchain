# V7 PDP-11 C toolchain, modernized for a modern host.
#
# modern/ holds the host-ported binaries (still emitting PDP-11 code):
#   cpp  preprocessor      c0  parser         c1  code generator
#   c2   peephole          as  assembler      ld  linker
#   cc   the driver (in modern/cc/)
#
# orig/ and c99/ are sources only (c99/ is the pcc-compilable modernisation);
# they are not built here.

PREFIX ?= /usr/local
LIBEXEC ?= $(PREFIX)/libexec

SUBDIRS = modern/cpp modern/cc modern/cc/c0 modern/cc/c1 modern/c2 modern/as modern/ld

all:
	for d in $(SUBDIRS); do $(MAKE) -C $$d; done

check:
	for d in $(SUBDIRS); do $(MAKE) -C $$d check 2>/dev/null || true; done

install:
	install -d $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(LIBEXEC)
	install -m755 modern/cc/c0/c0 modern/cc/c1/c1 modern/c2/c2 modern/cpp/cpp $(DESTDIR)$(PREFIX)/bin/
	install -m755 modern/cc/cc $(DESTDIR)$(PREFIX)/bin/cc
	install -m755 modern/as/as $(DESTDIR)$(PREFIX)/bin/as
	install -m755 modern/as/as2 $(DESTDIR)$(LIBEXEC)/as2
	install -m755 modern/ld/ld $(DESTDIR)$(PREFIX)/bin/ld

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/c0 $(DESTDIR)$(PREFIX)/bin/c1 \
	      $(DESTDIR)$(PREFIX)/bin/c2 $(DESTDIR)$(PREFIX)/bin/cpp \
	      $(DESTDIR)$(PREFIX)/bin/cc $(DESTDIR)$(PREFIX)/bin/as \
	      $(DESTDIR)$(PREFIX)/bin/ld $(DESTDIR)$(LIBEXEC)/as2

clean:
	for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done

distclean: clean

dist:
	tar czf pdp11-v7-toolchain-0.1.tar.gz \
	    orig c99 modern tools \
	    README.md PORTING.md COPYING AUTHORS NEWS ChangeLog INSTALL Makefile man

.PHONY: all check install uninstall clean distclean dist
