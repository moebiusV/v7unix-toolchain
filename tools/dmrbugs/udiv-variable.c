/*
 * Errors of dmr-cc, found by running its output (tools/dmrtest.sh) against pcc's.
 * The function test() returns the bits of the cases that came out wrong; pcc
 * returns 0, dmr-cc returns 87 (cases 1, 2, 4, 16 and 64):
 *
 *	dmr-cc  divides an unsigned by a variable with  clr r0; div b,r0.  The
 *	PDP-11 div takes a signed divisor and a quotient that fits a signed word,
 *	so it is wrong for a divisor of 32768 or more, and for a divisor of 1 with a
 *	dividend of 32768 or more.
 *
 * (V7 C also has no unsigned char and no u suffix; that is the language, not an error.)
 */
unsigned ud(a, b) unsigned a, b; { return a / b; }
unsigned um(a, b) unsigned a, b; { return a % b; }
int test()
{
	int bad;
	unsigned x, y, z;
	bad = 0;
	x = 60000; y = 40000;
	if (ud(x, y) != 1) bad |= 1;
	if (um(x, y) != 20000) bad |= 2;
	x = 65535; y = 32768;
	if (ud(x, y) != 1) bad |= 4;
	x = 100; y = 3;
	if (ud(x, y) != 33) bad |= 8;
	x = 65535; y = 65535;
	if (ud(x, y) != 1) bad |= 16;
	x = 1000; y = 40000;
	if (ud(x, y) != 0) bad |= 32;
	x = 65535; y = 1;
	if (ud(x, y) != 65535) bad |= 64;
	return bad;
}
