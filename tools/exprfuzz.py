#!/usr/bin/env python3
"""
exprfuzz -- differential test of a pcc target against the host compiler.

    exprfuzz.py SEED COUNT [--flags "ccom flags"] [--target aarch64|amd64] [--keep DIR] [--funcs N]

Generates COUNT random programs of well-defined C (integers of every width and
sign, float and double, arrays, structs, pointers, loops, conditionals, calls),
runs each with the host gcc and with the target's ccom, and reports the programs
whose results differ.  The result is a checksum of every variable and call, so
any wrong instruction shows.

Undefined behaviour is avoided by construction: signed arithmetic goes through
unsigned types, shift counts are masked, divisors are 1..8, floating point
values stay integers below 2**40 and are converted through long.

aarch64 needs ARM64CCOM (the ccom) and ARM64PY (a python with unicorn and
keystone); amd64 needs A64CCOM.  Failing programs are kept in --keep (default
$TMPDIR/exprfuzz).
"""
import os, random, subprocess, sys, tempfile, argparse, concurrent.futures as cf

here = os.path.dirname(os.path.abspath(__file__))

INTS = [('char', 8, True), ('unsigned char', 8, False), ('short', 16, True),
        ('unsigned short', 16, False), ('int', 32, True), ('unsigned', 32, False),
        ('long', 64, True), ('unsigned long', 64, False)]


def utype(t):
    name, bits, signed = t
    return {'char': 'unsigned char', 'short': 'unsigned short', 'int': 'unsigned',
            'long': 'unsigned long'}.get(name, name)


def wide(t):
    return 'unsigned long' if t[1] == 64 else 'unsigned'


class Gen:
    def __init__(self, rnd, nglob=6, small=False):
        self.r = rnd
        self.small = small
        self.glob = []          # (name, type) scalars
        self.arrs = []          # (name, type, len)
        self.funcs = []         # (name, [param types], ret type)
        self.consts = [0, 1, 2, 3, 7, 8, 15, 16, 31, 63, 100, 127, 128, 255, 256, 4095, 4096,
                       32767, 32768, 65535, 65536, 0x7fffffff, 0x80000000, 0xffffffff,
                       0x123456789abcdef, 1 << 40, -1, -2, -128, -4096, -32768, 0x55555555, 0xff00ff00]

    def ity(self):
        return self.r.choice(INTS)

    def const(self, t):
        c = self.r.choice(self.consts)
        if self.r.random() < 0.3:
            c = self.r.randrange(-1000, 1000)
        return '((%s)%dL)' % (t[0], c) if c < (1 << 62) else '((%s)%dUL)' % (t[0], c)

    # an integer expression of type t
    def iexpr(self, t, d, env):
        r = self.r
        if d <= 0 or r.random() < 0.15:
            k = r.random()
            if k < 0.45 and env:
                n, vt = r.choice(env)
                if vt in INTS:
                    return '(%s)%s' % (t[0], n) if vt != t else n
            if k < 0.6 and self.arrs:
                a = r.choice(self.arrs)
                if a[1] in INTS:
                    return '(%s)%s[(%s) & %d]' % (t[0], a[0], self.iexpr(r.choice(INTS), 0, env), a[2] - 1)
            if k < 0.7 and self.glob:
                n, vt = r.choice(self.glob)
                if vt in INTS:
                    return '(%s)%s' % (t[0], n)
            return self.const(t)
        k = r.random()
        T, UT, W = t[0], utype(t), wide(t)
        sub = lambda ty=t, dd=d - 1: self.iexpr(ty, dd, env)
        if k < 0.35:
            op = r.choice(['+', '-', '*', '&', '|', '^'])
            return '(%s)((%s)%s %s (%s)%s)' % (T, W, sub(), op, W, sub())
        if k < 0.45:
            s = r.choice(['<<', '>>'])
            n = '(%s & %d)' % (self.iexpr(self.ity(), d - 1, env), t[1] - 1)
            if s == '<<':
                return '(%s)((%s)%s << %s)' % (T, W, sub(), n)
            return '(%s)(%s >> %s)' % (T, sub(), n)
        if k < 0.52:
            op = r.choice(['/', '%'])
            dv = '((%s & 7) + 1)' % self.iexpr(self.ity(), d - 1, env)
            return '(%s)(%s %s %s)' % (T, sub(), op, dv)
        if k < 0.58:
            op = r.choice(['-', '~', '!'])
            if op == '-':
                return '(%s)(0 - (%s)%s)' % (T, W, sub())
            return '(%s)(%s%s)' % (T, op, sub())
        if k < 0.70:
            ty = self.ity()
            op = r.choice(['<', '<=', '>', '>=', '==', '!='])
            return '(%s)(%s %s %s)' % (T, self.iexpr(ty, d - 1, env), op, self.iexpr(ty, d - 1, env))
        if k < 0.77:
            return '(%s ? %s : %s)' % (self.iexpr(self.ity(), d - 1, env), sub(), sub())
        if k < 0.82:
            return '(%s && %s || %s)' % (self.iexpr(self.ity(), d - 1, env), self.iexpr(self.ity(), d - 1, env), self.iexpr(self.ity(), d - 1, env))
        if k < 0.90:
            return '(%s)%s' % (T, self.iexpr(self.ity(), d - 1, env))
        if k < 0.95:
            return '(%s)(long)(%s)' % (T, self.fexpr(r.choice(['double', 'float']), d - 1, env)[0])
        if self.funcs:
            f = r.choice(self.funcs)
            if f[2] in INTS:
                return '(%s)%s(%s)' % (T, f[0], ', '.join(self.arg(p, d - 1, env) for p in f[1]))
        return sub()

    def arg(self, p, d, env):
        if p in INTS:
            return self.iexpr(p, d, env)
        return self.fexpr(p, d, env)[0]

    # a floating point expression, with a bound on its magnitude
    def fexpr(self, ft, d, env):
        r = self.r
        if d <= 0 or r.random() < 0.2:
            k = r.random()
            if k < 0.5:
                v = r.randrange(-1000, 1000)
                return '((%s)%d)' % (ft, v), 1000
            if k < 0.8:
                return '((%s)(long)(%s))' % (ft, '(%s) & 0xffff' % self.iexpr(r.choice(INTS), 0, env)), 65535
            fenv = [(n, t) for n, t in env if t in ('float', 'double')]
            if fenv:
                n, _ = r.choice(fenv)
                return '((%s)%s)' % (ft, n), 1 << 24
            return '((%s)%d)' % (ft, r.randrange(1, 100)), 100
        k = r.random()
        a, ba = self.fexpr(ft, d - 1, env)
        b, bb = self.fexpr(ft, d - 1, env)
        if k < 0.4 and ba + bb < (1 << 24):
            return '(%s %s %s)' % (a, r.choice(['+', '-']), b), ba + bb
        if k < 0.6 and ba * bb < (1 << 24):
            return '(%s * %s)' % (a, b), ba * bb
        if k < 0.7:
            return '(%s / %s)' % (a, r.choice(['2', '4', '8', '0.5'])), ba * 2
        if k < 0.8:
            return '(-%s)' % a, ba
        if k < 0.9:
            c = '%s %s %s' % (a, r.choice(['<', '>', '<=', '>=', '==', '!=']), b)
            return '(%s ? %s : %s)' % (c, a, b), max(ba, bb)
        return '((%s)(%s)(%s))' % (ft, r.choice(['float', 'double']), a), ba

    def stmts(self, env, d, loops=2):
        r = self.r
        out = []
        wr = [e for e in env if not e[0].startswith('i')]    # loop counters are read only
        for _ in range(r.randrange(1, 4) if self.small else r.randrange(3, 9)):
            k = r.random()
            if k < 0.5:
                n, t = r.choice(wr)
                if t in INTS:
                    out.append('%s = %s;' % (n, self.iexpr(t, 3, env)))
                else:
                    out.append('%s = (%s)(long)(%s);' % (n, t, self.fexpr(t, 2, env)[0]) if r.random() < 0.3 else '%s = %s;' % (n, self.fexpr(t, 2, env)[0]))
            elif k < 0.6 and [a for a in self.arrs if a[0].startswith('la')]:
                a = r.choice([a for a in self.arrs if a[0].startswith('la')])
                if a[1] in INTS:
                    out.append('%s[(%s) & %d] = %s;' % (a[0], self.iexpr(r.choice(INTS), 1, env), a[2] - 1, self.iexpr(a[1], 3, env)))
            elif k < 0.72 and self.arrs and d > 0:
                # pointer walks, post-increment, indexed reads and writes
                a = r.choice(self.arrs)
                if a[1] in INTS and wr:
                    cands = [e for e in wr if e[1] in INTS]
                    if cands:
                        n, t = r.choice(cands)
                        ln = a[2]
                        kind = r.randrange(5)
                        v = 'q%d' % r.randrange(1000)
                        if kind == 0:
                            out.append('{ %s *%s = %s; int j%s; for (j%s = 0; j%s < %d; j%s++) %s += (%s)*%s++; }' % (
                                a[1][0], v, a[0], v, v, v, ln, v, n, t[0], v))
                        elif kind == 1 and a[0].startswith('la'):
                            out.append('{ %s *%s = %s; int j%s; for (j%s = 0; j%s < %d; j%s++) *%s++ = (%s)(%s + j%s); }' % (
                                a[1][0], v, a[0], v, v, v, ln, v, v, a[1][0], n, v))
                        elif kind == 2:
                            out.append('{ int j%s; for (j%s = 0; j%s < %d; j%s++) %s ^= (%s)%s[j%s]; }' % (
                                v, v, v, ln, v, n, t[0], a[0], v))
                        elif kind == 3:
                            out.append('{ int j%s = %d; while (j%s--) %s += (%s)%s[j%s & %d]; }' % (
                                v, ln, v, n, t[0], a[0], v, ln - 1))
                        else:
                            out.append('if ((unsigned long)(%s) & ((unsigned long)1 << ((%s) & %d))) %s = (%s)((unsigned long)%s + 1); else %s = (%s)((unsigned long)%s - 1);' % (
                                n, self.iexpr(self.ity(), 1, env), t[1] - 1, n, t[0], n, n, t[0], n))
            elif k < 0.8 and d > 0:
                out.append('if (%s) {' % self.iexpr(self.ity(), 2, env))
                out += ['  ' + s for s in self.stmts(env, d - 1, loops)]
                out.append('} else {')
                out += ['  ' + s for s in self.stmts(env, d - 1, loops)]
                out.append('}')
            elif k < 0.9 and d > 0 and loops > 0:
                v = 'i%d' % r.randrange(1000)
                out.append('{ int %s; for (%s = 0; %s < %d; %s++) {' % (v, v, v, r.randrange(1, 6), v))
                env2 = env + [(v, ('int', 32, True))]
                out += ['  ' + s for s in self.stmts(env2, d - 1, loops - 1)]
                out.append('} }')
            else:
                n, t = r.choice(wr)
                if t in INTS:
                    out.append('%s %s= %s;' % (n, r.choice(['+', '-', '^', '|']), self.iexpr(t, 2, env)))
        return out

    def function(self, idx):
        r = self.r
        ret = r.choice(INTS + ['double', 'float'] if False else INTS)
        params = [r.choice(INTS + ['double', 'float']) for _ in range(r.randrange(0, 4) if self.small else r.randrange(0, 11))]
        nm = 'fn%d' % idx
        lines = []
        env = []
        decl = []
        for i, p in enumerate(params):
            pn = 'p%d' % i
            env.append((pn, p))
            decl.append('%s %s' % (p[0] if p in INTS else p, pn))
        lines.append('%s %s(%s)' % (ret[0], nm, ', '.join(decl) or 'void'))
        lines.append('{')
        nloc = len(self.arrs)
        for i in range(r.randrange(0, 2)):
            lt = r.choice(INTS)
            lines.append('  %s la%d[8] = { %s };' % (lt[0], i, ', '.join(self.const(lt) for _ in range(r.randrange(1, 8)))))
            self.arrs.append(('la%d' % i, lt, 8))
        for i in range(r.randrange(1, 3) if self.small else r.randrange(2, 7)):
            t = r.choice(INTS + ['double', 'float'])
            ln = 'v%d' % i
            if t in INTS:
                lines.append('  %s %s = %s;' % (t[0], ln, self.iexpr(t, 2, env)))
            else:
                lines.append('  %s %s = %s;' % (t, ln, self.fexpr(t, 1, env)[0]))
            env.append((ln, t))
        lines += ['  ' + s for s in self.stmts(env, 2)]
        acc = '0UL'
        for n, t in env:
            if t in INTS:
                acc = '(%s * 31 + (unsigned long)(long)%s)' % (acc, n)
            else:
                acc = '(%s * 31 + (unsigned long)(long)(%s))' % (acc, n)
        for a in self.arrs[nloc:]:
            for j in range(8):
                acc = '(%s * 31 + (unsigned long)(long)%s[%d])' % (acc, a[0], j)
        del self.arrs[nloc:]
        lines.append('  return (%s)%s;' % (ret[0], acc))
        lines.append('}')
        self.funcs.append((nm, params, ret))
        return lines, params

    def program(self, nfuncs, only=None):
        r = self.r
        out = []
        for i in range(r.randrange(2, 6)):
            t = self.ity()
            n = 'g%d' % i
            self.glob.append((n, t))
            out.append('%s %s = %s;' % (t[0], n, self.const(t)))
        for i in range(r.randrange(1, 4)):
            t = self.ity()
            n = 'a%d' % i
            ln = r.choice([4, 8, 16])
            self.arrs.append((n, t, ln))
            out.append('%s %s[%d] = { %s };' % (t[0], n, ln, ', '.join(self.const(t) for _ in range(r.randrange(1, ln)))))
        body = []
        calls = []
        for i in range(nfuncs):
            lines, params = self.function(i)
            body += lines
            args = ', '.join(self.arg(p, 1, [(g[0], g[1]) for g in self.glob]) for p in params)
            st = ''
            if self.glob and r.random() < 0.7:
                gn, gt = r.choice(self.glob)
                st += '  %s = %s;\n' % (gn, self.iexpr(gt, 2, [(g[0], g[1]) for g in self.glob]))
            if self.arrs and r.random() < 0.7:
                an, at, al = r.choice(self.arrs)
                st += '  %s[%d] = %s;\n' % (an, r.randrange(al), self.iexpr(at, 2, [(g[0], g[1]) for g in self.glob]))
            calls.append(st + '  sum = sum * 1000003 + (unsigned long)(long)fn%d(%s);' % (i, args))
        out += body
        if only is not None:
            calls = calls[:only + 1]
            if only < 0:
                calls = []
        out.append('int test(void)')
        out.append('{')
        out.append('  unsigned long sum = 17; int k;')
        out += calls
        out.append('  for (k = 0; k < 3; k++) {')
        out += ['  ' + c for c in calls]
        out.append('  }')
        for n, t in self.glob:
            out.append('  sum = sum * 31 + (unsigned long)(long)%s;' % n)
        for n, t, ln in self.arrs:
            out.append('  for (k = 0; k < %d; k++) sum = sum * 31 + (unsigned long)(long)%s[k];' % (ln, n))
        out.append('  return (int)((sum ^ (sum >> 32)) & 0x7fffffff);')
        out.append('}')
        return '\n'.join(out) + '\n'


def host(src, work):
    c = os.path.join(work, 'h.c')
    open(c, 'w').write(src + '#include <stdio.h>\nint main(void){ printf("%d\\n", test()); return 0; }\n')
    x = os.path.join(work, 'h.x')
    p = subprocess.run(['gcc', '-w', '-std=gnu99', '-O0', '-fsanitize=undefined,float-cast-overflow',
                        '-fno-sanitize-recover=all', '-o', x, c], capture_output=True, text=True)
    if p.returncode:
        return None
    try:
        q = subprocess.run([x], capture_output=True, text=True, timeout=10)
    except subprocess.TimeoutExpired:
        return None
    if q.returncode or q.stderr:
        return None             # the program has undefined behaviour: not a test
    return q.stdout.strip()


def target(src, work, flags, tgt):
    f = os.path.join(work, 't.c')
    open(f, 'w').write(src)
    if tgt == 'aarch64':
        r = subprocess.run([os.path.join(here, 'arm64test.sh')] + flags.split() + [f], capture_output=True, text=True, timeout=300)
    else:
        r = subprocess.run([os.path.join(here, 'a64test.sh')] + flags.split() + [f], capture_output=True, text=True, timeout=300)
    out = r.stdout.strip() or r.stderr.strip()
    return out.split('\n')[-1][:200]


def one(args, only=None):
    seed, flags, tgt, nfuncs, keep = args[:5]
    rnd = random.Random(seed)
    g = Gen(rnd, small=len(args) > 5 and args[5])
    src = g.program(nfuncs, only)
    with tempfile.TemporaryDirectory() as w:
        h = host(src, w)
        if h is None:
            return seed, 'skip', ''
        t = target(src, w, flags, tgt)
        if h != t:
            os.makedirs(keep, exist_ok=True)
            open(os.path.join(keep, 'fuzz%d.c' % seed), 'w').write('/* host %s target %s [%s] */\n' % (h, t, flags) + src)
            return seed, 'DIFF', 'host %s target %s' % (h, t)
    return seed, 'ok', ''


def wellformed(src, w):
    """No missing returns, no uninitialised reads (a reduction can drop either)."""
    c = os.path.join(w, 'w.c')
    open(c, 'w').write(src)
    p = subprocess.run(['gcc', '-std=gnu99', '-O1', '-Wall', '-Wno-unused', '-Wno-parentheses', '-Werror=return-type',
                        '-Werror=uninitialized', '-Werror=maybe-uninitialized', '-c', '-o', '/dev/null', c],
                       capture_output=True, text=True)
    return p.returncode == 0


def differs(src, flags, tgt):
    with tempfile.TemporaryDirectory() as w:
        if not wellformed(src, w):
            return False
        h = host(src, w)
        if h is None:
            return False
        return target(src, w, flags, tgt) != h


def reduce_file(path, flags, tgt):
    """Delta-debug by lines: drop every chunk of lines that keeps the programs differing."""
    lines = [l for l in open(path).read().split('\n')]
    if lines and lines[0].startswith('/* host'):
        lines = lines[1:]
    n = max(len(lines) // 2, 1)
    while n >= 1:
        i = 0
        changed = False
        while i < len(lines):
            cand = lines[:i] + lines[i + n:]
            if differs('\n'.join(cand) + '\n', flags, tgt):
                lines = cand
                changed = True
            else:
                i += n
        if not changed:
            n //= 2
    return '\n'.join(lines) + '\n'


def main():
    if len(sys.argv) > 2 and sys.argv[1] == '--reduce':
        flags = sys.argv[3] if len(sys.argv) > 3 else '-xtemps -xdeljumps -xinline -xdce'
        tgt = sys.argv[4] if len(sys.argv) > 4 else 'aarch64'
        print(reduce_file(sys.argv[2], flags, tgt))
        return
    ap = argparse.ArgumentParser()
    ap.add_argument('seed', type=int)
    ap.add_argument('count', type=int)
    ap.add_argument('--flags', default='-xtemps -xdeljumps -xinline -xdce')
    ap.add_argument('--target', default='aarch64')
    ap.add_argument('--keep', default=os.path.join(os.environ.get('TMPDIR', '/tmp'), 'exprfuzz'))
    ap.add_argument('--funcs', type=int, default=4)
    ap.add_argument('-j', type=int, default=os.cpu_count() or 2)
    ap.add_argument('--small', action='store_true', help='small functions: easier to read when they differ')
    ap.add_argument('--narrow', action='store_true', help='for each differing seed, find the first function that differs')
    a = ap.parse_args()
    jobs = [(a.seed + i, a.flags, a.target, a.funcs, a.keep, a.small) for i in range(a.count)]
    bad = skip = 0
    with cf.ThreadPoolExecutor(max_workers=a.j) as ex:
        for seed, st, msg in ex.map(one, jobs):
            if st == 'DIFF':
                bad += 1
                print('seed %d: %s' % (seed, msg), flush=True)
                if a.narrow:
                    for fi in range(a.funcs):
                        sd, st2, m2 = one((seed, a.flags, a.target, a.funcs, a.keep + '/f', a.small), fi)
                        if st2 == 'DIFF':
                            print('   the first %d call(s) already differ: %s' % (fi + 1, m2), flush=True)
                            break
            elif st == 'skip':
                skip += 1
    print('exprfuzz: %d programs, %d differ, %d skipped (host did not compile)' % (a.count, bad, skip))
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
