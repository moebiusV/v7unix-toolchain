/* expect: 1 -- a test of a register after a join where each way in set the codes from it */
int src;

int get(v) int v; { return v; }

int viabic(a, sel) int a, sel;
{
	register c;

	c = sel ? (a & 0377) : get(a);
	if (c < 0) return 1;
	if (c == 0) return 2;
	return 3;
}

int viabis(a, sel) int a, sel;
{
	register c;

	c = sel ? (a | 0100000) : get(a);
	if (c < 0) return 1;
	if (c == 0) return 2;
	return 3;
}

int viacom(a, sel) int a, sel;
{
	register c;

	c = sel ? ~a : get(a);
	if (c >= 0) return 1;
	return 0;
}

int viasub(a, sel) int a, sel;
{
	register c;

	c = sel ? a - 32767 - 2 : get(a);
	if (c < 0) return 1;
	return 0;
}

int test()
{
	int bad;

	bad = 0;
	if (viabic(-1, 1) != 3 || viabic(-1, 0) != 1 || viabic(0, 1) != 2 || viabic(0, 0) != 2) bad |= 1;
	if (viabic(256, 1) != 2 || viabic(255, 1) != 3) bad |= 2;
	if (viabis(0, 1) != 1 || viabis(0, 0) != 2 || viabis(5, 0) != 3) bad |= 4;
	if (viacom(-1, 1) != 1 || viacom(0, 1) != 0 || viacom(-1, 0) != 0 || viacom(7, 0) != 1) bad |= 8;
	if (viasub(0, 1) != 0 || viasub(-32767, 1) != 0 || viasub(5, 1) != 1) bad |= 16;
	return bad ? bad : 1;
}
