# The V7 PDP-11 C toolchain

The Seventh Edition Unix C toolchain for the PDP-11, taken from the original
V7 sources and modernized to build on a modern host so it can *cross-compile*
PDP-11 code.  This is dmr's compiler: `cc` (the `c0`/`c1`/`c2` passes), `as`,
and `ld`, plus the `cc` driver.

It is an independent toolchain project — not tied to any particular OS or
emulation setup.

## Directory layout

The tree is three stages of the same sources, mirroring V7's `cmd/` layout
(`cc`, `cpp`, `as`, `ld`, plus `adb`, `make`, `yacc` and `lex`):

| dir       | what                                                                  |
|-----------|-----------------------------------------------------------------------|
| `orig/`   | the original V7 sources, unmodified, with V7's own makefiles           |
| `c99/`    | `orig/` run through `knr2c99.py` — just enough for pcc to compile it for the PDP-11 |
| `modern/` | the port of `c99/` to run on a 64-bit host (union-node + host fixes), still emitting PDP-11 code |
| `tools/`  | the porting scripts (`union-node.py`, `build-host.py`, `c0-host.py`, `table2c.py`, rules/specs) |

`orig → knr2c99 → c99 → union-node + host-fixes → modern`.  `adb`, `make`,
`yacc` and `lex` are in `orig/` but not yet ported to `c99/`/`modern/`.

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
    $(bindir)/              v7cc, v7as, v7ld — symlinks to the above
    $(libdir)/v7unix/       the PDP-11 target runtime (crt0.o, libc.a)
    $(mandir)/v7unix/man1/  the V7 manual pages (cc.1, as.1, ld.1, lex.1,
                            yacc.1, make.1, adb.1)
    $(mandir)/man1/         v7cc.1, v7as.1, v7ld.1 — .so stubs into v7unix

The default prefix is `/usr/local` (the BSD convention); Linux distros pass
`--prefix=/usr`.  Cross-compile a PDP-11 program with:

    v7cc hello.c -o hello

## Packaging

Template packaging files for the major distributions ship in `dist/` (rpm,
debian, arch, alpine, freebsd, openbsd, netbsd, gentoo, nixos, slackware); the
packager-facing guide is `README.distributions`.

## Notes on the port

The port is documented in `PORTING.md`.  Highlights: V7's
"member-access-by-name" (a `struct tnode *` could reach any member) required
collapsing the node structs into a tagged union; `table.s` was
reverse-engineered to `table.c` (`table2c.py`); the assembler was translated
from self-hosted PDP-11 assembly to C; and a handful of K&R-isms (juxtaposed
initializers, `=op` compound assignment, bare `return` leaving `r0` untouched)
were resolved to their C99 forms.
