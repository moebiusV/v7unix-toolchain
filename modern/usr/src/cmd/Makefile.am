# cmd — the V7 compiler driver (cc) and linker (ld), host-ported.
#
# Both are single source files (cc.c, ld.c) exactly as in the V7 tree; the
# passes they invoke live in the subdirectories c/, as/ and cpp/.

AM_CFLAGS = -std=c99 -O1

v7unix_libexec_PROGRAMS = cc ld cp mv rm cmp ar
cc_SOURCES = cc.c
ld_SOURCES = ld.c
cp_SOURCES = cp.c
mv_SOURCES = mv.c
rm_SOURCES = rm.c
cmp_SOURCES = cmp.c
ar_SOURCES = ar.c
ld_CFLAGS = -std=c99 -O2

check-local:
	$(SHELL) $(srcdir)/tests/run-tests.sh ./ld as/as

SUBDIRS = c as cpp make yacc sh

EXTRA_DIST = README tests
