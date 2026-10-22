#!/usr/bin/env python3
"""
pdp64size -- instruction counts, .text bytes and instruction-length statistics of the pdp64 pcc back end,
next to our amd64 and aarch64 (and i386) on the same files.

    pdp64size.py [--set fair|pcctests|all] [--arch pdp64,amd64,aarch64,i386] [-j N] [--out DIR] [--nowrap] [files...]

The "fair corpus" is the set of files that tools/corpus.py's aarch64, amd64 and i386 builds all compile with
`ours -O` (taken from RESULTS, default /tmp/co-final/results.jsonl); the sources, the include directories and the
defines are corpus.py's, and pdp64 gets the same preprocessed text as aarch64.  Per file and architecture:
  instructions   non-directive, non-label lines of the .s
  text           bytes of the .text section of the object
For pdp64 also: the length of every instruction (the `len` bits of its first parcel), the mnemonic and the shape
of each operand, so the tables of the report (lengths, mnemonics, addressing modes, pairs) can be made.
Files that fail to compile or assemble are listed with the first error, grouped by cause.
"""
import os, re, sys, json, glob, struct, argparse, tempfile, subprocess, collections
import concurrent.futures as cf

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import corpus
from corpus import ROOT, ARCH, CORPORA, BUILDS, run, ccomflags, cfiles

TOOL = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RESULTS = os.environ.get('RESULTS', '/tmp/co-final/results.jsonl')
MNEM = os.path.expanduser('~/claude/pdp64/spec/pdp64-mnemonics.hbnf')

CCOMFLAGS_NOSSA = '-xdmr -xtemps -xdeljumps -xinline -xdce'
COMMENT = {'pdp64': ';', 'amd64': '#', 'i386': '#', 'aarch64': '//'}


def fair_files():
    by = collections.defaultdict(dict)
    for l in open(RESULTS):
        r = json.loads(l)
        by[(r['corp'], r['src'])][r['arch']] = r
    out = []
    for (corp, src), v in sorted(by.items()):
        if all(a in v and 'ours-O' in v[a]['size'] for a in ('aarch64', 'amd64', 'i386')):
            out.append((corp, os.path.join(ROOT, src)))
    return out


def pcctests():
    return [('pcctests', f) for f in sorted(glob.glob(TOOL + '/tools/pcctests/*.c'))]


def preprocess(corp, src, arch, extra):
    a = ARCH[arch]
    c = CORPORA[corp] if corp in CORPORA else dict(inc=[], defs=[])
    incs = []
    for i in c['inc']:
        incs += ['-I', os.path.join(ROOT, i)]
    incs += ['-I', os.path.dirname(src)]
    if corp == 'pcctests':
        # as pcctest.sh: K&R-ish sources, no system headers needed
        cmd = ['gcc', '-E', '-P', '-undef', '-D__pcc__'] + a['cpp'] + incs + [src]
    else:
        sysinc = os.path.join(ROOT, 'sysroot', a['musl'], 'include')
        std = ['-std=gnu99', '-D__STDC_VERSION__=199901L', '-D__STDC_HOSTED__=1']
        cmd = ['gcc', '-E', '-P', '-undef', '-nostdinc'] + std + a['cpp'] + c['defs'] + incs + ['-isystem', sysinc, src]
    return run(cmd)


def sections(obj):
    """name -> (offset, size) of the ELF little-endian sections (32- or 64-bit)"""
    d = open(obj, 'rb').read()
    if d[4] == 2:
        shoff, = struct.unpack_from('<Q', d, 0x28)
        shentsize, shnum, shstrndx = struct.unpack_from('<HHH', d, 0x3A)
        fmt = '<IIQQQQ'
    else:
        shoff, = struct.unpack_from('<I', d, 0x20)
        shentsize, shnum, shstrndx = struct.unpack_from('<HHH', d, 0x2E)
        fmt = '<IIIIII'
    sh = []
    for i in range(shnum):
        name, typ, flags, addr, off, size = struct.unpack_from(fmt, d, shoff + i * shentsize)
        sh.append((name, off, size))
    stroff = sh[shstrndx][1]
    out = {}
    for name, off, size in sh:
        e = d.index(b'\0', stroff + name)
        out[d[stroff + name:e].decode()] = (off, size)
    return d, out


# ---- pdp64 operand shapes ------------------------------------------------------------------------------------------
REG = r'(?:r\d+|sp|fp)'


def shape(op):
    op = op.strip()
    if re.fullmatch(REG, op):
        return 'reg'
    if op.startswith('#'):
        return 'imm'
    if op.startswith('@'):
        return 'abs'
    if re.fullmatch(r'\(%s\)\+.*' % REG, op):
        return 'postinc'
    if re.fullmatch(r'-\(%s\)|\(%s\)-.*' % (REG, REG), op):
        return 'predec'
    if re.fullmatch(r'.*\(%s\)\(%s(\*\d)?\)' % (REG, REG), op):
        return 'index'
    if re.fullmatch(r'.*\((?:%s)\)' % REG, op):
        return 'base'
    return 'pcrel'      # a bare symbol or label


def split_ops(s):
    out, depth, cur = [], 0, ''
    for ch in s:
        if ch == '(':
            depth += 1
        elif ch == ')':
            depth -= 1
        if ch == ',' and depth == 0:
            out.append(cur)
            cur = ''
        else:
            cur += ch
    if cur.strip():
        out.append(cur)
    return out


def parse_s(path, arch):
    """the instruction lines of an assembly file: (section, mnemonic, [operands]); directives and labels skipped"""
    com = COMMENT[arch]
    sec = 'text'
    ins = []
    for line in open(path, errors='replace'):
        line = line.split(com)[0] if arch != 'amd64' and arch != 'i386' else line.split('#')[0]
        line = line.strip()
        while True:
            m = re.match(r'^("[^"]*"|[A-Za-z_.$][\w.$]*|\d+):\s*(.*)$', line)
            if not m:
                break
            line = m.group(2).strip()
        if not line:
            continue
        if line.startswith('.'):
            w = line.split()[0]
            if w in ('.byte', '.word', '.long', '.quad', '.space', '.even', '.align', '.ascii') and sec == 'text':
                rest = line[len(w):].strip()
                ins.append((sec, w, split_ops(rest) if rest else []))
                continue
            if w in ('.text',):
                sec = 'text'
            elif w in ('.data',):
                sec = 'data'
            elif w in ('.bss',):
                sec = 'bss'
            elif w == '.section':
                sec = 'text' if '.text' in line else 'data'
            continue
        w = line.split(None, 1)
        ins.append((sec, w[0], split_ops(w[1]) if len(w) > 1 else []))
    return ins


def count_ins(path, arch):
    return sum(1 for s, m, o in parse_s(path, arch) if s == 'text' and not m.startswith('.'))


def walk_pdp64(obj, ins):
    """pair every instruction line of .text with its encoded length"""
    d, secs = sections(obj)
    off, size = secs['.text']
    text = d[off:off + size]
    pos = 0
    out = []
    for sec, mn, ops in ins:
        if sec != 'text':
            continue
        if mn.startswith('.'):
            if mn == '.even':
                pos += pos & 1
            elif mn == '.align':
                n = int(ops[0])
                pos += (-pos) % n
            elif mn == '.space':
                pos += int(ops[0])
            elif mn == '.ascii':
                pos += sum(len(bytes(x.strip().strip('"'), 'utf-8').decode('unicode_escape')) for x in ops)
            else:
                pos += {'.byte': 1, '.word': 2, '.long': 4, '.quad': 8}[mn] * len(ops)
            continue
        if pos + 2 > len(text):
            break
        p, = struct.unpack_from('<H', text, pos)
        ln = (p >> 14) + 1
        out.append((mn, ops, ln * 2))
        pos += 2 * ln
    return out, (pos == len(text))


def text_bytes(obj):
    d, secs = sections(obj)
    return secs.get('.text', (0, 0))[1]


def image_bytes(obj):
    """text + read-only data + data, as `size` counts them (no bss): the sections a loader would map with contents"""
    d, secs = sections(obj)
    return sum(v[1] for k, v in secs.items() if k in ('.text', '.data', '.rodata') or k.startswith(('.text.', '.rodata.', '.data.')))


def one(job, nowrap, keep):
    corp, src, arch = job
    a = ARCH[arch]
    res = dict(corp=corp, src=src.replace(ROOT + '/', ''), arch=arch)
    ccom = os.path.join(BUILDS, 'ours', arch, 'ccom')
    if arch == 'pdp64' and os.environ.get('PDP64_CCOM'):
        ccom = os.environ['PDP64_CCOM']          # a scratch build instead of the committed one
    with tempfile.TemporaryDirectory() as w:
        r = preprocess(corp, src, arch, [])
        if r.returncode:
            res['fail'] = ('cpp', r.stderr.strip().split('\n')[0][:160])
            return res
        i = os.path.join(w, 'i.c')
        open(i, 'w').write(r.stdout)
        s, o = os.path.join(w, 'x.s'), os.path.join(w, 'x.o')
        fl = ccomflags(arch, True).split()
        if arch == 'pdp64' and nowrap:
            fl.append('-mnowrap')
        try:
            p = run([ccom] + fl + [i, s], timeout=corpus.CCOM_TIMEOUT)
        except subprocess.TimeoutExpired:
            res['fail'] = ('ccom', 'timeout')
            return res
        if p.returncode or not os.path.exists(s):
            m = [x for x in (p.stderr or p.stdout).strip().split('\n') if x.strip()]
            e = [x for x in m if 'warning' not in x]
            msg = e[0] if e else (m[0] if m else 'exit %d' % p.returncode)
            msg = re.sub(r'^<stdin>, line \d+: ', '', msg)
            msg = re.sub(r'0x[0-9a-f]+', '0x', msg)
            res['fail'] = ('ccom', msg[:200] + (' (signal %d)' % -p.returncode if p.returncode < 0 else ''))
            return res
        asm = [os.environ['AS64']] if arch == 'pdp64' and os.environ.get('AS64') else a['asm']
        q = run(asm + ['-o', o, s])
        if q.returncode:
            msg = ((q.stderr + q.stdout).strip().split('\n') or [''])[0]
            msg = re.sub(r'^\S*x\.s:\d+: ', '', msg)
            msg = re.sub(r'\d+:\d+:\d+: ', '', msg)
            res['fail'] = ('asm', msg[:200])
            if keep:
                os.makedirs(keep, exist_ok=True)
                os.replace(s, os.path.join(keep, res['src'].replace('/', '_') + '.s'))
            return res
        res['ins'] = count_ins(s, arch)
        res['text'] = text_bytes(o)
        res['image'] = image_bytes(o)
        if arch == 'pdp64':
            ins = parse_s(s, arch)
            walked, ok = walk_pdp64(o, ins)
            res['walk_ok'] = ok
            hist = collections.Counter()
            mn = collections.Counter()
            pair = collections.Counter()
            modes = collections.Counter()
            modelen = collections.Counter()
            longs = collections.Counter()
            for m, ops, ln in walked:
                sh = tuple(shape(x) for x in ops)
                hist[ln] += 1
                mn[m] += 1
                pair['%s %s' % (m, ','.join(sh))] += 1
                for x in sh:
                    modes[x] += 1
                    modelen['%s/%d' % (x, ln)] += 1
                if ln >= 6:
                    longs['%d %s %s' % (ln, m, ','.join(sh))] += 1
            res['hist'] = hist
            res['mn'] = mn
            res['pair'] = pair
            res['modes'] = modes
            res['modelen'] = modelen
            res['longs'] = longs
            if keep:
                os.makedirs(keep, exist_ok=True)
                os.replace(s, os.path.join(keep, res['src'].replace('/', '_') + '.s'))
    return res


ALIASES = set("""bhis blo mulu wait rol faddd fsubd fmuld fcmpd fdivd fsqrtd fadds fsubs fmuls fdivs fsqrts fcmps fmaddd fmsubd fnmaddd fnmsubd
fmadds fmsubs fnmadds fnmsubs fmind fmaxd fclassd fclasss frintd frintdz frintdm frintdp frintda""".split())


def all_mnemonics():
    """the instruction mnemonics of the grammar (the mn-* rules, not registers, modes or condition names), without
    the aliases that only spell another word"""
    names, rule = set(), None
    for line in open(MNEM):
        if line.startswith(';') or not line.strip():
            continue
        m = re.match(r'^([a-z0-9-]+)\s*=', line)
        if m:
            rule = m.group(1)
        if rule and rule.startswith('mn-'):
            names.update(re.findall(r'"([a-z0-9_]+)"\s*@', line))
    names.add('set')
    return sorted(names - ALIASES)


# mnemonics a C compiler does not emit without an intrinsic, an idiom pass or a library
INTRINSIC = {
    'clz', 'ctz', 'clo', 'cto', 'popcnt', 'bswap', 'bswapw', 'bswapl', 'bitrev', 'ror', 'rori', 'rol', 'dshr', 'extb', 'extzb', 'insb',
    'xnorpop', 'clmull', 'clmulh', 'utf8len', 'utf8dec', 'utf8enc',
    'cas', 'casdb', 'swap', 'swapdb', 'xadd', 'xadddb', 'xand', 'xanddb', 'xor_', 'xor_db', 'xxor', 'xxordb', 'xmax', 'xmaxdb', 'xmin',
    'xmindb', 'xmaxu', 'xmaxudb', 'xminu', 'xminudb', 'll', 'sc',
    'mmov', 'mset', 'mcmp', 'mscn', 'mscnn', 'mval', 'mcpy',
    'mulhu', 'btag', 'bntag', 'jtag',
    'mtps', 'mfps', 'spl', 'trap', 'tlbi', 'mtasid', 'mfasid', 'mtdp', 'mfdp', 'mtdmw', 'mfcpuid', 'icinv', 'prober', 'probew', 'trapl',
    'rti', 'rtt', 'halt', 'bpt', 'fence', 'wfe', 'sev', 'lpad', 'tlbia', 'tlbsync', 'icinva', 'wait',
    'fclass', 'fclass32', 'fcopysign', 'fmin', 'fmax', 'fabs', 'fsflags', 'fmadd', 'fmadd32', 'fmsub', 'fmsub32', 'fnmadd', 'fnmadd32',
    'fnmsub', 'fnmsub32', 'frint', 'frinta', 'frintz', 'frintm', 'frintp', 'fsqrt', 'fsqrt32', 'abs', 'min', 'minu', 'max', 'maxu', 'jmpi',
    'movub', 'movuw', 'movul', 'movuq', 'addm', 'subm', 'bicm', 'bism',
    'pushm', 'popm',
}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--set', default='fair', help='fair, pcctests, all (both)')
    ap.add_argument('--arch', default='pdp64,amd64,aarch64,i386')
    ap.add_argument('-j', type=int, default=os.cpu_count() or 2)
    ap.add_argument('--out', default='/tmp/pdp64size-out')
    ap.add_argument('--limit', type=int, default=0)
    ap.add_argument('--nowrap', action='store_true', help='pass -mnowrap: signed int arithmetic does not overflow')
    ap.add_argument('--keep', default='', help='keep the pdp64 .s files here')
    ap.add_argument('files', nargs='*')
    a = ap.parse_args()
    if a.files:
        files = [(next((c for c in CORPORA if os.path.abspath(f).startswith(os.path.join(ROOT, c.split('-')[0]))), 'other'), os.path.abspath(f)) for f in a.files]
    else:
        files = []
        if a.set in ('fair', 'all'):
            files += fair_files()
        if a.set in ('pcctests', 'all'):
            files += pcctests()
    if a.limit:
        files = files[:a.limit]
    archs = a.arch.split(',')
    jobs = [(c, f, ar) for c, f in files for ar in archs]
    os.makedirs(a.out, exist_ok=True)
    results = []
    with cf.ThreadPoolExecutor(max_workers=a.j) as ex:
        futs = [ex.submit(one, j, a.nowrap, a.keep) for j in jobs]
        for n, f in enumerate(cf.as_completed(futs)):
            results.append(f.result())
            if n % 200 == 0:
                sys.stderr.write('%d / %d\n' % (n, len(jobs)))
    json.dump(results, open(os.path.join(a.out, 'results.json'), 'w'))
    report(results, archs, a.out)


def report(results, archs, out):
    L = []
    by = collections.defaultdict(dict)
    for r in results:
        by[(r['corp'], r['src'])][r['arch']] = r
    # fair: files every requested architecture compiled
    ok = {k: v for k, v in by.items() if all(ar in v and 'ins' in v[ar] for ar in archs)}
    L.append('Files: %d; compiled and assembled by all of %s: %d' % (len(by), ','.join(archs), len(ok)))
    L.append('')
    L.append('## Totals over the files all architectures compiled')
    L.append('')
    L.append('"text" is text+read-only data+data (what `size` counts as text+data, no bss), so that pdp64, which keeps its constants in .text, is compared fairly.')
    L.append('')
    L.append('| corpus | files | ' + ' | '.join('%s ins | %s bytes' % (x, x) for x in archs) + ' | pdp64/amd64 ins | pdp64/amd64 bytes | pdp64/aarch64 ins | pdp64/aarch64 bytes |')
    L.append('|---|---|' + '---|' * (2 * len(archs) + 4))
    corps = sorted(set(k[0] for k in ok))

    def tot(keys, ar, f):
        return sum(ok[k][ar][f] for k in keys)
    for c in corps + ['**fair corpus (without pcctests)**', '**all**']:
        keys = [k for k in ok if c == '**all**' or k[0] == c or (c.startswith('**fair') and k[0] != 'pcctests')]
        cells = []
        for ar in archs:
            cells += [str(tot(keys, ar, 'ins')), str(tot(keys, ar, 'image'))]
        rat = []
        for o in ('amd64', 'aarch64'):
            if 'pdp64' in archs and o in archs:
                rat += ['%.2f' % (tot(keys, 'pdp64', 'ins') / tot(keys, o, 'ins')), '%.2f' % (tot(keys, 'pdp64', 'image') / tot(keys, o, 'image'))]
            else:
                rat += ['', '']
        L.append('| %s | %d | %s | %s |' % (c, len(keys), ' | '.join(cells), ' | '.join(rat)))
    L.append('')
    # failures
    for ar in archs:
        bad = [r for r in results if r['arch'] == ar and 'fail' in r]
        L.append('## %s: %d files failed' % (ar, len(bad)))
        g = collections.Counter(tuple(r['fail']) for r in bad)
        for (st, msg), k in g.most_common(40):
            ex = [r['src'] for r in bad if tuple(r['fail']) == (st, msg)][:2]
            L.append('* %d x %s: %s   (e.g. %s)' % (k, st, msg, ', '.join(ex)))
        L.append('')
    # pdp64 statistics, over every pdp64 file that compiled
    pd = [r for r in results if r['arch'] == 'pdp64' and 'ins' in r]
    if pd:
        H, MN, PR, MO, ML, LG = (collections.Counter() for _ in range(6))
        for r in pd:
            H.update({int(k): v for k, v in r['hist'].items()})
            MN.update(r['mn'])
            PR.update(r['pair'])
            MO.update(r['modes'])
            ML.update(r['modelen'])
            LG.update(r['longs'])
        n = sum(H.values())
        badwalk = [r['src'] for r in pd if not r.get('walk_ok')]
        if badwalk:
            L.append('(!) %d files whose .text did not walk out exactly: %s' % (len(badwalk), ', '.join(badwalk[:5])))
        by_bytes = sum(k * v for k, v in H.items())
        L.append('## pdp64 instruction lengths (%d files, %d instructions, %d bytes, %.3f bytes per instruction)' % (len(pd), n, by_bytes, by_bytes / n))
        L.append('')
        L.append('| bytes | instructions | share |')
        L.append('|---|---|---|')
        for k in sorted(H):
            L.append('| %d | %d | %.1f%% |' % (k, H[k], 100.0 * H[k] / n))
        L.append('')
        L.append('### what makes the 6- and 8-byte forms (length, mnemonic, operand shapes): top 25')
        L.append('')
        for k, v in LG.most_common(25):
            L.append('* %d x %s' % (v, k))
        L.append('')
        tot_m = sum(MN.values())
        L.append('## mnemonics by static frequency')
        L.append('')
        L.append('| rank | mnemonic | count | share | cumulative |')
        L.append('|---|---|---|---|---|')
        cum = 0
        for i, (m, v) in enumerate(MN.most_common(), 1):
            cum += v
            L.append('| %d | %s | %d | %.2f%% | %.2f%% |' % (i, m, v, 100.0 * v / tot_m, 100.0 * cum / tot_m))
        L.append('')
        allm = all_mnemonics()
        zero = [m for m in allm if MN[m] == 0]
        L.append('### never emitted (%d of %d mnemonics)' % (len(zero), len(allm)))
        L.append('')
        L.append('Not emittable by a C compiler without an intrinsic, an idiom pass, a library call or system code: ' +
                 ', '.join(m for m in zero if m in INTRINSIC))
        L.append('')
        L.append('Could be emitted from plain C and are not (yet): ' + ', '.join(m for m in zero if m not in INTRINSIC))
        L.append('')
        tm = sum(MO.values())
        L.append('## operand addressing modes (by operand)')
        L.append('')
        L.append('| mode | operands | share | 2 bytes | 4 | 6 | 8 |')
        L.append('|---|---|---|---|---|---|---|')
        for mo, v in MO.most_common():
            L.append('| %s | %d | %.1f%% | %d | %d | %d | %d |' % (mo, v, 100.0 * v / tm, ML['%s/2' % mo], ML['%s/4' % mo], ML['%s/6' % mo], ML['%s/8' % mo]))
        L.append('')
        L.append('## the 20 most frequent (mnemonic, operand shapes)')
        L.append('')
        for k, v in PR.most_common(20):
            L.append('* %d (%.1f%%)  %s' % (v, 100.0 * v / n, k))
        L.append('')
    open(os.path.join(out, 'report.md'), 'w').write('\n'.join(L) + '\n')
    print('\n'.join(L))


if __name__ == '__main__':
    main()
