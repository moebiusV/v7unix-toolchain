# as / as2 — the V7 PDP-11 assembler, ported to a 64-bit host.
#
# as1.c and as2.c are C translations of the original PDP-11 assembly
# (as11.s..as29.s).  Pass 1 (`as`) execs pass 2 (`as2`); as1.c's as2locate()
# finds `as2` beside argv[0] (this build tree) or via $AS2, falling back to
# the compiled-in V7_LIBEXECDIR/as2.

AM_CFLAGS = -std=c99 -O1

v7unix_libexec_PROGRAMS = as as2
as_SOURCES = as1.c
as2_SOURCES = as2.c

check-local:
	$(SHELL) $(srcdir)/tests/run-tests.sh ./as

EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL NEWS README tests
