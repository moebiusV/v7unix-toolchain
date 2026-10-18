#!/usr/bin/env python3
"""
corpus-report.py -- write the size report (doc/SIZE-REPORT.md in the pcc tree) from measured data.

    corpus-report.py [--results DIR] [--oldtargets DIR] [--out FILE]

Reads
  DIR/results.json        corpus.py: every file x target x compiler configuration
  OLD/pdp11-*-*.txt       pccdiff.sh -v output: dmr's cc against stock and ours on the V7 sources
  OLD/vax-ours-O0.txt     vaxdiff.py output: the real 32V cc and cc -O on the 32V sources
and writes the tables with the numbers computed here, so the prose never disagrees with them.
"""
import argparse, collections, importlib.util, json, os, re, sys

here = os.path.dirname(os.path.abspath(__file__))


def load(name, path):
    sp = importlib.util.spec_from_file_location(name, os.path.join(here, path))
    m = importlib.util.module_from_spec(sp)
    sp.loader.exec_module(m)
    return m


corpus = load('corpus', 'corpus.py')
old = load('oldt', 'corpus-oldtargets.py')
ARCH = corpus.ARCH
pct = lambda x, y: '%+.1f%%' % (100.0 * (x - y) / y) if y else ''


def sums(rs, keys):
    """sum the sizes of the files for which every key compiled"""
    ok = [r for r in rs if all(k in r['size'] for k in keys)]
    s = collections.Counter()
    for r in ok:
        for k in keys:
            s[k] += r['size'][k]
    return len(ok), s


def modern_tables(results):
    """per target: stock / ours / clang / gcc, unoptimized and optimized, all corpora together"""
    out = []
    archs = [a for a in ARCH if ARCH[a]['clang'] is not None]
    for title, keys in (('Without optimization', ('stock-O0', 'ours-O0', 'clang-O0', 'gcc-O0')),
                        ('With optimization (stock `-O`, ours `-O -xdmr`, clang/gcc `-Os`)', ('stock-O', 'ours-O', 'clang-Os', 'gcc-Os'))):
        out += ['#### ' + title, '',
                '| target | files | compiled by stock / ours | common files | stock | ours | clang | gcc | ours vs stock | ours vs clang |',
                '|---|---|---|---|---|---|---|---|---|---|']
        for a in archs:
            rs = [r for r in results if r['arch'] == a]
            if not rs:
                continue
            n, s = sums(rs, keys[:3])
            cs = sum(1 for r in rs if keys[0] in r['size'])
            co = sum(1 for r in rs if keys[1] in r['size'])
            ng, sg = sums([r for r in rs if keys[3] in r['size']], keys[:4]) if ARCH[a]['gcc'] else (0, None)
            g = ('%d (ours %d)' % (sg[keys[3]], sg[keys[1]])) if ng else ''
            out.append('| %s | %d | %d / %d | %d | %s | %s | %s | %s | %s | %s |' % (
                a, len(rs), cs, co, n, s[keys[0]] or 'n/a', s[keys[1]] or '', s[keys[2]] or '', g,
                pct(s[keys[1]], s[keys[0]]) if s[keys[0]] else 'stock fails', pct(s[keys[1]], s[keys[2]])))
        out.append('')
    return out


def per_corpus(results, arch, keys):
    out = ['| corpus | files | stock / ours compiled | common | stock | ours | clang | ours vs stock | ours vs clang |',
           '|---|---|---|---|---|---|---|---|---|']
    for c in corpus.CORPORA:
        rs = [r for r in results if r['arch'] == arch and r['corp'] == c]
        if not rs:
            continue
        n, s = sums(rs, keys[:3])
        cs = sum(1 for r in rs if keys[0] in r['size'])
        co = sum(1 for r in rs if keys[1] in r['size'])
        out.append('| %s | %d | %d / %d | %d | %s | %s | %s | %s | %s |' % (
            c, len(rs), cs, co, n, s[keys[0]] or '', s[keys[1]] or '', s[keys[2]] or '',
            pct(s[keys[1]], s[keys[0]]) if s[keys[0]] else '', pct(s[keys[1]], s[keys[2]])))
    return out


def failures(results):
    out = ['| target | files | stock fails | ours fails |', '|---|---|---|---|']
    for a in ARCH:
        rs = [r for r in results if r['arch'] == a]
        if rs:
            out.append('| %s | %d | %d | %d |' % (a, len(rs), sum(1 for r in rs if 'stock-O' not in r['size']),
                                                  sum(1 for r in rs if 'ours-O' not in r['size'])))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--results', default='/tmp/co-all')
    ap.add_argument('--oldtargets', default='/tmp/oldtargets')
    ap.add_argument('--out', default='SIZE-REPORT.md')
    a = ap.parse_args()
    results = json.load(open(os.path.join(a.results, 'results.json')))
    old.OUT = a.oldtargets
    L = []
    L += modern_tables(results)
    open(a.out + '.modern', 'w').write('\n'.join(L) + '\n')
    open(a.out + '.fail', 'w').write('\n'.join(failures(results)) + '\n')
    for arch in ARCH:
        if any(r['arch'] == arch for r in results):
            open(a.out + '.%s' % arch, 'w').write('\n'.join(per_corpus(results, arch, ('stock-O', 'ours-O', 'clang-Os'))) + '\n')
    print('\n'.join(L))


if __name__ == '__main__':
    main()
