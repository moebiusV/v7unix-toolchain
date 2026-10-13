#!/bin/sh
#
# x86test -- compile a C file with the x86-64 pcc ccom, link it with a tiny
# 32-bit driver and run it: prints what `int test()` returns.
#
# usage: A64CCOM=/path/to/x86-64-unknown-linux-ccom x86test.sh [ccom flags] x.c
#   (needs gcc -m64, i.e. 32-bit libc and as --32, on the host)
#
set -eu
ccom=${A64CCOM:?set A64CCOM to the x86-64 ccom}
cpp=${X86CPP:-gcc -E -P -m64 -undef -D__x86_64__}
flags=
while [ $# -gt 1 ]; do flags="$flags $1"; shift; done
src=$1
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
$cpp "$src" > "$tmp/t.i"
$ccom $flags "$tmp/t.i" "$tmp/t.s"
cat > "$tmp/main.c" <<'EOT'
#include <stdio.h>
int test(void);
int main(void) { printf("%d\n", test()); return 0; }
EOT
gcc -m64  -w -o "$tmp/t" "$tmp/main.c" "$tmp/t.s"
"$tmp/t"
