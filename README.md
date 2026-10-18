# The V7 PDP-11 C toolchain

This project is two things.

First, a port of the Seventh Edition Unix C toolchain (dmr's `cc` driver, the
`cpp` preprocessor, the `c0`/`c1`/`c2` passes (`c2` the peephole optimizer),
the `as` assembler, and the `ld` linker) to a modern Unix host.  It
cross-compiles ancient Unix source code, in the original C dialect, into
PDP-11 binaries that run on ancient Unix.

Second, a separate project in `pcc99/`: the original Unix source, modernized
just enough (K&R C → C99) that the modern `pcc` compiler can compile it into
binaries that still run properly on ancient Unix.  `pcc99/` is the staging area
on the way to the full `modern/` port (every program passes through it first),
and on its own terms it doubles as a stress test for pcc's PDP-11 support.

## Directory layout

The tree is three stages of the same sources, mirroring V7's `cmd/` layout
(`cc`, `cpp`, `as`, `ld`, plus `adb`, `make`, `yacc` and `lex`):

| dir       | what                                                                  |
|-----------|-----------------------------------------------------------------------|
| `orig/`   | the original V7 sources, unmodified, with V7's own makefiles           |
| `pcc99/`    | `orig/` run through `knr2c99.py`, just enough for pcc to compile it for the PDP-11 |
| `modern/` | the port of `pcc99/` to run on a 64-bit host (union-node + host fixes), still emitting PDP-11 code |
| `tools/`  | the porting scripts (`union-node.py`, `build-host.py`, `c0-host.py`, `table2c.py`, rules/specs) |

`orig → knr2c99 → pcc99 → union-node + host-fixes → modern`.  `make` and `yacc` are in `orig/` but not yet ported to
`pcc99/`/`modern/`; `lex` is not needed here.  `adb` is
target-resident: it will only ever be modernized to `pcc99/` (for pcc to
cross-compile it), never ported to `modern/`.  `mkfs` and `fsck` will be
ported to `modern/` here; filsys depends on this repo for them.

Layout invariant: `pcc99/` mirrors `orig/` exactly, differing only in the `.c` /
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

## The self-hosting root (froot1, froot2)

`make froot1` assembles `froot1/`, the minimal self-hosting cross-compilation
root: the ported toolchain plus the phase-1 makefile commands
(sh/cp/mv/rm/cmp), the V7 headers, and the reference source tree — enough to
rebuild the toolchain subset (cc/as/ld and friends) from `orig/` using only
the pieces inside `froot1/`.  `tools/selfhost.sh` runs that verification;
`make froot1-dist` packs it as `froot1.tar.gz`.  `froot2/` — froot1 plus
whatever else it takes to build the whole `/usr/src` tree — is the next
milestone.

## Packaging

Template packaging files for the major distributions ship in `dist/` (rpm,
debian, arch, alpine, freebsd, openbsd, netbsd, gentoo, nixos, slackware); the
packager-facing guide is `README.distributions`.

## Code quality: pcc against the original compilers

The `pcc` this project compiles with (a fork, `moebiusV/pcc`) is also being
taught the code-generation tricks of the compilers it replaces, so that its
output is no bigger than theirs.  The work goes in two steps.

1. **Bring the old targets up to the original standard.**  On the PDP-11 pcc is
   measured against dmr's `cc`, and on the VAX (target `vax-unknown-unix32v`,
   run on a real 32V under SIMH) against 32V's `cc -O`, which is Steve Johnson's
   original PCC (the ancestor of the pcc used here) with the VAX `c2` peephole
   optimizer.  All of it is opt-in:
   `-xdmr` (implied by `-O` on the PDP-11), plus `-xtemps -xdeljumps -xdce
   -xinline`.  Everything else in pcc is unchanged.  The tricks that lived in the
   old peephole optimizers (`c2`) are being moved into pcc's own machinery:
   into the IR passes in `mip/optim2.c` where they only concern control flow and
   values (loop rotation, cross-jumping), and into the pass-2 templates where
   they choose instructions (autoincrement operands, `inc`/`dec`, read-modify-write,
   `$name` operands).  `c2` itself now runs inside `ccom` (`-xc2`).
2. **Port whatever has an analogue to the modern targets** (i386, amd64), through
   the same IR passes and templates: `testl` instead of `cmpl $0`, byte
   read-modify-write, `sym(,%reg,4)` indexing, frameless leaf functions, loop
   rotation, `leal` for `&a[i]`.
3. **aarch64.**  The old aarch64 backend was a 32-bit ARM backend with its registers
   renamed and did not work; it was replaced by an AAPCS64 LP64 backend modelled on
   amd64 ([`AARCH64.md`](AARCH64.md)), checked on an emulated AArch64 (unicorn and
   keystone, no hardware needed).  `-xdmr` there is a text optimizer that turns the
   dmr-cc ideas into AArch64: post-index addressing for `*p++`, `cbz`/`tbz` for tests
   against zero and bits, extended and scaled index operands, `cset`, paired register
   saves.  The V7 libc comes to 28756 bytes, 11% under `clang -Os` on the files both can
   compile.  Writing it turned up, with a random-program fuzzer
   (`tools/exprfuzz.py`, host gcc as the oracle), seven bugs in code every target shares.

The result, in text+data bytes: PDP-11 libc 3.0% and commands 4.0% smaller than dmr's `cc`;
VAX libc 16.7% and commands 10.3% smaller than 32V's `cc -O`.  The only files still larger than
the original's are larger for a reason of correctness (the originals divide unsigned values
as signed and read some `long` constants wrongly; pcc does not) or are listed as open items.
`long long` is a real 64-bit type on the PDP-11 (it was 32 bits) and has its helpers on the VAX, which
32V's libc lacks.

Every change is checked on the real system, not just by looking at assembly: 332
bare-metal PDP-11 test runs, 249 VAX runs on a real 32V in several flag sets,
34 V7 commands built by pcc and run inside V7 beside the originals' builds, pcc
compiling itself (identical stages), and the i386 and amd64 suites.  The tools
are in `tools/`.

Where to read more:

* [`OPTIMIZATION-TRICKS.md`](OPTIMIZATION-TRICKS.md) -- each trick, how it was
  ported, the size tables, the bugs found in the originals and in upstream pcc.
* [`BIGGER-THAN-ORIGINAL.md`](BIGGER-THAN-ORIGINAL.md) -- the files that are still
  larger than the original compiler's and why, and what cleared the rest.
* [`OLD-K&R-FORMS.md`](OLD-K&R-FORMS.md) -- the old C forms in the V7 and 32V sources
  that pcc rejects (28 of 80 V7 commands, 18 of 605 32V sources): what they are, where they
  are used, what accepting them would take, and what it would break for C99.
* [`AARCH64.md`](AARCH64.md) -- the aarch64 backend: conventions, the text optimizer,
  the emulator harness, the fuzzer, and what running it found.
* [`DMR-OPTIMIZATIONS-OTHER-TARGETS.md`](DMR-OPTIMIZATIONS-OTHER-TARGETS.md) --
  the first survey of which tricks carry over to VAX and x86.
* [`PCC-OPTIMIZER.md`](PCC-OPTIMIZER.md) -- the original plan (older than the
  work above and partly overtaken by it).

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
