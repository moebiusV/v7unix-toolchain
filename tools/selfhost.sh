#!/bin/sh
#
# selfhost — compile the toolchain subset (cc/as/ld and friends) with froot1,
# the minimal self-hosting root.  froot1's contract is that it can rebuild the
# C toolchain from its own orig/ source using only the pieces inside froot1
# (the modern/ binaries).  froot2 will extend this to the whole /usr/src tree.
#
# Runs each piece in turn and reports PASS/FAIL per piece, so a single bad
# command does not stop the rest.  Exit status is 0 only if every piece built.
#
# Environment:
#   V7CHECK_ROOT      synthetic root dir (default: $TOPDIR/froot1)
#   V7CHECK_UNIXTREE  path to the unixtree checkout holding V7/usr/include

set -eu

TOPDIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
UNIXTREE=${V7CHECK_UNIXTREE:-"$TOPDIR/../../unixtree"}
FROOT=${V7CHECK_ROOT:-"$TOPDIR/froot1"}
INCLUDE="$FROOT/usr/include"
SRC="$FROOT/usr/src"

# --- assemble froot1 (tools + headers + source) -----------------------------
V7CHECK_ROOT="$FROOT" V7CHECK_UNIXTREE="$UNIXTREE" "$TOPDIR/tools/mkfroot.sh"

# --- point the toolchain at its pieces inside froot1 (same as v7check.sh) ---
export V7_C0="$FROOT/lib/c0"
export V7_C1="$FROOT/lib/c1"
export V7_C2="$FROOT/lib/c2"
export V7_CPP="$FROOT/lib/cpp"
export V7_AS="$FROOT/bin/as"
export V7_LD="$FROOT/bin/ld"
export V7_CRT0="$FROOT/lib/crt0.o"
export V7_LIB="$FROOT/lib"
export AS2="$FROOT/lib/as2"
export YACCPARSER="$FROOT/lib/yaccpar"

HOSTSH=$(command -v sh)
PATH="$FROOT/bin:$PATH"
export PATH

# --- run the whole build inside one synthetic root --------------------------
unshare -r -m -- "$HOSTSH" -c '
    set -u
    mount --bind "$1" /usr/include
    trap "umount /usr/include 2>/dev/null || true" EXIT
    shift
    SRC="$1"

    cd "$SRC/cmd"
    status=0

    step() {
        name="$1"; shift
        if "$@" >/tmp/selfhost-"$name".log 2>&1; then
            echo "PASS  $name"
        else
            echo "FAIL  $name  (see /tmp/selfhost-$name.log)"
            status=1
        fi
    }

    # the froot1 self-host subset: the C toolchain (cc/as/ld and friends).
    # froot2 pulls in the rest of the V7 command tree.
    for d in c as cpp make yacc; do
        step "$d" sh -c "cd $d && make"
    done

    # the toolchain single-file commands
    for f in ar cc ld; do
        step "$f" sh -c "cc -O -s -o $f $f.c"
    done

    exit $status
' selfhost "$INCLUDE" "$SRC"

rc=$?
if [ "$rc" -eq 0 ]; then
    echo "selfhost: toolchain subset build PASSED"
else
    echo "selfhost: some pieces FAILED (exit $rc)"
fi
exit $rc
