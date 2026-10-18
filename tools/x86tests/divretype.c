/* expect: 1 -- (unsigned)(a % b), (unsigned)(a / b), (unsigned)(a >> n) with signed operands: the conversion must not turn the operation unsigned */
unsigned rem(int a, int b) { return (unsigned)(a % b); }
unsigned quo(int a, int b) { return (unsigned)(a / b); }
unsigned shr(int a, int n) { return (unsigned)(a >> n); }
int irem(unsigned a, unsigned b) { return (int)(a % b); }
int iquo(unsigned a, unsigned b) { return (int)(a / b); }
unsigned mix(short p, int v) { return (unsigned)(int)((int)((int)p >> (v & 15)) % 8); }
int test()
{
	int bad = 0;

	if (rem(-1, 8) != (unsigned)-1 || rem(-17, 5) != (unsigned)-2) bad |= 1;
	if (quo(-17, 4) != (unsigned)-4 || quo(-1, 1) != (unsigned)-1) bad |= 2;
	if (shr(-256, 4) != (unsigned)-16) bad |= 4;
	if (irem(40000u, 7) != (int)(40000u % 7) || iquo(40000u, 3) != (int)(40000u / 3)) bad |= 8;
	if (mix(-256, 3) != (unsigned)((-256 >> 3) % 8) || mix(-256, 8) != (unsigned)-1) bad |= 16;
	return bad ? 2 * bad + 1000 : 1;
}
