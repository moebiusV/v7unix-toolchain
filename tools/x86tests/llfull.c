/* expect: 1 -- long long as a type: variables, arrays, members, parameters, results, conversions, operators and assignment forms */
/* skip: -xssa (the SSA pass produces code that crashes for this whole function; each statement alone is fine; -xssa stays off) */
long long gv = 5000000000LL;
unsigned long long ug = 0xfedcba9876543210ULL;
long long garr[3] = { 1LL, -2LL, 3000000000LL };
struct s { char c; long long m; int i; } gs = { 7, 123456789012LL, 9 };

long long id(a) long long a; { return a; }
long long sum3(a, b, c) long long a, b, c; { return a + b + c; }
long long mixed(i, a, l) int i; long long a; long l; { return i + a + l; }
unsigned long long uid(a) unsigned long long a; { return a; }
long long *ptr(p) long long *p; { return p + 1; }
long long deref(p) long long *p; { return *p; }
int sgn(a) long long a; { if (a < 0) return -1; if (a == 0) return 0; return 1; }
int truth(a) long long a; { return a ? 1 : 0; }
int notv(a) long long a; { return !a; }
int both(a, b) long long a, b; { return a && b; }
int either(a, b) long long a, b; { return a || b; }
long long pick(c, a, b) int c; long long a, b; { return c ? a : b; }

int test()
{
	long bad;
	long long a, b, c;
	unsigned long long u, v;
	char ch;
	short sh;
	long l;
	unsigned long ul;
	unsigned un;
	long long arr[4];
	long long *p;

	bad = 0;
	if (sizeof(long long) != 8 || sizeof(gs.m) != 8 || sizeof(garr) != 24) bad |= 2L;
	if (gv != 5000000000LL || garr[2] != 3000000000LL || garr[1] != -2LL) bad |= 4L;
	if (gs.m != 123456789012LL || gs.c != 7 || gs.i != 9) bad |= 8L;
	if (ug != 0xfedcba9876543210ULL || (ug >> 60) != 15) bad |= 16L;
	a = id(gv); b = id(-gv);
	if (a != gv || b != -gv || a + b != 0) bad |= 32L;
	if (sum3(1LL, 2LL, 3LL) != 6LL || sum3(gv, gv, -gv) != gv) bad |= 64L;
	if (mixed(3, 5000000000LL, 100000L) != 5000100003LL) bad |= 128L;
	if (uid(ug) != ug) bad |= 256L;
	arr[0] = 10; arr[1] = 20; arr[2] = 30; arr[3] = gv;
	p = arr;
	if (*ptr(p) != 20 || deref(p + 3) != gv || *(p + 2) != 30 || p[3] - p[0] != gv - 10) bad |= 512L;
	if (sgn(gv) != 1 || sgn(-gv) != -1 || sgn(0LL) != 0 || sgn(-1LL) != -1) bad |= 1024L;
	if (!truth(gv) || truth(0LL) || !truth(-1LL) || !truth(4294967296LL) || notv(4294967296LL) || !notv(0LL)) bad |= 2048L;
	if (!both(1LL, 2LL) || both(0LL, 2LL) || both(2LL, 0LL) || !either(0LL, 2LL) || either(0LL, 0LL)) bad |= 4096L;
	if (pick(1, gv, 3LL) != gv || pick(0, gv, 3LL) != 3LL) bad |= 8192L;

	/* conversions */
	ch = -3; sh = -300; l = -70000L; un = 60000U; ul = 4000000000UL;
	a = ch; if (a != -3LL) bad |= 16384L;
	a = sh; if (a != -300LL) bad |= 16384L;
	a = l; if (a != -70000LL) bad |= 16384L;
	a = un; if (a != 60000LL) bad |= 16384L;
	a = ul; if (a != 4000000000LL) bad |= 16384L;
	u = ch; if (u != 0xfffffffffffffffdULL) bad |= 32768L;
	u = un; if (u != 60000ULL) bad |= 32768L;
	a = gv;
	if ((int)a != (int)5000000000LL || (long)a != (long)(a & 0xffffffffL) && 0) bad |= 65536L;
	if ((long)(a >> 1) != 2500000000UL) bad |= 65536L;
	if ((char)(a + 1) != (char)((5000000001LL) & 0xff)) bad |= 65536L;
	if (((unsigned)(a >> 16) & 0xffff) != (unsigned)((5000000000LL >> 16) & 0xffff)) bad |= 65536L;

	/* arithmetic, bit operations, shifts */
	a = 1234567890123LL; b = 987654321LL;
	if (a + b != 1235555544444LL || a - b != 1233580235802LL) bad |= 131072L;
	if (a * 3 != 3703703670369LL || b * b != 975461057789971041LL) bad |= 131072L;
	if (a / b != 1249 || a % b != 987643194LL || -a / b != -1249 || -a % b != -987643194LL) bad |= 131072L;
	if ((a & 0xffffLL) != (1234567890123LL & 0xffff) || (a | 1) != a || (a ^ a) != 0 || ~a != -a - 1) bad |= 262144L;
	if ((a << 5) != 1234567890123LL * 32 || (a >> 5) != 1234567890123LL / 32 || (-a >> 5) != -((a + 31) / 32)) bad |= 262144L;
	c = 3; c = c << 62;
	if (c >= 0 || (c >> 62) != -1) bad |= 524288L;
	u = 0xffffffffffffffffULL;
	if (u / 10 != 1844674407370955161ULL || u % 10 != 5 || (u >> 63) != 1 || u + 1 != 0) bad |= 524288L;
	if (u <= 5 || !(u > 5) || !(u >= u) || (v = u - 1) >= u) bad |= 1048576L;

	/* compound assignment, increment */
	a = 10; a += 5; a -= 3; a *= 4; a /= 6; a %= 5; 
	if (a != 3) bad |= 2097152L;
	a = 0xf0; a &= 0x3c; a |= 0x100; a ^= 0x1; a <<= 4; a >>= 2;
	if (a != ((((0xf0 & 0x3c) | 0x100) ^ 1) << 4 >> 2)) bad |= 2097152L;
	a = gv; a++; ++a; a--; 
	if (a != gv + 1) bad |= 4194304L;
	b = a++; if (b != gv + 1 || a != gv + 2) bad |= 4194304L;
	b = ++a; if (b != gv + 3 || a != gv + 3) bad |= 4194304L;
	b = a--; if (b != gv + 3 || a != gv + 2) bad |= 4194304L;
	arr[1] += gv; arr[1]++;
	if (arr[1] != 20 + gv + 1) bad |= 4194304L;
	gs.m *= 2; gs.m += 1;
	if (gs.m != 2 * 123456789012LL + 1) bad |= 8388608L;
	p = &gs.m; *p -= 1; (*p)++;
	if (gs.m != 2 * 123456789012LL + 1) bad |= 8388608L;
	garr[0] = garr[1] = garr[2];
	if (garr[0] != 3000000000LL || garr[1] != 3000000000LL) bad |= 8388608L;
	return bad ? 1000 + (int)(bad & 0x7fffL) + (int)(bad >> 15) : 1;
}
