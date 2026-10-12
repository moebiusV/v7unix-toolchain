/* expect: 1 -- a register variable whose last use is inside a loop stays live round the loop (no -O: no flow graph) */
int x, y, z, w, p, q, r, t;

int run(register char *s, register int n)
{
	register int h;

	h = 0;
	while (n-- > 0)
		h = h + *s++ + ((x + y) * (z + w) + (p + q) * (r + t));
	return h;
}

int test(void)
{
	char buf[4];

	x = 1; y = 2; z = 3; w = 4; p = 5; q = 6; r = 7; t = 8;
	buf[0] = 10; buf[1] = 20; buf[2] = 30; buf[3] = 40;
	/* each pass adds 3*7 + 11*15 = 186 and one byte */
	return run(buf, 4) == 4 * 186 + 100;
}
