# c0 — V7's PDP-11 first pass (parser), host-ported.
#
# The committed host c0*.c are generated from the K&R sources by the
# three-step pipeline; see c0-host.py and the c0.rules.json / c0.spec.json
# specs in tools/.  This Makefile.am just compiles and links them.
#
# The V7 sources are riddled with pre-standard UB (sequence points,
# uninitialized locals, pointer punning); -O1 rather than -O2 keeps gcc's
# optimizer from exploiting it, and -fno-strict-aliasing covers the punning.

AM_CFLAGS = -std=c99 -O1 @NO_STRICT_ALIASING@

v7unix_libexec_PROGRAMS = c0
c0_SOURCES = c00.c c01.c c02.c c03.c c04.c c05.c globals.c
noinst_HEADERS = c0.h
