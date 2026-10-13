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

## Status of this work (2026-10-14)

All dmr/32V-inspired transformations are opt-in with `-xdmr` (`pcc -O` implies it on
the PDP-11). Everything else is unchanged.

| Target | Corpus | original cc | ours | |
|---|---|---|---|---|
| PDP-11 | libc, 89 files | dmr 17356 | 17234 | -0.7% |
| PDP-11 | `cmd/`, 39 files | dmr 82808 | 80902 | -2.3% |
| VAX (32V) | libc gen+stdio, 91 files | `cc -O` 21976 | 19172 | -12.7% |
| VAX (32V) | 39 command sources (30 no bigger) | `cc -O` 77760 | 70844 | -8.9% |
| i386 | libc | n/a | 20763 -> 20142 with `-xdmr` | -3.0% |
| amd64 | libc | n/a | 25227 -> 23524 with `-xdmr` | -6.8% |

All numbers are text+data bytes, assembled with the target's own assembler
(`tools/pccdiff.sh`, `tools/vaxdiff.py`, `tools/x86size.sh`). Flags used:
`-xdmr -xtemps -xdeljumps -xdce -xinline`, plus the target's `c2` where there is one.
`-xssa` is a net loss on both PDP-11 and VAX and stays off.

Correctness gates, all passing: 124 bare-metal PDP-11 programs (`tools/pcctest-all.sh`),
26 programs on real 32V under SIMH (`tools/vaxtest-all.sh`), the i386 and amd64
suites, pcc compiling itself with identical stage 1 and 2 (`tools/pccboot.sh`),
and 34 V7 commands, 72 tests, built by `pcc -O` and run inside real V7 next to
V7's own cc builds (`tools/v7progs.py`).

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
| 11 | Loop rotation (test at the bottom) | c2 | c2 | pdp11/vax via c2 | **done** (`common/x86peep.h`) | yes (needs a pass) |
| 12 | Branch reversal over `jbr`, jump chains, cross-jumping | c2 | c2 (+ `jumpsw`) | pdp11/vax via c2; `-xdeljumps` | partly | yes |
| 13 | Redundant load / store / test elimination (register tracking) | c2 `rmove` | c2 `rmove` | via c2 | **partly** (`testl` after and/or/xor; Z/S branches after add/sub/inc/dec) | yes (needs a pass) |
| 14 | Send a result straight to its destination (`bflow`) | c2 (2-address) | c2 (3-address) | via c2 | no (2-address ISA) | risc: no |
| 15 | Address constants: `movab`/`pushab`/`moval` | - | c2 | done (vax) | **done** on amd64 (`leaq sym(%rip)`, `movl $c`); `leal` for `&a[i]` open | m68k `lea` |
| 16 | Counted-loop instructions | `sob` | `sobgeq` `aoblss` `acbl` | via c2 | `loop` is slow: no | ppc `bdnz`, arm64 none |
| 17 | Switch via jump table | - | `casel` | no | exists (generic) | exists |
| 18 | Callee-saved registers in an entry mask | csv | `.word mask` | done (vax) | push/pop | no |
| 19 | Floating immediates | first word of a double | - | done (pdp11) | no | no |
| 20 | `x++;` without a copy of the old value | c1 | pcc | done (`-xdmr`) | open | generic |
| 21 | `*(&x + k)` is the name `x+k` | c1 | pcc | done (pdp11, vax) | done (generic) | generic |
| 22 | Dead mask test (`uchar & 0x100`) | c1 | c2 `bitopt` | open | open | front end |
| 23 | Register pressure: spill choice, pinned regs | c1 | pcc | partial | allocator | allocator |

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
switch to the unsigned relation. VAX (and i386/amd64) handle it with the same
code shape as the PDP-11's.

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

**pcc.** PDP-11 and VAX get it from `c2`. **x86 does not have a pass yet.** The
simplest implementation is a text pass like `frameless()`: find
`L1: cmp; jcc L2; body...; jmp L1; L2:` and rewrite. A cleaner one: do it in the
IR in `mip` before instruction selection (`optim2.c`) for every target.

## 12. Jump cleanup

**What.** Jump to a jump; branch around an unconditional jump reversed;
cross-jumping (two paths ending in the same sequence share it); code motion;
removing unreachable code.

**c2.** `comjump`, `xjump`, `codemove`, `redunbr`, `iterate`, `jumpsw`. `jumpsw`
(32V) additionally puts the *farther* of a conditional/unconditional pair on the
unconditional jump, because it has the larger range.

**pcc.** `-xdeljumps` removes jumps to the next label; the rest is `c2`. `jumpsw`
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

**Port.** x86 `leal sym,%eax` is the same size as `movl $sym,%eax`: no gain
unless an index is involved (see 3). m68k `pea`/`lea`.

## 16. Counted-loop instructions

**What.** `decl n; jgeq L` is `sobgeq n,L`; `incl i; cmpl i,lim; jlss L` is
`aoblss lim,i,L` (VAX); PDP-11 `sob r,L`.

**c2.** `addsob()` after all other passes (the displacement must be within the
instruction's byte range). Needs `incl/decl` to be there first, so
the `addX2 $1` -> `incX` rewrite matters.

**Port.** x86 `loop` is slower than `dec; jnz`: skip. PowerPC `bdnz`, SuperH `dt`,
VAX/PDP-11 only really.

## 17. Switch via jump table

**32V.** `casel`. **pcc.** The generic switch lowering in pass 1 already emits
jump tables where the target supports them; VAX does not use `casel` (not
needed for size parity).

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

## What is still open

* x86: loop rotation, `testl` after `decl`/`addl`, `leal` for `&a[i]`, the dead
  mask fold for every target, `jumpsw`-like layout not worth doing.
* VAX: `deroff` (+320 bytes over `cc -O`), `cvtlb` stores (we emit `movb`),
  `bicl2/bisl2` forms, `movq` pairs; command sources where we are still larger.
* Ship the PDP-11 and VAX `c2` as an in-process pass instead of a separate
  executable, and ship both with the NetBSD VAX dialect (the VAX `c2` parses the
  32V assembler dialect only).
