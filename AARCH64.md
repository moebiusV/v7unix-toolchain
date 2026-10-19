# The aarch64 backend

`arch/aarch64` in pcc is a new backend for 64-bit ARM (A64, LP64, little-endian).  It replaces
the file set that was there before, which was a 32-bit ARM backend with its registers renamed:
none of the 48 programs of the LP64 test corpus ran, the simplest functions crashed the
compiler, and a store to a global used `x0` as a scratch register while `x0` held a live
variable.  This one is modelled on the amd64 backend (same table shapes, same hooks) and is
checked on an emulated AArch64; `-xdmr` adds a text optimizer, `arch/aarch64/peep.h`, which
carries the dmr-cc ideas that make sense on this machine over to it.

## What works, what does not

Works: `char`/`short`/`int`/`long`/pointers, `float`/`double`, structs and unions (by value as
arguments and results, assigned, nested), arrays, bit-fields, `switch` (as compare chains),
function pointers, static data of every kind, large frames, large offsets and constants,
calls with any number of arguments, `<stdarg.h>` (`va_list`, `va_arg`, `va_copy`).  All of the LP64
corpus (`tools/a64tests`, 67 programs, five flag sets) passes, and so do the 127 programs of `x86tests` and `pcctests` that are valid
on an LP64 machine.  The calling convention is checked against clang: `tools/arm64abi.sh` compiles one file with pcc
and one with clang, lets each call the other's functions (structs of every size and class by value, as results,
around other arguments, with the registers running out, and through `...`), and runs them on the emulator.  The V7 libc and 29 of the commands compile and assemble.

Not done, and diagnosed rather than miscompiled where the front end can tell:

* `alloca` and variable-length arrays: `spalloc()` gives an error message.
* `va_arg` of a struct whose members are all floating-point (a diagnostic is given).
* Apple Silicon (`./configure --target=aarch64-apple-darwin`): the output is Mach-O assembler (underscore names, `L` labels,
  `.const_data`/`.cstring`, `@PAGE`/`@PAGEOFF`) and the 67 test programs assemble with `clang --target=arm64-apple-macos`, with and
  without `-xdmr`; nothing was run; variables defined elsewhere are reached through the GOT,
  functions are called directly (the linker makes stubs), and their addresses are taken directly.  (The preprocessor's own Mach-O branch does not compile; build `cc/ccom` alone.)
* `long double` is `double` (`ctype()` maps it), `long long` is `long`.  A real 128-bit `long double` needs a quad-precision
  value that lives in a `q` register or in memory behind library calls (`__addtf3` and the rest); PR #47 declares one (16 bytes,
  16-byte aligned) but computes it as a 64-bit `double` in `d` registers, and its `long double` constants come out as `ERR`, so there is
  nothing to take from it.
* jump tables for `switch` (`mygenswitch` returns 0, so the generic compare chain is used).
* `-xssa`: `structcopy` and the varargs tests fail with it, so `pcc -O` leaves it off for this target (it is on for the other Linux targets).  Asking for it explicitly is unsupported.

Structs follow AAPCS64 (since 18 October 2026): up to 16 bytes in one or two `x` registers, one to four
`float`s or `double`s (also inside nested structs and arrays) in as many `v` registers, anything larger copied by the
caller and passed by address, a result of up to 16 bytes back in `x0`/`x1` (or `v0`..`v3`), a larger result written
through the address in `x8`.  A struct that does not fit the registers left goes to the stack and takes the remaining
registers with it, as the standard says.

Where it still differs from the standard:

* a register pair is not aligned to an even register for a 16-byte-aligned type (no such type exists here);
* `long double` is `double`, so `long double` arguments and the 128-bit floating-point aggregates the standard
  describes are not what this compiler passes;
* plain `char` is signed (Apple's arm64 does the same; AAPCS64 proper has it unsigned).

## Registers and the frame

| register | use |
|---|---|
| `x0`-`x7` | integer and pointer arguments, `x0` the result |
| `x8` | struct result address |
| `x9`-`x15` | scratch, caller-saved, allocated |
| `x16`, `x17` | scratch of the code the table prints (large offsets, symbol addresses, constants): never allocated |
| `x19`-`x28` | callee-saved, allocated |
| `x29` | frame pointer |
| `x30` | link register |
| `d0`-`d7` | floating arguments, `d0` the result |
| `d8`-`d15` | callee-saved (low 64 bits), allocated |
| `d16`-`d30` | scratch, allocated |
| `d31` | floating scratch, never allocated |

pcc's register allocator keeps its colours in an `int` mask, so a register class may have at most
31 members (`mkext` says "too many regs in a class" and the build then leaves a stale
`external.c`: always read the output of `./mkext`).  That is why class B stops at `d30`.

```
        high addresses
        |  caller's frame / stack arguments   |
        +-------------------------------------+  <- entry sp
        |  callee-saved registers, if any     |
        |  automatic variables and spills     |
        +-------------------------------------+
        |  frame record: x29, x30             |  <- x29
        +-------------------------------------+
        |  outgoing stack arguments           |  <- sp (constant after the prologue)
        low addresses
```

`x29` points at the bottom of the locals, so automatic variables have small positive offsets
(`AUTOINIT` is 16 bytes, the frame record).  Outgoing arguments are written at `[sp,#n]`, and
the size of that area is found by looking at the trees (`offcalc` in `local2.c`).  A parameter
that was passed on the stack is the one thing whose distance from `x29` depends on the frame
size, which is only known after the function has been selected: it is given the offset
`PARAMMARK + n` and `fpoff()` turns that into the real one where the address is printed.

Prologue: `stp x29,x30,[sp,#-N]!` and `mov x29,sp` when there are no outgoing arguments and the
frame is below 512 bytes, otherwise `sub sp,sp,#N` and a store of the pair above the argument
area; the callee-saved registers are stored in pairs.  A function that makes no calls, has no
frame references and uses no callee-saved register has no prologue at all.

## How the table prints

`A1` and `AL` are registers named by the width of the node's type (`w0` for `int`, `x0` for
`long` and pointers, `s0` for `float`, `d0` for `double`); `I1`, `IL` are always the 32-bit
names and `U1`, `UL`, `UR` the 64-bit ones.  Every template that reads or writes memory starts
with `ZP`, which loads `x16` with an address or offset that does not fit the instruction, and
`adrput()` prints the operand as `[x16]` or `[xN,x16]` in that case.  Symbols are reached by
`adrp x16,sym` and `[x16,:lo12:sym]`; the code model is the small, non-PIC one.

A value in a register is right only in the bits of its type.  A conversion to a smaller or
equal type does nothing; a conversion to a larger type extends from the smaller one
(`sxtb`, `uxtb`, `sxth`, `uxth`, `sxtw`, or a 32-bit `mov`, which clears the upper half).
Loads from memory extend as they load (`ldrsb`, `ldrh`, `ldrsw`...).

Constants are made by the shortest of `movz`, `movn`, `orr` with a bitmask immediate, or
`movz` and `movk`.  Immediates for `add`, `sub`, `cmp` and for the logical instructions are
special shapes (`SIMM12`, `SNIMM12`, `SLOGIMM`, `SLOGIMM64`, `SSH32`, `SSH64`); anything else
is loaded into a register first by the generic rewrite.

A floating compare must treat a NaN as unordered.  pass 1 turns `if (a < b) s` into a jump over
`s` on the opposite condition and marks that node `ATTR_FP_SWAPPED`; `cbgen()` uses it to
choose between `mi`/`ls`/`gt`/`ge` (the condition itself: false for a NaN) and
`pl`/`hi`/`le`/`lt` (its negation: true for a NaN).

Struct copies are inline: unrolled up to 128 bytes, a loop with a counter above that.

## `-xdmr`: the text optimizer

The assembly of the whole file is kept in memory (`bjobcode` opens a memory stream as
`stdout`) and `ejobcode` hands it to `a64_peep()` in `peep.h`, which parses the instructions
the compiler prints, finds the registers each one reads and writes, computes liveness over each
function's labels and branches, and applies, until nothing changes:

| rewrite | before | after |
|---|---|---|
| copy propagation, dead code | `mov x3,x2` ... `ldr w0,[x3]` | `ldr w0,[x2]` |
| post-index addressing | `mov x3,x2; add x2,x2,#1; ldrsb w3,[x3]` | `ldrsb w3,[x2],#1` |
| (and the access first, or at the old address) | `ldr w0,[x1]; add x1,x1,#4`; `add x1,x1,#1; ldrb w0,[x1,#-1]` | `ldr w0,[x1],#4`; `ldrb w0,[x1],#1` |
| compare with zero | `cmp w0,#0; b.ne L` | `cbnz w0,L` |
| sign test | `cmp w0,#0; b.lt L` | `tbnz w0,#31,L` |
| bit test | `and w1,w0,#8; cmp w1,#0; b.eq L` | `tbz w0,#3,L` (a mask of several bits: `tst`) |
| repeated compare | `cmp w0,#0; b.ne A; cmp w0,#0; b.ge B` | the second `cmp` is dropped |
| redundant extension | `ldrb w3,[x0]; uxtb w3,w3` | the `uxtb` is dropped |
| a byte tested after `sxtb` | `ldrb w3,[x0]; sxtb w3,w3; cmp w3,#0; b.ne L` | `ldrb w3,[x0]; cbnz w3,L` |
| index in the operand | `sxtw x0,w22; add x1,x23,x0` | `add x1,x23,w22,sxtw` (also `sub`, `cmp`, and `mov w,w` as `uxtw`) |
| scaled index | `lsl x1,x1,#2; add x2,x0,x1; ldr w3,[x2]` | `ldr w3,[x0,x1,lsl #2]` |
| a condition as a value | `b.lt L; movz w0,#1; b M; L: movz w0,#0; M:` | `cset w0,ge` |
| repeated `adrp` | `adrp x16,s` ... `adrp x16,s` | the second is dropped |

Rules it keeps: loads are never removed, merged or paired (a `volatile` object is read by a load
the compiler cannot tell from any other), stores stay in order, an instruction it does not
recognize is a barrier that reads and writes everything, and a line with a comment or a second
statement is left as it is.  It was run under AddressSanitizer and UBSan over the whole
V7 libc output (that found a one-byte overrun in the operand splitter).

`pcc -O` on aarch64 passes `-xdmr`.

### Measured effect

Text plus data of the V7 `libc` (89 files), assembled with clang's integrated assembler:

| flags | bytes |
|---|---|
| none | 42500 |
| `-xtemps -xdeljumps -xinline -xdce` | 27908 |
| ... with `-xdmr` (text optimizer) | 24332 |

(Before `.align` was fixed these were 47756, 32308 and 28756: the generic code prints `.align 8` for
an 8-byte object, which x86 reads as bytes and AArch64 as 2^8, so every floating constant was padded
to 256 bytes and every function to 16.  `HASP2ALIGN` in `macdefs.h` now makes it `.align 3`.  It was the
largest single reason pcc looked big next to clang.)

Against `clang -Os` pcc is larger.  On the 76 libc files clang can compile, `-xdmr` gives
16560 bytes against 15739 for clang (+5%); on the 9 command sources both can compile, 11440
against 11125 (+3%).  (`tools/a64size.sh`, `A64REF=1`; pass `A64REFFLAGS="-fno-asynchronous-unwind-tables
-fno-unwind-tables"` or clang's `.eh_frame`, which `size` counts and pcc has no equivalent of, makes
clang look 35% bigger: an earlier version of this document compared against that number and
wrongly claimed pcc was 11% smaller.)  What `-xdmr` shows is the gain over pcc's own output.

## Testing

* `tools/arm64run.py` assembles the compiler's output (directives are laid out by the script,
  every instruction goes through keystone) and runs `test()` in unicorn, no libc and no kernel.
  `tools/arm64test.sh` is the one-file driver and `tools/arm64test-all.sh` runs
  `tools/a64tests/*.c` under five flag sets.  Set `ARM64CCOM` and `ARM64PY` (a venv with
  `unicorn` and `keystone-engine`).
* the tests are the same programs the amd64 backend runs; `a64args`, `a64fp`, `a64big`,
  `a64arith` and `a64data` were written for this port (argument passing on registers and the
  stack, floating point including NaN, frames and offsets beyond one instruction, constants of
  every shape, structs by value, switch, initialized data).
* `tools/exprfuzz.py` writes random programs of well-defined C (every integer width and sign,
  `float` and `double`, arrays, pointers walked with `*p++`, loops, calls), checks with the host
  compiler under UBSan that they are well defined, and compares the host's result with the
  target's.  `--narrow` finds the first call that differs and `--reduce` shrinks a failing
  program.  It found real bugs in shared pcc code, below.
* clang assembles the output (`clang --target=aarch64-linux-gnu -c -x assembler`) and `size`
  reads the object, so `tools/a64size.sh` measures bytes without a linker.

Quirks of the harness: keystone reads a bare decimal as hexadecimal once a symbol resolver is
set (the harness rewrites `#N`), and a symbol named like a register (`d10`, `s12`, `w1`) cannot
be called in the emulator tests.

## What running it found in shared code

Each of these was wrong on every LP64 target, or on all targets, and is fixed with a test:

| defect | where | test |
|---|---|---|
| `soft_cmp` ordered a negative or zero floating constant by sign instead of by magnitude, so `0.0 > -1.0` folded to 0 | `common/softfloat.c` | `fpconstcmp` |
| removing `x << 0` returned the operand with its own type, losing the type the node had been given (`(int)(unsigned)p << 0` then `%`: "Cannot generate code") | `cc/ccom/optim.c` (`zapshift`) | `shiftzero` |
| `(x << a) << b` was merged to one shift by `a+b` even when that is past the type: 0 became a shift by the sum modulo 64 | `cc/ccom/optim.c` | `shiftmerge` |
| `(unsigned)(a % b)` and `(unsigned)(a / b)` with signed operands were retyped to the unsigned operation (amd64, i386; PDP-11 had been fixed) | `arch/amd64/local.c`, `arch/i386/local.c`, `arch/aarch64/local.c` | `divretype` |
| `(long)(int)(u << c)` became `(long)u << c`: the unsigned shift wraps at 32 bits | `arch/amd64/local.c`, `arch/aarch64/local.c` | `shiftwiden` |
| PDP-11: the no-op conversion `long` to `long` did not name the register the operand was in | `arch/pdp11/table.c` | `shiftzero` |
| dead code removal deleted the only statement of a basic block and left the edges into it, so liveness lost what was live after it and a live assignment in a loop before it was deleted (shown by loop rotation, `-xdmr`; the same on amd64) | `mip/regs.c` (`bbunlink`) | `dceloop` |

## Open

Varargs (`<stdarg.h>`), `alloca`, jump tables, the standard's struct passing, `ldp`/`stp`
pairing of ordinary loads and stores (not done because of `volatile`), and a lowering of
`a ? 1 : 0` to `cset` before instruction selection (the peephole recovers the common case).
