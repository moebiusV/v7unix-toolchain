#!/bin/sh
#
# x86test -- compile a C file with the i386 pcc ccom, link it with a tiny
# 32-bit driver and run it: prints what `int test()` returns.
#
# usage: X86CCOM=/path/to/i386-unknown-linux-ccom x86test.sh [ccom flags] x.c
#   (needs gcc -m32, i.e. 32-bit libc and as --32, on the host)
#
set -eu
ccom=${X86CCOM:?set X86CCOM to the i386 ccom}
cpp=${X86CPP:-gcc -E -P -m32 -undef -D__i386__}
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
gcc -m32 -no-pie -w -o "$tmp/t" "$tmp/main.c" "$tmp/t.s"
"$tmp/t"
