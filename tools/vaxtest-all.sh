#!/bin/sh
#
# vaxtest-all -- run tools/pcctests/*.c (or $VAXTESTS) under 32V on SIMH with
# the VAX pcc, once per flag set, and compare with each file's "expect: N".
#
# usage: VAXCCOM=... VAX780=... VAX32V_DISK=... vaxtest-all.sh ["ccom flags" ...]
#
set -u
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
[ $# -eq 0 ] && set -- "-xtemps -xdeljumps -xdce -xinline" "-xdmr -xtemps -xdeljumps -xdce -xinline" "-xdmr -xtemps -xdeljumps -xdce -xinline -xc2"
dir=${VAXTESTS:-$here/pcctests}
pass=0; fail=0
for flags in "$@"; do
	out=$(python3 "$here/vax32v.py" -f "$flags" "$dir"/*.c 2>&1)
	for t in "$dir"/*.c; do
		n=$(basename "$t" .c)
		want=$(sed -n '1s/.*expect: \(-\{0,1\}[0-9]*\).*/\1/p' "$t")
		got=$(printf '%s\n' "$out" | awk -v n="$n" '$1==n{print $2; exit}')
		if [ "$got" = "$want" ]; then pass=$((pass+1))
		else fail=$((fail+1)); echo "FAIL [$flags] $n: want $want, got ${got:-?} $(printf '%s\n' "$out" | awk -v n="$n" '$1==n{$1="";$2="";print; exit}' | cut -c1-80)"; fi
	done
done
echo "vaxtest-all: $pass passed, $fail failed"
[ $fail -eq 0 ]
