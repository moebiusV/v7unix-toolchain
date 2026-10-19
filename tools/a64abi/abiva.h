/* structs through "...", checked against clang: see abi.h for the scheme */
#define CAT2(a, b) a ## b
#define CAT(a, b) CAT2(a, b)
#define N(x) CAT(PFX, x)
#define O(x) CAT(OFX, x)
typedef __builtin_va_list va_list;
#define va_start(v, l) __builtin_va_start(v, l)
#define va_arg(v, t) __builtin_va_arg(v, t)
#define va_end(v) __builtin_va_end(v)

struct V8  { int a, b; };
struct V12 { int a, b, c; };
struct V16 { long a, b; };
struct V24 { long a, b, c; };

/* n structs of the kinds in k[], then an int and a double, summed in a way that shows every field */
long N(vsum)(const char *k, int n, ...)
{
	va_list ap;
	long s = 0;
	int i;

	va_start(ap, n);
	for (i = 0; i < n; i++) {
		switch (k[i]) {
		case '8': { struct V8 v = va_arg(ap, struct V8); s = s * 10 + v.a + v.b; break; }
		case 'c': { struct V12 v = va_arg(ap, struct V12); s = s * 10 + v.a + v.b + v.c; break; }
		case 'g': { struct V16 v = va_arg(ap, struct V16); s = s * 10 + v.a + v.b; break; }
		case 'x': { struct V24 v = va_arg(ap, struct V24); s = s * 10 + v.a + v.b + v.c; break; }
		case 'i': s = s * 10 + va_arg(ap, int); break;
		case 'l': s = s * 10 + va_arg(ap, long); break;
		}
	}
	va_end(ap);
	return s;
}

long O(vsum)(const char *, int, ...);

int N(check)(char **K)
{
	struct V8 a; struct V12 b; struct V16 c; struct V24 d;

	a.a = 1; a.b = 1; b.a = 1; b.b = 1; b.c = 1; c.a = 1; c.b = 2; d.a = 1; d.b = 1; d.c = 2;
	if (O(vsum)(K[0], 1, a) != 2) return 101;
	if (O(vsum)(K[1], 1, b) != 3) return 102;
	if (O(vsum)(K[2], 1, c) != 3) return 103;
	if (O(vsum)(K[3], 1, d) != 4) return 104;
	/* mixed, enough to run out of registers and continue on the stack */
	if (O(vsum)(K[4], 4, a, b, c, d) != 2334) return 105;
	if (O(vsum)(K[5], 5, 7, a, b, c, d) != 72334) return 106;
	if (O(vsum)(K[6], 9, 1, 1, 1, 1, 1, 1, a, c, d) != 111111234) return 107;
	if (O(vsum)(K[7], 9, 1, 1, 1, 1, 1, 1, 1, a, 5L) != 111111125) return 108;
	if (O(vsum)(K[8], 4, c, c, c, c) != 3333) return 109;
	return 1;
}
