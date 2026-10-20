/* expect: 1 -- c ? a : b used as a condition (branches from each arm), with side effects */
int n, m, trace;

int t(v) int v; { trace = trace * 10 + v; return v; }

int sel(c, a, b) int c, a, b;
{
	if (c ? a : b)
		return 1;
	return 0;
}

int loop(q, lim) int q, lim;
{
	int i;

	i = 0;
	while (q ? t(i) < lim : !t(i + 100) ? 0 : i < lim - 1)
		i++;
	return i;
}

int mixed(a, b, c, d) int a, b, c, d;
{
	int r;

	r = 0;
	if ((a ? b : c) && d) r |= 1;
	if ((a ? b : c) || d) r |= 2;
	if (!(a ? b : c)) r |= 4;
	if ((a ? (b ? c : d) : 0)) r |= 8;
	return r;
}

int test()
{
	int bad;

	bad = 0;
	if (sel(1, 1, 0) != 1 || sel(1, 0, 1) != 0 || sel(0, 1, 0) != 0 || sel(0, 0, 1) != 1) bad |= 1;
	if (loop(1, 4) != 4) bad |= 2;
	if (loop(0, 4) != 3) bad |= 4;
	if (mixed(1, 1, 0, 1) != (1 | 2)) bad |= 8;
	if (mixed(0, 1, 0, 0) != 4) bad |= 16;
	if (mixed(1, 0, 1, 1) != 14) bad |= 32;
	trace = 0; n = (1 ? t(3) : t(4)) ? 7 : 8;
	if (trace != 3 || n != 7) bad |= 64;
	trace = 0; m = (0 ? t(3) : t(0)) ? 7 : 8;
	if (trace != 0 || m != 8) bad |= 128;
	return bad ? 2 * bad + 1000 : 1;
}
