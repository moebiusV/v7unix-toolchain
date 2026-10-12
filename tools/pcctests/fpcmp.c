/* expect: 1 -- floating-point comparisons with zero and with shared constants */
double zero, pos, neg, big;
double s1, s2;

int run(double x)
{
	int r;

	r = 0;
	if (x == 0) r |= 1;
	if (x != 0) r |= 2;
	if (x < 0)  r |= 4;
	if (x <= 0) r |= 8;
	if (x > 0)  r |= 16;
	if (x >= 0) r |= 32;
	return r;
}

/* the same constants used several times: they share storage */
double mix(double x)
{
	return (x * 10 + 10) / 10 - 1 + 10 * 0.5;
}

int test(void)
{
	int bad;

	bad = 0;
	zero = 0; pos = 1.5; neg = -2.5;
	if (run(zero) != (1 | 8 | 32)) bad |= 1;
	if (run(pos)  != (2 | 16 | 32)) bad |= 2;
	if (run(neg)  != (2 | 4 | 8)) bad |= 4;
	if (run(zero + 0.0) != (1 | 8 | 32)) bad |= 8;
	s1 = mix(2.0);				/* (20+10)/10 - 1 + 5 = 7 */
	if ((int)s1 != 7) bad |= 16;
	s2 = 0;
	while (s2 < 3.0) s2 += 1.0;
	if (s2 != 3.0) bad |= 32;
	if (!(0 == zero)) bad |= 64;		/* constant on the left */
	if (!(0 < pos)) bad |= 128;
	if (0 < neg) bad |= 256;
	return bad == 0;
}
