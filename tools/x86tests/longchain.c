/* expect: 1 -- a = b = c = 0 and other chained long assignments */
long a, b, c;
unsigned long ua, ub;
struct s { long x, y; } st;

int test()
{
	int bad;
	long loc1, loc2;

	bad = 0;
	a = 5; b = 6; c = 7;
	a = b = c = 0;
	if (a != 0 || b != 0 || c != 0) bad |= 2;
	a = b = 100000L;
	if (a != 100000L || b != 100000L) bad |= 4;
	loc1 = loc2 = 0;
	if (loc1 != 0 || loc2 != 0) bad |= 8;
	st.x = st.y = 0;
	if (st.x != 0 || st.y != 0) bad |= 16;
	ua = ub = 0xfffffff0UL;
	if (ua != 0xfffffff0UL || ub != 0xfffffff0UL) bad |= 32;
	a = (b = 0) + 7;
	if (a != 7 || b != 0) bad |= 64;
	return bad ? 2 * bad + 1000 : 1;
}
