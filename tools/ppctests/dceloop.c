/* expect: 1 -- an assignment in a loop is live after it although the block after the loop has one statement that dead code removal deletes */
double sink;
long g;
long f(int c, long p2)
{
	double v1 = -596;
	int i;

	if (c > 2) {
		for (i = 0; i < 3; i++)
			;
	} else {
		for (i = 0; i < 2; i++)
			;
		for (i = 0; i < 1; i++)
			;
		for (i = 0; i < 1; i++)
			v1 = 8;
		p2 = p2 + 7;
	}
	p2 = c;
	sink = v1;
	return p2 * 10 + (long)v1;
}
int test()
{
	int bad = 0;

	if (f(1, 0) != 18) bad |= 1;
	if (f(9, 0) != 9 * 10 - 596) bad |= 2;
	if (sink != -596) bad |= 4;
	return bad ? 2 * bad + 1000 : 1;
}
