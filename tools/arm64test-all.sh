#!/bin/sh
#
# arm64test-all -- run tools/a64tests/*.c (the LP64 tests; first line "/* expect: N */")
# with the aarch64 ccom on the emulator (arm64test.sh), then the struct-passing tests against
# clang (arm64abi.sh, tools/a64abi).  ARM64CCOM must be set.
#
set -u
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
[ $# -eq 0 ] && set -- ""
dir=${ARM64TESTS:-$here/a64tests}
pass=0; fail=0
for flags in "$@"; do
	for t in "$dir"/*.c; do
		want=$(sed -n '1s/.*expect: \(-\{0,1\}[0-9]*\).*/\1/p' "$t")
		skip=$(sed -n 's/.*skip: \([^ ]*\) .*/\1/p' "$t" | head -1)
		if [ -n "$skip" ] && [ "${flags#*$skip}" != "$flags" ]; then continue; fi
		got=$("$here/arm64test.sh" $flags "$t" 2>&1 | tail -1)
		if [ "$got" = "$want" ]; then pass=$((pass+1))
		else fail=$((fail+1)); echo "FAIL [$flags] $(basename "$t"): want $want, got $got"; fi
	done
done
# the calling convention against clang: a pcc-compiled and a clang-compiled file call each other
for flags in "$@"; do
	for pair in abi abiva; do
		got=$("$here/arm64abi.sh" $flags "$here/a64abi/${pair}_pcc.c" "$here/a64abi/${pair}_clang.c" 2>&1 | tail -1)
		if [ "$got" = 1 ]; then pass=$((pass+1))
		else fail=$((fail+1)); echo "FAIL [$flags] $pair (against clang): got $got"; fi
	done
done
# a 128-bit long double (the compiler built with -DA64QUAD): ARM64QUADCCOM names that ccom
if [ -n "${ARM64QUADCCOM:-}" ]; then
	for flags in "$@"; do
		for t in "$here"/a64quad/q*.c; do
			case $(basename "$t") in
			q[4-9]*)	# arithmetic: with the quad runtime (tf.c) built by clang
				got=$(ARM64CCOM=$ARM64QUADCCOM "$here/arm64abi.sh" $flags "$t" "$here/a64quad/tf.c" 2>&1 | tail -1) ;;
			*)	got=$(ARM64CCOM=$ARM64QUADCCOM "$here/arm64test.sh" $flags "$t" 2>&1 | tail -1) ;;
			esac
			if [ "$got" = 1 ]; then pass=$((pass+1))
			else fail=$((fail+1)); echo "FAIL [$flags] $(basename "$t") (long double): got $got"; fi
		done
		got=$(ARM64CCOM=$ARM64QUADCCOM "$here/arm64abi.sh" $flags "$here/a64quad/ld_pcc.c" "$here/a64quad/ld_clang.c" 2>&1 | tail -1)
		if [ "$got" = 1 ]; then pass=$((pass+1))
		else fail=$((fail+1)); echo "FAIL [$flags] ld (long double against clang): got $got"; fi
	done
fi
echo "arm64test-all: $pass passed, $fail failed"
[ $fail -eq 0 ]
