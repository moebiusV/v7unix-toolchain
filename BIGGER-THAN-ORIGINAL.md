# Files where pcc `-O` is bigger than the original compiler, and why

Measured 2026-10-17 with `tools/pccdiff.sh -v -k DIR` (PDP-11, against dmr's `cc`;
text+data bytes after V7 `as`).  Totals are below the originals; this is about the
files that are not.  "Fixable" says whether the cause can be removed without making
the compiler wrong, and where the change would go.

PDP-11 libc: 25 of 89 files are bigger, by 2 to 62 bytes (252 bytes in all, more than
made up by about 400 bytes saved in the other 64 files: libc is 152 bytes smaller overall).  Commands: 10 of 51, by 2 to 454 bytes.

## The causes, most files first

**A. Deferred addressing is missing for a pointer held in memory.**  dmr stores
through a pointer that lives in a global or a struct with one instruction,
`movb r3,*10+__iob` or `movb *(r4),r0`; pcc loads the pointer into a register first
(`mov 10+__iob,r0; movb r3,(r0)`), 2 bytes more each time.  Hits every stdio
`putc`/`getc` macro use.  *Files:* puts (+8), putchar (+4), putw (+4), getw (+2),
fgets (+2), getchar (+2), sprintf, filbuf, flsbuf, and most of the cpp.c (+454),
macro.c, ld.c and c21.c excess.  *Fixable:* yes.  The VAX backend has the same
feature (`deferred()` in `arch/vax/order.c`, the `*` flag in the packed register);
the PDP-11 backend has the `*off(rN)` half (a `UMUL` of an `OREG`) but not `*name`,
and not the case where the pointer is also incremented in memory (`*p->_ptr++`).
Place: `arch/pdp11/order.c` (`offstar`, `myormake`), `local2.c` (`adrput`).

**B. A value computed only for its effect is still converted.**  `putc(c, f)` is
`cond ? *p++ = c : _flsbuf(c, f)`; used as a statement its value is dropped, but
pcc still emits `movb r3,r3` (sign extension of the assignment's value) before the
store.  2 bytes per `putc`.  *Files:* puts, putchar, putw, cpp.c.  *Fixable:* yes.
Evaluate the arms of a `?:` whose value is unused for effect only; an IR rewrite in
`clocal` or a `FOREFF` rule.

**C. Comparing two bytes in memory.**  dmr: `cmpb (r4),(r3)+`.  pcc loads both
into registers and compares them: `movb (r4),r1; movb (r3)+,r0; cmp r1,r0` (+4).
*Files:* strcmp (+4), strncmp (+4), index (+4), rindex (+4), getenv (+2).
*Fixable:* yes, a template for `cmpb` with two byte operands that are memory or
autoincrement (`arch/pdp11/table.c`).

**D. The address of a local goes through a register.**  `f(&x)`: dmr
`mov r5,-(sp); add $6,(sp)`; pcc `mov r5,r0; add $6,r0; mov r0,-(sp)` (+2).
*Files:* printf (+2), sprintf (+4), ctime, and several in cmd/.  *Fixable:* yes, a
template for `ADDROF` of an automatic straight to the stack.

**E. Unsigned divide by a variable.**  qsort (+20) divides an unsigned by a variable.
dmr emits `clr r0; div (sp)+,r0`, which is wrong whenever the divisor is 32768 or
more; pcc calls a helper that is right, and appends the helper's text to the file
(about 30 bytes, paid once per file that uses it).  *Not fixable by being smaller
in the same way:* the inline form is incorrect.  *Improvable:* put the helpers in
libc once instead of in every object (a decision for the C library, not the
compiler).  Also affects scanf, getpw, mon where they divide.

**F. 32-bit arithmetic.**  fseek (+62) and ctime (+46) work on `long`.  dmr updates
a long in memory in place (`add r0,-12(r5); add r1,-10(r5); adc -12(r5)`) and keeps
temporaries on the stack with `-(sp)`/`(sp)+`; pcc loads the pair into registers,
operates, stores it back, and spills temporaries to frame slots.  It also zero
extends where dmr sign extends (`clr -(sp)` against `sxt -(sp)`) in one place: the
conversion looks right for the unsigned operand but needs checking.  *Fixable:*
partly, a large piece of work.  In-place long `+=`/`-=` on memory is a template
(`arch/pdp11/table.c`); the spills are the register allocator's choice.

**G. Operand order in address arithmetic.**  malloc (+40), qsort: `p + (n << 1)`
is `mov r2,r0; asl r0; add r4,r0` in dmr (3 instructions) and
`mov r4,r0; mov r2,r1; asl r1; add r1,r0` in pcc (4): the pointer is put in the
result register first, and the shifted index then needs a second one.  *Fixable:*
yes, in pass 2 prefer the operand that needs computing as the one that goes in the
destination register (commutative `PLUS`), `arch/pdp11/order.c` or `table.c`.

**H. Register and frame choices (not yet examined file by file).**  crypt (+6),
mon (+6), ld.c (+48, 71 more `mov`s), c21.c (+124, 45 more `cmp`s): pcc copies
parameters into registers (`mov 12(r5),r2`) where dmr uses them in place, and
reserves frame slots for spills.  Probably the allocator's choices rather than a
single missing rule.  *Fixable:* unknown until looked at.

**I. Differences that are not size.**  Jump placement (`jbr L3` against dmr's
repeated `jmp cret`), the order of `cmp` operands with the branch reversed, static
data in `.bss` against `.data`, and symbol-name truncation to 7 characters in dmr's
output are the same number of bytes.

## Per file, PDP-11 libc

| file | bytes over | main cause |
|---|---|---|
| fseek | +62 | F |
| ctime | +46 | F, D |
| malloc | +40 | G |
| qsort | +20 | E, G |
| puts | +8 | A, B |
| crypt, mon, flsbuf, scanf | +6 | H, A, E |
| sprintf, getlogin, index, rindex, strcmp, strncmp, putchar, putw, getpw, filbuf | +4 | C or A/B/D |
| getenv, endopen, fgets, getchar, getw, printf | +2 | C or A |

(`getlogin`, `endopen`, `filbuf`, `flsbuf`, `getpw`, `scanf` were read quickly; their
exact split between the causes above has not been counted.)

## VAX (32V `cc -O`)

6 of 38 command sources are bigger: deroff +184, cron +52, dd +44, chmod +32, dcheck
+12, args +12.  Instruction counts against `cc -O` over these files:
`addl3` +18 and `incl` −15 (a `x++` whose old value is not needed still goes through
`addl3 $1,x,r0` in a few places), `movb` +14 against `cvtlb` −14 (same size), `subl2`
−22 (frame set-up written differently), `movl` +19 (copies c2 cannot yet drop).
*Fixable:* the `addl3` against `incl` and the `movl` copies, in the same IR passes
(copy propagation, below); the rest are equal in bytes.

## What would fix most of it

In order of bytes and files per piece of work: A (deferred modes), C (`cmpb`
memory to memory), D (`&auto` to the stack), B (dead conversion), G (operand
order), then E as a libc packaging decision, F as a larger project, H by looking.
A, B, C, D and G together should remove most of the 252 bytes.
