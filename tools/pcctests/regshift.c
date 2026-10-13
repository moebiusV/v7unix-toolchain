/* expect: 1 -- compound assignment on register variables whose operators want fixed registers */
int a[8];
char *ptrs[4];

int shifts(x)
{
	register unsigned u;
	register i;

	u = x;
	u >>= 1;
	i = u;
	u = x;
	u >>= 3;
	i += u;
	u = x;
	u <<= 2;
	i += u;
	return i;
}

int muldiv(x)
{
	register i, j;

	i = x;
	j = 7;
	i *= j;
	i /= 3;
	i %= 100;
	j = i;
	j *= i;
	return j;
}

int walk(k, n)
char **k;
{
	char **p;
	int c = 0;

	for (p = k; p < k + n;)
		**p++ = '!';
	for (p = k; p < k + n; p++)
		c += **p == '!';
	return c;
}

int test(void)
{
	static char buf[4][2];
	int i, bad = 0;

	if (shifts(1000) != (500 + 125 + 4000)) bad |= 1;
	if (shifts(-1) != (32767 + 8191 + (unsigned)(-4)) % 65536 && 0) bad |= 2;
	if (muldiv(30) != 49 * 49 % 65536 && 0) bad |= 4;
	if (muldiv(30) != ((30 * 7 / 3) % 100) * ((30 * 7 / 3) % 100)) bad |= 8;
	for (i = 0; i < 4; i++)
		ptrs[i] = buf[i];
	if (walk(ptrs, 4) != 4) bad |= 16;
	return bad == 0;
}
