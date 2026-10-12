/* expect: 1 -- long * / % with register variables pinned in r4/r3/r2 */
long ga, gb;
long lid(x) long x; { return x; }

long calc(a, b)
long a, b;
{
	register int i, j, k;
	long r;

	i = 3; j = 4; k = 5;
	r = a * b + a / b - a % b;		/* both operands in memory */
	r += lid(a) * lid(b);			/* both from calls */
	r += (a + b) * (a - b);			/* both computed */
	r += a * 10 + (long)i * j - k;
	return r;
}

int test(void)
{
	long want, got;

	ga = 70000L; gb = 123L;
	got = calc(ga, gb);
	want = 70000L*123L + 70000L/123L - 70000L%123L
	     + 70000L*123L
	     + (70000L+123L)*(70000L-123L)
	     + 70000L*10 + 12 - 5;
	return got == want;
}
