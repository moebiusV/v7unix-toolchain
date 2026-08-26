# cpp — V7's C preprocessor (John Reiser, 1978), host-ported.
#
# Three sources make up cpp: cpp.c (the preprocessor proper), cpy.y (a yacc
# grammar for #if expressions) and yylex.c (the #if lexer, #include'd into the
# generated cpy.c).  The original V7 grammar wrote actions as "={ ... }" (a
# V7-yacc-ism); that '=' is stripped in cpy.y so bison -y accepts it.

CC      = gcc
CFLAGS  = -std=c99 -O1 -Wall -Wno-comment
YACC    = bison -y

all: cpp

# bison -y writes y.tab.c; rename to cpy.c so yylex.c's "# include" resolves.
cpy.c: cpy.y yylex.c
	$(YACC) cpy.y
	mv -f y.tab.c cpy.c
	rm -f y.tab.h

cpp: cpp.o cpy.o
	$(CC) -o $@ cpp.o cpy.o

cpp.o: cpp.c
	$(CC) $(CFLAGS) -c -o $@ cpp.c

cpy.o: cpy.c
	$(CC) $(CFLAGS) -c -o $@ cpy.c

check: cpp
	@tests/run-tests.sh ./cpp

clean:
	rm -f cpp cpp.o cpy.o cpy.c y.tab.c y.tab.h

.PHONY: all check clean
