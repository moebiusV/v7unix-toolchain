# man — V7 manual pages, installed in a dedicated "v7unix" man section so they
# keep their original Unix names (cc.1, as.1, ld.1, lex.1, yacc.1, make.1,
# adb.1) without colliding with the host's own cc(1)/as(1)/ld(1).
#
# FHS and BSD hier(7) permit additional manual sections beyond man1..man8
# (Perl ships 3pm, OpenSSL 3ssl, ...); here the section is a whole man-tree
# root under $(mandir)/v7unix.  `man cc` finds these once $(mandir)/v7unix is
# on MANPATH — a drop-in is shipped in dist/ and documented in
# README.distributions.
#
# The v7-prefixed command names (v7cc, v7as, v7ld) also get a manpage in the
# normal man1/ spot — a .so include stub into the v7unix section — so `man
# v7cc` works with no MANPATH configuration.

v7unixman1dir = $(mandir)/v7unix/man1
dist_v7unixman1_DATA = \
	cc.1 \
	as.1 \
	ld.1 \
	lex.1 \
	yacc.1 \
	make.1 \
	adb.1

install-data-hook:
	mkdir -p $(DESTDIR)$(mandir)/man1
	printf '.so v7unix/man1/cc.1\n' > $(DESTDIR)$(mandir)/man1/v7cc.1
	printf '.so v7unix/man1/as.1\n'  > $(DESTDIR)$(mandir)/man1/v7as.1
	printf '.so v7unix/man1/ld.1\n'  > $(DESTDIR)$(mandir)/man1/v7ld.1

uninstall-hook:
	rm -f $(DESTDIR)$(mandir)/man1/v7cc.1 \
	      $(DESTDIR)$(mandir)/man1/v7as.1 \
	      $(DESTDIR)$(mandir)/man1/v7ld.1
