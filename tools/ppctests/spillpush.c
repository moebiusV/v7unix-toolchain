/* expect: 1 -- a long (or word) that waits for another value: spilled, reloaded once, nested calls in between */
long lf(v) long v; { return v * 3L; }
int  wf(v) int v; { return v + 1; }
long gl;

long diff1(c, off) int c; long off;
{
	long p;

	p = off;
	p += c - lf(7L);			/* int widened, minus a call: the call result waits */
	return p;
}

long diff2(a, b) long a, b;
{
	return a - lf(b) - lf(a);		/* two calls: no push may be live across the second */
}

long diff3(c) int c;
{
	return (long)c - lf((long)wf(c)) + gl;
}

int word1(a, b) int a, b;
{
	return a - wf(b) - wf(a + b);
}

long mixed(c, d) int c, d;
{
	return (long)c * lf(2L) - (long)d - lf((long)(c + d));
}

int test()
{
	int bad;

	bad = 0;
	gl = 1000L;
	if (diff1(100, 5000L) != 5000L + 100 - 21L) bad |= 1;
	if (diff2(10L, 4L) != 10L - 12L - 30L) bad |= 2;
	if (diff3(5) != 5L - 18L + 1000L) bad |= 4;
	if (word1(50, 20) != 50 - 21 - 71) bad |= 8;
	if (mixed(3, 4) != 18L - 4L - 21L) bad |= 16;
	if (diff1(-100, -5L) != -5L - 100 - 21L) bad |= 32;
	return bad ? 2 * bad + 1000 : 1;
}
