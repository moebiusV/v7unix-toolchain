/* expect: 1 -- the address of a local as a call argument, first, middle and last, with others around it */
int rd(p) int *p; { return *p; }
int wr(p, v) int *p; int v; { *p = v; return v; }
int three(a, p, b) int a; int *p; int b; { return a * 100 + *p * 10 + b; }
int two(p, q) int *p, *q; { return *p + *q; }
int arr(a) int *a; { return a[0] + a[1] + a[2]; }

int test(void)
{
	int bad = 0;
	int x, y;
	int v[3];

	x = 7; y = 4;
	if (rd(&x) != 7) bad |= 1;
	wr(&y, 9);
	if (y != 9) bad |= 2;
	if (three(1, &x, 2) != 172) bad |= 4;
	if (two(&x, &y) != 16) bad |= 8;
	v[0] = 1; v[1] = 2; v[2] = 3;
	if (arr(v) != 6 || arr(&v[0]) != 6) bad |= 16;
	return bad == 0;
}
