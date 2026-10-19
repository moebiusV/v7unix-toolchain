/*
 * tf.c -- IEEE 754 binary128 arithmetic for the emulator tests, under libgcc's names (__addtf3,
 * __lttf2, __extenddftf2, ...).  Round to nearest even; no exceptions; quiet NaNs.  Written for clarity,
 * not speed, on unsigned __int128, and checked against the host's libgcc by tf-check.sh before the
 * emulator tests rely on it.
 *
 * On AArch64 (clang) long double is the quad; on a host the check compiles it with -DTFHOST, where the quad
 * is __float128, and -DTFPFX=my_ to put the functions under another prefix.
 */
typedef unsigned __int128 u128;
typedef unsigned long long u64;
typedef long long s64;
typedef unsigned int u32;
typedef int s32;

#ifdef TFHOST
typedef __float128 qd;
#else
typedef long double qd;
#endif

#ifndef TFPFX
#define TFPFX __
#endif
#define CAT2(a, b) a ## b
#define CAT(a, b) CAT2(a, b)
#define F(n) CAT(TFPFX, n)

#define ONE	((u128)1)
/* keep the optimizer from seeing a floating-point operation in integer code and calling the very function being defined
   (a 128-bit operand of an asm statement loses its high half on AArch64 clang: hide the two halves) */
#define HIDE(v)	do { u64 h_ = (u64)((v) >> 64), l_ = (u64)(v); \
	__asm__("" : "+r"(h_), "+r"(l_)); (v) = ((u128)h_ << 64) | l_; } while (0)
#define SIGNQ	(ONE << 127)
#define FRACQ	((ONE << 112) - 1)
#define INFQ	(((u128)0x7fff) << 112)
#define QNANQ	(((u128)0x7fff8) << 108)

static u128 q2b(qd x) { u128 r; __builtin_memcpy(&r, &x, 16); return r; }
static qd b2q(u128 r) { qd x; __builtin_memcpy(&x, &r, 16); return x; }

/* a finite nonzero number is m * 2^(e-126) with bit 126 of m set; cls 0 zero, 1 finite, 2 inf, 3 nan */
typedef struct { int sign, cls, e; u128 m; } U;

static int top(u128 v)	/* index of the highest set bit */
{
	int n = 0;

	if (v >> 64) { n += 64; v >>= 64; }
	if (v >> 32) { n += 32; v >>= 32; }
	if (v >> 16) { n += 16; v >>= 16; }
	if (v >> 8) { n += 8; v >>= 8; }
	if (v >> 4) { n += 4; v >>= 4; }
	if (v >> 2) { n += 2; v >>= 2; }
	if (v >> 1) n += 1;
	return n;
}

/* from an encoding with mb mantissa bits (hidden one included) and eb exponent bits, in the low bits of v */
static U unpack(u128 v, int mb, int eb)
{
	U u;
	int bias = (1 << (eb - 1)) - 1;
	int ex;
	u128 fr = v & ((ONE << (mb - 1)) - 1);
	int t;

	HIDE(v);
	ex = (int)((v >> (mb - 1)) & ((1 << eb) - 1));
	u.sign = (int)((v >> (mb - 1 + eb)) & 1);
	u.e = 0;
	u.m = 0;
	if (ex == (1 << eb) - 1) {
		u.cls = fr ? 3 : 2;
	} else if (ex == 0 && fr == 0) {
		u.cls = 0;
	} else {
		u.cls = 1;
		if (ex == 0) {
			fr <<= 0;
			t = top(fr);
			u.e = 1 - bias - (mb - 1 - t);
			u.m = fr << (126 - t);
		} else {
			u.e = ex - bias;
			u.m = (fr | (ONE << (mb - 1))) << (126 - (mb - 1));
		}
	}
	return u;
}

/*
 * Round value m * 2^(e-126) (bit 126 of m set, sticky: more, nonzero, below m's lsb) to a format with mb
 * mantissa bits and eb exponent bits; return its encoding with the sign in place.
 */
static u128 pack(int sign, int e, u128 m, int sticky, int mb, int eb)
{
	int bias = (1 << (eb - 1)) - 1, emin = 1 - bias, emax = bias;
	int shift = 127 - mb;
	u128 q, rem, half, sg = (u128)sign << (mb - 1 + eb);

	if (e < emin) {
		int d = emin - e;

		if (d >= 127) {
			sticky |= m != 0;
			m = 0;
		} else {
			sticky |= (m & ((ONE << d) - 1)) != 0;
			m >>= d;
		}
		e = emin;
	}
	q = m >> shift;
	rem = m & ((ONE << shift) - 1);
	half = ONE << (shift - 1);
	if (rem > half || (rem == half && (sticky || (q & 1))))
		q++;
	if (q >> mb) {			/* carried into a new top bit */
		q >>= 1;
		e++;
	}
	if (e > emax)
		return sg | ((u128)((1 << eb) - 1) << (mb - 1));
	if (q >> (mb - 1))		/* normal */
		return sg | ((u128)(e + bias) << (mb - 1)) | (q & ((ONE << (mb - 1)) - 1));
	return sg | q;			/* subnormal, or zero */
}

static u128 packq(U u, int sticky) { return pack(u.sign, u.e, u.m, sticky, 113, 15); }
static u128 qnan(void) { return QNANQ; }
static u128 qinf(int s) { return INFQ | ((u128)s << 127); }
static u128 qzero(int s) { return (u128)s << 127; }

/* a + b (b's sign already flipped for subtraction) */
static u128 addq(u128 x, u128 y)
{
	U a = unpack(x, 113, 15), b = unpack(y, 113, 15), t;
	u128 ma, mb;
	int d, sticky = 0;

	if (a.cls == 3 || b.cls == 3)
		return qnan();
	if (a.cls == 2)
		return (b.cls == 2 && a.sign != b.sign) ? qnan() : qinf(a.sign);
	if (b.cls == 2)
		return qinf(b.sign);
	if (a.cls == 0 && b.cls == 0)
		return qzero(a.sign & b.sign);
	if (a.cls == 0)
		return y;
	if (b.cls == 0)
		return x;
	if (a.e < b.e || (a.e == b.e && a.m < b.m)) {	/* a is the larger magnitude */
		t = a; a = b; b = t;
	}
	d = a.e - b.e;
	ma = a.m >> 1;			/* top at 125: room above for a carry */
	mb = b.m >> 1;
	if (d >= 126) {
		sticky = 1;
		mb = 0;
	} else if (d) {
		sticky = (mb & ((ONE << d) - 1)) != 0;
		mb >>= d;
	}
	/* r * 2^(a.e - 125); the bits lost below mb make a sticky fraction */
	if (a.sign == b.sign) {
		ma += mb;
	} else {
		ma -= mb;
		if (sticky)
			ma--;		/* a - (mb + f) = (a - mb - 1) + (1 - f), and 1 - f is nonzero */
		if (ma == 0 && !sticky)
			return qzero(0);
	}
	d = top(ma);
	a.e = a.e - 125 + d;
	a.m = ma << (126 - d);
	return packq(a, sticky);
}

/* 128 x 128 -> 256 bits */
static void mul256(u128 a, u128 b, u128 *hi, u128 *lo)
{
	u64 a0 = (u64)a, a1 = (u64)(a >> 64), b0 = (u64)b, b1 = (u64)(b >> 64);
	u128 p00 = (u128)a0 * b0, p01 = (u128)a0 * b1, p10 = (u128)a1 * b0, p11 = (u128)a1 * b1;
	u128 mid = (p00 >> 64) + (u64)p01 + (u64)p10;

	*lo = (u64)p00 | (mid << 64);
	*hi = p11 + (p01 >> 64) + (p10 >> 64) + (mid >> 64);
}

static u128 mulq(u128 x, u128 y)
{
	U a = unpack(x, 113, 15), b = unpack(y, 113, 15);
	u128 hi, lo, m;
	int s = a.sign ^ b.sign, sticky, e, sh;

	if (a.cls == 3 || b.cls == 3)
		return qnan();
	if (a.cls == 2 || b.cls == 2) {
		if (a.cls == 0 || b.cls == 0)
			return qnan();
		return qinf(s);
	}
	if (a.cls == 0 || b.cls == 0)
		return qzero(s);
	mul256(a.m, b.m, &hi, &lo);		/* both have the top at 126: product top at 252 or 253 */
	e = a.e + b.e;
	sh = (hi >> 125) ? 127 : 126;		/* hi is bits 128..255: top bit 253 is hi bit 125 */
	if (sh == 127)
		e++;
	/* want the top at bit 126: shift the 256-bit product right by (top - 126) */
	{
		int tp = sh == 127 ? 253 : 252, r = tp - 126;		/* r in 126..127 */
		sticky = (lo & ((ONE << r) - 1)) != 0;
		m = (lo >> r) | (hi << (128 - r));
	}
	a.sign = s;
	a.m = m;
	a.e = e;
	return packq(a, sticky);
}

static u128 divq(u128 x, u128 y)
{
	U a = unpack(x, 113, 15), b = unpack(y, 113, 15);
	u128 q = 0, r, mb;
	int s = a.sign ^ b.sign, e, i;

	if (a.cls == 3 || b.cls == 3)
		return qnan();
	if (a.cls == 2)
		return b.cls == 2 ? qnan() : qinf(s);
	if (b.cls == 2)
		return qzero(s);
	if (b.cls == 0)
		return a.cls == 0 ? qnan() : qinf(s);
	if (a.cls == 0)
		return qzero(s);
	e = a.e - b.e;
	r = a.m >> 14;				/* 113-bit significands: top at 112 */
	mb = b.m >> 14;
	if (r < mb) {
		r <<= 1;
		e--;
	}
	for (i = 0; i < 127; i++) {		/* 127 quotient bits, the first is the integer one */
		q <<= 1;
		if (r >= mb) {
			r -= mb;
			q |= 1;
		}
		r <<= 1;
	}
	a.sign = s;
	a.m = q;				/* top at 126 */
	a.e = e;
	return packq(a, r != 0);
}

/* the order of two numbers: -1, 0, 1, or 2 when one is a NaN */
static int cmpq(u128 x, u128 y)
{
	U a = unpack(x, 113, 15), b = unpack(y, 113, 15);
	u128 ma, mb;

	HIDE(x);
	HIDE(y);
	ma = x & ~SIGNQ;
	mb = y & ~SIGNQ;
	if (a.cls == 3 || b.cls == 3)
		return 2;
	if (ma == 0 && mb == 0)
		return 0;
	if (a.sign != b.sign)
		return a.sign ? -1 : 1;
	if (ma == mb)
		return 0;
	return (ma < mb) != (a.sign != 0) ? -1 : 1;
}

qd F(addtf3)(qd a, qd b) { return b2q(addq(q2b(a), q2b(b))); }
qd F(subtf3)(qd a, qd b) { return b2q(addq(q2b(a), q2b(b) ^ SIGNQ)); }
qd F(multf3)(qd a, qd b) { return b2q(mulq(q2b(a), q2b(b))); }
qd F(divtf3)(qd a, qd b) { return b2q(divq(q2b(a), q2b(b))); }

/* libgcc's conventions: an unordered compare answers so that the C comparison is false */
int F(eqtf2)(qd a, qd b) { int c = cmpq(q2b(a), q2b(b)); return c == 0 ? 0 : 1; }
int F(netf2)(qd a, qd b) { int c = cmpq(q2b(a), q2b(b)); return c == 0 ? 0 : 1; }
int F(lttf2)(qd a, qd b) { int c = cmpq(q2b(a), q2b(b)); return c == 2 ? 1 : c; }
int F(letf2)(qd a, qd b) { int c = cmpq(q2b(a), q2b(b)); return c == 2 ? 1 : c; }
int F(gttf2)(qd a, qd b) { int c = cmpq(q2b(a), q2b(b)); return c == 2 ? -1 : c; }
int F(getf2)(qd a, qd b) { int c = cmpq(q2b(a), q2b(b)); return c == 2 ? -1 : c; }
int F(unordtf2)(qd a, qd b) { return cmpq(q2b(a), q2b(b)) == 2; }

/* floating-point formats to and from the quad */
static qd ext(u128 v, int mb, int eb)
{
	U u = unpack(v, mb, eb);

	if (u.cls == 3)
		return b2q(qnan());
	if (u.cls == 2)
		return b2q(qinf(u.sign));
	if (u.cls == 0)
		return b2q(qzero(u.sign));
	return b2q(packq(u, 0));
}

static u128 trunc_to(qd x, int mb, int eb)
{
	U u = unpack(q2b(x), 113, 15);
	u128 s = (u128)u.sign << (mb - 1 + eb), all = ((u128)((1 << eb) - 1)) << (mb - 1);

	if (u.cls == 3)
		return all | (ONE << (mb - 2));
	if (u.cls == 2)
		return s | all;
	if (u.cls == 0)
		return s;
	return pack(u.sign, u.e, u.m, 0, mb, eb);
}

qd F(extendsftf2)(float f) { u32 v; __builtin_memcpy(&v, &f, 4); return ext(v, 24, 8); }
qd F(extenddftf2)(double d) { u64 v; __builtin_memcpy(&v, &d, 8); return ext(v, 53, 11); }
float F(trunctfsf2)(qd x) { u32 v = (u32)trunc_to(x, 24, 8); float f; __builtin_memcpy(&f, &v, 4); return f; }
double F(trunctfdf2)(qd x) { u64 v = (u64)trunc_to(x, 53, 11); double d; __builtin_memcpy(&d, &v, 8); return d; }

/* integers */
static qd fromu(u64 v, int neg)
{
	U u;

	if (v == 0)
		return b2q(0);
	u.sign = neg;
	u.cls = 1;
	u.e = top(v);
	u.m = (u128)v << (126 - u.e);
	return b2q(packq(u, 0));
}

qd F(floatsitf)(s32 i) { return fromu(i < 0 ? -(u64)(s64)i : (u64)i, i < 0); }
qd F(floatunsitf)(u32 i) { return fromu(i, 0); }
qd F(floatditf)(s64 i) { return fromu(i < 0 ? -(u64)i : (u64)i, i < 0); }
qd F(floatunditf)(u64 i) { return fromu(i, 0); }

/* truncate toward zero; out of range saturates, a NaN is 0 */
static u128 tou(qd x, int *neg, int *over)
{
	U u = unpack(q2b(x), 113, 15);
	u128 v;

	*neg = u.sign;
	*over = 0;
	if (u.cls == 3)
		return 0;
	if (u.cls == 2) {
		*over = 1;
		return 0;
	}
	if (u.cls == 0 || u.e < 0)
		return 0;
	if (u.e >= 127) {
		*over = 1;
		return 0;
	}
	v = u.m >> (126 - u.e);
	return v;
}

s32 F(fixtfsi)(qd x)
{
	int n, o;
	u128 v = tou(x, &n, &o);

	if (o || v > (n ? 2147483648u : 2147483647u))
		return n ? (-2147483647 - 1) : 2147483647;
	return n ? (s32)-(s64)v : (s32)v;
}

s64 F(fixtfdi)(qd x)
{
	int n, o;
	u128 v = tou(x, &n, &o);

	if (o || v > (n ? (u128)1 << 63 : ((u128)1 << 63) - 1))
		return n ? (-9223372036854775807LL - 1) : 9223372036854775807LL;
	return n ? (s64)(0 - (u64)v) : (s64)v;
}

u32 F(fixunstfsi)(qd x)
{
	int n, o;
	u128 v = tou(x, &n, &o);

	if (n)
		return 0;
	if (o || v > 0xffffffffu)
		return 0xffffffffu;
	return (u32)v;
}

u64 F(fixunstfdi)(qd x)
{
	int n, o;
	u128 v = tou(x, &n, &o);

	if (n)
		return 0;
	if (o || v > 0xffffffffffffffffULL)
		return 0xffffffffffffffffULL;
	return (u64)v;
}
