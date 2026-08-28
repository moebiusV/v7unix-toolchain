# V7 cc makefile, pruned to the passes present here (c0/c1/c2 + table.s).
# Cross-compile for the PDP-11 with:  make CC="pcc --target=pdp11"
CC = cc
CFLAGS = -O -n -s

all: c0 c1 c2

c0: c00.o c01.o c02.o c03.o c04.o c05.o
	$(CC) $(CFLAGS) -o c0 c00.o c01.o c02.o c03.o c04.o c05.o
c00.o c01.o c02.o c03.o c04.o c05.o: c0.h

c1: c10.o c11.o c12.o c13.o table.o
	$(CC) $(CFLAGS) -o c1 c10.o c11.o c12.o c13.o table.o
c10.o c11.o c12.o c13.o: c1.h

table.o: table.s cvopt
	cvopt <table.s >table.i
	as -o table.o table.i
	rm table.i

c2: c20.o c21.o
	$(CC) -O -o c2 c20.o c21.o
