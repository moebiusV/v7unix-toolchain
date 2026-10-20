/* expect: 1 -- loop shapes: while, for, do, break, continue, nested, empty body */
int a[16];
int sum(int n)
{
	int s = 0;

	while (--n >= 0)
		s += a[n];
	return s;
}

int count(int lim)
{
	int i, c = 0;

	for (i = 0; i < lim; i++) {
		if (i % 3 == 0)
			continue;
		if (i > 12)
			break;
		c += i;
	}
	return c;
}

int nest(int n)
{
	int i, j, t = 0;

	for (i = 0; i < n; i++)
		for (j = 0; j < i; j++)
			t += j;
	return t;
}

int dowhile(int n)
{
	int k = 0;

	do
		k += 2;
	while (--n > 0);
	return k;
}

int spin(int n)
{
	while (n-- > 0)
		;
	return n;
}

int never(int n)
{
	int c = 0;

	while (n < 0)
		c++;
	return c;
}

unsigned wrap(unsigned u)
{
	int c = 0;

	while ((int)(--u) < 0 && c < 5)
		c++;
	return c;
}

int test(void)
{
	int i, bad = 0;

	for (i = 0; i < 16; i++)
		a[i] = i;
	if (sum(16) != 120) bad |= 1;
	if (sum(0) != 0) bad |= 2;
	if (count(20) != 1+2+4+5+7+8+10+11) bad |= 4;
	if (nest(5) != 0+0+1+(0+1+2)+(0+1+2+3)) bad |= 8;
	if (dowhile(3) != 6) bad |= 16;
	if (spin(5) != -1) bad |= 32;
	if (never(3) != 0) bad |= 64;
	if (wrap(0x80000000u) != 0) bad |= 128;
	if (wrap(0) != 5) bad |= 256;
	return bad == 0;
}
