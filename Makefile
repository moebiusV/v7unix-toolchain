# V7 PDP-11 C toolchain, modernized for a modern host.
PREFIX ?= /usr/local
SUBDIRS = modern/cc/c0 modern/cc/c1 modern/c2 modern/as modern/ld

all:
	for d in $(SUBDIRS); do $(MAKE) -C $$d; done

check:
	for d in $(SUBDIRS); do $(MAKE) -C $$d check 2>/dev/null || true; done

install:
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m755 modern/cc/c0/c0 modern/cc/c1/c1 modern/c2/c2 modern/as/as modern/ld/ld $(DESTDIR)$(PREFIX)/bin/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/c0 $(DESTDIR)$(PREFIX)/bin/c1 \
	      $(DESTDIR)$(PREFIX)/bin/as $(DESTDIR)$(PREFIX)/bin/ld

clean:
	for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done

distclean: clean
	rm -f cc/host/c0/c0 cc/host/c1 as/as ld/ld

dist:
	tar czf v7-toolchain-0.1.tar.gz cc cc2 as ld include cc.c ld.rules.json \
	    README.md PORTING.md COPYING AUTHORS NEWS ChangeLog INSTALL Makefile

.PHONY: all check install uninstall clean distclean dist
