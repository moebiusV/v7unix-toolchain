#!/bin/sh
# Golden-output test for the PDP-11 linker.
#   run-tests.sh /path/to/ld /path/to/as
#
# Assembles main.s + foo.s (with `as`), links them (with `ld`), and compares
# the resulting a.out byte-for-byte against tests/a.out — the golden, verified
# byte-identical to V7's own `ld` under simh.

set -u
here=$(cd "$(dirname "$0")" && pwd)
LD=${1:-"$here/../ld"}
AS=${2:-"$here/../as/as"}

# Resolve to absolute paths so `as` (pass 1) can locate its sibling `as2`
# (as1.c finds pass 2 next to argv[0]).
for v in LD AS; do
	eval "p=\$$v"
	case "$p" in
		/*) ;;
		*)  eval "$v=$(cd "$(dirname "$p")" && pwd)/$(basename "$p")" ;;
	esac
done

tmp=$(mktemp -d "${TMPDIR:-/tmp}/ldcheck.XXXXXX") || exit 1
trap 'rm -rf "$tmp"' 0 1 2 3 15

(cd "$tmp" && \
	"$AS" -u -o main.o "$here/main.s" && \
	"$AS" -u -o foo.o "$here/foo.s" && \
	"$LD" main.o foo.o) || {
	echo "FAIL  ld (assemble/link exited nonzero)"
	exit 1
}
if cmp -s "$tmp/a.out" "$here/a.out"; then
	echo "ok    ld (main.o + foo.o -> a.out)"
else
	echo "FAIL  ld (a.out differs from golden)"
	exit 1
fi
