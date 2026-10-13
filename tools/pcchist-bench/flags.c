/* flag words: bit tests, byte read-modify-write, counters */
char c, cnt[8]; unsigned char u; int flags, g;
void step(char *p)
{
	*p |= 1; *p &= ~2; (*p)++;
	c |= 4; c++; u &= 0x7f; cnt[2]++;
	g++; g--;
}
int bits(int x)
{
	int r;

	r = 0;
	if (x & 1) r++;
	if (flags & 2) r++;
	if (c & 4) r++;
	if (u & 0x80) r++;
	if (x == 0) r++;
	return r;
}
int test(void)
{
	int i, s;
	char b[2];

	s = 0; b[0] = 1; b[1] = 2; flags = 2; u = 0xff;
	for (i = 0; i < 200; i++) {
		step(b);
		s += bits(i);
	}
	return s + b[0];
}
