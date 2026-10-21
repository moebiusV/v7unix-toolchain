/* expect: 1 -- (x | C1) == C2 where x has no bit of C1, folded; and the cases that must not fold */
typedef unsigned long u32;
int calls;
u32 next(v) u32 v; { calls++; return v; }
u32 big = 0x02000000UL;

int eq9(a) u32 a; { return ((a & 0xff) | 0x02000000UL) == 0x02000009UL; }
int ne9(a) u32 a; { return ((a & 0xff) | 0x02000000UL) != 0x02000009UL; }
int no(a) u32 a; { return ((a & 0xff) | 0x02000000UL) == 0x03000009UL; }
int no2(a) u32 a; { return ((a & 0xff) | 0x02000000UL) != 0x020001ffUL; }
int wide(a) u32 a; { return ((a & 0x1ff) | 0x100UL) == 0x1ffUL; }
int eff(a) u32 a; { return ((next(a) & 0xff) | 0x02000000UL) == 0x03000009UL; }
int byte(c) unsigned char c; { return ((u32)c | 0x100UL) == 0x141UL; }
int shifted(a) u32 a; { return (((a >> 8) & 0xff) | 0x00010000UL) == 0x00010041UL; }

u32 andor(a) u32 a; { return ((a | 0x02000000UL) & 0xffUL); }
u32 andor2(a) u32 a; { return ((a | 0x0200000fUL) & 0xffUL); }

int test()
{
	if (!eq9(0x1234a509UL) || eq9(0x1234a50aUL)) return 2;
	if (ne9(0x1234a509UL) || !ne9(0x12340000UL)) return 3;
	if (no(0x09UL) || no(0xffffUL)) return 4;
	if (!no2(0x09UL) || !no2(0xff0000ffUL) || !no2(0xffUL)) return 5;
	if (wide(0x0ffUL) != 1 || wide(0x1ffUL) != 1) return 7;
	if (wide(0x200UL) != 0) return 9;
	calls = 0;
	if (eff(5)) return 10;
	if (calls != 1) return 11;
	if (!byte(0x41) || byte(0x42)) return 12;
	if (!shifted(0x4100UL) || shifted(0x4200UL)) return 13;
	if (andor(0x12345678UL) != 0x78UL || andor(0x02000000UL) != 0) return 14;
	if (andor2(0x12345670UL) != 0x7fUL) return 15;
	return 1;
}
