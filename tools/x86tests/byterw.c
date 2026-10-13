/* expect: 1 -- c |= k, c &= k, c++, c-- on bytes in memory (orb/andb/incb/decb), signed and unsigned */
char c, buf[4]; unsigned char u;

void f(char *p)
{
	*p |= 1;
	*p &= ~2;
	(*p)++;
	p[1]--;
	buf[2]++;
	u &= 0x7f;
	c |= 4;
	c++;
}

int test(void)
{
	int bad;
	char b[2];

	bad = 0;
	b[0] = 2; b[1] = 10; buf[2] = 5; u = 0xff; c = 8;
	f(b);
	if (b[0] != 2) bad |= 1;		/* 2|1=3, &~2=1, ++ = 2 */
	if (b[1] != 9) bad |= 2;
	if (buf[2] != 6) bad |= 4;
	if (u != 0x7f) bad |= 8;
	if (c != 13) bad |= 16;			/* 8|4=12, ++ = 13 */
	b[0] = 127; b[1] = -128;
	(*b)++; b[1]--;
	if (b[0] != -128) bad |= 32;		/* wraps */
	if (b[1] != 127) bad |= 64;
	return bad == 0;
}
