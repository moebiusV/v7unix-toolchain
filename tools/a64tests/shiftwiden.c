/* expect: 1 -- (long)(int)(u << n): the shift wraps at 32 bits before the conversion to long */
long a(unsigned x, int n) { return (long)(int)(x << n); }
long b(unsigned x) { return (long)(int)(x << 22); }
long c(int x) { return (long)(x << 4); }
unsigned long d(unsigned x) { return (unsigned long)(x << 22); }
int test()
{
	int bad = 0;

	if (a(4096, 22) != 0 || a(1, 31) != -2147483647L - 1) bad |= 1;
	if (b(4096) != 0 || b(1024) != 0 || b(512) != -2147483647L - 1) bad |= 2;
	if (c(5) != 80 || c(-5) != -80) bad |= 4;
	if (d(4096) != 0 || d(3) != 3UL << 22) bad |= 8;
	return bad ? 2 * bad + 1000 : 1;
}
