# as32v -- the 32V VAX assembler, built to run on a 64-bit host.
#
# The sources are 32V's usr/src/cmd/as with a few host fixes (see README).  The program stores
# pointers in ints (yacc values, the symbol table), so it is built as a 32-bit program (-m32);
# configure builds it only when the compiler can do that.  The grammar is run through the tree's
# V7 yacc, which still understands the old `= {` action syntax.

if HAVE_M32

AM_CFLAGS = -m32 -std=gnu89 -w -O1 -fcommon -Dvax
AM_CPPFLAGS = -I$(srcdir) -I.
AM_LDFLAGS = -m32

v7unix_libexec_PROGRAMS = as32v
as32v_SOURCES = as0.c as1.c as2.c as3.c as.h as.yh instrs a.out.h
BUILT_SOURCES = as0.c as.yh
CLEANFILES = as0.c as.yh y.tab.c y.tab.h

YACC_V7 = $(top_builddir)/modern/usr/src/cmd/yacc/yacc
YACC_PARSER = $(top_srcdir)/modern/usr/src/cmd/yacc/yaccpar

as0.c: as0.y yyfix.sed
	YACCPARSER=$(YACC_PARSER) $(YACC_V7) -d $(srcdir)/as0.y
	sed -f $(srcdir)/yyfix.sed y.tab.c >as0.c
	mv y.tab.h as.yh
	rm -f y.tab.c

as.yh: as0.c

check-local:
	$(SHELL) $(srcdir)/tests/run-tests.sh ./as32v

endif

EXTRA_DIST = README tests as0.y yyfix.sed
