#!/usr/bin/env python3
"""
c0-host.py -- c0 (V7 first pass) host-specific fixes, applied after knr2c99 +
union-node.  Mirrors ../build-host.py (the c1 pass) but for c0's V7-isms:

  * error() and outcode() are K&R varargs passthroughs -> stdarg/vfprintf.
  * outcode() walks the raw arg stack (`ap = &a; *ap++`); on a 64-bit host the
    mixed int16_t / char* / long args must become va_arg with per-format types.
  * LCON (32-bit long) is emitted as high word then low word (PDP-11 word order).
  * STRING / FSEL / struct-field sites pass pointers-as-numbers or a
    `struct field *` through a `struct node *` slot -> explicit casts.
  * paraml/parame were `struct hshtab **` used as `*`, with the parameter list
    "next" pointer stuffed into the 16-bit hoffset; host uses a real `next`.
  * cb / ccp generic byte-vs-word pointers -> `char *`.
  * build()'s register `t1` is int16_t in one arm and node* in the SIZEOF arm.
  * block(a,b,c,d,e) K&R default trailing arg -> explicit NULL.
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

    # ---- error(): K&R varargs -> stdarg/vfprintf ---------------------------
    s = s.replace(
        'int16_t error(int16_t s, int16_t p1, int16_t p2, int16_t p3, int16_t p4, int16_t p5, int16_t p6);',
        'int error(char *s, ...);')
    s = s.replace(
        'int16_t error(int16_t s, int16_t p1, int16_t p2, int16_t p3, int16_t p4, int16_t p5, int16_t p6)\n{',
        'int error(char *s, ...)\n{\n\tva_list ap;')
    s = s.replace(
        '\tfprintf(stderr, s, p1, p2, p3, p4, p5, p6);',
        '\tva_start(ap, s);\n\tvfprintf(stderr, s, ap);\n\tva_end(ap);')

    # ---- outcode(): K&R varargs -> va_list --------------------------------
    s = s.replace('void outcode(char *s, int16_t a);', 'void outcode(char *s, ...);')
    s = s.replace(
        'void outcode(char *s, int16_t a)\n{\n\tregister int16_t *ap;\n\tregister FILE *bufp;\n\tint16_t n;\n\tregister char *np;',
        'void outcode(char *s, ...)\n{\n\tva_list ap;\n\tregister FILE *bufp;\n\tint n;\n\tregister char *np;')
    s = s.replace('\tap = &a;', '\tva_start(ap, s);')
    s = s.replace('\t\tputc(*ap++, bufp);', '\t\tputc(va_arg(ap, int), bufp);')
    s = s.replace(
        '\t\tputc(*ap, bufp);\n\t\tputc(*ap++>>8, bufp);',
        '\t\tn = va_arg(ap, int);\n\t\tputc(n, bufp);\n\t\tputc(n>>8, bufp);')
    s = s.replace('\t\tnp = *ap++;', '\t\tnp = va_arg(ap, char *);')

    # ---- LCON: 32-bit long -> high word, then low word (PDP-11 word order) --
    s = s.replace(
        '\toutcode("BNNN", tp->u.tnode.op, tp->u.tnode.type, tp->u.lnode.lvalue);',
        '\toutcode("BN", tp->u.tnode.op, tp->u.tnode.type);\n'
        '\toutcode("NN", (int16_t)(tp->u.lnode.lvalue >> 16), (int16_t)tp->u.lnode.lvalue);')

    # ---- STRING: pointer-as-number through the tr1 slot -------------------
    s = s.replace(
        'outcode("BNNN", NAME, STATIC, tp->u.tnode.type, tp->u.tnode.tr1);',
        'outcode("BNNN", NAME, STATIC, tp->u.tnode.type, (int)(intptr_t)tp->u.tnode.tr1);')

    # ---- FSEL: tr2 holds a `struct field *`, not a node -------------------
    s = s.replace(
        'outcode("BNNN",tp->u.tnode.op,tp->u.tnode.type,tp->u.tnode.tr2->bitoffs,tp->u.tnode.tr2->flen);',
        'outcode("BNNN",tp->u.tnode.op,tp->u.tnode.type,'
        '((struct field *)tp->u.tnode.tr2)->bitoffs,((struct field *)tp->u.tnode.tr2)->flen);')

    # FSEL built in c01 build(): 6th arg is the field ptr passed through hstrp.
    # Also split the `*cp++ = block(..., *--cp, ...)` sequence-point UB.
    s = s.replace(
        '\t\t\t*cp++ = block(FSEL,UNSIGN,NULL,NULL,*--cp,p2->u.tnode.tr1->u.hshtab.hstrp);',
        '\t\t\tp1 = *--cp;\n'
        '\t\t\t*cp++ = block(FSEL,UNSIGN,NULL,NULL,p1,(struct node *)p2->u.tnode.tr1->u.hshtab.hstrp);')

    # ---- struct-field via strp/hstrp (c03.c declare) ----------------------
    s = s.replace('dsym->u.hshtab.hstrp->bitoffs', '((struct field *)dsym->u.hshtab.hstrp)->bitoffs')
    s = s.replace('dsym->u.hshtab.hstrp->flen', '((struct field *)dsym->u.hshtab.hstrp)->flen')

    # ---- paraml/parame: "next" pointer instead of the 16-bit hoffset ------
    s = s.replace('parame->u.hshtab.hoffset = dsym;', 'parame->u.hshtab.next = dsym;')
    s = s.replace('parame->u.hshtab.hoffset = 0;', 'parame->u.hshtab.next = 0;')
    s = s.replace('paraml = paraml->u.hshtab.hoffset;', 'paraml = paraml->u.hshtab.next;')

    # ---- cb: byte cursor, not a word pointer ------------------------------
    s = s.replace('\tint16_t sclass, scflag, *cb;', '\tint16_t sclass, scflag;\n\tchar *cb;')
    s = s.replace('\tregister int16_t *cb;', '\tregister char *cb;')
    s = s.replace('\tint16_t width, isarray, o, brace, realtype, *cb;',
                  '\tint16_t width, isarray, o, brace, realtype;\n\tchar *cb;')

    # ---- ccp walks the bytes of cval (getcc) ------------------------------
    s = s.replace('\tccp = &cval;', '\tccp = (char *)&cval;')

    # ---- build()'s t1 is a node* in the SIZEOF arm, int16_t elsewhere -----
    s = s.replace(
        '\t\tt1 = cblock(length(p1));\n\t\tt1->u.tnode.type = UNSIGN;\n\t\t*cp++ = t1;',
        '\t\tstruct node *tn = cblock(length(p1));\n\t\ttn->u.tnode.type = UNSIGN;\n\t\t*cp++ = tn;')

    # ---- plength takes a node, not a struct tname -------------------------
    s = s.replace('int16_t plength(struct tname *ap)', 'int16_t plength(struct node *ap)')

    # ---- `sizeof(*xprtype())` = size of a node (V7 type-name idiom) --------
    s = s.replace('sizeof(*xprtype())', 'sizeof(struct node)')

    # ---- gblock: allocate raw bytes, return void* (host pointers differ) ---
    s = s.replace('char * gblock(int16_t n);', 'void * gblock(int16_t n);')
    s = s.replace(
        'char * gblock(int16_t n)\n{\n\tregister int16_t *p;',
        'void * gblock(int16_t n)\n{\n\tregister char *p;')
    # bump-allocate from the mmap'd arena (main sets coremax); no sbrk (it would
    # fight glibc's malloc heap).
    s = s.replace(
        '\t\tif (sbrk(1024) == -1) {\n\t\t\terror("Out of space");\n\t\t\texit(1);\n\t\t}\n\t\tcoremax += 1024;',
        '\t\terror("Out of space");\n\t\texit(1);')
    # main: arena from mmap, not sbrk(0)
    s = s.replace(
        'coremax = funcbase = curbase = sbrk(0);',
        'funcbase = curbase = mmap(NULL, 16<<20, PROT_READ|PROT_WRITE,\n'
        '\t\t\t   MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);\n'
        '\tif (funcbase == MAP_FAILED) {\n'
        '\t\terror("Out of space");\n'
        '\t\texit(1);\n'
        '\t}\n'
        '\tcoremax = funcbase + (16<<20);')

    # ---- xprtype: decl1(sc) not decl1(&sc) (storage class by value) --------
    s = s.replace('\tdecl1(&sc, &typer, 0, tyb);', '\tdecl1(sc, &typer, 0, tyb);')
    # scp saves/restores the expression-stack pointer `cp` (node**)
    s = s.replace('\tstruct node *scp;', '\tstruct node **scp;')

    # ---- strinit: zerloc is a dummy node* slot, not an int16_t -------------
    s = s.replace('\tstatic int16_t zerloc;', '\tstatic struct node *zerloc;')

    # ---- statement(): o1 is a label (int16_t) everywhere except the NAME
    #      case, where it briefly holds csym (a node*); give that arm its own var.
    s = s.replace('\t\t\to1 = csym;', '\t\t\tstruct node *onp = csym;')
    s = s.replace('pushdecl(o1);', 'pushdecl(onp);')
    s = s.replace('defsym = o1;', 'defsym = onp;')
    s = s.replace('o1->u.hshtab', 'onp->u.hshtab')

    # ---- forstmt()'s ss is a byte cursor (char*), not a word pointer --------
    s = s.replace('\tint16_t sline1, *ss;', '\tint16_t sline1;\n\tchar *ss;')

    # pexpr()'s t is a node*, o stays int16_t
    s = s.replace('\tregister int16_t o, t;', '\tregister int16_t o;\n\tstruct node *t;')

    # ---- main returns int --------------------------------------------------
    s = s.replace('int16_t main(int16_t argc, char *argv[])',
                  'int main(int argc, char *argv[])')

    # ---- block(a,b,c,d,e) -> explicit trailing NULL -----------------------
    s = _add_default_arg(s, 'block', 6, 'NULL')

    return s


def main():
    for path in sys.argv[1:]:
        with open(path) as f:
            sys.stdout.write(transform(f.read()))


if __name__ == '__main__':
    main()
