/* expect: 1 -- c ? a : b as a call argument, each arm moved into the argument slot */
int g1, g2;

int three(a, b, c) int a, b, c; { return a * 100 + b * 10 + c; }

int pick(c, v) int c, v;
{
	return three(c ? 1 : 2, v, c ? g1 : g2);
}

int nest(c, d) int c, d;
{
	return three(c ? (d ? 1 : 2) : 3, 4, c ? 5 : 6);
}

int test()
{
	int bad;

	bad = 0;
	g1 = 7; g2 = 8;
	if (pick(1, 9) != 197) bad |= 1;
	if (pick(0, 9) != 298) bad |= 2;
	if (nest(1, 1) != 145) bad |= 4;
	if (nest(1, 0) != 245) bad |= 8;
	if (nest(0, 1) != 346) bad |= 16;
	return bad ? bad : 1;
}
