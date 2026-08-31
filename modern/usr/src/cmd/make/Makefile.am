# make — V7's build driver (2.58, March 1979), host-ported.
#
# Six C files + a shared header `defs` + `gram.y`.  gram.y is a V7-yacc grammar
# (with an embedded lexer); bison -y regenerates it as gram.c, keeping the
# parser tables identical to V7's.  `defs` is the shared header, not a source.

AM_CPPFLAGS = -I$(srcdir)
AM_CFLAGS = -std=c99 -O1

v7unix_libexec_PROGRAMS = make
make_SOURCES = doname.c dosys.c files.c ident.c main.c misc.c
nodist_make_SOURCES = gram.c
BUILT_SOURCES = gram.c
CLEANFILES = gram.c y.tab.c y.tab.h

# bison -y writes y.tab.c; rename to gram.c so the build links it in.
gram.c: $(srcdir)/gram.y $(srcdir)/defs
	$(YACC) $(srcdir)/gram.y
	mv -f y.tab.c $@
	rm -f y.tab.h

EXTRA_DIST = defs gram.y
