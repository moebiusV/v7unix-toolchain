#!/bin/sh
#
# v7check — build V7 source with the modern/ ported toolchain inside a
# synthetic V7 root.
#
# froot/ is that root, assembled on demand.  It combines the two halves of the
# repo into V7's own filesystem layout:
#
#   modern/ host binaries  ->  froot/bin        (cc, as, ld, make, yacc, ar, cpp)
#   modern/ passes + lib/  ->  froot/lib        (c0, c1, c2, cpp, as2, cvopt,
#                                                 crt0.o, libc.a, yaccpar)
#   unixtree V7 headers    ->  froot/usr/include (stdio.h, sys.s, ...)
#   orig/ source           ->  froot/usr/src     (the reference source tree)
#
# Every entry is a symlink back to the real tree, so no chroot is needed: the
# dynamically-linked tools keep their own glibc.  The build then runs under
# `unshare -r -m` — a real uid-0 in a fresh user + mount namespace, the
# "fakeroot" session — which bind-mounts froot/usr/include over /usr/include
# so the makefiles' hardcoded `#include <...>` and "/usr/include/sys.s" resolve.
#
# fakeroot(1) itself is deliberately NOT used here.  fakeroot and
# `unshare -r -m` do not compose: `fakeroot unshare` breaks the uid_map write
# (libfakeroot intercepts it), and `unshare fakeroot` breaks chown (EINVAL on
# unmapped ids).  A compile-only build needs path redirection (fakeroot cannot
# do it — its chroot is faked), not chown-to-arbitrary-uid.  fakeroot remains
# the documented fallback for privilege-only steps (mkfs device nodes, ar
# archive ownership) on hosts where unprivileged user namespaces are disabled.
#
# Usage (run from a V7 source directory, or pass a command):
#   cd froot/usr/src/cmd/cpp && ../../../../tools/v7check.sh all   # make "all"
#   tools/v7check.sh cc -o /tmp/cc.out cc.c                         # raw cc
#
# Environment:
#   V7CHECK_ROOT      synthetic root dir (default: $TOPDIR/froot)
#   V7CHECK_UNIXTREE  path to the unixtree checkout holding V7/usr/include
#   V7CHECK_KEEP      if set, do not remove froot/ on exit

set -eu

TOPDIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
UNIXTREE=${V7CHECK_UNIXTREE:-"$TOPDIR/../../unixtree"}
FROOT=${V7CHECK_ROOT:-"$TOPDIR/froot"}
INCLUDE="$FROOT/usr/include"

MODERN="$TOPDIR/modern"
LIB="$TOPDIR/lib"

# --- assemble froot/ on demand -------------------------------------------------
rm -rf "$FROOT"
mkdir -p "$FROOT/bin" "$FROOT/lib" "$FROOT/usr/include/sys"

# bin/: the tools the makefiles invoke by bare name.
ln -s "$MODERN/usr/src/cmd/cc"        "$FROOT/bin/cc"
ln -s "$MODERN/usr/src/cmd/ld"        "$FROOT/bin/ld"
ln -s "$MODERN/usr/src/cmd/as/as"     "$FROOT/bin/as"
ln -s "$MODERN/usr/src/cmd/as/as2"    "$FROOT/bin/as2"
ln -s "$MODERN/usr/src/cmd/cpp/cpp"   "$FROOT/bin/cpp"
ln -s "$MODERN/usr/src/cmd/make/make" "$FROOT/bin/make"
ln -s "$MODERN/usr/src/cmd/yacc/yacc" "$FROOT/bin/yacc"
ln -s "$MODERN/usr/src/cmd/ar"        "$FROOT/bin/ar"
ln -s "$MODERN/usr/src/cmd/c/cvopt"   "$FROOT/bin/cvopt"

# lib/: the passes + target runtime (cc/as/yacc resolve these via V7_*).
ln -s "$MODERN/usr/src/cmd/c/c0"    "$FROOT/lib/c0"
ln -s "$MODERN/usr/src/cmd/c/c1"    "$FROOT/lib/c1"
ln -s "$MODERN/usr/src/cmd/c/c2"    "$FROOT/lib/c2"
ln -s "$MODERN/usr/src/cmd/c/cvopt" "$FROOT/lib/cvopt"
ln -s "$MODERN/usr/src/cmd/cpp/cpp" "$FROOT/lib/cpp"
ln -s "$MODERN/usr/src/cmd/as/as2"  "$FROOT/lib/as2"
ln -s "$MODERN/usr/src/cmd/yacc/yaccpar" "$FROOT/lib/yaccpar"
for f in crt0.o fcrt0.o mcrt0.o fmcrt0.o libc.a; do
	ln -s "$LIB/$f" "$FROOT/lib/$f"
done

# usr/src/: the reference source tree (orig/usr/src).
ln -s "$TOPDIR/orig/usr/src" "$FROOT/usr/src"

# usr/include/: gunzip the V7 headers out of the unixtree checkout.
if [ ! -d "$UNIXTREE/V7/usr/include" ]; then
	echo "v7check: V7 headers not found under $UNIXTREE/V7/usr/include" >&2
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
echo "v7check: assembled froot/ ($FROOT)"

# --- point the tools at the passes + runtime inside froot/ ---------------------
# The modern cc driver resolves its passes from V7_* (or its compiled-in
# V7_LIBEXECDIR); the originals hardcoded /lib/c0, /bin/as, /bin/ld.  Here we
# point everything at the froot/ copies so the whole toolchain is self-contained.
export V7_C0="$FROOT/lib/c0"
export V7_C1="$FROOT/lib/c1"
export V7_C2="$FROOT/lib/c2"
export V7_CPP="$FROOT/lib/cpp"
export V7_AS="$FROOT/bin/as"
export V7_LD="$FROOT/bin/ld"
export V7_CRT0="$FROOT/lib/crt0.o"
export V7_LIB="$FROOT/lib"
# pass-1 `as` execs pass-2 `as2`; yacc reads its parser skeleton.  When a
# makefile invokes a bare `as`/`yacc` (argv[0] has no '/'), each falls back to
# its compiled-in path — point them at the froot/ copies instead.
export AS2="$FROOT/lib/as2"
export YACCPARSER="$FROOT/lib/yaccpar"

# --- PATH: bare `cc`/`as`/`ld`/`make`/`yacc`/`ar`/`cvopt` in the makefiles
# resolve here.  (sh is still the host's; the ported sh runs only in the
# cross-compile phase.)
PATH="$FROOT/bin:$PATH"
export PATH

if [ "$#" -eq 0 ]; then set -- make; fi

echo "v7check: entering synthetic V7 root (unshare -r -m)"
echo "v7check: running: $*"

unshare -r -m -- sh -c '
	set -eu
	mount --bind "$1" /usr/include
	trap "umount /usr/include 2>/dev/null || true" EXIT
	shift
	exec "$@"
' v7check "$INCLUDE" "$@"

rc=$?
if [ -z "${V7CHECK_KEEP:-}" ]; then rm -rf "$FROOT"; fi
exit $rc
