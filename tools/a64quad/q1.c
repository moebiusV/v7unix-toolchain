/* expect: 1 -- long double is a 16-byte, 16-aligned binary128: size, layout, constants, copies, arrays, pointers */
struct S { char c; long double x; };
struct T { long double a; int n; };
typedef unsigned long u64;

static u64 hi(long double *p) { return ((u64 *)p)[1]; }
static u64 lo(long double *p) { return ((u64 *)p)[0]; }

long double g1 = 1.0L, g2 = -2.5L, gz = 0.0L;
long double ga[3] = { 1.0L, 2.0L, 0.5L };

int test(void)
{
	long double a = 1.5L, b, c[4];
	long double *p;
	struct S s;
	struct T t;
	int i;

	if (sizeof(long double) != 16) return 2;
	if (sizeof(struct S) != 32) return 3;
	if ((char *)&s.x - (char *)&s != 16) return 4;
	if (sizeof(struct T) != 32) return 5;
	if (hi(&g1) != 0x3fff000000000000UL || lo(&g1) != 0) return 6;
	if (hi(&g2) != 0xc000400000000000UL || lo(&g2) != 0) return 7;
	if (hi(&gz) != 0 || lo(&gz) != 0) return 8;
	if (hi(&a) != 0x3fff800000000000UL) return 9;
	b = a;
	if (hi(&b) != hi(&a) || lo(&b) != lo(&a)) return 10;
	for (i = 0; i < 4; i++)
		c[i] = ga[i % 3];
	if (hi(&c[1]) != 0x4000000000000000UL) return 11;
	if (hi(&c[2]) != 0x3ffe000000000000UL) return 12;
	if (hi(&c[3]) != hi(&c[0])) return 13;
	p = &c[2];
	if (p - c != 2) return 14;
	if (hi(p + 1) != 0x3fff000000000000UL) return 15;
	if (((char *)&c[1] - (char *)&c[0]) != 16) return 16;
	t.a = a;
	t.n = 7;
	if (hi(&t.a) != hi(&a) || t.n != 7) return 17;
	s.x = g2;
	if (hi(&s.x) != hi(&g2)) return 18;
	*p = g1;
	if (hi(&c[2]) != 0x3fff000000000000UL) return 19;
	return 1;
}
