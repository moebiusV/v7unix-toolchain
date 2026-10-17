/* expect: 1 -- sizeof is unsigned: n * sizeof x widens to long without a sign */
long widen(u) unsigned u; { return (long)u; }
long mulw(me) int me; { return (long)(me * sizeof(long[20])); }

int test()
{
	unsigned x;
	int m, bad;

	bad = 0;
	x = 40000;
	m = 500;
	if (widen(x) != 40000L) bad |= 1;
	if (mulw(m) != 40000L) bad |= 2;
	if (sizeof(int) - 3 < 0) bad |= 4;		/* unsigned: 2 - 3 is large */
	if ((long)(m * sizeof(long[20])) < 0) bad |= 8;
	return bad ? 2 * bad + 1000 : 1;
}
