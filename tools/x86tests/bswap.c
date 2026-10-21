/* expect: 1 -- the four bytes of a 32-bit word written the other way round: three ways of saying it, a near miss, and an operand with an effect (not a byte swap) */
typedef unsigned int u32;
u32 vals[8] = { 0, 1, 0xffffffffu, 0x12345678u, 0x80000001u, 0xdeadbeefu, 0x000000ffu, 0xff000000u };
int calls;
u32 next(u32 *p) { calls++; return *p; }

u32 ref(x) u32 x;
{
	unsigned char b0, b1, b2, b3;

	b0 = x; b1 = x >> 8; b2 = x >> 16; b3 = x >> 24;
	return (u32)b0 << 24 | (u32)b1 << 16 | (u32)b2 << 8 | b3;
}

u32 swap1(x) u32 x; { return (x & 0x000000ff) << 24 | (x & 0x0000ff00) << 8 | (x & 0x00ff0000) >> 8 | (x & 0xff000000) >> 24; }
u32 swap2(x) u32 x; { return (x << 24) | ((x << 8) & 0x00ff0000) | ((x >> 8) & 0x0000ff00) | (x >> 24); }
u32 swap3(x) u32 x; { return (x >> 24) | (x >> 8 & 0xff00) | (x << 8 & 0xff0000) | (x << 24 & 0xff000000); }
u32 near(x) u32 x; { return (x & 0x000000ff) << 24 | (x & 0x0000ff00) << 8 | (x & 0x00ff0000) >> 8 | (x & 0xff000000) >> 16; }
u32 effect(p) u32 *p; { return (next(p) & 0xff) << 24 | (next(p) & 0xff00) << 8 | (next(p) & 0xff0000) >> 8 | (next(p) & 0xff000000) >> 24; }

int test()
{
	int i;
	u32 v;

	for (i = 0; i < 8; i++) {
		v = vals[i];
		if (swap1(v) != ref(v)) return 10 + i;
		if (swap2(v) != ref(v)) return 20 + i;
		if (swap3(v) != ref(v)) return 30 + i;
		if (near(v) != ((v & 0xff) << 24 | (v & 0xff00) << 8 | (v & 0xff0000) >> 8 | (v & 0xff000000) >> 16)) return 40 + i;
		calls = 0;
		if (effect(&vals[i]) != ref(v)) return 50 + i;
		if (calls != 4) return 60 + i;
	}
	return 1;
}
