# cc2 — Ritchie's c2 peephole optimizer, modernized to C99.
#
# cc2.pdp11.c is the PDP-11-target source (int16_t, sbrk()).  The host build
# mechanically swaps sbrk()/K&R-varargs for malloc()/<stdarg.h> via
# build-host.py; the committed source stays the single cc2.pdp11.c.

CC      = gcc
CFLAGS  = -std=c99 -O2 -Wall

all: cc2

host/cc2.host.c: cc2.pdp11.c build-host.py
	@mkdir -p host
	python3 build-host.py cc2.pdp11.c > $@

cc2: host/cc2.host.c
	$(CC) $(CFLAGS) -o $@ host/cc2.host.c

# Golden-output check: cc2's output on tests/test.s must match tests/test.opt.s,
# which is V7's own c2 output (verified byte-identical under simh).
check: cc2
	@tests/run-tests.sh ./cc2

clean:
	rm -f cc2
	rm -rf host

.PHONY: all check clean
