#!/usr/bin/env python3
"""
union-node.py -- refactor a V7 tree-node struct family into one `union node`.

V7 C kept one global member-name space: a `struct tnode *` could reach `value`
(a `struct cnode` member), `lvalue`, `cstr`, `tr1`, `tr2`, and so on, because
the compiler resolved the member *name* to an offset regardless of the pointer's
declared type.  C99 forbids that.  This script (run on knr2c99's modernized
output) collapses the node structs into one `struct node` with a shared prefix
and a union `u`, and qualifies every divergent member access with its sub-struct.

The struct family is passed as `--spec` (a JSON file):

  { "struct_names": ["tnode","cnode","lnode","fnode"],
    "member_map":  {"value":"cnode","lvalue":"lnode","cstr":"fnode",
                    "tr1":"tnode","tr2":"tnode"},
    "inits":       [["struct tnode funcblk { NAME, 0, NULL, NULL, NULL, NULL };",
                     "struct node funcblk = { NAME, 0, NULL, NULL, { .tnode = { NULL, NULL } } };"]],
    "intx": false }

`member_map` names the divergent members and their sub-struct; members present
in every struct (typically op/type and any shared pointers) are left top-level
in `struct node`.  `inits` is a list of [old, new] explicit initializer rewrites
for the struct literals that need re-shaping into designated initializers.
`intx` (if true) handles the c1 `->lvalue.intx[i]` / `->fvalue.intx[i]`
type-punning by turning it into a `((int16_t *)&...)[i]` cast.

Usage:  python3 union-node.py --spec c0.spec.json file.c > file.node.c
"""

import json
import re
import sys


def _add_intx(src):
    """c1-specific: `x->lvalue.intx[i]` -> `((int16_t *)&x->u.lconst.lvalue)[i]`."""
    repls = []
    def ph(m):
        base, kind, idx = m.group(1), m.group(2), m.group(3)
        sub = 'lconst' if kind == 'lvalue' else 'ftconst'
        repls.append('((int16_t *)&%s->u.%s.%s)[%s]' % (base, sub, kind, idx))
        return '\x01INTX%d\x01' % (len(repls) - 1)
    src = re.sub(r'(\w+)->(lvalue|fvalue)\.intx\[(\w+)\]', ph, src)
    return src, repls


def rewrite(src, spec):
    # 1. struct name -> struct node (first, so `&hreg` etc. are already node*)
    for s in spec['struct_names']:
        src = re.sub(r'\bstruct\s+' + s + r'\b', 'struct node', src)

    # 2a. generic struct-literal initializers: `struct node NAME { LIST };` ->
    #     `struct node NAME = { { .SUB = { LIST } } };` (everything in the union)
    for name, sub in spec.get('wrap_inits', {}).items():
        src = re.sub(r'struct node\s+' + name + r'\s*(?:=\s*)?\{([^}]*)\}\s*;',
                     lambda m: 'struct node ' + name + ' = { { .' + sub
                               + ' = { ' + m.group(1).strip() + ' } } };', src)

    # 2b. explicit initializer rewrites (literal pairs)
    for old, new in spec.get('inits', []):
        src = src.replace(old, new)

    # 3. intx type-punning -> placeholder (c1 only)
    intx_repls = []
    if spec.get('intx'):
        src, intx_repls = _add_intx(src)

    # 4. member access (arrow and dot)
    member = spec['member_map']
    def repl(m):
        op, name = m.group(1), m.group(2)
        if name in member:
            return op + 'u.' + member[name] + '.' + name
        return m.group(0)
    src = re.sub(r'(->|\.)([A-Za-z_]\w*)', repl, src)

    # 5. restore intx casts
    def restore(m):
        return intx_repls[int(m.group(1))]
    src = re.sub(r'\x01INTX(\d+)\x01', restore, src)

    return src


def main():
    spec_path = sys.argv[sys.argv.index('--spec') + 1]
    files = [a for a in sys.argv[1:] if a != '--spec' and a != spec_path]
    spec = json.load(open(spec_path))
    for path in files:
        with open(path) as f:
            sys.stdout.write(rewrite(f.read(), spec))


if __name__ == '__main__':
    main()
