/*
 * dmr-cc widens an unsigned to a long by sign extension: 65535 becomes -1L.
 * test() returns a bit for each case that is right: 3 is correct (pcc),
 * 0 is what dmr-cc gives.   tools/dmrtest.sh dmrbugs/unsigned-to-long.c
 */
long wide(i) int i; { return (long)(unsigned)i; }
long widev(u) unsigned u; { return (long)u; }
int test()
{
	unsigned u;
	long a, b;

	u = (unsigned)-1;
	a = wide(-1);
	b = widev(u);
	return (a == 65535L ? 1 : 0) | (b == 65535L ? 2 : 0);
}
