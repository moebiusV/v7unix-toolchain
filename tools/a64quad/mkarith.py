#!/usr/bin/env python3
"""
mkarith.py -- write the long double arithmetic tests (q4.c .. q7.c): every expected result comes from the
host's __float128 (gcc + libquadmath), so the tests check pcc's calls into the library and the library
(tf.c) at once.  Run from tools/a64quad; needs gcc with libquadmath.

  q4: + - * / and unary minus        q5: comparisons and conditions
  q6: conversions to and from every type        q7: += -= *= /=, ++ --, ?:, arguments in expressions
"""
import random, subprocess, tempfile, os, struct

random.seed(2026)
ONE = 1 << 112
def q(sign, exp, frac): return (sign << 127) | (exp << 112) | frac
def lit(hi_lo): return ((hi_lo >> 64) & (2**64 - 1), hi_lo & (2**64 - 1))
SPECIAL = [q(0, 16383, 0), q(1, 16383, 0), q(0, 16383, ONE >> 1), q(0, 16384, ONE >> 2), q(0, 16383 - 1, 0), q(0, 0, 0), q(1, 0, 0),
           q(0, 0x7fff, 0), q(1, 0x7fff, 0), q(0, 0x7fff, ONE >> 1), q(0, 1, 0), q(0, 0, 1), q(0, 0, ONE - 1), q(0, 0x7ffe, ONE - 1),
           q(0, 16383 + 100, 12345), q(1, 16383 - 100, 99999999999), q(0, 16383 + 112, 0), q(0, 16383 + 113, 0), q(0, 16383, 1)]
def rnd():
    k = random.random()
    if k < 0.15: return random.choice(SPECIAL)
    if k < 0.55: return q(random.getrandbits(1), 16383 + random.randint(-30, 30), random.getrandbits(112))
    if k < 0.75: return q(random.getrandbits(1), 16383 + random.randint(-3, 3), random.getrandbits(112) & ~((1 << random.randint(0, 100)) - 1))
    return q(random.getrandbits(1), random.randint(0, 0x7ffe), random.getrandbits(112))

HOST = r'''
#include <stdio.h>
#include <string.h>
#include <stdint.h>
typedef unsigned __int128 u128;
typedef __float128 qd;
static qd B(u128 v) { qd x; memcpy(&x, &v, 16); return x; }
static void P(qd x) { u128 v; memcpy(&v, &x, 16); printf("%016llx %016llx\n", (unsigned long long)(v >> 64), (unsigned long long)v); }
int main(void)
{
	char op[16]; unsigned long long ah, al, bh, bl;
	while (scanf("%15s %llx %llx %llx %llx", op, &ah, &al, &bh, &bl) == 5) {
		u128 a = ((u128)ah << 64) | al, b = ((u128)bh << 64) | bl;
		qd x = B(a), y = B(b);
		if (!strcmp(op, "add")) P(x + y);
		else if (!strcmp(op, "sub")) P(x - y);
		else if (!strcmp(op, "mul")) P(x * y);
		else if (!strcmp(op, "div")) P(x / y);
		else if (!strcmp(op, "neg")) P(-x);
		else if (!strcmp(op, "tod")) { double d = (double)x; uint64_t v; memcpy(&v, &d, 8); printf("%016llx %016llx\n", 0ULL, (unsigned long long)v); }
		else if (!strcmp(op, "tof")) { float f = (float)x; uint32_t v; memcpy(&v, &f, 4); printf("%016llx %016llx\n", 0ULL, (unsigned long long)v); }
		else if (!strcmp(op, "toi")) printf("%016llx %016llx\n", 0ULL, (unsigned long long)(unsigned)(int)x);
		else if (!strcmp(op, "tol")) printf("%016llx %016llx\n", 0ULL, (unsigned long long)(long long)x);
		else if (!strcmp(op, "tou")) printf("%016llx %016llx\n", 0ULL, (unsigned long long)(unsigned)x);
		else if (!strcmp(op, "toul")) printf("%016llx %016llx\n", 0ULL, (unsigned long long)x);
		else if (!strcmp(op, "fromi")) P((qd)(int)al);
		else if (!strcmp(op, "fromu")) P((qd)(unsigned)al);
		else if (!strcmp(op, "froml")) P((qd)(long long)al);
		else if (!strcmp(op, "fromul")) P((qd)(unsigned long long)al);
		else if (!strcmp(op, "fromd")) { double d; unsigned long long v = al; memcpy(&d, &v, 8); P((qd)d); }
		else if (!strcmp(op, "fromf")) { float f; unsigned v = (unsigned)al; memcpy(&f, &v, 4); P((qd)f); }
		else printf("?\n");
	}
	return 0;
}
'''
def host(jobs):
    d = tempfile.mkdtemp()
    open(d + '/h.c', 'w').write(HOST)
    subprocess.check_call(['gcc', '-w', '-o', d + '/h', d + '/h.c', '-lquadmath'])
    inp = ''.join('%s %x %x %x %x\n' % (op, a >> 64, a & (2**64 - 1), b >> 64, b & (2**64 - 1)) for op, a, b in jobs)
    out = subprocess.run([d + '/h'], input=inp, capture_output=True, text=True).stdout.split('\n')
    return [tuple(int(x, 16) for x in l.split()) for l in out[:len(jobs)]]

PRE = '''typedef unsigned long u64;
typedef unsigned int u32;
static long double mk(u64 hi, u64 lo) { u64 w[2]; w[0] = lo; w[1] = hi; return *(long double *)w; }
/* x has these bits (any NaN for an expected NaN) */
static int is(long double x, u64 hi, u64 lo)
{
	u64 *w = (u64 *)&x;
	u64 h = w[1] & 0x7fffffffffffffffUL;

	if (h > 0x7fff000000000000UL || (h == 0x7fff000000000000UL && w[0] != 0))
		return (hi & 0x7fffffffffffffffUL) > 0x7fff000000000000UL || ((hi & 0x7fffffffffffffffUL) == 0x7fff000000000000UL && lo != 0);
	return w[1] == hi && w[0] == lo;
}
'''
def header(desc): return '/* expect: 1 -- %s; expected values from the host\'s __float128 (mkarith.py) */\n' % desc + PRE

def arith():
    jobs = []
    for op in ('add', 'sub', 'mul', 'div'):
        for _ in range(26):
            jobs.append((op, rnd(), rnd()))
    for _ in range(10): jobs.append(('neg', rnd(), 0))
    res = host(jobs)
    n = 1
    s = header('+ - * / and unary minus on long double (library calls), against the host') + 'static long double A[%d], B[%d];\n\n' % (len(jobs), len(jobs))
    body = ''
    for i, ((op, a, b), (rh, rl)) in enumerate(zip(jobs, res)):
        ah, al = lit(a); bh, bl = lit(b)
        sym = {'add': '+', 'sub': '-', 'mul': '*', 'div': '/'}.get(op)
        x = 'mk(0x%016xUL, 0x%016xUL)' % (ah, al); y = 'mk(0x%016xUL, 0x%016xUL)' % (bh, bl)
        if op == 'neg': body += '\tif (!is(-%s, 0x%016xUL, 0x%016xUL)) return %d;\n' % (x, rh, rl, i + 2)
        else: body += '\tif (!is(%s %s %s, 0x%016xUL, 0x%016xUL)) return %d;\n' % (x, sym, y, rh, rl, i + 2)
    s = s.split('static long double A')[0] + 'int test(void)\n{\n' + body + '\treturn 1;\n}\n'
    return s

def compare():
    vals = [rnd() for _ in range(9)] + [q(0, 0x7fff, ONE >> 1), q(0, 16383, 0), q(1, 16383, 0), q(0, 0, 0), q(1, 0, 0)]
    s = header('long double comparisons and conditions, NaNs included') + '\n'
    exp = {}
    def val(v):
        # classes: ordering via python on the real values
        sign = v >> 127; e = (v >> 112) & 0x7fff; f = v & (ONE - 1)
        if e == 0x7fff and f: return None
        if e == 0x7fff: return float('-inf') if sign else float('inf')
        return v
    from fractions import Fraction
    def num(v):
        sign = v >> 127; e = (v >> 112) & 0x7fff; f = v & (ONE - 1)
        if e == 0x7fff: return None if f else (float('-inf') if sign else float('inf'))
        m = (f | ONE) if e else f
        x = Fraction(m) * (Fraction(2) ** ((e if e else 1) - 16383 - 112))
        return -x if sign else x
    body = ''
    k = 2
    pairs = [(a, b) for a in vals[:7] for b in vals[:7]] + [(a, a) for a in vals]
    random.shuffle(pairs)
    for a, b in pairs[:40]:
        na, nb = num(a), num(b)
        ah, al = lit(a); bh, bl = lit(b)
        x = 'mk(0x%016xUL, 0x%016xUL)' % (ah, al); y = 'mk(0x%016xUL, 0x%016xUL)' % (bh, bl)
        r = {}
        if na is None or nb is None:
            r = dict(lt=0, le=0, gt=0, ge=0, eq=0, ne=1)
        else:
            r = dict(lt=na < nb, le=na <= nb, gt=na > nb, ge=na >= nb, eq=na == nb, ne=na != nb)
        # as values, and as if/else on the same condition both ways round
        for nm, sym in (('lt', '<'), ('le', '<='), ('gt', '>'), ('ge', '>='), ('eq', '=='), ('ne', '!=')):
            want = int(bool(r[nm]))
            body += '\tx = %s; y = %s;\n' % (x, y)
            body += '\tif ((x %s y) != %d) return %d;\n' % (sym, want, k)
            body += '\tif (x %s y) { if (!%d) return %d; } else { if (%d) return %d; }\n' % (sym, want, k + 1, want, k + 2)
            body += '\tif (!(x %s y)) { if (%d) return %d; } else { if (!%d) return %d; }\n' % (sym, want, k + 3, want, k + 4)
            k += 5
    body += '''	/* truth values */
	x = mk(0x3fff000000000000UL, 0); y = mk(0, 0); z = mk(0x8000000000000000UL, 0); n = mk(0x7fff800000000000UL, 0);
	if (!x) return %d; if (y) return %d; if (z) return %d; if (!n) return %d;
	if (!!y) return %d; if (!!x == 0) return %d;
	if (!(x && n)) return %d; if (x && y) return %d; if (y || z) return %d; if (!(y || x)) return %d;
	if ((x ? 5 : 6) != 5 || (y ? 5 : 6) != 6) return %d;
	i = 0; w = mk(0x4001400000000000UL, 0);	/* 5.0 */
	one = mk(0x3fff000000000000UL, 0);
	while (w) { w -= one; i++; if (i > 10) return %d; }
	if (i != 5) return %d;
	for (w = mk(0x4000000000000000UL, 0); w; w -= one) i++;
	if (i != 7) return %d;
''' % tuple(range(k, k + 14))
    k += 14
    s += 'int test(void)\n{\n\tlong double x, y, z, n, w, one;\n\tint i;\n\n' + body + '\treturn 1;\n}\n'
    return s

def conv():
    jobs = []
    ints = [0, 1, 2, 7, 255, 65535, 2**31 - 1, 2**31, 2**32 - 1, 2**32, 2**53 + 1, 2**63 - 1, 2**63, 2**64 - 1, 12345678901234567]
    for v in ints: jobs.append(('fromul', v, 0))
    for v in (0, 1, -1, -2, 2**31 - 1, -2**31, 123456789, -987654321, 2**63 - 1, -2**63, -123456789012345678):
        jobs.append(('froml', v & (2**64 - 1), 0))
    for v in (0, 1, -1, 2**31 - 1, -2**31, 99999, -99999): jobs.append(('fromi', v & (2**64 - 1), 0))
    for v in (0, 1, 2**32 - 1, 2**31): jobs.append(('fromu', v, 0))
    for _ in range(14): jobs.append(('fromd', struct.unpack('<Q', struct.pack('<d', random.choice([random.uniform(-1e5, 1e5), random.uniform(-1e-5, 1e-5), 1e300, -1e-310, 0.0, -0.0, 5e-324, float('inf'), float('nan')])))[0], 0))
    for _ in range(8): jobs.append(('fromf', struct.unpack('<I', struct.pack('<f', random.choice([random.uniform(-1e3, 1e3), 1e-40, 3.4e38, 0.0, -0.0, float('inf')])))[0], 0))
    res = host(jobs)
    s = header('long double conversions to and from float, double, int, unsigned, long and unsigned long') + '\n'
    body = ''
    k = 2
    for (op, a, b), (rh, rl) in zip(jobs, res):
        src = {'fromul': '(unsigned long)%dUL', 'froml': '(long)%dL', 'fromi': '(int)%d', 'fromu': '(unsigned)%dU'}.get(op)
        if op == 'froml': v = a - 2**64 if a >= 2**63 else a; expr = '(long double)(long)(%dL)' % v if v != -2**63 else '(long double)(-9223372036854775807L-1)'
        elif op == 'fromi': v = a - 2**64 if a >= 2**63 else a; expr = '(long double)(int)(%d)' % v if v != -2**31 else '(long double)(-2147483647-1)'
        elif op == 'fromu': expr = '(long double)(unsigned)%dU' % a
        elif op == 'fromul': expr = '(long double)(unsigned long)%dUL' % a
        elif op == 'fromd': expr = '(long double)*(double *)&(u64[]){%dUL}' % b if False else None
        else: expr = None
        if expr:
            body += '\tif (!is(%s, 0x%016xUL, 0x%016xUL)) return %d;\n' % (expr, rh, rl, k)
        elif op == 'fromd':
            body += '\t{ u64 bits = %dUL; double d = *(double *)&bits; if (!is((long double)d, 0x%016xUL, 0x%016xUL)) return %d; }\n' % (a, rh, rl, k)
        else:
            body += '\t{ u32 bits = %dU; float f = *(float *)&bits; if (!is((long double)f, 0x%016xUL, 0x%016xUL)) return %d; }\n' % (a, rh, rl, k)
        k += 1
    # narrowing: from quads
    qs = [q(0, 16383, 0), q(0, 16383 + 5, 0x123456789abcdef), q(1, 16383 + 40, 77), q(0, 16383 + 62, ONE - 1), q(0, 16383 + 31, 5), q(1, 16383 + 31, 0), q(0, 16383 - 3, 0xabc),
          q(0, 16383 + 1100, 1), q(0, 16383 - 1100, 1), q(0, 16383 + 128, 3), q(1, 16383 + 70, 1), q(0, 16383 + 23, 0x123456789abcdef0123), q(0, 16383 - 140, 5), q(0, 16383 + 1023, 0)]
    nj = [(op, v, 0) for v in qs for op in ('tod', 'tof', 'toi', 'tol', 'tou', 'toul')]
    nres = host(nj)
    ctype = {'tod': ('double', 'u64', 'd'), 'tof': ('float', 'u32', 'f')}
    for (op, v, _), (rh, rl) in zip(nj, nres):
        h, l = lit(v)
        x = 'mk(0x%016xUL, 0x%016xUL)' % (h, l)
        # skip results that are undefined in C (out of range integers)
        sign = v >> 127; e = ((v >> 112) & 0x7fff) - 16383
        if op in ('toi', 'tou') and e >= 31 + (0 if op == 'tou' else 0): continue
        if op in ('tol',) and e >= 63: continue
        if op in ('toul',) and (e >= 64 or sign): continue
        if op == 'tou' and sign: continue
        if op in ('tod', 'tof'):
            t, u, c = ctype[op]
            body += '\t{ %s d = (%s)%s; %s *p = (%s *)&d; if (*p != 0x%x) return %d; }\n' % (t, t, x, u, u, rl, k)
        elif op == 'toi': body += '\tif ((int)%s != %d) return %d;\n' % (x, (rl if rl < 2**31 else rl - 2**32) if True else 0, k) if rl & 0xffffffff == rl else ''
        elif op == 'tol': body += '\tif ((long)%s != %dL) return %d;\n' % (x, rl - 2**64 if rl >= 2**63 else rl, k) if rl != 2**63 else ''
        elif op == 'tou': body += '\tif ((unsigned)%s != %dU) return %d;\n' % (x, rl & 0xffffffff, k)
        else: body += '\tif ((unsigned long)%s != %dUL) return %d;\n' % (x, rl, k)
        k += 1
    s += 'int test(void)\n{\n' + body + '\treturn 1;\n}\n'
    return s

def misc():
    s = header('long double: op=, ++, --, ?:, calls inside expressions, arrays') + '''
static long double F2(long double a, long double b) { return a * b + a; }
static long double G;
static long double *gp(void) { G = G + G; return &G; }

int test(void)
{
	long double x = mk(0x3fff000000000000UL, 0), two = mk(0x4000000000000000UL, 0), three = mk(0x4000800000000000UL, 0);
	long double a[3], *p = a;
	int n = 0, i;

	x += two;			/* 3 */
	if (!is(x, 0x4000800000000000UL, 0)) return 2;
	x *= two;			/* 6 */
	if (!is(x, 0x4001800000000000UL, 0)) return 3;
	x -= three;			/* 3 */
	x /= two;			/* 1.5 */
	if (!is(x, 0x3fff800000000000UL, 0)) return 4;
	x++;				/* 2.5 */
	if (!is(x, 0x4000400000000000UL, 0)) return 5;
	if (!is(x--, 0x4000400000000000UL, 0)) return 6;
	if (!is(x, 0x3fff800000000000UL, 0)) return 7;
	if (!is(++x, 0x4000400000000000UL, 0)) return 8;
	if (!is(--x, 0x3fff800000000000UL, 0)) return 9;
	a[0] = two; a[1] = three; a[2] = x;
	p[1] += a[0];			/* 5 */
	if (!is(a[1], 0x4001400000000000UL, 0)) return 10;
	*p++ *= two;			/* a[0] = 4, p -> a[1] */
	if (!is(a[0], 0x4001000000000000UL, 0) || p != a + 1) return 11;
	G = two;
	*gp() += three;			/* the call is evaluated once: G = 4 + 3 = 7 */
	if (!is(G, 0x4001c00000000000UL, 0)) return 12;
	if (!is(F2(two, three), 0x4002000000000000UL, 0)) return 13;	/* 2*3+2 = 8 */
	return 1;
}
'''
    return s

for name, text in (('q4', arith()), ('q5', compare()), ('q6', conv()), ('q7', misc())):
    open(name + '.c', 'w').write(text)
    print(name, len(text))
