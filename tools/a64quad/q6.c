/* expect: 1 -- long double conversions to and from float, double, int, unsigned, long and unsigned long; expected values from the host's __float128 (mkarith.py) */
typedef unsigned long u64;
typedef unsigned int u32;
static long double mk(u64 hi, u64 lo) { u64 w[2]; w[0] = lo; w[1] = hi; return *(long double *)w; }
/* x has these bits (any NaN for an expected NaN) */
static int is(long double x, u64 hi, u64 lo)
{
	u64 *w = (u64 *)&x;
	u64 h = w[1] & 0x7fffffffffffffffUL;

	if (h > 0x7fff000000000000UL || (h == 0x7fff000000000000UL && w[0] != 0))
		return (hi & 0x7fffffffffffffffUL) > 0x7fff000000000000UL || ((hi & 0x7fffffffffffffffUL) == 0x7fff000000000000UL && lo != 0);
	return w[1] == hi && w[0] == lo;
}

int test(void)
{
	if (!is((long double)(unsigned long)0UL, 0x0000000000000000UL, 0x0000000000000000UL)) return 2;
	if (!is((long double)(unsigned long)1UL, 0x3fff000000000000UL, 0x0000000000000000UL)) return 3;
	if (!is((long double)(unsigned long)2UL, 0x4000000000000000UL, 0x0000000000000000UL)) return 4;
	if (!is((long double)(unsigned long)7UL, 0x4001c00000000000UL, 0x0000000000000000UL)) return 5;
	if (!is((long double)(unsigned long)255UL, 0x4006fe0000000000UL, 0x0000000000000000UL)) return 6;
	if (!is((long double)(unsigned long)65535UL, 0x400efffe00000000UL, 0x0000000000000000UL)) return 7;
	if (!is((long double)(unsigned long)2147483647UL, 0x401dfffffffc0000UL, 0x0000000000000000UL)) return 8;
	if (!is((long double)(unsigned long)2147483648UL, 0x401e000000000000UL, 0x0000000000000000UL)) return 9;
	if (!is((long double)(unsigned long)4294967295UL, 0x401efffffffe0000UL, 0x0000000000000000UL)) return 10;
	if (!is((long double)(unsigned long)4294967296UL, 0x401f000000000000UL, 0x0000000000000000UL)) return 11;
	if (!is((long double)(unsigned long)9007199254740993UL, 0x4034000000000000UL, 0x0800000000000000UL)) return 12;
	if (!is((long double)(unsigned long)9223372036854775807UL, 0x403dffffffffffffUL, 0xfffc000000000000UL)) return 13;
	if (!is((long double)(unsigned long)9223372036854775808UL, 0x403e000000000000UL, 0x0000000000000000UL)) return 14;
	if (!is((long double)(unsigned long)18446744073709551615UL, 0x403effffffffffffUL, 0xfffe000000000000UL)) return 15;
	if (!is((long double)(unsigned long)12345678901234567UL, 0x40345ee2a2eb5a5cUL, 0x3800000000000000UL)) return 16;
	if (!is((long double)(long)(0L), 0x0000000000000000UL, 0x0000000000000000UL)) return 17;
	if (!is((long double)(long)(1L), 0x3fff000000000000UL, 0x0000000000000000UL)) return 18;
	if (!is((long double)(long)(-1L), 0xbfff000000000000UL, 0x0000000000000000UL)) return 19;
	if (!is((long double)(long)(-2L), 0xc000000000000000UL, 0x0000000000000000UL)) return 20;
	if (!is((long double)(long)(2147483647L), 0x401dfffffffc0000UL, 0x0000000000000000UL)) return 21;
	if (!is((long double)(long)(-2147483648L), 0xc01e000000000000UL, 0x0000000000000000UL)) return 22;
	if (!is((long double)(long)(123456789L), 0x4019d6f345400000UL, 0x0000000000000000UL)) return 23;
	if (!is((long double)(long)(-987654321L), 0xc01cd6f345880000UL, 0x0000000000000000UL)) return 24;
	if (!is((long double)(long)(9223372036854775807L), 0x403dffffffffffffUL, 0xfffc000000000000UL)) return 25;
	if (!is((long double)(-9223372036854775807L-1), 0xc03e000000000000UL, 0x0000000000000000UL)) return 26;
	if (!is((long double)(long)(-123456789012345678L), 0xc037b69b4ba630f3UL, 0x4e00000000000000UL)) return 27;
	if (!is((long double)(int)(0), 0x0000000000000000UL, 0x0000000000000000UL)) return 28;
	if (!is((long double)(int)(1), 0x3fff000000000000UL, 0x0000000000000000UL)) return 29;
	if (!is((long double)(int)(-1), 0xbfff000000000000UL, 0x0000000000000000UL)) return 30;
	if (!is((long double)(int)(2147483647), 0x401dfffffffc0000UL, 0x0000000000000000UL)) return 31;
	if (!is((long double)(-2147483647-1), 0xc01e000000000000UL, 0x0000000000000000UL)) return 32;
	if (!is((long double)(int)(99999), 0x400f869f00000000UL, 0x0000000000000000UL)) return 33;
	if (!is((long double)(int)(-99999), 0xc00f869f00000000UL, 0x0000000000000000UL)) return 34;
	if (!is((long double)(unsigned)0U, 0x0000000000000000UL, 0x0000000000000000UL)) return 35;
	if (!is((long double)(unsigned)1U, 0x3fff000000000000UL, 0x0000000000000000UL)) return 36;
	if (!is((long double)(unsigned)4294967295U, 0x401efffffffe0000UL, 0x0000000000000000UL)) return 37;
	if (!is((long double)(unsigned)2147483648U, 0x401e000000000000UL, 0x0000000000000000UL)) return 38;
	{ u64 bits = 0UL; double d = *(double *)&bits; if (!is((long double)d, 0x0000000000000000UL, 0x0000000000000000UL)) return 39; }
	{ u64 bits = 9221120237041090560UL; double d = *(double *)&bits; if (!is((long double)d, 0x7fff800000000000UL, 0x0000000000000000UL)) return 40; }
	{ u64 bits = 1UL; double d = *(double *)&bits; if (!is((long double)d, 0x3bcd000000000000UL, 0x0000000000000000UL)) return 41; }
	{ u64 bits = 9221120237041090560UL; double d = *(double *)&bits; if (!is((long double)d, 0x7fff800000000000UL, 0x0000000000000000UL)) return 42; }
	{ u64 bits = 9221120237041090560UL; double d = *(double *)&bits; if (!is((long double)d, 0x7fff800000000000UL, 0x0000000000000000UL)) return 43; }
	{ u64 bits = 9221120237041090560UL; double d = *(double *)&bits; if (!is((long double)d, 0x7fff800000000000UL, 0x0000000000000000UL)) return 44; }
	{ u64 bits = 1UL; double d = *(double *)&bits; if (!is((long double)d, 0x3bcd000000000000UL, 0x0000000000000000UL)) return 45; }
	{ u64 bits = 9221120237041090560UL; double d = *(double *)&bits; if (!is((long double)d, 0x7fff800000000000UL, 0x0000000000000000UL)) return 46; }
	{ u64 bits = 9223372036854775808UL; double d = *(double *)&bits; if (!is((long double)d, 0x8000000000000000UL, 0x0000000000000000UL)) return 47; }
	{ u64 bits = 9221120237041090560UL; double d = *(double *)&bits; if (!is((long double)d, 0x7fff800000000000UL, 0x0000000000000000UL)) return 48; }
	{ u64 bits = 4670380312632226264UL; double d = *(double *)&bits; if (!is((long double)d, 0x400d086264c6bf1dUL, 0x8000000000000000UL)) return 49; }
	{ u64 bits = 9218868437227405312UL; double d = *(double *)&bits; if (!is((long double)d, 0x7fff000000000000UL, 0x0000000000000000UL)) return 50; }
	{ u64 bits = 9094988921128908188UL; double d = *(double *)&bits; if (!is((long double)d, 0x43e37e43c8800759UL, 0xc000000000000000UL)) return 51; }
	{ u64 bits = 13891695264279749456UL; double d = *(double *)&bits; if (!is((long double)d, 0xc00c9373dd35ee35UL, 0x0000000000000000UL)) return 52; }
	{ u32 bits = 1119183881U; float f = *(float *)&bits; if (!is((long double)f, 0x40056ac812000000UL, 0x0000000000000000UL)) return 53; }
	{ u32 bits = 0U; float f = *(float *)&bits; if (!is((long double)f, 0x0000000000000000UL, 0x0000000000000000UL)) return 54; }
	{ u32 bits = 2147483648U; float f = *(float *)&bits; if (!is((long double)f, 0x8000000000000000UL, 0x0000000000000000UL)) return 55; }
	{ u32 bits = 2147483648U; float f = *(float *)&bits; if (!is((long double)f, 0x8000000000000000UL, 0x0000000000000000UL)) return 56; }
	{ u32 bits = 2139095040U; float f = *(float *)&bits; if (!is((long double)f, 0x7fff000000000000UL, 0x0000000000000000UL)) return 57; }
	{ u32 bits = 71362U; float f = *(float *)&bits; if (!is((long double)f, 0x3f7a16c200000000UL, 0x0000000000000000UL)) return 58; }
	{ u32 bits = 3283275223U; float f = *(float *)&bits; if (!is((long double)f, 0xc007659bae000000UL, 0x0000000000000000UL)) return 59; }
	{ u32 bits = 3293160636U; float f = *(float *)&bits; if (!is((long double)f, 0xc008934978000000UL, 0x0000000000000000UL)) return 60; }
	{ double d = (double)mk(0x3fff000000000000UL, 0x0000000000000000UL); u64 *p = (u64 *)&d; if (*p != 0x3ff0000000000000) return 61; }
	{ float d = (float)mk(0x3fff000000000000UL, 0x0000000000000000UL); u32 *p = (u32 *)&d; if (*p != 0x3f800000) return 62; }
	if ((int)mk(0x3fff000000000000UL, 0x0000000000000000UL) != 1) return 63;
	if ((long)mk(0x3fff000000000000UL, 0x0000000000000000UL) != 1L) return 64;
	if ((unsigned)mk(0x3fff000000000000UL, 0x0000000000000000UL) != 1U) return 65;
	if ((unsigned long)mk(0x3fff000000000000UL, 0x0000000000000000UL) != 1UL) return 66;
	{ double d = (double)mk(0x4004000000000000UL, 0x0123456789abcdefUL); u64 *p = (u64 *)&d; if (*p != 0x4040000000000000) return 67; }
	{ float d = (float)mk(0x4004000000000000UL, 0x0123456789abcdefUL); u32 *p = (u32 *)&d; if (*p != 0x42000000) return 68; }
	if ((int)mk(0x4004000000000000UL, 0x0123456789abcdefUL) != 32) return 69;
	if ((long)mk(0x4004000000000000UL, 0x0123456789abcdefUL) != 32L) return 70;
	if ((unsigned)mk(0x4004000000000000UL, 0x0123456789abcdefUL) != 32U) return 71;
	if ((unsigned long)mk(0x4004000000000000UL, 0x0123456789abcdefUL) != 32UL) return 72;
	{ double d = (double)mk(0xc027000000000000UL, 0x000000000000004dUL); u64 *p = (u64 *)&d; if (*p != 0xc270000000000000) return 73; }
	{ float d = (float)mk(0xc027000000000000UL, 0x000000000000004dUL); u32 *p = (u32 *)&d; if (*p != 0xd3800000) return 74; }
	if ((long)mk(0xc027000000000000UL, 0x000000000000004dUL) != -1099511627776L) return 75;
	{ double d = (double)mk(0x403dffffffffffffUL, 0xffffffffffffffffUL); u64 *p = (u64 *)&d; if (*p != 0x43e0000000000000) return 76; }
	{ float d = (float)mk(0x403dffffffffffffUL, 0xffffffffffffffffUL); u32 *p = (u32 *)&d; if (*p != 0x5f000000) return 77; }
	if ((long)mk(0x403dffffffffffffUL, 0xffffffffffffffffUL) != 9223372036854775807L) return 78;
	if ((unsigned long)mk(0x403dffffffffffffUL, 0xffffffffffffffffUL) != 9223372036854775807UL) return 79;
	{ double d = (double)mk(0x401e000000000000UL, 0x0000000000000005UL); u64 *p = (u64 *)&d; if (*p != 0x41e0000000000000) return 80; }
	{ float d = (float)mk(0x401e000000000000UL, 0x0000000000000005UL); u32 *p = (u32 *)&d; if (*p != 0x4f000000) return 81; }
	if ((long)mk(0x401e000000000000UL, 0x0000000000000005UL) != 2147483648L) return 82;
	if ((unsigned long)mk(0x401e000000000000UL, 0x0000000000000005UL) != 2147483648UL) return 83;
	{ double d = (double)mk(0xc01e000000000000UL, 0x0000000000000000UL); u64 *p = (u64 *)&d; if (*p != 0xc1e0000000000000) return 84; }
	{ float d = (float)mk(0xc01e000000000000UL, 0x0000000000000000UL); u32 *p = (u32 *)&d; if (*p != 0xcf000000) return 85; }
	if ((long)mk(0xc01e000000000000UL, 0x0000000000000000UL) != -2147483648L) return 86;
	{ double d = (double)mk(0x3ffc000000000000UL, 0x0000000000000abcUL); u64 *p = (u64 *)&d; if (*p != 0x3fc0000000000000) return 87; }
	{ float d = (float)mk(0x3ffc000000000000UL, 0x0000000000000abcUL); u32 *p = (u32 *)&d; if (*p != 0x3e000000) return 88; }
	if ((int)mk(0x3ffc000000000000UL, 0x0000000000000abcUL) != 0) return 89;
	if ((long)mk(0x3ffc000000000000UL, 0x0000000000000abcUL) != 0L) return 90;
	if ((unsigned)mk(0x3ffc000000000000UL, 0x0000000000000abcUL) != 0U) return 91;
	if ((unsigned long)mk(0x3ffc000000000000UL, 0x0000000000000abcUL) != 0UL) return 92;
	{ double d = (double)mk(0x444b000000000000UL, 0x0000000000000001UL); u64 *p = (u64 *)&d; if (*p != 0x7ff0000000000000) return 93; }
	{ float d = (float)mk(0x444b000000000000UL, 0x0000000000000001UL); u32 *p = (u32 *)&d; if (*p != 0x7f800000) return 94; }
	{ double d = (double)mk(0x3bb3000000000000UL, 0x0000000000000001UL); u64 *p = (u64 *)&d; if (*p != 0x0) return 95; }
	{ float d = (float)mk(0x3bb3000000000000UL, 0x0000000000000001UL); u32 *p = (u32 *)&d; if (*p != 0x0) return 96; }
	if ((int)mk(0x3bb3000000000000UL, 0x0000000000000001UL) != 0) return 97;
	if ((long)mk(0x3bb3000000000000UL, 0x0000000000000001UL) != 0L) return 98;
	if ((unsigned)mk(0x3bb3000000000000UL, 0x0000000000000001UL) != 0U) return 99;
	if ((unsigned long)mk(0x3bb3000000000000UL, 0x0000000000000001UL) != 0UL) return 100;
	{ double d = (double)mk(0x407f000000000000UL, 0x0000000000000003UL); u64 *p = (u64 *)&d; if (*p != 0x47f0000000000000) return 101; }
	{ float d = (float)mk(0x407f000000000000UL, 0x0000000000000003UL); u32 *p = (u32 *)&d; if (*p != 0x7f800000) return 102; }
	{ double d = (double)mk(0xc045000000000000UL, 0x0000000000000001UL); u64 *p = (u64 *)&d; if (*p != 0xc450000000000000) return 103; }
	{ float d = (float)mk(0xc045000000000000UL, 0x0000000000000001UL); u32 *p = (u32 *)&d; if (*p != 0xe2800000) return 104; }
	{ double d = (double)mk(0x4016000000000123UL, 0x456789abcdef0123UL); u64 *p = (u64 *)&d; if (*p != 0x4160000000001234) return 105; }
	{ float d = (float)mk(0x4016000000000123UL, 0x456789abcdef0123UL); u32 *p = (u32 *)&d; if (*p != 0x4b000000) return 106; }
	if ((int)mk(0x4016000000000123UL, 0x456789abcdef0123UL) != 8388608) return 107;
	if ((long)mk(0x4016000000000123UL, 0x456789abcdef0123UL) != 8388608L) return 108;
	if ((unsigned)mk(0x4016000000000123UL, 0x456789abcdef0123UL) != 8388608U) return 109;
	if ((unsigned long)mk(0x4016000000000123UL, 0x456789abcdef0123UL) != 8388608UL) return 110;
	{ double d = (double)mk(0x3f73000000000000UL, 0x0000000000000005UL); u64 *p = (u64 *)&d; if (*p != 0x3730000000000000) return 111; }
	{ float d = (float)mk(0x3f73000000000000UL, 0x0000000000000005UL); u32 *p = (u32 *)&d; if (*p != 0x200) return 112; }
	if ((int)mk(0x3f73000000000000UL, 0x0000000000000005UL) != 0) return 113;
	if ((long)mk(0x3f73000000000000UL, 0x0000000000000005UL) != 0L) return 114;
	if ((unsigned)mk(0x3f73000000000000UL, 0x0000000000000005UL) != 0U) return 115;
	if ((unsigned long)mk(0x3f73000000000000UL, 0x0000000000000005UL) != 0UL) return 116;
	{ double d = (double)mk(0x43fe000000000000UL, 0x0000000000000000UL); u64 *p = (u64 *)&d; if (*p != 0x7fe0000000000000) return 117; }
	{ float d = (float)mk(0x43fe000000000000UL, 0x0000000000000000UL); u32 *p = (u32 *)&d; if (*p != 0x7f800000) return 118; }
	return 1;
}
