/* expect: 1 -- 64-bit constants and addresses in every size class */
long g;
long a[4];
long kinds(long which)
{
	switch (which) {
	case 0: return 0;
	case 1: return 70000;
	case 2: return -5;
	case 3: return 0xffffffffL;
	case 4: return 0x80000000L;
	case 5: return -0x80000000L;
	case 6: return -0x80000001L;
	case 7: return 0x123456789abcdefL;
	}
	return 99;
}

int test(void)
{
	long *p = &g;
	long bad = 0;

	if (kinds(0) != 0) bad |= 1;
	if (kinds(1) != 70000) bad |= 2;
	if (kinds(2) != -5) bad |= 4;
	if (kinds(3) != 4294967295L) bad |= 8;
	if (kinds(4) != 2147483648L) bad |= 16;
	if (kinds(5) != -2147483648L) bad |= 32;
	if (kinds(6) != -2147483649L) bad |= 64;
	if (kinds(7) != 81985529216486895L) bad |= 128;
	*p = 12345678901L;
	if (g != 12345678901L) bad |= 256;
	a[2] = 7; p = &a[2];
	if (*p != 7 || p != a + 2) bad |= 512;
	return bad == 0;
}
