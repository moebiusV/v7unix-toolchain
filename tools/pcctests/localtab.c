/* expect: 1 -- local arrays and structs initialized with constants (an image copied in), changed afterwards, in a loop, in a recursive function */
extern int use2(int);
struct pt { char tag; int v[6]; char *name; };
static char nm[] = "nm";

static int sumtab(void)
{
	const unsigned char t[40] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40 };
	int i, s = 0;
	for (i = 0; i < 40; i++) s += t[i];
	return s;
}

static int modtab(int k)
{
	int a[16] = { 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
	int i, s = 0;
	a[k & 15] += 100;
	for (i = 0; i < 16; i++) s += a[i];
	return s;
}

static int sparse(void)
{
	char c[64] = { 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 7 };
	int i, s = 0;
	for (i = 0; i < 64; i++) s += c[i];
	return s;
}

static int structs(int which)
{
	struct pt p[3] = { { 1, { 10, 20, 30, 40, 50, 60 }, nm }, { 2, { 11, 21, 31, 41, 51, 61 }, nm }, { 3, { 12, 22, 32, 42, 52, 62 }, nm } };
	return p[which].tag * 1000 + p[which].v[5] + (p[which].name == nm);
}

static int rec(int n)
{
	int w[12] = { 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144 };
	if (n <= 0) return w[11];
	return rec(n - 1) + w[n % 12];
}

static const char *strs(int i)
{
	const char *s[8] = { "zero", "one", "two", "three", "four", "five", "six", "seven" };
	return s[i & 7];
}

int test()
{
	int i, s = 0;

	if (sumtab() != 820) return 2;
	if (sumtab() != 820) return 3;
	if (modtab(3) != 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14 + 15 + 16 + 17 + 18 + 19 + 20 + 100) return 4;
	if (modtab(3) != 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14 + 15 + 16 + 17 + 18 + 19 + 20 + 100) return 5;
	if (sparse() != 9 * 32 + 7) return 6;
	if (structs(1) != 2000 + 61 + 1 || structs(2) != 3000 + 62 + 1) return 7;
	for (i = 0; i < 5; i++) s += rec(3) - rec(2);
	if (s != 5 * 3) return 8;
	if (strs(2)[0] != 't' || strs(10)[1] != 'w' || strs(7)[1] != 'e') return 9;
	return 1;
}
