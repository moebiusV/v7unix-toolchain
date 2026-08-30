# c — V7's PDP-11 compiler passes (c0, c1, c2), host-ported.
#
# All three passes live in one directory, mirroring V7's /usr/src/cmd/c.
# c0 (parser) and c1 (code generator) keep -O1 + -fno-strict-aliasing: the
# V7 sources are full of pre-standard UB the optimizer would otherwise
# exploit.  c2 is a straightforward port and builds at -O2.

AM_CFLAGS = -std=c99 -O1 @NO_STRICT_ALIASING@

v7unix_libexec_PROGRAMS = c0 c1 c2

c0_SOURCES = c00.c c01.c c02.c c03.c c04.c c05.c c0globals.c
c1_SOURCES = c10.c c11.c c12.c c13.c table.c c1globals.c

# c2.c is the PDP-11-target source (int16_t, sbrk()); the host build swaps
# sbrk()/K&R-varargs for malloc()/<stdarg.h> via build-host.py.
c2_CFLAGS = -std=c99 -O2
nodist_c2_SOURCES = c2.host.c
BUILT_SOURCES = c2.host.c
CLEANFILES = c2.host.c
c2.host.c: $(srcdir)/c2.c $(srcdir)/build-host.py
	$(PYTHON3) $(srcdir)/build-host.py $(srcdir)/c2.c > $@

noinst_HEADERS = c0.h c1.h

check-local:
	$(SHELL) $(srcdir)/tests/run-tests.sh ./c2

EXTRA_DIST = c2.c build-host.py README tests
