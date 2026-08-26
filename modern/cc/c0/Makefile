# Host build of V7 c0 (the PDP-11 first pass / parser).
#
# The committed host/c0/*.c are generated from the K&R sources by a three-step
# pipeline; see c0-host.py and the c0.rules.json / c0.spec.json specs in the
# parent directory.  This Makefile just compiles and links them.
#
# NOTE: the V7 sources are riddled with pre-standard UB (sequence points,
# uninitialized locals, pointer punning).  We run at -O1 rather than -O2 to
# keep gcc's optimizer from exploiting it; drop to -O0 if a given input still
# miscompiles.

CC      = gcc
CFLAGS  = -std=c99 -O1 -Wall -fno-strict-aliasing

OBJS = c00.o c01.o c02.o c03.o c04.o c05.o globals.o

all: c0

c0: $(OBJS)
	$(CC) -o $@ $(OBJS)

%.o: %.c c0.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f c0 $(OBJS)

.PHONY: all clean
