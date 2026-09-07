#!/bin/sh
#
# mkfroot — assemble froot/, the self-contained V7 cross-compilation root.
#
# froot/ is a synthetic copy of the V7 filesystem layout holding the seed
# toolchain, plus the V7 headers and the reference source tree — everything
# needed to compile the whole V7 source tree with the original makefiles and
# pathnames:
#
#   seed binaries          ->  froot/bin        (cc, as, ld, make, yacc, ar, cpp, sh)
#   seed passes + runtime  ->  froot/lib        (c0, c1, c2, cpp, as2, cvopt,
#                                                 crt0.o, libc.a, yaccpar)
#   orig/ headers          ->  froot/usr/include (stdio.h, sys.s, ...)
#   orig/ source           ->  froot/usr/src     (the reference source tree)
#
# Usage: mkfroot.sh [c17|c99]   (default c17)
#   c17  seed from modern/ (the C17 host toolchain; the froot runs on Linux)
#   c99  seed from pcc99/  (the C99 tier pcc built; the froot runs under simh)
#
# Binaries are *copied* (not symlinked) so froot/ is self-contained: it can be
# chrooted into, or tarred up for redistribution (make froot-dist).
#
# orig/ is the single V7 source of truth: source, reference binaries and libs,
# and headers all live there, so no external checkout is needed.
#
# Environment:
#   V7CHECK_ROOT      synthetic root dir (default: $TOPDIR/froot)

set -eu

# Mode selects the seed tier:
#   c17  seed from modern/ (the C17 host toolchain; the froot runs on Linux)
#   c99  seed from pcc99/  (the C99 tier pcc built; the froot runs under simh)
MODE=${1:-c17}
case "$MODE" in
c17|modern) MODE=c17 ;;
c99|pcc)    MODE=c99 ;;
*) echo "mkfroot: bad mode '$1' (want c17 or c99)" >&2; exit 2 ;;
esac

# Scope selects how much of the tree the root must rebuild:
#   toolchain  the minimal set that rebuilds the toolchain itself (froot)
#   all        the whole /usr/src tree, `make all` (froot2)
SCOPE=${2:-toolchain}
case "$SCOPE" in
toolchain|tc) SCOPE=toolchain ;;
all)          SCOPE=all ;;
*) echo "mkfroot: bad scope '$2' (want toolchain or all)" >&2; exit 2 ;;
esac

TOPDIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
FROOT=${V7CHECK_ROOT:-"$TOPDIR/froot"}

MODERN="$TOPDIR/modern"
PCC99="$TOPDIR/pcc99"
LIB="$TOPDIR/lib"

rm -rf "$FROOT"
mkdir -p "$FROOT/bin" "$FROOT/lib" "$FROOT/usr"
# "all" scope: the extra dirs the original makefiles reference even when empty.
[ "$SCOPE" = all ] && mkdir -p "$FROOT/etc"

# bin/: the tools the makefiles invoke by bare name.  Copied, not symlinked,
# so froot/ is self-contained (it could be chrooted into).
if [ "$MODE" = c17 ]; then
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
    # makefiles run self-hosted inside froot rather than leaning on the host's.
    for t in cp mv rm cmp; do
        cp "$MODERN/usr/src/cmd/$t" "$FROOT/bin/$t"
    done
    cp "$MODERN/usr/src/cmd/sh/sh" "$FROOT/bin/sh"
else
    # pcc99/: the C99 tier's PDP-11 a.out binaries (run under simh/V7).
    for t in cc ld ar cp mv rm cmp make yacc sh as; do
        cp "$PCC99/bin/$t" "$FROOT/bin/$t"
    done
    # cvopt is a host build tool the pcc99 tier has no target build of yet.
    cp "$MODERN/usr/src/cmd/c/cvopt" "$FROOT/bin/cvopt"
fi

# lib/: the passes + target runtime (cc/as/yacc resolve these via V7_*).
if [ "$MODE" = c17 ]; then
    cp "$MODERN/usr/src/cmd/c/c0"    "$FROOT/lib/c0"
    cp "$MODERN/usr/src/cmd/c/c1"    "$FROOT/lib/c1"
    cp "$MODERN/usr/src/cmd/c/c2"    "$FROOT/lib/c2"
    cp "$MODERN/usr/src/cmd/c/cvopt" "$FROOT/lib/cvopt"
    cp "$MODERN/usr/src/cmd/cpp/cpp" "$FROOT/lib/cpp"
    cp "$MODERN/usr/src/cmd/as/as2"  "$FROOT/lib/as2"
    cp "$MODERN/usr/src/cmd/yacc/yaccpar" "$FROOT/lib/yaccpar"
else
    for f in c0 c1 c2 cpp as2; do
        cp "$PCC99/lib/$f" "$FROOT/lib/$f"
    done
    cp "$MODERN/usr/src/cmd/c/cvopt"      "$FROOT/lib/cvopt"
    cp "$MODERN/usr/src/cmd/yacc/yaccpar" "$FROOT/lib/yaccpar"
fi
# The target runtime (crt0.o, ... libc.a) is a build product, and libc.a in
# particular is built *through* this script: lib/Makefile builds libc.a by
# running v7check.sh, which assembles froot/ by calling back into mkfroot.sh.
# On a clean clone those files do not exist yet while that build is running, so
# copy only what is already built; a subsequent `make froot` (after `all`)
# picks up the rest.
for f in crt0.o fcrt0.o mcrt0.o fmcrt0.o libc.a; do
    [ -f "$LIB/$f" ] && cp "$LIB/$f" "$FROOT/lib/$f"
done

# usr/src/: the reference source tree (orig/usr/src).
cp -r "$TOPDIR/orig/usr/src" "$FROOT/usr/src"

# usr/include/: the V7 headers from orig/ (the source of truth).
cp -r "$TOPDIR/orig/usr/include" "$FROOT/usr/include"

# etc/: the reference config, for the "all" scope (orig/ is the source of truth).
if [ "$SCOPE" = all ] && [ -d "$TOPDIR/orig/etc" ]; then
    cp -r "$TOPDIR/orig/etc" "$FROOT/etc"
fi

# README: how to point the self-contained toolchain at its own pieces.
cat > "$FROOT/README" <<'EOF'
froot — the minimal self-hosting V7 cross-compilation environment.

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

echo "mkfroot: assembled froot/ ($FROOT, $MODE/$SCOPE)"
