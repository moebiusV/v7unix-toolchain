/* expect: 1 -- the low word of a long xor/and/or/sum/product/shift is the same computed on the low words, and a long and with a mask of one half */
unsigned long a = 0x12345678UL, b = 0x9abcdef0UL;
long sa = -123456L;
unsigned char uc = 0xf3;
char sc = -5;
unsigned short tab[256];

int w1(x) unsigned long x; { return (int)((x ^ b) & 0xff); }
int w2(x) unsigned long x; { return (int)((x ^ (unsigned long)uc) & 0xff) << 1; }
unsigned w3(x) unsigned long x; { return (unsigned)(x + b); }
unsigned w4(x) unsigned long x; { return (unsigned)(x * 7UL + 3); }
int w5(x) long x; { return (int)(x - (long)sc); }
unsigned w6(x) unsigned long x; { return (unsigned)((x << 3) | (b & 0xf0f0UL)); }
int w7(x) long x; { return (int)~x; }
int w8(x) long x; { return (int)-x; }
unsigned long m1(x) unsigned long x; { return x & 0xffff0000UL; }
unsigned long m2(x) unsigned long x; { return x & 0x0000ffffUL; }
unsigned long m3(x) unsigned long x; { return x & 0xffffffffUL; }
unsigned long m4(x) unsigned long x; { return (x ^ b) & 0x00ff00ffUL; }
short w9(x) unsigned long x; { return (short)(x ^ 0x8001UL); }

int test()
{
	int i;
	unsigned long v;

	if (w1(a) != (int)((a ^ b) & 0xff)) return 2;
	if (w1(a) != (0x78 ^ 0xf0)) return 3;
	if (w2(a) != ((0x78 ^ 0xf3) << 1)) return 4;
	if (w3(a) != (unsigned)((a + b) & 0xffffUL)) return 5;
	if (w3(a) != (unsigned)(0x5678U + 0xdef0U)) return 6;
	if (w4(a) != (unsigned)((a * 7UL + 3) & 0xffffUL)) return 7;
	if (w4(a) != (unsigned)(0x5678U * 7U + 3)) return 8;
	if (w5(sa) != (int)((sa + 5) & 0xffffL) && w5(sa) != (int)(sa + 5)) return 9;
	if (w6(a) != (unsigned)(((0x5678U << 3) | 0xf0f0U) & 0xffffU)) return 10;
	if (w7(a) != (int)(~0x5678U & 0xffffU)) return 11;
	if (w8(sa) != (int)(-sa)) return 12;
	if (m1(a) != 0x12340000UL) return 13;
	if (m2(a) != 0x5678UL) return 14;
	if (m3(a) != a) return 15;
	if (m4(a) != ((a ^ b) & 0x00ff00ffUL)) return 16;
	if ((unsigned short)w9(a) != (unsigned short)((a ^ 0x8001UL) & 0xffffUL)) return 17;
	for (i = 0; i < 256; i++) tab[i] = i * 3;
	v = 0xdeadbeefUL;
	for (i = 0; i < 40; i++) {
		unsigned long x = v;
		v = (v >> 8) ^ tab[(x ^ (unsigned long)i) & 0xff];
		v += (unsigned long)i * 65537UL;
	}
	if (v != 0x002726a4UL) return 18;
	return 1;
}
