# Files where pcc `-O` is bigger than the original compiler, and why

Measured 2026-10-17 with `tools/pccdiff.sh -v -k DIR` (PDP-11, against dmr's `cc`;
text+data bytes after V7 `as`) and `tools/vaxdiff.py` (VAX, against 32V's `cc -O`).
Totals are below the originals; this is about the files that are not.  The goal set
for this work: the only files larger than the original are larger because their code
is more correct.  The PDP-11 meets it.  The VAX meets it for every file but one, which
is the open item (and `mktemp`, which is the correct code).  Optimization numbers per technique are in `OPTIMIZATION-TRICKS.md`;
the files pcc cannot compile at all are in `OLD-K&R-FORMS.md`.

## PDP-11

libc: 16838 bytes against dmr's 17356 (-3.0%); 1 of 89 files is bigger (it was 25).
commands: 94170 against 98102 (-4.0%) for the 52 sources that compile; 1 of 52 is bigger (it was 10).

| file | bytes over | cause |
|---|---|---|
| `libc/gen/qsort.c` | +12 | **correctness**: the unsigned-divide helper (below); without its 34 bytes the file is 20 smaller than dmr's |
| `cmd/sh/print.c` | +54 | **correctness**: `udiv` and `urem`, 74 bytes; without them 16 smaller |

### The cause

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

### The files that were bigger and are not now

`mon`, `fseek`, `malloc`, `flsbuf` were left at the end of the previous round, "not understood in detail".
They were understood by reading the assembly of the pieces dmr-cc produces:

| file | was | cause | fix |
|---|---|---|---|
| `libc/gen/mon.c` | +6 | a long made from a word held a register pair while the other operand was built; a word converted among `int`, `unsigned` and pointer cost a copy; **and a miscompile**: the long left shift printed the result's register instead of the pinned `r0:r1` | `SSXT` (OPTIMIZATION-TRICKS.md 56), `nopconv` (55), the template prints `r0` |
| `libc/stdio/fseek.c` | +6 | a 32-bit value spilled to frame slots where dmr pushes and pops | `spillpush` in c2 (53) |
| `libc/gen/malloc.c` | +12 | `*t++ = *s++` through a pointer the allocator keeps in the frame, add and use as separate steps; `(nw + BLOCK/WORD) / (BLOCK/WORD)` unsigned (a shift pair, correct) where dmr emits a signed `sxt; div` | `SSTARINC` (54) |
| `libc/stdio/flsbuf.c` | +4 | an address-taken `char` temporary stored from a register after template selection | `narrowload` after a spill (34) |

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
| `p->m++`, `p->m op= v`: pointer kept once, offset in the address | `hoistbase()` | commands -64 |
| register result folded; `OR` operand order | `retfold` | getw |
| codes at a join; `?:` arms into the argument slot; `clr r0` argument | c2 | fgets, getc users, endopen, filbuf, getlogin, sh |
| char from a word in memory | `narrowload` | commands |
| `sizeof` unsigned | `arch/pdp11/macdefs.h` | getlogin (and a miscompile, see OPTIMIZATION-TRICKS.md) |
| autoincrement from `add $2,rN` | c2 `autoinc` | stdio, commands |
| in-place pointer step through the frame | `SSTARINC` | malloc, realloc |
| spilled value as push/pop | c2 `spillpush` | fseek |
| one-word conversions retype | `nopconv` | mon |
| long from a word pushed with `sxt` | `SSXT` | mon |
| matcher ranks an operand already in the result register one cheaper | `findops()` in `mip/match.c` | commands -38, i386 -13, amd64 -24 |

## VAX (32V `cc -O`)

libc gen+stdio: 18336 against 21976 (-16.6%); 2 of 91 files are bigger.  56 command sources:
106204 against 118444 (-10.3%); 0 of 56 are bigger (11 were at the start of this round).

| file | bytes over | cause |
|---|---|---|
| `libc/gen/mktemp.c` | +92 | **correctness**: `unsigned % 10` calls `__urem` (the helper is appended to the file); 32V's cc divides unsigned values as signed (`tools/dmrbugs/udiv-variable-vax.c`: 15 failing-case bits under 32V's cc, 0 under pcc) |
| `libc/stdio/doscan.c` | +4 | **open**: `*iop->_ptr++` through a `FILE *` parameter in its stack slot is `movl 4(r0),r0; movl 4(ap),r2; incl 4(r2)` (11 bytes) where 32V's cc has `incl 4(r0); subl3 $1,4(r0),r0` (8): ours loads the old pointer into the register that held the `FILE *`.  `movl 4(r0),r1; incl 4(r0)` would be 7; a choice of result register for a memory-pointer post-increment, not tried |

`ed` also pays 41 bytes for `__urem` and that is correctness too; it is still smaller than `cc -O` in total.

What cleared the files that were bigger, in the order found with `tools/vaxfunsize.py`:

| cause | where it was | fix |
|---|---|---|
| `map[(ino-1)/16] \|= m` computed the index three times | `dump` `mark`, `add`; `deroff` | the address of an `op=` / `++` lvalue kept once (OPTIMIZATION-TRICKS.md 36) |
| a dense `switch` as a compare chain | `grep`, `ed` `advance` | `casel` table (17, and the flow-graph edge, 41) |
| `y++;` as `addl3 $1,y,r0; movl r0,y` | `grep` `main` | `(x += 1) - 1` as a statement is `x += 1` (37) |
| `iop->_ptr++` loaded the parameter twice | `getc` everywhere (`cron`, `doscan`) | pointer kept once, offset in the address (36) |
| parameters copied to registers for one use; loads of unused parameters | `stty`, `gtty`, `calloc`, `cfree`, `fgets` | `parmprop` for the VAX (40) |
| `while (*s++ = *t++)` as a load, a store and a `tst` | `strcat`, `strncat`, `strcatn` | width-aware liveness and `tst` after `mov` in c2 (42, 43) |
| `movl X,r0; movl r0,Y` in a function that returns nothing | `setbuf` | `ret # noval` (45) |
| a dead load of the old pointer for `*p++;` | `ungetc` | dead register writes in c2 (45) |
| a modified `double` parameter copied to a frame slot; `op x,r0; movq r0,Y` | `modf` | parameter conversions looked through so the slot is reused (40); `pairfold` (57) |
| three loads of one stack slot for `getc` through a spilled `FILE *`; `casel` reading every register | `doscan` | `casel` liveness and the second backward pass (58) |
| `*--p->ptr = c` as `subl3`, `movl` | `ungetc` | `decl M; movl M,r` (44) |
| `movl r10,r0; movb r0,*p` | `puts` | `movb r10,*p` (44) |
| functions aligned to 4 | every file | `.align 1` under `-xdmr` (46): 40 bytes in libc, 396 in the commands |
