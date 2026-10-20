/* expect: 1 -- elements of local arrays of every size, indexed by a variable and by an expression */
int fill(c, s, n, d, q) char *c; short *s; int *n; double *d; long long *q;
{
	int i;

	for (i = 0; i < 4; i++) {
		c[i] = i + 1; s[i] = (i + 1) * 100; n[i] = (i + 1) * 1000;
		d[i] = i + 0.5; q[i] = (long)(i + 1) * 10000L;
	}
	return 0;
}

int pick(i, j) int i, j;
{
	char c[4]; short s[4]; int n[4]; double d[4]; long long q[4];
	int r;

	fill(c, s, n, d, q);
	r = 0;
	if (c[i] != i + 1) r |= 1;
	if (s[i] != (i + 1) * 100) r |= 2;
	if (n[i + j] != (i + j + 1) * 1000) r |= 4;
	if (d[j] != j + 0.5) r |= 8;
	if (q[i] != (long)(i + 1) * 10000L) r |= 16;
	n[j] = n[i] + s[j];
	if (n[j] != (i + 1) * 1000 + (j + 1) * 100 && i != j) r |= 32;
	c[j] += 1;
	s[i] -= 1;
	if (c[j] != j + 2 && i != j) r |= 64;
	return r;
}

long long gq[4];

/* a long long element is two longs to the compiler; an index must not break the upper half */
int quads(i) int i;
{
	long long lq[4];
	int r;

	r = 0;
	gq[i] = 70000L; lq[i] = gq[i] + 70000L;
	if (lq[i] != 140000L) r |= 1;
	gq[i] = -5; lq[i] = gq[i];
	if (lq[i] != -5 || gq[i] >= 0) r |= 2;
	lq[i] = gq[i] & 3; gq[i] |= 8;
	if (lq[i] != 3 || gq[i] != -5L) r |= 4;
	return r;
}

int test()
{
	int bad;

	bad = 0;
	if (pick(0, 0) != 0) bad |= 1;
	if (pick(1, 2) != 0) bad |= 2;
	if (pick(3, 0) != 0) bad |= 4;
	if (pick(2, 1) != 0) bad |= 8;
	if (quads(0) != 0 || quads(2) != 0 || quads(3) != 0) bad |= 16;
	return bad ? 2 * bad + 1000 : 1;
}
