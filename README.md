# The V7 PDP-11 C toolchain

The Seventh Edition Unix C toolchain for the PDP-11, taken from the original
V7 sources and modernized to build on a modern host so it can *cross-compile*
PDP-11 code.  This is dmr's compiler: `cc` (the `c0`/`c1`/`c2` passes), `as`,
and `ld`, plus the `cc` driver.

It is an independent toolchain project — not tied to any particular OS or
emulation setup.

## Components

| dir       | what                                                            |
|-----------|-----------------------------------------------------------------|
| `cc/`     | the C compiler — `c00`-`c05` (first pass / parser), `c10`-`c13` (code generator), `c20`/`c21` (peephole), `table.s` (machine description) |
| `as/`     | the PDP-11 assembler, originally self-hosted assembly (`as11.s`..`as29.s`), translated to C (`as1.c`/`as2.c`) |
| `ld/`     | the PDP-11 linker (`ld.c`)                                      |
| `cc.c`    | the `cc` driver (cpp → c0 → c1 → c2 → as → ld)                  |
| `include/`| headers                                                         |

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
make -C cc/host      # -> cc/host/c1  (code generator)
make -C cc/host/c0   # -> cc/host/c0/c0  (parser)
```

End-to-end: `cc/host/c0/c0 src.c t1 t2` emits the intermediate tree; then
`cc/host/c1 t1 t2 out.s` emits PDP-11 assembly.

## Notes on the port

The port is documented in the memory notes and in the commit history of the
toolchain's parent repo.  Highlights: V7's "member-access-by-name" (a `struct
tnode *` could reach any member) required collapsing the node structs into a
tagged union; `table.s` was reverse-engineered to `table.c` (`table2c.py`);
the assembler was translated from self-hosted PDP-11 assembly to C; and a
handful of K&R-isms (juxtaposed initializers, `=op` compound assignment, bare
`return` leaving `r0` untouched) were resolved to their C99 forms.
