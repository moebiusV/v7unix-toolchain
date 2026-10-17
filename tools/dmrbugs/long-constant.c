/*
 * dmr-cc gets a long constant wrong when bit 15 of its value is set: the high
 * word comes out one too small, as if the low word had been sign-extended and
 * the borrow kept.  40000L is -25536L (high word -1), 99999L and 100000L have
 * high word 0 instead of 1, and so on; 65536L, 70000L, 131072L and 1000000L
 * are right.  Each value is compared with the same number made by additions.
 *
 * test() returns a bit for each case that is right: 15 is correct (pcc);
 * dmr-cc gives 12 (40000L and 100000L wrong).   tools/dmrtest.sh dmrbugs/long-constant.c
 */
int test()
{
	long a, a2, b, b2, c, c2, d, d2;
	int r;

	r = 0;
	a = 40000L;  a2 = 20000; a2 = a2 + a2;
	b = 100000L; b2 = 20000; b2 = b2 + b2; b2 = b2 + b2; b2 = b2 + 20000;
	c = 70000L;  c2 = 30000; c2 = c2 + c2; c2 = c2 + 10000;
	d = 131072L; d2 = 16384; d2 = d2 + d2; d2 = d2 + d2; d2 = d2 + d2;
	if (a == a2) r |= 1;
	if (b == b2) r |= 2;
	if (c == c2) r |= 4;
	if (d == d2) r |= 8;
	return r;
}
