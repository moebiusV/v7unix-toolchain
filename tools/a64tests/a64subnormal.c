/* expect: 1 -- decimal constants in the subnormal range are rounded once, to the nearest */
typedef unsigned long u64;

int test(void)
{
	double d;
	float f;

	d = 4.9406564584124654e-324;
	if (*(u64 *)&d != 0x0000000000000001UL) return 2;
	d = 9.8813129168249309e-324;
	if (*(u64 *)&d != 0x0000000000000002UL) return 3;
	d = 1.4821969375237396e-323;
	if (*(u64 *)&d != 0x0000000000000003UL) return 4;
	d = 2.4703282292062327e-324;
	if (*(u64 *)&d != 0x0000000000000000UL) return 5;
	d = 2.4703282292062328e-324;
	if (*(u64 *)&d != 0x0000000000000001UL) return 6;
	d = 2.191533144019326e-308;
	if (*(u64 *)&d != 0x000fc241c9d488b1UL) return 7;
	d = 1.609013784837619e-308;
	if (*(u64 *)&d != 0x000b91eeefe09f07UL) return 8;
	d = 6.63694244975383e-309;
	if (*(u64 *)&d != 0x0004c5c0e10c167dUL) return 9;
	d = 7.422848294814013e-309;
	if (*(u64 *)&d != 0x0005566d0b13de0dUL) return 10;
	d = 8.904477431643436e-309;
	if (*(u64 *)&d != 0x0006672b7a7f6855UL) return 11;
	d = 2.2250738585072009e-308;
	if (*(u64 *)&d != 0x000fffffffffffffUL) return 12;
	d = 1e-310;
	if (*(u64 *)&d != 0x000012688b70e62bUL) return 13;
	d = 1e-320;
	if (*(u64 *)&d != 0x00000000000007e8UL) return 14;
	f = 1.4012984643e-45f;
	if (*(unsigned *)&f != 0x00000001U) return 40;
	f = 2.8025969286e-45f;
	if (*(unsigned *)&f != 0x00000002U) return 41;
	f = 7.0064923217e-46f;
	if (*(unsigned *)&f != 0x00000001U) return 42;
	f = 7.0064923216e-46f;
	if (*(unsigned *)&f != 0x00000000U) return 43;
	f = 1.1754942e-38f;
	if (*(unsigned *)&f != 0x007fffffU) return 44;
	f = 5.877471754e-39f;
	if (*(unsigned *)&f != 0x00400000U) return 45;
	return 1;
}
