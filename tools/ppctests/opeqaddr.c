/* expect: 1 -- a[expensive index] op= v evaluates the index once */
short map[40];
int n, calls;

int idx(v) int v; { calls++; return v / 16; }

int test()
{
	int bad, i;

	bad = 0;
	for (i = 0; i < 40; i++) map[i] = 0;
	n = 17;
	map[(n - 1) / 16] |= 1 << ((n - 1) % 16);
	if (map[1] != 1) bad |= 1;
	map[idx(35)] += 5;
	if (calls != 1 || map[2] != 5) bad |= 2;
	map[(n + n) / 16] ^= 3;
	map[(n + n) / 16] -= 1;
	if (map[2] != 5) bad |= 4;
	i = 3;
	map[(i + n) / 16 + 1] <<= 1;
	map[i * n / 16 - 2] |= 8;
	if (map[2] != 10 || map[1] != 9) bad |= 8;
	return bad ? 2 * bad + 1000 : 1;
}
