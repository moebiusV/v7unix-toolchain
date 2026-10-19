/* expect: 1 -- long double through "...": in the registers, on the stack, mixed with int and double */
typedef __builtin_va_list va_list;
#define va_start(v,l) __builtin_va_start(v,l)
#define va_arg(v,t) __builtin_va_arg(v,t)
#define va_end(v) __builtin_va_end(v)
typedef unsigned long u64;

/* the high words of n long doubles, each possibly preceded by an int or a double (kinds in k) */
static u64 pick(const char *k, int n, ...)
{
	va_list ap;
	u64 s = 0;
	int i;
	long double x;

	va_start(ap, n);
	for (i = 0; i < n; i++) {
		if (k[i] == 'i')
			s = s * 3 + (u64)va_arg(ap, int);
		else if (k[i] == 'd')
			s = s * 3 + (va_arg(ap, double) == 2.0);
		else {
			x = va_arg(ap, long double);
			s = s * 3 + (((u64 *)&x)[1] >> 48 & 7);
		}
	}
	va_end(ap);
	return s;
}

int test(void)
{
	long double a = 1.5L, b = 2.5L;	/* high words 0x3fff8.. and 0x40004..: low bits of the top 16: 7 and 0 */

	if (pick("q", 1, a) != 7) return 2;
	if (pick("qq", 2, a, b) != 21) return 3;
	if (pick("iqdq", 4, 4, a, 2.0, b) != ((4*3+7)*3+1)*3) return 4;
	/* six quads: the first eight fp registers are not enough once ints and doubles are mixed in */
	if (pick("qqqqqqqqqq", 10, a, a, a, a, a, a, a, a, a, b) != 7*19683*3 + 7*6561*3*0 + 0 + 7*(19683*3-1)/2*0 + 0) {
		/* computed below instead */
	}
	{
		u64 want = 0;
		int i;
		for (i = 0; i < 10; i++)
			want = want * 3 + (i == 9 ? 0 : 7);
		if (pick("qqqqqqqqqq", 10, a, a, a, a, a, a, a, a, a, b) != want) return 5;
	}
	if (pick("iiiiiiiqiq", 10, 1, 1, 1, 1, 1, 1, 1, a, 1, b) == 0) return 6;
	return 1;
}
