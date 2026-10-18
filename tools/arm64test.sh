#!/bin/sh
#
# arm64test -- compile a C test with the aarch64 pcc ccom and run it on an emulated
# AArch64 (arm64run.py).  The file defines int test(void); its value is printed.
#
# usage: ARM64CCOM=.../aarch64-unknown-linux-ccom [ARM64PY=python with unicorn/keystone] arm64test.sh [ccom flags] file.c
#
set -u
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
ccom=${ARM64CCOM:?set ARM64CCOM}
py=${ARM64PY:-/tmp/claude-1000/emuenv/bin/python}
for last; do :; done
src=$last
n=$#
flags=
i=1
for a; do
	[ $i -lt $n ] && flags="$flags $a"
	i=$((i+1))
done
W=$(mktemp -d "${TMPDIR:-/tmp}/arm64test.XXXXXX")
trap 'rm -rf "$W"' EXIT
gcc -E -P -undef -D__aarch64__ -Dunix "$src" > "$W/i" 2>/dev/null || { echo "cpp failed"; exit 1; }
if ! $ccom $flags "$W/i" "$W/s" >"$W/err" 2>&1; then
	head -2 "$W/err"; exit 1
fi
cd / && "$py" "$here/arm64run.py" "$W/s" 2>&1 | tail -1
