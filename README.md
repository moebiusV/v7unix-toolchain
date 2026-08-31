# The V7 PDP-11 C toolchain

This project is two things.

First, a port of the Seventh Edition Unix C toolchain (dmr's `cc` driver, the
`cpp` preprocessor, the `c0`/`c1`/`c2` passes (`c2` the peephole optimizer),
the `as` assembler, and the `ld` linker) to a modern Unix host.  It
cross-compiles ancient Unix source code, in the original C dialect, into
PDP-11 binaries that run on ancient Unix.

Second, a separate project in `c99/`: the original Unix source, modernized
just enough (K&R C → C99) that the modern `pcc` compiler can compile it into
binaries that still run properly on ancient Unix.  `c99/` is the staging area
on the way to the full `modern/` port (every program passes through it first),
and on its own terms it doubles as a stress test for pcc's PDP-11 support.

## Directory layout

The tree is three stages of the same sources, mirroring V7's `cmd/` layout
(`cc`, `cpp`, `as`, `ld`, plus `adb`, `make`, `yacc` and `lex`):

| dir       | what                                                                  |
|-----------|-----------------------------------------------------------------------|
| `orig/`   | the original V7 sources, unmodified, with V7's own makefiles           |
| `c99/`    | `orig/` run through `knr2c99.py`, just enough for pcc to compile it for the PDP-11 |
| `modern/` | the port of `c99/` to run on a 64-bit host (union-node + host fixes), still emitting PDP-11 code |
| `tools/`  | the porting scripts (`union-node.py`, `build-host.py`, `c0-host.py`, `table2c.py`, rules/specs) |

`orig → knr2c99 → c99 → union-node + host-fixes → modern`.  `make` and `yacc` are in `orig/` but not yet ported to
`c99/`/`modern/`; `lex` is not needed here.  `adb` is
target-resident: it will only ever be modernized to `c99/` (for pcc to
cross-compile it), never ported to `modern/`.  `mkfs` and `fsck` will be
ported to `modern/` here; filsys depends on this repo for them.

Layout invariant: `c99/` mirrors `orig/` exactly, differing only in the `.c` /
`.h` files (the C99 modernization for pcc); `modern/` may additionally
modernize makefiles and lex/yacc inputs (the full host port).

## Building and installing

The host tools build with the standard GNU autotools flow:

    ./configure && make && make check && make install

`configure` is shipped, so autoconf/automake are not needed to build (only to
regenerate from `configure.ac`/`Makefile.am`).  Build dependencies are a C99
compiler (`gcc`, `clang`, or `pcc`), `bison`, and `python3`.

Install layout (default prefix `/usr/local`; every directory configurable via
the standard `./configure` flags):

    $(libexecdir)/v7unix/   the 8 host binaries (cc, cpp, c0, c1, c2, as, as2,
                            ld) under their original V7 names
    $(bindir)/              v7cc, v7as, v7ld (symlinks to the above)
    $(libdir)/v7unix/       the PDP-11 target runtime (crt0.o, libc.a)
    $(mandir)/v7unix/man1/  the V7 manual pages (cc.1, as.1, ld.1, lex.1,
                            yacc.1, make.1, adb.1)
    $(mandir)/man1/         v7cc.1, v7as.1, v7ld.1 (.so stubs into v7unix)

The default prefix is `/usr/local` (the BSD convention); Linux distros pass
`--prefix=/usr`.  Cross-compile a PDP-11 program with:

    v7cc hello.c -o hello

## Packaging

Template packaging files for the major distributions ship in `dist/` (rpm,
debian, arch, alpine, freebsd, openbsd, netbsd, gentoo, nixos, slackware); the
packager-facing guide is `README.distributions`.

## Notes on the port

The full account is [`PORTING.md`](PORTING.md), and it is worth reading for its
own sake: it is as much archaeology as porting log.  Dragging 1979 code,
unchanged, onto a 64-bit machine turns up things no manual records, and
`PORTING.md` pins each one down.  A few of them:

-   **the readable null page** (§4.6) — the PDP-11 had no memory protection,
    so V7 code freely dereferences NULL and reads garbage, betting on "garbage
    ≠ what I'm looking for" to skip a branch; on a modern host every one of
    those reads is a SIGSEGV, and the port hit them one source file at a time.
-   **the `delay()` brace bug** (§4.2) — a real control-flow bug, shipped for
    years, where the indentation says what dmr meant and the braces do the
    opposite.
-   **middle-endian word order** (§4.3) — the PDP-11 stored 32-bit values as
    *high word first*, each word little-endian; "portable" code that reads a
    long as four bytes in order silently breaks.
-   **the implicit union** (§2.4, §6.3) — C had no `union`, so dmr's tree
    nodes use "member-access-by-name": one `struct tnode *` legally reads any
    field of any node shape, because the 16-bit layout happens to overlap
    them.
-   **`return` as a tail call** (§2.3) — a bare `return` leaves `r0` alone,
    so a function's result falls through into the next; the assembler leans on
    this hard (§6.1).
-   **early Unix C as a Lisp** (§8.5) — the code is lousy with macros for the
    same reason Lisp is: the language kept inviting you to build new syntax.

`PORTING.md` is meant to be read front to back: §2 the C dialect K&R allowed
and C99 took away, §3 the mechanical-then-manual port, §4 the obstacles no
tutorial mentions, §6 the choices in dmr's code worth noticing, §8 the
build-tool ports.  Read it to see why the toolchain is shaped as it is — or
for a tour of 1979 C under a modern compiler's x-ray.
