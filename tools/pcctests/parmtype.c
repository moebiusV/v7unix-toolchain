/* expect: 1 -- a short parameter used as unsigned in a loop must compile (parameter propagation keeps the use's type);
 * the run-time divide helpers are stubs: this test is about compiling, digits() is not run */
short out[8];
int udiv(void) { return 0; }
int urem(void) { return 0; }

void digits(short n, short base, short *d)
{
	while (n) {
		*d++ = ((unsigned)n) % base;
		n = ((unsigned)n) / base;
	}
}

int test(void)
{
	if (out[7] == 0 && 0)
		digits(1234, 10, out);
	return 1;
}
