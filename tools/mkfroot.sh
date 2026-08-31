#!/bin/sh
#
# mkfroot — assemble froot/, the self-contained V7 cross-compilation root.
#
# froot/ is a synthetic copy of the V7 filesystem layout holding only the
# binaries the v7unix-toolchain package ships, plus the V7 headers and the
# reference source tree — everything needed to compile the whole V7 source
# tree with the original makefiles and pathnames:
#
#   modern/ host binaries  ->  froot/bin        (cc, as, ld, make, yacc, ar, cpp, sh)
#   modern/ passes + lib/  ->  froot/lib        (c0, c1, c2, cpp, as2, cvopt,
#                                                 crt0.o, libc.a, yaccpar)
#   unixtree V7 headers    ->  froot/usr/include (stdio.h, sys.s, ...)
#   orig/ source           ->  froot/usr/src     (the reference source tree)
#
# Binaries are *copied* (not symlinked) so froot/ is self-contained: it can be
# chrooted into, or tarred up for redistribution (make froot-dist).
#
# Environment:
#   V7CHECK_ROOT      synthetic root dir (default: $TOPDIR/froot)
#   V7CHECK_UNIXTREE  path to the unixtree checkout holding V7/usr/include

set -eu

TOPDIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
UNIXTREE=${V7CHECK_UNIXTREE:-"$TOPDIR/../../unixtree"}
FROOT=${V7CHECK_ROOT:-"$TOPDIR/froot"}
INCLUDE="$FROOT/usr/include"

MODERN="$TOPDIR/modern"
LIB="$TOPDIR/lib"

rm -rf "$FROOT"
mkdir -p "$FROOT/bin" "$FROOT/lib" "$FROOT/usr/include/sys"

# bin/: the tools the makefiles invoke by bare name.  Copied, not symlinked,
# so froot/ is self-contained (it could be chrooted into).
cp "$MODERN/usr/src/cmd/cc"        "$FROOT/bin/cc"
cp "$MODERN/usr/src/cmd/ld"        "$FROOT/bin/ld"
cp "$MODERN/usr/src/cmd/as/as"     "$FROOT/bin/as"
cp "$MODERN/usr/src/cmd/as/as2"    "$FROOT/bin/as2"
cp "$MODERN/usr/src/cmd/cpp/cpp"   "$FROOT/bin/cpp"
cp "$MODERN/usr/src/cmd/make/make" "$FROOT/bin/make"
cp "$MODERN/usr/src/cmd/yacc/yacc" "$FROOT/bin/yacc"
cp "$MODERN/usr/src/cmd/ar"        "$FROOT/bin/ar"
cp "$MODERN/usr/src/cmd/c/cvopt"   "$FROOT/bin/cvopt"
# NB: sh is deliberately NOT copied.  v7check leaves `sh` as the host's so the
# makefiles' shell commands run under a POSIX sh; the ported V7 sh is for the
# cross-compile (run-the-binary) phase, not the build phase.

# lib/: the passes + target runtime (cc/as/yacc resolve these via V7_*).
cp "$MODERN/usr/src/cmd/c/c0"    "$FROOT/lib/c0"
cp "$MODERN/usr/src/cmd/c/c1"    "$FROOT/lib/c1"
cp "$MODERN/usr/src/cmd/c/c2"    "$FROOT/lib/c2"
cp "$MODERN/usr/src/cmd/c/cvopt" "$FROOT/lib/cvopt"
cp "$MODERN/usr/src/cmd/cpp/cpp" "$FROOT/lib/cpp"
cp "$MODERN/usr/src/cmd/as/as2"  "$FROOT/lib/as2"
cp "$MODERN/usr/src/cmd/yacc/yaccpar" "$FROOT/lib/yaccpar"
for f in crt0.o fcrt0.o mcrt0.o fmcrt0.o libc.a; do
    cp "$LIB/$f" "$FROOT/lib/$f"
done

# usr/src/: the reference source tree (orig/usr/src).
cp -r "$TOPDIR/orig/usr/src" "$FROOT/usr/src"

# usr/include/: gunzip the V7 headers out of the unixtree checkout.
if [ ! -d "$UNIXTREE/V7/usr/include" ]; then
    echo "mkfroot: V7 headers not found under $UNIXTREE/V7/usr/include" >&2
    echo "         set V7CHECK_UNIXTREE to the unixtree checkout" >&2
    exit 2
fi
for f in "$UNIXTREE"/V7/usr/include/*.gz; do
    [ -e "$f" ] || continue
    gunzip -c "$f" > "$INCLUDE/$(basename "$f" .gz)"
done
for f in "$UNIXTREE"/V7/usr/include/sys/*.gz; do
    [ -e "$f" ] || continue
    gunzip -c "$f" > "$INCLUDE/sys/$(basename "$f" .gz)"
done

# README: how to point the self-contained toolchain at its own pieces.
cat > "$FROOT/README" <<'EOF'
froot — a self-contained V7 cross-compilation environment.

Point the toolchain at its own pieces, then build V7 source with its
original makefiles and pathnames:

    export FROOT=$PWD
    export PATH=$FROOT/bin:$PATH
    export V7_C0=$FROOT/lib/c0
    export V7_C1=$FROOT/lib/c1
    export V7_C2=$FROOT/lib/c2
    export V7_CPP=$FROOT/lib/cpp
    export V7_AS=$FROOT/bin/as
    export V7_LD=$FROOT/bin/ld
    export V7_CRT0=$FROOT/lib/crt0.o
    export V7_LIB=$FROOT/lib
    export AS2=$FROOT/lib/as2
    export YACCPARSER=$FROOT/lib/yaccpar

    cd usr/src/cmd/cat && make          # or any V7 command source dir
EOF

echo "mkfroot: assembled froot/ ($FROOT)"
