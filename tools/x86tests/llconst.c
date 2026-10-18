/* expect: 1 -- long long constants: negative, over 32 bits, passed and stored */
long long id(a) long long a; { return a; }
long long g1 = -100000LL, g2 = 10000000000LL, g3 = -10000000000LL;

int test()
{
	int bad;
	long long x;

	bad = 0;
	x = id(-100000LL);
	if ((int)(x >> 32) != -1 || (unsigned long)(x & 0xffffffffLL) != 0xfffe7960UL) bad |= 2;
	x = id(10000000000LL);
	if ((int)(x >> 32) != 2 || (unsigned long)(x & 0xffffffffLL) != 1410065408UL) bad |= 4;
	x = id(-10000000000LL);
	if ((int)(x >> 32) != -3 || (unsigned long)(x & 0xffffffffLL) != 2884901888UL) bad |= 8;
	if (g1 != -100000LL || g2 != 10000000000LL || g3 != -10000000000LL) bad |= 16;
	if (id(-1LL) + 1 != 0) bad |= 32;
	return bad ? 2 * bad + 1000 : 1;
}
