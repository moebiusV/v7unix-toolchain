/* expect: 1 -- constants of every shape, immediates at their limits, shifts, division, narrow-type arithmetic */
long k1(void) { return 0x123456789abcdef0L; }
long k2(void) { return -1L; }
unsigned long k3(void) { return 0xffffffff00000000UL; }
unsigned long k4(void) { return 0x00ff00ff00ff00ffUL; }
unsigned k5(void) { return 0xffff0000u; }
long k6(void) { return 0x100000000L; }
long k7(void) { return -0x100000000L; }
long k8(void) { return 0xffffL << 16; }
long k9(void) { return 0x7fffffffffffffffL; }
int k10(void) { return -2147483647 - 1; }
long addk(long x, int w) { return x + 4095 + (long)w; }
long addk2(long x) { return x + 4096; }
long addk3(long x) { return x + 0xfff000; }
long addk4(long x) { return x + 0x1001000; }
long subk(long x) { return x - 4095; }
long subk2(long x) { return x - 70000; }
int addn(int x) { return x + -4095; }
int cmpk(long x) { return x == 4095 ? 1 : x == -4095 ? 2 : x > 70000 ? 3 : 0; }
unsigned long andk(unsigned long x) { return x & 0xff00ff00ff00ff00UL; }
unsigned andk2(unsigned x) { return x & 0xffff; }
unsigned andk3(unsigned x) { return x & 0x12345; }
long ork(long x) { return x | 0x8000000000000001L; }
int xork(int x) { return x ^ 0x55555555; }
int sh1(int x, int n) { return x << n; }
int sh2(int x, int n) { return x >> n; }
unsigned sh3(unsigned x, int n) { return x >> n; }
long sh4(long x, int n) { return x << n; }
long sh5(long x, int n) { return x >> n; }
unsigned long sh6(unsigned long x, int n) { return x >> n; }
int shc(int x) { return (x << 5) + (x >> 3) + ((unsigned)x >> 28); }
long shlc(long x) { return (x << 40) + (x >> 7); }
int dv1(int a, int b) { return a / b; }
int dv2(int a, int b) { return a % b; }
unsigned dv3(unsigned a, unsigned b) { return a / b; }
unsigned dv4(unsigned a, unsigned b) { return a % b; }
long dv5(long a, long b) { return a / b; }
long dv6(long a, long b) { return a % b; }
unsigned long dv7(unsigned long a, unsigned long b) { return a / b; }
unsigned long dv8(unsigned long a, unsigned long b) { return a % b; }
int dv9(int a) { return a / 7 + a % 7; }
char c1(char a, char b) { return a + b; }
unsigned char c2(unsigned char a, unsigned char b) { return a * b; }
short sg1(short a, short b) { return a * b; }
unsigned short sg2(unsigned short a, unsigned short b) { return a - b; }
int c3(char a) { return a < 0; }
int c4(unsigned char a) { return a > 200; }
int c5(char a, char b) { return a < b; }
int c6(unsigned char a, unsigned char b) { return a < b; }
int c7(short a) { return (a >> 2) + (a << 3); }
int c8(unsigned short a) { return a >> 3; }
long wd1(int i) { return i; }
long wd2(unsigned i) { return i; }
long wd3(char c) { return c; }
long wd4(unsigned char c) { return c; }
long wd5(short s) { return s; }
long wd6(unsigned short s) { return s; }
int n1(int x) { return -x; }
int n2(int x) { return ~x; }
long n3(long x) { return -x; }
long n4(long x) { return ~x; }
int u1(unsigned a, unsigned b) { return a < b; }
int u2(unsigned long a, unsigned long b) { return a >= b; }
int u3(long a, long b) { return a <= b; }
int u4(int a, int b) { return a > b; }

int test()
{
	int bad = 0;

	if (k1() != 0x123456789abcdef0L || k2() != -1 || k3() != 0xffffffff00000000UL) bad |= 1;
	if (k4() != 0x00ff00ff00ff00ffUL || k5() != 0xffff0000u || k6() != 4294967296L) bad |= 2;
	if (k7() != -4294967296L || k8() != 0xffff0000L || k9() != 9223372036854775807L || k10() >= 0) bad |= 4;
	if (addk(1, 2) != 4098 || addk2(1) != 4097 || addk3(1) != 0xfff001 || addk4(1) != 0x1001001) bad |= 8;
	if (subk(5000) != 905 || subk2(70001) != 1 || addn(5000) != 905) bad |= 16;
	if (cmpk(4095) != 1 || cmpk(-4095) != 2 || cmpk(70001) != 3 || cmpk(5) != 0) bad |= 32;
	if (andk(0x1234567812345678UL) != 0x1200560012005600UL || andk2(0x12345678u) != 0x5678u || andk3(0xffffffffu) != 0x12345) bad |= 64;
	if (ork(2) != (long)0x8000000000000003UL || xork(0xff) != 0x555555aa) bad |= 128;
	if (sh1(3, 4) != 48 || sh2(-64, 3) != -8 || sh3(0x80000000u, 31) != 1) bad |= 256;
	if (sh4(1, 40) != 1099511627776L || sh5(-1024, 3) != -128 || sh6(0x8000000000000000UL, 63) != 1) bad |= 512;
	if (shc(0x12345678) != (0x12345678 << 5) + (0x12345678 >> 3) + 1) bad |= 1024;
	if (shlc(3) != (3L << 40) + 0) bad |= 2048;
	if (dv1(-7, 2) != -3 || dv2(-7, 2) != -1 || dv3(4000000000u, 3) != 1333333333u || dv4(4000000000u, 7) != 4000000000u % 7) bad |= 4096;
	if (dv5(-1000000000000L, 7) != -142857142857L || dv6(-1000000000000L, 7) != -1 || dv7(18000000000000000000UL, 10) != 1800000000000000000UL || dv8(18000000000000000007UL, 10) != 7) bad |= 8192;
	if (dv9(100) != 14 + 2 || dv9(-100) != -14 - 2) bad |= 16384;
	if (c1(100, 100) != -56 || c2(20, 20) != 144 || sg1(300, 300) != 24464 || sg2(1, 2) != 65535) bad |= 32768;
	if (!c3(-1) || c3(1) || !c4(201) || c4(200) || !c5(-3, 2) || c5(2, -3) || !c6(2, 200) || c6(200, 2)) bad |= 65536;
	if (c7(-100) != -25 + -800 || c8(65535) != 8191) bad |= 131072;
	if (wd1(-5) != -5 || wd2(4000000000u) != 4000000000L || wd3(-3) != -3 || wd4(250) != 250 || wd5(-30000) != -30000 || wd6(60000) != 60000) bad |= 262144;
	if (n1(5) != -5 || n2(5) != -6 || n3(1L << 40) != -(1L << 40) || n4(0) != -1) bad |= 524288;
	if (!u1(1, 4000000000u) || u1(4000000000u, 1) || !u2(~0UL, 1) || u2(1, ~0UL) || !u3(-5, 3) || u3(3, -5) || !u4(5, -5) || u4(-5, 5)) bad |= 1048576;
	return bad ? 2 * bad + 1000 : 1;
}
