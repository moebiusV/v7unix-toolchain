/* expect: 1 -- unsigned divide and remainder with 32-bit operands (needs a 32-bit int) */
unsigned ud(a, b) unsigned a, b; { return a / b; }
unsigned um(a, b) unsigned a, b; { return a % b; }

int test(void)
{
	int bad = 0;

	if (ud(4000000000u, 7u) != 571428571u) bad |= 1;
	if (um(4000000000u, 7u) != 3u) bad |= 2;
	if (ud(4294967295u, 1u) != 4294967295u) bad |= 4;
	if (um(4294967295u, 1u) != 0u) bad |= 8;
	if (ud(4294967295u, 2u) != 2147483647u) bad |= 16;
	if (ud(4294967295u, 3000000000u) != 1u) bad |= 32;
	if (um(4294967295u, 3000000000u) != 1294967295u) bad |= 64;
	if (ud(100u, 3000000000u) != 0u || um(100u, 3000000000u) != 100u) bad |= 128;
	if (ud(3000000000u, 3000000000u) != 1u) bad |= 256;
	if (ud(0u, 9u) != 0u) bad |= 512;
	return bad == 0;
}
