#!/bin/sh
#
# emutest-all -- run tools/pcctests/*.c (or $EMUTESTS) on an emulated CPU.
#
# usage: EMU_PY=/path/to/venv/python emutest-all.sh ARCH CCOM ["ccom flags" ...]
#   ARCH: aarch64, arm, mips.  The venv needs unicorn, ziglang, pyelftools.
#
set -u
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
py=${EMU_PY:?set EMU_PY to the venv python}
arch=$1; ccom=$2; shift 2
[ $# -eq 0 ] && set -- "" "-xdmr" "-xtemps -xdeljumps -xdce" "-xdmr -xtemps -xdeljumps -xdce"
dir=${EMUTESTS:-$here/pcctests}
pass=0; fail=0
for flags in "$@"; do
	for t in "$dir"/*.c; do
		want=$(sed -n '1s/.*expect: \(-\{0,1\}[0-9]*\).*/\1/p' "$t")
		got=$($py "$here/emutest.py" $arch $ccom $flags "$t" 2>&1 | tail -1 | cut -c1-70)
		if [ "$got" = "$want" ]; then pass=$((pass+1))
		else fail=$((fail+1)); echo "FAIL [$flags] $(basename "$t"): want $want, got $got"; fi
	done
done
echo "emutest-all $arch: $pass passed, $fail failed"
