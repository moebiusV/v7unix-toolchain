#!/bin/sh
#
# pccdiff -- scoreboard: how close is modern pcc's PDP-11 output to dmr-cc's?
#
# For each .c in the corpus (default: orig/usr/src/libc), compile it two ways:
#   dmr: cpp -> c0 -> c1 -> c2     (the ported host tools in modern/)
#   pcc: (a) pcc -S then c2        (ccom's plain output, V7 peephole only)
#        (b) pcc -O -S             (ccom -x flags, then c2: both optimizers)
# assemble each and compare the sizes (text+data bytes).  The target is dmr's
# code SIZE, not its bytes: label names, instruction choice and pcc's own
# optimizer (-xtemps/-xssa/...) are all fair game.
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

# text+data bytes of an assembled object (a.out header words 1 and 2)
osize() {
	od -An -tu2 -j2 -N4 "$1" | awk '{ print $1 + $2 }'
}

# assemble $1 (.s) -> $2 (.o); print its size, or nothing on failure
asize() {
	"$AS" -u -o "$2" "$1" >/dev/null 2>&1 && osize "$2"
}

pfail=0; dfail=0; tot=0
sd=0; sc=0; sx=0		# summed bytes: dmr, pcc -S+c2, pcc -O (x flags + c2)
okc=0; okx=0			# files where pcc is no bigger than dmr
files=$(for a in "$@"; do
	if [ -d "$a" ]; then find "$a" -name '*.c' | sort; else echo "$a"; fi
done)

for f in $files; do
	b=$(basename "$f" .c); d=$(dirname "$f")
	w=$W/$b; mkdir -p "$w"
	# --- dmr side
	if ! { "$CPP" -I"$d" -I"$INC" "$f" "$w/i" &&
	       "$C0" "$w/i" "$w/t1" "$w/t2" &&
	       "$C1" "$w/t1" "$w/t2" "$w/dmr.raw" &&
	       "$C2" "$w/dmr.raw" "$w/dmr.s"; } >/dev/null 2>&1; then
		dfail=$((dfail+1)); [ $verbose = 1 ] && echo "DMRFAIL $f"; continue
	fi
	# --- pcc side, two ways: ccom then c2 only, and the driver's full -O
	if ! { "$PCC" -S ${PCCDIFF_SFLAGS:-} -I"$d" -I"$INC" -o "$w/pccc.raw" "$f" &&
	       "$C2" = "$w/pccc.raw" "$w/pccc.s"; } >"$w/err" 2>&1 ||
	   ! "$PCC" -O ${PCCDIFF_XFLAGS:-} -S -I"$d" -I"$INC" -o "$w/pccx.s" "$f" >>"$w/err" 2>&1; then
		pfail=$((pfail+1))
		[ $verbose = 1 ] && echo "PCCFAIL $f: $(grep -m1 -i error "$w/err")"
		continue
	fi
	bd=$(asize "$w/dmr.s" "$w/dmr.o"); bc=$(asize "$w/pccc.s" "$w/pccc.o")
	bx=$(asize "$w/pccx.s" "$w/pccx.o")
	if [ -z "$bd" ] || [ -z "$bc" ] || [ -z "$bx" ]; then
		pfail=$((pfail+1)); [ $verbose = 1 ] && echo "ASFAIL  $f"; continue
	fi
	tot=$((tot+1)); sd=$((sd+bd)); sc=$((sc+bc)); sx=$((sx+bx))
	[ "$bc" -le "$bd" ] && okc=$((okc+1))
	[ "$bx" -le "$bd" ] && okx=$((okx+1))
	[ $verbose = 1 ] && printf '%-8s dmr %5d  c2-only %5d (%+d)  -O %5d (%+d)  %s\n' \
	    "$([ "$bx" -le "$bd" ] && echo OK || echo BIGGER)" "$bd" "$bc" $((bc-bd)) "$bx" $((bx-bd)) "$f"
	if [ -n "$keep" ]; then
		norm "$w/dmr.s" >"$keep/$b.dmr"; norm "$w/pccc.s" >"$keep/$b.pcc"
		norm "$w/pccx.s" >"$keep/$b.pccx"
	fi
done

pct() { awk -v a="$1" -v b="$2" 'BEGIN { printf "%+.1f%%", b ? (a-b)*100/b : 0 }'; }
echo "pccdiff: $tot files compared ($pfail pcc-failed, $dfail dmr-failed); text+data bytes:"
echo "  dmr cc:                  $sd"
echo "  pcc -S + c2 only:        $sc  ($(pct $sc $sd))   $okc/$tot files no bigger than dmr"
echo "  pcc -O (x flags + c2):   $sx  ($(pct $sx $sd))   $okx/$tot files no bigger than dmr"
