# as / as2 — the V7 PDP-11 assembler, ported to a 64-bit host.
#
# as1.c and as2.c are C translations of the original PDP-11 assembly
# (as11.s..as29.s).  Pass 1 (`as`) execs pass 2 (`as2`); as1.c's as2locate()
# finds `as2` beside argv[0] (this build tree) or via $AS2, so no install
# path is baked in.

CC      = gcc
CFLAGS  = -std=c99 -O1 -Wall

PROGS = as as2

all: $(PROGS)

as: as1.c
	$(CC) $(CFLAGS) -o $@ as1.c

as2: as2.c
	$(CC) $(CFLAGS) -o $@ as2.c

# Golden-object check: as must assemble tests/*.s to byte-identical a.out.
check: $(PROGS)
	@tests/run-tests.sh ./as

clean:
	rm -f $(PROGS)

.PHONY: all check clean
