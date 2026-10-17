/* expect: 1 -- pointers spilled to the frame (r2-r4 taken by register variables) stepped as they are used */
int g(a, b, c) int a, b, c; { return a + b + c; }

int wcopy(from, to, n) int *from, *to; int n;
{
	register int i, j, k;
	int *s, *t;

	i = n; j = 1; k = 2;
	s = from; t = to;
	g(i, j, k);
	while (i--) *t++ = *s++;
	return j + k;
}

int bcopy(from, to) char *from, *to;
{
	register int i, j, k;
	char *s, *t;

	i = 0; j = 1; k = 2;
	s = from; t = to;
	g(i, j, k);
	while (*t++ = *s++) i++;
	return i + j + k;
}

int sums(cp, ip) char *cp; int *ip;
{
	register int i, j, k;
	char *c;
	int *w, tot;

	i = 0; j = 1; k = 2;
	c = cp; w = ip;
	g(i, j, k);
	tot = 0;
	tot += *c++; tot += *c++;		/* 1 + 2 */
	tot += *w++;				/* 100 */
	*c++ = 99;				/* cp[2] */
	*w++ = 7;				/* ip[1] */
	return tot + j + k + (int)(c - cp) + (int)(w - ip) + i;
}

int test()
{
	int bad, i, w1[4], w2[4];
	char b1[8], b2[8];

	bad = 0;
	for (i = 0; i < 4; i++) { w1[i] = i + 10; w2[i] = 0; }
	if (wcopy(w1, w2, 4) != 3) bad |= 1;
	for (i = 0; i < 4; i++) if (w2[i] != i + 10) bad |= 2;
	b1[0] = 'x'; b1[1] = 'y'; b1[2] = 0;
	if (bcopy(b1, b2) != 2 + 3) bad |= 4;
	if (b2[0] != 'x' || b2[1] != 'y' || b2[2] != 0) bad |= 8;
	for (i = 0; i < 8; i++) b1[i] = i + 1;
	w1[0] = 100; w1[1] = 0;
	if (sums(b1, w1) != 1 + 2 + 100 + 1 + 2 + 3 + 2) bad |= 16;
	if (b1[2] != 99 || w1[1] != 7) bad |= 32;
	return bad ? bad : 1;
}
