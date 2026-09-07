#!/bin/sh
#
# mkfroot — assemble froot1/, the self-contained V7 cross-compilation root.
#
# froot1/ is a synthetic copy of the V7 filesystem layout holding only the
# binaries the v7unix-toolchain package ships, plus the V7 headers and the
# reference source tree — everything needed to compile the whole V7 source
# tree with the original makefiles and pathnames:
#
#   modern/ host binaries  ->  froot1/bin        (cc, as, ld, make, yacc, ar, cpp, sh)
#   modern/ passes + lib/  ->  froot1/lib        (c0, c1, c2, cpp, as2, cvopt,
#                                                 crt0.o, libc.a, yaccpar)
#   orig/ headers          ->  froot1/usr/include (stdio.h, sys.s, ...)
#   orig/ source           ->  froot1/usr/src     (the reference source tree)
#
# Binaries are *copied* (not symlinked) so froot1/ is self-contained: it can be
# chrooted into, or tarred up for redistribution (make froot1-dist).
#
# orig/ is the single V7 source of truth: source, reference binaries and libs,
# and headers all live there, so no external checkout is needed.
#
# Environment:
#   V7CHECK_ROOT      synthetic root dir (default: $TOPDIR/froot1)

set -eu

TOPDIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
FROOT=${V7CHECK_ROOT:-"$TOPDIR/froot1"}

MODERN="$TOPDIR/modern"
LIB="$TOPDIR/lib"

rm -rf "$FROOT"
mkdir -p "$FROOT/bin" "$FROOT/lib" "$FROOT/usr"

# bin/: the tools the makefiles invoke by bare name.  Copied, not symlinked,
# so froot1/ is self-contained (it could be chrooted into).
cp "$MODERN/usr/src/cmd/cc"        "$FROOT/bin/cc"
cp "$MODERN/usr/src/cmd/ld"        "$FROOT/bin/ld"
cp "$MODERN/usr/src/cmd/as/as"     "$FROOT/bin/as"
cp "$MODERN/usr/src/cmd/as/as2"    "$FROOT/bin/as2"
cp "$MODERN/usr/src/cmd/cpp/cpp"   "$FROOT/bin/cpp"
cp "$MODERN/usr/src/cmd/make/make" "$FROOT/bin/make"
cp "$MODERN/usr/src/cmd/yacc/yacc" "$FROOT/bin/yacc"
cp "$MODERN/usr/src/cmd/ar"        "$FROOT/bin/ar"
cp "$MODERN/usr/src/cmd/c/cvopt"   "$FROOT/bin/cvopt"
# the makefile commands (phase-1 ports): sh, cp, mv, rm, cmp — so the V7
# makefiles run self-hosted inside froot1 rather than leaning on the host's.
for t in cp mv rm cmp; do
    cp "$MODERN/usr/src/cmd/$t" "$FROOT/bin/$t"
done
cp "$MODERN/usr/src/cmd/sh/sh" "$FROOT/bin/sh"

# lib/: the passes + target runtime (cc/as/yacc resolve these via V7_*).
cp "$MODERN/usr/src/cmd/c/c0"    "$FROOT/lib/c0"
cp "$MODERN/usr/src/cmd/c/c1"    "$FROOT/lib/c1"
cp "$MODERN/usr/src/cmd/c/c2"    "$FROOT/lib/c2"
cp "$MODERN/usr/src/cmd/c/cvopt" "$FROOT/lib/cvopt"
cp "$MODERN/usr/src/cmd/cpp/cpp" "$FROOT/lib/cpp"
cp "$MODERN/usr/src/cmd/as/as2"  "$FROOT/lib/as2"
cp "$MODERN/usr/src/cmd/yacc/yaccpar" "$FROOT/lib/yaccpar"
# The target runtime (crt0.o, ... libc.a) is a build product, and libc.a in
# particular is built *through* this script: lib/Makefile builds libc.a by
# running v7check.sh, which assembles froot1/ by calling back into mkfroot.sh.
# On a clean clone those files do not exist yet while that build is running, so
# copy only what is already built; a subsequent `make froot1` (after `all`)
# picks up the rest.
for f in crt0.o fcrt0.o mcrt0.o fmcrt0.o libc.a; do
    [ -f "$LIB/$f" ] && cp "$LIB/$f" "$FROOT/lib/$f"
done

# usr/src/: the reference source tree (orig/usr/src).
cp -r "$TOPDIR/orig/usr/src" "$FROOT/usr/src"

# usr/include/: the V7 headers from orig/ (the source of truth).
cp -r "$TOPDIR/orig/usr/include" "$FROOT/usr/include"

# README: how to point the self-contained toolchain at its own pieces.
cat > "$FROOT/README" <<'EOF'
froot1 — the minimal self-hosting V7 cross-compilation environment.

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

echo "mkfroot: assembled froot1/ ($FROOT)"
