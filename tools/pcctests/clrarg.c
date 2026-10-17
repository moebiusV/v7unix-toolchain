/* expect: 1 -- a long argument made from an unsigned has a zero high word (clr -(sp)) */
long id(a, b, c) long a; int b; long c; { return a + b + c; }

int test()
{
	unsigned u;
	int m, bad;
	long r;

	bad = 0;
	u = 50000;
	m = 7;
	r = id((long)u, 3, (long)(m * sizeof(long[10])));
	if (r != 50283L) bad |= 1;
	r = id((long)(m * sizeof(long[10])), 1, (long)u);
	if (r != 50281L) bad |= 2;
	return bad ? 2 * bad + 1000 : 1;
}
