#!/bin/sh
#
# ppctest -- compile a C file with the 32-bit PowerPC pcc ccom, link it with a tiny
# driver and run it under qemu-ppc: prints what `int test()` returns.
#
# usage: PPCCCOM=/path/to/powerpc-unknown-linux-ccom ppctest.sh [ccom flags] x.c
#   (needs powerpc-linux-gnu-gcc with a static libc, and qemu-ppc, on the host)
#
set -eu
ccom=${PPCCCOM:-$HOME/claude/corpora/builds/ours/powerpc/ccom}
cc=${PPCGCC:-powerpc-linux-gnu-gcc}
flags=
while [ $# -gt 1 ]; do flags="$flags $1"; shift; done
src=$1
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
$cc -E -P -undef -D__powerpc__ -D_BIG_ENDIAN "$src" > "$tmp/t.i"
$ccom $flags "$tmp/t.i" "$tmp/t.s"
cat > "$tmp/main.c" <<'EOT'
#include <stdio.h>
int test(void);
int main(void) { printf("%d\n", test()); return 0; }
EOT
$cc -static -w -o "$tmp/t" "$tmp/main.c" "$tmp/t.s"
${PPCQEMU:-qemu-ppc} "$tmp/t"
