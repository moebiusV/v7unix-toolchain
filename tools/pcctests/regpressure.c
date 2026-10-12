/* expect: 1 -- three register variables live across div/mod/shift; long values live across a call */
int g, h;
unsigned gu, hu;
long lg;
int ts, ds;

int half(x) int x; { return x / 2; }

int divs(register int a, register int t, register int c)
{
	g = h / 8;
	g += h % 5;
	gu = hu >> 3;
	gu >>= 1;
	return a + t + c;
}

long span(loc)
long loc;
{
	register int *sp;
	register int *lp;

	sp = &g; lp = &h;
	return loc + ts + half(ds) + *sp + *lp;
}

long shifts(register int a, register int b, long v)
{
	long w;

	w = v << 3;
	w += (v << a);
	w += half(b);
	w <<= 2;
	return w;
}

int test(void)
{
	int bad;

	bad = 0;
	h = 100; hu = 800;
	if (divs(1, 2, 3) != 6) bad |= 1;
	if (g != 12 + 0 || gu != 50) bad |= 2;
	g = 3; h = 4; ts = 5; ds = 8;
	if (span(1000L) != 1000L + 5 + 4 + 3 + 4) bad |= 4;
	if (shifts(2, 6, 3L) != ((3L << 3) + (3L << 2) + 3) << 2) bad |= 8;
	return bad == 0;
}
