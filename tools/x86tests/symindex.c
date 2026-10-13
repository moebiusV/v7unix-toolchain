/* expect: 1 -- sym(,reg,scale) addressing and zero-extending masks */
int ia[10];
long la[4];
char ca[10];
unsigned short sa[6];
unsigned mask8(unsigned x) { return x & 0xff; }
unsigned mask16(unsigned x) { return x & 0xffff; }
unsigned shmask(unsigned x) { return (x >> 8) & 0xff; }
int sum(int n)
{
	int s = 0;

	while (--n >= 0)
		s += ia[n];
	return s;
}

int test(void)
{
	int i, bad = 0;

	for (i = 0; i < 10; i++) { ia[i] = i * 3; ca[i] = i + 100; }
	for (i = 0; i < 6; i++) sa[i] = i * 1000;
	for (i = 0; i < 4; i++) la[i] = i * 100000;
	if (sum(10) != 135) bad |= 1;
	ia[7] += 5;
	if (ia[7] != 26) bad |= 2;
	if (ca[9] != 109 || sa[5] != 5000 || la[3] != 300000) bad |= 4;
	if (mask8(0x12345678) != 0x78) bad |= 8;
	if (mask16(0x12345678) != 0x5678) bad |= 16;
	if (shmask(0x12345678) != 0x56) bad |= 32;
	if (mask8(-1) != 255 || mask16(-1) != 65535) bad |= 64;
	return bad == 0;
}
