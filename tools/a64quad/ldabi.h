/* long double (binary128) across the compiler boundary; no literals and no arithmetic: only moves */
#define CAT2(a, b) a ## b
#define CAT(a, b) CAT2(a, b)
#define N(x) CAT(PFX, x)
#define O(x) CAT(OFX, x)
typedef unsigned long u64;
typedef __builtin_va_list va_list;
#define va_start(v, l) __builtin_va_start(v, l)
#define va_arg(v, t) __builtin_va_arg(v, t)
#define va_end(v) __builtin_va_end(v)

struct One { long double x; };
struct Two { long double a, b; };
struct Dbl { double d; };

static long double N(mk)(u64 hi)
{
	u64 w[2];
	long double r;

	w[0] = hi * 3;
	w[1] = hi;
	r = *(long double *)w;
	return r;
}
static u64 N(hi)(long double x) { return ((u64 *)&x)[1]; }
static u64 N(lo)(long double x) { return ((u64 *)&x)[0]; }

long double N(id)(long double x) { return x; }
long double N(second)(int a, long double x, double d, long double y, int b)
{
	if (a != 5 || b != 9 || d != 2.0) return N(mk)(0);
	return y;
}
long double N(ninth)(long double a, long double b, long double c, long double d, long double e,
    long double f, long double g, long double h, long double i, long double j)
{
	return j;
}
long double N(stk)(int a, long double v0, long double v1, long double v2, long double v3,
    long double v4, long double v5, long double v6, long double v7, int b, long double s, int c)
{
	if (a != 1 || b != 2 || c != 3) return N(mk)(0);
	return s;
}
struct One N(sone)(struct One s) { return s; }
struct Two N(stwo)(struct Two s) { struct Two r; r.a = s.b; r.b = s.a; return r; }
long double N(sfirst)(struct Two s, long double extra) { return extra; }
u64 N(vpick)(int n, ...)
{
	va_list ap;
	u64 s = 0;
	int i;

	va_start(ap, n);
	for (i = 0; i < n; i++) {
		if (i % 3 == 0)
			s = s * 5 + (u64)va_arg(ap, int);
		else if (i % 3 == 1) {
			long double x = va_arg(ap, long double);
			s = s * 5 + (((u64 *)&x)[1] >> 40 & 3) + 1;
		} else
			s = s * 5 + (va_arg(ap, double) == 2.0);
	}
	va_end(ap);
	return s;
}

long double O(id)(long double);
long double O(second)(int, long double, double, long double, int);
long double O(ninth)(long double, long double, long double, long double, long double,
    long double, long double, long double, long double, long double);
long double O(stk)(int, long double, long double, long double, long double, long double, long double,
    long double, long double, int, long double, int);
struct One O(sone)(struct One);
struct Two O(stwo)(struct Two);
long double O(sfirst)(struct Two, long double);
u64 O(vpick)(int, ...);

int N(check)(void)
{
	long double a = N(mk)(0x3fff800000000000UL), b = N(mk)(0x4000400000000000UL), r;
	long double v[10];
	struct One o, o2;
	struct Two t, t2;
	int i;

	r = O(id)(a);
	if (N(hi)(r) != N(hi)(a) || N(lo)(r) != N(lo)(a)) return 101;
	r = O(second)(5, a, 2.0, b, 9);
	if (N(hi)(r) != N(hi)(b) || N(lo)(r) != N(lo)(b)) return 102;
	for (i = 0; i < 10; i++)
		v[i] = (i == 9) ? b : a;
	r = O(ninth)(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9]);
	if (N(hi)(r) != N(hi)(b)) return 103;
	r = O(stk)(1, a, a, a, a, a, a, a, a, 2, b, 3);
	if (N(hi)(r) != N(hi)(b) || N(lo)(r) != N(lo)(b)) return 104;
	o.x = b;
	o2 = O(sone)(o);
	if (N(hi)(o2.x) != N(hi)(b)) return 105;
	t.a = a; t.b = b;
	t2 = O(stwo)(t);
	if (N(hi)(t2.a) != N(hi)(b) || N(hi)(t2.b) != N(hi)(a)) return 106;
	r = O(sfirst)(t, b);
	if (N(hi)(r) != N(hi)(b)) return 107;
	/* through "..." */
	if (O(vpick)(3, 7, a, 2.0) != (7UL * 5 + 1 + 0xff) * 5 + 1 - 0xff * 5) {
		/* the expected value is computed with our own copy */
	}
	if (O(vpick)(3, 7, a, 2.0) != N(vpick)(3, 7, a, 2.0)) return 108;
	if (O(vpick)(7, 1, a, 2.0, 3, b, 2.0, 4) != N(vpick)(7, 1, a, 2.0, 3, b, 2.0, 4)) return 109;
	if (O(vpick)(12, 1, a, 2.0, 1, a, 2.0, 1, a, 2.0, 1, b, 2.0) != N(vpick)(12, 1, a, 2.0, 1, a, 2.0, 1, a, 2.0, 1, b, 2.0)) return 110;
	return 1;
}
