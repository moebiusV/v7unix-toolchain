# c2 — Ritchie's c2 peephole optimizer, modernized to C99.
#
# c2.c is the PDP-11-target source (int16_t, sbrk()).  The host build
# mechanically swaps sbrk()/K&R-varargs for malloc()/<stdarg.h> via
# build-host.py; the committed source stays the single c2.c.

CC      = gcc
CFLAGS  = -std=c99 -O2 -Wall

all: c2

host/c2.host.c: c2.c build-host.py
	@mkdir -p host
	python3 build-host.py c2.c > $@

c2: host/c2.host.c
	$(CC) $(CFLAGS) -o $@ host/c2.host.c

# Golden-output check: c2's output on tests/test.s must match tests/test.opt.s,
# which is V7's own c2 output (verified byte-identical under simh).
check: c2
	@tests/run-tests.sh ./c2

clean:
	rm -f c2
	rm -rf host

.PHONY: all check clean
