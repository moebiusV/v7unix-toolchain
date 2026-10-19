/* expect: 1 -- static functions called several times: small ones are copied into the calls, big ones are called */
static int tiny(a) int a; { return a + 1; }

static int big(a, b) int a, b;
{
	int i, s = 0;

	for (i = 0; i < a; i++) {
		if (i & 1)
			s += i * b;
		else
			s -= i + b;
		if (s > 1000)
			s = s % 97;
		else if (s < -1000)
			s = -(s % 89);
	}
	return s + tiny(b) + tiny(a);
}

static inline int bigi(a) int a;
{
	int i, s = 1;

	for (i = 1; i < a; i++) {
		s = s * 3 + i;
		if (s > 5000)
			s -= 4999;
		if ((s & 7) == 3)
			s ^= 0x55;
	}
	return s;
}

int test()
{
	if (tiny(4) != 5) return 2;
	if (big(10, 3) != big(10, 3)) return 3;
	if (big(7, 2) + big(5, 4) != big(7, 2) + big(5, 4)) return 4;
	if (bigi(20) != bigi(20) || bigi(5) == bigi(6)) return 5;
	if (big(0, 1) != 3) return 6;
	return 1;
}
