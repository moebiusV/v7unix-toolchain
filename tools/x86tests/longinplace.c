/* expect: 1 -- long locals and parameters updated in place, several live across calls and long multiplies */
long lcalls;
long ident(v) long v; { lcalls++; return v; }

long two(offset, n) long offset; int n;
{
	long p, q;

	p = ident(1000L);
	q = 70000L;
	p += n - ident(3L);
	offset -= n;
	q += offset;
	p += ident(2L);
	q -= p;
	return p * 3 + q + offset;
}

long mixed(a, b) long a, b;
{
	long t;
	int i;

	t = 0;
	for (i = 0; i < 5; i++) {
		t += a;
		a -= ident(1L);
		t -= b / 2;
		b += i;
	}
	return t + a + b;
}

int test(void)
{
	int bad = 0;
	long r;

	/* the values are those of the same code with 32-bit ints on the host */
	if ((r = two(5000L, 7)) != 81998L) bad |= 1;
	if ((r = mixed(10L, 20L)) != 21L) bad |= 2;
	if ((r = two(70000L, 100)) != 211998L) bad |= 4;
	if ((r = mixed(-100000L, 100000L)) != -750009L) bad |= 8;
	return bad == 0;
}
