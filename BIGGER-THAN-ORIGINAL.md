# Files where pcc `-O` is bigger than the original compiler, and why

Measured 2026-10-17 with `tools/pccdiff.sh -v -k DIR` (PDP-11, against dmr's `cc`;
text+data bytes after V7 `as`) and `tools/vaxdiff.py` (VAX, against 32V's `cc -O`).
Totals are below the originals; this is about the files that are not.  The goal set
for this work: the only files larger than the original are larger because their code
is more correct.  "Fixable" says whether the cause can be removed without making the
compiler wrong, and where the change would go.

## PDP-11

libc: 16918 bytes against dmr's 17356 (-2.5%); 6 of 89 files are bigger (it was 25).
commands: 86566 against 90178 (-4.0%); 2 of 51 files are bigger (it was 10).

| file | bytes over | cause |
|---|---|---|
| `libc/gen/qsort.c` | +14 | **correctness**: the unsigned-divide helper (below); without its 34 bytes the file is 20 smaller than dmr's |
| `cmd/sh/print.c` | +58 | **correctness**: `udiv` and `urem`, 74 bytes; without them 16 smaller |
| `libc/gen/malloc.c` | +12 | allocator choices (B); partly **correctness**: `(nw + BLOCK/WORD) / (BLOCK/WORD)` is unsigned (sizeof is), pcc shifts, dmr emits a signed `sxt; div` |
| `libc/gen/mon.c` | +6 | a parameter used twice is copied into a register (A) |
| `libc/stdio/fseek.c` | +6 | the same, and a 32-bit value spilled to frame slots where dmr pushes and pops (A, B) |
| `cmd/sh/main.c` | +4 | parameters copied into registers (A) |
| `libc/stdio/flsbuf.c` | +4 | an address-taken `char` temporary is stored from a register after template selection: `mov 4(r5),r0; movb r0,r0; movb r0,-10(r5)` against dmr's `movb 4(r5),-12(r5)` (C) |
| `libc/stdio/sprintf.c` | +2 | a parameter copied into a register (A) |

### The causes

**Unsigned divide by a variable (qsort, sh/print).  EXPLAINED, not a defect of ours.**
dmr emits `clr r0; div b,r0`, 4 bytes, which is wrong for a divisor of 32768 or more
(the `div` takes a signed divisor) and for a divisor of 1 with a large dividend: shown
by running dmr's own output (`tools/dmrbugs/udiv-variable.c`, `tools/dmrtest.sh`:
87 failing-case bits under dmr-cc, 0 under pcc).  pcc calls `udiv` / `urem`, routines
it writes into each file that uses them: `udiv` is 17 words, `urem` 20, and they are right
for every divisor (`pcctests/udiv16.c` covers divisor 0/1, 2, 32767, 32768, 40000 and
dividends 0, equal, smaller, 65535).  A shared copy in a runtime library would save about
60 bytes over the two corpora and needs the driver to link a support library; not done.
The same applies to `uldiv` and `ulrem`.

**A. A parameter used twice is copied into a register.**  The register costs a 2-word
`mov 4(r5),r2` and saves a word per use (an operand `4(r5)` has an index word), so at two
uses it is a draw in bytes and the register pressure causes spills.  `parmprop()` in
`arch/pdp11/local2.c` already uses a parameter in place when it has one use.  Using it in
place at two uses as well was tried and made libc 14 bytes and commands 6 bytes bigger,
so it was dropped.  *Fixable:* only by a better spill decision in the allocator.

**B. Allocator spill choices.**  malloc and fseek keep a temporary in a frame slot where
dmr keeps nothing live; `mov _alloct,r0; mov (r0),r0` is two loads where dmr reused the
register.  *Fixable:* in `mip/regs.c`, not by a table rule.

**C. A temporary whose address is taken.**  `c1 = c; write(fd, &c1, 1)`: the
temporary is a stack slot, but its stores are chosen as register stores and the
slot is filled in afterwards.  *Fixable:* decide the home of address-taken temporaries
before template selection (`cstknode()` in `trees.c` creates them).

## What was fixed on the way, with the file that showed it

| change | where | files where the pattern showed up |
|---|---|---|
| compare two bytes in memory (`cmpb`) | `clocal` | strcmp, strncmp, index, rindex, getenv |
| `&local` straight to the stack | `SARGSUB` shape | printf, sprintf, ctime, cmd/ |
| `?:` value dropped for a statement | `unusedval()` in pass 1 | puts, putchar, putw, cpp |
| deferred store through a pointer in memory | `postincfold`, `SSTARX` | stdio, malloc |
| `x op= e` in a condition | `andorbr()` | malloc |
| 32-bit arithmetic in place | `longrw`, `longmem`, `longcall` | fseek, ctime, sh/print, adb, ld |
| small struct assignment | `smallstasg` | malloc |
| PLUS leaves its result in the right operand's register | table rule | qsort, malloc |
| jump-table switch | `mygenswitch` | 792 bytes over the commands |
| `c ? a : b` as a condition | `andorbr()` `QUEST` | adb/input, sh |
| register result folded; `OR` operand order | `retfold` | getw |
| codes at a join; `?:` arms into the argument slot; `clr r0` argument | c2 | fgets, getc users, endopen, filbuf, getlogin, sh |
| char from a word in memory | `narrowload` | commands |
| `sizeof` unsigned | `arch/pdp11/macdefs.h` | getlogin (and a miscompile, see OPTIMIZATION-TRICKS.md) |

## VAX (32V `cc -O`)

libc gen+stdio: 19056 against 21976 (-13.3%).  56 command sources: 109980 against 118444
(-7.1%).  11 of the 56 command files are bigger: `dump` +240, `deroff` +148, `grep` +116,
`ed` +60, `cron` +52, `dd` +40, `chmod` +32, `dcheck` +24, `dumpdir` +24, `col` +20, `args` +12.

Only `ed` is understood in detail: `__urem` costs 41 bytes, and 32V's cc divides unsigned
values as signed (shown by running it, `tools/dmrbugs/udiv-variable-vax.c`, 15 failing-case
bits under 32V's cc, 0 under pcc), so that is **correctness**.  For the rest,
`tools/vaxfunsize.py` (a model of VAX instruction lengths, per function, with dmr names
truncated to 7 characters) points at the functions: `dump` `mark` +88 and `add` +87;
`grep` `main` +133 and `advance` +52; `col` `emit` +42; `chmod` `where` +36; `deroff`
`eqn` +58.  Known causes so far: 32V builds a decision tree for a sparse `switch` where we
compare in a chain; we use `extzv` for `>> 4`; `addl3 $1,x,r0` where 32V has `incl`;
`movl` copies c2 cannot yet drop (2640 redundant moves over 130 files are still removed by
c2 rather than before it).  Not yet fixed; each needs its function read.
