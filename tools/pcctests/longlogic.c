/* expect: 1 -- a branch on the result of long &, | and ^ looks at both words */
int andz(b) unsigned long b; { if ((b & 0x80000000L) == 0) return 1; return 0; }
int andlo(b) unsigned long b; { if ((b & 0x00000001L) == 0) return 1; return 0; }
int orz(a, b) unsigned long a, b; { if ((a | b) == 0) return 1; return 0; }
int xorz(a, b) unsigned long a, b; { if ((a ^ b) == 0) return 1; return 0; }
int xorne(a, b) unsigned long a, b; { return (a ^ b) != 0; }
int andne(a, b) unsigned long a, b; { return (a & b) != 0; }

int test()
{
	int bad;

	bad = 0;
	if (andz(0x80000000L) != 0 || andz(0x7fffffffL) != 1 || andz(0L) != 1) bad |= 2;
	if (andlo(1L) != 0 || andlo(0x10000L) != 1 || andlo(0x80000000L) != 1) bad |= 4;
	if (orz(0L, 0L) != 1 || orz(0L, 0x10000L) != 0 || orz(2L, 0L) != 0) bad |= 8;
	if (xorz(5L, 5L) != 1 || xorz(5L, 0x10005L) != 0 || xorz(0x10005L, 5L) != 0) bad |= 16;
	if (!xorne(0x10000L, 0L) || xorne(7L, 7L)) bad |= 32;
	if (!andne(0x10000L, 0x30000L) || andne(0x10000L, 0x2ffffL)) bad |= 64;
	return bad ? 2 * bad + 1000 : 1;
}
