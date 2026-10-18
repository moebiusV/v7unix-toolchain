#!/bin/sh
#
# x86test-all -- run tools/x86tests/*.c (first line "/* expect: N */") with the
# i386 ccom under plain and optimizing flag sets.  X86CCOM must be set.
#
set -u
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
[ $# -eq 0 ] && set -- "" "-xdmr" "-xtemps -xdeljumps -xinline -xdce" "-xdmr -xtemps -xdeljumps -xinline -xdce" "-xdmr -xtemps -xdeljumps -xinline -xdce -xssa"
dir=${X86TESTS:-$here/x86tests}
pass=0; fail=0
for flags in "$@"; do
	for t in "$dir"/*.c; do
		want=$(sed -n '1s/.*expect: \(-\{0,1\}[0-9]*\).*/\1/p' "$t")
		# "skip: FLAGS (reason)" on any line: a known failure under FLAGS
		skip=$(sed -n 's/.*skip: \([^ ]*\) .*/\1/p' "$t" | head -1)
		if [ -n "$skip" ] && [ "${flags#*$skip}" != "$flags" ]; then continue; fi
		got=$("$here/x86test.sh" $flags "$t" 2>&1 | tail -1)
		if [ "$got" = "$want" ]; then pass=$((pass+1))
		else fail=$((fail+1)); echo "FAIL [$flags] $(basename "$t"): want $want, got $got"; fi
	done
done
echo "x86test-all: $pass passed, $fail failed"
[ $fail -eq 0 ]
