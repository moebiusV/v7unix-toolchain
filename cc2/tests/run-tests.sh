#!/bin/sh
# Golden-output test for the peephole optimizer (cc2).
#   run-tests.sh /path/to/cc2
#
# tests/test.s is V7 c1's output for a small function; tests/test.opt.s is
# V7's own c2 output for the same input (verified byte-identical under simh).
# This checks that the host cc2 reproduces V7's c2 exactly.

set -u
here=$(cd "$(dirname "$0")" && pwd)
CC2=${1:-"$here/../cc2"}

tmp=$(mktemp -d "${TMPDIR:-/tmp}/cc2check.XXXXXX") || exit 1
trap 'rm -rf "$tmp"' 0 1 2 3 15

"$CC2" "$here/test.s" "$tmp/out.s" || {
	echo "FAIL  cc2 (exited nonzero)"
	exit 1
}
if cmp -s "$tmp/out.s" "$here/test.opt.s"; then
	echo "ok    cc2 (test.s -> test.opt.s)"
else
	echo "FAIL  cc2 (output differs from golden)"
	exit 1
fi
