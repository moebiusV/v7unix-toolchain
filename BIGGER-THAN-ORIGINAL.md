# Files where pcc `-O` is bigger than the original compiler, and why

Measured 2026-10-17 with `tools/pccdiff.sh -v -k DIR` (PDP-11, against dmr's `cc`;
text+data bytes after V7 `as`).  Totals are below the originals; this is about the
files that are not.  "Fixable" says whether the cause can be removed without making
the compiler wrong, and where the change would go.

PDP-11 libc: 25 of 89 files were bigger when this was first written and 17 are now (2 to
36 bytes, 114 bytes in all; libc as a whole is 334 bytes under dmr's, 17022 against 17356,
after the changes below).  The first count was 25 of 89 files bigger, by 2 to 62 bytes (252 bytes in all, more than
made up by about 400 bytes saved in the other 64 files: libc is 152 bytes smaller overall).  Commands: 10 of 51, by 2 to 454 bytes.

## The causes, most files first

**A. Deferred addressing is missing for a pointer held in memory.  PARTLY FIXED.**
dmr stores through a pointer that lives in a global or the frame with one
instruction, `movb r3,*10+__iob; inc 10+__iob`; pcc made a copy of the pointer in a
register first (`t = p; p = p + 1; *t = c`).  pass 1 produces those three statements for
`*p++`; `postincfold()` in `mip/optim2.c` (-xdmr, targets with `MEM_DEFERRED`: pdp11,
vax) now puts the use first, through the pointer itself, and the increment after it.
Only for a global or a frame variable: through a register the deferred form `*(rN)` is
`@0(rN)`, an index word, which is as long as loading the pointer (found when getpass.c
grew 2 bytes).  libc 17138 -> 17132, commands 88268 -> 88202.  Still different from
dmr: a pointer reached through another pointer in a register (`*iop->_ptr++`, where
dmr's `*(r4)` is no shorter either), and the `movb r3,r3` that remains where the
value of the assignment is used (putchar's return value).

**B. A value computed only for its effect is still converted.**  `putc(c, f)` is
`cond ? *p++ = c : _flsbuf(c, f)`; used as a statement its value is dropped, but
pcc still emits `movb r3,r3` (sign extension of the assignment's value) before the
store.  2 bytes per `putc`.  *Files:* puts, putchar, putw, cpp.c.  *Fixable:* yes.
Evaluate the arms of a `?:` whose value is unused for effect only; an IR rewrite in
`clocal` or a `FOREFF` rule.

**C. Comparing two bytes in memory.  FIXED** in `clocal` (PDP-11, VAX; the x86
backends got compare narrowing too, which they never had).  dmr: `cmpb (r4),(r3)+`.  pcc loads both
into registers and compares them: `movb (r4),r1; movb (r3)+,r0; cmp r1,r0` (+4).
*Files:* strcmp (+4), strncmp (+4), index (+4), rindex (+4), getenv (+2).
*Fixable:* yes, a template for `cmpb` with two byte operands that are memory or
autoincrement (`arch/pdp11/table.c`).

**D. The address of a local goes through a register.  FIXED** (`SARGSUB` now also
matches frame pointer plus constant).  `f(&x)`: dmr
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

**F. 32-bit arithmetic.  FIXED for the common case.**  fseek (+62) and ctime (+46)
update longs.  dmr keeps every long in memory and updates it there
(`add r0,-12(r5); add r1,-10(r5); adc -12(r5)`); pcc kept them in register pairs, and
when the allocator spilled one it loaded the pair, updated it and stored it back around
every use, because a spilled temporary is rewritten as load / operate / store.  Now,
when two longs are live across calls (long multiply and divide are calls), the ones
updated with `x += e` / `x -= e` stay in memory, where the existing in-place templates
apply, and a long parameter updated that way is used in its own slot with no copy
(`longmem()` in `arch/pdp11/local2.c`).  The rule is that narrow on purpose: every long
in memory made libc 38 bytes smaller and the commands 316 bytes bigger, and a single
long that lives across calls fits the callee-saved pair and costs 2 bytes more per use
in memory (ld.c, ftell).  fseek +62 -> +10, ctime +46 -> gone, libc 17132 -> 17022.
What remains in fseek is the `sxt` against `clr` sign extension of an int into a long.

**G. Operand order in address arithmetic.  FIXED** with a table rule: a sum may be left
in the register of its right operand (`add AL,AR`), so `p + (n << 1)` is `mov r2,r0;
asl r0; add r4,r0` and `p` is no longer copied into the result register first.  My first
attempt, swapping the operands in a pass, produced wrong code: the shared `setleft()`
puts pointers first again at emit time, after the instruction was chosen for the other
order, so the add went into the variable instead of the sum.  That rule, unlike most of
this work, is not conditional on `-xdmr`: a table rule has nothing to test, and a
special shape would hide the register from the allocator.

**K. Small structures.  FIXED** (-xdmr).  A structure of 2 or 4 bytes assigned as a
statement was the general counted block move (`mov $1,r2; 1: mov (r0)+,(r1)+; sob
r2,1b`); it is now one or two movs (malloc's `*t++ = *s++` on a union of one word).

**H. Register and frame choices (not yet examined file by file).**  crypt (+6),
mon (+6), ld.c (+48, 71 more `mov`s), c21.c (+124, 45 more `cmp`s): pcc copies
parameters into registers (`mov 12(r5),r2`) where dmr uses them in place, and
reserves frame slots for spills.  Probably the allocator's choices rather than a
single missing rule.  *Fixable:* unknown until looked at.

**I. Differences that are not size.**  Jump placement (`jbr L3` against dmr's
repeated `jmp cret`), the order of `cmp` operands with the branch reversed, static
data in `.bss` against `.data`, and symbol-name truncation to 7 characters in dmr's
output are the same number of bytes.

**J. A variable updated in a condition (found in malloc).**  `if (++temp > 1)`:
dmr `inc L1; cmp $1,L1`; pcc loaded, incremented, stored and compared through a
register.  **FIXED** in pass 1 (`andorbr`, `-xdmr`): the update becomes a statement
and the variable is compared.  Not done for a register variable, a plain
assignment or a compare with 0, where the old form is smaller.

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
