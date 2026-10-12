/* expect: 1 -- floating-point immediates: arithmetic and comparisons with one-word constants */
double one, half, two, neg, tenth;

/* result bits for x against 1.0, in both operand orders */
int rel(double x)
{
	int r;

	r = 0;
	if (x < 1.0)  r |= 1;
	if (x <= 1.0) r |= 2;
	if (x > 1.0)  r |= 4;
	if (x >= 1.0) r |= 8;
	if (x == 1.0) r |= 16;
	if (x != 1.0) r |= 32;
	if (1.0 < x)  r |= 64;
	if (1.0 > x)  r |= 128;
	return r;
}

double arith(double x)
{
	/* 10.0, 0.5, 2.0, -1.0 and 2.5 are all a single word */
	return ((x * 10.0 + 2.5) / 0.5 - 2.0) * 2.0 + -1.0;
}

int test(void)
{
	int bad;
	double a;

	bad = 0;
	one = 1; half = 0.5; two = 2; neg = -1.5; tenth = 0.1;
	if (rel(half) != (1 | 2 | 32 | 128)) bad |= 1;
	if (rel(one)  != (2 | 8 | 16)) bad |= 2;
	if (rel(two)  != (4 | 8 | 32 | 64)) bad |= 4;
	if (rel(neg)  != (1 | 2 | 32 | 128)) bad |= 8;
	a = arith(1.0);			/* ((10+2.5)/0.5 - 2)*2 - 1 = 45 */
	if ((int)a != 45) bad |= 16;
	a = arith(0.0);			/* ((2.5)/0.5 - 2)*2 - 1 = 5 */
	if ((int)a != 5) bad |= 32;
	a = tenth * 10.0;		/* 0.1 is not an immediate */
	if (!(a > 0.99 && a < 1.01)) bad |= 64;
	a = 0.0;
	if (a != 0.0 || !(a == 0) || a < 0 || a > 0) bad |= 128;
	return bad == 0;
}
