/* expect: 1 -- unsigned / and %, int and long, small and large divisors */
unsigned ud(a, b) unsigned a, b; { return a / b; }
unsigned um(a, b) unsigned a, b; { return a % b; }
unsigned long uld(a, b) unsigned long a, b; { return a / b; }
unsigned long ulm(a, b) unsigned long a, b; { return a % b; }

unsigned dc(a) unsigned a; { return a / 10 + a % 10 * 3 + a / 32767 + a % 32767; }
unsigned dd(a) unsigned a; { return a / 2 + a % 2; }

int test(void)
{
	int bad = 0;

	if (ud(100u, 7u) != 14) bad |= 1;
	if (um(100u, 7u) != 2) bad |= 2;
	if (ud(65535u, 1u) != 65535u) bad |= 4;
	if (um(65535u, 1u) != 0) bad |= 8;
	if (ud(65535u, 2u) != 32767u) bad |= 16;
	if (ud(65535u, 40000u) != 1) bad |= 32;
	if (um(65535u, 40000u) != 25535u) bad |= 64;
	if (ud(100u, 40000u) != 0) bad |= 128;
	if (um(100u, 40000u) != 100) bad |= 256;
	if (ud(40000u, 40000u) != 1 || um(40000u, 40000u) != 0) bad |= 512;
	if (ud(0u, 5u) != 0 || um(0u, 5u) != 0) bad |= 1024;
	if (uld(1000000UL, 7UL) != 142857UL) bad |= 2048;
	if (ulm(1000000UL, 7UL) != 1UL) bad |= 4096;
	if (uld(4000000000UL, 3000000000UL) != 1L) bad |= 8192;
	if (ulm(4000000000UL, 3000000000UL) != 1000000000UL) bad |= 16384;
	if (dc(65535u) != 6553u + 5u * 3 + 2u + 1u) bad |= 32768;
	if (dd(65535u) != 32767u + 1u) bad |= 32768;
	if (dc(1234u) != 123u + 4u * 3 + 0u + 1234u) bad |= 32768;
	return bad == 0;
}
