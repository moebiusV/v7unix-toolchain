# Which dmr-cc optimizations carry over to the VAX and x86 backends

Reviewed 2026-10-13 after the PDP-11 backend reached size parity with dmr-cc
(libc 17226 bytes `-O` vs dmr 17356).  Method: built `ccom` for
`vax-unknown-bsd` and `i386-unknown-linux` from the same tree (needs
`CFLAGS='-std=gnu89 -w'` with a current gcc) and compiled one file of idioms
(`pointer++` update through memory, `*d++ = *s++`, bit tests, byte
read-modify-write, compare with 0, indexing, `g++`, struct access, switch,
register loop, unsigned divide) with `-xtemps -xdeljumps -xinline -xdce`.
Only instruction patterns were compared; no VAX assembler or simulator was
available, so nothing here is measured in bytes or run.

Caveat found on the way: the VAX backend does not compile with a current gcc
without that flag, and its output for `(*ip++)++` looks wrong (the pointer
register is overwritten).  It needs a working test setup before changing.

## What the PDP-11 work did, and where it applies

| dmr / PDP-11 optimization | VAX | x86 (i386) |
|---|---|---|
| Autoincrement/decrement addressing `(rN)+`, `-(rN)` for `*p++`, `++*q++`, `*d++ = *s++` | **Yes, big.** VAX has the modes for every size; today it emits `movl r2,r3; addl3 $1,r2,r2` per use | No such mode (only string ops). Not applicable |
| In-place read-modify-write on memory (`inc M`, `add r,M`, `bisb $1,(r)`) | **Yes.** Today: `movl g,r0; addl3 $1,g,r0; movl r0,g` for `g++`; bytes go through `cvtbl`/`movb` | Mostly done already (`incl g`, `addl $5,lg`, `incl (%edx)`). **Byte `|=`, `&=`, `++` still load/modify/store** (`movsbl; orl; movb`): `orb`/`andb`/`incb` to memory |
| Bit test as `bit` / `bitb` | **Yes.** Emits `bicl3 $-5,r0,r0; cmpl r0,$0`; want `bitl $4,r0` / `bitb` | **Yes.** Emits `andl $4,%eax; je`; want `testl $4,%eax` / `testb $8,flags` (no copy, no extra register) |
| Compare with zero as `tst` | **Yes.** `cmpl r0,$0` (also redundant after an instruction that already set the codes) | **Yes.** `cmpl $0,%eax` -> `testl %eax,%eax` (shorter); `movb (%eax),%al; ... movsbl; cmpl $0` in a `while (*d++ = *s++)` loop should test the byte directly |
| Compare/test bytes in memory without widening (`cmpb M,$c`) | **Yes** (`cmpb`, `tstb`) | **Yes** (`cmpb $c,M`, `testb`) |
| Indexed addressing `tab(rN)` | **Yes, big.** VAX index mode `tab[rN]` (scaled); today `movab tab,r0; ashl $2,r1,r1; addl2 r1,r0` | Done for words (`(%eax,%edx,4)`); bytes still do `movl $ctab,%eax; addl %edx,%eax` where `ctab(%edx)` is one operand |
| Return value computed straight into r0/eax | Already fine | Already fine |
| `register` variables honoured in declaration order | Not needed (many registers; temps already do it) | Not needed |
| Frameless leaf functions (no `csv`/`cret`) | **No.** `calls`/`ret` and the entry mask are architectural | **Yes.** Leaves with no locals and no calls still do `pushl %ebp; movl %esp,%ebp; leave` (and `enter $4,$0` when a callee-saved register is used, which is slow and large).  Omit the frame pointer, address arguments off `%esp` |
| Long (2-word) arithmetic and compare shortcuts (`add $k,lo; adc hi`, `tst hi`) | N/A: 32-bit native; only `long long` | N/A for `long`; applies to `long long` on i386 (`addl/adcl`, test high word for sign) |
| Immediate and shared FP constants, `tstf`/`clrf` | Check: VAX has short float literals; probably fine | x87: `fldz`/`fld1` exist; constants in memory otherwise.  Low value |
| `.bss` statics collected at end (c2 interaction) | N/A | N/A (assembler-specific; only matters if a peephole pass is added) |
| Unsigned `/` and `%` by a power of two as shift/mask | Already in `optim.c` (machine independent), but see next row | Same |
| **Do not retype `DIV`/`MOD` across int/unsigned conversion** (wrong-code bug found on PDP-11) | `clocal` has the same shortcut, not tested (VAX build problem above) | Same shortcut in `clocal`, but `return n / m` with unsigned operands still produced `divl` (unsigned) when tried; not reproduced |
| Register liveness fixes: `OREG` base register counts as a use; fixed registers live through loops when no flow graph is built | In `mip/regs.c`: **already shared.**  The OREG fix applies everywhere; the "no flow graph" case only matters if a target pins registers (PDP-11 only) | Same |
| `mov x,x` removal, spill-slot sharing | Spill-slot sharing is in `mip/regs.c` (shared, only used when `-xtemps` spills); the text pass for `mov x,x` is PDP-11 only and would need the assembler syntax per target | Same |

## What does not carry over

* Everything about word size, `csv`/`cret`, odd/even register pairs for
  `ashc`/`mul`/`div`, and the one-word FP immediates is PDP-11 specific.
* c2 (the V7 optimizer) runs on PDP-11 assembly text.  The VAX/x86 backends
  have no text optimizer; the easy wins above are pattern templates in
  `table.c` / `order.c` / `local2.c` instead.

## Suggested order if this is pursued

1. x86: bit/zero tests (`testl`/`testb`), byte RMW to memory (`orb`/`andb`/
   `incb`), byte indexing (`ctab(%edx)`), frame-pointer omission for leaves.
   Each is a small template or `myreader` rewrite; the PDP-11 versions
   (`bittest`, `byterw`, `longrw` in `arch/pdp11/local2.c`) are the pattern.
2. VAX (after fixing the build and the `(*ip++)++` output): autoincrement
   and autodecrement addressing (`inctree`/`myormake` in
   `arch/pdp11/order.c` is the pattern), in-place RMW, `bitl`/`tstl`,
   scaled index mode.
3. Both: re-check `clocal` SCONV for the DIV/MOD retyping bug (PDP-11 found
   it with a K&R `f(n) unsigned n; { return n / 4; }`); i386 did not show it
   for the cases tried.

Measure with a per-target version of `tools/pccdiff.sh` (needs the target
assembler for byte counts; instruction counts are a usable proxy).
