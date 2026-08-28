# ld — PDP-11 linker, ported to a 64-bit host (middle-endian archives).

AM_CFLAGS = -std=c99 -O2

v7unix_libexec_PROGRAMS = ld
ld_SOURCES = ld.c

check-local:
	$(SHELL) $(srcdir)/tests/run-tests.sh ./ld ../as/as

EXTRA_DIST = tests
