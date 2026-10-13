#!/bin/sh
#
# pcctest -- run a C test on a simulated bare PDP-11 and print its result.
#
# The file defines `int test(void)`.  It is compiled with pcc (flags given
# before the file), assembled and linked with V7 as/ld and a tiny start stub
# (sp=0157000, call _test, HALT), loaded at address 0 in simh, run until HALT,
# and r0 -- test's return value -- is printed as a signed decimal.  No V7
# kernel, so no system calls: the test must be self-contained.
#
# usage: pcctest.sh [pcc flags ...] test.c
#   PCC=path/to/pcc   (default ~/claude/pcc/install/bin/pcc)
#
set -eu

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
top=$(CDPATH= cd -- "$here/.." && pwd -P)
C=$top/modern/usr/src/cmd
AS=$C/as/as; LD=$C/ld; export AS2=$C/as/as2
PCC=${PCC:-$HOME/claude/pcc/install/bin/pcc}
SIM=${SIM:-pdp11}

for last; do :; done
src=$last
W=$(mktemp -d "${TMPDIR:-/tmp}/pcctest.XXXXXX")
trap 'rm -rf "$W"' EXIT

cat >"$W/start.s" <<'EOS'
.globl	start
.text
start:
	mov	$157000,sp
	setd
	jsr	pc,_test
	0
	br	.
EOS

"$PCC" -S "$@" -o "$W/t.s"
"$AS" -u -o "$W/t.o" "$W/t.s"
"$AS" -u -o "$W/start.o" "$W/start.s"
"$LD" -o "$W/t.out" "$W/start.o" "$W/t.o" -L "$top/lib" -lc
# load text+data only: the file also carries a symbol table, and the bss
# (which a C program may rely on being zero) lives right after the data
python3 - "$W" <<'EOS'
import sys, struct
W = sys.argv[1]
b = open(W + '/t.out', 'rb').read()
magic, text, data = struct.unpack('<HHH', b[:6])
open(W + '/t.bin', 'wb').write(b[16:16 + text + data])
EOS

python3 - "$W" <<'EOP'
import sys, struct
W = sys.argv[1]
b = open(W + '/t.bin', 'rb').read()
if len(b) % 2:
    b += b'\0'
with open(W + '/r.ini', 'w') as f:
    f.write("set cpu 11/70\nset cpu 1m\n")
    for i in range(0, len(b), 2):
        f.write("deposit %o %o\n" % (i, struct.unpack('<H', b[i:i+2])[0]))
    f.write("set cpu history=100000\ndeposit pc 0\nrun\nexamine r0\nshow cpu history=100000\nexit\n")
EOP

timeout 120 "$SIM" "$W/r.ini" >"$W/out" 2>&1 || true
cp "$W/out" "${HISTOUT:-/tmp/pcchist.out}"
python3 - "$W/out" <<'EOP'
import sys, re
t = open(sys.argv[1]).read()
m = re.search(r'^R0:\s+([0-7]+)', t, re.M)
if 'HALT instruction' not in t or not m:
    sys.stderr.write("did not halt\n" + t[-300:]); sys.exit(1)
v = int(m.group(1), 8)
print(v - 65536 if v >= 32768 else v)
EOP
