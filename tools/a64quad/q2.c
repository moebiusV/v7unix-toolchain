/* expect: 1 -- long double arguments and results: registers, stack, mixed with int and double, structs of them */
typedef unsigned long u64;
struct One { long double x; };
struct Two { long double a, b; };
struct Mix { double d; double e; };

static u64 hi(long double *p) { return ((u64 *)p)[1]; }

long double id(long double x) { return x; }
long double second(int a, long double x, double d, long double y, int b)
{
	if (a != 5 || b != 9 || d != 2.0) return 0.0L;
	return y;
}
long double ninth(long double a, long double b, long double c, long double d, long double e,
    long double f, long double g, long double h, long double i, long double j)
{
	return j;
}
long double tenth_mixed(int a, long double v0, long double v1, long double v2, long double v3,
    long double v4, long double v5, long double v6, long double v7, int b, long double stk, int c)
{
	if (a != 1 || b != 2 || c != 3) return 0.0L;
	return stk;
}
struct One sone(struct One s) { return s; }
struct Two stwo(struct Two s) { struct Two r; r.a = s.b; r.b = s.a; return r; }
long double sfirst(struct Two s, long double extra) { return extra; }

int test(void)
{
	long double a = 1.5L, b = 2.5L, r;
	long double v[10];
	struct One o, o2;
	struct Two t, t2;
	int i;

	r = id(a);
	if (hi(&r) != hi(&a)) return 2;
	r = second(5, a, 2.0, b, 9);
	if (hi(&r) != hi(&b)) return 3;
	for (i = 0; i < 10; i++)
		v[i] = (i == 9) ? b : a;
	r = ninth(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9]);
	if (hi(&r) != hi(&b)) return 4;
	r = tenth_mixed(1, a, a, a, a, a, a, a, a, 2, b, 3);
	if (hi(&r) != hi(&b)) return 5;
	o.x = b;
	o2 = sone(o);
	if (hi(&o2.x) != hi(&b)) return 6;
	t.a = a; t.b = b;
	t2 = stwo(t);
	if (hi(&t2.a) != hi(&b) || hi(&t2.b) != hi(&a)) return 7;
	r = sfirst(t, b);
	if (hi(&r) != hi(&b)) return 8;
	return 1;
}
