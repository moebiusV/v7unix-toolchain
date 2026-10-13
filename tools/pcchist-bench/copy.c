/* string copy, length and compare loops */
char a[64], b[64];
int test(void)
{
	register char *p, *q;
	int i, n;

	for (i = 0; i < 40; i++) a[i] = 'a' + (i & 7);
	a[40] = 0;
	n = 0;
	for (i = 0; i < 6; i++) {
		p = b; q = a;
		while (*p++ = *q++)
			;
		p = b;
		while (*p++)
			n++;
	}
	return n;
}
