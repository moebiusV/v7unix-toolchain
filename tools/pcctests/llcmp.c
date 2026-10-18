/* expect: 1 -- long long comparisons of computed values, against zero and constants, signed and unsigned */
long long sum(x, y) long long x, y; { return x + y; }
int ne0(x, y) long long x, y; { return x + y != 0; }
int eq0(x, y) long long x, y; { return x + y == 0; }
int lt0(x, y) long long x, y; { return x - y < 0; }
int gt0(x, y) long long x, y; { return x - y > 0; }
int le(x, y) long long x, y; { return x + 1 <= y; }
int ge(x, y) long long x, y; { return x >= y + 1; }
int ult(x, y) unsigned long long x, y; { return x + 1 < y; }
int ugt(x, y) unsigned long long x, y; { return x - 1 > y; }
int ne5(x) long long x; { return x * 2 != 10000000000LL; }

int test()
{
	int bad;

	bad = 0;
	if (!ne0(1LL, 0LL) || ne0(5LL, -5LL) || !ne0(4294967296LL, -1LL)) bad |= 2;
	if (!eq0(5LL, -5LL) || eq0(1LL, 0LL) || eq0(4294967296LL, -1LL)) bad |= 4;
	if (!lt0(1LL, 4294967297LL) || lt0(4294967297LL, 1LL) || lt0(7LL, 7LL)) bad |= 8;
	if (!gt0(4294967297LL, 1LL) || gt0(1LL, 4294967297LL) || gt0(7LL, 7LL)) bad |= 16;
	if (!le(4LL, 5LL) || le(5LL, 5LL) || !le(-4294967296LL, 0LL)) bad |= 32;
	if (!ge(6LL, 5LL) || ge(5LL, 5LL) || !ge(4294967296LL, 4294967295LL)) bad |= 64;
	if (!ult(1ULL, 3ULL) || ult(3ULL, 3ULL) || !ult(1ULL, 18446744073709551615ULL)) bad |= 128;
	if (!ugt(18446744073709551615ULL, 3ULL) || ugt(3ULL, 18446744073709551615ULL) || ugt(2ULL, 2ULL)) bad |= 256;
	if (ne5(5000000000LL) || !ne5(5000000001LL)) bad |= 512;
	if (sum(1LL, 2LL) != 3LL) bad |= 1024;
	return bad ? 2 * bad + 1000 : 1;
}
