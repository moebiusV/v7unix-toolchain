/* expect: 1 -- GNU case ranges over 64-bit values: the range 10^18 ... 9999999999999999999 used to make ccom run forever (it was expanded one case at a time); ranges ending at the largest value */
int foo(unsigned long long a)
{
	switch (a) {
	case 1000000000000000000ULL ... 9999999999999999999ULL: return 19;
	default: return 20;
	}
}

int top(unsigned long long a)
{
	switch (a) {
	case 0 ... 5: return 1;
	case 18446744073709551600ULL ... 18446744073709551615ULL: return 2;
	case 100000 ... 200000: return 3;
	default: return 0;
	}
}

int sgn(long long a)
{
	switch (a) {
	case -9223372036854775807LL - 1 ... -1000000000000LL: return 1;
	case 1000000000000LL ... 9223372036854775807LL: return 2;
	default: return 0;
	}
}

int test(void)
{
	if (foo(1000000000000000000ULL) != 19 || foo(9999999999999999999ULL) != 19 || foo(999999999999999999ULL) != 20 || foo(10000000000000000000ULL) != 20 || foo(0) != 20)
		return 0;
	if (top(0) != 1 || top(5) != 1 || top(6) != 0 || top(18446744073709551599ULL) != 0 || top(18446744073709551600ULL) != 2 || top(18446744073709551615ULL) != 2 || top(150000) != 3 || top(200001) != 0)
		return 0;
	if (sgn(-9223372036854775807LL - 1) != 1 || sgn(-1000000000000LL) != 1 || sgn(-999999999999LL) != 0 || sgn(0) != 0 || sgn(1000000000000LL) != 2 || sgn(9223372036854775807LL) != 2)
		return 0;
	return 1;
}
