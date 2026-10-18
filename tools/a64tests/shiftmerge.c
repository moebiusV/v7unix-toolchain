/* expect: 1 -- two constant left shifts whose counts add up to the width or more are 0, not a shift by the sum modulo the width */
unsigned long a(unsigned long x) { return (x << 40) << 63; }
unsigned long b(unsigned long x) { return (x * 1099511627776UL) << 24; }
unsigned c(unsigned x) { return (x << 20) << 12; }
unsigned d(unsigned x) { return (x << 16) << 15; }
unsigned long e(unsigned long x) { return (x << 32) << 31; }
long f(long x) { return (x >> 40) >> 40; }
int test()
{
	int bad = 0;

	if (a(1) != 0 || a(0xffffffffUL) != 0) bad |= 1;
	if (b(1) != 0) bad |= 2;
	if (c(0xffffffffu) != 0) bad |= 4;
	if (d(1) != 0x80000000u || d(2) != 0) bad |= 8;
	if (e(1) != 0x8000000000000000UL || e(2) != 0) bad |= 16;
	if (f(-1) != -1 || f(1) != 0) bad |= 32;
	return bad ? 2 * bad + 1000 : 1;
}
