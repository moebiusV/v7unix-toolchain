# yacc — V7's parser generator (S. C. Johnson, 1978), host-ported.
#
# Its defining contract is byte-identical `y.tab.c` output vs V7 (PORTING.md
# §8.2): only yacc's *own* source is C99-modernized and output-neutral.  The
# `yaccpar` skeleton is data, not compiled C — it is copied byte-for-byte into
# the generated parser, so it stays V7's (K&R `yyparse()`), shipped and
# installed verbatim.  It is found at run time via the YACCPARSER env var or
# the compiled-in YACC_PARSER_PATH default (see `files`).

AM_CPPFLAGS = -I$(srcdir)
AM_CFLAGS = -std=c99 -O1

v7unix_libexec_PROGRAMS = yacc
yacc_SOURCES = y1.c y2.c y3.c y4.c
noinst_HEADERS = dextern files

v7unix_libexec_DATA = yaccpar

EXTRA_DIST = dextern files yaccpar
