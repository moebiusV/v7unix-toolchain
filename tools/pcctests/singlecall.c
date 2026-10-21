/* expect: 1 -- static functions named at one call only are inlined there; the ones that are not (two calls, address taken, recursive, static local) still work */
static int count;
static int tbl[4] = { 1, 2, 4, 8 };

static int once_loop(int n, int k)
{
	int i, s = 0;
	for (i = 0; i < n; i++)
		s += tbl[i & 3] * k;
	return s;
}

static int once_early(int x)
{
	if (x < 0)
		return -1;
	if (x == 0)
		return 0;
	count++;
	return x * 2;
}

static int with_static(int x)
{
	static int calls;
	calls += x;
	return calls;
}

static int twice(int x) { return x + 100; }
static int viaptr(int x) { return x * 7; }
static int recur(int n) { return n <= 0 ? 0 : n + recur(n - 1); }
static int dead_only(int x) { return x * 1000; }

int (*fp)(int) = viaptr;

static int in_loop(int x) { return x + 1; }

/* used before it is defined: a call, never inlined, and the function must be written */
static int later(int);
static int user_of_later(int x) { return later(x) + 1; }
static int later(int x) { return x * 9; }

int test()
{
	int i, s = 0;

	if (once_loop(8, 3) != 3 * (1 + 2 + 4 + 8) * 2) return 2;
	if (once_early(-5) != -1 || once_early(0) != 0 || count != 0) return 3;
	if (once_early(21) != 42 || count != 1) return 4;
	with_static(5);
	if (with_static(6) != 11) return 5;
	if (twice(1) + twice(2) != 203) return 6;
	if ((*fp)(3) != 21 || viaptr(2) != 14) return 7;
	if (recur(10) != 55) return 8;
	for (i = 0; i < 5; i++)
		s = in_loop(s);
	if (s != 5) return 9;
	if (user_of_later(2) != 19) return 10;
	if (count > 100)
		return dead_only(1);
	return 1;
}
