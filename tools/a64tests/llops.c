/* expect: 1 -- long long multiply, divide, remainder and shifts, signed and unsigned, with values over 32 bits */
long long mul(a, b) long long a, b; { return a * b; }
long long dv(a, b) long long a, b; { return a / b; }
long long md(a, b) long long a, b; { return a % b; }
unsigned long long udv(a, b) unsigned long long a, b; { return a / b; }
unsigned long long umd(a, b) unsigned long long a, b; { return a % b; }
unsigned long long ushr(a, n) unsigned long long a; int n; { return a >> n; }
long long sshr(a, n) long long a; int n; { return a >> n; }
long long shl(a, n) long long a; int n; { return a << n; }

int test()
{
	int bad;
	long long big;
	unsigned long long ub;

	bad = 0;
	big = 100000L; big = big * 100000L;		/* 10^10 */
	if (mul(100000LL, 100000LL) != big) bad |= 2;
	if (mul(-100000LL, 100000LL) != -big) bad |= 4;
	if (mul(-100000LL, -100000LL) != big) bad |= 8;
	if (mul(big, 3LL) != big + big + big) bad |= 16;
	if (dv(big, 100000LL) != 100000LL) bad |= 32;
	if (dv(-big, 100000LL) != -100000LL) bad |= 64;
	if (dv(-big, -7LL) != big / 7) bad |= 128;
	if (md(big + 5LL, 100000LL) != 5) bad |= 256;
	if (md(-(big + 5LL), 100000LL) != -5) bad |= 512;
	if (md(big + 5LL, -100000LL) != 5) bad |= 1024;
	ub = big; ub = ub * 1000000000LL;		/* 10^19: over 2^63 */
	if (udv(ub, 1000000000LL) != big) bad |= 2048;
	if (umd(ub + 7LL, 1000000000LL) != 7) bad |= 4096;
	if (udv(ub, ub) != 1 || udv(ub, ub + 1LL) != 0) bad |= 8192;
	if (umd(ub, ub - 1LL) != 1) bad |= 16384;
	if (ushr(ub, 0) != ub || ushr(ub, 63) != 1 || ushr(ub, 40) != ub / 1099511627776LL) bad |= 32768;
	if (sshr(-big, 3) != -(big / 8) - 1 && sshr(-big, 3) != -(big / 8)) bad |= 65536;
	if (shl(3LL, 40) != 3 * 1099511627776LL) bad |= 131072;
	return bad ? 2 * (bad & 0x3ffff) + 1000 : 1;
}
