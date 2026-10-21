/* expect: 1 -- unsigned chars kept across a long computation (spilled to the frame) are zero-extended when reloaded */
typedef unsigned long u32;
u32 ref(x) u32 x;
{
	unsigned char b0, b1, b2, b3;

	b0 = x; b1 = x >> 8; b2 = x >> 16; b3 = x >> 24;
	return (u32)b0 << 24 | (u32)b1 << 16 | (u32)b2 << 8 | b3;
}
u32 g = 0xdeadbeefUL;
int test()
{
	u32 r = ref(g);
	if (r != 0xefbeaddeUL) return 2;
	g = 0x80ff7f80UL;
	r = ref(g);
	if (r != 0x807fff80UL) return 4;
	return 1;
}
