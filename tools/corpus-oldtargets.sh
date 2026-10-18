#!/bin/sh
#
# corpus-oldtargets -- stock PortableCC against this branch on the two targets clang cannot reach,
# with and without optimization, on the corpora the size documents use.
#   PDP-11: V7 libc and cmd, tools/pccdiff.sh (dmr's cc as the reference)
#   VAX:    32V libc gen+stdio and cmd, tools/vaxdiff.py (32V's cc and cc -O as the reference)
# Raw output goes to $OUT (default /tmp/oldtargets); tools/corpus-oldtargets.py turns it into a table.
#
# needs: tools/corpus-build.sh output (stock builds under $CORPORA/builds/stock/{pdp11,vax}),
# the PDP-11 install of this branch (~/claude/pcc/install), and the 32V environment of vaxtest-all.sh
# (VAX780, VAX32V_DISK, VAX32V_INC, VAXCCOM = this branch's VAX ccom).
set -u
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
top=$here/..
B=${CORPORA:-$HOME/claude/corpora}/builds/stock
OUT=${OUT:-/tmp/oldtargets}
V=${V32SRC:-$HOME/claude/facetlang/unixtree/32V/usr/src}
mkdir -p "$OUT"
unset VAXC2

# --- PDP-11
ln -sf "$top/modern/usr/src/cmd/c/c2" "$B/pdp11/install/libexec/pdp11-bsd-cc2"
for kind in ours stock; do
	if [ $kind = ours ]; then P=$HOME/claude/pcc/install/bin/pcc; else P=$B/pdp11/install/bin/pdp11-bsd-pcc; fi
	for c in libc cmd; do
		PCC=$P "$here/pccdiff.sh" -v "$top/orig/usr/src/$c" >"$OUT/pdp11-$kind-$c.txt" 2>&1
	done
done

# --- VAX: guest-measured; one run per (compiler, flags), strictly one at a time
files="$V/libc/gen/*.c.gz $V/libc/stdio/*.c.gz $V/cmd/*.c.gz"
OPT="-xtemps -xdeljumps -xdce -xinline"
for kind in ours stock; do
	if [ $kind = ours ]; then CC=$VAXCCOM; G=; else CC=$B/vax/cc/ccom/vax-unknown-bsd-ccom; G=--gnu; fi
	for cfg in O0 O; do
		if [ $cfg = O0 ]; then F=; else F=$OPT; [ $kind = ours ] && F="-xdmr $OPT"; fi
		python3 -I "$here/vaxdiff.py" $G --ccom "$CC" -f "$F" $files >"$OUT/vax-$kind-$cfg.txt" 2>&1
	done
done
echo done >"$OUT/DONE"
