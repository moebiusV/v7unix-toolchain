/* expect: 1 -- (int)(u << 0) % n: the shift by zero is removed but its retyped result keeps its type */
int m(short p, int d) { return (int)((unsigned)(int)p << 0) % (d + 1); }
int r(short p, int d) { return (int)((unsigned)(int)p >> 0) / (d + 1); }
long l(unsigned short p, int d) { return (long)((unsigned long)p << 0) % (d + 3); }
int test()
{
	int bad = 0;

	if (m(5, 1) != 1 || m(-7, 2) != (int)((unsigned)-7 % 3 * 0 + (-7 % 3))) bad |= 1;
	if (r(20, 3) != 5 || r(-20, 3) != -5) bad |= 2;
	if (l(40000, 1) != 0 || l(40001, 1) != 1) bad |= 4;
	return bad ? 2 * bad + 1000 : 1;
}
