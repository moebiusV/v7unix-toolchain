#!/bin/sh
#
# pcctest-all -- run every tools/pcctests/*.c under several pcc flag sets on
# the simulated PDP-11 (see pcctest.sh) and compare with its "expect: N".
#
# usage: pcctest-all.sh [flag-set ...]     (default: "" "-xdmr" "-O" "-O -xssa")
#
set -u
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
[ $# -eq 0 ] && set -- "" "-xdmr" "-O" "-O -xssa"
pass=0; fail=0; skipped=0
for flags in "$@"; do
	for t in "$here"/pcctests/*.c; do
		want=$(sed -n '1s/.*expect: \(-\{0,1\}[0-9]*\).*/\1/p' "$t")
		# "skip: FLAGS (reason)" on any line: a known failure under FLAGS
		skip=$(sed -n 's/.*skip: \([^ ]*\) .*/\1/p' "$t" | head -1)
		if [ -n "$skip" ] && [ "${flags#*$skip}" != "$flags" ]; then
			skipped=$((skipped+1)); continue
		fi
		got=$("$here/pcctest.sh" $flags "$t" 2>&1 | tail -1)
		if [ "$got" = "$want" ]; then pass=$((pass+1))
		else fail=$((fail+1)); echo "FAIL [$flags] $(basename "$t"): want $want, got $got"; fi
	done
done
echo "pcctest-all: $pass passed, $fail failed, $skipped skipped"
[ $fail -eq 0 ]
