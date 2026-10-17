/* expect: 1 -- a variable assigned inside a condition: ++n > 1, (c += 3) < 10, (x = e) == 0, short-circuit */
int a, c, x, y, calls;
int f(v) int v; { calls++; return v; }

int test(void)
{
	int bad = 0, n, k, r;

	a = 0;
	if (++a > 1) bad |= 1;
	if (++a > 1) ; else bad |= 2;
	c = 5;
	if ((c += 3) < 10) ; else bad |= 4;
	if ((c += 3) < 10) bad |= 8;
	if (c != 11) bad |= 16;
	y = 7;
	if ((x = y) == 0) bad |= 32;
	if ((x = f(0)) == 0) ; else bad |= 64;
	calls = 0;
	if (a < 0 && (x = f(5)) > 1) bad |= 128;	/* not evaluated */
	if (calls != 0 || x != 0) bad |= 256;
	if (a > 0 && (x = f(5)) > 1) ; else bad |= 512;
	if (calls != 1 || x != 5) bad |= 1024;
	n = 10; k = 0;
	while ((n -= 3) > 0) k++;
	if (k != 3 || n != -2) bad |= 2048;
	r = 0;
	if ((r |= 4) == 4 && (r |= 1) == 5) ; else bad |= 4096;
	return bad == 0;
}
