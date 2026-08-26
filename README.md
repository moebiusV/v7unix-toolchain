# The V7 PDP-11 C toolchain

The Seventh Edition Unix C toolchain for the PDP-11, taken from the original
V7 sources and modernized to build on a modern host so it can *cross-compile*
PDP-11 code.  This is dmr's compiler: `cc` (the `c0`/`c1`/`c2` passes), `as`,
and `ld`, plus the `cc` driver.

It is an independent toolchain project — not tied to any particular OS or
emulation setup.

## Directory layout

The tree is three stages of the same sources, mirroring V7's `cmd/` layout
(`cc`, `cpp`, `as`, `ld`, plus `adb` and `make`):

| dir      | what                                                                 |
|----------|----------------------------------------------------------------------|
| `orig/`  | the original V7 sources, unmodified, with V7's own makefiles          |
| `c99/`   | a copy of `orig/` run through `knr2c99.py` — just enough for pcc to compile it for the PDP-11 |
| `modern/`| the port of `c99/` to run on a 64-bit host (union-node + host fixes), still emitting PDP-11 code |
| `tools/` | the porting scripts (`knr2c99.py` glue, `union-node.py`, `build-host.py`, `c0-host.py`, `table2c.py`, rules/specs) |

`orig → knr2c99 → c99 → union-node + host-fixes → modern`.  `adb` and `make`
are fetched into `orig/` and `c99/` but not yet ported to `modern/`.

## Building the host tools

The K&R sources are ported to C99 with a three-step pipeline (per file):

```
knr2c99.py --dialect v7 --rules <X>.rules.json   # K&R -> C99
union-node.py --spec <X>.spec.json               # node structs -> union node
build-host.py / c0-host.py                       # host-specific fixes
```

The generated, host-compilable output is committed under `cc/host/` and
`cc/host/c0/`, so a plain `make` builds the binaries:

```
make -C modern/cc/c1  # -> c1  (code generator)
make -C modern/cc/c0  # -> c0  (parser)
```

End-to-end: `modern/cc/c0/c0 src.c t1 t2` emits the intermediate tree; then
`modern/cc/c1/c1 t1 t2 out.s` emits PDP-11 assembly.

## Notes on the port

The port is documented in the memory notes and in the commit history of the
toolchain's parent repo.  Highlights: V7's "member-access-by-name" (a `struct
tnode *` could reach any member) required collapsing the node structs into a
tagged union; `table.s` was reverse-engineered to `table.c` (`table2c.py`);
the assembler was translated from self-hosted PDP-11 assembly to C; and a
handful of K&R-isms (juxtaposed initializers, `=op` compound assignment, bare
`return` leaving `r0` untouched) were resolved to their C99 forms.
