/* expect: 1 -- 'register' parameters and locals live in r4/r3/r2 across calls */
int sink;
int id(x) int x; { return x; }

/* K&R parameters, three register variables, calls in between */
int sum3(a, b, c)
register int a, b;
int c;
{
	register int t;

	t = id(a) + id(b);
	t += id(c);
	return t + a + b;
}

/* prototype-style, a char and a pointer */
int walk(register char *s, register int n)
{
	register int h;

	h = 0;
	while (n-- > 0)
		h = h * 3 + id(*s++);
	return h;
}

/* more register variables than registers: the extra ones are plain autos */
int five(a, b, c, d, e)
register int a, b, c, d, e;
{
	return id(a) + 2 * id(b) + 3 * id(c) + 4 * id(d) + 5 * id(e);
}

/* recursion: each frame's registers must be saved and restored */
int fact(register int n)
{
	register int r;

	if (n <= 1)
		return 1;
	r = fact(n - 1);
	return n * r;
}

/* register char */
int cnt(s, ch)
register char *s;
register char ch;
{
	register int n;

	n = 0;
	while (*s)
		if (*s++ == ch)
			n++;
	return n;
}

int test(void)
{
	int bad;

	bad = 0;
	if (sum3(1, 2, 3) != 9) bad |= 2;		/* 1+2+3 + 1+2 */
	if (walk("ab", 2) != 97*3 + 98) bad |= 4;
	if (five(1, 2, 3, 4, 5) != 1+4+9+16+25) bad |= 8;
	if (fact(6) != 720) bad |= 16;
	if (cnt("banana", 'a') != 3) bad |= 32;
	return bad == 0;
}
