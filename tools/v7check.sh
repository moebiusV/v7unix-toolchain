#!/bin/sh
#
# v7check — build V7 source with the modern/ ported toolchain, inside a
# synthetic V7 root.  Runs the *original* makefiles (or any command) against
# the *original* source; the ported tools do the work.
#
# Why a fake root at all: the V7 makefiles are not self-contained.  The
# preprocessor searches "/usr/include" for <stdio.h> &c, and as/makefile reads
# "/usr/include/sys.s" literally.  A bare PATH prefix cannot intercept those,
# so we materialise a synthetic V7 /usr/include and bind-mount it over the
# host's for the duration of the build.
#
# Mechanism: `unshare -r -m` (Linux user + mount namespace) — a real uid-0
# inside the namespace, enough for bind mounts and chmod/mknod, without root.
#
#   fakeroot is deliberately NOT used here.  fakeroot and `unshare -r -m` do
#   not compose: `fakeroot unshare` breaks the uid_map write (libfakeroot
#   intercepts it), and `unshare fakeroot` breaks chown (EINVAL on unmapped
#   ids).  A compile-only build needs path redirection (fakeroot cannot do it —
#   its chroot is faked), not chown-to-arbitrary-uid.  fakeroot remains the
#   documented fallback for privilege-only steps (mkfs device nodes, ar archive
#   ownership) on hosts where unprivileged user namespaces are disabled.
#
# Usage (run from a V7 source directory, or pass a command):
#   cd orig/usr/src/cmd/cpp && ../../tools/v7check.sh all     # make "all"
#   tools/v7check.sh cc -o /tmp/cc.out cc.c                    # raw cc
#
# Environment:
#   V7CHECK_ROOT      staging dir for the synthetic root (default: mktemp)
#   V7CHECK_UNIXTREE  path to the unixtree checkout holding V7/usr/include
#   V7CHECK_KEEP      if set, do not remove the staging dir on exit

set -eu

TOPDIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
UNIXTREE=${V7CHECK_UNIXTREE:-"$TOPDIR/../../unixtree"}
ROOT=${V7CHECK_ROOT:-"$(mktemp -d "${TMPDIR:-/tmp}/v7check.XXXXXX")"}
INCLUDE="$ROOT/usr/include"

MODERN="$TOPDIR/modern"
LIB="$TOPDIR/lib"

# --- synthetic V7 /usr/include (gunzipped from the unixtree checkout) ---------
stage_include() {
	mkdir -p "$INCLUDE/sys"
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
	echo "v7check: staged V7 headers into $INCLUDE"
}

# --- point the cc driver at the built passes + target runtime -----------------
# The modern cc driver resolves its passes from V7_* (or its compiled-in
# V7_LIBEXECDIR); the originals hardcoded /lib/c0, /bin/as, /bin/ld.
export V7_C0="$MODERN/usr/src/cmd/c/c0"
export V7_C1="$MODERN/usr/src/cmd/c/c1"
export V7_C2="$MODERN/usr/src/cmd/c/c2"
export V7_CPP="$MODERN/usr/src/cmd/cpp/cpp"
export V7_AS="$MODERN/usr/src/cmd/as/as"
export V7_LD="$MODERN/usr/src/cmd/ld"
export V7_CRT0="$LIB/crt0.o"
export V7_LIB="$LIB"

# --- PATH: bare `cc`/`as`/`ld`/`make`/`yacc` in the makefiles resolve here ----
# (sh/mv/rm/cp/cmp are still the host's until the modern/ ports of those land.)
PATH="$MODERN/usr/src/cmd:$MODERN/usr/src/cmd/as:$PATH"
export PATH

stage_include

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
if [ -z "${V7CHECK_KEEP:-}" ]; then rm -rf "$ROOT"; fi
exit $rc
