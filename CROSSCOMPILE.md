# Cross-Compilation Environment

Status: **persisted plan, nothing here is implemented yet.** This documents the
environment strategy for cross-compiling Research Unix source on a modern host:
the `v7env` PATH wrapper, and a full isolation chamber (proot / jail / chroot)
so that historical build scripts invoking binaries by absolute path (`/bin/cc`,
`/lib/cpp`) resolve to the ported toolchain instead of the host's amd64 tools.

## 1. The PATH insight

All host binaries are installed in `$(libexecdir)/v7unix/` under their original
names (`cc`, `cpp`, `c0`, `c1`, `c2`, `as`, `as2`, `ld`), with `v7`-prefixed
symlinks (`v7cc`, `v7as`, `v7ld`) in `$(bindir)` as a convenience. That install
shape makes the original names addressable, which is what turns a plain PATH
prefix into an immersive build environment:

    PATH=$(libexecdir)/v7unix:$PATH

This gives two usage styles from one layout:

- **Explicit** - `v7cc`, `v7as`, `v7ld` from `$(bindir)` for one-off PDP-11
  builds while working on normal modern projects.
- **Immersive** - prepend the libexec dir so bare `cc`/`as`/`ld` resolve to the
  cross tools, letting vintage invocations run unmodified.

Important limit, stated plainly: the tools are *cross*-compilers (host amd64
binaries that emit PDP-11 assembly and link host libc), not PDP-11 emulation.
The immersive mode gives a V7-flavored *build* environment, not a V7 userspace.
Today it covers the compiler/assembler/linker; `make`, `yacc`, `sh` and the
command utilities (`cp`/`rm`/`cmp`/`mv`/`echo`/…) are ported next (phase 1,
`modern/` — see §7), so the immersive story for whole-tree builds is still
incomplete.

## 2. `v7env` (build this first)

A thin wrapper, roughly ten lines, zero new dependencies. It runs a command (or
a subshell) with the libexec dir prepended to PATH and the `V7_*` overrides
exported. Two design points that matter:

- **Subshell/command wrapper, not a mutating exporter.** `v7env make` or
  `v7env sh`, matching the `env`/`schroot`/`nix develop` idiom. A login shell
  whose PATH has the cross `cc`/`as`/`ld` shadowing the host ones would break
  every normal host build, so opt in per command.
- **PATH only.** No chroot, no proot. Its virtue is that it does nothing but
  arrange PATH and env.

    v7env make            # bare `make`/`cc`/`as`/`ld` now hit the cross tools
    v7env sh -c 'cc -S x.c'

## 3. The absolute-path problem

V7 Makefiles and command files do not rely on PATH. They invoke binaries by
absolute path: `/bin/cc`, `/bin/as`, `/usr/bin/make`, `/lib/cpp`, `/lib/c0`,
`/lib/c1`, `/lib/c2`, `/lib/as2`. A PATH prefix does not intercept those, so a
naive immersive shell still leaks absolute calls out to the host's modern
amd64 `cc`/`as`/`ld` and the build fails. This is the hard hurdle, and the
reason the isolation chamber below is a definite deliverable rather than an
optional extra.

## 4. The isolation chamber (proot / jail / chroot)

A synthetic "V7 root" that maps the historical absolute paths onto the host
install. The full mapping, correcting the common mistake of only mapping
`/bin` and `/usr/bin` (the `cc` driver execs its passes by *compiled-in*
absolute paths, so `/lib` and `/usr/lib` must be mapped too):

| V7 path        | role                   | host target (default `/usr/local`) |
|----------------|------------------------|------------------------------------|
| `/bin/cc`      | C driver               | `libexec/v7unix/cc`                |
| `/bin/as`      | assembler              | `libexec/v7unix/as`                |
| `/bin/ld`      | loader                 | `libexec/v7unix/ld`                |
| `/lib/cpp`     | preprocessor           | `libexec/v7unix/cpp`               |
| `/lib/c0`      | pass 1                 | `libexec/v7unix/c0`                |
| `/lib/c1`      | pass 2                 | `libexec/v7unix/c1`                |
| `/lib/c2`      | peephole optimizer     | `libexec/v7unix/c2`                |
| `/lib/as2`     | assembler pass 2       | `libexec/v7unix/as2`               |
| `/lib/crt0.o`  | PDP-11 C runtime       | `lib/v7unix/crt0.o`                |
| `/lib/libc.a`  | PDP-11 libc            | `lib/v7unix/libc.a`                |
| `/usr/include` | V7 headers (once extracted) | `include/v7unix`              |
| `/bin/make`    | build driver (once ported) | `libexec/v7unix/make`          |
| `/bin/sh`      | shell (TBD, see §5)    | `libexec/v7unix/sh`                |

Three pieces, split by the problem each solves:

**`fakeroot` (Debian) - the primary wrapper.** fakeroot preloads `libfakeroot`
so `chown`/`chmod`/`mknod`/`chroot` appear to succeed while running unprivileged.
That is exactly the chamber's privileged half: `mkfs` of a V7 image (device
nodes, ownership, and mode bits inside the PDP-11 filesystem), `ar`/`cpio`
building archives that preserve ownership, and the `.deb`/`.rpm` packaging runs.
For the preload to hook a binary, the binary must be **dynamically linked**: a
static binary ignores `LD_PRELOAD`, so fakeroot cannot fake its syscalls.
Compile the toolchain's `mkfs`/`ar`/`cpio` (and the rest) dynamically, which is
the default. fakeroot is Linux/glibc-only (FreeBSD has an incomplete port;
OpenBSD/NetBSD have none). On the BSDs the packaging fake-root is their native
mechanism: ports staging (FreeBSD) and pkgsrc `FAKE_ROOT` (NetBSD), not syscall
interception; the privileged `mkfs`/`chroot` steps fall back to real root or a
`jail`.

**Path redirection - the driver's compiled-in paths, plus a `chroot` where
needed.** fakeroot does not redirect file paths (and its `chroot` is faked), so
it cannot turn `/bin/cc` into the cross tool by itself. That part is already
mostly solved: the `cc` driver execs its passes by compiled-in absolute paths
(`V7_LIBEXECDIR`), so only the rare script that literally invokes `/bin/cc` or
`/usr/bin/make` by absolute path needs a synthetic root. For those, a `chroot`
root (or `jail` on FreeBSD, `mount_nullfs` binds on the BSDs) maps the table
above; a no-root fallback is `proot -b` with the same bind set.

## 5. What the full-tree build actually needs

Verified against `unixtree/V7/usr/src` (42 makefiles). External commands the
build invokes, with makefile counts:

`cc`(157) `cp`(109) `rm`(102) `cmp`(93) `as`(39) `make`(31) `mv`(29)
`echo`(23) `yacc`(17) `lex`(16) `cat`(13) `ar`(13) `tp`(12) `sed`(12)
`tar`(11) `size`(8) `ld`(8) `touch`(7) `install`(7) `pr`(6) `diff`(6)
`strip`(5) `mkdir`(3) `grep`(3) `chown`(3) `chmod`(3) `ls`(2) `du`(2) —
plus `sh` (every recipe runs under `/bin/sh`).

- **`make` - required.** The build driver.
- **`yacc` - required.** 12 grammars: awk, bc, egrep, expr, eqn, neqn, struct,
  m4, mip, cpp, make, lex.
- **`lex` - required.** 2 scanners: awk (`awk.lx.l`) and struct (`lextab.l`).
- **`ar` - required.** 13 uses (libc.a, libm.a, libplot.a, libdbm.a, ...).
- **`sh` - required.** The shell that runs every recipe.
- **`cpio` - not used.** V7 archives with `tar` (11) and `tp` (12), never cpio.

Decision: port the whole build toolset with original V7 behavior: `make`,
`yacc`, `lex`, `ar`, `sh`, the `cc`/`as`/`ld` passes (done), and the utilities
`cp`, `rm`, `cmp`, `mv`, `echo`, `cat`, `sed`, `tar`, `tp`, `size`, `touch`,
`install`, `pr`, `diff`, `strip`, `mkdir`, `grep`, `chown`, `chmod`, `ls`, `du`.

## 6. `binfmt_misc`: rejected

Registering the ported shell with the kernel's binfmt_misc is rejected on three
grounds: it is global magic-byte matching that cannot be scoped to a build tree
and would hijack every text file system-wide; it targets the wrong layer; and
the real gap is not an exec format but the shebang convention (V7 scripts begin
with a bare `#` marking an sh script, versus the modern `#!`). That gap is fixed
by translating the first line or invoking through a wrapper, not by kernel magic.

## 7. Sequencing

The port lives in **`modern/`** (phase 1: everything the *toolchain* build
needs) and **`modern2/`** (phase 2: whole-tree only). The split rule is: if a
command is needed by phase 1 it goes in `modern/`, not `modern2/`.

- **`modern/` (phase 1, compile the toolchain)** — `cc` `cpp` `c0` `c1` `c2`
  `as` `as2` `ld` (all done) **plus** `make` `yacc` `sh` `mv` `cp` `rm` `cmp`
  (pending).  The toolchain build genuinely uses them: `sh` (globs
  `as1?.s`/`y?.o`, redirection `cvopt <table.s >table.i`), `mv` (implicit
  `.y.c` rule `mv y.tab.c $@`), `rm` (c/makefile `rm table.i`), `cp`/`cmp`
  (install/verify targets).
- **`modern2/` (phase 2, whole tree)** — `lex` `ar` `echo` `cat` `touch`
  `mkdir` `grep` `chmod` `chown` `ls` `du` `sed` `tar` `tp` `size` `install`
  `pr` `diff` `strip`.

Sequencing:

1. **`v7env`** - ship it (PATH + env, no deps); document explicit vs immersive.
2. **Phase 1 pure C** - `make`, `yacc`, `sh`, `mv`, `cp`, `rm`, `cmp` (the
   `cc`/`as`/`ld` passes are done).
3. **Phase 2 pure C** - `lex` `ar` `echo` `cat` `touch` `mkdir` `grep`
   `chmod` `chown` `ls` `du` `sed` `tar` `tp` `size` `install` `pr` `diff`
   `strip`, with original V7 behavior.
4. **Whole tree, assembly** - `factor`, `primes`, `bas`, `roff`, `standalone`.
5. **Isolation / test harness** - `tools/v7check.sh` (built) runs the original
   makefiles + source inside a synthetic V7 root; see §9.
6. **`v7crosscompile`** - the full-tree wrapper; first milestone is building one
   V7 tool from `orig/` source, not `make`-ing the whole userland, because the
   `c99/` staging area exists precisely because the source does not compile
   as-is.

## 8. Open questions

- Which layer does the full-tree build target: original V7 makefiles or the
  modernized `c99/`/`modern/` trees? (Determines whether `sh` is required.)
- How large is the `sh` port in practice (each builtin is a separate C file);
  is a reduced-feature `sh` acceptable for the build, or must it be full V7 sh?
- Persist the chamber as a per-invocation temp root or a permanent jail?
- Confirm fakeroot's dynamic-linking requirement across the distros (any tool
  that must be faked by `libfakeroot` cannot be static); does any target tool
  need static linking for a different reason?

## 9. Test harness (`tools/v7check.sh`) — built

`tools/v7check.sh` runs the *original* makefiles + source against the *ported*
`modern/` tools inside a synthetic V7 root. It:

1. Gunzips V7 headers from `unixtree/V7/usr/include` into a staging
   `/usr/include` (with `sys/`).
2. Sets the `cc` driver's `V7_*` overrides (`V7_C0/C1/C2/CPP/AS/LD/CRT0/LIB`)
   to the built `modern/` passes + target runtime, and prepends
   `modern/usr/src/cmd` + `modern/usr/src/cmd/as` to `PATH` (so bare
   `cc`/`as`/`ld`/`make`/`yacc` in the makefiles resolve to the ports).
3. Runs under `unshare -r -m` (Linux user + mount namespace), bind-mounting the
   synthetic V7 headers onto `/usr/include` for the duration — because cpp
   hardcodes `/usr/include` for `<stdio.h>`, and `as/makefile` reads
   `/usr/include/sys.s` literally.

### fakeroot vs `unshare -r -m`

The original plan (§4) named fakeroot primary. Testing on this host found the
two do **not** compose:

- `fakeroot unshare -r -m` → `unshare: write failed /proc/self/uid_map:
  Operation not permitted` (libfakeroot intercepts the uid_map write).
- `unshare -r -m -- fakeroot` → `chown` fails with `EINVAL` on unmapped ids.

`unshare -r -m` alone gives real uid-0 inside the namespace — enough for bind
mounts and chmod/mknod — without root, and it *redirects paths* (which
fakeroot cannot: its `chroot` is faked). For a compile-only build we need path
redirection, not chown-to-arbitrary-uid, so the harness uses `unshare -r -m`.
fakeroot remains the fallback for *privilege-only* steps (mkfs device nodes,
ar archive ownership) on hosts where unprivileged user namespaces are disabled.

### Status / blocker

The harness works end-to-end (stages headers, binds `/usr/include`, drives the
real `cc`). It surfaced, and drove to fix, the port bugs that blocked the
toolchain-from-`orig` build: the c0/c1 "readable null page" crashes
(`disarray(NULL)` in `build()`, `chkleaf()`'s uninitialised `tr2`), the
assembler's `section()` opcode-pop leak, and the c1 codegen NUL/octal-sign
drift.  With those fixed, `v7check cc -c cpp.c` produces `cpp.o` byte-identical
to V7's own assembler.  See PORTING.md §4.

