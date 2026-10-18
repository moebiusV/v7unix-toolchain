#!/bin/sh
#
# a64size -- total text+data bytes of the files compiled by the aarch64 pcc ccom
# with the given flags and assembled by clang (the assembler is the only part of
# its aarch64 target used).  With A64REF=1 the same files are compiled by clang
# at -Os for comparison.  K&R libc is fine.
#
# usage: ARM64CCOM=... [A64INC=include dir] [A64REF=1] a64size.sh "ccom flags" file.c ...
#
set -u
ccom=${ARM64CCOM:?set ARM64CCOM}
flags=$1; shift
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
tot=0; bad=0; reftot=0; refbad=0; n=0; mtot=0
for f in "$@"; do
	d=$(dirname "$f")
	if ! gcc -E -P -undef -D__aarch64__ -Dunix -I"$d" -I"${A64INC:-.}" "$f" > "$tmp/i" 2>/dev/null; then
		bad=$((bad+1)); continue
	fi
	rm -f "$tmp/s" "$tmp/o"
	if ! $ccom $flags "$tmp/i" "$tmp/s" >/dev/null 2>&1; then
		bad=$((bad+1)); continue
	fi
	if ! clang --target=aarch64-linux-gnu -c -x assembler -o "$tmp/o" "$tmp/s" >/dev/null 2>&1; then
		bad=$((bad+1)); continue
	fi
	sz=$(size "$tmp/o" 2>/dev/null | awk 'NR==2{print $1+$2}')
	tot=$((tot+${sz:-0})); n=$((n+1))
	if [ "${A64REF:-}" = 1 ]; then
		if clang --target=aarch64-linux-gnu -std=gnu89 -w -Os -c -x c -o "$tmp/r.o" "$tmp/i" >/dev/null 2>&1; then
			rs=$(size "$tmp/r.o" 2>/dev/null | awk 'NR==2{print $1+$2}')
			reftot=$((reftot+${rs:-0})); mtot=$((mtot+${sz:-0}))
			[ -n "${A64V:-}" ] && echo "$f ${sz:-0} ${rs:-0}"
		else
			refbad=$((refbad+1))
		fi
	fi
done
echo "$tot bytes in $n files, $bad failed"
[ "${A64REF:-}" = 1 ] && echo "clang -Os: $reftot bytes (pcc on the same files: $mtot), $refbad files clang could not compile"
exit 0
