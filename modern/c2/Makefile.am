# c2 — Ritchie's c2 peephole optimizer, modernized to C99.
#
# c2.c is the PDP-11-target source (int16_t, sbrk()).  The host build
# mechanically swaps sbrk()/K&R-varargs for malloc()/<stdarg.h> via
# build-host.py; the committed source stays the single c2.c.

AM_CFLAGS = -std=c99 -O2

v7unix_libexec_PROGRAMS = c2
nodist_c2_SOURCES = c2.host.c
BUILT_SOURCES = c2.host.c

c2.host.c: $(srcdir)/c2.c $(srcdir)/build-host.py
	$(PYTHON3) $(srcdir)/build-host.py $(srcdir)/c2.c > $@

check-local:
	$(SHELL) $(srcdir)/tests/run-tests.sh ./c2

EXTRA_DIST = c2.c build-host.py README tests
