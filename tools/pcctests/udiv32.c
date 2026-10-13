/* expect: 1 -- unsigned divide and remainder with 32-bit operands (unsigned long) */
unsigned long ud(a, b) unsigned long a, b; { return a / b; }
unsigned long um(a, b) unsigned long a, b; { return a % b; }

int test(void)
{
	int bad = 0;

	if (ud(4000000000UL, 7UL) != 571428571UL) bad |= 1;
	if (um(4000000000UL, 7UL) != 3UL) bad |= 2;
	if (ud(4294967295UL, 1UL) != 4294967295UL) bad |= 4;
	if (um(4294967295UL, 1UL) != 0UL) bad |= 8;
	if (ud(4294967295UL, 2UL) != 2147483647UL) bad |= 16;
	if (ud(4294967295UL, 3000000000UL) != 1UL) bad |= 32;
	if (um(4294967295UL, 3000000000UL) != 1294967295UL) bad |= 64;
	if (ud(100UL, 3000000000UL) != 0UL || um(100UL, 3000000000UL) != 100UL) bad |= 128;
	if (ud(3000000000UL, 3000000000UL) != 1UL) bad |= 256;
	if (ud(0UL, 9UL) != 0UL) bad |= 512;
	return bad == 0;
}
