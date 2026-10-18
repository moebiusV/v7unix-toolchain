/* expect: 1 -- float and double arithmetic, conversions, comparisons (including NaN), returns */
double zero;
double dneg(x) double x; { return -x; }
float fneg1(float x) { return -x; }
double dmix(a, b, c) double a, b, c; { return (a * b - c) / (a + b); }
float fmix(float a, float b) { return a * b + a / b - a; }
int dlt(a, b) double a, b; { return a < b; }
int dle(a, b) double a, b; { return a <= b; }
int dgt(a, b) double a, b; { return a > b; }
int dge(a, b) double a, b; { return a >= b; }
int deq(a, b) double a, b; { return a == b; }
int dne(a, b) double a, b; { return a != b; }
int fcmpv(float a, float b) { if (a < b) return -1; if (a > b) return 1; return 0; }
double itod(i) int i; { return i; }
double ltod(l) long l; { return l; }
double utod(u) unsigned u; { return u; }
double ultod(u) unsigned long u; { return u; }
float itof(int i) { return i; }
int dtoi(d) double d; { return (int)d; }
long dtol(d) double d; { return (long)d; }
unsigned dtou(d) double d; { return (unsigned)d; }
unsigned long dtoul(d) double d; { return (unsigned long)d; }
int ftoi(float f) { return (int)f; }
double ftod(float f) { return f; }
float dtof(double d) { return (float)d; }
double sumarr(a, n) double *a; int n; { double s = 0; int i; for (i = 0; i < n; i++) s += a[i]; return s; }
float fsum(float *a, int n) { float s = 0; while (n--) s += *a++; return s; }
int truth(d) double d; { return d ? 1 : 0; }
int shortc(a, b) double a, b; { return a > 0 && b > 0; }
double cond(c, a, b) int c; double a, b; { return c ? a : b; }
double gd = 2.25;
float gf = 1.5f;
double garr[4] = { 1.5, 2.5, 3.5, 4.5 };

int test()
{
	int bad = 0;
	double arr[4], nan;
	float farr[3];
	unsigned long big;

	if (dneg(2.5) != -2.5 || fneg1(1.25f) != -1.25f) bad |= 1;
	if (dmix(3., 4., 2.) != 10. / 7. || fmix(6.f, 3.f) != 6.f * 3.f + 6.f / 3.f - 6.f) bad |= 2;
	nan = zero / zero;
	if (!dlt(1., 2.) || dlt(2., 1.) || dlt(nan, 1.) || dlt(1., nan)) bad |= 4;
	if (!dle(1., 1.) || dle(2., 1.) || dle(nan, nan)) bad |= 8;
	if (!dgt(2., 1.) || dgt(1., 2.) || dgt(nan, 1.)) bad |= 16;
	if (!dge(2., 2.) || dge(1., 2.) || dge(nan, nan)) bad |= 32;
	if (!deq(1., 1.) || deq(1., 2.) || deq(nan, nan)) bad |= 64;
	if (dne(1., 1.) || !dne(1., 2.) || !dne(nan, nan)) bad |= 128;
	if (fcmpv(1.f, 2.f) != -1 || fcmpv(2.f, 1.f) != 1 || fcmpv(1.f, 1.f) != 0) bad |= 256;
	if (itod(-7) != -7. || ltod(1L << 40) != 1099511627776. || utod(4000000000u) != 4000000000. ) bad |= 512;
	big = 1UL << 63;
	if (ultod(big) != 9223372036854775808. || itof(16777216) != 16777216.f) bad |= 1024;
	if (dtoi(-3.9) != -3 || dtoi(3.9) != 3 || dtol(-5e12) != -5000000000000L) bad |= 2048;
	if (dtou(4000000000.) != 4000000000u || dtoul(5e9) != 5000000000UL || ftoi(2.75f) != 2) bad |= 4096;
	if (ftod(0.5f) != 0.5 || dtof(0.25) != 0.25f) bad |= 8192;
	arr[0] = 1.; arr[1] = 2.; arr[2] = 3.; arr[3] = 4.;
	farr[0] = 1.f; farr[1] = 2.f; farr[2] = 4.f;
	if (sumarr(arr, 4) != 10. || fsum(farr, 3) != 7.f || sumarr(garr, 4) != 12.) bad |= 16384;
	if (!truth(0.5) || truth(0.) || !shortc(1., 1.) || shortc(1., 0.) || shortc(0., 1.)) bad |= 32768;
	if (cond(1, 1.5, 2.5) != 1.5 || cond(0, 1.5, 2.5) != 2.5 || gd * 2 != 4.5 || gf + gf != 3.f) bad |= 65536;
	return bad ? 2 * bad + 1000 : 1;
}
