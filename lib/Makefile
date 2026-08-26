# PDP-11 C runtime startup — V7 /usr/src/libc/csu, assembled by our own `as`.
#
# These are target objects (they run on the PDP-11), not host binaries; they
# install to /usr/local/lib/pdp11-v7/ (see the top-level Makefile).  The .s
# sources are V7 originals, kept unmodified.

AS = ../modern/as/as

CRT = crt0.o fcrt0.o mcrt0.o fmcrt0.o

all: $(CRT)

%.o: %.s
	$(AS) -u -o $@ $<

clean:
	rm -f $(CRT)

.PHONY: all clean
