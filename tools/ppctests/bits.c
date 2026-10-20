/* expect: 1 -- bit tests of bytes, shorts and words, in memory and in registers, both polarities */
char c; short s; int flags; unsigned char uc; unsigned short us;

int t1(void)
{
	int r;

	r = 0;
	if (c & 4) r |= 1;
	if (!(c & 4)) r |= 2;
	if (c & 0x80) r |= 4;
	if (s & 0x100) r |= 8;
	if (s & 0x8000) r |= 16;
	if (uc & 0x80) r |= 32;
	if (us & 0x8000) r |= 64;
	if (flags & 0x10000) r |= 128;
	if ((flags & 6) == 0) r |= 256;
	if ((flags & 6) != 0) r |= 512;
	return r;
}

int t2(register char *p, register int x)
{
	int r;

	r = 0;
	if (*p & 1) r |= 1;
	if ((x & 8) == 0) r |= 2;
	if (x == 0) r |= 4;
	if (x != 0) r |= 8;
	return r;
}

int test(void)
{
	int bad;
	char b;

	bad = 0;
	c = 4; s = 0x100; flags = 6; uc = 0x80; us = 0;
	if (t1() != (1 | 8 | 32 | 512)) bad |= 1;
	c = (char)0x80; s = (short)0x8000; flags = 0x10000; uc = 0; us = 0x8000;
	if (t1() != (2 | 4 | 16 | 64 | 128 | 256)) bad |= 2;
	c = 0; s = 0; flags = 0; uc = 0; us = 0;
	if (t1() != (2 | 256)) bad |= 4;
	b = 1;
	if (t2(&b, 8) != (1 | 8)) bad |= 8;
	b = 2;
	if (t2(&b, 0) != (2 | 4)) bad |= 16;
	return bad == 0;
}
