/* expect: 1 -- *p++ and *--p walks, in every context */
char cbuf[16];
int  ibuf[8];
char *gp;

int mylen(register char *s)
{
	register char *e;

	e = s;
	while (*e++)
		;
	return e - s - 1;
}

char *mycpy(register char *d, register char *s)
{
	char *r;

	r = d;
	while (*d++ = *s++)
		;
	return r;
}

int isum(register int *p, register int n)
{
	register int t;

	t = 0;
	while (n-- > 0)
		t += *p++;
	return t;
}

void ifill(register int *p, int n)
{
	register int i;

	for (i = 0; i < n; i++)
		*p++ = i * 3;
}

/* walk backwards with pre-decrement */
int rev(register char *e, char *b)
{
	register int h;

	h = 0;
	while (e > b)
		h = h * 5 + *--e;
	return h;
}

/* the walked pointer is used again afterwards, and value contexts */
int ctx(register char *s, register int *q)
{
	register char c;
	int bad, v;
	char *start;

	bad = 0;
	start = s;
	c = *s++;				/* load, then step */
	if (c != 'a' || s != start + 1) bad |= 1;
	if (*s++ != 'b') bad |= 2;
	if (*s++ != 'x') bad |= 4;		/* compare of the loaded value */
	if (s != start + 3) bad |= 8;
	v = *q++ + *q++;			/* two steps in one expression */
	if (v != 3 || q != ibuf + 2) bad |= 16;
	*q++ = 40; *q++ = 50;
	if (ibuf[2] != 40 || ibuf[3] != 50 || q != ibuf + 4) bad |= 32;
	return bad;
}

/* more live pointers than registers: some must live in memory */
int many(a, b, c, d, e)
char *a, *b, *c, *d, *e;
{
	int n;

	n = 0;
	while (*a++ && *b++ && *c++ && *d++ && *e++)
		n++;
	return n * 100 + (a - cbuf) + (e - cbuf);
}

int test(void)
{
	int bad, i;

	bad = 0;
	mycpy(cbuf, "hello");
	if (mylen(cbuf) != 5) bad |= 1;
	if (cbuf[4] != 'o' || cbuf[5] != 0) bad |= 2;
	for (i = 0; i < 8; i++) ibuf[i] = 0;
	ifill(ibuf, 5);
	if (ibuf[4] != 12 || ibuf[5] != 0) bad |= 4;
	ibuf[0] = 1; ibuf[1] = 2;
	if (isum(ibuf, 5) != 1 + 2 + 6 + 9 + 12) bad |= 8;
	if (rev("abc" + 3, "abc") != ('c' * 25 + 'b' * 5 + 'a')) bad |= 16;
	mycpy(cbuf, "abx");
	ibuf[0] = 1; ibuf[1] = 2;
	if (ctx(cbuf, ibuf) != 0) bad |= 32;
	mycpy(cbuf, "abcdef");
	if (many(cbuf, cbuf, cbuf, cbuf, cbuf + 2) != 4 * 100 + 5 + 7) bad |= 64;
	return bad == 0;
}
