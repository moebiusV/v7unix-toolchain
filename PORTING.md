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

### 4.4 Generic registers — one variable, two types

The code reuses a single register variable as `int` in one arm and a pointer in
another (`build()`'s `t1`, `getree()`'s `t`, `unoptim()`'s `p`), which worked
when both were 16 bits and is a type error when a pointer is 64.  Each such
register had to be split into two typed variables.

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
