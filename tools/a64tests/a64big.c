/* expect: 1 -- frames and offsets too big for one instruction: large local arrays, far stack arguments, large struct copies */
struct huge { char b[300]; };
struct huge hg1, hg2;

long far(n) int n;
{
	char buf[20000];
	long q[3000];
	int i;
	long s;

	for (i = 0; i < 20000; i += 100) buf[i] = (char)(i / 100 + 1);
	for (i = 0; i < 3000; i++) q[i] = i * 3;
	s = 0;
	for (i = 0; i < 20000; i += 100) s += buf[i];
	return s + q[2999] + q[n] + buf[19900];
}

int withargs(a, b, c, d, e, f, g, h, i, j) int a, b, c, d, e, f, g, h, i, j;
{
	int pad[9000];

	pad[0] = a; pad[8999] = j; pad[4500] = i;
	return pad[0] + pad[8999] * 10 + pad[4500] * 100 + h * 1000 + g;
}

int copyhuge(void)
{
	struct huge l1, l2;
	int i, r = 0;

	for (i = 0; i < 300; i++) hg1.b[i] = (char)(i * 7);
	l1 = hg1;
	l2 = l1;
	hg2 = l2;
	for (i = 0; i < 300; i++) if (hg2.b[i] != (char)(i * 7) || l2.b[i] != hg1.b[i]) r++;
	return r;
}

int bigarr[100000];
char bigchars[70000];

int test()
{
	int bad = 0;

	if (far(5) != 10368L) bad |= 1;
	if (withargs(1, 2, 3, 4, 5, 6, 7, 8, 9, 10) != 1 + 100 + 900 + 8000 + 7) bad |= 2;
	if (copyhuge() != 0) bad |= 4;
	bigarr[99999] = 7; bigarr[50000] = 5;
	bigchars[69999] = 9; bigchars[65536] = 3;
	if (bigarr[99999] + bigarr[50000] != 12 || bigchars[69999] + bigchars[65536] != 12) bad |= 8;
	return bad ? 2 * bad + 1000 : 1;
}
