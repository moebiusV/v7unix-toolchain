# Porting the V7 PDP-11 C toolchain to a modern host

This is the story of taking the Seventh Edition Unix C toolchain — `cc` (the
`c0`/`c1`/`c2` passes), `as`, and `ld` — out of 1979 and making it build on a
64-bit Linux host, so it can cross-compile PDP-11 code.  It is as much a story
about how the C language changed between K&R C and C99 as it is about the
toolchain itself: almost every hard problem turned out to be a place where the
old compiler relied on something C99 no longer allows, or on a PDP-11 property
that doesn't exist on a modern machine.

## 1. What the toolchain actually is

V7's compiler is three programs connected by a text/binary pipe:

    cpp     the preprocessor (in V7, part of `cc`; a separate pass)
    c0      the first pass — lexer, parser, expression tree builder
    c1      the second pass — reads c0's tree, emits PDP-11 assembly
    c2      the peephole optimizer (runs between c1 and `as`)
    as      the assembler (self-hosted PDP-11 assembly)
    ld      the linker

`c0` and `c1` are where the interesting work is, and they embody two of the
least-appreciated design decisions dmr made:

* **A data-driven code generator.**  `c1` does not contain `switch(op)` logic
  for every PDP-11 operator.  Instead the entire instruction-selection
  knowledge lives in a *machine description table* (`table.s` in the source),
  and `c1` is a generic tree-walker that consults it.  This is why the port
  could produce byte-identical output: `c1` + the table is a fixed function of
  the tree, with no heuristic to get wrong.

* **The two-pass split.**  `c0` parses C and writes the tree as a compact
  binary file; `c1` reads it back.  The tree is a serialized form of the
  program's structure, and the two halves communicate only through it.

## 2. The C dialect K&R C allowed (and C99 took away)

The very first obstacle is that the sources are *not* valid C99.  Each gap
below is a place where the old language was more permissive, and each had to be
resolved mechanically.

### 2.1 Juxtaposed initializers

V7 writes an initializer with no `=` at all:

    int x 3;
    char mov[] "mov";

The initializer simply follows the declarator, juxtaposed.  C99 requires `=`.
The modernizer rewrites these to `int x = 3;` and `char mov[] = "mov";`.  This
"conjunctive" style was how C's initializers looked before the `=` was settled
on; the grammar tolerated it because there was no ambiguity the parser had to
disambiguate.

### 2.2 `=op` compound assignment

Before `+=`, `-=`, `<<=` etc. existed, the spelling was `=+`, `=-`, `=<<`:

    t =+ 2;      /* t += 2 */

The two-character operator `=+` parses as "assignment of addition", but a
naive text modernizer reads it as `t = (+2)` — assignment of a unary-plus
constant — which silently changes nothing's value but *does* change the parse.
This bit the port: `cvopt` used `t =+ 2`, `t =+ 4`, `f =+ 16`, and a script
that only recognized `+=` (but not the historical `=+`) rewrote them to unary
plus, corrupting `table.i` with raw control bytes.

### 2.3 Bare `return` — and why it is a tail call

This is the one worth dwelling on.  On the PDP-11, a function's return value
is passed in register `r0`.  So a `return` statement doesn't *move* a value;
it just jumps back, and whatever is in `r0` is the result.  Two consequences:

* `return f();` compiles to "call `f()`, then jump home" — the value of `f()`
  is already in `r0`, so the caller's `return` adds no instruction.  That is
  precisely a tail call: the callee's epilogue *is* the caller's epilogue.

* A **bare `return;`** — or a `return` with no value — leaves `r0` untouched.
  V7's code relied on this: a function that "returned nothing" actually
  returned the last value computed, as a free channel.  When modernized, these
  had to become explicit `return 0;` where the value mattered, or be left as
  `return;` where it didn't.  The bug class this opens is real: a bare `return`
  in a value-returning function is a silent data flow, not a type error.

The punchline the source makes explicit: the assembler does the same thing by
hand.  See §6.1.

### 2.4 Member-access-by-name — the implicit union

This is the single biggest port problem, and it is a *deliberate* economy, not
an oversight.  In early C there was **one global member-name space**.  A
`struct tnode *` could reach any member name the compiler had seen — `value`
(a constant's field), `class` (a symbol's field), `tr1` (a tree's field) —
regardless of the pointer's declared type, because the compiler resolved the
*name* to a byte offset and didn't type-check which struct it came from.

V7 leaned on this everywhere: its tree nodes are a tagged union
(`tnode`/`cnode`/`lnode`/`fnode` for operators/constants, `hshtab`/`phshtab`
for symbols), and the code reaches across them constantly — `tree->value`,
`tree->lvalue`, `tree->cstr`, `tree->hclass`, all through a `struct tnode *`.

C99 forbids reading one union member through another.  The port collapses the
family into one `struct node { union { … } u; }` and qualifies every access
(`tree->u.cnode.value`).  In `c1` the `op`/`type` pair stays top-level because
every node shape shares it; in `c0` even that goes into the union, because a
`NAME` node *is* a hash-table entry — the parser and the symbol table are the
same objects.

### 2.5 Default trailing arguments and tentative definitions

Two more K&R-isms:

* **Default trailing args.**  `tnode(a, b, c)` was a call to a function
  declared `tnode(op, type, tr1, tr2)`; the missing `tr2` defaulted to
  whatever garbage was on the stack, which the callee never read for unary
  operators.  The port makes the `NULL` explicit.

* **Tentative definitions.**  `c1.h` declared `char *funcbase;` etc. *without*
  `extern`, and every translation unit that included it contributed a tentative
  definition that the linker merged as a "common" symbol.  C99 has no common
  symbols, so the globals move to `extern` declarations plus one `globals.c`
  with the real definitions.

### 2.6 Implicit `int`

Everywhere.  `error(s, p1, …, p6)` with no return type, `register *p;` with no
base type, parameters with no type.  The modernizer turns these into `int`
(or, on a 64-bit host, the correct `int16_t`/pointer type), which is where the
next set of problems begins — because on a host, `int` is 32 bits and pointers
are 64, while the code assumed both were 16.

## 3. The port, mechanical then manual

The port is a pipeline of three passes over each source file, plus two
one-off reverse-engineering jobs.

### 3.1 The three passes

    knr2c99.py --dialect v7 --rules X.rules.json   # K&R → C99
    union-node.py --spec X.spec.json               # struct family → union node
    build-host.py / c0-host.py                     # host-specific fixes

`knr2c99.py` handles the *mechanical* dialect changes: old-style function
definitions become prototypes, `=op` → `op=`, juxtaposed initializers get `=`,
missing includes are added, and word-sized `int` becomes `int16_t` (because the
code genuinely depends on 16-bit arithmetic — `MAXINT` is 077777).

`union-node.py` does the member-access-by-name refactor, driven by a spec file
that names the struct family and maps each divergent member to its sub-struct.

The `build-host.py`/`c0-host.py` pass is where the *non-mechanical* knowledge
lives, because it is where the assumptions about a 16-bit PDP-11 meet a
64-bit host (§4).  It is the only pass that required judgment.

### 3.2 Reverse-engineering `table.s`

`table.s` is the machine-description table, assembled into `table.o`.  Rather
than translate the assembly by hand, the port writes a tool (`table2c.py`)
that reads the *assembled object* and re-emits it as host-layout C: the text
section becomes null-terminated bytecode strings plus `struct optab[]`, the
data section becomes `struct table[]`.  The round-trip is verified lossless —
reconstructing text+data from the emitted structs reproduces `table.o`
byte-for-byte.  This is the safest possible translation: no hand-reading of the
instruction patterns, just a faithful re-serialization.

### 3.3 The assembler: `.s` → C

The assembler is the one non-C component — it is written in PDP-11 assembly
(`as11.s` … `as29.s`), self-hosted because `as` can't use the C library.  It is
translated to C (`as1.c`, `as2.c`).  This is where the tail calls show up
(§6.1), and where the middle-endian word order (§4.3) had to be reproduced
exactly for the emitted object files to match the on-disk `crt0.o`/`fcrt0.o`.

## 4. Obstacles no tutorial mentioned

### 4.1 `sbrk` fights glibc's `malloc`

The V7 allocator is a bump allocator: `curbase`/`coremax` into a region grown
with `sbrk(1024)`.  On a modern host, `sbrk` and glibc's `malloc` both manage
the same heap; the toolchain's raw `sbrk` corrupted `malloc`'s metadata and the
second `freopen` died with `munmap_chunk(): invalid pointer`.  The fix is to
allocate a private arena with `mmap` and bump within it, never touching `sbrk`.

### 4.2 The `delay()` brace bug — indentation that changes control flow

In `c1`'s `delay()`:

    if (opdope[p->op]&BINARY) {
        if (p->op==LOGAND || p->op==LOGOR)
            return(0);
        }
        p1 = sdelay(&p->tr2);        /* <— OUTSIDE the if */

The closing brace is mis-indented; `p1 = sdelay(&p->tr2)` runs unconditionally.
For a unary operator `p->tr2` is NULL, and `sdelay` dereferences it.  On the
PDP-11 this is harmless — address 0 is readable, returns garbage that isn't
`INCAFT`/`DECAFT`, so `sdelay` returns 0.  On a host with memory protection it
segfaults.  This is the classic "it worked on the old machine" bug, and it is
the kind of thing that only shows up when you run the code, not when you read
it.

### 4.3 Middle-endian word order

The PDP-11 is not little-endian and not big-endian for multi-word values: a
16-bit word is little-endian, but a 32-bit `long` is stored **high word first,
then low word**, each word little-endian.  (And V7's filesystem block pointers
are 3 bytes — `[hi, lo, mid]` — the quirk that trips everyone.)  `c1` reads a
double as two 16-bit words, and `c0` emits a 32-bit `long` constant as high
word then low word; both had to reproduce this order exactly or the
intermediate code would be misread.

The mechanical port initially "fixed" the word split the obvious-but-wrong way:
V7's `p->lvalue.intx[0]`/`[1]` (high/low) became `((int16_t*)&lvalue)[0]`/`[1]`,
which reinterprets the *host* `int32_t`'s bytes and so depends on the host's
endianness — swapping high and low on a little-endian host.  The correct,
endian-independent form extracts by value: `(uint32_t)v >> 16` (high) and
`(uint32_t)v & 0177777` (low).  (`pdp11_double`/`pdp11_float` in §4.8 do the
same for floats.)

### 4.4 Generic registers — one variable, two types

The code reuses a single register variable as `int` in one arm and a pointer in
another (`build()`'s `t1`, `getree()`'s `t`, `unoptim()`'s `p`), which worked
when both were 16 bits and is a type error when a pointer is 64.  Each such
register had to be split into two typed variables.

### 4.5 V7's `printf` skips a NUL `%c`; glibc's doesn't

`psoct()` in `c1` prints a signed octal with `printf("%c%o", sign, n)`, where
`sign` is `'-'` for a negative number and NUL for a positive one.  V7's
`doprnt.s` skips a NUL `%c` (`bic $!377,r0; beq`), so the NUL never reached the
output.  glibc's `printf` has no such special case and emits the NUL byte, so a
positive operand was prefixed with a stray NUL.  The port emits the sign only
when it is real:

    if (sign)
            putchar(sign);
    printf("%o", n);

### 4.6 The readable null page — a recurring class of crash

The PDP-11 had **no memory protection**: address 0 was readable, so V7 code
freely dereferenced `NULL` and read garbage, relying on "garbage != what I'm
looking for" to skip the branch.  x86-64 maps address 0 to a fault, so each of
those reads becomes a SIGSEGV.  This is not one bug but a family, found one at a
time by feeding the port real V7 source (`tools/v7check.sh`).  Fixed so far:

- **`c0` — the "empty arglist" marker is `NULL`.**  `tree()` pushes `*cp++ =
  NULL` for a bare call `f()` (`case MCALL`), and `build(CALL)` pops it into
  `disarray()`/`chkfun()`, which dereferenced it.  Added `if (p==NULL)
  return(NULL)` to `disarray` and `chkfun` (c01.c), and guarded the `t2 =
  p2->type` read in `build()` (`t2 = p2 ? p2->u.tnode.type : 0`).
- **`c1` — `chkleaf()` built a unary `LOAD` node with `tr2` uninitialised.**
  It sets `op`/`type`/`degree`/`tr1` but not `tr2`, then `cexpr` reads
  `tr2->type` (c10.c:483).  Set `lbuf.u.tnode.tr2 = NULL` and guarded the
  `tr2`-dereference with `tree->u.tnode.tr2!=NULL &&`.
- **`c2` — `dualop()` dereferences a `NULL` `code` string** (a `MOV` node whose
  operand string was empty, so `copy()` returned 0).  Not yet fixed at the time
  of writing; see §7.
- **`c1` — `xdcalc()` reads `p->type` on a `NULL` node.**  `dcalc()` already
  returns 0 for `p==NULL`, but `xdcalc()` then did `if (d<20 && p->type==CHAR)`
  with `p` still `NULL`.  Guarded the dereference (`crypt.c` was the first file
  to trip it).
- **`c1` — `sreorder()` reads `p->tr1` after `optim()` collapses `p` to a
  leaf.**  `optim(p)` can return a `NAME`/`CON` node, after which
  `p->u.tnode.tr1` is the *name* node's `offset`/`nloc` read as a pointer; on
  V7 that read the null page and skipped the switch, on x86 it faulted.
  Re-check `opdope[p->op]&LEAF` after each `*treep = p = optim(p)`
  (`malloc.c`).

The lesson for any further port: grep the original for a bare `->field` on a
pointer that can be a `copy()`/`alloc()` result, and add the `NULL` guard the
PDP-11 got for free.

### 4.7 The hardware stack becomes a fixed buffer

The assembler's expression parser pushes and pops directly on the PDP-11
**hardware stack** (register R6): `mov r2,-(sp)` / `bis (sp)+,r2`.  That stack
is the whole stack segment, effectively unbounded, so a *small* push/pop
imbalance never bites.  The port had to turn `sp` into a fixed C buffer
(`static int estk[64]`, `sp = estk + ESTK`), so any imbalance now overflows a
concrete array and clobbers its neighbours.

The one that broke `as` on real C output was in `section()` (`.text`/`.data`/
`.bss`, `as26.s` `opl25/26/27`).  `opline()` pushes the opcode before
dispatching, and **every** handler must pop it.  The original ends with
`tst (sp)+`; the port dropped that one instruction — it also dropped the
`mov r0,-(sp)`/`mov (sp)+,r0` temporary, but those cancel, so the net was a
single leaked slot per section directive.  `cpp.s` has 36 such directives, and
each leak pushed the parser's stack one slot lower until `expres1`'s `PUSH('+')`
underflowed `estk` and wrote `0x2b` into the high 32 bits of the adjacent
`adrp` pointer (a `static int *` one buffer below `estk`).  `getx()` then did
`*adrp++` through a now-garbage pointer → SIGSEGV on input that V7 assembles
fine.  The fix is the missing pop, restoring the original's balance:

    dot = savdot[type - 025];
    dotrel = type - 023;
    sp++;                              /* tst (sp)+: pop the opcode */

Confirmed by running `as` to completion on `cpp.s`: `sp == estk+64` (empty) at
`saexit`.

The general lesson for this port: every `jsr pc,<handler>` from `opline()` that
does not go through the early `xpr` path has exactly one matching `tst (sp)+` /
`bis (sp)+,r2` / `mov (sp)+,rN` before its `rts pc`, because `opline()` pushed
the opcode.  When a handler in the C port neither `POP()`s nor `sp++`s, it leaks
one slot per call — grep each `oplNN`/`section`/`opl17` for the missing pop.

### 4.8 The PDP-11 floating-point format is not IEEE-754

A PDP-11 `double` is four 16-bit words: word 0 = sign(1) + exponent(8,
excess-128) + the high 7 mantissa bits, words 1-3 the low 48 mantissa bits; the
implicit leading bit makes the mantissa a fraction in `[0.5, 1.0)`.  A 32-bit
`float` is the same in two words (23 explicit mantissa bits).  None of that
matches IEEE-754's sign + excess-1023 exponent + 52-bit fraction, so dumping
the host `double`'s bytes (`printf("%o;%o;%o;%o", fvalue)`) emitted the wrong
representation.

`pdp11_double(d, w[4])` / `pdp11_float(d, w[2])` (c12.c) convert the host
double *by value* via `frexp`/`ldexp` — `frexp` yields `m ∈ [0.5, 1.0)` and
`e`, then `M = (uint64_t)ldexp(m, 56)` is the 56-bit mantissa with the implicit
1 in bit 55 — so the result is independent of both the host's byte order and
its float representation.  The same helper replaces the SFCON ("short float"
that fits one word) test and the sign-toggle in `optim()` (c12.c), which had
also been expressed as byte casts.  Remaining known gap: a decimal literal that
needs more than 53 bits (e.g. `.03`) rounds to IEEE-754's 53 bits on the host,
where V7's own `atof` rounded to 56, so one or two low mantissa bits can differ
(`ecvt.o` differs from the V7 image by exactly one word).

## 5. What the port produced

Both halves now build as host binaries:

    make -C cc/host       # cc/host/c1   — the code generator
    make -C cc/host/c0    # cc/host/c0/c0 — the parser

and the pipeline runs end-to-end: `c0 src.c t1 t2` emits the tree, `c1 t1 t2
out.s` emits PDP-11 assembly.  A recursive-factorial-plus-loop test produces
correct PDP-11 code (`jsr r5,csv` prologue, `jsr pc,*$_fact` for the recursion,
`mul r1,r1` etc.).

## 6. dmr's architectural choices worth noticing

### 6.1 Tail calls in the assembler

The PDP-11 assembler ends several routines with a bare `jmp` to another
routine, not `jsr` + `rts`:

    assem1:
        jmp     assem          ; tail-call back into the main assembly loop
        ...
        jmp     aexit          ; tail-call into the error/exit path

`jsr pc,assem` would push a return address and `rts` would pop it; a bare `jmp`
does neither, so the callee's own `rts` returns to *its* caller.  It is the
same optimization compilers do today, done by hand in 1975 assembly — and it
is exactly the observation from §2.3: on the PDP-11 the return value (in `r0`)
and the return address (on the stack) are the whole calling convention, so a
tail call is literally just "jump instead of call-and-return".  The C-level
mirror is that `return f();` needs no instruction, and a bare `return;` is a
tail call that leaves `r0` flowing through.

### 6.2 The data-driven machine description

`c1` is not a hand-written case analysis of every instruction; the entire
instruction-selection table is data (`table.s` → `table.o` → `table.c`), and
`c1` is a generic interpreter over it.  This is a much earlier idea than it
usually gets credit for — it's the ancestor of the "table-driven code
generator" school.

### 6.3 The implicit union as economy

The member-access-by-name discipline (§2.4) looks like a type-safety hole, but
it is also what let one `struct node` serve as tree node *and* symbol-table
entry without a tagged-union ceremony the 1979 compiler didn't have.  The
modern port has to *reintroduce* that ceremony in C99 — which is the irony:
what was free in the old language became the hardest single refactor.

## 7. Two kinds of "port", and what's left

There are actually two different activities here, and it is worth keeping them
straight, because they have very different end goals:

1. **Host-port the cross-toolchain.**  `c0`, `c1`, `as`, `ld` run *on the
   host* so they can cross-compile PDP-11 code from a modern machine.  This is
   the pipeline above, and it is the part that makes a *cross-toolchain*.

2. **Source-modernize the target-resident programs.**  Anything that only ever
   runs *on* the PDP-11 — `adb` foremost, but ultimately all of V7's userland
   — doesn't need a host port.  It just needs its K&R C updated to the
   c99/pcc dialect, so modern pcc can compile it *for* the PDP-11 target.  No
   `union-node`, no host fixes; it's `knr2c99` plus whatever the dialect
   checker flags.

`adb` falls squarely in the second bucket: it's a PDP-11 debugger (it reads
PDP-11 `a.out` files and can poke the running kernel), so there is no useful
"run it on the host" target.  Its "port" is purely source modernization so pcc
cross-compiles it for pdp11.

* **Byte-identical verification** — run the host `c0`/`c1` against the real V7
  compiler on real V7-era sources and diff the assembly (the `runv7` helper
  boots a real V7 for this).
* **Float/endianness** — confirm the double-as-two-words handling matches a
  real V7's, since it is the one place still endianness-sensitive.
* **`adb`** (bucket 2) — modernize `cmd/adb/*.c` to the c99/pcc dialect so pcc
  cross-compiles it for pdp11.
* **Wire `cpp` and the `cc` driver** — to turn the passes into a single
  cross-compiling `cc`.
* **`c0` segfault on real V7 source** — `tools/v7check.sh` surfaced this:
  cross-compiling the original `cmd/cc.c` crashes `c0` in `disarray(NULL)`
  (a NULL node on the expression stack, reached `build()` → `tree()` →
  `statement()`; `CMSIZ=40` overflow is separately checked and errors cleanly,
  so it's a logic bug, not a size bug).  Trivial files compile; real V7 sources
  with deep/compound expressions trip it.  Blocks the toolchain-from-`orig`
  build until fixed.

## 8. The build tools: make, yacc — a third kind of port

§7 drew two kinds of port.  There is a third, which this section covers: the
**build tools** — `make`, `yacc`, `lex`, `ar` — that must run *on the host* so
they can drive a cross-compilation of the V7 tree.  They are neither the
cross-toolchain (they emit no PDP-11 code) nor target-resident (they are not
compiled *by* pcc); they are host programs in their own right.  They go
through the same `orig → c99 → modern` pipeline but skip `union-node` and
`build-host.py`, because their hard problems are a different set of host-API
collisions.

The census that chose them (42 makefiles under `usr/src`): the programs that
must be *ported* are `make`, `yacc`, `lex`, `ar`, **plus `sh` and every command
the makefiles invoke** — `cp`, `rm`, `cmp`, `mv`, `echo`, `cat`, `touch`,
`mkdir`, `grep`, `chmod`, `chown`, `ls`, `du`, `sed`, `tar`, `tp`, `size`,
`install`, `pr`, `diff`, `strip`.  The goal is full self-hosting: build the V7
tree with *no* host commands.  `cpio` is not used at all.  Directory split
(see CROSSCOMPILE.md §7): phase-1 commands (`sh` `mv` `cp` `rm` `cmp`) live in
`modern/`; whole-tree-only commands in `modern2/`.

### 8.1 `make` — host adaptations

`make` is six C files + `defs` + `gram.y`.  K&R→C99 is mechanical; the rest is
these moves, each a place where V7 assumed a PDP-11 (or a pre-POSIX host) that
a modern host does not provide:

* **`waitpid` → `childpid`.**  V7 names its child-PID global `waitpid`; a
  modern host has `waitpid(2)`.  Renamed (`defs`, `main.c`, `dosys.c`).

* **`sprintf` returns `int` now.**  V7's `sprintf` returned `char*` (the
  buffer), so `fatal(sprintf(buf, s, t))` was valid.  glibc's returns `int`;
  the two uses become `sprintf(buf, …); fatal(buf);` (`misc.c`).

* **`signal` handling.**  `sigivalue = (int)signal(SIGINT, SIG_IGN) & 01` —
  extracting the "was it ignored" bit from the old handler — becomes
  `== SIG_IGN`, and handlers are typed `void (*)(int)` (`main.c`).

* **`srchdir()`: `sys/dir.h` → `dirent.h`.**  V7 read a directory as a raw
  `FILE*` of `struct direct` records; a modern host has `opendir`/`readdir`.
  `struct opendir.dirfc` becomes `DIR*`, and `doclose()` calls `closedir`
  (`files.c`, `defs`, `dosys.c`).

* **V7 `ar.h`/`a.out.h` on-disk layouts.**  `lookarch()` parses the `a(b)`
  archive-member notation; the PDP-11's middle-endian 32-bit fields (`ar_date`,
  `ar_size`) are kept as `int16_t[2]` and reassembled with `mkl()` — the same
  technique as `ld` (§4.3) (`files.c`).

* **`builtin[]` made writable.**  The built-in suffix rules are `char *`
  string literals, but `eqsign()` splits a `NAME=value` line *in place*.  On
  the PDP-11 string literals were writable; a modern host puts them in
  `.rodata`, so `builtin[]` becomes `char [][64]` (`files.c`).

* **The `AS=as -` default.**  The `#ifdef vax` alternative was `"AS=as".` — a
  stray `.` that never compiled — so only the PDP-11 `"AS=as -"` branch
  survives (`files.c`).

* **`mkqlist(NULL)` returned garbage.**  A bare `return;` from a `char*`
  function; it now returns the empty string (`misc.c`).

* **`gram.y` for bison.**  V7's grammar uses `%term` (not `%token`), `= {` for
  actions, a mid-rule `%{…%}` block, and `%type` comma separators — all V7
  yacc-isms.  For bison these become `%token`, bare `{`, a prologue block, and
  space separators; the embedded lexer (`yylex`/`retsh`/`nextlin`) is
  C99-modernized but the grammar rules are unchanged, so the parser tables are
  identical to V7's (`gram.y`).

### 8.2 `yacc` — host adaptations

`yacc` is four C files + `dextern` + `files` + `yaccpar`.  Its defining
contract is **byte-identical output**: it must emit the same `y.tab.c` as V7
yacc, so `yaccpar` stays byte-for-byte V7's (K&R `yyparse()`) and the emission
logic is untouched; only yacc's *own* source is C99-modernized, output-neutral:

* **WORD32.**  The host has 32-bit `int`, so yacc uses 32-bit bit-packing; this
  only changes the internal lookahead-set layout, never the emitted tables
  (verified: all 12 V7 grammars byte-identical against the unmodified V7 yacc).

* **HUGE, not MEDIUM.**  V7 ships yacc `MEDIUM` (5200 words of storage), but
  `mip/cgram.y` needs 6461, so `MEDIUM` overflows.  `HUGE` (12000) is
  output-neutral for everything that fits and additionally handles `cgram.y`.

* **`error()` → varargs.**  V7's `error(s, a1)` was a K&R "generic arg" — one
  16-bit word holding either an `int` or a `char*`.  On a 64-bit host a `char*`
  does not fit in a word, so it becomes `void error(char *s, ...)` with
  `vfprintf` (`y1.c`).

* **`YACCPARSER`.**  yacc looks up `yaccpar` by a compiled-in path; the env
  var lets a build-tree yacc find its skeleton before it is installed (`y1.c`).

Note the division of labour this encodes: **`modern/yacc` parses ancient code**
— it is a reproducer you point at `cpy.y`/`gram.y` and diff against V7's
output.  The `modern/` **build** uses **bison** to regenerate those grammars as
modern C99; bison is not byte-identical, which is exactly why the ported yacc
still has a job.

### 8.3 What `knr2c99.py` does not do (the manual follow-ups)

The `orig → c99` step is `knr2c99.py --dialect v7` (§3.1), but a few things
are always left for the hand — some of which are now tool flags:

* **`#include`'d macros.**  Files whose macros live in an included header
  (yacc's looping `PLOOP`/`TLOOP`, make's `ALLOC`) need `--include-dir` so the
  CPP expands them; and `--define unix` so `#ifdef unix` blocks stay active.
  Both are now flags on `knr2c99.py`.

* **Cross-file prototypes.**  It only inserts *same-file* forward decls; a
  split program needs every cross-file function declared in its shared header
  (yacc's `dextern`, make's `defs`), replacing V7's few "strange type" decls.

* **`long int` word-size bug.**  It rewrites `long`→`int32_t` and
  `int`→`int16_t` independently, so `typedef long int TIMETYPE;` comes out
  `int32_t int16_t` — hand-fix to `int32_t`.

And the smaller ones: K&R functions with a comment in their header are skipped
(`metas`/`concat`/`srchdir`/`enbint`), `char *calloc();` clashes with
`stdlib.h`'s `void *calloc()`, and K&R "generic arg" calls (`error("x")` now
that `error` takes two args) need their arity padded.

### 8.4 `make`/`yacc` ported (2026-08-30) — extra moves and gotchas

Both tools now build in `modern/usr/src/cmd/{make,yacc}/` (autotools, `-std=c99`),
wire into `configure.ac`/`SUBDIRS`, and `make check` stays green.  Beyond the
moves in §8.1/§8.2, these came up during the actual port:

* **`SHELLCOM` → `shellcom` is not a header constant.**  `#define SHELLCOM
  "/bin/sh"` becomes `const char shellcom[]`, but a *definition* in the shared
  `defs` header gives every `.o` its own copy → multiple-definition at link.
  Declare it `static` in the one file that uses it (`dosys.c`), not in `defs`.

* **`-std=c99` hides POSIX decls.**  `dirfd`, `fork`, `wait`, `execl`, `execvp`
  are all undeclared under strict C99 (glibc's `__STRICT_ANSI__`).  `defs` starts
  with `#define _DEFAULT_SOURCE 1` before any include (the feature-test carve-out).

* **yacc `ACTNAME` → `actname` collides with a local.**  `setup()` declares its
  own `char actname[8];` (for the `$$%d` mid-rule nonterminal), which shadows the
  new global `const char *actname`.  Rename the local to `aname[8]`, else
  `fopen(actname, "w")` opens a garbage name and yacc dies with "cannot open temp
  file" before reading any grammar.

* **y4.c's `#define a amem` is not sed-safe.**  The bare `a` also occurs as the
  English article in comments and in the *error-string* literals `"a array
  overflow"` / `"clobber of a array"`.  A naive `s/\ba\b/amem/` corrupts those
  strings and changes yacc's output/errors.  Resolve the alias in code only (the
  other aliases `mem`/`pa`/`yypact`/`greed` are sed-safe, but note `yypact` also
  names the *output table* `arout("yypact", …)` — keep that string verbatim).

* **`enum` vs the sizes.**  yacc's `HUGE` sizes and `WORD32` word-packing are
  folded into `dextern` as an `enum` + `static inline` accessors (`bit`/`setbit`/
  `nwords`/`assoc`/`plevel`/`toktype`/…), and the `TLOOP`/`PLOOP`/`WSLOOP`/
  `ITMLOOP` loop macros are expanded inline (they are `for`-headers, so they
  cannot become functions).  `yaccpar` stays byte-for-byte V7's (K&R `yyparse`).

### 8.5 Why the macro habit at all — early Unix C as a Lisp

The `TLOOP`/`PLOOP`/`WSLOOP`/`BIT`/`ASSOC` layer that §8.2 dissolves is not a
porting artifact — it is verbatim V7.  Every one of those `#define`s is in
S. C. Johnson's `yacc` (1978), in the shared `dextern` header, carried forward
unchanged through `orig → c99` and only expanded away in `modern/`:

    #define TLOOP(i)   for(i=1;i<=ntokens;++i)
    #define NTLOOP(i)  for(i=0;i<=nnonter;++i)
    #define PLOOP(s,i) for(i=s;i<nprod;++i)
    #define WSLOOP(s,j) for(j=s;j<cwp;++j)
    #define ITMLOOP(i,p,q) q=pstate[i+1];for(p=pstate[i];p<q;++p)
    #define BIT(a,i)   ((a)[(i)>>5] & (1<<((i)&037)))
    #define ASSOC(i)   ((i)&03)
    #define PLEVEL(i)  (((i)>>4)&077)

The "Lispish" reading is exactly right: `#define TLOOP(i) for(…)` is a macro
that *manufactures a new loop construct* — the same shape as a Lisp `defmacro`
that emits a `do` form.  It is a small domain-specific language over C: named
iteration idioms (`TLOOP` = "for each token", `PLOOP` = "for each production")
and named bit-slicing accessors (`ASSOC`/`PLEVEL`/`TYPE`).

There were two concrete reasons, and only one of them is aesthetic:

* **Machine dependence — the real driver.**  On the PDP-11 `int` is 16 bits, on
  a VAX 32.  So `BIT`/`SETBIT`/`NWORDS` each have two bodies under
  `#ifdef WORD32`/`#else` (shift by 5 vs shift by 4), and `#ifdef HUGE`/
  `#ifdef MEDIUM` select a memory budget.  The macro layer was the portability
  shim that let one source tree compile for both word sizes.
* **Bit-packing as storage.**  `toklev[]` packs associativity (2 bits),
  precedence (5 bits) and type (6 bits) into a single 16-bit word — memory was
  precious — so `ASSOC`/`PLEVEL`/`TYPE` are the *names* for those bit-fields,
  doing by macro what a `struct` with bitfields does directly.

The loop constructors are the genuinely Lispish part: `TLOOP`/`PLOOP`/`WSLOOP`
exist purely so the algorithm reads "for each token / production / working-set
entry" instead of spelling the bounds every time — idiom-abstraction, not
necessity.

This was house style, not a one-off.  `make` has the same thing (`ALLOC(type)`,
`unequal`, `FSTATIC`), and the original `sh` is full of it.  In 1978 the
preprocessor was the only metaprogramming tool there was, and the Bell Labs
people came out of the Lisp/Macro tradition, so they reached for it.  The
no-`#define` rule for `modern/` is the 2026 counter-move against exactly this:
where Johnson wrote `TLOOP(i)` because a 16-bit-word abstraction had to be
written once, the modern port writes `for (i=1; i<=ntokens; ++i)` inline and a
real `static inline int bit(int *a, int i)` — type-checked, debuggable,
greppable.  Same behaviour, no macro layer.

### 8.6 `sh` — the Bourne shell (ported 2026-08-30)

`/bin/sh` is S. R. Bourne's shell: the last phase-1 command and the largest
single port in the project — 24 `.c` files, ~3300 lines, and a *macro dialect*
layered on top of C.  It is best understood as three stacked difficulties.

**1. The ALGOL control-flow layer (`mac.h`).**  Bourne wrote every `if`/`while`/
`for` through ~20 macros, ~1400 invocations across the tree:

    #define IF if(          #define THEN  ){
    #define ELIF } else if (  #define ELSE  } else {
    #define FI ;}            #define FOR   for(
    #define WHILE while(     #define DO    ){
    #define OD ;}            #define REP   do{
    #define PER }while(      #define DONE  );
    #define LOOP for(;;){    #define POOL  }
    #define SWITCH switch(   #define IN    ){
    #define ENDSW }          #define BEGIN {    #define END }
    #define LOCAL static     #define PROC  extern
    #define REG register     #define ANDF  &&    #define ORF ||
    #define NEQ ^            #define TRUE  (-1)  #define FALSE 0

So `IF x THEN y ELSE z FI` is `if( x ){ y } else { z ;}`.  `NEQ` as `^` is a
Bourne pun: `(a==0)NEQ(b)` is `(a==0)^(b)`, which is `!=` for boolean operands.

**2. The typedef system (`mode.h`).**  V7's C has no `void`, so the shell
defines its own type vocabulary through `TYPE`/`STRUCT`/`UNION` macros:

    TYPE char     CHAR;   TYPE char BOOL;   TYPE int  UFD;
    TYPE int      INT;    TYPE float REAL;  TYPE long int L_INT;
    TYPE int      VOID;   /* ← int, not void: V7 had no void */
    TYPE char    *STRING; TYPE char MSG[];
    STRUCT stat   STATBUF;  /* typedef struct stat STATBUF */
    STRUCT fileblk *FILE;   /* …and ~15 more struct-pointer typedefs */

`VOID` is `int` (a V7-ism); the modern port turns it into real `void`.

**3. The V7 syscall/header layer.**  `gtty`/`stty`/`ioctl(FIOCLEX)` (from
`<sgtty.h>`), `setbrk`/`sbrk` plus a stack allocator (`stak.h`), `<execargs.h>`
for the `ps` args, and the build is `cc -n -s -O` with the assembler prepending
`/usr/include/sys.s` (the syscall numbers).

**Pipeline.**  `knr2c99` runs first (orig → c99) and, as with yacc's `TLOOP`,
leaves the macros *unexpanded* in its output — it resolves them (via
`--include-dir`) only to parse.  So `main(c,v) INT c; STRING v[];` becomes
`int16_t main(INT c, STRING v[])`: the K&R params move into the signature, but
the typedef names and the `IF/THEN/FI` stay put.  (One warning so far:
`name.c:218 namscan: unhandled declarator 'Func'`, left unchanged.)  The modern
stage then does the remodelling: expand the control-flow macros to real
`if`/`while`/`for`, rewrite the headers as `typedef`/`enum`/real functions (the
no-`#define` rule), and adapt the V7 syscalls.

**What the modern stage actually had to change** (the "epic" part — each of
these is a distinct V7→host adaptation, in roughly the order they bit):

1. **`alloc`/`free`/`getenv`/`setenv`/`rename` collide with libc.**  The shell
   replaces `malloc`/`free` entirely with its own arena allocator (`#define
   alloc malloc`, and its own `free`), and defines its own `getenv`/`setenv`/
   `rename` (an *fd*-rename, `dup2`+`close`, not a path rename).  On a modern
   host each of these shadows a libc symbol, so they are renamed with a `_sh`
   suffix to stay recognisable — `alloc`→`alloc_sh` (which returns `void*`),
   `free`→`free_sh`, `getenv`→`getenv_sh`, `setenv`→`setenv_sh`,
   `rename`→`rename_sh` — and `<stdio.h>` (which drags in `FILE`/`tmpnam`) is
   dropped from the files that included it for no reason.

2. **Pointer-tagging cheats (`Lcheat`/`Rcheat`).**  V7 stored flag bits
   (`BUSY`, `ARGMK`) in the *low bit of a pointer* — the allocator ORs `|1`
   into a block pointer, the arg scanner does the same to `gchain`.  The
   original did this with two casts that only work when `int` == pointer:
   `Lcheat(a)=((a)._cheat)` (an int lvalue aliasing a pointer) and
   `Rcheat(a)=((int)(a))`.  On a 64-bit host `(int)` truncates, so both become
   `uintptr_t` puns: `Lcheat(a)=(*(uintptr_t*)(&(a)))`, `Rcheat(a)=((uintptr_t)(a))`.

3. **The arena base is the sbrk break, not a data object.**  `address end[]`
   was V7's *end-of-BSS* symbol — the base of the circular-fit arena.  Defining
   it as a normal `address end[1]` array puts it in the middle of `.bss`, so the
   allocator tramples other globals.  It becomes `char *end` initialised to
   `(char*)sbrk(0)` at the top of `main`, and `addblok` grows the brk itself
   (`setbrk(reqd)`) rather than relying on the SIGSEGV/MEMF trap to grow it
   on demand (a mechanism that does not survive glibc's `signal()` semantics).

4. **Implicit-union member access.**  The shell's AST is ~10 structs that all
   share a leading `{INT typ; IOPTR io;}` header.  V7 accessed `t->forktyp`,
   `t->swarg`, `t->comset` through a generic `TREPTR t` — legal in a compiler
   without struct member type-checking.  C99 rejects it, so each access gets a
   cast to the concrete node type (`((FORKPTR)t)->forktyp`, `((SWPTR)t)->swarg`,
   …); `makefork`/`makelist` declare `t` as the real type and cast on the
   `return`.

5. **Common symbols → one definition.**  Every V7 global was a tentative
   definition (`INT flags;`, `MSG notfound;`) merged by the linker as a common
   symbol.  gcc 10+ defaults to `-fno-common`, so `defs.h`/`stak.h` declare
   them all `extern` and a new `data.c` defines the ~35 that carry no
   initialiser (`flags`, `dolc`, `wdarg`, `peekc`, the two `jmp_buf`s, the stack
   pointers, …).  `environ` is deliberately *not* defined — it resolves to
   libc's own environment.

6. **Offsets stored in pointer variables.**  `relstak()` returns a byte offset
   (`int`) but callers held it in a `STRING`/`STKPTR` and fed it back to
   `absstak()`.  The variables that are *only* offsets become `int`; the one
   that is both (`argp` in `macro.c`) is cast through `intptr_t`.

7. **The readable null page, again** (§4.6).  `execute` called
   `syslook(com[0], commands)` *before* the `argn==0` test; for a pure
   assignment (`x=42`) `com[0]` is `ENDARGS`=0, and `syslook` does `*w`.  V7's
   address 0 was readable; Linux's is not, so the test is reordered
   `argn==0 || syslook(...)`.

8. **Syscall shape-shifters.**  `dup(fa|DUPFLG,fb)` (V7's two-arg dup) →
   `dup2`; `ioctl(fb,FIOCLEX,0)` → `fcntl(F_SETFD,FD_CLOEXEC)`;
   `gtty(fd,&sb)`/`stty` (from `<sgtty.h>`) → `isatty(fd)`; `signal(sig,1)`
   (1 was SIG_IGN) → `signal(sig,SIG_IGN)` with the old-handler probe cast
   through `intptr_t`; `times(long[4])` (V7) → POSIX `times(struct tms*)`; and
   the `char`/`short` parameters that K&R default-promoted had to be re-typed
   `int` or `CHAR` in the prototypes.

9. **Implicit-`int` "void" functions.**  V7 wrote ~40 helpers with no return
   type (`error`, `done`, `stdsigs`, `assign`, `copy`, `copyto`, …); `knr2c99`
   faithfully turned those into `int16_t`, so they came back as `int`.  They
   are all statement-called with the value never read, so the port declares
   them `void` — and the four that genuinely *return* a value only to fall
   through `failed()` (`chkopen`, `create`, `stoi`, `getpath`) stay `int`, with
   `failed`/`error`/`exitsh`/`done` marked `__attribute__((noreturn))` so the
   compiler stops warning about the unreachable fall-through.  The result is a
   `-Wall` build with no `-Wreturn-type` diagnostics left.

The result builds as `modern/usr/src/cmd/sh/sh` (a `v7unix_libexec_PROGRAMS`
in the autotools tree, `SUBDIRS` gains `sh`) and runs the classic smoke test —
`x=42; echo "x is $x"`, a `for … do … done` loop, backtick substitution, and a
pipe all come out byte-for-byte as V7's shell would have produced them.
