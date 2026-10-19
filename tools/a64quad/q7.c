/* expect: 1 -- long double: op=, ++, --, ?:, calls inside expressions, arrays; expected values from the host's __float128 (mkarith.py) */
typedef unsigned long u64;
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
