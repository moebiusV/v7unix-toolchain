# sh — the V7 Bourne shell (S. R. Bourne, 1978), host-ported.
#
# 21 C files sharing a tangle of headers.  V7 wrote its control flow through
# the ALGOL-ish macros in mac.h (IF/THEN/FI, WHILE/DO/OD, FOR/DONE, …); the
# modern tree expands those inline, turns the type vocabulary in mode.h into
# real typedefs, the constants into enums and the value macros into functions.
# The full story is in PORTING.md §8.6.

AM_CPPFLAGS = -I$(srcdir)
AM_CFLAGS = -std=c99 -O1

v7unix_libexec_PROGRAMS = sh

sh_SOURCES = \
	args.c blok.c builtin.c cmd.c ctype.c data.c error.c expand.c \
	fault.c io.c macro.c main.c msg.c name.c print.c service.c \
	setbrk.c stak.c string.c word.c xec.c

EXTRA_DIST = \
	brkincr.h ctype.h defs.h dup.h mac.h mode.h name.h stak.h sym.h \
	timeout.h
