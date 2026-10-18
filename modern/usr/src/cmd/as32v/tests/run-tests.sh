#!/bin/sh
# as32v smoke test: assemble a VAX function and check the a.out header (magic 0410 = 0x108, text 12 bytes)
# and the code bytes: .word 0; addl3 8(ap),4(ap),r0; ret
set -u
as=${1:-./as32v}
d=$(dirname "$0")
t=$(mktemp -d "${TMPDIR:-/tmp}/as32v.XXXXXX"); trap 'rm -rf "$t"' EXIT
"$as" -o "$t/a.o" "$d/addl.s" || { echo "FAIL: assemble"; exit 1; }
hdr=$(od -An -tx1 -N8 "$t/a.o" | tr -d ' \n')
[ "$hdr" = 080100000c000000 ] || { echo "FAIL: header $hdr"; exit 1; }
code=$(od -An -tx1 -j32 -N9 "$t/a.o" | tr -d ' \n')
[ "$code" = 0000c1ac08ac045004 ] || { echo "FAIL: code $code"; exit 1; }
echo "PASS: as32v"
