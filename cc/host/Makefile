# Host build of V7 c1 (the PDP-11 code generator).
#
# The committed host/*.c are generated from the K&R sources by a three-step
# pipeline; see ../README.host for the full recipe.  This Makefile just
# compiles and links them.

CC      = gcc
CFLAGS  = -std=c99 -O1 -Wall -fno-strict-aliasing

OBJS = c10.o c11.o c12.o c13.o table.o globals.o

all: c1

c1: $(OBJS)
	$(CC) -o $@ $(OBJS)

%.o: %.c c1.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f c1 $(OBJS)

.PHONY: all clean
