#!/bin/sh
#
# c2stats -- what is c2 still doing after the IR passes?
#
# Compile a corpus with the target's ccom, run that target's c2 with its -n
# switch on the result, and add up the counters it prints.  A counter that is
# large is work that has not been moved into mip/optim2.c or the templates yet.
#
# usage: c2stats.sh vax|pdp11 [file.c[.gz] ...]
#
#   vax    needs VAXCCOM (the vax-unknown-unix32v ccom), VAXC2 (a host build of
#          cc2/cc2.vax.c) and VAX32V_INC (32V's /usr/include); the default
#          corpus is 32V libc gen+stdio and the first 40 commands.
#   pdp11  uses PCC_CCOM (default ~/claude/pcc/install/libexec/pdp11-bsd-ccom)
#          and builds cc2/cc2.pdp11.c for the host; the default corpus is
#          orig/usr/src/libc gen+stdio and cmd.
#   CCOM_FLAGS overrides the ccom flags (default: -xdmr -xtemps -xdeljumps -xdce
#   -xinline).
#
set -u
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
top=$(CDPATH= cd -- "$here/.." && pwd -P)
target=${1:?usage: c2stats.sh vax|pdp11 [files]}; shift
flags=${CCOM_FLAGS:--xdmr -xtemps -xdeljumps -xdce -xinline}
W=$(mktemp -d "${TMPDIR:-/tmp}/c2stats.XXXXXX")
trap 'rm -rf "$W"' EXIT
: >"$W/stats"
n=0; bad=0

case $target in
vax)
	ccom=${VAXCCOM:?set VAXCCOM}; c2=${VAXC2:?set VAXC2}; inc=${VAX32V_INC:?set VAX32V_INC}
	src=${V32SRC:-$HOME/claude/facetlang/unixtree/32V/usr/src}
	if [ $# -eq 0 ]; then
		set -- $src/libc/gen/*.c.gz $src/libc/stdio/*.c.gz $(ls $src/cmd/*.c.gz | head -40)
	fi
	;;
pdp11)
	ccom=${PCC_CCOM:-$HOME/claude/pcc/install/libexec/pdp11-bsd-ccom}
	pcc=${PCC:-$HOME/claude/pcc/install/bin/pcc}
	if [ -z "${PDP11C2:-}" ]; then
		PDP11C2=$W/c2
		gcc -std=gnu17 -w -o "$PDP11C2" "$HOME/claude/pcc/cc2/cc2.pdp11.c" || exit 1
	fi
	c2=$PDP11C2
	if [ $# -eq 0 ]; then
		set -- $top/orig/usr/src/libc/gen/*.c $top/orig/usr/src/libc/stdio/*.c $top/orig/usr/src/cmd/*.c
	fi
	;;
*)	echo "usage: c2stats.sh vax|pdp11 [files]" >&2; exit 2;;
esac

for f in "$@"; do
	b=$(basename "$f" .gz); b=$(basename "$b" .c)
	d=$(dirname "$f")
	case $f in *.gz) zcat "$f" >"$W/$b.c";; *) cp "$f" "$W/$b.c";; esac
	case $target in
	vax)
		gcc -E -P -undef -Dvax -Dunix -D__vax__ -nostdinc -I "$inc" -I "$d" \
		    "$W/$b.c" -o "$W/$b.i" 2>/dev/null || { bad=$((bad+1)); continue; }
		;;
	pdp11)
		"$pcc" -E -I"$d" -I"$top/orig/usr/include" "$f" -o "$W/$b.i" \
		    >/dev/null 2>&1 || { bad=$((bad+1)); continue; }
		;;
	esac
	$ccom $flags "$W/$b.i" "$W/$b.s" >/dev/null 2>&1 || { bad=$((bad+1)); continue; }
	case $target in
	vax)	"$c2" -n "$W/$b.s" "$W/$b.o" 2>>"$W/stats";;
	pdp11)	"$c2" = - "$W/$b.s" "$W/$b.o" 2>>"$W/stats";;
	esac
	n=$((n+1))
	rm -f "$W/$b".[cis] "$W/$b.o"
done

echo "c2stats: $target, $n files ($bad skipped), ccom flags: $flags"
awk '{ c = $1; $1 = ""; a[$0] += c } END { for (k in a) printf "%7d %s\n", a[k], k }' \
    "$W/stats" | sort -rn
