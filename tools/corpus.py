#!/usr/bin/env python3
"""
corpus -- compile real C99 projects with each pcc backend and compare code size.

    corpus.py [--arch aarch64,amd64,i386] [--only lua,sqlite] [--flags "..."] [-j N] [--out DIR]

For each source file of each corpus, and each architecture: preprocess with the
host gcc against that architecture's musl headers (no GNU extensions are defined
for the headers), compile with the architecture's pcc ccom, assemble, and count
text+data with `size`.  The same preprocessed file is compiled by clang -Os (all
three architectures) and gcc -Os (x86) with unwind tables off, so the numbers
are comparable.  Files that pcc rejects or crashes on are listed with the first
line of the message, grouped.

Needs: the ccoms from tools/corpus-build.sh (stock = PortableCC master, ours = this branch), the
corpora under ~/claude/corpora, and musl headers installed in ~/claude/corpora/sysroot/<arch>.
"""
import os, re, sys, glob, json, shutil, hashlib, subprocess, tempfile, argparse, collections
import concurrent.futures as cf

ROOT = os.environ.get('CORPORA', os.path.expanduser('~/claude/corpora'))

CCOM_TIMEOUT = 90      # seconds; a compiler that has not finished by then is hung (see the GNU case-range test)
BUILDS = os.path.join(ROOT, 'builds')
GCCX = os.path.join(ROOT, 'gcc-cross', 'prefix')      # tools/corpus-gcc-cross.sh
# pcc targets that clang can also compile for.  asm: how to assemble pcc's output; clang: the reference compiler.
# ssa: pcc's -O includes -xssa on Linux targets but not on the BSD ones (os/bsd/ccconfig.h: PCC_NO_SSA).
# Position-independent code is turned off everywhere so the references match pcc's absolute code.
def _t(triple, cpp, extra=(), asm=None, gcc=None, musl=None, ssa=True):
    return dict(musl=musl, cpp=cpp, ssa=ssa, gcc=gcc, c2=None,
                asm=asm or ['clang', '--target=' + triple, '-c', '-x', 'assembler'],
                clang=['clang', '--target=' + triple] + list(extra))

ARCH = {
    'aarch64': _t('aarch64-linux-gnu', ['-D__aarch64__', '-D__LP64__', '-D_LP64'], ['-fno-pic'], musl='aarch64'),
    'amd64': _t('x86_64-linux-gnu', ['-D__x86_64__', '-D__LP64__', '-D_LP64'], ['-fno-pic', '-fcf-protection=none'],
                asm=['gcc', '-m64', '-c', '-x', 'assembler'], gcc=['gcc', '-m64', '-fno-pie', '-fcf-protection=none'], musl='x86_64'),
    'i386': _t('i386-linux-gnu', ['-D__i386__'], ['-fno-pic', '-fcf-protection=none'],
               asm=['gcc', '-m32', '-c', '-x', 'assembler'], gcc=['gcc', '-m32', '-fno-pie', '-fcf-protection=none'], musl='i386'),
    'arm': _t('arm-linux-gnueabi', ['-D__arm__'], ['-fno-pic', '-marm'], musl='arm'),
    'powerpc': _t('powerpc-linux-gnu', ['-D__powerpc__', '-D_BIG_ENDIAN'], ['-fno-pic'], musl='powerpc'),
    'm68k': _t('m68k-linux-gnu', ['-D__m68k__'], ['-fno-pic'], musl='m68k'),
    'mips': _t('mips-linux-gnu', ['-D__mips__', '-D_MIPS_SIM=_MIPS_SIM_ABI32'], ['-fno-pic', '-mno-abicalls'], musl='mips'),
    'mips64': _t('mips64-linux-gnuabi64', ['-D__mips__', '-D__mips64', '-D__LP64__', '-D_LP64'], ['-fno-pic', '-mno-abicalls'], musl='mips64'),
    'riscv64': _t('riscv64-linux-gnu', ['-D__riscv', '-D__riscv_xlen=64', '-D__LP64__', '-D_LP64'], ['-fno-pic'], musl='riscv64', ssa=False),
    # the old targets: no clang or gcc, so only stock against ours.  Sizes come from the target's own
    # assembler (V7 as, 32V as as as32v) run on the host; -O output also goes through the target's c2.
    'pdp11': dict(musl='i386', cpp=['-Dpdp11', '-D__pdp11__'], ssa=False, clang=None, asm=None, c2='c2pdp11',
                  gcc=[GCCX + '/bin/pdp11-aout-gcc'], gccsize=GCCX + '/bin/pdp11-aout-size'),
    'vax': dict(musl='i386', cpp=['-Dvax', '-D__vax__'], ssa=False, clang=None, asm=None, c2='c2vax',
                gcc=[GCCX + '/bin/vax-netbsdelf-gcc'], gccsize=GCCX + '/bin/vax-netbsdelf-size'),
    'sparc64': _t('sparcv9-linux-gnu', ['-D__sparc__', '-D__sparc64__', '-D__LP64__', '-D_LP64'], ['-fno-pic'], musl='sparc64', ssa=False),
}


def cfiles(pat, excl=()):
    out = []
    for p in pat if isinstance(pat, list) else [pat]:
        out += glob.glob(os.path.join(ROOT, p), recursive=True)
    return sorted(f for f in out if not any(e in f for e in excl))


TOOL = os.path.expanduser('~/claude/facetlang/pdp11/v7unix-toolchain')
V32 = os.path.expanduser('~/claude/facetlang/unixtree/32V/usr/src')


def gzfiles(pat):
    return sorted(glob.glob(os.path.join(V32, pat)))


CORPORA = {
    # the corpora the size documents use, for the two old targets (dmr's and 32V's headers, K&R sources)
    'v7': dict(files=lambda: cfiles([TOOL + '/orig/usr/src/libc/**/*.c', TOOL + '/orig/usr/src/cmd/*.c']),
               inc=[], defs=['-Dunix'], arches=['pdp11'], sys=TOOL + '/orig/usr/include'),
    '32v': dict(files=lambda: gzfiles('libc/gen/*.c.gz') + gzfiles('libc/stdio/*.c.gz') + gzfiles('cmd/*.c.gz'),
                inc=[], defs=['-Dunix'], arches=['vax'], sys=os.path.join(ROOT, '32v-include')),
    'lua': dict(files=lambda: cfiles('lua-5.4.7/src/*.c'), inc=['lua-5.4.7/src'], defs=['-DLUA_USE_POSIX']),
    'sqlite': dict(files=lambda: cfiles('sqlite-amalgamation-3460100/sqlite3.c'),
                   inc=['sqlite-amalgamation-3460100'],
                   defs=['-DSQLITE_OMIT_LOAD_EXTENSION', '-DSQLITE_THREADSAFE=0', '-DSQLITE_OMIT_WAL']),
    'zlib': dict(files=lambda: cfiles('zlib/*.c', ['example', 'minigzip']), inc=['zlib'], defs=[]),
    'lz4': dict(files=lambda: cfiles('lz4/lib/*.c'), inc=['lz4/lib'], defs=[]),
    'cjson': dict(files=lambda: cfiles('cjson/cJSON.c') + cfiles('cjson/cJSON_Utils.c'), inc=['cjson'], defs=[]),
    'bearssl': dict(files=lambda: cfiles('bearssl/src/**/*.c'), inc=['bearssl/inc', 'bearssl/src'], defs=[]),
    'mbedtls': dict(files=lambda: cfiles('mbedtls/library/*.c'), inc=['mbedtls/include', 'mbedtls/library'], defs=[]),
    'chibicc': dict(files=lambda: cfiles('chibicc/*.c', ['main.c']), inc=['chibicc'], defs=[]),
    'c-testsuite': dict(files=lambda: cfiles('c-testsuite/tests/single-exec/*.c'), inc=[], defs=[]),
    'gcc-torture': dict(files=lambda: cfiles('gcc-src/gcc/testsuite/gcc.c-torture/execute/*.c'), inc=[], defs=[], stride=8),
}


def run(cmd, **kw):
    """run a command with a time limit; a command that does not finish is reported as exit 124"""
    kw.setdefault('timeout', 300)
    try:
        return subprocess.run(cmd, capture_output=True, text=True, errors='replace', **kw)
    except subprocess.TimeoutExpired:
        return subprocess.CompletedProcess(cmd, 124, '', 'timeout')


def size_of(obj, tool='size'):
    r = run([tool, obj])
    try:
        a = r.stdout.strip().split('\n')[1].split()
        return int(a[0]) + int(a[1])
    except Exception:
        return None


def ahdr_size(obj, wordsz):
    """text+data from a V7/32V a.out header: 16-bit words for the PDP-11, 32-bit for the VAX"""
    try:
        d = open(obj, 'rb').read(32)
        if wordsz == 2:
            return int.from_bytes(d[2:4], 'little') + int.from_bytes(d[4:6], 'little')
        return int.from_bytes(d[4:8], 'little') + int.from_bytes(d[8:12], 'little')
    except Exception:
        return None


def old_assemble(arch, s, o, w):
    """assemble s with the target's own assembler on the host; returns (size, error)"""
    if arch == 'pdp11':
        env = dict(os.environ, AS2=TOOL + '/modern/usr/src/cmd/as/as2')
        q = run([TOOL + '/modern/usr/src/cmd/as/as', '-u', '-o', o, s], env=env)
        return (ahdr_size(o, 2), None) if q.returncode == 0 and os.path.exists(o) else (None, (q.stdout + q.stderr).strip().split('\n')[0][:200])
    q = run([TOOL + '/modern/usr/src/cmd/as32v/as32v', '-o', o, s])
    return (ahdr_size(o, 4), None) if q.returncode == 0 and os.path.exists(o) else (None, (q.stdout + q.stderr).strip().split('\n')[0][:200])


REGS = {'ap', 'fp', 'sp', 'pc'}
ESC = {'n': 10, 't': 9, 'r': 13, 'b': 8, 'f': 12, 'v': 11, 'a': 7, '\\': 92, '"': 34, "'": 39}


def _bytes(lit):
    out, i = [], 0
    while i < len(lit):
        c = lit[i]
        if c == '\\' and i + 1 < len(lit):
            n = lit[i + 1]
            if n in '01234567':
                j = i + 1
                while j < len(lit) and j < i + 4 and lit[j] in '01234567':
                    j += 1
                out.append(int(lit[i + 1:j], 8) & 255)
                i = j
                continue
            out.append(ESC.get(n, ord(n)))
            i += 2
        else:
            out.append(ord(c))
            i += 1
    return out


def _sym(m):
    t = m.group(0)
    if t in REGS or re.fullmatch(r'r\d+', t) or re.fullmatch(r'L\d+', t):
        return t
    return '_' + t


def gnu_to_v7(path):
    """stock pcc writes GNU assembler syntax for the VAX; 32V's as and c2 want the same program spelled the
    old way: registers without %, L<n> local labels, symbols with a leading underscore (without it a name
    like `index` is an instruction), .comm without an alignment, no .local/.type/.size, strings as .byte.
    Spelling only; no instruction changes."""
    out = []
    for line in open(path, errors='replace').read().split('\n'):
        line = line.replace('%', '')
        line = re.sub(r'\.L(\d+)', r'L\1', line)
        m = re.match(r'^\s*\.(type|size|ident|file|section|p2align|local)\b', line)
        if m:
            continue
        m = re.match(r'^\s*\.(ascii|asciz|string)\s+"(.*)"\s*$', line)
        if m:
            b = _bytes(m.group(2)) + ([0] if m.group(1) != 'ascii' else [])
            for k in range(0, len(b), 16):
                out.append('\t.byte ' + ','.join(str(x) for x in b[k:k + 16]))
            continue
        m = re.match(r'^\s*(\.l?comm\s+[\w.$]+,\s*\w+),\s*\w+\s*$', line)
        if m:
            line = '\t' + m.group(1)
        # label, then mnemonic/directive, then operands
        m = re.match(r'^([A-Za-z_.][\w.$]*):(.*)$', line)
        if m:
            lab, line = m.group(1), m.group(2)
            out.append((lab if re.fullmatch(r'L\d+', lab) else '_' + lab) + ':')
            if not line.strip():
                continue
        m = re.match(r'^(\s*)([A-Za-z.][\w.]*)(\s.*)?$', line)
        if m and m.group(3):
            ops = re.sub(r'(?<![\w.$])[A-Za-z_][\w.$]*(?![\w.$])', _sym, m.group(3))
            line = m.group(1) + m.group(2) + ops
        out.append(line)
    open(path, 'w').write('\n'.join(out))


def shorten_symbols(path):
    """32V's as keeps 8 characters of a name, so long C99 identifiers collide; map each to a unique short
    one.  Pure renaming: the instructions and their order do not change."""
    t = open(path, errors='replace').read()
    names = {}

    def rep(m):
        n = m.group(0)
        if n not in names:
            names[n] = '_q%x' % len(names)
        return names[n]
    t = re.sub(r'(?<![\w.$])_[A-Za-z0-9_$.]{8,}(?![\w.$])', rep, t)
    open(path, 'w').write(t)


_FP = {}


def _hash_file(p):
    if p not in _FP:
        try:
            _FP[p] = hashlib.sha1(open(p, 'rb').read()).hexdigest()[:12]
        except OSError:
            _FP[p] = 'missing'
    return _FP[p]


def _version(cmd):
    k = ' '.join(cmd)
    if k not in _FP:
        r = run(cmd + ['--version'])
        _FP[k] = hashlib.sha1((r.stdout + r.stderr).encode()).hexdigest()[:12]
    return _FP[k]


def fingerprint(arch, key):
    """what the result for this configuration depends on: the compiler binary and its flags, the
    assembler, the peephole c2, the reference compiler's version.  A rerun compiles a configuration again
    only when this changed."""
    a = ARCH[arch]
    if key.startswith(('stock', 'ours')):
        kind = key.split('-')[0]
        fl = ccomflags(arch, kind == 'ours') if key.endswith('-O') else ''
        parts = [_hash_file(os.path.join(BUILDS, kind, arch, 'ccom')), fl]
        if a['asm'] is None:
            parts.append(_hash_file(TOOL + '/modern/usr/src/cmd/as/as' if arch == 'pdp11' else TOOL + '/modern/usr/src/cmd/as32v/as32v'))
            if a['c2']:
                parts.append(_hash_file(TOOL + '/modern/usr/src/cmd/c/c2' if arch == 'pdp11' else os.path.join(BUILDS, a['c2'])))
        else:
            parts.append(_version(a['asm'][:1]))
        return ':'.join(parts)
    if key.startswith('clang'):
        return _version(a['clang'][:1]) if a['clang'] else 'none'
    return _version(a['gcc'][:1]) if a['gcc'] and shutil.which(a['gcc'][0]) else 'none'


ALLKEYS = ('clang-O0', 'clang-Os', 'gcc-O0', 'gcc-Os', 'stock-O0', 'stock-O', 'ours-O0', 'ours-O')


def ccomflags(arch, ours):
    f = '-xtemps -xdeljumps -xinline -xdce'      # what -O passes to ccom in both
    if ARCH[arch]['ssa']:
        f += ' -xssa'
    return ('-xdmr ' + f) if ours else f


def one(job, prev=None):
    corp, arch, src = job
    a, c = ARCH[arch], CORPORA[corp]
    res = dict(corp=corp, arch=arch, src=src.replace(ROOT + '/', ''), size={}, fail={})
    # configurations whose compiler, flags and source are unchanged keep their earlier result
    srcfp = _hash_file(src)
    fps = {k: fingerprint(arch, k) + '|' + srcfp for k in ALLKEYS}
    res['fp'] = fps
    stale = set(ALLKEYS)
    if prev:
        # results from before fingerprints existed: stock, clang and gcc are what they were then (their
        # binaries have not changed since); the fork's compiler is not, so its configurations are redone
        adopt = not prev.get('fp')
        if 'all' in prev['fail'] and (adopt or prev['fp'].get('src') == srcfp):
            prev['fp'] = dict(fps, src=srcfp)
            return prev
        for k in ALLKEYS:
            same = (not k.startswith('ours')) if adopt else prev['fp'].get(k) == fps[k]
            if same:
                stale.discard(k)
                if k in prev['size']:
                    res['size'][k] = prev['size'][k]
                if k in prev['fail']:
                    res['fail'][k] = prev['fail'][k]
        if not stale:
            return prev
    res['fp']['src'] = srcfp
    with tempfile.TemporaryDirectory() as w:
        if src.endswith('.gz'):
            import gzip
            local = os.path.join(w, os.path.basename(src)[:-3])
            open(local, 'wb').write(gzip.open(src).read())
            incdir = os.path.dirname(src)
        else:
            local, incdir = src, os.path.dirname(src)
        incs = []
        for i in c['inc']:
            incs += ['-I', os.path.join(ROOT, i)]
        incs += ['-I', incdir]
        sysinc = c.get('sys') or os.path.join(ROOT, 'sysroot', a['musl'], 'include')
        std = [] if c.get('sys') else ['-std=gnu99', '-D__STDC_VERSION__=199901L', '-D__STDC_HOSTED__=1']
        cppcmd = ['gcc', '-E', '-P', '-undef', '-nostdinc'] + std + a['cpp'] + c['defs'] + incs + ['-isystem', sysinc, local]
        if arch in ('pdp11', 'vax') and not c.get('sys'):
            cppcmd += ['-D__ILP32__']
        r = run(cppcmd)
        if r.returncode:
            res['fail']['all'] = ('cpp', r.stderr.strip().split('\n')[0][:160])
            return res
        i = os.path.join(w, 'i.c')
        text = r.stdout
        if arch == 'pdp11' and not c.get('sys'):
            # musl's struct timespec pads with `int :8*(sizeof(time_t)-sizeof(long))`, 32 bits once long long
            # is 64 bits, which a 16-bit int cannot hold; the padding is layout for another machine
            text = re.sub(r'int :8\*\(sizeof\(time_t\)-sizeof\(long\)\)\*\(1234[!=]=4321\);', '', text)
        open(i, 'w').write(text)
        refs = []
        common = ['-std=gnu99', '-w', '-fno-asynchronous-unwind-tables', '-fno-unwind-tables']
        if a['clang']:
            refs = [('clang-O0', a['clang'] + common + ['-O0']), ('clang-Os', a['clang'] + common + ['-Os'])]
        if a['gcc'] and shutil.which(a['gcc'][0]):
            refs += [('gcc-O0', a['gcc'] + common + ['-O0']), ('gcc-Os', a['gcc'] + common + ['-Os'])]
        for key, cmd in refs:
            if key not in stale:
                continue
            o = os.path.join(w, key + '.o')
            if run(cmd + ['-c', '-x', 'c', '-o', o, i]).returncode == 0:
                res['size'][key] = size_of(o, a.get('gccsize', 'size') if key.startswith('gcc') else 'size')
        for key, kind, fl in (('stock-O0', 'stock', ''), ('stock-O', 'stock', ccomflags(arch, False)),
                              ('ours-O0', 'ours', ''), ('ours-O', 'ours', ccomflags(arch, True))):
            if key not in stale:
                continue
            ccom = os.path.join(BUILDS, kind, arch, 'ccom')
            if not os.path.exists(ccom):
                res['fail'][key] = ('build', 'no ccom for %s/%s' % (kind, arch))
                continue
            s = os.path.join(w, key + '.s')
            try:
                p = run([ccom] + fl.split() + [i, s], timeout=CCOM_TIMEOUT)
            except subprocess.TimeoutExpired:
                res['fail'][key] = ('ccom', 'timeout')
                continue
            if p.returncode or not os.path.exists(s):
                m = [x for x in (p.stderr or p.stdout).strip().split('\n') if x.strip()]
                # the first line that is not a warning is the reason; warnings come first when there are many
                e = [x for x in m if 'warning' not in x]
                msg = e[0] if e else (m[0] if m else 'exit %d' % p.returncode)
                msg = re.sub(r'^<stdin>, line \d+: ', '', msg)
                msg = re.sub(r'0x[0-9a-f]+', '0x', msg)
                res['fail'][key] = ('ccom', msg[:200] + (' (signal %d)' % -p.returncode if p.returncode < 0 else ''))
                continue
            o = os.path.join(w, key + '.o')
            if a['asm'] is None:
                if arch == 'vax':
                    gnu_to_v7(s)
                    shorten_symbols(s)
                if fl and a['c2']:
                    # optimized output goes through the target's peephole c2, the same program for both compilers
                    s2 = os.path.join(w, key + '.c2.s')
                    c2 = os.path.join(BUILDS, a['c2'])
                    if arch == 'pdp11':
                        c2 = TOOL + '/modern/usr/src/cmd/c/c2'
                    q = run([c2, s, s2], timeout=CCOM_TIMEOUT)
                    if q.returncode or not os.path.exists(s2):
                        res['fail'][key] = ('c2', (q.stderr or q.stdout).strip().split('\n')[0][:200] or 'c2 failed')
                        continue
                    s = s2
                sz, err = old_assemble(arch, s, o, w)
                if sz is None:
                    res['fail'][key] = ('asm', err or '')
                    continue
                res['size'][key] = sz
                continue
            q = run(a['asm'] + ['-o', o, s])
            if q.returncode:
                res['fail'][key] = ('asm', (q.stderr.strip().split('\n') or [''])[0][:200])
                continue
            res['size'][key] = size_of(o)
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--arch', default=','.join(ARCH))
    ap.add_argument('--only', default=','.join(CORPORA))
    ap.add_argument('-j', type=int, default=os.cpu_count() or 2)
    ap.add_argument('--out', default='/tmp/corpus-out')
    ap.add_argument('--full', action='store_true', help='no striding of the big test suites')
    ap.add_argument('--limit', type=int, default=0, help='at most N files per corpus (for trials)')
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    jobs = []
    for corp in a.only.split(','):
        files = CORPORA[corp]['files']()
        if CORPORA[corp].get('stride') and not a.full:
            files = files[::CORPORA[corp]['stride']]     # thousands of tiny tests: every Nth is enough
        if a.limit:
            files = files[:a.limit]
        for arch in a.arch.split(','):
            if 'arches' in CORPORA[corp] and arch not in CORPORA[corp]['arches']:
                continue
            for f in files:
                jobs.append((corp, arch, f))
    # results are appended to results.jsonl as they finish; a rerun with the same --out skips what is there
    jl = os.path.join(a.out, 'results.jsonl')
    done = {}
    if os.path.exists(jl):
        for l in open(jl):
            try:
                r = json.loads(l)
                done[(r['corp'], r['arch'], r['src'])] = r
            except Exception:
                pass
    key = lambda j: (j[0], j[1], j[2].replace(ROOT + '/', ''))
    sys.stderr.write('%d jobs, %d with an earlier result\n' % (len(jobs), sum(1 for j in jobs if key(j) in done)))
    changed = 0
    with cf.ThreadPoolExecutor(max_workers=a.j) as ex, open(jl, 'a') as out:
        def safe(j):
            try:
                return one(j, done.get(key(j)))
            except Exception as e:      # one bad job must not lose the others
                sys.stderr.write('job %s failed: %r\n' % (key(j), e))
                return done.get(key(j)) or dict(corp=j[0], arch=j[1], src=key(j)[2], size={}, fail={'all': ('harness', repr(e)[:150])})
        futs = [ex.submit(safe, j) for j in jobs]
        for n, f in enumerate(cf.as_completed(futs)):
            r = f.result()
            k = (r['corp'], r['arch'], r['src'])
            if done.get(k) is not r:
                out.write(json.dumps(r) + '\n')
                out.flush()
                changed += 1
            done[k] = r
            if n % 500 == 0:
                sys.stderr.write('%d / %d (%d recompiled)\n' % (n, len(jobs), changed))
    sys.stderr.write('%d results changed\n' % changed)
    results = [done[key(j)] for j in jobs]
    with open(jl + '.tmp', 'w') as out:      # compact: one line per job
        for r in results:
            out.write(json.dumps(r) + '\n')
    os.replace(jl + '.tmp', jl)
    json.dump(results, open(os.path.join(a.out, 'results.json'), 'w'), indent=1)
    report(results, a.out)


GROUPS = [('Unoptimized: pcc with no flags, clang -O0, gcc -O0', ['stock-O0', 'ours-O0', 'clang-O0', 'gcc-O0']),
          ('Optimized: stock `-O`, ours `-O -xdmr`, clang -Os, gcc -Os', ['stock-O', 'ours-O', 'clang-Os', 'gcc-Os'])]


def report(results, out):
    lines = ['Bytes of text+data (`size`).  stock = current PortableCC master; ours = this branch; clang/gcc built',
             'without unwind tables and without PIE.  Each row sums only the files that stock, ours and clang all',
             'compiled (gcc, x86 only, joins when it did too); "ok" is how many of the files each compiler managed.', '']
    by = collections.defaultdict(list)
    for r in results:
        by[(r['corp'], r['arch'])].append(r)
    pct = lambda x, y: '%+.0f%%' % (100.0 * (x - y) / y) if y else ''
    order = {a: n for n, a in enumerate(ARCH)}

    def row(name, arch, rs, keys):
        st, ou, cl, gc = keys
        if ARCH[arch]['clang'] is None:          # the old targets: gcc is the only outside reference
            cl, gc = gc, None
        need = (st, ou, cl) if any(cl in r['size'] for r in rs) else (st, ou)
        ok = [r for r in rs if all(k in r['size'] for k in need)]
        sm = collections.Counter()
        for r in ok:
            for k in keys:
                if k in r['size']:
                    sm[k] += r['size'][k]
        okg = [r for r in ok if gc and gc in r['size']]
        gs = sum(r['size'][gc] for r in okg)
        gp = sum(r['size'][ou] for r in okg)
        n = lambda k: sum(1 for r in rs if k in r['size'])
        return '| %s | %s | %d | %d / %d / %d | %d | %s | %s | %s | %s | %s | %s |' % (
            name, arch, len(rs), n(st), n(ou), n(cl), len(ok), sm[st] or '', sm[ou] or '', sm[cl] or '',
            ('%d (ours %d)' % (gs, gp)) if okg else '', pct(sm[ou], sm[st]), pct(sm[ou], sm[cl])), sm

    for title, keys in GROUPS:
        lines += ['## ' + title, '',
                  '| corpus | arch | files | ok stock/ours/clang | common | stock | ours | clang | gcc | ours vs stock | ours vs clang |',
                  '|---|---|---|---|---|---|---|---|---|---|---|']
        for (corp, arch), rs in sorted(by.items(), key=lambda kv: (order[kv[0][1]], kv[0][0])):
            lines.append(row(corp, arch, rs, keys)[0])
        for arch in ARCH:
            rs = [r for r in results if r['arch'] == arch]
            if rs:
                lines.append(row('**all**', arch, rs, keys)[0])
        lines.append('')
    for (corp, arch), rs in sorted(by.items(), key=lambda kv: (order[kv[0][1]], kv[0][0])):
        for key in ('stock-O', 'ours-O'):
            bad = [r for r in rs if key in r['fail'] or 'all' in r['fail']]
            if not bad:
                continue
            lines.append('### %s / %s / %s: %d files failed' % (corp, arch, key, len(bad)))
            g = collections.Counter(tuple(r['fail'].get(key) or r['fail']['all']) for r in bad)
            for (st, msg), k in g.most_common(8):
                lines.append('* %d x %s: %s' % (k, st, msg))
            lines.append('')
    open(os.path.join(out, 'report.md'), 'w').write('\n'.join(lines) + '\n')
    print('\n'.join(lines))


if __name__ == '__main__':
    main()
