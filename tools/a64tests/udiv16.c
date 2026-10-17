/* expect: 1 -- unsigned divide and remainder by a variable, every branch of the helper (values fit 16 bits) */
unsigned ud(a, b) unsigned a, b; { return a / b; }
unsigned um(a, b) unsigned a, b; { return a % b; }

int test(void)
{
	int bad = 0;

	if (ud(65535u, 1u) != 65535u || um(65535u, 1u) != 0u) bad |= 1;
	if (ud(40000u, 2u) != 20000u || um(40001u, 2u) != 1u) bad |= 2;
	if (ud(65535u, 40000u) != 1u || um(65535u, 40000u) != 25535u) bad |= 4;
	if (ud(40000u, 40000u) != 1u || um(40000u, 40000u) != 0u) bad |= 8;
	if (ud(100u, 40000u) != 0u || um(100u, 40000u) != 100u) bad |= 16;
	if (ud(0u, 40000u) != 0u || um(0u, 40000u) != 0u) bad |= 32;
	if (ud(65535u, 32768u) != 1u || um(65535u, 32768u) != 32767u) bad |= 64;
	if (ud(32767u, 32768u) != 0u || um(32767u, 32768u) != 32767u) bad |= 128;
	if (ud(65535u, 3u) != 21845u || um(65535u, 3u) != 0u) bad |= 256;
	if (ud(60000u, 32767u) != 1u || um(60000u, 32767u) != 27233u) bad |= 512;
	if (ud(7u, 7u) != 1u || ud(6u, 7u) != 0u || um(6u, 7u) != 6u) bad |= 1024;
	if (ud(0u, 1u) != 0u || um(0u, 1u) != 0u) bad |= 2048;
	return bad == 0;
}
