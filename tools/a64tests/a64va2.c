/* expect: 1 -- varargs: mixed int/pointer/double, named stack parameters, va_list passed on, va_copy, structs */
typedef __builtin_va_list va_list;
#define va_start(v,l) __builtin_va_start(v,l)
#define va_arg(v,t) __builtin_va_arg(v,t)
#define va_end(v) __builtin_va_end(v)
#define va_copy(d,s) __builtin_va_copy(d,s)

struct pt { long x, y; };
struct big { long a[4]; };

static long vsum(int n, va_list ap)
{
	long s = 0;
	while (n-- > 0)
		s += va_arg(ap, int);
	return s;
}
long viasum(int n, ...)
{
	va_list ap;
	long r;
	va_start(ap, n);
	r = vsum(n, ap);
	va_end(ap);
	return r;
}
/* a format-driven walk: i int, l long, d double, s string length, p struct pt */
long fmt(const char *f, ...)
{
	va_list ap;
	long r = 0;
	const char *s;
	va_start(ap, f);
	for (; *f; f++) {
		switch (*f) {
		case 'i': r += va_arg(ap, int); break;
		case 'l': r += va_arg(ap, long); break;
		case 'd': r += (long)va_arg(ap, double); break;
		case 's': for (s = va_arg(ap, const char *); *s; s++) r++; break;
		case 'p': { struct pt p = va_arg(ap, struct pt); r += p.x * 10 + p.y; break; }
		case 'b': { struct big b = va_arg(ap, struct big); r += b.a[0] + b.a[3]; break; }
		}
	}
	va_end(ap);
	return r;
}
/* nine named integer parameters: the variable part starts on the stack */
long many(long a, long b, long c, long d, long e, long f, long g, long h, long i, ...)
{
	va_list ap;
	long r = a + b + c + d + e + f + g + h + i;
	va_start(ap, i);
	r += va_arg(ap, long);
	r += va_arg(ap, long) * 100;
	va_end(ap);
	return r;
}
long twice(int n, ...)
{
	va_list ap, bp;
	long s = 0;
	va_start(ap, n);
	va_copy(bp, ap);
	while (n-- > 0)
		s += va_arg(ap, long);
	s += va_arg(bp, long) * 1000;
	va_end(ap);
	va_end(bp);
	return s;
}
int test(void)
{
	struct pt p;
	struct big b;
	p.x = 3; p.y = 4;
	b.a[0] = 5; b.a[1] = 0; b.a[2] = 0; b.a[3] = 6;
	if (viasum(4, 1, 2, 3, 4) != 10) return 0;
	if (viasum(12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12) != 78) return 0;
	if (fmt("ildsp", 1, 20L, 3.9, "abcd", p) != 1 + 20 + 3 + 4 + 34) return 0;
	if (fmt("bi", b, 7) != 5 + 6 + 7) return 0;
	if (fmt("dddddddddd", 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0) != 55) return 0;
	if (fmt("iiiiiiiiiiiiii", 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1) != 14) return 0;
	if (many(1, 2, 3, 4, 5, 6, 7, 8, 9, 10L, 2L) != 45 + 10 + 200) return 0;
	if (twice(3, 1L, 2L, 3L) != 6 + 1000) return 0;
	return 1;
}
