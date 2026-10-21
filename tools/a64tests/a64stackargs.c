/* expect: 1 -- calls with arguments on the stack: one to six of them, constants, registers, computed and nested calls, 32 and 64 bits */
long k7(long a, long b, long c, long d, long e, long f, long g) { return a + 2 * b + 3 * c + 4 * d + 5 * e + 6 * f + 7 * g; }
long k8(long a, long b, long c, long d, long e, long f, long g, long h) { return k7(a, b, c, d, e, f, g) + 8 * h; }
long k12(long a, long b, long c, long d, long e, long f, long g, long h, long i, long j, long k, long l)
{
	return k8(a, b, c, d, e, f, g, h) + 9 * i + 10 * j + 11 * k + 12 * l;
}
int i9(int a, int b, int c, int d, int e, int f, int g, int h, int i) { return a - b + c - d + e - f + g - h + i; }
long id(long v) { return v; }

long t7(long x) { return k7(x, 2, 3, 4, 5, 6, x + 1); }
long t8(long x, long y) { return k8(1, 2, 3, 4, 5, 6, x, y); }
long t12(long x) { return k12(x, x + 1, 3, 4, 5, 6, id(7), id(x) + 1, 9, 10, 11, id(id(12))); }
int t9(int x, int y) { return i9(1, 2, 3, 4, 5, 6, 7, x, y); }
long nested(long x) { return k7(1, 2, 3, 4, 5, 6, k7(x, 0, 0, 0, 0, 0, id(2))); }

int test()
{
	long ref7, ref8;

	ref7 = 5 + 2 * 2 + 3 * 3 + 4 * 4 + 5 * 5 + 6 * 6 + 7 * 6;
	if (t7(5) != 5 + 4 + 9 + 16 + 25 + 36 + 7 * 6) return 2;
	if (t8(7, 9) != 1 + 4 + 9 + 16 + 25 + 36 + 7 * 7 + 8 * 9) return 3;
	if (t12(4) != k8(4, 5, 3, 4, 5, 6, 7, 5) + 9 * 9 + 100 + 121 + 144) return 4;
	if (t9(8, 9) != 1 - 2 + 3 - 4 + 5 - 6 + 7 - 8 + 9) return 5;
	if (nested(3) != 1 + 4 + 9 + 16 + 25 + 36 + 7 * (3 + 14)) return 6;
	return 1;
}
