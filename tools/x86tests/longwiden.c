/* expect: 1 -- long *, / and % with an operand widened from a word: pushed directly, sign or zero filled.
   (dmr-cc fails dl and md here only because it reads 100000L wrongly: dmrbugs/long-constant.c) */
int gi;
unsigned gu;
long gl;

long dl(a, n) long a; int n; { return a / n; }
long dr(n, a) int n; long a; { return n / a; }
long md(a, n) long a; int n; { return a % n; }
long mu(a, n) long a; int n; { return a * n; }
long udiv_(a, u) long a; unsigned u; { return a / u; }
long gdiv() { return gl / gi; }
long gmul() { return (long)gi * gl; }
long reg(a, n) long a; int n;
{
	register int i, j, k;
	long r;

	i = n; j = 3; k = 4;
	r = ((long)i << 12) / j + (long)j * k;
	return r + a % i;
}

int test()
{
	int bad;

	bad = 0;
	if (dl(100000L, 7) != 14285L) bad |= 2;
	if (dl(-100000L, -7) != 14285L) bad |= 4;
	if (dr(-9, 3L) != -3L) bad |= 8;
	if (md(100000L, 7) != 5L) bad |= 16;
	if (md(-100000L, 7) != -5L) bad |= 32;
	if (mu(70000L, -3) != -210000L) bad |= 64;
	gi = -4; gl = 100L;
	if (gdiv() != -25L) bad |= 128;
	if (gmul() != -400L) bad |= 256;
	if (reg(1000L, 6) != ((6L << 12) / 3) + 12L + 1000L % 6) bad |= 512;
	return bad ? 2 * bad + 1000 : 1;
}
