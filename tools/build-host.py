#!/usr/bin/env python3
"""
build-host.py -- c1 host-specific fixes, applied after knr2c99 + union-node.

The union-node refactor makes the node structs C99-legal; this pass handles the
remaining V7-isms that only matter once you compile on a 64-bit host:

  * `error()` is a K&R varargs passthrough (``fprintf(stderr, s, p1..p6)``) ->
    ``int error(char *s, ...)`` using <stdarg.h>/vfprintf.
  * implicit-``int`` parameters that are really pointers: comarg/delay/chkleaf/
    ispow2/outname/acommute/insert take a ``struct node *``/``struct table *``/
    ``char *`` where knr2c99 could only recover ``int``.
  * member-access-by-name on the *non-node* structs: ``table->op`` (the first
    field of struct table) and ``insp->op`` (struct instab) are really
    ``tabop``/``iop``.
  * ``getblk``'s ``register *p;`` (implicit-int generic byte pointer) -> ``char *``.
  * ``sbrk()`` returns ``void *`` on the host, so ``== -1`` becomes ``(char *)-1``.
  * ``main`` returns int (not the word-sized int16_t).
  * K&R default trailing arguments: ``tnode(a,b,c)`` and ``branch(a,b)`` get an
    explicit ``NULL``/``0``.

Usage:  python3 build-host.py file.c > file.host.c
"""

import re
import sys


def _add_default_arg(src, fname, nargs, default):
    """Add `default` to every n-arg call of fname() that's missing it (has n-1)."""
    out = []
    i = 0
    key = fname + '('
    while True:
        j = src.find(key, i)
        if j < 0:
            out.append(src[i:])
            break
        out.append(src[i:j])
        k = j + len(key)
        depth = 1
        commas = 0
        while k < len(src) and depth > 0:
            c = src[k]
            if c == '(':
                depth += 1
            elif c == ')':
                depth -= 1
            elif c == ',' and depth == 1:
                commas += 1
            k += 1
        call = src[j:k]
        if commas == nargs - 2:          # called with one arg too few
            call = call.rstrip()[:-1] + ', ' + default + ')'
        out.append(call)
        i = k
    return ''.join(out)


def transform(src):
    s = src

    # error(): K&R varargs passthrough -> stdarg + vfprintf
    s = s.replace(
        'int16_t error(int16_t s, int16_t p1, int16_t p2, int16_t p3, int16_t p4, int16_t p5, int16_t p6);',
        'int error(char *s, ...);')
    s = s.replace(
        'int16_t error(int16_t s, int16_t p1, int16_t p2, int16_t p3, int16_t p4, int16_t p5, int16_t p6)\n{',
        'int error(char *s, ...)\n{\n\tva_list ap;')
    s = s.replace(
        '\tfprintf(stderr, s, p1, p2, p3, p4, p5, p6);',
        '\tva_start(ap, s);\n\tvfprintf(stderr, s, ap);\n\tva_end(ap);')

    # implicit-int params that are really pointers
    s = s.replace('comarg(int16_t atree,', 'comarg(struct node *atree,')
    s = s.replace('delay(struct node **treep, int16_t table,',
                  'delay(struct node **treep, struct table *table,')
    s = s.replace('chkleaf(struct node *atree, int16_t table,',
                  'chkleaf(struct node *atree, struct table *table,')
    s = s.replace('ispow2(int16_t atree)', 'ispow2(struct node *atree)')
    s = s.replace('outname(int16_t s)', 'outname(char *s)')
    s = s.replace('acommute(int16_t atree)', 'acommute(struct node *atree)')
    s = s.replace('insert(int16_t op, int16_t atree,',
                  'insert(int16_t op, struct node *atree,')

    # implicit-int returns that are really pointers
    s = s.replace('int16_t pow2(', 'struct node *pow2(')
    s = s.replace('int16_t ncopy(', 'struct node *ncopy(')
    s = s.replace('int16_t sdelay(', 'struct node *sdelay(')
    s = s.replace('int16_t acommute(', 'struct node *acommute(')
    s = s.replace('int16_t hardlongs(', 'struct node *hardlongs(')
    s = s.replace('int16_t lvfield(', 'struct node *lvfield(')
    s = s.replace('int16_t outname(', 'char *outname(')

    # a local `opt` (match()'s result) recovered as char*
    s = s.replace('char *opt;', 'struct optab *opt;')

    # member-access-by-name on the non-node structs
    s = s.replace('table->op', 'table->tabop')
    s = s.replace('insp->op', 'insp->iop')

    # getblk: implicit-int generic byte pointer
    s = s.replace(
        'void *getblk(int16_t size)\n{\n\tregister int16_t *p;',
        'void *getblk(int16_t size)\n{\n\tregister char *p;')

    # sbrk returns void* on the host
    s = s.replace('sbrk(1024) == -1', 'sbrk(1024) == (char *)-1')

    # main returns int
    s = s.replace('int16_t main(int16_t argc, char *argv[])',
                  'int main(int argc, char *argv[])')

    # getree()'s generic register `t` is int / char* / node* in different switch
    # cases.  Split the char* (outname) and node* (*sp) uses off into `s`/`tn`.
    s = s.replace('register int16_t t, op;',
                  'register int16_t t, op;\n\tstruct node *tn;')
    s = s.replace('t = *--sp;\n\t\t\t*sp++ = tnode(op, geti(), *--sp, t);',
                  'tn = *--sp;\n\t\t\t*sp++ = tnode(op, geti(), *--sp, tn);')
    lines = s.split('\n')
    out = []
    i = 0
    while i < len(lines):
        ln = lines[i]
        if 't = outname(s);' in ln:
            out.append(ln.replace('t = outname(s);', 'outname(s);'))
            if i + 1 < len(lines):
                out.append(re.sub(r'\bt\b', 's', lines[i + 1]))
                i += 2
                continue
        out.append(ln)
        i += 1
    s = '\n'.join(out)

    # unoptim()'s `p` is int16_t* for one double walk, struct node* elsewhere
    s = s.replace('register int16_t *p;', 'register struct node *p;')
    s = s.replace('p = &fv;\n\t\tp++;\n\t\tif (*p++==0 && *p++==0 && *p++==0) {',
                  'int16_t *wp = (int16_t *)&fv + 1;\n\t\tif (*wp++==0 && *wp++==0 && *wp++==0) {')

    # pswitch()'s first arg is a swtab array aliased onto the funcbase buffer
    s = s.replace('pswitch(funcbase, swp, t);',
                  'pswitch((struct swtab *)funcbase, swp, t);')

    # K&R default trailing arguments
    s = _add_default_arg(s, 'tnode', 4, 'NULL')
    s = _add_default_arg(s, 'branch', 3, '0')

    return s


def main():
    for path in sys.argv[1:]:
        with open(path) as f:
            sys.stdout.write(transform(f.read()))


if __name__ == '__main__':
    main()
