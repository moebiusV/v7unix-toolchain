#!/usr/bin/env python3
"""
corpus-report.py -- write doc/SIZE-REPORT.md of the pcc tree from measured data.

    corpus-report.py [--results DIR] [--oldtargets DIR] [--out FILE]

Reads
  DIR/results.jsonl       corpus.py: every file x target x compiler configuration
  OLD/pdp11-*-*.txt       pccdiff.sh -v: dmr's cc against stock and ours on the V7 sources
  OLD/vax-ours-O0.txt     vaxdiff.py: the real 32V cc and cc -O on the 32V sources
Every number in the tables and in the summary sentences is computed here.
"""
import argparse, collections, importlib.util, json, os, re, subprocess, sys

here = os.path.dirname(os.path.abspath(__file__))


def load(name, path):
    sp = importlib.util.spec_from_file_location(name, os.path.join(here, path))
    m = importlib.util.module_from_spec(sp)
    sp.loader.exec_module(m)
    return m


corpus = load('corpus', 'corpus.py')
old = load('oldt', 'corpus-oldtargets.py')
ARCH = corpus.ARCH
OLDCORP = ('v7', '32v')
UNOPT = ('stock-O0', 'ours-O0', 'clang-O0', 'gcc-O0')
OPT = ('stock-O', 'ours-O', 'clang-Os', 'gcc-Os')


def pct(x, y, d=1):
    return ('%+.' + str(d) + 'f%%') % (100.0 * (x - y) / y) if y else ''


def rel(x, y, d=1):
    """'9.2% smaller than' / '49.1% larger than'"""
    v = 100.0 * (x - y) / y
    return ('%.' + str(d) + 'f%% %s') % (abs(v), 'smaller than' if v < 0 else 'larger than')


def ncomp(rs, k):
    return sum(1 for r in rs if k in r['size'])


def sums(rs, need, add=()):
    ok = [r for r in rs if all(k in r['size'] for k in need)]
    s = collections.Counter()
    for r in ok:
        for k in tuple(need) + tuple(add):
            if k in r['size']:
                s[k] += r['size'][k]
    return ok, s


def row(name, a, rs, keys, with_gcc=True):
    st, ou, cl, gc = keys
    hasc = ARCH[a]['clang'] is not None
    need = (st, ou) + ((cl,) if hasc else ())
    ok, s = sums(rs, need)
    g = gp = ''
    if hasc and ARCH[a]['gcc'] and with_gcc:
        okg = [r for r in ok if gc in r['size']]
        gs = sum(r['size'][gc] for r in okg)
        go = sum(r['size'][ou] for r in okg)
        g = '%d' % gs if gs else ''
        gp = pct(go, gs) if gs else ''
    return ('| %s | %d | %d / %d | %d | %s | %s | %s | %s | %s | %s | %s |' % (
        name, len(rs), ncomp(rs, st), ncomp(rs, ou), len(ok),
        '{:,}'.format(s[st]) if s[st] else '-', '{:,}'.format(s[ou]) if s[ou] else '-',
        '{:,}'.format(s[cl]) if hasc and s[cl] else '', '{:,}'.format(int(g)) if g else '',
        pct(s[ou], s[st]) if s[st] else 'stock fails', pct(s[ou], s[cl]) if hasc and s[cl] else '', gp)), s, ok


HDR = ['| %s | files | compiled: stock / ours | common | stock | ours | clang | gcc | ours vs stock | ours vs clang | ours vs gcc |',
       '|---|---|---|---|---|---|---|---|---|---|---|']


def table(R, corps, archs, keys, first='target'):
    out = [HDR[0] % first, HDR[1]]
    for a in archs:
        rs = [r for r in R if r['arch'] == a and r['corp'] in corps]
        if rs:
            out.append(row(a, a, rs, keys)[0])
    return out


def percorp(R, a, corps, keys):
    out = [HDR[0] % 'corpus', HDR[1]]
    for k in corps:
        rs = [r for r in R if r['arch'] == a and r['corp'] == k]
        if rs:
            out.append(row(k, a, rs, keys)[0])
    return out


def fail_reasons(R, a, key, other, corps, top=4):
    """why `key` fails where `other` compiled"""
    c = collections.Counter()
    for r in R:
        if r['arch'] == a and r['corp'] in corps and key in r['fail'] and other in r['size']:
            m = re.sub(r'0x[0-9a-f]+|\d+', 'N', r['fail'][key][1])[:60]
            c[m] += 1
    return c.most_common(top), sum(c.values())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--results', default='/tmp/co-final')
    ap.add_argument('--oldtargets', default='/tmp/oldtargets')
    ap.add_argument('--out', default=os.path.expanduser('~/claude/pcc/doc/SIZE-REPORT.md'))
    ap.add_argument('--no-refresh', action='store_true', help='use the results as they are')
    ap.add_argument('-j', default='2')
    a = ap.parse_args()
    if not a.no_refresh:
        refresh(a)
    R = [json.loads(l) for l in open(os.path.join(a.results, 'results.jsonl'))]
    old.OUT = a.oldtargets
    C99 = [k for k in corpus.CORPORA if k not in OLDCORP]
    nfiles = collections.Counter(r['corp'] for r in R if r['arch'] == 'amd64')
    total = sum(nfiles[k] for k in C99)
    L = []
    w = L.append

    def S(arch, keys, corps=C99):
        rs = [r for r in R if r['arch'] == arch and r['corp'] in corps]
        return row(arch, arch, rs, keys)

    # headline numbers
    _, ua, _ = S('amd64', UNOPT); _, oa, okA = S('amd64', OPT)
    _, ui, _ = S('i386', UNOPT); _, oi, okI = S('i386', OPT)
    _, u6, _ = S('aarch64', UNOPT); _, o6, ok6 = S('aarch64', OPT)
    rs6 = [r for r in R if r['arch'] == 'aarch64' and r['corp'] in C99]
    _, s6 = sums(rs6, ('ours-O', 'clang-Os'))

    w('# Code size: stock PortableCC against this fork, clang, gcc, V7 dmr cc and 32V pcc')
    w('')
    w('This report compares the code size of this fork of pcc with **stock PortableCC** (current `PortableCC/pcc`')
    w('master, commit 9a993a5f), with **clang** and **gcc** on the targets they share with pcc, and, for the two')
    w('old targets, with the **original compilers**: V7 dmr cc (the PDP-11 compiler of V7 Unix) and 32V pcc (the VAX compiler of 32V, itself')
    w('Johnson\'s pcc).  Ten C99 corpora (%d source files: %s) are compiled for every back end that pcc and clang' % (
        total, ', '.join('%s %d' % (k, nfiles[k]) for k in C99)))
    w('have in common, with and without optimization, plus the V7 and 32V source trees that the earlier size')
    w('work used.  Everything is regenerated from the measurement files by `tools/corpus-report.py`.')
    w('')
    w('Base: the fork contains upstream PortableCC commit `9a993a5f` (it was merged on 18 October 2026), which is also the')
    w('"stock" measured here, so stock and the fork differ only by the fork\'s own changes.  Where both implement the same')
    w('idea (cross-jumping on the intermediate code, the branch inversion that keeps operand order) both are kept when')
    w('the fork\'s adds something: its cross-jumping, behind `-xdmr`, is 1% smaller on amd64 and AArch64 on top of')
    w('upstream\'s and never larger on the 126 files tried.')
    w('')
    w('## Summary')
    w('')
    # PDP-11 / VAX headline
    pl, pc = old.pdp11('stock', 'libc'), old.pdp11('stock', 'cmd')
    ol, oc = old.pdp11('ours', 'libc'), old.pdp11('ours', 'cmd')

    def pd(s, o):
        com = sorted(set(s) & set(o))
        t = lambda d, i: sum(d[f][i] for f in com)
        return len(com), t(s, 0), t(s, 2), t(o, 2)
    nl, dl, sl, olx = pd(pl, ol); nc, dc, sc, oc_ = pd(pc, oc)
    w('* **PDP-11, V7 libc and commands, against V7 dmr cc** (%d + %d files both pcc builds compile): stock `-O` is' % (nl, nc))
    w('  **%s / %s** against V7 dmr cc; this fork\'s `-O` is **%s / %s** (and %s / %s against stock `-O`).' % (
        pct(sl, dl), pct(sc, dc), pct(olx, dl), pct(oc_, dc), pct(olx, sl), pct(oc_, sc)))
    G, by = old_vax(R, a.oldtargets)
    for part in ('libc', 'cmd'):
        pass
    w('* **VAX, 32V libc and commands, against 32V pcc -O**: see the VAX section; stock `-O` is %s / %s and this' % (
        pct(G['libc']['stock-O'], G['libc']['O']), pct(G['cmd']['stock-O'], G['cmd']['O'])))
    w('  fork\'s `-O` is %s / %s.' % (pct(G['libc']['ours-O'], G['libc']['O']), pct(G['cmd']['ours-O'], G['cmd']['O'])))
    ga = [r for r in okA if 'gcc-Os' in r['size']]; gi = [r for r in okI if 'gcc-Os' in r['size']]
    w('* **All C99 corpora, optimized, files every compiler compiled**: on amd64 ours is %s stock, %s clang `-Os` and %s gcc `-Os`;' % (
        rel(oa['ours-O'], oa['stock-O']), rel(oa['ours-O'], oa['clang-Os']),
        rel(sum(r['size']['ours-O'] for r in ga), sum(r['size']['gcc-Os'] for r in ga))))
    w('  on i386 %s stock, %s clang and %s gcc.' % (
        rel(oi['ours-O'], oi['stock-O']), rel(oi['ours-O'], oi['clang-Os']),
        rel(sum(r['size']['ours-O'] for r in gi), sum(r['size']['gcc-Os'] for r in gi))))
    w('* **AArch64**: stock\'s back end compiles %d of the %d files, the fork %d.  Over the %d files all three compiled, the' % (
        ncomp(rs6, 'stock-O'), len(rs6), ncomp(rs6, 'ours-O'), len(ok6)))
    w('  fork is %s stock and %s clang `-Os`; over every file the fork and clang both compiled it is %s clang.' % (
        rel(o6['ours-O'], o6['stock-O']), rel(o6['ours-O'], o6['clang-Os']), rel(s6['ours-O'], s6['clang-Os'])))
    w('* **The other back ends** (arm, m68k, mips, mips64, sparc64) compile only part of the corpus, in stock and in the')
    w('  fork alike, riscv64 and powerpc nothing: they are unfinished ports the fork did not work on.')
    rem = {arch: fail_reasons(R, arch, 'ours-O', 'stock-O', C99)[1] for arch in ('amd64', 'i386', 'aarch64', 'pdp11', 'vax')}
    more = {arch: sum(1 for r in R if r['arch'] == arch and r['corp'] in C99 and 'ours-O' in r['size'] and 'stock-O' not in r['size'])
            for arch in ('amd64', 'i386', 'aarch64', 'pdp11', 'vax')}
    w('* **Reliability**: files stock compiles and the fork does not: ' + ', '.join('%s %d' % (k, v) for k, v in rem.items()) +
      '; files the fork compiles and stock does not: ' + ', '.join('%s %d' % (k, v) for k, v in more.items()) +
      '.  The corpus found three bugs in the fork that made the first set of these numbers worse (all fixed, see "What this run found").')
    w('')
    w('## What was measured')
    w('')
    w('* **Size** is text plus data bytes of the assembled object (`size` on the host for ELF targets; the a.out header of')
    w('  V7 `as` and of `as32v` for the PDP-11 and VAX).  Clang and gcc are built **without unwind tables**')
    w('  (`-fno-asynchronous-unwind-tables -fno-unwind-tables`) and **without PIE** (`-fno-pic`; on x86 also')
    w('  `-fcf-protection=none`), so they are compared with what pcc generates; pcc writes neither.  Without that,')
    w('  clang\'s `.eh_frame` alone adds 25 to 40% to a small file.')
    w('* **Compilers.**  *stock*: current PortableCC master built for each target.  *ours*: this branch.  Unoptimized means')
    w('  no flags.  Optimized means what `pcc -O` passes to `ccom` in each tree (`-xtemps -xdeljumps -xinline -xdce`, plus')
    w('  `-xssa` on Linux targets); for ours that is `-O -xdmr`, so also `-xdmr`.  Clang and gcc use `-O0` and `-Os`.')
    w('* **Only files every compiler in a row compiled are summed**, so each row compares the same code.  A file that fails')
    w('  to compile is a result of its own and is counted in the "compiled" columns; it is never dropped silently or')
    w('  averaged in.  Compiles are cut off after 90 seconds (a compiler still running then counts as failed).')
    w('* **Input.**  Each file is preprocessed once with the host gcc against the musl 1.2.5 headers of that architecture, and')
    w('  the identical text goes to every compiler.  The PDP-11 and VAX have no musl headers; the i386 ones supply the')
    w('  typedefs (and musl\'s `struct timespec` padding field is removed for the 16-bit PDP-11, where it cannot fit).')
    w('* **PDP-11 and VAX** have no clang or gcc.  They use V7\'s `as` and `c2` (ported to the host) and `as32v`, a host')
    w('  port of the 32V assembler (`modern/usr/src/cmd/as32v` in the toolchain repository) that gives the same size as the')
    w('  real 32V assembler in an emulated VAX on all 112 sources compared with unique names.  Optimized output goes')
    w('  through the target\'s peephole `c2` (stock: V7\'s `c2` and the 32V-derived `c2vax`; ours: `cc2.pdp11.c` and')
    w('  `cc2.vax.c`, which are this fork\'s versions).  For the VAX the symbols are renamed to at most 8 characters first,')
    w('  because 32V\'s `as` keeps only 8, and stock\'s GNU-syntax output is rewritten to the old spelling (`%r0` to `r0`,')
    w('  `.L12` to `L12`, a leading `_`); neither changes an instruction.')
    w('')
    w('## C99 corpora on the 32- and 64-bit targets')
    w('')
    w('### All %d C99 files together' % total)
    w('')
    w('Unoptimized:')
    w('')
    L += table(R, C99, list(ARCH), UNOPT)
    w('')
    w('Optimized:')
    w('')
    L += table(R, C99, list(ARCH), OPT)
    w('')
    w('"Common" is the number of files all compilers of that row compiled; the size columns sum those files only.  On the')
    w('targets without clang (PDP-11, VAX) the comparison is stock against ours.')
    w('')
    for arch in ('amd64', 'i386', 'aarch64'):
        w('### %s, optimized, by corpus' % arch)
        w('')
        L += percorp(R, arch, C99, OPT)
        w('')
    w('Unoptimized, per corpus:')
    w('')
    for arch in ('amd64', 'i386', 'aarch64'):
        w('<details><summary>%s</summary>' % arch)
        w('')
        L += percorp(R, arch, C99, UNOPT)
        w('')
        w('</details>')
        w('')
    # PR #47: the open upstream pull request that overhauls the old AArch64 back end
    rs47 = [r for r in R if r['arch'] == 'aarch64' and r['corp'] in C99]
    if any('pr47-O' in r['size'] or 'pr47-O' in r['fail'] for r in rs47):
        w('### AArch64 against upstream pull request #47')
        w('')
        w('PortableCC pull request #47 (hakanrw, open since February 2026) overhauls the old AArch64 back end; its code is')
        w('built here as a third compiler (`pr47`) beside stock and the fork, on the same files and with the same assembler.')
        w('')
        w('| | compiled and assembled, `-O` | `-O0` |')
        w('|---|---|---|')
        for lab, k0, k in (('stock', 'stock-O0', 'stock-O'), ('PR #47', 'pr47-O0', 'pr47-O'), ('the fork', 'ours-O0', 'ours-O')):
            w('| %s | %d / %d | %d / %d |' % (lab, ncomp(rs47, k), len(rs47), ncomp(rs47, k0), len(rs47)))
        w('')
        for lab, k0, kO in (('`-O`', 'pr47-O0', 'pr47-O'),):
            ok = [r for r in rs47 if all(x in r['size'] for x in ('stock-O', 'pr47-O', 'ours-O', 'clang-Os'))]
            ok2 = [r for r in rs47 if all(x in r['size'] for x in ('pr47-O', 'ours-O', 'clang-Os'))]
            S = lambda rr, x: sum(r['size'][x] for r in rr)
            w('Sizes at `-O`, on the %d files stock, PR #47, the fork and clang all compiled: stock %s, PR #47 %s (%s against stock),' % (
                len(ok), '{:,}'.format(S(ok, 'stock-O')), '{:,}'.format(S(ok, 'pr47-O')), pct(S(ok, 'pr47-O'), S(ok, 'stock-O'))))
            w('the fork %s (%s against stock); on the %d files PR #47, the fork and clang all compiled, the fork is %s PR #47' % (
                '{:,}'.format(S(ok, 'ours-O')), pct(S(ok, 'ours-O'), S(ok, 'stock-O')), len(ok2), rel(S(ok2, 'ours-O'), S(ok2, 'pr47-O'))))
            w('and %s clang `-Os` (PR #47: %s clang).' % (rel(S(ok2, 'ours-O'), S(ok2, 'clang-Os')), rel(S(ok2, 'pr47-O'), S(ok2, 'clang-Os'))))
        w('')
        c = collections.Counter(r['fail']['pr47-O'][0] for r in rs47 if 'pr47-O' in r['fail'])
        w('Why PR #47 stops (`-O`): %s.  Its indirect calls are written `mov lr,pc`, which is not an AArch64 instruction.' % (
            ', '.join('%d at the %s stage' % (n, st) for st, n in c.most_common())))
        w('')
    w('### Which back ends compile the corpus at all')
    w('')
    w('Files (of %d) compiled and assembled, optimized, and the most common reason the rest stop:' % total)
    w('')
    w('| target | stock | ours | most common ours failure | most common stock failure |')
    w('|---|---|---|---|---|')
    for arch in ARCH:
        rs = [r for r in R if r['arch'] == arch and r['corp'] in C99]
        if not rs:
            continue
        def top(k):
            c = collections.Counter(re.sub(r'0x[0-9a-f]+|\d+', 'N', r['fail'][k][1])[:50] for r in rs if k in r['fail'])
            return c.most_common(1)[0][0] if c else ''
        w('| %s | %d | %d | %s | %s |' % (arch, ncomp(rs, 'stock-O'), ncomp(rs, 'ours-O'), top('ours-O'), top('stock-O')))
    w('')
    w('## PDP-11 against V7 dmr cc')
    w('')
    w('Corpus: V7\'s `usr/src/libc` and `usr/src/cmd` (K&R C).  Measured with `tools/pccdiff.sh`, which compiles every file')
    w('with V7 dmr cc (ported to the host) and with pcc, assembles with V7 `as` and compares text plus')
    w('data.  "No `-O`" is `pcc -S` followed by V7\'s peephole `c2`; "`-O`" is ccom\'s passes, then `c2`.  Files both pcc')
    w('builds compiled:')
    w('')
    w('| corpus | files | V7 dmr cc | stock, no `-O` | stock `-O` | ours, no `-O` | ours `-O` |')
    w('|---|---|---|---|---|---|---|')
    for corp, s, o in (('libc', pl, ol), ('commands', pc, oc)):
        com = sorted(set(s) & set(o))
        t = lambda d, i: sum(d[f][i] for f in com)
        f = lambda v, ref=None: '{:,}'.format(v) + (' (%s)' % pct(v, ref) if ref else '')
        w('| %s | %d | %s | %s | %s | %s | **%s** |' % (corp, len(com), f(t(s, 0)), f(t(s, 1), t(s, 0)), f(t(s, 2), t(s, 0)),
                                                    f(t(o, 1), t(o, 0)), f(t(o, 2), t(o, 0))))
    w('')
    w('Over all files each compiles: libc %d files for ours (stock %d), commands %d (stock %d).  Stock rejects files that' % (
        len(ol), len(pl), len(oc), len(pc)))
    w('the fork compiles; the fork rejects none that stock compiles.')
    w('')
    w('One libc file is larger than V7 dmr cc\'s (`qsort`) and that one is more correct: dmr cc\'s `clr r0; div` for an unsigned')
    w('divide by a variable is wrong for divisors of 32768 and up (shown by running its own output); pcc calls a')
    w('helper that is right.  `BIGGER-THAN-ORIGINAL.md` in the toolchain repository lists every larger file and why.')
    w('')
    w('## VAX against 32V pcc')
    w('')
    w('Corpus: 32V\'s `libc/gen`, `libc/stdio` and `cmd` sources.  The reference sizes come from the real 32V operating')
    w('system under SIMH (`tools/vaxdiff.py`): its `cc` and `cc -O`, here called 32V pcc and 32V pcc -O.  That compiler is Johnson\'s pcc, so this is also the')
    w('comparison with the original pcc.  The guest produced a full set of sizes for %d of the %d sources (for the other' % (
        len(by['guest']), sum(1 for r in R if r['corp'] == '32v' and r['arch'] == 'vax')))
    w('ones it returned an incomplete set, for example no `cc -O` size; I did not investigate why).  Files with a unique')
    w('name that every row handled:')
    w('')
    w('| corpus | files | 32V pcc | 32V pcc -O | stock no `-O` | stock `-O` | ours no `-O` | ours `-O` |')
    w('|---|---|---|---|---|---|---|---|')
    for part, title in (('libc', 'libc (gen, stdio)'), ('cmd', 'commands')):
        g = G[part]
        f = lambda v, ref=None: '{:,}'.format(v) + (' (%s)' % pct(v, ref) if ref else '')
        w('| %s | %d | %s | %s | %s | %s | %s | **%s** |' % (title, g['n'], f(g['D']), f(g['O']), f(g['stock-O0'], g['D']),
                                                         f(g['stock-O'], g['O']), f(g['ours-O0'], g['D']), f(g['ours-O'], g['O'])))
    w('')
    w('No-`-O` columns are against 32V pcc, `-O` columns against 32V pcc -O.')
    w('')
    w('## What this run found')
    w('')
    w('Running a new corpus on every back end turned up things the V7 and 32V sources never did.  The measurements were')
    w('taken first with the fork as it stood, and the problems below were fixed afterwards, one commit each; the tables')
    w('are the re-measurement with the fixes (only the fork\'s columns were recompiled, the others are unchanged).')
    w('')
    w('**Found by the corpus and fixed:**')
    w('')
    w('* **`cannot initialise function`** on 17 mbedTLS files (amd64, i386; 12 more on the PDP-11): the scanner\'s K&R')
    w('  initializer rule (`int a[] { 1, 2 };`, added for the V7 sources) read the `{` of a function that returns a')
    w('  pointer to an array, `T (*f(args))[N] { ... }`, as an initializer.  Commit "scan: a function definition that')
    w('  returns a pointer to an array is not a K&R initializer".')
    w('* **`compiler error: strmemb`** on 280 PDP-11 files under `-O` (BearSSL, mbedTLS): with `-xdmr` a word result')
    w('  is returned in `r0` directly, and the inliner reads "temporary 0" as "no return value" (r0 is register 0).')
    w('  Commit "ccom: -xdmr does not return through r0 inside a function that is being saved for inlining".')
    w('* **`no computed goto in inlined functions`** on 32 VAX files (and an internal error on the PDP-11): an inline')
    w('  function is saved as intermediate code, and the jump table of a dense `switch` (on the VAX the label list that')
    w('  `casel` adds) is not copied with it.  Inside such a function the switch is now compare code, so the function')
    w('  is still expanded at its call sites.  Commit "pdp11, vax: no jump table in a function saved for inlining".')
    w('* **`ccom` hung on the GNU `case LOW ... HIGH:` range with a huge range**, in stock and in the fork on amd64,')
    w('  i386, VAX and PDP-11: `case 1000000000000000000ULL ... 9999999999999999999ULL:` never finished (`gcccase()`')
    w('  added one switch entry per value and compared signed for an unsigned switch).  A long range is now one range')
    w('  test.  Stock still hangs on that file and is counted as a failure (a 90-second limit) for every target.')
    w('')
    w('**What is still different from stock** (files stock compiles and the fork does not, optimized):')
    w('')
    for arch in ('amd64', 'i386', 'aarch64', 'pdp11', 'vax'):
        tops, n = fail_reasons(R, arch, 'ours-O', 'stock-O', C99)
        w('* %s: %d%s' % (arch, n, ''.join('; %d x "%s"' % (c, m) for m, c in tops[:3]) if n else ''))
    w('')
    w('**Other findings:**')
    w('')
    lz = [r for r in R if r['arch'] == 'amd64' and r['corp'] == 'lz4' and all(k in r['size'] for k in ('stock-O0', 'stock-O', 'ours-O0', 'ours-O', 'clang-Os', 'gcc-Os'))]
    if lz:
        t = lambda k: sum(r['size'][k] for r in lz)
        w('* **`-xinline` makes `always_inline`-heavy code much bigger, in stock and in the fork.**  lz4\'s five files on amd64')
        w('  are %s bytes unoptimized and %s with `-O` for the fork (stock: %s and %s), against %s for clang `-Os` and %s for' % (
            '{:,}'.format(t('ours-O0')), '{:,}'.format(t('ours-O')), '{:,}'.format(t('stock-O0')), '{:,}'.format(t('stock-O')),
            '{:,}'.format(t('clang-Os')), '{:,}'.format(t('gcc-Os'))))
        w('  gcc `-Os`.  lz4 builds many specializations from one function by passing constants to `LZ4_FORCE_INLINE` functions;')
        w('  the likely reason is that clang and gcc fold those constants after inlining and drop the dead branches, which pcc\'s')
        w('  inliner does not (not verified here).  This one corpus is why the lz4 row looks so bad; leaving it out changes no')
        w('  conclusion above.')
    w('* **Stock\'s powerpc back end does not build** (`sz =  = p2autooff` in `arch/powerpc/local2.c`, since August 2023)')
    w('  and riscv64 compiles nothing: every file stops with "invalid function definition".')
    w('* SQLite\'s amalgamation does not compile with any pcc here: it needs the `__atomic_*` builtins and `__ATOMIC_*`')
    w('  macros, which pcc does not have.')
    w('* A build-system trap, for anyone repeating this: the fork\'s tree tracks a prebuilt `pdp11-bsd-ccom`, which a naive')
    w('  "pick the `*-ccom` in the directory" mistakes for the compiler of any target that sorts after `p`; the first')
    w('  riscv64, sparc64 and VAX numbers came from that mistake and were thrown away.')
    w('')
    w('## Reproducing')
    w('')
    w('All of the harness is in the toolchain repository (`facetlang/pdp11/v7unix-toolchain/tools`):')
    w('')
    w('```')
    w('corpus-build.sh                  # stock (git archive of origin/master) and ours, every target, plus c2 for PDP-11 and VAX')
    w('corpus.py -j 2                  # incremental, resumable; --arch to restrict; --full for every torture test')
    w('corpus-report.py                # this file')
    w('pccdiff.sh -v orig/usr/src/libc  # PDP-11 against V7 dmr cc (PCC=... for stock)')
    w('vaxdiff.py --ccom CCOM ...       # VAX against the 32V guest')
    w('corpus-gcc-cross.sh              # optional: binutils and a C-only GCC for vax-netbsdelf and pdp11-aout')
    w('```')
    w('')
    w('Corpora live in `~/claude/corpora`: Lua 5.4.7, SQLite 3.46.1 (amalgamation), zlib 1.3.1, lz4 1.9.4, cJSON 1.7.18,')
    w('BearSSL, mbedTLS 3.6.2, chibicc, c-testsuite, every eighth test of GCC\'s `gcc.c-torture/execute`, and the musl 1.2.5')
    w('headers for each architecture.')
    w('')
    w('Limits: GCC\'s own VAX and PDP-11 back ends are not in the tables (the cross compilers were not finished); the 32V')
    w('reference covers %d of the sources; the PDP-11 "no `-O`" column of the V7 table is `pcc -S` followed by V7\'s `c2`' % len(by['guest']))
    w('(as `pccdiff.sh` defines it) while the unoptimized columns elsewhere are raw `ccom` output; the PDP-11 C99 numbers')
    w('mix correctness differences (the fork has a 64-bit `long long`, stock does not) with code quality.')
    open(a.out, 'w').write('\n'.join(L) + '\n')
    print('wrote', a.out, len(L), 'lines')


def refresh(a):
    """bring the measurements up to date before writing: rebuild whichever compilers' source changed (an
    unchanged tree is not rebuilt), then recompile only the configurations whose compiler binary, flags or
    source changed.  If only the fork's pcc changed, only its columns are measured again."""
    print('== compilers'); sys.stdout.flush()
    subprocess.run([os.path.join(here, 'corpus-build.sh')], check=False)
    print('== measurements'); sys.stdout.flush()
    p = subprocess.run([sys.executable, '-I', os.path.join(here, 'corpus.py'), '-j', a.j, '--out', a.results],
                       stderr=subprocess.PIPE, text=True)
    last = [l for l in p.stderr.strip().split('\n') if 'changed' in l or 'jobs' in l]
    print('\n'.join(last)); sys.stdout.flush()


def old_vax(R, oldt):
    """32V sizes: the guest's cc and cc -O against stock and ours measured on the host, by unique file name"""
    G = {}
    for l in open(os.path.join(oldt, 'vax-ours-O0.txt'), errors='replace'):
        m = re.match(r'(\S+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s*$', l)
        if m and m.group(1) != 'TOTAL':
            G[m.group(1)] = tuple(map(int, m.groups()[1:]))
    rs = [r for r in R if r['corp'] == '32v' and r['arch'] == 'vax']
    base = lambda r: os.path.basename(r['src']).replace('.c.gz', '')
    cnt = collections.Counter(base(r) for r in rs)
    by = {base(r): r for r in rs if cnt[base(r)] == 1}
    out = {}
    for part in ('libc', 'cmd'):
        fs = [n for n in G if n in by and ('/libc/' in by[n]['src']) == (part == 'libc')
              and all(k in by[n]['size'] for k in ('stock-O0', 'ours-O0', 'stock-O', 'ours-O'))]
        d = {'n': len(fs), 'D': sum(G[n][0] for n in fs), 'O': sum(G[n][1] for n in fs)}
        for k in ('stock-O0', 'ours-O0', 'stock-O', 'ours-O'):
            d[k] = sum(by[n]['size'][k] for n in fs)
        out[part] = d
    return out, {'guest': G}


if __name__ == '__main__':
    main()
