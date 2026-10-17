/* expect: 1 -- x &= ~(b & m) and friends with unsigned and int operands mixed */
unsigned f1(nm, b, m) unsigned nm; int b, m; { nm &= ~(b & m); return nm; }
unsigned f2(nm, m) unsigned nm; int m; { nm &= ~m; return nm; }
unsigned f3(nm, b, m) unsigned nm; int b, m; { nm |= b & m; return nm; }
int f4(a, b) int a; unsigned b; { return a & ~b; }
unsigned f5(a, b, c) unsigned a, b, c; { return a & ~(b | c); }
int f6(a, b) int a, b; { return ~a & ~b; }

int test()
{
	int bad;

	bad = 0;
	if (f1(0xff, 0x0f, 0x3c) != 0xf3) bad |= 1;
	if (f2(0xff, 0x0f) != 0xf0) bad |= 2;
	if (f3(0xf0, 0x0f, 0x3c) != 0xfc) bad |= 4;
	if (f4(0x3c, 0x0f) != 0x30) bad |= 8;
	if (f5(0xff, 0x0f, 0xc0) != 0x30) bad |= 16;
	if (f6(0x0f, 0x30) != ~0x3f) bad |= 32;
	return bad ? bad : 1;
}
