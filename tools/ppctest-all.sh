#!/bin/sh
#
# ppctest-all -- run tools/ppctests/*.c (first line "/* expect: N */") with the
# 32-bit PowerPC ccom under plain and optimizing flag sets.  PPCCCOM must be set.
#
set -u
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
[ $# -eq 0 ] && set -- "" "-xdmr" "-xtemps -xdeljumps -xinline -xdce" "-xdmr -xtemps -xdeljumps -xinline -xdce" "-xdmr -xtemps -xdeljumps -xinline -xdce -xssa"
dir=${PPCTESTS:-$here/ppctests}
pass=0; fail=0
for flags in "$@"; do
	for t in "$dir"/*.c; do
		want=$(sed -n '1s/.*expect: \(-\{0,1\}[0-9]*\).*/\1/p' "$t")
		# "skip: FLAGS (reason)" on any line: a known failure under FLAGS
		skip=$(sed -n 's/.*skip: \([^ ]*\) .*/\1/p' "$t" | head -1)
		if [ -n "$skip" ] && [ "${flags#*$skip}" != "$flags" ]; then continue; fi
		got=$("$here/ppctest.sh" $flags "$t" 2>&1 | tail -1)
		if [ "$got" = "$want" ]; then pass=$((pass+1))
		else fail=$((fail+1)); echo "FAIL [$flags] $(basename "$t"): want $want, got $got"; fi
	done
done
echo "ppctest-all: $pass passed, $fail failed"
[ $fail -eq 0 ]
