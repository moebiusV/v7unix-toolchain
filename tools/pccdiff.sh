#!/bin/sh
#
# pccdiff -- scoreboard: how close is modern pcc's PDP-11 output to dmr-cc's?
#
# For each .c in the corpus (default: orig/usr/src/libc), compile it two ways:
#   dmr: cpp -> c0 -> c1 -> c2     (the ported host tools in modern/)
#   pcc: pcc -O -S                 (ccom, then V7's c2 via the driver's -O)
# normalize both listings and report identical / differing / pcc-failed.
#
# usage: pccdiff.sh [-v] [-k KEEPDIR] [file.c | dir ...]
#   PCC=path/to/pcc   (default ~/claude/pcc/install/bin/pcc; its libexec must
#                      hold pdp11-bsd-cc2 -> modern/usr/src/cmd/c/c2)
#
set -u

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
top=$(CDPATH= cd -- "$here/.." && pwd -P)
C=$top/modern/usr/src/cmd
AS=$C/as/as; export AS2=$C/as/as2
CPP=$C/cpp/cpp; C0=$C/c/c0; C1=$C/c/c1; C2=$C/c/c2
INC=$top/orig/usr/include
PCC=${PCC:-$HOME/claude/pcc/install/bin/pcc}

verbose=0; keep=
while getopts vk: o; do
	case $o in v) verbose=1;; k) keep=$OPTARG;; *) exit 2;; esac
done
shift $((OPTIND-1))
[ $# -eq 0 ] && set -- "$top/orig/usr/src/libc"

W=$(mktemp -d "${TMPDIR:-/tmp}/pccdiff.XXXXXX")
trap 'rm -rf "$W"' EXIT
[ -n "$keep" ] && mkdir -p "$keep"

# normalize: drop stab lines (~x=y) and blank lines, strip leading whitespace,
# put a label that shares a line with an instruction on its own line, drop
# labels nothing refers to (they assemble to nothing), and renumber the rest
# by first appearance.
norm() {
	grep -v '^~[^~]' "$1" | sed -e 's/^[ 	]*//' -e '/^$/d' -e 's/[ 	][ 	]*/ /g' \
	    -e 's/^\(L[0-9][0-9]*:\)\(.\)/\1\
\2/' >"$W/norm.tmp"
	awk '
	function labels(line, mode,    l, out) {
		out = ""
		while (match(line, /L[0-9]+/)) {
			l = substr(line, RSTART, RLENGTH)
			if (mode == 1) ref[l] = 1
			else { if (!(l in m)) m[l] = "L" ++n; l = m[l] }
			out = out substr(line, 1, RSTART-1) l
			line = substr(line, RSTART+RLENGTH)
		}
		return out line
	}
	NR == FNR { if ($0 !~ /^L[0-9]+:$/) labels($0, 1); next }
	/^L[0-9]+:$/ { l = substr($0, 1, length($0)-1); if (!(l in ref)) next }
	{ print labels($0, 2) }' "$W/norm.tmp" "$W/norm.tmp"
}

objsame=0; same=0; diff=0; pfail=0; dfail=0; tot=0; ninsn_d=0; ninsn_p=0
files=$(for a in "$@"; do
	if [ -d "$a" ]; then find "$a" -name '*.c' | sort; else echo "$a"; fi
done)

for f in $files; do
	tot=$((tot+1))
	b=$(basename "$f" .c); d=$(dirname "$f")
	w=$W/$b; mkdir -p "$w"
	# --- dmr side
	if ! { "$CPP" -I"$d" -I"$INC" "$f" "$w/i" &&
	       "$C0" "$w/i" "$w/t1" "$w/t2" &&
	       "$C1" "$w/t1" "$w/t2" "$w/dmr.raw" &&
	       "$C2" "$w/dmr.raw" "$w/dmr.s"; } >/dev/null 2>&1; then
		dfail=$((dfail+1)); [ $verbose = 1 ] && echo "DMRFAIL $f"; continue
	fi
	# --- pcc side
	if ! "$PCC" -O -S -I"$d" -I"$INC" -o "$w/pcc.s" "$f" >"$w/err" 2>&1; then
		pfail=$((pfail+1))
		[ $verbose = 1 ] && echo "PCCFAIL $f: $(grep -m1 -i error "$w/err")"
		continue
	fi
	norm "$w/dmr.s" >"$w/dmr.n"; norm "$w/pcc.s" >"$w/pcc.n"
	nd=$(grep -c '^[a-z]' "$w/dmr.n"); np=$(grep -c '^[a-z]' "$w/pcc.n")
	ninsn_d=$((ninsn_d+nd)); ninsn_p=$((ninsn_p+np))
	# the real bar: assembled objects (as -u) byte-identical
	if "$AS" -u -o "$w/dmr.o" "$w/dmr.s" >/dev/null 2>&1 &&
	   "$AS" -u -o "$w/pcc.o" "$w/pcc.s" >/dev/null 2>&1 &&
	   cmp -s "$w/dmr.o" "$w/pcc.o"; then
		objsame=$((objsame+1)); [ $verbose = 1 ] && echo "OBJSAME $f"
	fi
	if cmp -s "$w/dmr.n" "$w/pcc.n"; then
		same=$((same+1)); [ $verbose = 1 ] && echo "SAME    $f"
	else
		diff=$((diff+1)); [ $verbose = 1 ] && echo "DIFF    $f  (dmr $nd, pcc $np)"
	fi
	[ -n "$keep" ] && cp "$w/dmr.n" "$keep/$b.dmr" && cp "$w/pcc.n" "$keep/$b.pcc"
done

echo "pccdiff: $tot files: $same text-identical, $objsame object-identical, $diff differ, $pfail pcc-failed, $dfail dmr-failed"
echo "         lines (compiled-by-both only): dmr $ninsn_d, pcc $ninsn_p"
