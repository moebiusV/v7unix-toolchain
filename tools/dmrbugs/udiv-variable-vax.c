/*
 * 32V's cc (Johnson's pcc) compiles unsigned / and % as the signed divl3, so a
 * value with the high bit set is divided as a negative number.  Run with
 *	vax32v.py --guestcc udiv-variable-vax.c     (32V's own cc: bits set)
 *	vax32v.py -f "..." --ccom CCOM udiv-variable-vax.c   (pcc: 0)
 * test() returns the bits of the cases that came out wrong.
 */
unsigned ud(a, b) unsigned a, b; { return a / b; }
unsigned um(a, b) unsigned a, b; { return a % b; }
int test()
{
	int bad;
	unsigned x, y;

	bad = 0;
	x = 4000000000; y = 7;
	if (ud(x, y) != 571428571) bad |= 1;
	if (um(x, y) != 3) bad |= 2;
	x = 4294967295; y = 3000000000;
	if (ud(x, y) != 1) bad |= 4;
	if (um(x, y) != 1294967295) bad |= 8;
	x = 100; y = 7;
	if (ud(x, y) != 14 || um(x, y) != 2) bad |= 16;
	return bad;
}
