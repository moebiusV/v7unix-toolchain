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
| PDP-11 | libc, 89 files | dmr 17356 | 16918 | -2.5% |
| PDP-11 | `cmd/`, 51 files | dmr 90178 | 86566 | -4.0% |
| VAX (32V) | libc gen+stdio, 91 files | `cc -O` 21976 | 19056 | -13.3% |
| VAX (32V) | 56 command sources | `cc -O` 118444 | 109980 | -7.1% |
| i386 | libc gen+stdio (91 files) | n/a | 21382 (27839 with no flags) | -23% |
| amd64 | libc gen+stdio (91 files) | n/a | 24771 (39919 with no flags) | -38% |

All numbers are text+data bytes, assembled with the target's own assembler
(`tools/pccdiff.sh`, `tools/vaxdiff.py`, `tools/x86size.sh`). Flags used:
`-xdmr -xtemps -xdeljumps -xdce -xinline`, plus the target's `c2` where there is one
(`-xc2`, run inside ccom). `-xssa` is a net loss on both PDP-11 and VAX and stays off.
The x86 "no flags" column is the upstream default; most of that difference is
`-xtemps` and friends, not `-xdmr`, which is worth 1126 bytes of the 22508 on i386.

PDP-11 files still bigger than dmr's (6 of 89 in libc, 2 of 51 in `cmd/`, 8 in all);
every one is explained in `BIGGER-THAN-ORIGINAL.md`, and the two big ones
(`qsort`, `sh/print`) are the correct unsigned-divide helpers. VAX: 11 of the 56
command files are bigger than `cc -O`; not yet examined function by function
(`tools/vaxfunsize.py` is the tool for that).

Correctness gates, all passing: 212 bare-metal PDP-11 test runs (`tools/pcctest-all.sh`,
four flag sets), 144 VAX runs on real 32V under SIMH (`tools/vaxtest-all.sh`, four
flag sets, one with c2 and one with `-xssa`), 110 i386 and 100 amd64 runs, pcc
compiling itself with identical stage 1 and 2 (`tools/pccboot.sh`; note it builds
the committed `HEAD`, not the working tree), and 34 V7 commands, 72 tests, built by
`pcc -O` and run inside real V7 next to V7's own cc builds (`tools/v7progs.py`).

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
| 17 | Switch via jump table | inline compare chain, sorted | `casel` | **done** (pdp11 `mygenswitch`); vax open | exists (generic) | needs `mygenswitch` per target |
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

**VAX.** Not done: `casel` needs the table in the instruction stream, and the selector in a
register, which the IR cannot express without an `XASM` or a computed goto. 32V's cc
does not use `casel` for sparse cases either (it builds a decision tree, which is why
`dump.c` is larger here).

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
`or` in r3 (the left operand) and moved it: `bis r0,r3; mov r3,r0`. **pcc.**
`retfold()` in `mip/optim2.c` (`-xdmr`, all targets) does two things. (a) `orswap`
puts the computed operand of an `OR` first when the other is a register variable and
the computed one has no side effects, except for an in-place update `v = v | E`
(there `v` stays first; swapping it cost 2 bytes in `cp.c`). (b) `r = E; ret = r`
directly before the return, where `r` is a register, becomes `ret = E`: it needs
`toepilog()`, "the next statement jumps straight to the epilogue" (a goto to a label
followed only by labels and `EPILOG`, or no statement at all), because then nothing but
the result is live. That version works on `REG` nodes; a register variable on the
PDP-11 is a `REG` in the IR, not a `TEMP`, so a pass in `regs.c` written for `TEMP`s
never fired. **Why not a table rule:** see "What is still open" (no matcher shape for a
register that is not a constant).

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

## c2 inside ccom, and what running it found

`ccom -xc2` buffers its assembly and runs c2 (`cc2/cc2.<mach>.c`, built into ccom
as `c2.o` with `main` renamed `c2main`) on it, in a forked copy of itself with the
text passed through temporary files; if anything fails the unoptimized text is
written instead.  The driver passes `-xc2` on the PDP-11 and 32V targets instead
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
type so c2 sees `bicb2 $-128,(r)`.  The remaining 6 command sources that are
larger than `cc -O` (deroff +184, cron +52, dd +44, chmod +32, ...) are byte
stores through `cvtbl`/`movb` pairs where 32V used `cvtlb`.

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

**In 32V's cc** (Johnson's PCC with the 32V tables, run with `tools/vax32v.py --guestcc`):

* Unsigned `/` and `%` are compiled as the signed `divl3` (`table.c` groups `TUNSIGNED`
  and `TULONG` with `TINT` for `DIV` and `MOD`), so a value with the top bit set is
  divided as a negative number: `4000000000u / 7u` and `4294967295u / 3000000000u` are
  wrong.  `tools/dmrbugs/udiv-variable-vax.c` returns the failing cases (15 under 32V's
  cc, 0 under pcc).  pcc's `__udiv` and `__urem` are right for every value and cost a
  few dozen bytes per file that divides (`ed` pays 41).

**In upstream pcc** (PortableCC/pcc, the PDP-11 and VAX backends and the shared
code they rely on):

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
* The VAX backend's floating-constant symbol was never initialised (a
  `#define sap sss` compatibility macro hid it), so the compiler looped forever
  on a program with a `double` constant under `-xdeljumps`.

## The scoreboard and the old K&R sources

`tools/pccdiff.sh` compiles `sh/*.c` with `-Dvoid=Void_` (the sources use
`struct void`), skips the files that are only `#include`d, and reports each
rejected source under the kind of its first error.  The C dialect pcc accepts was
deliberately **not** changed: 29 of the command sources are rejected because they
use members of another struct's namespace (20), implicit externs (6) or old
declaration forms (3).

## What is still open

* **PDP-11, the last bytes** (details in `BIGGER-THAN-ORIGINAL.md`): the allocator copies a
  parameter used twice into a register where dmr uses it in place (`fseek`, `mon`,
  `sh/main`; using it in place at two uses was tried and made libc 14 bytes bigger);
  an address-taken char temporary is stored through a register after template
  selection (`flsbuf`); dmr's `clr -(sp)` for a zero long word is done by a c2
  peephole, not a template.
* **A matcher-level "register, but not a constant" shape.** A commutative rule that
  leaves the result in the right operand's register (`bis AL,AR`) would do what
  `retfold`'s operand swap does, but `match()` ranks a rewrite no worse than an exact
  match, so such a rule wins even for `x | 040` and costs a `mov $40,r1`. A shape bit
  that excludes `SCON` while still allowing the operand to be put in a register
  would make it a table rule for every target.
* **VAX:** 11 of the 56 command files are bigger than `cc -O` (`dump` +240, `deroff`
  +148, `grep` +116, `ed` +60, ...). `tools/vaxfunsize.py` points at the functions
  (`dump`: `mark`, `add`; `grep`: `main`, `advance`; `col`: `emit`). Known causes:
  32V's decision-tree switch, our `extzv` for `>> 4`, `addl3` where 32V has `incl`,
  `casel`. The c2 counters (`tools/c2stats.sh vax`) say 2640 redundant moves remain
  for c2 to remove over 130 files: the next IR pass to write is a block-local copy
  forwarder over `TEMP`s.
* x86: `testl` after `decl`/`addl`, `leal` for `&a[i]`, the dead mask fold for every
  target (22 above), `jumpsw`-like layout not worth doing.
* DCE (`-xdce`, pass 1 dead-arm discard and constant folding exist; constant `switch`,
  static function and data removal, lazy string emission and the `#ifdef`-equivalence
  corpus test are deferred until dmr parity is finished).
* Ship the VAX `c2` with the NetBSD VAX dialect (it parses the 32V assembler
  dialect only).
