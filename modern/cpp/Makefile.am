# cpp — V7's C preprocessor (John Reiser, 1978), host-ported.
#
# Three sources make up cpp: cpp.c (the preprocessor proper), cpy.y (a yacc
# grammar for #if expressions) and yylex.c (the #if lexer, #include'd into the
# generated cpy.c).  The original V7 grammar wrote actions as "={ ... }" (a
# V7-yacc-ism); that '=' is stripped in cpy.y so bison -y accepts it.

AM_CPPFLAGS = -I$(srcdir)
AM_CFLAGS = -std=c99 -O1

v7unix_libexec_PROGRAMS = cpp
cpp_SOURCES = cpp.c
nodist_cpp_SOURCES = cpy.c
BUILT_SOURCES = cpy.c

# bison -y writes y.tab.c; rename to cpy.c so yylex.c's "# include" resolves.
cpy.c: $(srcdir)/cpy.y $(srcdir)/yylex.c
	$(YACC) $(srcdir)/cpy.y
	mv -f y.tab.c $@
	rm -f y.tab.h

check-local:
	$(SHELL) $(srcdir)/tests/run-tests.sh ./cpp

EXTRA_DIST = cpy.y yylex.c README tests
