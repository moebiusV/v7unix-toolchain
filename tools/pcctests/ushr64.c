/* expect: 1 -- unsigned long long right shifts by constants 1 to 63, against the division by the power of two; and a 64-bit rotate */
typedef unsigned long long u64;
u64 vals[4] = { 0xfedcba9876543210ULL, 0x8000000000000001ULL, 0x00000001ffffffffULL, 0xffffffffffffffffULL };

#define CHK(k) if ((x >> k) != x / (1ULL << k)) return k
int sh(x) u64 x;
{
	CHK(1); CHK(2); CHK(3); CHK(7); CHK(8); CHK(15); CHK(16); CHK(17); CHK(24); CHK(31);
	CHK(32); CHK(33); CHK(39); CHK(40); CHK(47); CHK(48); CHK(56); CHK(62); CHK(63);
	return 0;
}

u64 rol1(x) u64 x; { return (x << 1) | (x >> 63); }
u64 rol13(x) u64 x; { return (x << 13) | (x >> 51); }
u64 rol40(x) u64 x; { return (x << 40) | (x >> 24); }

int test()
{
	int i, r;

	for (i = 0; i < 4; i++) {
		if ((r = sh(vals[i])) != 0) return 10 * (i + 1) + r % 10 + 100 * (r / 10);
	}
	if (rol1(0x8000000000000001ULL) != 0x0000000000000003ULL) return 2;
	if (rol13(0x0123456789abcdefULL) != 0x2468acf13579bde0ULL << 0 && rol13(0x0123456789abcdefULL) != ((0x0123456789abcdefULL << 13) | (0x0123456789abcdefULL / (1ULL << 51)))) return 3;
	if (rol40(0x0123456789abcdefULL) != ((0x0123456789abcdefULL << 40) | (0x0123456789abcdefULL / (1ULL << 24)))) return 4;
	return 1;
}
