/* expect: 0 -- signed % and / by constants, variables and memory (div takes any source) */
int g;
int a(int y) { return y % 4; }
int b(int y, int z) { return y % z; }
int c(int y) { return y / 12 + y % 12; }
int d(int y) { return y % g; }
int test(void)
{
	int bad;

	bad = 0;
	if (a(7) != 3 || a(-7) != -3 || a(8) != 0) bad |= 1;
	if (b(100, 7) != 2 || b(-100, 7) != -2 || b(100, -7) != 2) bad |= 2;
	if (c(100) != 8 + 4 || c(-100) != -8 + -4) bad |= 4;
	g = 9;
	if (d(100) != 1 || d(-100) != -1) bad |= 8;
	return bad;
}
