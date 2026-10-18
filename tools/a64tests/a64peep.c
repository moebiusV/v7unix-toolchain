/* expect: 1 -- the shapes the aarch64 text optimizer rewrites: pointer walks (post-index), compares with zero, bit tests, narrow values, indexed access, conditions as values */
char gs[40] = "hello, world";
short gh[10] = { 1, -2, 3, -4, 5, 6, 7, 8, 9, 10 };
int gi[10] = { 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 };
long gl[10] = { 1L << 33, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

int slen(char *s) { char *p = s; while (*p++) ; return (int)(p - s - 1); }
char *scpy(char *d, char *s) { char *r = d; while ((*d++ = *s++)) ; return r; }
int scmp(char *a, char *b) { while (*a == *b++) if (*a++ == 0) return 0; return a[0] - b[-1]; }
int sumh(short *p, int n) { int s = 0; while (n--) s += *p++; return s; }
long suml(long *p, int n) { long s = 0; while (n-- > 0) s += *--p + 0, p += 2; return s; }
long sumdown(long *p, int n) { long s = 0; p += n; while (n--) s += *--p; return s; }
int sumi(int *p, int n) { int i, s = 0; for (i = 0; i < n; i++) s += p[i]; return s; }
int sumidx(int *p, int *q, int n) { int i, s = 0; for (i = 0; i < n; i++) s += p[i] * q[n - 1 - i]; return s; }
void fill(char *p, int n, char c) { while (n--) *p++ = c; }
void incall(int *p, int n) { while (n--) (*p++)++; }
int sgn(int x) { if (x < 0) return -1; if (x > 0) return 1; return 0; }
int sgnl(long x) { if (x < 0) return -1; if (x == 0) return 0; return 1; }
int isz(int x) { return x == 0; }
int nz(long x) { return x != 0; }
int bit3(int x) { return (x & 8) != 0; }
int nobit3(unsigned x) { return (x & 8) == 0; }
int bit40(unsigned long x) { return (x & (1UL << 40)) != 0; }
int mask6(int x) { return (x & 6) == 0; }
int mask6b(int x) { return (x & 6) != 0; }
int ge(int a, int b) { return a >= b; }
int lt(int a, int b) { return a < b; }
int ult(unsigned a, unsigned b) { return a < b; }
long lge(long a, long b) { return a >= b ? 7 : 3; }
int chk(char c) { return c == 0; }
int chu(unsigned char c) { return c != 0; }
int ext(char *p) { int r = 0; if (*p) r += 1; if (p[1] >= 0) r += 2; return r; }
int arr2(int *a, int i, int j) { return a[i] + a[j] + a[i + 1]; }
long arrl(long *a, int i) { return a[i] - a[i - 1]; }
long arrs(short *a, unsigned i) { return a[i] + a[i + 1]; }
int cond1(int a, int b) { int r = 0; r = a < b; r += (a == b) * 2; r += (a > b) * 4; return r; }
int twocmp(int x) { if (x == 0) return 10; if (x < 0) return 20; return 30; }
int swit(int x) { switch (x) { case 1: return 5; case 2: return 6; case 4: return 8; } return 0; }

int test()
{
	int bad = 0, i;
	char buf[40];
	int a[6] = { 1, 2, 3, 4, 5, 6 };
	long l[4] = { 1, 2, 3, 4 };

	if (slen(gs) != 12 || slen("") != 0) bad |= 1;
	scpy(buf, gs);
	if (scmp(buf, gs) != 0 || scmp("abc", "abd") >= 0 || scmp("b", "a") <= 0) bad |= 2;
	if (sumh(gh, 10) != 43) bad |= 4;
	if (sumdown(gl, 4) != (1L << 33) + 2 + 3 + 4) bad |= 8;
	if (sumi(gi, 10) != 550 || sumidx(a, a, 6) != 1*6 + 2*5 + 3*4 + 4*3 + 5*2 + 6*1) bad |= 16;
	fill(buf, 5, 'x'); buf[5] = 0;
	if (slen(buf) != 5 || buf[4] != 'x') bad |= 32;
	incall(a, 6);
	if (a[0] != 2 || a[5] != 7) bad |= 64;
	if (sgn(-5) != -1 || sgn(5) != 1 || sgn(0) != 0 || sgnl(-(1L << 40)) != -1 || sgnl(0) != 0 || sgnl(1L << 40) != 1) bad |= 128;
	if (!isz(0) || isz(3) || !nz(1L << 33) || nz(0)) bad |= 256;
	if (!bit3(8) || bit3(7) || !bit3(-1) || nobit3(8) != 0 || !nobit3(7) || !bit40(1UL << 40) || bit40(1UL << 39)) bad |= 512;
	if (!mask6(1) || mask6(2) || mask6b(1) || !mask6b(4)) bad |= 1024;
	if (!ge(3, 3) || ge(2, 3) || !lt(-2, -1) || lt(1, 1) || !ult(1, 4000000000u) || ult(4000000000u, 1) || lge(1, 1) != 7 || lge(0, 1) != 3) bad |= 2048;
	if (!chk(0) || chk(1) || !chu(255) || chu(0) || ext("a") != 3 || ext("") != 2) bad |= 4096;
	if (arr2(gi, 1, 4) != 20 + 50 + 30 || arrl(gl, 2) != 3 - 2 || arrs(gh, 3) != -4 + 5) bad |= 8192;
	if (cond1(1, 2) != 1 || cond1(2, 2) != 2 || cond1(3, 2) != 4) bad |= 16384;
	if (twocmp(0) != 10 || twocmp(-3) != 20 || twocmp(9) != 30 || swit(1) != 5 || swit(4) != 8 || swit(3) != 0) bad |= 32768;
	if (suml(l, 0) != 0) bad |= 65536;
	return bad ? 2 * bad + 1000 : 1;
}
