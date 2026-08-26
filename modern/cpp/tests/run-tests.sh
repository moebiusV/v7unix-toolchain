#!/bin/sh
# cpp golden test: preprocess tests/test.i and compare to tests/test.expected.
set -e

CPP="${1:-./cpp}"
dir="$(dirname "$0")"

out="$(mktemp)"
trap 'rm -f "$out"' EXIT

"$CPP" -P "$dir/test.i" > "$out" 2>/dev/null

if cmp -s "$out" "$dir/test.expected"; then
    echo "PASS: cpp preprocesses tests/test.i to the golden output"
    exit 0
fi

echo "FAIL: output differs from tests/test.expected" >&2
diff -u "$dir/test.expected" "$out" >&2 || true
exit 1
