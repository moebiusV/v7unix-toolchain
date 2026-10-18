# Optimization tricks of dmr-cc and 32V's cc, and how they carry over to modern pcc

This is a catalog of the code-improvement tricks used by two historic C compilers,
what each one does, how it was implemented then, how it is implemented in the
modern pcc (`~/claude/pcc`), and how it can be brought to other backends.

The two compilers:

* **dmr-cc** (Dennis Ritchie's C compiler, V7 Unix, PDP-11): `cpp -> c0 -> c1 -> c2`.
  `c0` parses, `c1` generates assembly, `c2` is a peephole optimizer run over
  c1's output (the "-O" pass).
* **32V cc** (the VAX port of V7, 1979): this is *Johnson's pcc* (front end
  `ccom`, code generator tables in `table.c`) followed by a VAX `c2`.
  The VAX `c2` is a fork of the PDP-11 `c2`: same file layout
  (`c20.c c21.c c22.c`), same core functions (`iterate rmove jumpsw addsob comjump
  xjump backjmp redunbr findcon setcon repladdr codemove refcount`), plus a
  backward-flow pass (`bmove`/`bflow`), bit/field instruction handling
  (`bitopt bicopt bixprep`) and typed operands for byte/word/long forms.
  The PDP-11 `c2` has `reducelit`, `dualop`/`singop` and `toofar`/`ilen` instead.

Where dmr-cc is a hand-written one-pass-per-stage compiler, 32V's cc gets most
of its quality from `c2`; the compiler proper (pcc) is generic.

## Status of this work (2026-10-17)

All dmr/32V-inspired transformations are opt-in with `-xdmr` (`pcc -O` implies it on
the PDP-11). Everything else is unchanged. Three things are not gated, on purpose:
the PDP-11 `PLUS` right-operand rule in `table.c` (a table rule has nothing to test),
sizeof being unsigned on the PDP-11 (a correctness fix), and the shared bug fixes.

| Target | Corpus | original cc | ours | |
|---|---|---|---|---|
| PDP-11 | libc, 89 files | dmr 17356 | 16838 | -3.0% |
| PDP-11 | `cmd/`, 52 files that compile | dmr 98102 | 94174 | -4.0% |
| VAX (32V) | libc gen+stdio, 91 files | `cc -O` 21976 | 18308 | -16.7% |
| VAX (32V) | 56 command sources | `cc -O` 118444 | 106204 | -10.3% |
| i386 | libc gen+stdio (91 files) | n/a | 21369 (27839 with no flags) | -23% |
| amd64 | libc gen+stdio (91 files) | n/a | 24747 (39919 with no flags) | -38% |

All numbers are text+data bytes, assembled with the target's own assembler
(`tools/pccdiff.sh`, `tools/vaxdiff.py`, `tools/x86size.sh`). Flags used:
`-xdmr -xtemps -xdeljumps -xdce -xinline`, plus the target's `c2` where there is one
(`-xc2`, run inside ccom). `-xssa` is a net loss on both PDP-11 and VAX and stays off.
The x86 "no flags" column is the upstream default; most of that difference is
`-xtemps` and friends, not `-xdmr`, which is worth 1126 bytes of the 22508 on i386.

**The goal for the two historic targets** was set as: a file may be larger than the
original compiler's only because its code is more correct.  That is met.

* PDP-11: 2 of the 141 files are larger than dmr-cc's, `libc/gen/qsort.c` (+12) and
  `cmd/sh/print.c` (+54); both are the unsigned-divide helpers (`udiv`, `urem`), which
  dmr-cc replaces by a `div` that is wrong for a divisor of 32768 or more
  (`tools/dmrbugs/udiv-variable.c`).
* VAX: 1 of the 147 files is larger than `cc -O`'s, `libc/gen/mktemp.c` (+92, the `__urem` helper;
  32V's cc divides unsigned values as signed, `tools/dmrbugs/udiv-variable-vax.c`).

The files pcc rejects are listed, with the old forms that cause it, in
`OLD-K&R-FORMS.md`: 28 of the 80 V7 command sources and 18 of 605 32V sources.  The
dialect was not changed.

Correctness gates, all passing: 332 bare-metal PDP-11 test runs (`tools/pcctest-all.sh`,
four flag sets), 249 VAX runs on real 32V under SIMH (`tools/vaxtest-all.sh`, four
flag sets, one with c2 and one with `-xssa`), 254 i386 and 240 amd64 runs, pcc
compiling itself with identical stage 1 and 2 (`tools/pccboot.sh`; note it builds
the committed `HEAD`, not the working tree), and 34 V7 commands, 72 tests, built by
`pcc -O` and run inside real V7 next to V7's own cc builds (`tools/v7progs.py`).
A test returns 1 when it passes and `2*bad+1000` when it fails; it used to return
`bad`, so a failing first assertion (bad == 1) was reported as a pass (see
"Bugs found by compiling real programs").

## The tricks at a glance

| # | Trick | dmr (PDP-11) | 32V (VAX) | pcc status | x86 | other backends |
|---|---|---|---|---|---|---|
| 1 | `register` variables in fixed registers | c1 (r4,r3,r2) | cc (r11 down) | done (pdp11, vax) | not needed: allocator is good | no |
| 2 | Autoincrement / autodecrement operands | c1 `(r)+` `-(r)` | pcc table + c2 | done (pdp11, vax) | none (no such mode) | m68k, arm (post-index), maybe |
| 3 | Indexed addressing with a symbol | c1 `sym(r)` | pcc `sym[r]` | done (pdp11, vax) | **done** `sym(,%r,4)` | m68k, arm |
| 4 | Deferred modes (`*sym`) | `*sym` | `*sym` | done (vax) | none | m68k (memory indirect) |
| 5 | `inc`/`dec` and in-place read-modify-write | c1 | pcc + c2 | done | done (byte RMW) | partly |
| 6 | `tst` for compare with zero; reuse of condition codes | c1 + c2 | pcc + c2 | done | done (`testl`) | most |
| 7 | Compare chars/shorts without widening | c1 | pcc (`cmpb`) | done | done | yes |
| 8 | Bit tests (`bit $m,x`; `bic`/`bis`) | c1 | `bitl`, `jbs`/`jbc` | done | done (`test`) | yes |
| 9 | Masks as zero-extension / field extract | - | c2 `bicopt` -> `movzbl`/`extzv` | vax via c2 | **done** `movzbl/movzwl` | arm `uxtb`, mips `andi`, ... |
| 10 | Frameless leaf functions | c1/`csv` skipped | - | done (pdp11) | done (i386) | yes |
| 11 | Loop rotation (test at the bottom) | c2 | c2 | **done on the IR** (`looprot`, all targets) | done (same pass) | free: it is in `mip/` |
| 12 | Branch reversal over `jbr`, jump chains, cross-jumping | c2 | c2 (+ `jumpsw`) | IR: `deljumps`, `tailmerge` (all targets); c2 for the rest | done (same pass) | free: it is in `mip/` |
| 13 | Redundant load / store / test elimination (register tracking) | c2 `rmove` | c2 `rmove` | via c2 | **partly** (`testl` after and/or/xor; Z/S branches after add/sub/inc/dec) | yes (needs a pass) |
| 14 | Send a result straight to its destination (`bflow`) | c2 (2-address) | c2 (3-address) | via c2 | no (2-address ISA) | risc: no |
| 15 | Address constants: `movab`/`pushab`/`moval` | - | c2 | done (vax) | **done** on amd64 (`leaq sym(%rip)`, `movl $c`); `leal` for `&a[i]` open | m68k `lea` |
| 16 | Counted-loop instructions | `sob` | `sobgeq` `aoblss` `acbl` | via c2 | `loop` is slow: no | ppc `bdnz`, arm64 none |
| 17 | Switch via jump table | inline compare chain, sorted | `casel` | **done** (pdp11 and vax `mygenswitch`) | exists (generic) | needs `mygenswitch` per target |
| 18 | Callee-saved registers in an entry mask | csv | `.word mask` | done (vax) | push/pop | no |
| 19 | Floating immediates | first word of a double | - | done (pdp11) | no | no |
| 20 | `x++;` without a copy of the old value | c1 | pcc | done (`-xdmr`, pass 1) | generic (pass 1) | generic |
| 21 | `*(&x + k)` is the name `x+k` | c1 | pcc | done (pdp11, vax) | done (generic) | generic |
| 22 | Dead mask test (`uchar & 0x100`) | c1 | c2 `bitopt` | open | open | front end |
| 23 | Unsigned divide: inline for small constants, helpers otherwise | `clr r0; div` | - | done (pdp11) | n/a | per libc |
| 24 | Register pressure: spill choice, pinned regs | c1 | pcc | partial | allocator | allocator |
| 25 | Compare two bytes in memory (`cmpb (r4),(r3)+`) | c1 | `cmpb` | done (pdp11, vax, x86 narrowing) | done (`CBRANCH` in `local.c`) | any with memory operands |
| 26 | `c ? a : b` as a condition branches from each arm | c1 | pcc | done (pass 1, all targets) | done (same) | generic |
| 27 | Update `x op= e` in a condition as a statement, then compare | c1 | - | done (pass 1, all targets) | done (same) | generic |
| 28 | Long in memory updated in place (`add r0,-12(r5); adc ...`, `almul`/`aldiv`/`alrem`) | c1 | - | done (pdp11) | no (64-bit ints) | 16-bit targets |
| 29 | Store through a pointer kept in memory (`movb r3,*_p; inc _p`) | c1 | pcc | done (pdp11, vax: `postincfold`, `SSTARX`) | none | m68k |
| 30 | Small struct assignment as moves | c1 | - | done (pdp11) | n/a | per target |
| 31 | Low word of a long (`mov r1,r0`) and zero-high-word long arguments | c1 | - | done (pdp11) | n/a | 16-bit targets |
| 32 | Last update of a register variable that is also the result (`bis r3,r0`) | c1 | - | done (IR `retfold`, all targets) | done (same) | generic |
| 33 | Condition codes at a join; `?:` arms straight into the argument slot; `clr r0; mov r0,D` | c2 | c2 | done (pdp11 c2 port); vax open | no (no c2) | no |
| 34 | A char from a word in memory is a byte load | c1 | - | done (pdp11) | free: x86 loads partial registers | little-endian targets |
| 35 | `sizeof` is unsigned | c0 | pcc | done (pdp11: **was a bug**) | already `unsigned long` | all 16-bit targets |
| 36 | `x op= e` / `x++` through a pointer: pointer kept once, offset in the address | c1 | pcc | done (pass 1, all targets) | done (same) | generic |
| 37 | `x++;` as a statement leaves no copy of the old value | c1 | pcc | done (pass 1, all targets) | generic | generic |
| 38 | Deferred operand through a pointer in the frame (`clrb *-16(r5)`) | c1 | pcc | done (pdp11) | n/a | m68k |
| 39 | A parameter used from its slot when its uses are cheap | c1 | pcc | done (pdp11) | allocator | allocator |
| 40 | VAX: a parameter used once or twice is read from its slot; unused ones are not loaded | - (never copies) | cc (never copies) | done (vax) | allocator | allocator |
| 41 | A table jump (`casel`) is a jump the flow graph must see | - | - | done (`mip/optim2.c`; **was a latent miscompile**) | n/a | any target with a jump table in asm |
| 42 | c2 liveness by width: a byte written into a register does not replace the rest | - | (c2 never looked across branches) | done (vax c2) | n/a | no |
| 43 | A `tst` of a value a `mov`/`cvt`/`movz` just stored is redundant | c2 | c2 | done (vax c2) | n/a | no |
| 44 | `movl X,r; movb r,Y` is `movb X,Y`; `sub $1,M,r; movl r,M` is `decl M; movl M,r` | c1 | pcc | done (vax c2) | n/a | no |
| 45 | A function that returns nothing does not want r0 at its `ret`; dead register writes | c2 | (c2 look-ahead) | done (vax) | allocator | allocator |
| 46 | Functions start on a word, not a longword | - | `.align 1` | done (vax, `-xdmr`) | no (speed) | no |
| 47 | c2 says so when it fails | - | - | done | n/a | n/a |
| 48 | Indexed operands of 8-byte objects, frame-relative indexing (`movaq`, `QADDR`) | - | pcc | done (vax) | `lea` | m68k |
| 49 | Bit tests as `jbs`/`jbc`; mask as zero-extension (`BITTEST`, `maskzext`) | c1 | c2 + pcc | done (vax) | `test` | yes |
| 50 | Narrow load pushed in one instruction (`SCVTARG`); a struct copy source as an address (`SSTRSRC`) | - | pcc | done (vax) | n/a | n/a |
| 51 | `x & ~y` as `bic`; `~~x` removed; r1 live at a `ret` only after a quad return | - | c2 | done (vax) | `andn` | arm `bic` |
| 52 | PDP-11 c2: `add $2,rN` before a use of `-2(rN)` is `(rN)+` | c1 | - | done (pdp11 c2) | n/a | no |
| 53 | PDP-11 c2: a value spilled and reloaded at once is a push and a pop | c1 | - | done (pdp11 c2) | n/a | no |
| 54 | PDP-11: `*(p += k) - k` with `p` in the frame is a use through `p`, then `add` in place | c1 | - | done (`SSTARINC`) | n/a | m68k |
| 55 | PDP-11: conversions among the one-word types change only the type | c1 | - | done (`nopconv`) | n/a | 16-bit targets |
| 56 | PDP-11: a long made from a word is pushed as `mov x,-(sp); sxt -(sp)` | c1 | - | done (`SSXT`) | n/a | 16-bit targets |
| 57 | A double result straight to its destination (`pairfold`) | - | c2 | done (vax c2) | n/a | no |
| 58 | A `casel` reads only its selector; the backward pass runs again | - | - | done (vax c2) | n/a | no |
| 59 | PDP-11: long long as a memory object operated on by helpers | - | - | done (pdp11) | native | other 16-bit targets |
| 60 | VAX: long long helpers written into the file; quad constants in memory | - | - | done (vax) | native | other targets without a libgcc |
| 61 | VAX c2: a load that overwrites the pointer it was loaded through | - | - | done (vax c2) | n/a | no |
| 62 | aarch64: `mov xc,xb; add xb,xb,#k; ldr Rt,[xc]` is a post-index access (autoincrement) | c1 `(r)+` | - | done (aarch64 `-xdmr`, `peep.h`) | n/a | arm (post-index) |
| 63 | aarch64: a compare with zero as a branch (`cbz`/`cbnz`), the sign and a single bit as `tbz`/`tbnz`, `and`+compare as `tst` | c1 `tst`/`bit` | `bitl`, `jbs` | done (aarch64) | `test` | riscv `beqz`, arm `cbz` |
| 64 | aarch64: the extension and the shift in the operand (`add x,x,w,sxtw`, `[x,x,lsl #2]`) | c1 `sym(r)` | `sym[r]` | done (aarch64) | `sym(,%r,4)` | arm |
| 65 | aarch64: a condition as a value (`cset`) | - | - | done (aarch64, peephole) | `setcc` | arm `cset`/`it` |
| 66 | aarch64: frame record set up by a pre-indexed `stp`, torn down by a post-indexed `ldp`; saved registers in pairs; leaf functions have no frame | c1 `csv`/`cret` | entry mask | done (aarch64) | `leave` | riscv, arm |

"done" means implemented and covered by tests that run on the real system (PDP-11
under SIMH with V7, VAX under SIMH with 32V) or natively (x86).

---

## 1. `register` variables in fixed registers

**What.** Variables declared `register` (up to three on the PDP-11, six on the
VAX) live in fixed registers for the whole function. Register parameters are
copied in at entry.

**dmr.** c1 hands out r4, r3, r2 in declaration order; everything else lives in
memory, even under -O. That makes code predictable, and loops over a `register`
pointer are tight.

**32V.** pcc does the same from r11 downward; the entry mask (`.word 0xc00`)
says which to save.

**pcc.** `REGVARS`/`REGVAR0` in `macdefs.h` turn it on; `pass1` (pftn.c
`regvar_take`) gives the registers out, `bfcode` copies register parameters in.
`regs.c` keeps the allocator away from them; if the allocator runs out of
registers the compiler re-executes itself without pinned variables
(`regvars_retry`). The VAX port is the same code with `REGVARS 6`.

**Port.** Only worth it where the allocator is weaker than a human's declaration
hints. On x86 pcc's allocator already does well; skip. It is a good first step
for any new small-register target.

## 2. Autoincrement and autodecrement operands

**What.** `*p++` is the single operand `(rN)+`, `*--p` is `-(rN)`; the pointer
moves by the size of what it points to.

**dmr/32V.** The addressing mode exists in hardware (PDP-11 modes 2/4, VAX modes
8/7). c1 / pcc's shape matcher (`STARREG`) recognizes `*p++` where `p` is in a
register and the step equals the object size.

**pcc.** pass 1 builds `*p++` as `(p += n) - n` instead of `(t = p, p += n, t)`
(`AUTOINC` in `trees.c`, gated by `AUTOINC_SIZE` per target). In pass 2,
`offstar` recognizes the shape and marks the `UMUL` (`INCMARK`); after
register assignment `myormake` turns it into an `OREG` with the R2 flag
`RINCR`/`RDECR`, printed `(rN)+` / `-(rN)`. Needs the pointer in a hard
register, so it works for `register` variables.

**Port.** Only machines with the mode: m68k (`(An)+`, `-(An)`), ARM
post/pre-indexed load/store, PowerPC `lwzu`. Copy `inctree()`/`myormake()` from
`arch/pdp11` or `arch/vax`. On x86 the equivalent is `lodsb`/`stosb`, which
gain nothing in compiled code.

## 3. Indexed addressing with a symbol

**What.** `a[i]` becomes one operand: `_a(r1)` (PDP-11, `i` pre-scaled),
`_a[r1]` (VAX, the hardware scales by the operand size), `a(,%edx,4)` (x86).

**pcc.** In `offstar` find `UMUL(PLUS(&sym, LS(i, k)))` where `1<<k` is the
object size; make `i` (and a base register, if any) registers; `myormake`
builds an `OREG` carrying the symbol name and the packed registers
(`R2PACK(base, index, flags)`). VAX also folds a constant added to the index into
the displacement (`_C+1[r10]` for `C[i+1]`) and a constant added to a base. `regs.c` liveness
must see the index register (`R2NOINDEX` hook).

**Port.** i386 done (`order.c`, `symindex()`); amd64 needs the same only in
non-PIC code (`a(,%rdx,8)` uses a 32-bit absolute relocation). m68k has
`(d8,An,Xn)`; ARM has scaled register offsets but no symbol displacement.
`&a[i]` as a value (`leal a(,%edx,4),%eax`) is **not** done: it needs a table
rule that matches a `PLUS` whose right operand is a shift, which the matcher
cannot express; do it in `mycanon` by rewriting to `UMUL` + address-of, or
add a pass 2 shape.

## 4. Deferred modes (`*sym`, `*off(rB)`)

**What.** `--infile->_cnt` with `infile` a global pointer: `decl *_infile` — no
register for the pointer. It is the heart of stdio's `getc`/`putc` macros.

**32V.** pcc emitted it from the `STARNM` shape (UMUL of a NAME).

**pcc.** `deferred()` in `arch/vax/order.c`: the child of the `UMUL` is a word in
memory (NAME or OREG with a displacement). Only `*(name+k)` is expressible, not
`(*name)+k`. Printed through the R2 flag 1 (`*`).

**Port.** Only VAX and 680x0 (memory indirect modes). Not x86.

## 5. `inc`/`dec`, in-place read-modify-write

**What.** `x = x + 1` on a memory variable is `incl x`; `x += 5` is `addl2 $5,x`
(one instruction, no register); on bytes `incb`, `orb`, `andb`, `bisb`, `bicb`.
Also `x++;` as a statement leaves no copy of the old value.

**dmr/32V.** c1 / pcc pattern match `ASSIGN(x, OP(x, const))` for pure `x`.
`c2` merges `subl3 $1,x,r0; movl r0,x` into `subl2 $1,x` (`redun3`, `bflow`).

**pcc.** PDP-11: `wordrw()`/byte forms in `local2.c` mark the `ASSIGN`
(`n_qual`) and the template prints `inc`/`dec`. VAX: `markincdec()` marks a
statement-level `x = x +- 1` (and splits a branch on `(x = x - 1) >= 0` into the
statement plus a branch on `x`, so `c2` yields `decl x; jgeq`). `ecomp()` turns the statement
`x++;` into `x += 1`. `x++` with the value used is `(x += 1) - 1` for
ints/pointers in memory (`AUTOINC_MEM`), which avoids the temporary. `cc2.vax.c`
turns `addX2 $1,d` into `incX d`.

**Port.** x86 has `incl/decl/orb/andb` on memory (done for bytes in `-xdmr`);
mark ASSIGN nodes the same way. RISC machines have no memory operands: skip.
**Caution.** The marked node must only be matched by the FOREFF/FORCC template
that prints `inc`; never rewrite the right operand to a constant in a context
where the value may be needed (INAREG): that silently assigns the constant.

## 6. `tst` and condition-code reuse

**What.** Compare with zero is `tstl x` (one operand, shorter than `cmpl x,$0`),
and a `tst` right after an instruction that set the codes for the same value is
deleted.

**dmr/32V.** c1/pcc choose `tst`; `c2` tracks the operand whose codes are current
(`ccloc`, `setcc`) and deletes a redundant `tst` (counted as `nrtst`).

**pcc.** VAX `ZQ` template; `cc2.vax.c` deletes `tstX` after `incX/decX` of the same
operand when only the Z/N flags are tested (V and C differ). x86: `testl %eax,%eax`
(done) and `RESCC` templates already mark instructions that set flags.

**Port.** x86: also drop `testl` after `decl`/`addl` (open; use `RESCC` on those
templates and let pass 2 skip the compare). Most ISAs with condition codes.

## 7. Compare bytes and shorts without widening

**What.** `if (c == 'x')` is `cmpb c,$120`, not `cvtbl c,r0; cmpl r0,$120`.

**pcc.** `clocal` `CBRANCH`: if the left operand is `SCONV` from a char/short and
the constant fits the narrow type, drop the conversion; for unsigned narrow types
switch to the unsigned relation.  Two operands that are both `SCONV` from the same
narrow type are narrowed together (`*s1 == *s2++` is `cmpb (r4),(r3)+` on the
PDP-11 and VAX).  PDP-11 and VAX had this first.  i386 and amd64 did not: upstream's
version in `arch/i386/local.c` is disabled (`#ifdef notyet`, "breaks sometimes"), so
the x86 backends compared `movzbl; cmpl $200`.  They now have their own, under
`-xdmr`: `cmpb $200,(%eax)`, and `movb (%eax),%al; cmpb %al,(%edx)` for two bytes
(x86 has no memory-to-memory compare).  libc: i386 19252 -> 19052, amd64 22572 ->
22404.

**Port.** Any target with byte/halfword compare instructions (x86, 680x0). ARM,
MIPS, RISC-V have none: the load already extends.

## 8. Bit tests and mask operations

**What.** `if (x & 4)` is `bit $4,x` / `bitl $4,x` / `testb $4,x` (no result
register); `x |= m` and `x &= ~m` are `bis`/`bic`.

**pcc.** PDP-11 `BITTEST` qualifier on the compare; VAX `bitl/bitb` templates;
x86 `testl/testb` with the same qualifier. `jbs/jbc` (VAX) come from `c2`.

**Port.** x86 done. m68k `btst`; RISC-V/arm64 `tbnz` for single bits (needs a
pass 2 rule on `CBRANCH(AND(x, 1<<k))`).

## 9. Masks as zero-extension or field extraction

**What.** `x & 0xff` is one instruction (`movzbl`), `(x >> k) & mask` is
`extzv`. Shorter and faster than a 32-bit immediate `and`.

**32V.** `c2` `bicopt()`: a `bicl2 $mask` that keeps only low bits becomes
`movzbl`/`extzv`, looking back through `ashl` and `cvt` instructions.

**pcc.** x86: a dedicated `AND` rule before the generic `OPSIMP` rules prints
`movzbl %al,%eax` / `movzwl %ax,%eax` (`ZY`). VAX gets it through `cc2.vax.c`.

**Port.** arm `uxtb/uxth`/`ubfx`, aarch64 `ubfx`, RISC-V Zbb `zext.b`, PowerPC
`rlwinm`. The rule belongs before the generic AND rule and must not set the
condition codes (a rule without `RESCC`/`FORCC`).

## 10. Frameless leaf functions

**What.** A function that calls nothing and needs no frame does not set one up
(PDP-11: no `csv`/`cret`, no `r5`; i386: no `pushl %ebp`).

**pcc.** A text pass over each function's assembly (`frameless()` in
`arch/pdp11/code.c`, same idea in `arch/i386/code.c`) when it only touches
the arguments relative to `sp`. amd64 already omits frames. Off with
`-xnoframeless`.

**Port.** Any ABI where the callee may use the caller's stack arguments through the
stack pointer. Not worth it on RISC (already frameless when leaf).

## 11. Loop rotation

**What.** `while (c) body` is compiled as `jmp test; top: body; test: c; jcc top`:
one branch per iteration instead of two.

**c2.** In both c2s (`loopiv`): a conditional branch to a label that ends in an
unconditional jump back is rotated. No front-end involvement.

**pcc.** `looprot()` in `mip/optim2.c`, run by `deljumps` under `-xdmr`, does it
on the IR for every target.  Pattern: a label, up to 16 statements and a
conditional branch out of the loop, a body, then a `jbr` back to the label with the
branch's target label right after it.  The test block (with its labels, so a
`continue` still finds it) is moved below the body, its condition inverted and
aimed at a new label at the top of the body, and the back-jump becomes the entry
jump in front of the body (dropped if nothing falls into the loop).  Not done for
floating-point tests (inverting those is wrong with NaNs) or an empty body.  The
new label's `interpass` is allocated before `deljumps` takes its mark, since the
working storage is freed when it returns.

This replaced a text pass over the assembly in `common/x86peep.h`; i386 libc went
19500 -> 19498 and amd64 22819 -> 22816.  PDP-11 and VAX sizes are unchanged because
c2 had already done it; c2's own rotation now finds nothing left to do.  Test:
`pcctests/looprot.c` (while, for with `continue`, `break`, nested, do-while, a test
with side effects, a float loop, a loop at the top of the function).

Pitfall found on the way: the list head has no `interpass`, so a loop at the very
start of the node list needs the queue position remembered (`headip`).

## 12. Jump cleanup

**What.** Jump to a jump; branch around an unconditional jump reversed;
cross-jumping (two paths ending in the same sequence share it); code motion;
removing unreachable code.

**c2.** `comjump`, `xjump`, `codemove`, `redunbr`, `iterate`, `jumpsw`. `jumpsw`
(32V) additionally puts the *farther* of a conditional/unconditional pair on the
unconditional jump, because it has the larger range.

**pcc.** `-xdeljumps` removes jumps to the next label, and, under `-xdmr`, `tailmerge()` in
`mip/optim2.c` does the cross-jumping: two blocks that end in the same run of statements
(`nodeeq` compares trees, `labbefore` finds a label to jump to, `mergetails` rewrites)
share it. The rest is `c2`. `jumpsw`
is not worth porting to x86: the assembler relaxes jump sizes itself, and
reordering needs distances the compiler does not know. (It could be forced:
`{disp8}`/`{disp32}` or `.d8`/`.d32` select the form, but relaxation already
picks the optimal one.) It does matter on the PDP-11, where the assembler
turns an out-of-range `jne` into three words.

## 13. Redundant load/store/test elimination

**What.** Track what each register holds (`regs[]`, `conloc`, `ccloc`); delete
`mov` that loads a value a register already has, a store of what memory holds, a
`tst` after a move. `repladdr` substitutes a register for an operand it equals.

**c2.** `rmove()` in both c2s.

**Port.** On x86 there is no post-pass; a text pass could do the same with a
register-content table, but the pcc allocator's coalescing already removes most
redundant moves. Measure first; the likely wins are `movl x,%eax` after `movl %eax,x`.

## 14. Result straight to the destination

**What.** `addl3 a,b,r0; movl r0,c` is `addl3 a,b,c`; a two-address
`mov a,r0; add b,r0; mov r0,c` is `mov a,c; add b,c` when `r0` dies.

**c2.** `bflow` (VAX) does a backward walk with `uses[]`: when the next
instruction is the only user of a register and just copies it, send the result
there directly; also turns a 2-address into a 3-address instruction and back
(`redun3`).

**Port.** Only 3-address machines would gain in a post-pass; modern pcc's
allocator and coalescing do this for them. A quick check on a new target:
look for `op ...,rX; mov rX,dest` in the output.

## 15. Address constants: `movab`, `pushab`, `moval`

**What.** `foo(&x)` is `pushal _x` (6 bytes) instead of `movab _x,r0; pushl r0`
(9 bytes); `&sym + k` is `_sym+k`; an indexed address is `moval sym[r],dest`.

**c2.** The block "pushax or movax possibility" in `bflow` turns `movl $sym,r`
into `moval sym,r` and then, if followed by a push of that register, `pushal`.

**pcc.** `myp2tree` folds `&sym + k` into `&(sym+k)` (the node stays an address-of
so existing templates still print it as an address; do **not** convert it to a
bare constant: a named `ICON` is a *memory operand* in the VAX templates).
`cc2.vax.c`: `movab` -> `moval` when not indexed, `pushl r` of that -> `pushal`.

**Address constants as operands (VAX, `-xdmr`).** `cmpl _filesp,$_files` and
`movl $_files,x` need no register: a special shape `SADDRN` (an `ADDROF` of a
named object) is accepted by the compare and assign templates and `adrput` prints
`$name+off`; c2 turns the `movl` into `moval`.

**Port.** x86 `leal sym,%eax` is the same size as `movl $sym,%eax`: no gain
unless an index is involved.  With an index, i386 `-xdmr` writes `&a[i]` as one
`leal a(,%ecx,4),%edx` (special shapes `SSYMADDR`/`SSCALE`; libc 20142 -> 20132).
**Why i386 may use it and amd64 may not (position-independent executables).**
A PIE is loaded at a random address, so every address it holds must be fixed up
when it is loaded.  An absolute address written into an instruction, as in
`leal a(,%ecx,4),%edx` (the 32-bit address of `a` is part of the instruction), is
patched by the dynamic loader, which is called a text relocation.

* **i386:** the relocation is `R_386_32`, a full 32-bit address, and the loader
  can patch it.  The linker accepts it in a PIE, with the warning "creating
  DT_TEXTREL in a PIE" (the code pages must be writable while the loader patches
  them, and are then no longer shared between processes).  The i386 backend
  already does exactly this for every `movl $sym,%reg` and `sym(,%reg,4)`, so
  `leal` adds no new kind of relocation; I linked a test with it to check.
  (Text relocations are a cost: `-xdmr` output on i386 is not for a shared
  library that must stay shareable.)
* **amd64:** the relocation would be `R_X86_64_32S`, a 32-bit sign-extended
  address.  A 64-bit PIE can be loaded anywhere in a 47-bit address space, so
  the loader cannot patch a 32-bit field, and the linker refuses ("relocation
  R_X86_64_32S ... recompile with -fPIE").  The position-independent form is
  `leaq a(%rip),%rdx; leaq (%rdx,%rcx,4),%rax`, two instructions, which is no
  smaller than what pcc writes now, so amd64 keeps the old code.

m68k `pea`/`lea`.

## 16. Counted-loop instructions

**What.** `decl n; jgeq L` is `sobgeq n,L`; `incl i; cmpl i,lim; jlss L` is
`aoblss lim,i,L` (VAX); PDP-11 `sob r,L`.

**c2.** `addsob()` after all other passes (the displacement must be within the
instruction's byte range). Needs `incl/decl` to be there first, so
the `addX2 $1` -> `incX` rewrite matters.

**Port.** x86 `loop` is slower than `dec; jnz`: skip. PowerPC `bdnz`, SuperH `dt`,
VAX/PDP-11 only really.

## 17. Switch via jump table

**dmr-cc.** An inline compare-and-branch sequence in case order (no table) for
small switches, a table for dense ones (`range <= 3 * ncase` in c1). **32V.**
`casel`.

**pcc.** The PDP-11 `mygenswitch` (`arch/pdp11/code.c`) emits a table of label
words when it is smaller than the compare chain for the same cases, and the
selector goes through `jmp *table(r)` after a range check; the table is an
`IP_ASM` node in the IR so the optimizer sees the labels (`SKIP_UNREF_LABELS`
would otherwise prune the targets: `savlab` records them, `swlabel` is the
wrapper in pass 1). It saved 792 bytes over the commands. dmr's own rule is
`range <= 3 * ncase`; ours is "table if it is smaller than the chain", which
also accounts for the range check. Test: `pcctests/switchtab.c`.

**VAX.** `mygenswitch` in `arch/vax/code.c`, same rule (a table when it is smaller than
the chain, `chain` counted from the length of each `cmpl`/`jeql`, `table` from `casel`'s two
literals and two bytes per entry).  The dispatch is an extended-asm node, `casel %0,$min,$range`
with the selector as an `"r"` input (the allocator puts it in a register: this is the same
`XASM` the `ffs` builtin uses), followed by an `IP_ASM` node holding `Ltab:` and one
`.word Lcase-Ltab` per value in the range, gaps pointing at the default.  That is the layout
32V's cc writes, so the VAX c2 port recognises it and renumbers the table label.  Out of range
falls through, so a switch with a default gets a `branch(default)` after the table; without one
the fall-through is the end of the switch (pass 1 emits the dispatch after the body).  Case
labels are recorded with `swlabel` so nothing prunes them.  `advance()` in grep (19 cases) went from
52 bytes over `cc -O` (model, `tools/vaxfunsize.py`) to level with it.  Test: `pcctests/casesel.c` (negative minimum, byte and
unsigned selectors, with and without default).

## 18. Entry-mask register saving

**What.** The VAX `calls` instruction saves the registers named in the word at the
function entry; no explicit pushes. Saving by *copy* into another register (what
pcc's allocator does by default) costs two instructions per register.

**pcc.** `SAVEREGS_BY_MASK` in `macdefs.h` makes `regs.c` put a used
callee-saved register in the mask instead of generating copies.

**Port.** Only machines with a hardware save mask: VAX, (formerly) 68k `movem`.

## 19. Floating immediates

**dmr.** A double whose last three words are zero (1.0, 0.5, 10.0) is the first
word as an immediate operand of the FP11. **pcc.** PDP-11 `myp2tree`/`fpimm`.
Port: not applicable elsewhere (x86 `fld1`/`fldz` only for 0 and 1).

## 20. Statement `x++` with no copy

Covered in 5. In pass 1 `ecomp()` recognizes `(t = x, x += 1, t)` at the root of a
statement and keeps only `x += 1`. Pure win on every target; gated by `-xdmr` only
for consistency.

## 21. `*(&x + k)` is the name `x+k`

**What.** Dereferencing a static address plus constant is a direct memory operand
`x+k`; no address register. **pcc.** `clocal` `UMUL` case with `symconst()`
(pdp11, vax); x86 gets this from generic code.
**Trap.** The NAME node keeps the offset in its value field, and a target's
`clocal` `NAME` case may reset that field (the pdp11 one does for statics declared
inside a function), so check what looking at the node a second time does.

## 22. Dead mask test (open)

`(unsigned char)x & 0x100` is always 0; `c2`'s `bitopt` deletes the test and the
branch when the bit number is outside the operand size ("you dummy!"). It is
only valid for **unsigned** narrow operands: a signed char is sign-extended, so
bit 8 is its sign bit. Best done in the front end as a constant fold for every
target (`trees.c`, AND of a zero-extended narrow value with a constant that has
no bits in the low part).

## 23. Unsigned divide without helpers

**What.** dmr-cc divides an unsigned by `clr r0; div x,r0`, which is right only when
the divisor is below 32768.  pcc keeps that for a constant divisor of 2 to 32767
(the one case that is always right) and calls a helper for anything else.
**Why a section.** The helpers (`udiv`, `urem`, `uldiv`, `ulrem`) are not in V7's
libc, so they are written into the assembly of each file that calls them, as
routines local to the file; `ZK`/`Zk` templates note the use and `ejobcode()`
appends them.  Any new target whose C library lacks a helper the code generator
calls has the same choice: ship it with the compiler or inline it.

## 24. Register allocator hints

The PDP-11 work found several allocator issues worth knowing for any port:

* fixed result registers need `addalledges()` in `regs.c` (NRES) or a live value
  can be clobbered;
* pinned `register` variables can starve long arithmetic: retry without them;
* spill choice (`nospill`, upstream `ee0b0c4c`) mattered for ~6 bytes of libc.

---

## 25. Compare two bytes in memory

**What.** `cmpb (r4),(r3)+` against loading both into registers and comparing (+4 bytes
per compare in strcmp-like loops). **pcc.** `clocal` narrows a compare of two operands that
were widened from the same narrow kind back to a byte compare (`arch/pdp11/local.c`,
`arch/vax/local.c`); i386 and amd64 got the same in the `CBRANCH` case of their
`local.c` (they had none). The PDP-11 table has `cmpb` with memory/autoincrement
operands. Test: `pcctests/cmpbytes.c`, `cmpchar.c`. **Port.** Needs a compare instruction with
memory operands, so not RISC targets.

## 26. `c ? a : b` as a condition

**What.** `while (quote ? readchar() != EOR : !eol(readchar()))`: dmr branches from each
arm; pcc made a 0 or 1 in a register and tested it again (`mov $1,r0; jbr; clr r0;
tst r0; jeq`). **pcc.** `andorbr()` in `cc/ccom/trees.c` gets a `QUEST` case under
`-xdmr`: the condition branches to a new label, each arm is run through `andorbr`
with the same true and false labels, and a jump to a join label is added after the
first arm when either label is "fall through". `comops()` must be applied to each arm
first (they are not reached by the usual pass). Pass 1, so every target. Test:
`pcctests/questcond.c` (also in `x86tests`, `a64tests`).

## 27. `x op= e` in a condition

**What.** `if (++temp > 1)`: `inc L1; cmp $1,L1`, not load, increment, store and
compare through a register. **pcc.** In `andorbr` (`-xdmr`): the update becomes a
statement and the variable is compared. Not for a register variable, a plain
assignment or a compare with 0, where the old form is smaller (the update sets the
codes). `updates()` in `trees.c` decides.

## 28. Long in memory, updated in place

**What.** dmr keeps a `long` in memory and updates it there: `add r0,-12(r5); add
r1,-10(r5); adc -12(r5)`. pcc keeps longs in register pairs, and when the allocator
spills one it loads, updates and stores around every use.

**pcc (`arch/pdp11/local2.c`).** Three parts.
(a) `longrw()` rewrites `x = x + e` / `x = x - e` on a long in memory as one
node with `LONG_ADD`/`LONG_SUB` in `n_qual`, which `longaddsub()` prints (a constant
under 64K uses `add $c,lo; adc hi`). (b) `x *= e`, `x /= e`, `x %= e` become calls
to `almul`, `aldiv`, `alrem` with the address of `x` (`longcall()`), as dmr does; the
library routines update in place. (c) `longmem()` decides which long temporaries go
to memory: when two or more longs live across calls (a long multiply or divide is a
call) the ones updated with a non-constant `+=`/`-=` do, and an updated long
**parameter** always uses its own slot (`lm_param`/`lm_demote`; no copy). The rule
is narrow on purpose: every long in memory made libc 38 bytes smaller and commands
316 bigger. Order matters: count the longs live across calls *before* demoting
parameters, or the locals stop qualifying (found in fseek). Tests:
`pcctests/longinplace.c`, `longcall.c`, `longmath.c`.

## 29. Store through a pointer kept in memory

**What.** `*iop->_ptr++ = c` with the pointer in a global or the frame: `movb
r3,*10+__iob; inc 10+__iob`, not a copy of the pointer in a register first.
**pcc.** `postincfold()` in `mip/optim2.c` (targets that define `MEM_DEFERRED`: pdp11,
vax) turns `t = x; x = x + k; use(*t)` into the use first and the increment after;
`SSTARX` is the PDP-11 special shape for `*name` (a deferred operand) with its own
load/store/inc/tst rules in `table.c`. Only for a global or a frame variable: through a
register the deferred form is `@0(rN)`, an index word, as long as loading the pointer.
A store-to-`*name` that used `mov AR,AL` miscompiled `++*ip` as `mov $1,*_ip`; the rule
is "ZV" (see `pcctests/postincmem.c`, `derefglobal.c`).

## 30. Small struct assignment

**What.** A 2 or 4 byte struct assigned as a statement is one or two `mov`s, not the
counted block move. **pcc.** `smallstasg()` in `arch/pdp11/local2.c` (malloc's
`*t++ = *s++` on a union of one word).

## 31. Low word of a long; zero high word

**What.** `(int)l` of a long in a register pair is `mov r1,r0`, no arithmetic;
`(long)u` of an unsigned has a zero high word, and as an argument that is `clr
-(sp)`. **pcc.** `lowword()` (needs 16-bit int and 32-bit long, guarded by size);
the zero high word is the c2 peephole `clrmov` (item 33). Tests: `lowword.c`,
`clrarg.c`.

## 32. The last update of a register variable that is also the result

**What.** `return i | (getc(f) << 8)` with `i` in r3: dmr `bis r3,r0`; pcc built the
`or` in r3 (the left operand) and moved it: `bis r0,r3; mov r3,r0`.  Two pieces.

(a) `retfold()` in `mip/optim2.c` (`-xdmr`, all targets): `r = E; ret = r` directly before
the return, where `r` is a register, becomes `ret = E`.  It needs `toepilog()`, "the next
statement jumps straight to the epilogue" (a goto to a label followed only by labels and
`EPILOG`, or no statement at all), because then nothing but the result is live.  It works on
`REG` nodes: a register variable on the PDP-11 is a `REG` in the IR, not a `TEMP`, so a pass in
`regs.c` written for `TEMP`s never fired.

(b) The `OR` itself is a **matcher** fix, and it helped every target.  `chcheck()` in
`mip/match.c` upgrades an operand that a template modifies (`RLEFT`/`RRIGHT`) from "direct" (1) to
"must be in a register" (3), so a register that already holds the value cost the same as one
that has to be loaded, and a rule that leaves its result in the right operand (`bis AL,AR`) tied
with the plain one; the earlier rule won.  `findops()` now ranks such an operand one cheaper
(`inplace`), without changing the code `shswitch()` uses to walk the tree.  Alone it made the
PDP-11 commands 38 bytes smaller, i386 13 and amd64 24, and the VAX unchanged.  With it a
table rule is enough: `OR` may leave its result in the right operand's register
(`arch/pdp11/table.c`), and `x | 040` still gets `bis $40,r0` (the first attempt at the table
rule, before the ranking fix, produced `mov $40,r1; bis r0,r1`).  An IR operand swap I wrote
first was a workaround for exactly this ranking defect, matched only `REG` nodes, and is gone.

## 33. c2 peepholes that need the label graph (PDP-11 c2 port, `cc2/cc2.pdp11.c`)

`ccloc` (what register the condition codes describe) is forgotten at every label.
Three additions look at the predecessors of a label:

* `joinscc`: `mov rN,rN` or `tst rN` right after a label is redundant when every way
  in sets the codes from `rN`: the instruction before the label unless it jumps away,
  and every `jbr` to it (all references must be unconditional `jbr`s: `l->refc` must
  equal the number found). Only `mov`, `bic`, `bis`, `com` and `clr` count (the others
  can set the overflow bit, which a signed branch reads and `tst` clears). Test:
  `pcctests/ccjoin.c`. fgets and the whole `getc` family.
* `movjoin`: `mov S,r0 ... L: mov r0,D` with `r0` dead after (only moves that do not
  mention r0, then `jsr pc,...`) becomes `mov S,D` in each predecessor: the arms of
  `c ? 1 : 2` as a call argument go straight to the argument slot (`(sp)`). Test:
  `pcctests/qarg.c`.
* `clrmov`: `clr r0 ... mov r0,D` with `r0` dead, only plain moves between: `clr D` at
  the place of the second move.

All three use `r0dead()`: the VAX c2 has the real liveness (`findlive`), the PDP-11 one
does not, and calls clobber r0, which is what makes "dead before the call" decidable
from the text.

## 34. A char from a word in memory

**What.** `char c1 = i` with `i` an int in memory: `movb 4(r5),r0`, not `mov 4(r5),r0;
movb r0,r0`. The low byte is at the same address on a little-endian machine.
**pcc.** `narrowload()` in `arch/pdp11/local2.c` rewrites `SCONV(char, NAME or OREG of
int)` as the same operand typed `char`. Test: `pcctests/narrowload.c`. When the source
is an address-taken temporary the store comes after template selection and this does
not apply (flsbuf).

## 35. `sizeof` is unsigned

See "Defects in upstream pcc". On a 16-bit target it matters for size too: the
product is unsigned, so widening it to long is `clr`, not `tst; sxt`.

## 36. The lvalue of `x op= e` and `x++` is used twice: keep its pointer once

**What.** pass 1 rewrites `x op= e` as `x = x op e` and `x++` as `t = x, x = x + 1, t`
with a *copy* of the lvalue.  Unless the lvalue has side effects the copy is the same
tree again, so `map[(ino - 1) / 16] |= m` computed the index three times (dump's `mark`
+88 bytes against `cc -O`), and `iop->_ptr++` with `iop` a parameter loaded `iop` twice
into two registers, so the two operands were different `OREG`s and `incl` could not match
(`getc` is this, everywhere).  32V's cc computes the index once.

**pcc (`cc/ccom/trees.c`, `-xdmr`, every target).**
`addrcost()` counts the work in an address: binary operators (not a constant added,
subtracted, shifted or multiplied in: the scale of an index), a `UMUL` inside it, a pointer
variable in memory (it has to be loaded to be indexed from), a call.  When the count is 2 or
more, `hoistbase()` puts the pointer part in a temporary and leaves a constant offset in the
address: `p->n += 3` is `t = p; t->n = t->n + 3`, which on the VAX is
`movl 4(ap),r1; incl 4(r1)` and on the PDP-11 `mov 2(sp),r0; inc 4(r0)`.  A bare
`*gp` (address is a name) is left alone: a deferred operand is one instruction.
Hoisting the *whole* address was tried first and was worse on the PDP-11 (`gb->n++` became
`mov $4,r0; add _gb,r0; inc (r0)`; commands +88 bytes): keeping the offset in the addressing
mode is the point.  Tests: `pcctests/opeqaddr.c`, `ptrmember.c`.

**A bug this found, in every configuration:** `has_se()`, which decides whether the lvalue
needs a temporary at all, did not treat a call as a side effect, so `map[idx(35)] += 5`
called `idx` twice.  It does now.

## 37. `x++;` as a statement leaves no copy

pass 1 writes `x++` as `((t = x, x += 1), t)`, and the `+= 1` is itself a `COMOP(0, x = x + 1)` whose
`0` is a placeholder, so the whole thing is `COMOP(0, COMOP(COMOP(t = x, ...), t))`.  `ecomp()`
recognises the statement form `x++;` and keeps only `x += 1`, but it looked at the root, where
the placeholder was, so for most variables the fold never fired and the copy `t = x` stayed (`movl
r11,r0; incl r11` for a register variable; the VAX cannot rely on DCE to remove it, `dce()` is off
there).  `ecomp()` now skips a leading `COMOP(constant, X)`, and a statement that is only a
constant is dropped.  The VAX had worked around this with `AUTOINC_MEM`, which wrote an integer
`x++` as `(x += 1) - 1`; with the fold fixed the plain form is smaller in every case
(`movl _n,r0; incl _n` against `addl3 $1,_n,r0; movl r0,_n; decl r0`), so `AUTOINC_MEM` and the
special case in `unusedval()` that existed for it are removed (VAX libc 19056 -> 19040).  Test:
`pcctests/stmtinc.c`.  Lesson: a pattern matched at the root of a tree breaks silently when a
rewrite adds a wrapper; check the real shape (`p1fwalk`) before trusting the fold works.

## 38. Frame-slot pointers: deferred operands and use-first increments

**What.** `putc('\0', &buf)` on a local struct stores through `buf._ptr`, a pointer in the frame:
dmr `clrb *-16(r5)`; pcc loaded the pointer into a register first.  **pcc.** Two things were wrong.
`isptrvar()` (the test `postincfold()` uses to reorder `t = x; x = x + 1; use(*t)` into use-first)
accepted `NAME` and the `UMUL(r5 - k)` spelling of a frame variable, but at that point a frame
variable is already an `OREG k(r5)`, so no frame pointer was ever reordered.  And the PDP-11
shapes `STARNM` (`shumul()` in `order.c`) and `SSTARX` (`special()`) matched `UMUL(NAME)` only; both
now also match `UMUL(OREG k(r5))`, non-indexed, so every existing deferred rule (byte stores,
`clrb`, loads, `inc`, `tst`) applies, printed `*-16(r5)`.  Test: `pcctests/frameptr.c`.

## 39. A parameter used in place when the uses are cheap

`parmprop()` (`arch/pdp11/local2.c`) uses a parameter from its stack slot instead of copying it to
a register.  The copy is one 2-word `mov 4(r5),r2`; an operand `4(r5)` has an index word, so a use as
an operand (`cmp`, `add`, `mov` to the stack, a same-type `mov`) costs one word more than `r2`, and a use
that needs a register anyway (the base of a dereference, a conversion, a shift, a store to a char)
costs the 2-word load again.  In place when the extra words are at most 2 and there are at most two
uses (`ptcost()`).  Two earlier attempts: every two-use parameter in place (libc +14), and the same
with all uses counted as one word (the char store case lost).  After the replacement `narrowload`
runs again, because a slot under a char conversion is only now visible.  The gain is small (a few
bytes, `fputc`) and is mostly that the register stays free.

## 40. VAX: a parameter used once or twice is read from its slot

With `-xtemps`, pcc loads every parameter into a temporary at entry (`movl 4(ap),r1`) so the
register allocator (the pass that decides which values live in which registers) may keep it in a
register.  A register operand is one byte and `4(ap)` is two, so the copy, three bytes, pays for itself
only from the third use.  32V's cc never copies a parameter at all.

`parmprop()` in `arch/vax/local2.c` (run from `myreader`, under `-xdmr`) removes the entry load of a
parameter that has at most two other occurrences, reads or writes, none of them the base of an address
or a call argument that needs a register, and gives each occurrence the slot as an operand.  A parameter
nothing reads has its load deleted (`cfree(p, n, size)` loaded two words for nothing).  `stty`:

```
movl 4(ap),r1; movl 8(ap),r0; pushl r0; pushl $1; pushl r1     ->     pushl 8(ap); pushl $1; pushl 4(ap)
```

Two supporting changes.  The entry load arrives in pass 2 as the tree `UMUL(PLUS(REG ap, ICON))`, and for
a `double` or a `char` parameter under a conversion (the caller passes a `double`, or an `int`, and the
function takes it back as its declared type); `argoreg()` drops a conversion that only retypes the slot
(the same type, or an integer narrowed from a longer one: the first bytes of a little-endian slot are the
low ones) and makes the load the `OREG` (a register-plus-offset operand) it would become anyway, which is
what `parmprop` and the allocator's `temparg` look for.  `temparg` (`mip/regs.c`: a spilled parameter keeps its
own slot instead of a second copy in the frame) stopped at the first label after the prologue; on the VAX
the arguments are loaded after a second label, so it never found them.  `ARGSAFTERLABEL` in the VAX
`macdefs.h` lets it skip the label.  `temparg` is what lets a spilled parameter keep its own slot: `modf`'s `double arg`, modified all
through the function, is now updated in `4(ap)` as 32V's cc does, where it was copied to `-12(fp)` first.

Result: VAX libc 18724 to 18576, commands 107612 to 107240 (-148, -372).  Tests: `pcctests/parmslot.c`
(parameters used 0, 1, 2 and many times, modified, passed on, a double, a char) and `dblslot.c` (a double
parameter updated in place; char, short and unsigned char parameters read narrow).  The PDP-11 version (39) uses a
cost model because a PDP-11 operand with an index word costs a whole word.

## 41. A table jump is a jump the flow graph must know about

A dense `switch` on the VAX is `casel sel,$min,$range` followed by a table of word offsets (17).  In pcc
it is an `XASM` node (inline assembly) and a block of assembly text, so the flow graph (the map of which
block can run after which, `mip/optim2.c`) saw one block running into the next and no edge to the case
labels.  A variable that was live across the switch therefore looked dead at the `casel`, and the allocator
gave its register to the selector:

```
neg(c)  { r = 0; switch (c) { case -3: r += 1; ... } return r; }
	clrl r0			r = 0
	movl 4(ap),r0		the selector, in r's register: r is gone
	casel r0,$-3,$5
```

It had been hidden because the selector was usually copied to another register first; once a parameter was
read from its slot (40) the copy was gone and `pcctests/casesel.c` failed.  `iscasel()` now ends a basic block
at the instruction and gives it an edge to every label of the function, as a computed `goto` has.  Test:
`pcctests/casellive.c` (a variable live across a dense switch, the selector a global and a parameter).
Lesson, for any target: a jump that goes through a table the IR does not contain has to say where it can land.

## 42. c2 knows how wide a register write is

`movb (r10)+,r0` does not replace `r0`; it replaces its low byte.  c2's liveness (which registers hold a
value that will still be read, `findlive()` in `cc2/cc2.vax.c`) was conservative about it: a byte or word
written into a register counted as a read, so `r0` looked live around every byte loop and the fetch
elimination could not turn

```
movb (r10)+,r0; movb r0,(r11)+; tstb r0; jneq L        into        movb (r10)+,(r11)+; jneq L
```

the form 32V's cc produces for `while (*s++ = *t++)`.  Liveness now keeps two masks per register in one
integer, "the whole value is still needed" and "some of it is": `insrw()` reports what an instruction reads
whole, reads as a byte or word, replaces whole and replaces in part, and only a whole write ends the whole
value.  A byte or word `mov`, `tst` and `cmp` of a plain register are the narrow cases; every other
instruction keeps the conservative reading.  Cleared `strcat`, `strcatn` and `strncat` and part of `doscan`; together with 43 it took 68 bytes from
libc and 260 from the commands.  Test: `pcctests/c2peep.c`.

## 43. A compare with zero after a move or conversion is redundant

`mov r,x; tst r` and `cvtbl m,r; tstl r`: the first instruction already set the condition codes from the
value (the signed result: negative, zero), and `tst` would set the same.  `tstaftermov()` deletes the `tst`
of a register that a `mov` of the same width has just stored, and `cvtcc()` records the type of a `cvt` or
`movz`'s *destination* as the type its codes belong to (a `cvtbl` leaves the codes of a long; a `cvtlb`, of a
byte, so `cvtlb` then `tstl` is kept).  Both only before a signed or equality branch: `tst` clears the overflow
and carry bits and a `mov` leaves carry alone, which the unsigned branches would see.  The same rule as for
`incX` and `decX`.  The first has to run before the fetch elimination of 42, or two readers of the register
hide the pattern.

## 44. c2: two small peepholes

* `movl X,rN; movb rN,Y` is `movb X,Y`: the byte store reads only the low byte of `rN`, and the low byte of
  `X` is at `X`'s address on a little-endian machine.  Not for a constant (`$300` does not fit the byte
  operand), an indexed source (the index scales by the size of the operand) or a source with a side effect
  (`(r10)+` would step by 4).  `*iop->_ptr++ = c` in `putc`.
* `subl3 $1,M,rD; movl rD,M` with `rD` still wanted afterwards is `decl M; movl M,rD` (and `addl3 $1`, `incl`):
  two bytes less for a memory operand with a displacement.  `*--iop->_ptr = c` in `ungetc`.  When `rD` is not
  wanted the existing fold (the result goes straight to its destination) does better, so the new rule yields
  to it.  It reads `M` once more than the original, which matters only for a volatile location, and pcc
  passes no `volatile` to c2 (nor does any other c2 rule).

## 45. A function that returns nothing does not want r0 at its `ret`

c2's liveness treats `r0` (and `r1` after a quad return, `c2_quadret`) as read by every `ret`: it is the
return value.  For a function that returns nothing that is a waste, and it blocked the fold
`movl 8(ap),r0; movl r0,8(r11)` to `movl 8(ap),8(r11)` in `setbuf`.  32V's c2 never looked across a branch, so
it never had the problem (see "Defects in the originals").

ccom knows whether the function returns a value: a `FORCE` node, an assignment to `r0` or to the pair `r0:r1`,
or a struct return (`vax_retagg`, set in `bfcode`).  Without one it writes `ret	# noval` (both places that
print a `ret`: `eoftn()` and the `J` code for a jump to the epilogue) and c2 reads the marker: nothing is live
at that `ret`, and a `ret` with a value and one without are never cross-jumped into each other
(`comjump`).  c2 prints the comment as an ordinary assembler comment.  With the exact liveness a second rule
became safe: a `mov`, `cvt` or `movz` into a register that nothing reads afterwards, from a source with no
side effect, is deleted (`deadwrite()`, `*iop->_ptr++;` as a statement loaded the old pointer into `r0` for
nothing), unless the next instruction branches on the codes it set.

A bug of mine that this surfaced: the first scan for a returned value looked for an assignment to `r0` or `r1`
and missed the register pair that a double or a long long returns in (`XR0`), so `modf` got `ret # noval`.
`pcctests/retvalue.c` returns a double, a long, a struct and an int, and an int function that uses `r0`
freely and returns nothing.

## 46. Functions start on a word, not a longword (VAX, `-xdmr`)

pcc aligns a function to 4 bytes (`ALFTN`); 32V's cc used `.align 1`.  The entry mask that starts a VAX
function is a word, and nothing else needs more.  `-xdmr` sets `ALFTN` to a word on the VAX: 40 bytes in libc,
396 in the commands, in padding.  It trades the speed of an aligned entry for size, as the rest of `-xdmr`
does; i386 and amd64 are not changed.

## 47. c2 says so when it fails

`ccom -xc2` runs c2 in a forked copy of itself; if the child failed it wrote the unoptimized assembly and said
nothing.  Twice that made a c2 failure look like a code-size regression (the `+106` of an uninitialised
`sscanf %n`, the first time).  `common/c2run.h` now prints `warning: c2 failed (exit N / signal N); the code is
not optimized`.

## 48. VAX: indexed operands of 8-byte objects, frame-relative indexing

An index register on the VAX scales by the size of the *operand*: `movl base[r1]` steps 4, `movq base[r1]`
steps 8.  pcc splits a `long long` (and a double treated as a pair) into two longs at displacements 0 and 4,
and the halves would be indexed with a scale of 4: wrong.  An earlier version simply turned the indexed
form off for them; the fix keeps it.  `quadaddr()` (`arch/vax/local2.c`) rewrites `*addr` of a quad element
whose address is `base + scaled index` to `*(&*addr)` where the inner access is only an address
(`n_qual == QADDR`) computed by `movaq base[index],rX`, whose scale is 8, and the halves are `(rX)` and
`4(rX)`.  `quadindex()`/`indexform()` (`order.c`) recognise base-plus-index forms, now also `fp-k`
(a local array) and a symbol, and exempt the `QADDR` access from the check that a `UMUL` is already an
address.  The address register is 32 bits, as the whole VAX is.  Tests: `pcctests/frameindex.c`, `indexed.c`.

## 49. VAX: bit tests and masks

`if (x & 0200)` is `bitb $128,x; jeql` and c2 makes it `jbc`/`jbs`.  `bittest()` rewrites
`EQ`/`NE` of `(x & mask)` against zero so the compare takes `x` and the mask directly and marks the node
(`n_qual = BITTEST`); the compare rules choose the operands, including a byte through a pointer, and `ZQ` prints
`bit` for `cmp`.  A byte that was widened is tested as it lies when the mask is inside it.  `maskzext()`:
`(int)c & 0377` is `movzbl x,r` (a mask that keeps exactly a byte or a word is a zero-extending load, as
in `getc`'s `*p++ & 0377`).  The first version mutated the tree before it knew the operand fitted and died with
`cannot generate code` on `((p->c |= 0100) & 0100)`; the operand is checked first now.

## 50. VAX: a narrow load pushed in one instruction, a struct copy source as an address

* `SCVTARG`: a `char` or `short` in memory passed as an argument is `cvtbl m,-(sp)`, not a load into a register
  and a push.  A template cannot say it, because the memory operand is a `UMUL` whose child the matcher turns
  into an `OREG` only for the node it matched (tried first, in the ledger below), so it is a special shape
  accepted for `NAME` and `OREG` operands; c2 fuses what remains.
* `SSTRSRC`: the source of a struct assignment that is `&name` or `&frame-object` is used as the operand
  of the copy itself, without first putting its address in a register.  Test: `pcctests/structcopy.c`.

## 51. VAX: `x & ~y`, double complement, r1 at a `ret`

`x & ~y` is `bicl3 y,x,r` (bit clear takes the complement itself).  `newmode` showed a complement of a
complement, `mcoml r0,r0; mcoml r0,r0`, where the operand of the `~` was already one; the look-through of
`COMPL` in `optim2()` and `wordint()` (the test for a type that is one word, so a conversion around it does
not hide the pattern) takes the pair away.  Test: `pcctests/bicmix.c`.  c2: `repladdr()` must not turn an `inc` pattern into a
use of the copy, and the `cvt`/`movz` fusion (a `cvtbl` followed by a `movzbl` of the same value) is allowed
across the cases it used to refuse.  `c2_quadret`: `r1` is live at a `ret` only if some function in the file
returns a quad or a double; ccom sets it in `bfcode`.  Without it `r1` was live at every `ret`, which kept
the fetch eliminations from firing on any value that went through `r1`.  Test: `pcctests/quadret.c`.

## 52. PDP-11 c2: autoincrement from `add $2,rN`

pcc makes pointers in registers that are not `register` variables, and only `register` variables got
`(rN)+`.  c2's `autoinc()` (`cc2/cc2.pdp11.c`): `add $2,rN` (`inc rN` for a byte) just before the one
instruction that uses `-2(rN)` makes that use `(rN)+`; `sub $2,rN` before a use of `(rN)` makes it `-(rN)`.  The
PDP-11 steps as the operand is evaluated, so nothing else changes, and both forms set the codes the same way.
`nregref()` counts the uses of a register in an instruction's operands, so the rewrite is made only when the
register is used once.

## 53. PDP-11 c2: a spilled value waits on the stack

```
mov r0,-16(r5); mov r1,-14(r5); <loads>; sub -16(r5),r0; sub -14(r5),r1
	becomes   mov r1,-(sp); mov r0,-(sp); <loads>; sub (sp)+,r0; sub (sp)+,r1
```

dmr-cc pushes a long that has to wait for another to be computed; pcc stores it in frame slots.  `spillpush()`
does the dmr form when the slots are used only here and nothing between touches the stack or leaves the
straight line (`funcextent()`, `slotrefs()`).  An uninitialised `sscanf %n` in it crashed ccom on `c20.c`; the
failure was hidden by c2 falling back (47).  `fseek`.

## 54. PDP-11: `*(p += k) - k`, the use of a pointer kept in the frame

`p++` on a pointer that the allocator keeps in the frame is `UMUL(MINUS(ASSIGN(v, PLUS(v,k)), k))` by the time
pass 2 sees it: add, then use the old value.  `SSTARINC` (`stinc()`) matches it for a frame variable `v` and the
rules make the access through `v` first and `add $k,v` in place after (`ZM` loads, `ZO` stores or copies), as
dmr-cc does.  It covers a pointer that the allocator spills as a long-lived temporary (`malloc`, `realloc`);
a pointer spilled by the short-lived mechanism is not covered (ledger 13, 14).

## 55. PDP-11: conversions among the one-word types change only the type

`int`, `unsigned`, `short` and pointers are one word, so `(int)p` or `(unsigned)i` of a plain operand changes
nothing but its type.  As a node it is a template that moves its operand into a register to convert it
(`mov r1,r0; mov r0,*_buf`); `nopconv()` retypes the operand and drops the node.  `mon`.

## 56. PDP-11: a long made from a word is pushed as `mov x,-(sp); sxt -(sp)`

`((long)bufsiz << 15) / o` holds a register pair for the widened operand while the other operand is built.
dmr-cc widens straight onto the stack.  A `SSXT` shape (`widened()` in `arch/pdp11/local2.c`) matches a
`SCONV` of a word that is a register, a name or a frame slot, and the helper-call templates (`lmul`, `ldiv`,
`lrem`, `uldiv`, `ulrem`) push it with `mov x,-(sp)` and `sxt -(sp)` (`clr` for an unsigned source).  Test:
`pcctests/longwiden.c`.  The same file in `mon` uncovered a miscompile of an older template:

The long left shift is `ashc AR,A1` on the pinned pair `r0:r1` (`NLEFT(R01), NRES(R01)`), but `A1` printed
the register the allocator coloured for the *result*, which can be `r2:r3` after a copy; the shift then ran
on the wrong pair.  The template prints `r0` now.  `monscale.c` returns the `profil` scale that `monitor()`
computes and compares it with a reference.  It had passed for weeks because of the test convention bug
described under "Bugs found by compiling real programs": a failing first assertion returned `bad == 1`, which
is also the pass value.

## 57. c2: a double result goes straight to its destination

pcc moves a `double` with `movq` (the same eight bits as `movd`).  `muld3 a,b,r0; movq r0,Y` has a reader of
a different type than the writer, so the fetch elimination, which wants the same type, left both instructions
and a trip through `r0:r1`.  `pairfold()` accepts a quad move as the reader of a double that a three-operand
instruction, a move, a negate or a conversion has just computed, if nothing reads the other half of the pair,
`r1`: `mnegd 4(ap),r0; movq r0,4(ap)` is `mnegd 4(ap),4(ap)`.  Two-operand forms read their destination and
are not folded.  Test: `pcctests/dblslot.c`.

## 58. c2: a `casel` does not read every register; the backward pass runs again

Liveness took a `casel` (the dense switch) to read all of `r0`-`r5`: the selector is one of them and where
the jump goes is a table, so it said everything.  The table follows the `casel` as one jump node per case, each
with its label, so the targets' liveness is already in the walk; a `casel` now reads its selector and the
registers live at its cases and its fall-through.  Any loop that contains a switch (`doscan` has one)
had `r1` and the rest live around the whole loop.

The backward pass (`bmove`/`bflow`: fetch elimination, results straight to destinations, `fwdaddr`) ran once, before
the main loop.  The main loop makes copies of copies: three loads of the same stack slot become
`movl 4(ap),r2; movl r2,r1`, which `fwdaddr` can use only if it runs again.  It does now, until nothing changes.
`getc` through a spilled `FILE *`: `movl 4(ap),r2; movl r2,r1; addl3 $1,4(r1),4(r2)` is `movl 4(ap),r2; incl 4(r2)`.

## 59. PDP-11: long long is a 64-bit object in memory

The PDP-11 has no 64-bit operations and no registers to hold one, and pcc's backend used to map `long long`
to `long` (`sizeof` was 4; a comment said "remove as soon as 64-bit is added").  It is now eight bytes
(`SZLONGLONG`), four words with the most significant first, as a long is two words with the high one first.
Every operation on it is a call to a helper that takes the addresses of its operands:

```
a + b          (_lladd(&t, &a, &b), &t)          t a temporary on the stack
a < b          _llcmp(&a, &b) < 0                (_llucm for unsigned)
(int)a         (int)_lltol(&a)                   (long)
(long long)i   (_llfls(&t, (long)i), &t)         (_llflu for unsigned sources)
```

`lowerll()` (`arch/pdp11/lowerll.c`, called from `ecomp()` after `optim()`) walks each expression tree after
pass 1 has typed and converted it as for any integer type, and rewrites every node whose value is a long long
into an expression whose value is the *address* of the result, and every consumer of one (a comparison, a
conversion to int, a condition, a call argument, a `?:`) into a call or a test of one.  Pass 2 never sees a
long long computation, only pointers, ints and calls.  Operators handled: `+ - * / %`, `& | ^`, `<< >>`
(arithmetic for signed, logical for unsigned), unary `- ~ !`, all comparisons (the unsigned ones included),
`&& || ?:` and `if`/`while` conditions, `=` and every compound assignment, `++` and `--` in both forms,
conversions among every integer type, arrays, struct members, pointers to long long, and constants.

* **Passing and returning.**  An argument is its four words, pushed so that the memory image on the stack is
  the object (read from a copy made before the call).  A result comes back as the address of a static buffer,
  `_llret`, in `r0`, as V7 returns a struct; the caller copies it out at once.  `cgram.y` calls `llreturn()`
  for a `return` in a long long function.
* **Constants** are words in the text of the file (`__Lc0`, ...), referred to by address; a static
  initializer writes four words (`ninval()`).
* **The helpers** are C (`arch/pdp11/ll.c`, using no long long), compiled by this compiler,
  turned into `llhelpers.h` by `arch/pdp11/genll.py`, and written into each file that calls them, as routines local
  to the file, like `udiv`: a program that never uses a long long pays nothing, and one that does pays for the
  helpers it uses and the helpers those call (`llh_deps`).  Division is 64 rounds of shift and subtract.
  `make` has no rule for `llhelpers.h`: run `genll.py` after changing `ll.c` or the code generator.
* **Unsigned long `>>`** (any program, not only long long) is a call to `_llulr` now: `ashc` shifts
  arithmetically, so the old code shifted ones into the top of an unsigned value (see the defects chapter).

**Not supported** (each is a clean diagnostic, not a crash): conversion between a long long and
`float`/`double`, `switch` on a long long.  Division by zero returns all ones and the dividend as remainder
(no trap).  Bit-fields of long long are accepted by the declaration and not tested.
Tests: `pcctests/llfull.c` (variables, arrays, structs, parameters, results, conversions, every operator),
`llops.c`, `llcmp.c`, `llconst.c`; `ll.c` was also checked against a 64-bit host over 13 edge values and all
operators (every shift count 0..69).

## 60. VAX: long long helpers, and what running them found

The VAX backend does add, subtract, and, or, xor, compare and shift left and arithmetic right inline, as pairs of
longwords (`addl2`/`adwc`, `ashq`), and called `__muldi3`, `__divdi3`, `__moddi3`, `__udivdi3`, `__umoddi3` and
`__lshrdi3` for the rest, none of which 32V's libc has: a program that multiplied two long longs did not link.
`ejobcode()` (`arch/vax/code.c`) now writes the ones a file calls, as routines local to the file, like `__udiv`
(`__muldi3` from `emul` with the sign corrections, the divisions as 64 rounds of shift and subtract with `ashq`
and `sbwc`, `__lshrdi3` from `ashq` and a mask).  Test: `pcctests/llops.c` on real 32V.

Using them found four defects that were already there (checked against the pushed commit; see the defects
chapter): 32V's assembler gives a quad immediate a zero high longword (so `-100000LL` was a large positive
number); `addl2`/`adwc` claimed to leave the condition codes of the whole quad, so a compare of a computed quad
was dropped (`x + 1 != 0` was always true); a static long long was written with `.quad`, which 32V's `as` has
not got; and the low half of a quad constant was printed as the whole constant.  Constants outside 0..2^31
are now memory operands (a pool written out at the end of the file, `qpool`), static data is `.long lo,hi`,
and a quad add/subtract no longer says it set the codes.  Tests: `llconst.c`, `llcmp.c`, `llfull.c`.

## 61. VAX c2: a pointer overwritten by what it points to

`*iop->_ptr++` through a `FILE *` that lives in its stack slot compiled to
`movl 4(ap),r0; movl 4(r0),r0; movl 4(ap),r2; incl 4(r2)`: the old pointer went into the register that held the
`FILE *`, which was then loaded again.  `renameload()` (`cc2/cc2.vax.c`) rewrites
`movl S,rA; movl d(rA),rA; movl S,rB` (`S` a plain memory operand, no side effect, no register) to
`movl S,rB; movl d(rB),rA; movl S,rB`; the third is then a redundant load and goes.  That finished `doscan`
(+16 to 0 over this and 58), so the only VAX file larger than `cc -O` is `mktemp`.

## 62. aarch64: the text optimizer (`arch/aarch64/peep.h`)

The new aarch64 backend (`AARCH64.md`) keeps the assembly of the file in memory under `-xdmr` and
runs one pass over it when the job is over, as `x86peep.h` does for x86 but with a parse of every
instruction, the registers it reads and writes, and liveness over each function's labels and
branches.  It is the aarch64 home of several of the tricks above:

* **Autoincrement (2).**  AArch64 has the mode: `ldr w0,[x1],#4` loads and then adds 4 to `x1`.
  pcc's tree for `*p++` is a copy of `p`, an `add` and a use of the copy; the pass finds
  `mov xc,xb; add xb,xb,#k; ...; ldr Rt,[xc]` (and the access *before* the `add`, and the access
  after it at `#-k`) and makes it one instruction when nothing between uses `xb` or `xc` and `xc`
  is dead afterwards.  `strcmp`'s loop went from 9 instructions to 5.
* **Compare with zero, bit tests (6, 8).**  `cmp w0,#0; b.ne L` is `cbnz w0,L`; `cmp w0,#0; b.lt L`
  is `tbnz w0,#31,L`; `and w1,w0,#8; cmp w1,#0; b.eq L` is `tbz w0,#3,L` (a mask of several bits:
  `tst`).  Only when the condition codes are dead after the branch, which the liveness says.
  A second compare of what is already compared (`cmp w0,#0; b.ne A; cmp w0,#0; b.ge B`) goes.
* **Narrow values (7, 9).**  A value is right only in the bits of its type, so a `char` is extended
  before it is compared or widened.  The pass keeps what each register is known to hold (a byte
  from `ldrb` is already zero-extended; `ldrsb` is sign-extended) and drops an extension of what is
  already extended, and a `sxtb` before a test against zero when the register is a loaded byte.
* **Indexed addressing (3).**  `sxtw x0,w22; add x1,x23,x0` is `add x1,x23,w22,sxtw`;
  `lsl x1,x1,#2; add x2,x0,x1; ldr w3,[x2]` is `ldr w3,[x0,x1,lsl #2]`; the same for `sub` and
  `cmp` (the zero-extension `mov w0,w0` becomes `uxtw`).  The uses may be several instructions
  after the producer as long as nothing writes the registers in between.
* **A condition as a value.**  pass 1 makes `a < b` as a value into branches and two constants;
  `b.lt L; movz w0,#1; b M; L: movz w0,#0; M:` is `cset w0,ge`.
* **Frame (10).**  `stp x29,x30,[sp,#-N]!` and `mov x29,sp` instead of `sub sp,sp,#N; stp ...;
  add x29,sp,#0`, the registers saved in pairs, and no frame for a leaf function that touches
  neither the frame pointer nor a callee-saved register.

What it does not do: loads are never removed, merged (`ldp`) or reordered, because a `volatile`
object is read by a load the compiler cannot tell from any other.  An instruction it does not
parse is a barrier.  It was run under AddressSanitizer over the whole libc output (a one-byte
overrun in the operand splitter turned up).

libc (89 files): 47756 bytes with no flags, 33740 with `-xtemps -xdeljumps -xinline -xdce`, 28780
with `-xdmr`; on the 76 files `clang -Os` can also compile, 18852 against clang's 21187 (-11%).

**Port.** The same pass shape (parse, def/use, liveness, patterns) is what a RISC-V or ARM32
backend would want; the patterns are per target.

## How to port a trick to another backend

1. **Measure first.** `tools/x86size.sh` (i386/amd64), `tools/vaxdiff.py` (VAX under
   real 32V), `tools/pccdiff.sh` (PDP-11). Count instruction mnemonics in the
   output of the original and of pcc (`vaxops.sh` does it for the VAX): the
   biggest differences name the missing tricks.
2. **Test where it runs.** Every trick above is covered by programs in
   `tools/pcctests`, `tools/x86tests`, run on the real system or an emulator.
   A trick that rewrites a tree (inc/dec, autoincrement, index) must be tested
   in value contexts as well as statement contexts.
3. **Gate with `-xdmr`**, document in the target's `macdefs.h`.
4. **Where the rewrite lives:** pass 1 (`clocal`, `trees.c`) for tree shape;
   pass 2 `offstar`/`myormake` for addressing modes; `myreader` for marking
   nodes; the table for templates; `zzzcode` for conditional text. A text
   post-pass (`frameless()`) or a `c2` for what needs whole-function knowledge.
5. **Beware** of these traps found along the way: `R2PACK` does not parenthesise
   its arguments; `sap`/`sss` macros in older `local.c` files; `slval()` shares
   storage with `n_left`; a pass 2 `OREG` made after the child got its own
   instruction drops that instruction (mark the node in `offstar`); templates
   treating a named `ICON` as memory.
6. **Which backends build and pass today** (see `DMR-OPTIMIZATIONS-OTHER-TARGETS.md`):
   i386, amd64, VAX, PDP-11 work; MIPS and ARM partly; m68k, i86, powerpc,
   m16c, pdp7 do not build; riscv, sparc64, hppa, pdp10 do not configure.

## Bugs found by compiling real programs

Optimizations are only as good as the testing behind them. Three kinds of testing
turned up compiler bugs that the unit tests had not:

* **pcc compiles pcc** (`tools/pccboot.sh`): gcc builds stage 0, stage 0 builds
  stage 1 with `-xdmr -xtemps ...`, stage 1 builds stage 2; the compiler, the
  preprocessor, the driver and all 36 objects must be byte identical.  It found
  (a) spill-slot sharing that sized every slot as one word, so two temporaries of
  different sizes overlapped on amd64, and (b) `deljumps` inverting a relation by
  swapping the operands, which put a constant on the left (`cmpl %eax,$1`).
  Both are fixed; the self-built compiler produces the same code as the
  gcc-built one on 198 compiles once labels are renumbered.
* **V7 commands inside V7** (`tools/v7progs.py`): `pcc -O` builds the commands of
  /usr/src/cmd, V7 assembles and links them, and each runs next to the build by
  V7's own cc on the same input.  It found a crash for `register unsigned x; x >>= n`
  (`findmops` chose a rule that wants the operand in r1 for a variable pinned in
  r4), a crash for `**p++ = 0` (an autoincrement made after the increment
  already had its own instruction), and that unsigned `/` and `%` called
  helpers V7's libc does not have.
* **More from the V7 commands**: unsigned `/` and `%` called `udiv`, `urem`,
  `uldiv` and `ulrem`, none of which V7's libc has, so any program using them
  failed to link; ccom now appends the ones a file calls as local routines
  (`arch/pdp11/helpers.h`), and a constant divisor of 2 to 32767 is the inline
  `clr r0; div` of dmr-cc.  The `*(&x+k)` rewrite of item 21 dropped the offset of
  a member of a function's own `static` struct, because `clocal()` resets the
  offset of such a NAME when it sees the node again (V7 `ls` printed `/bin/e`);
  those keep the long form.  A rename of one table template also changed an older
  template with the same name and broke `f(p++)` calls.  And `findmops` may not
  do `x = x op y` in place with a rule that wants a fixed register.
* **32V under SIMH** (`tools/vax32v.py`): found that the saved 32V disk had a
  stale free list, which made everything we put on it unreliable.

* **Differential fuzzing against the host compiler** (`tools/exprfuzz.py`, written for the aarch64
  port, usable for any LP64 target).  It writes programs of well-defined C, checks with gcc under
  UBSan that they are well defined, and compares gcc's checksum with the target's; `--narrow` finds
  the first call that differs and `--reduce` shrinks the program.  It found, in code every target
  shares or that amd64 and aarch64 share:
  (a) `soft_cmp` ordered negative and zero floating constants by sign instead of magnitude, so
  `0.0 > -1.0` was folded to 0 (`common/softfloat.c`; test `fpconstcmp`);
  (b) removing `x << 0` returned the operand with its own type and lost the type the node had been
  given, so `(int)((unsigned)p << 0) % n` was "Cannot generate code" (`optim.c` `zapshift`;
  `shiftzero`);
  (c) `(x << a) << b` was merged into one shift by `a+b` even when that reaches the width, and the
  hardware shift took the count modulo 64 (`optim.c`; `shiftmerge`);
  (d) `(unsigned)(a % b)` with signed operands was retyped to an unsigned `%` by the shortcut in
  `clocal` that the PDP-11 had fixed in 2026 (amd64 and i386; `divretype`);
  (e) `(long)(int)(u << c)` became `(long)u << c` (amd64; `shiftwiden`);
  (f) a PDP-11 template for the no-op conversion `long` to `long` did not say that the result is the
  operand's register (`table.c`; `shiftzero`);
  (g) **dead code removal in `regs.c` deleted a live assignment**: when it deleted the only
  statement of a basic block it took the block out of the list but left the edges that led to it, so
  liveness lost everything that is live after that block, and a variable assigned in the loop before
  it looked dead (`dceloop`).  Loop rotation made the shape common (the block after the loop is no
  longer reached through a label), which is how it showed with `-xdmr`; `bbunlink()` now points the
  edges at the block's successor.

* **A test that could not fail** (found late in the PDP-11 work).  56 tests in `tools/pcctests`, `x86tests`
  and `a64tests` returned `bad ? bad : 1`: `bad` is a bit mask, and a failing *first* assertion sets only bit 1, so it returned
  `1`, the value that means pass.  `monscale.c` (the `profil` scale `monitor()` computes) had been "passing"
  on a miscompile.  Tests now return `2*bad+1000` on failure, which is never 1 (56 test files in
  `pcctests`, `x86tests` and `a64tests`).  Two failures that had been invisible appeared at once: `monscale`
  (the `ashc` template below) and `longcall` (an expectation of 427 for a value the comment next to it computes
  as 420).
* **A miscompile in an upstream PDP-11 template** (`ashc AR,A1`, long left shift): see "Defects ... upstream"
  and 56.
* **A latent miscompile of the VAX `casel`**: see 41.
* **pcc's own c2 failures looked like size regressions**: see 47.

## c2 inside ccom, and what running it found

`ccom -xc2` buffers its assembly and runs c2 (`cc2/cc2.<mach>.c`, built into ccom
as `c2.o` with `main` renamed `c2main`) on it, in a forked copy of itself with the
text passed through temporary files; if anything fails the unoptimized text is
written instead (and, since 47, a warning).  The driver passes `-xc2` on the PDP-11 and 32V targets instead
of exec'ing `cc2` (`PCC_EXTERNAL_C2=1` brings the separate program back).  The
output is byte-identical to the two-program path (checked on all libc and command
sources of both targets).

Making the VAX tests run c2 for the first time found three faults in the c2 sources:

* **c2 assumed r0-r5 are dead at labels and branches.**  True of the original
  ccom, whose temporaries lived within a statement; `-xtemps` keeps loop counters
  in them, so `subl3 $1,r0,r0; movl r0,r1; incl r1; jleq` became `decl r0; incl r0`
  and lost the decrement (the `swap` test crashed).  c2 now computes liveness of
  r0-r5 by dataflow over the labels (`findlive()`: blocks from label to label,
  fixpoint, indirect jumps and instructions it does not understand read all
  registers, only a write that surely replaces the whole register kills one).
  Treating everything as live first cost 1.5% on VAX libc; the dataflow gets
  most of it back (19064 unsound, 19360 all live, 19224 now).
* **Two more c2 bugs that all-live had hidden**: the fetch elimination
  (`movl rA,rB; op ..rB..`) substituted rA into `incl`/`decl`/`sob`/`aob`/`acb`,
  which modify their operand (`decl r10; incl r10` lost a decrement), and it turned
  `mull2 r0,r0` into `mull3 r0,r0,r1` whenever the second operand equalled the
  copy's source (`sq()` under `-xssa` returned garbage).
* **An empty input crashed both c2s** (a `NULL` operand pointer), and the opcode
  hash table was read one slot past its end (`>` for `>=`).  Found with
  AddressSanitizer; the VAX c2's output also depended on compiler flags because
  of it.
* The VAX unsigned-divide helpers used labels c2 does not recognise as labels, so
  c2 deleted the code after their first `ret`; they use compiler-numbered labels.

Also on the VAX, `fwdaddr()` in c2 forwards `movl rA,rB` into a following
address use (`(rB)`, `d(rB)`, `[rB]`), which makes stdio's `getc` four
instructions instead of five, and `x = (char)((int)x & c)` is done in the narrow
type so c2 sees `bicb2 $-128,(r)`.  No VAX command source is larger than `cc -O` any more; the three libc files
that are are listed under "What is still open".

## Defects in the originals and in upstream pcc

Found by running the output of the new compiler on real systems.  Defects that
our own changes introduced and then fixed are not listed.  The old ones were
latent: nothing the original compilers produced reached them.

**In the original Unix code** (V7 and 32V `c2`, the assembly optimizers):

* The opcode hash table is searched with `if (ophp > &ophash[OPHS]) ophp = ophash;`
  after `*ophp++`, so a full probe sequence reads `ophash[OPHS]`, one past the
  table (V7 `c20.c` and 32V `c20.c`; the lookup routine in the same file uses
  `>=`).  Harmless while the table has free slots near the end.
* 32V `c2` took r0-r5 to be dead at every label and branch.  That was a fact
  about the 32V `ccom`, whose temporaries never outlived a statement, not a
  property of the language; any code generator that keeps a value in one of them
  around a loop gets live registers overwritten.
* 32V `c2`'s fetch elimination (`movl rA,rB` followed by a use of `rB`) replaces
  `rB` by `rA` in the next instruction even when that instruction modifies the
  operand (`incl`, `decl`, `sob`, `aob`, `acb`), which changes `rA`; and the
  three-operand conversion that follows tests whether the second operand equals
  the copy's source instead of whether it was the eliminated register, so
  `mull2 r0,r0` after `movl r0,r1; mull3 r0,r1,r0` became `mull3 r0,r0,r1`.
* After an `.lcomm`, 32V `c2` goes on with a stale label number for the next
  label it makes (6 of the 91 libc files differ from the original's output once
  the port avoids it).

**In dmr-cc itself** (found by running its output, `tools/dmrtest.sh`, against pcc's):

* Unsigned division and remainder by a variable are compiled as `clr r0; div b,r0`.
  The PDP-11 `div` takes a signed divisor and needs a quotient that fits a signed
  word, so the result is wrong for a divisor of 32768 or more and for a divisor of
  1 with a dividend of 32768 or more: `60000u / 40000u` is not 1, and neither is
  `65535u / 65535u`.  `tools/dmrbugs/udiv-variable.c` returns the failing cases
  (87 under dmr-cc, 0 under pcc).  pcc's `udiv` and `urem` are right for every divisor
  and cost a few bytes per file that uses them (see BIGGER-THAN-ORIGINAL.md); that
  is the intended trade, since size is not worth a wrong answer.

* A `long` made from an `unsigned` is sign-extended: `(long)(unsigned)65535` is `-1L`, because dmr-cc
  emits `sxt` for the high word whatever the source's signedness.  `tools/dmrbugs/unsigned-to-long.c`
  (2 of 2 cases wrong under dmr-cc, right under pcc).  pcc emits `clr`.
* A `long` constant with bit 15 of its value set has a high word that is one too small: `40000L` is
  `-25536L` (high word -1), `99999L` and `100000L` have high word 0 instead of 1, and `65536L`, `70000L`,
  `131072L` and `1000000L` are right.  `tools/dmrbugs/long-constant.c` compares each against the same number
  built by additions (12 of 15 under dmr-cc, 15 under pcc).  Any V7 program whose constants fall in that
  range was compiled wrongly.
* `getlogin` and `malloc` use `sizeof` in ways that differ in signedness between dmr-cc's compile and ours;
  that is the `sizeof` fix under "upstream pcc" and not a dmr defect, but it is why those two files were not
  byte-identical.

**In 32V's cc** (Johnson's PCC with the 32V tables, run with `tools/vax32v.py --guestcc`):

* Unsigned `/` and `%` are compiled as the signed `divl3` (`table.c` groups `TUNSIGNED`
  and `TULONG` with `TINT` for `DIV` and `MOD`), so a value with the top bit set is
  divided as a negative number: `4000000000u / 7u` and `4294967295u / 3000000000u` are
  wrong.  `tools/dmrbugs/udiv-variable-vax.c` returns the failing cases (15 under 32V's
  cc, 0 under pcc).  pcc's `__udiv` and `__urem` are right for every value and cost a
  few dozen bytes per file that divides (`ed` pays 41).

**Found while adding long long, and already in the pushed commit (`c07dcdb4`)** — each reproduced on a
build of that commit with the test named, and fixed since.  These are miscompiles of ordinary programs, not of
long long: the helpers and the 64-bit tests use constructs the earlier tests never did.

PDP-11 (`arch/pdp11/table.c`, `local.c`, `local2.c`):

* **A long loaded through a pointer into a register pair that holds the pointer** was
  `mov 4(r0),r0; mov 6(r0),r1`, which reads the second word through the register the first just overwrote.
  Two templates (the `UMUL` and the `ASSIGN` of a long from an `OREG`) printed the words in a fixed order; a
  comment on the first said "XXX - avoid OREG index register to be overwritten".  `ZY` and `Zy` print the word
  that does not overwrite the base first.  Old compiler: `return a->lo` as a long, with `-O`, loaded garbage
  (`pcctests/longload.c`: did not halt).
* **A branch on the result of a long `&`, `|` or `^`** used the condition codes of the last of the two
  instructions (`bic AR,AL; bic UR,UL`), which describe only the low word: the templates said they set them
  (`RESCC`, `FORCC`).  `if ((x & 0x80000000L) == 0)` branched on `clr r1`.  The codes claim is gone; the compare
  is done on both words (`pcctests/longlogic.c`: 1244).  Costs 2 bytes at each of two places in `access.c` of the commands
  (an extra `tst r1`), where the old output was right only because the high word of the mask was zero.
* **`a = b = c = 0` for long variables**: the template that clears a long in memory claimed its result was in a
  register (`INBREG`), so the outer assignment stored an uninitialised `r0:r1` (`pcctests/longchain.c`: 1180).
* **`unsigned long >> n` shifted the sign in**: `RS` becomes `LS` by a negative count for every type but
  `unsigned` and `unsigned char`, and `ashc` is arithmetic (`pcctests/ulshr.c`).  Now a call to a helper that
  clears the top bits.
* **`-x` for an unsigned long** was "Cannot generate code" (the negate template listed `TLONG` only).
* **A long left shift printed the wrong register pair** (found earlier in the session, `monitor()`).

VAX (`arch/vax/table.c`, `local2.c`, `local.c`):

* **A comparison of a computed long long was dropped**: the quad `PLUS` and `MINUS` templates accepted a `FORCC`
  cookie without setting the codes, so `x + y != 0` was compiled without the compare (`pcctests/llcmp.c`:
  2276 on the old compiler).
* **A quad immediate had a zero high longword on 32V**: `movq $-100000,...` is read by 32V's `as` as a
  longword, zero-extended.  `-100000LL` as an argument was 4294867296.  A long long constant outside 0..2^31 is
  now a memory operand.
* **The low half of a quad constant operand was printed as the whole constant** (`cmpl r0,$0x11facd96d7c`).
* **A static long long was written with `.quad`**, which 32V's `as` does not have: any file with a
  `long long` initializer failed to assemble.  `.long lo,hi` now (`pcctests/llconst.c`: BUILDFAIL).
* **No long long multiply, divide, remainder or logical shift right**: undefined symbols at link time.

x86 (i386, amd64): the new tests pass on both; `-xssa` makes the generated code for the whole of
`x86tests/llfull.c` crash (each statement alone is fine); `-xssa` stays off, the test is skipped under it.

**In upstream pcc** (PortableCC/pcc, the PDP-11 and VAX backends and the shared
code they rely on):

* `a[f()] += v` called `f` twice (and `p->m[f()]++`, any lvalue containing a call):
  `has_se()` in `trees.c` did not count a call as a side effect, so the lvalue was
  copied instead of kept in a temporary.  Found by a test written for something else
  (`pcctests/opeqaddr.c`, fails without `-xdmr` on the old compiler).
* `sizeof` was typed `int` on the PDP-11 (`SIZET` is `INTPTR`, which is `INT` on a
  16-bit target), so `(long)(n * sizeof x)` sign-extended a product over 32767.
  dmr-cc compiles the same expression with `clr` for the high word (getlogin.c).
  `arch/pdp11/macdefs.h` now sets `SIZET` to `UNSIGNED`. Test: `pcctests/sizeofuns.c`.
  With it, malloc's `(nw + BLOCK/WORD) / (BLOCK/WORD)` is an unsigned divide (a shift
  pair) where dmr emits a signed `sxt; div`.
* `struct symtab` in `pass1.h` gained a field in the middle and broke the
  positional initializer of `nulsym` in `pftn.c`; gcc accepts that, pcc compiling
  itself does not. New fields go last (`tools/pccboot.sh` is what catches it).

* `cgram.y` rewrites `a > b` as `b < a` and `a <= b` as `b >= a`.  The PDP-11
  template `cmp AL,AR` assumes the operands are where the source put them, so a
  constant landed on the left, the `tst` form was lost and the branch came out
  inverted for `>` and `<=`.
* `mip/optim2.c` (`-xdeljumps`) inverts a relation by swapping the operands, which
  also puts a constant on the left, a form the backends do not accept
  (`cmpl %eax,$1`).
* `arch/pdp11/local.c` retypes a `DIV` or `MOD` under an int/unsigned conversion
  to the outer type, which changes the signedness of the division.
* `register unsigned x; x >>= n` crashed the compiler: `findmops` in
  `mip/match.c` picked a rule that wants its operand in a fixed register for a
  variable pinned to another.
* `**p++ = 0` crashed the PDP-11 backend: the operand left to `myormake` already
  had its own increment instruction, which was then lost.
* Unsigned `/` and `%` call helpers (`udiv`, `urem`, `uldiv`, `ulrem`) that V7's
  libc, and 32V's, do not have, so such programs fail to link.
* `-xssa` on the PDP-11 miscompiles V7's `getnum` (not traced to its cause; SSA
  stays off).
* The PDP-11 `LS` template for a `long` printed `ashc AR,A1`, the register the allocator chose for the
  result, although the value sits in the pinned pair `r0:r1` (`NLEFT(R01), NRES(R01)`).  When the result was
  coloured `r2:r3`, the instruction shifted whatever was in that pair and the result was wrong.  libc's
  `monitor()` computes `((long)bufsiz << 15) / o`; its `profil` scale came out 0.  The template prints `r0`.
* `temparg()` in `mip/regs.c` (a spilled parameter keeps its own slot) assumed the argument loads follow the
  first label after the prologue; on the VAX they follow a second one, so it never found them.
* `ccom -xc2` swallowed a c2 failure (47).
* The VAX backend's floating-constant symbol was never initialised (a
  `#define sap sss` compatibility macro hid it), so the compiler looped forever
  on a program with a `double` constant under `-xdeljumps`.

## Tried and dropped

Optimizations that were written, measured, and not kept, and the reason.  The rule that decided each one
is the user's: a root fix beats a workaround, and a net win beats copying dmr-cc.

1. **A fold of `t = E; X = t` on temporaries in the register allocator (`dce()` in `mip/regs.c`).**  Never
   fired: PDP-11 register variables are `REG` nodes, not temporaries.  Replaced by `retfold()` in
   `optim2.c` on `REG` nodes (kept, 32).
2. **`OR` leaves its result in the right operand's register, as a table rule.**  First attempt made `x | 040`
   into `mov $40,r1; bis r0,r1` (commands +6), because `chcheck()` ranked an operand that is already in the
   result register (cost 3) like one that must be loaded.  A special shape "a register, not a constant"
   (`SNOCON`) cannot work (special shapes return only `SRDIR`: `Cannot generate code`).  An interim IR
   operand swap (`orswap`) was a workaround and was removed.  The root fix, `findops()` ranks an operand in
   place one cheaper, made the table rule work: i386 -13, amd64 -24, pdp11 commands -38, vax 0.
3. **`parmprop`: every two-use parameter in place.**  libc +14, commands +6; the per-use cost model (39) is
   neutral to -2.  "Always in place when `r2`, `r3`, `r4` are all register variables" (dmr-cc's rule, which
   never copies a parameter): costs 28 bytes in 6 files and saves 8 in 3 (net +20).  Dropped; the net win was chosen over
   dmr-cc's rule.
4. **A `POSTSPILL` hook (rerun `narrowload` after the allocator rewrites a spilled temporary).**  Never
   reached for `flsbuf`'s parameter, whose uses are short-lived reloads (`treerewrite`), not `leafrewrite`.
   Removed.
5. **VAX `AUTOINC_MEM`: `x++` in memory as `(x += 1) - 1`.**  Added to hide a dead copy of the old value;
   removed once `ecomp()`'s statement fold looked past the `COMOP(0, ...)` placeholder (libc 19056 to 19040).
   Also tried: pointers excluded; pointers through another pointer only.  Both were workarounds for the
   duplicated lvalue and were replaced by `hoistbase()` (36).
6. **Hoisting the whole lvalue address of `x op= e` / `x++`.**  PDP-11 `gb->n++` became
   `mov $4,r0; add _gb,r0; inc (r0)` (commands +88).  Replaced by hoisting only the pointer; the offset stays
   in the addressing mode.
7. **A VAX template for a narrow `UMUL` widened onto the stack (`cvtbl *p,-(sp)`).**  Templates cannot make the
   `UMUL` child an `OREG`: `shswitch` calls `offstar` on the matched node only.  `SCVTARG` (50) is kept for
   `NAME` and `OREG`; c2 does the rest.
8. **VAX narrowing `(char)x & mask` in `clocal` with a retyped constant.**  The `bitb` rules want a word
   constant (`Cannot generate code`).  Replaced by `BITTEST`, a tagged compare node (49), as on the PDP-11.
9. **VAX c2: `repladdr` guard alone for `addl3 $1,X,r; movl r,X`.**  Commands +16 when the fusion cannot
   happen because `ret` was assumed to keep `r1` live.  Replaced by exact liveness (`c2_quadret`, 51; 45).
10. **VAX: turning indexed addressing off for `long long`.**  The user rejected a disable as a fix; replaced
    by `movaq` (scale 8) and halves (48).
11. **`longmem`: every long in memory.**  libc -38, commands +316.  The narrow rule is kept.
12. **Expectations I got wrong in tests** (not compiler changes): `*p++ + *p++` (undefined; dmr-cc, pcc and
    `pcc -O` all differ), `long long` multiply on 32V (no helper), 16-bit-only constants in VAX tests.
13. **`SPILL_TO_OPERAND` (PDP-11, `mip/regs.c` `RewriteProgram`): treat a spilled short-lived temporary like a
    long-lived one, as a memory operand, instead of `treerewrite`'s load, operate, store.**  It was meant to make
    `realloc`'s `*t++ = *s++` and the long temporaries of `fseek` the in-place code dmr-cc writes.  The compiler
    then rejected files (16 suite failures, libc 83 files compiled instead of 89).  The cause was first guessed
    to be that a register cannot be synthesised after the fact; that was wrong.  What the crash was: the
    sharing of spill slots indexes its arrays by temporary number, and a short-lived temporary (`REGW` outside
    `nblock`, ids above `tempmax`) is outside the arrays.  Later the mechanism's purpose was reached another way
    (`SSTARINC` for the long-lived case, 54; `spillpush`, 53); the short-lived case remains.
14. **`SSTARINC` covers a pointer the allocator spills as a long-lived temporary** (the case it can see); the
    short-lived case is the part that item 13 would have covered.
15. **The `tst`-after-`mov` rule in c2's main loop only.**  It deleted the `tst` but the fetch elimination, which runs
    once before the loop, had already looked at the sequence with the `tst` in it, so `movb (r10)+,r0;
    movb r0,(r11)+` never collapsed.  Moved before the fetch elimination (43).  It still did not collapse:
    liveness read the byte write into `r0` as a read of `r0` (42).  The root was in the liveness, not in the rule.

16. **The result of the PDP-11 long shift copied by the template** (`ZS`: shift in `r0:r1`, then `mov` to the
    pair the allocator chose, no `NRES(R01)`).  Tried while chasing a missing copy; `libc/sys/stat.c` +4
    bytes because the allocator could no longer see that the result was in `r0:r1`.  The original form
    (`NRES(R01)`, the template prints `r0`) is back.
17. **Unsigned long `>>` as a tree** (`(long)x >> n` and a mask, or the mask built from a shifted constant) in
    `clocal()`.  With the mask applied after the shift the allocator lost the copy of a result that
    `NRES(R01)` pins, in one shape of expression (`(b << 1) | (a >> 31)`); I did not find why.  A call to a helper
    (`_llulr`) is correct in every shape and costs nothing in libc or the commands (16838, same).  This is a
    workaround for an allocator fault I did not trace: the fault is still there for any other tree that
    ends in a pinned-pair template under another instruction.
18. **A long long held in registers** (the `LL0` register class that `macdefs.h` already names, four floating
    registers): there are no registers to hold one, and the class is not wired to anything.  Memory
    objects and helper calls instead (59).

## The scoreboard and the old K&R sources

`tools/pccdiff.sh` compiles `sh/*.c` with `-Dvoid=Void_` (the sources use
`struct void`) and everything with `-Dunix` (V7's `cc` defined it), skips the files that are only
`#include`d, and reports each rejected source under the kind of its first error.  The C dialect pcc
accepts was deliberately **not** changed: of the 80 V7 command sources, 28 are rejected (20 use members of
another struct's namespace, 5 old declaration forms, 3 implicit externs), and 18 of the 605 32V sources.
`OLD-K&R-FORMS.md` describes each form, where the sources use it, what accepting it would take and what it
would break for valid C99.

## What is still open

**PDP-11:** nothing that correctness allows.  `qsort` (+12) and `sh/print` (+54) carry `udiv`/`urem`.
The remaining small differences of an earlier round (`mon`, `fseek`, `malloc`, `flsbuf`) were all taken apart;
see `BIGGER-THAN-ORIGINAL.md` and the ledger above.

**VAX:** `mktemp` (+92) only: `unsigned % 10` calls `__urem`, which 32V's cc replaces by a signed `divl3`.
Correctness.

**long long:** done on both targets (59, 60).  What is not: `long long` <-> `float`/`double` conversion on the PDP-11,
`switch` on a long long on the PDP-11 (both are diagnostics), and on the VAX nothing known.  The first is
a matter of writing the helpers (the PDP-11 FPU converts at most a long).

**x86:** `testl` after `decl`/`addl`, `leal` for `&a[i]`, the dead mask fold for every target (22 above).

**DCE** (`-xdce`: pass 1 dead-arm discard and constant folding exist; constant `switch`, static function and
data removal, lazy string emission and the `#ifdef`-equivalence corpus test are deferred).

**The old forms:** a decision whether to accept any of them (`OLD-K&R-FORMS.md`).

**Ship** the VAX `c2` with the NetBSD VAX dialect (it parses the 32V assembler dialect only).
