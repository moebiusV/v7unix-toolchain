/* expect: 1 -- unsigned long >> n is a logical shift; -x on an unsigned long */
unsigned long shr(x, n) unsigned long x; int n; { return x >> n; }
unsigned long neg(x) unsigned long x; { return -x; }

int test()
{
	int bad;
	unsigned long u;

	bad = 0;
	if (shr(0x80000001UL, 31) != 1) bad |= 2;
	if (shr(0xffffffffUL, 16) != 0xffffUL) bad |= 4;
	if (shr(0x80000000UL, 1) != 0x40000000UL) bad |= 8;
	if (shr(5UL, 0) != 5UL) bad |= 16;
	u = 0xf0000000UL;
	if ((u >> 4) != 0x0f000000UL || (u >> 28) != 15UL) bad |= 32;
	u = 0x80000000UL;
	if ((u >> 31) != 1UL) bad |= 64;
	if (neg(1UL) != 0xffffffffUL || neg(0UL) != 0UL || neg(0x80000000UL) != 0x80000000UL) bad |= 128;
	return bad ? 2 * bad + 1000 : 1;
}
