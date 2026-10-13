#!/bin/sh
#
# x86size -- total text+data bytes of the files (K&R libc is fine) compiled by
# the i386 pcc ccom with the given flags and assembled by the host's 32-bit as.
#
# usage: X86CCOM=... [X86INC=include dir] x86size.sh "ccom flags" file.c ...
#
set -u
ccom=${X86CCOM:?set X86CCOM}
flags=$1; shift
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
tot=0; bad=0
for f in "$@"; do
	d=$(dirname "$f")
	if ! gcc -E -P -m32 -undef -D__i386__ -Dunix -I"$d" -I"${X86INC:-.}" "$f" > "$tmp/i" 2>/dev/null; then
		bad=$((bad+1)); continue
	fi
	rm -f "$tmp/s" "$tmp/o"
	if ! $ccom $flags "$tmp/i" "$tmp/s" >/dev/null 2>&1; then
		bad=$((bad+1)); continue
	fi
	if ! gcc -m32 -c -x assembler -o "$tmp/o" "$tmp/s" >/dev/null 2>&1; then
		bad=$((bad+1)); continue
	fi
	n=$(size "$tmp/o" 2>/dev/null | awk 'NR==2{print $1+$2}')
	tot=$((tot+${n:-0}))
done
echo "$tot bytes, $bad files failed"
