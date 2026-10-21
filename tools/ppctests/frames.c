/* expect: 1 -- frames: none for a leaf, calls only, locals, a huge local array, saved registers, ten arguments */
int leaf(a, b) int a, b; { return a * 3 + b; }
int callsonly(a) int a; { return leaf(a, 1) + leaf(a, 2); }
int locals(a) int a; { int v[5], i, s; for (i = 0; i < 5; i++) v[i] = a + i; s = 0; for (i = 0; i < 5; i++) s += v[i]; return s; }
int huge(n) int n; { char big[40000]; int i; big[0] = 1; big[39999] = n; for (i = 1; i < 39999; i++) big[i] = 0; return big[0] + big[39999] + leaf(n, 2); }
int manyargs(a, b, c, d, e, f, g, h, i, j) int a, b, c, d, e, f, g, h, i, j; { return a + b * 2 + c * 3 + d * 4 + e * 5 + f * 6 + g * 7 + h * 8 + i * 9 + j * 10; }
int callmany(x) int x; { return manyargs(x, 1, 2, 3, 4, 5, 6, 7, 8, 9) + callsonly(x); }
int keep(a, b, c) int a, b, c; { int x, y; x = leaf(a, b); y = leaf(b, c); return x * 100 + y + a + b + c; }
int rec(n) int n; { return n <= 1 ? 1 : n * rec(n - 1); }

int test()
{
	int bad = 0;

	if (leaf(2, 3) != 9) bad |= 1;
	if (callsonly(4) != 12 + 1 + 12 + 2) bad |= 2;
	if (locals(10) != 60) bad |= 4;
	if (huge(5) != 1 + 5 + 17) bad |= 8;
	if (callmany(2) != 2 + 2 + 6 + 12 + 20 + 30 + 42 + 56 + 72 + 90 + 7 + 8) bad |= 32;
	if (keep(1, 2, 3) != 5 * 100 + 9 + 6) bad |= 64;
	if (rec(6) != 720) bad |= 128;
	return bad ? 1000 + bad : 1;
}
