/* expect: 1 -- a long shifted by 16 to 31 is a word move; also its high word as an int */
typedef unsigned long u32;
u32 gu = 0xdeadbeefUL; long gs = -0x12345678L;
int hi16(void) { return (int)(gu >> 16); }
int hi24(void) { return (int)(gu >> 24); }
u32 a16(u32 x) { return x >> 16; }
u32 a24(u32 x) { return x >> 24; }
u32 a19(u32 x) { return x >> 19; }
long s16(long x) { return x >> 16; }
long s21(long x) { return x >> 21; }
u32 l16(u32 x) { return x << 16; }
u32 l20(u32 x) { return x << 20; }
int test()
{
	if ((unsigned)hi16() != 0xdeadU) return 2;
	if ((unsigned)hi24() != 0xdeU) return 3;
	if (a16(gu) != 0xdeadUL) return 4;
	if (a24(gu) != 0xdeUL) return 5;
	if (a19(gu) != 0x1bd5UL) return 6;
	if (s16(gs) != (-0x12345678L >> 16)) return 7;
	if (s21(gs) != (-0x12345678L >> 21)) return 8;
	if (s16(0x12345678L) != 0x1234L) return 9;
	if (l16(gu) != 0xbeef0000UL) return 10;
	if (l20(gu) != 0xeef00000UL) return 11;
	return 1;
}
