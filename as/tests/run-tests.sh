#!/bin/sh
# Golden-output test for the PDP-11 assembler.
#   run-tests.sh /path/to/as
#
# `as2' must sit next to `as' (pass 1 locates it from argv[0]).  Each
# tests/N.s is assembled and its a.out compared byte-for-byte against
# tests/N.aout.  Exit status is nonzero if any test fails.

set -u

here=$(cd "$(dirname "$0")" && pwd)
AS=${1:-"$here/../as"}

# Resolve $AS to an absolute path so the cd below cannot break it, and so
# argv[0] carries a '/' (which as1.c uses to find the sibling as2).
case "$AS" in
	/*) ;;
	*)  AS="$(cd "$(dirname "$AS")" && pwd)/$(basename "$AS")" ;;
esac

tmp=$(mktemp -d "${TMPDIR:-/tmp}/ascheck.XXXXXX") || exit 1
trap 'rm -rf "$tmp"' 0 1 2 3 15

pass=0
fail=0
for src in "$here"/*.s; do
	base=$(basename "$src" .s)
	golden="$here/$base.aout"
	[ -f "$golden" ] || { echo "skip  $base (no golden a.out)"; continue; }

	rm -f "$tmp/a.out" /tmp/atm1a /tmp/atm2a /tmp/atm3a
	if (cd "$tmp" && "$AS" "$src" >/dev/null 2>&1); then
		if cmp -s "$tmp/a.out" "$golden"; then
			echo "ok    $base"
			pass=$((pass+1))
		else
			echo "FAIL  $base (a.out differs from golden)"
			fail=$((fail+1))
		fi
	else
		echo "FAIL  $base (assembler exited nonzero)"
		fail=$((fail+1))
	fi
done

echo "----"
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ] || exit 1
