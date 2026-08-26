# ld — PDP-11 linker, ported to a 64-bit host (middle-endian archives).

CC      = gcc
CFLAGS  = -std=c99 -O2 -Wall

all: ld

ld: ld.c
	$(CC) $(CFLAGS) -o $@ ld.c

# Byte-identical check: assemble main.s + foo.s (with the sibling `as`), link
# them, and compare the a.out to the golden — which was verified byte-identical
# to V7's own `ld` under simh.
check: ld
	@tests/run-tests.sh ./ld ../as/as

clean:
	rm -f ld

.PHONY: all check clean
