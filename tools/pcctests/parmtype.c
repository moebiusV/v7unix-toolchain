/* expect: 1 -- short parameters used as unsigned in a loop (parameter propagation keeps the use's type) */
short out[8];

int nibbles(short n, short shift, short mask, short *d)
{
	short *s;

	s = d;
	while (n) {
		*d++ = ((unsigned)n) & mask;
		n = ((unsigned)n) >> shift;
	}
	return d - s;
}

int test(void)
{
	/* 0x1234 in hex digits */
	if (nibbles(0x1234, 4, 15, out) != 4) return 0;
	return out[0] == 4 && out[1] == 3 && out[2] == 2 && out[3] == 1;
}
