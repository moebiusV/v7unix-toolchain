# cc — the V7 C compiler driver, host-ported.
#
# This is just the driver: it forks/execs the four passes (cpp, c0, c1, c2),
# then /bin/as and /bin/ld.  The pass paths default to V7's /lib/* and can be
# redirected with -B <dir>; /bin/as and /bin/ld are hardcoded (as in V7).

CC      = gcc
CFLAGS  = -std=c99 -O1 -Wall -Wno-parentheses

all: cc

cc: cc.c
	$(CC) $(CFLAGS) -o $@ cc.c

check: cc
	@tests/run-tests.sh ./cc

clean:
	rm -f cc

.PHONY: all check clean
