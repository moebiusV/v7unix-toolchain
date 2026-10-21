#!/bin/sh
#
# a64test-all -- run tools/a64tests/*.c (first line "/* expect: N */") with the
# x86-64 ccom under plain and optimizing flag sets.  A64CCOM must be set.
#
set -u
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
[ $# -eq 0 ] && set -- "" "-xdmr" "-xtemps -xdeljumps -xinline -xdce" "-xdmr -xtemps -xdeljumps -xinline -xdce" "-xdmr -xtemps -xdeljumps -xinline -xdce -xssa" "-xdmr -xtemps -xdeljumps -xinline -xdce -xabs"
dir=${A64TESTS:-$here/a64tests}
pass=0; fail=0
for flags in "$@"; do
	for t in "$dir"/*.c; do
		want=$(sed -n '1s/.*expect: \(-\{0,1\}[0-9]*\).*/\1/p' "$t")
		got=$("$here/a64test.sh" $flags "$t" 2>&1 | tail -1)
		if [ "$got" = "$want" ]; then pass=$((pass+1))
		else fail=$((fail+1)); echo "FAIL [$flags] $(basename "$t"): want $want, got $got"; fi
	done
done
echo "a64test-all: $pass passed, $fail failed"
[ $fail -eq 0 ]
