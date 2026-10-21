/* expect: 1 -- functions that need saved registers, locals, calls with stack arguments, recursion and varargs, without a frame pointer; addresses of the form disp(base,index,scale); copies of registers */
typedef __builtin_va_list va_list;
#define va_start(v,l) __builtin_va_start(v,l)
#define va_arg(v,t) __builtin_va_arg(v,t)
#define va_end(v) __builtin_va_end(v)
struct S { long a[3]; unsigned char x[16]; unsigned char y[16]; long n; };
long sink;

long many(long a, long b, long c, long d, long e, long f, long g, long h, long i) { return a + 2 * b + 3 * c + 4 * d + 5 * e + 6 * f + 7 * g + 8 * h + 9 * i; }

long keep(long x, long y)
{
	long r = many(x, y, 3, 4, 5, 6, 7, 8, 9);	/* stack arguments: a frame, not eliminated */
	long s = x * 3;
	r += many(1, 2, 3, 4, 5, 6, 7, 8, 9);
	return r + s + y;
}

long chain(long x)				/* x lives across the calls: a callee-saved register or two */
{
	long a = x + 1;
	extern long ident(long);
	long b = ident(a);
	long c = ident(b + a);
	return a + b + c + x;
}
long ident(long v) { sink += v; return v; }

long loc(long n)				/* a local array and a call */
{
	long t[5];
	int i;

	for (i = 0; i < 5; i++) t[i] = n + i;
	return ident(t[0]) + t[4] + t[(n & 1) + 1];
}

long rec(long n) { return n <= 1 ? 1 : n * rec(n - 1); }

long vsum(int n, ...)
{
	va_list ap;
	long s = 0;

	va_start(ap, n);
	while (n-- > 0) s += va_arg(ap, long);
	va_end(ap);
	return s;
}

double dbl(double a, double b, long k) { double t[3]; t[0] = a; t[1] = b; t[2] = a * b; return ident(k) + t[2] + t[k & 1]; }

void xorb(struct S *s, unsigned long i) { s->x[i] ^= s->y[i]; }
long idx(struct S *s, long i) { return s->a[i] + s->n; }
struct S gs;

int test()
{
	struct S s;
	long i, v;

	if (keep(1, 2) != 575) return 2;
	if (chain(10) != 11 + 11 + 22 + 10) return 3;
	if (loc(4) != 17) return 4;
	if (rec(10) != 3628800L) return 5;
	if (vsum(4, 1L, 2L, 3L, 4L) != 10 || vsum(0) != 0) return 6;
	if (dbl(2.0, 3.0, 1) != 1 + 6.0 + 3.0) return 7;
	for (i = 0; i < 16; i++) { gs.x[i] = i; gs.y[i] = 255 - i; }
	for (i = 0; i < 16; i++) xorb(&gs, i);
	for (i = 0; i < 16; i++) if (gs.x[i] != (i ^ (255 - i))) return 8;
	gs.a[0] = 10; gs.a[1] = 20; gs.a[2] = 30; gs.n = 5;
	for (i = 0; i < 3; i++) if (idx(&gs, i) != 10 * (i + 1) + 5) return 9;
	s.a[2] = 77; s.n = 1;
	v = idx(&s, 2);
	if (v != 78) return 10;
	return 1;
}
