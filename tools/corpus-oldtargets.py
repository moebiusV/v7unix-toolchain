#!/usr/bin/env python3
"""
corpus-oldtargets.py -- turn the raw output of corpus-oldtargets.sh into the PDP-11 and VAX tables.

    corpus-oldtargets.py [OUTDIR]          (default /tmp/oldtargets)

Each row sums the files that both stock pcc and this branch compiled and assembled, so the two
columns are always the same files.  Sizes are text+data bytes after the target's own assembler.
"""
import os, re, sys, collections

OUT = sys.argv[1] if len(sys.argv) > 1 else '/tmp/oldtargets'


def pdp11(kind, corp):
    """{file: (dmr, unopt, opt)} from pccdiff -v"""
    d = {}
    p = os.path.join(OUT, 'pdp11-%s-%s.txt' % (kind, corp))
    for l in open(p, errors='replace'):
        m = re.match(r'(?:OK|BIGGER)\s+dmr\s+(\d+)\s+c2-only\s+(\d+) \([+-]\d+\)\s+-O\s+(\d+)', l)
        if m:
            d[l.split()[-1]] = tuple(int(x) for x in m.groups())
    return d


def vax(kind, cfg):
    """{file: (32V cc, 32V -O, pcc, pcc+c2)} from vaxdiff"""
    d = {}
    p = os.path.join(OUT, 'vax-%s-%s.txt' % (kind, cfg))
    for l in open(p, errors='replace'):
        m = re.match(r'(\S+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s*$', l)
        if m and m.group(1) != 'TOTAL':
            d[m.group(1)] = tuple(int(x) for x in m.groups()[1:])
    return d


def pct(a, b):
    return '%+.1f%%' % (100.0 * (a - b) / b) if b else ''


def main():
    L = []
    L.append('### PDP-11 (V7 corpus, reference: dmr\'s `cc`)')
    L.append('')
    L.append('| corpus | files in common | dmr cc | stock, no flags | stock `-O` | ours, no flags | ours `-O` | stock `-O` vs dmr | ours `-O` vs dmr | ours vs stock `-O` |')
    L.append('|---|---|---|---|---|---|---|---|---|---|')
    for corp in ('libc', 'cmd'):
        s, o = pdp11('stock', corp), pdp11('ours', corp)
        com = sorted(set(s) & set(o))
        t = lambda d, i: sum(d[f][i] for f in com)
        n = {k: len(v) for k, v in (('stock', s), ('ours', o))}
        L.append('| %s | %d (stock compiled %d, ours %d) | %d | %d | %d | %d | %d | %s | %s | %s |' % (
            corp, len(com), n['stock'], n['ours'], t(s, 0), t(s, 1), t(s, 2), t(o, 1), t(o, 2),
            pct(t(s, 2), t(s, 0)), pct(t(o, 2), t(o, 0)), pct(t(o, 2), t(s, 2))))
    L.append('')
    L.append('"No flags" is `pcc -S` followed by the V7 peephole `c2`; `-O` adds the compiler\'s own optimizer')
    L.append('(ours: with `-xdmr`) and then `c2`.  Both use the same V7 assembler.')
    L.append('')
    L.append('### VAX (32V corpus, reference: 32V\'s `cc` and `cc -O`)')
    L.append('')
    sO0, oO0, sO, oO = vax('stock', 'O0'), vax('ours', 'O0'), vax('stock', 'O'), vax('ours', 'O')
    com = sorted(set(sO0) & set(oO0) & set(sO) & set(oO))
    t = lambda d, i: sum(d[f][i] for f in com)
    L.append('Files that stock and ours both compiled and the 32V assembler accepted: %d (stock %d, ours %d of the corpus).' % (
        len(com), len(set(sO0) & set(sO)), len(set(oO0) & set(oO))))
    L.append('')
    L.append('| | no c2 | + 32V c2 |')
    L.append('|---|---|---|')
    L.append('| 32V `cc` / `cc -O` | %d / %d | |' % (t(sO0, 0), t(sO0, 1)))
    L.append('| stock, no flags | %d | %d |' % (t(sO0, 2), t(sO0, 3)))
    L.append('| stock `-O` | %d | %d |' % (t(sO, 2), t(sO, 3)))
    L.append('| ours, no flags | %d | %d |' % (t(oO0, 2), t(oO0, 3)))
    L.append('| ours `-O` (with `-xdmr`) | %d | %d |' % (t(oO, 2), t(oO, 3)))
    L.append('')
    L.append('Ours `-O` + c2 against 32V `cc -O`: %s; stock `-O` + c2 against 32V `cc -O`: %s; ours against stock: %s.' % (
        pct(t(oO, 3), t(sO0, 1)), pct(t(sO, 3), t(sO0, 1)), pct(t(oO, 3), t(sO, 3))))
    s = '\n'.join(L) + '\n'
    open(os.path.join(OUT, 'oldtargets.md'), 'w').write(s)
    print(s)


if __name__ == '__main__':
    main()
