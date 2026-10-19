#!/bin/sh
#
# arm64abi -- check the AArch64 calling convention against clang.  One C file is compiled by
# pcc, another by clang; they call each other, the object code is joined and run on the
# emulated AArch64 (arm64run.py).  The pcc file defines int test(void); its value is printed.
#
# usage: ARM64CCOM=... [ARM64PY=...] arm64abi.sh [ccom flags] pcc-side.c clang-side.c
#
set -u
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
ccom=${ARM64CCOM:?set ARM64CCOM}
py=${ARM64PY:-/tmp/claude-1000/emuenv/bin/python}
n=$#
[ $n -ge 2 ] || { echo "usage: arm64abi.sh [flags] pcc.c clang.c"; exit 2; }
i=1; flags=
for a; do
	[ $i -lt $((n-1)) ] && flags="$flags $a"
	[ $i -eq $((n-1)) ] && pccsrc=$a
	[ $i -eq $n ] && clsrc=$a
	i=$((i+1))
done
W=$(mktemp -d "${TMPDIR:-/tmp}/arm64abi.XXXXXX")
trap 'rm -rf "$W"' EXIT
gcc -E -P -undef -D__aarch64__ -Dunix "$pccsrc" > "$W/i" 2>/dev/null || { echo "cpp failed"; exit 1; }
$ccom $flags "$W/i" "$W/p.s" >"$W/err" 2>&1 || { head -2 "$W/err"; exit 1; }
clang --target=aarch64-linux-gnu -std=gnu99 -w -O1 -fno-asynchronous-unwind-tables -fno-unwind-tables \
    -fno-stack-protector -S -o "$W/c.s" "$clsrc" || { echo "clang failed"; exit 1; }
grep -v '^\s*\.\(file\|type\|size\|ident\|addrsig\|section\)' "$W/c.s" |
    "$py" -c '
import re, struct, sys
# keystone has no floating-point immediate: fmov d1, #2.0  becomes  mov x16, #bits ; fmov d1, x16
for l in sys.stdin:
    m = re.match(r"\s*fmov\s+([ds])(\d+),\s*#([-+0-9.eE]+)", l)
    if m:
        k, r, v = m.group(1), m.group(2), float(m.group(3))
        bits = struct.unpack("<Q", struct.pack("<d", v))[0] if k == "d" else struct.unpack("<I", struct.pack("<f", v))[0]
        if k == "d":
            sys.stdout.write("\tmov x16, #%d\n\tfmov d%s, x16\n" % (bits, r))
        else:
            sys.stdout.write("\tmov w16, #%d\n\tfmov s%s, w16\n" % (bits, r))
    else:
        sys.stdout.write(l)
' > "$W/c2.s"
cat "$W/p.s" "$W/c2.s" > "$W/s"
cd / && "$py" "$here/arm64run.py" "$W/s" 2>&1 | tail -1
