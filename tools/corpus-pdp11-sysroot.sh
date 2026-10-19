#!/bin/sh
#
# corpus-pdp11-sysroot -- the C library headers for the PDP-11 half of the size corpus.
#
# There is no musl for the PDP-11, and the i386 headers say int is 32 bits: uint32_t is unsigned int, INT_MAX is
# 0x7fffffff.  Compiled for a 16-bit int that makes every uint32_t a 16-bit type, and code like  x >> 24  on one
# is undefined (gcc deletes it, pcc emits a shift that means nothing).  Neither result is a fair measurement.
# This copies the i386 headers and gives them the PDP-11's types: int 16 bits, long 32, long long 64, pointers
# 16 bits (the i386 headers' _Addr is int, which is then right).
#
# usage: corpus-pdp11-sysroot.sh [CORPORA-DIR]       (default ~/claude/corpora)
#
set -eu
root=${1:-$HOME/claude/corpora}
src=$root/sysroot/i386
dst=$root/sysroot/pdp11
rm -rf "$dst"
cp -r "$src" "$dst"
inc=$dst/include

# the fixed-width types
sed -i 's/^typedef signed int      int32_t;/typedef signed long     int32_t;/; s/^typedef unsigned int    uint32_t;/typedef unsigned long   uint32_t;/' "$inc/bits/alltypes.h"
grep -q 'typedef signed long     int32_t' "$inc/bits/alltypes.h" || { echo "int32_t not patched"; exit 1; }
grep -q 'typedef unsigned long   uint32_t' "$inc/bits/alltypes.h" || { echo "uint32_t not patched"; exit 1; }

# the fastest types of at least 16 and 32 bits, and the pointer-sized limits
sed -i 's/^typedef int32_t int_fast16_t;/typedef int16_t int_fast16_t;/; s/^typedef uint32_t uint_fast16_t;/typedef uint16_t uint_fast16_t;/;
        s/^#define INT_FAST16_MIN  INT32_MIN/#define INT_FAST16_MIN  INT16_MIN/; s/^#define INT_FAST16_MAX  INT32_MAX/#define INT_FAST16_MAX  INT16_MAX/;
        s/^#define UINT_FAST16_MAX UINT32_MAX/#define UINT_FAST16_MAX UINT16_MAX/;
        s/^#define INTPTR_MIN      INT32_MIN/#define INTPTR_MIN      INT16_MIN/; s/^#define INTPTR_MAX      INT32_MAX/#define INTPTR_MAX      INT16_MAX/;
        s/^#define UINTPTR_MAX     UINT32_MAX/#define UINTPTR_MAX     UINT16_MAX/;
        s/^#define PTRDIFF_MIN     INT32_MIN/#define PTRDIFF_MIN     INT16_MIN/; s/^#define PTRDIFF_MAX     INT32_MAX/#define PTRDIFF_MAX     INT16_MAX/;
        s/^#define SIZE_MAX        UINT32_MAX/#define SIZE_MAX        UINT16_MAX/' "$inc/bits/stdint.h"
# a long constant must be a long on a machine whose int is 16 bits
sed -i 's/^#define INT32_MAX  (0x7fffffff)/#define INT32_MAX  (0x7fffffffL)/; s/^#define UINT32_MAX (0xffffffffu)/#define UINT32_MAX (0xffffffffuL)/' "$inc/stdint.h"

# limits.h
sed -i 's/^#define INT_MIN  (-1-0x7fffffff)/#define INT_MIN  (-1-0x7fff)/; s/^#define INT_MAX  0x7fffffff/#define INT_MAX  0x7fff/; s/^#define UINT_MAX 0xffffffffU/#define UINT_MAX 0xffffU/' "$inc/limits.h"

echo "wrote $dst"
