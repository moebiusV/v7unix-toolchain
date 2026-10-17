/* expect: 1 -- (int) of a long in memory is its low word: statics, autos, parameters, signed and unsigned.
   (Only where long is twice as wide as int, the PDP-11, is there anything to check.) */
long gl;
unsigned long gul;

int low(q) long q; { return (int)q; }
unsigned ulow(q) unsigned long q; { return (unsigned)q; }
int sum(p, q) int *p; long q; { *p += (int)q; return (int)q + (int)gl; }
int mix() { long a; unsigned long b; a = 70000L; b = 4000000000UL; return (int)a + (int)b + (int)gul; }

int test(void)
{
	int bad = 0, k;
	long big;

	if (sizeof(long) != 2 * sizeof(int))
		return 1;

	big = 305419896L;			/* 0x12345678 */
	if (low(big) != 0x5678) bad |= 1;
	if (ulow(big) != 0x5678) bad |= 2;
	if (low(-2L) != -2) bad |= 4;
	gl = 65537L; gul = 131075UL;		/* low words 1 and 3 */
	k = 10;
	if (sum(&k, 100000L) != (int)100000L + 1) bad |= 8;
	if (k != 10 + (int)100000L) bad |= 16;
	if (mix() != (int)70000L + (int)4000000000UL + 3) bad |= 32;
	if ((int)gl != 1 || (unsigned)gul != 3) bad |= 64;
	if ((int)(gl + 1) != 2) bad |= 128;
	return bad == 0;
}
