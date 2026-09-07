#!/bin/sh
#
# build-pcc99 — cross-compile the pcc99/ tier (V7 source modernized for pcc)
# into a PDP-11 toolchain, host-side.  For each tool: pcc -S (PDP-11 asm),
# then the ported V7 assembler, then the ported V7 linker.  The result is a set
# of PDP-11 a.out binaries that only run under simh.
#
# Output mirrors orig/'s filesystem layout:
#   pcc99/bin/{cc ld ar cp mv rm cmp make yacc sh as}
#   pcc99/lib/{c0 c1 c2 cpp as2 crt0.o fcrt0.o fmcrt0.o mcrt0.o libc.a}
# (orig/bin + orig/lib hold the V7 *reference*; pcc99/ holds the pcc-built
# equivalents.)
#
# The binaries are "their own thing" (like modern/) — pcc's codegen is ~64%
# larger than dmr-cc's, so they are NOT byte-identical to orig/.  What must be
# byte-identical is their OUTPUT when they run under V7 (see tools/pcc99-check.sh).
#
# Env overrides: PCC, AS, AS2, LD, CVOPT.

set -eu

TOPDIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
PCC=${PCC:-/home/david/claude/pcc/install/bin/pdp11-bsd-pcc}
AS=${AS:-"$TOPDIR/modern/usr/src/cmd/as/as"}
AS2=${AS2:-"$TOPDIR/modern/usr/src/cmd/as/as2"}
LD=${LD:-"$TOPDIR/modern/usr/src/cmd/ld"}
CVOPT=${CVOPT:-"$TOPDIR/modern/usr/src/cmd/c/cvopt"}

PCC99="$TOPDIR/pcc99"
BIN="$PCC99/bin"
LIB="$PCC99/lib"
# V7's cpp predefines `unix` (and `pdp11`); pcc predefines `pdp11` but not
# `unix`, so pass it explicitly to match V7 cc's compilation environment.
INCLUDES="-I $TOPDIR/modern/usr/include -I $PCC99/usr/include -Dunix"
CRT0="$TOPDIR/lib/crt0.o"      # host-side link uses the reference crt0.o
LIBDIR="$PCC99/lib"

export AS2

mkdir -p "$BIN" "$LIB"
SCRATCH=$(mktemp -d "${TMPDIR:-/tmp}/pcc99build.XXXXXX")
trap 'rm -rf "$SCRATCH"' EXIT
cd "$SCRATCH"

# pcc -S + as -u -> .o.  $1 = src.c (absolute), $2 = optional extra -I dir.
co() {
    base=$(basename "$1" .c)
    extra=${2:-}
    "$PCC" -S -O0 $INCLUDES $extra "$1" -o "$base.s" 2>"$base.pcc.log" \
        || { echo "FAIL pcc $1"; tail -5 "$base.pcc.log"; return 1; }
    "$AS" -u -o "$base.o" "$base.s" 2>"$base.as.log" \
        || { echo "FAIL as  $1"; tail -5 "$base.as.log"; return 1; }
}

# ld.  $1 = mode (-n|-i), $2 = out path, then objs...
lnk() {
    mode=$1; out=$2; shift 2
    "$LD" "$mode" -s -o "$out" "$CRT0" "$@" -L "$LIBDIR" -lc 2>"$(basename "$out").ld.log" \
        || { echo "FAIL ld  $out"; tail -5 "$(basename "$out").ld.log"; return 1; }
}

S="$PCC99/usr/src/cmd"

echo "build-pcc99: compiling pcc99/ tier with pcc -> pcc99/{bin,lib}"

# --- flat single-file tools -> bin ----------------------------------------
for t in cc ld ar cp mv rm cmp; do
    co "$S/$t.c" && lnk -n "$BIN/$t" "$t.o" \
        && printf '  %-5s %6s bytes magic=%s\n' "$t" "$(stat -c%s "$BIN/$t")" "$(od -An -tx1 -N2 "$BIN/$t" | tr -d ' \n')"
done

# --- c0 -> lib ------------------------------------------------------------
for f in c00 c01 c02 c03 c04 c05; do co "$S/c/$f.c" "-I $S/c"; done
lnk -n "$LIB/c0" c00.o c01.o c02.o c03.o c04.o c05.o \
    && printf '  %-5s %6s bytes magic=%s\n' c0 "$(stat -c%s "$LIB/c0")" "$(od -An -tx1 -N2 "$LIB/c0" | tr -d ' \n')"

# --- c1 -> lib (table.i pre-converted by the host cvopt) ------------------
"$CVOPT" < "$S/c/table.s" > table.i
"$AS" -u -o table.o table.i 2>table.as.log
for f in c10 c11 c12 c13; do co "$S/c/$f.c" "-I $S/c"; done
lnk -n "$LIB/c1" c10.o c11.o c12.o c13.o table.o \
    && printf '  %-5s %6s bytes magic=%s\n' c1 "$(stat -c%s "$LIB/c1")" "$(od -An -tx1 -N2 "$LIB/c1" | tr -d ' \n')"

# --- c2 -> lib (separate I/D) --------------------------------------------
for f in c20 c21; do co "$S/c/$f.c" "-I $S/c"; done
lnk -i "$LIB/c2" c20.o c21.o \
    && printf '  %-5s %6s bytes magic=%s\n' c2 "$(stat -c%s "$LIB/c2")" "$(od -An -tx1 -N2 "$LIB/c2" | tr -d ' \n')"

# --- cpp -> lib (cpp.c + cpy.c; cpy.c #includes yylex.c) ------------------
co "$S/cpp/cpp.c" "-I $S/cpp"
co "$S/cpp/cpy.c" "-I $S/cpp"
lnk -n "$LIB/cpp" cpp.o cpy.o \
    && printf '  %-5s %6s bytes magic=%s\n' cpp "$(stat -c%s "$LIB/cpp")" "$(od -An -tx1 -N2 "$LIB/cpp" | tr -d ' \n')"

# --- make -> bin ----------------------------------------------------------
for f in ident main doname misc files dosys gram; do co "$S/make/$f.c" "-I $S/make"; done
lnk -n "$BIN/make" ident.o main.o doname.o misc.o files.o dosys.o gram.o \
    && printf '  %-5s %6s bytes magic=%s\n' make "$(stat -c%s "$BIN/make")" "$(od -An -tx1 -N2 "$BIN/make" | tr -d ' \n')"

# --- yacc -> bin (separate I/D) ------------------------------------------
for f in y1 y2 y3 y4; do co "$S/yacc/$f.c" "-I $S/yacc"; done
lnk -i "$BIN/yacc" y1.o y2.o y3.o y4.o \
    && printf '  %-5s %6s bytes magic=%s\n' yacc "$(stat -c%s "$BIN/yacc")" "$(od -An -tx1 -N2 "$BIN/yacc" | tr -d ' \n')"

# --- sh -> bin (20 .c files) ----------------------------------------------
shobjs=""
for f in "$S"/sh/*.c; do
    base=$(basename "$f" .c)
    co "$f" "-I $S/sh"
    shobjs="$shobjs $base.o"
done
# shellcheck disable=SC2086
lnk -n "$BIN/sh" $shobjs \
    && printf '  %-5s %6s bytes magic=%s\n' sh "$(stat -c%s "$BIN/sh")" "$(od -An -tx1 -N2 "$BIN/sh" | tr -d ' \n')"

# --- as / as2 (PDP-11 assembly, assembled by the ported V7 as) ------------
"$AS" "$PCC99/usr/include/sys.s" "$S"/as/as1?.s 2>as.log || { echo "FAIL as (assembler)"; tail as.log; }
"$LD" -n -s -o "$BIN/as" a.out 2>as.ld.log && rm -f a.out \
    && printf '  %-5s %6s bytes magic=%s\n' as "$(stat -c%s "$BIN/as")" "$(od -An -tx1 -N2 "$BIN/as" | tr -d ' \n')"
"$AS" "$PCC99/usr/include/sys.s" "$S"/as/as2?.s 2>as2.log || { echo "FAIL as2 (assembler)"; tail as2.log; }
"$LD" -n -s -o "$LIB/as2" a.out 2>as2.ld.log && rm -f a.out \
    && printf '  %-5s %6s bytes magic=%s\n' as2 "$(stat -c%s "$LIB/as2")" "$(od -An -tx1 -N2 "$LIB/as2" | tr -d ' \n')"

# --- runtime (crt0.o etc.) -> lib, copied from the reference lib/ ---------
for f in crt0.o fcrt0.o fmcrt0.o mcrt0.o; do cp "$TOPDIR/lib/$f" "$LIB/$f"; done

echo "build-pcc99: done.  pcc99/bin + pcc99/lib now mirror orig/bin + orig/lib"
