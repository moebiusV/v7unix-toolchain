#!/bin/sh
#
# check-tools -- compile a *copy* of the V7 tool source (orig/usr/src/cmd)
# with the ported toolchain, and diff each binary against the V7 reference in
# orig/bin + orig/lib.  Runs *inside* tools/v7check.sh, so the V7 makefiles
# see V7 headers and the ported cc/as/ld/make/yacc on PATH.
#
set -eu

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
topdir=$(CDPATH= cd -- "$here/.." && pwd -P)
ORIG="$topdir/orig"

B=$(mktemp -d "${TMPDIR:-/tmp}/checktools.XXXXXX")
trap 'rm -rf "$B"' EXIT

ok=0; bad=0
chk() {   # chk <name> <built> <reference>
	if [ ! -f "$3" ]; then
		printf '  SKIP %-6s (no reference %s)\n' "$1" "$3"
	elif cmp -s "$2" "$3"; then
		printf '  OK   %-6s\n' "$1"; ok=$((ok+1))
	else
		printf '  DIFF %-6s\n' "$1"; bad=$((bad+1))
	fi
}

echo "check-tools: compiling a copy of orig/ src with the ported toolchain"

# cc, ld -- flat .c, built by /usr/src/cmd/cmake's rule
cd "$B"
cp "$ORIG/usr/src/cmd/cc.c" "$ORIG/usr/src/cmd/ld.c" .
cc -n -s -O cc.c -o cc
chk cc cc "$ORIG/bin/cc"
cc -n -s -O ld.c -o ld
chk ld ld "$ORIG/bin/ld"

# c0, c2 -- c/makefile.
mkdir -p "$B/c"; cd "$B/c"
cp "$ORIG/usr/src/cmd/c/"*.c "$ORIG/usr/src/cmd/c/"*.h \
   "$ORIG/usr/src/cmd/c/"*.s "$ORIG/usr/src/cmd/c/makefile" .
make c0 c2
chk c0 c0 "$ORIG/lib/c0"
chk c2 c2 "$ORIG/lib/c2"

# c1 -- built piecemeal: the makefile's table.o rule regenerates cvopt with
# the ported cc (cross-compiling it to PDP-11), so instead run the pre-built
# *host* cvopt (froot/bin) to convert table.s -> table.i, then assemble.
for f in c10 c11 c12 c13; do cc -O -n -s -c $f.c; done
cvopt < table.s > table.i
as -o table.o table.i
cc -O -n -s -o c1 c10.o c11.o c12.o c13.o table.o
chk c1 c1 "$ORIG/lib/c1"

# as, as2 -- as/makefile (assembly, no helpers)
mkdir -p "$B/as"; cd "$B/as"
cp "$ORIG/usr/src/cmd/as/"*.s "$ORIG/usr/src/cmd/as/makefile" .
make
chk as as "$ORIG/bin/as"
chk as2 as2 "$ORIG/lib/as2"

# cpp -- cpp/makefile (needs yacc for cpy.y)
mkdir -p "$B/cpp"; cd "$B/cpp"
cp "$ORIG/usr/src/cmd/cpp/"* .
make
chk cpp cpp "$ORIG/lib/cpp"

echo "check-tools: $ok identical, $bad different"
[ "$bad" = 0 ]
