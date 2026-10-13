/* expect: 1 -- long +/- constants: carry and borrow across the words */
long a, b;

int test(void)
{
	int bad;

	bad = 0;
	a = 65535L; a += 1; if (a != 65536L) bad |= 1;
	a = 65536L; a -= 1; if (a != 65535L) bad |= 2;
	a = 70000L; a += 65535; if (a != 135535L) bad |= 4;
	a = 70000L; a -= 65535; if (a != 4465L) bad |= 8;
	a = 5L; a += -1; if (a != 4L) bad |= 16;
	a = 5L; a -= -1; if (a != 6L) bad |= 32;
	a = 0L; a -= 1; if (a != -1L) bad |= 64;
	a = -1L; a += 1; if (a != 0L) bad |= 128;
	a = 100000L; a += -70000L; if (a != 30000L) bad |= 256;
	a = 100000L; a -= -70000L; if (a != 170000L) bad |= 512;
	a = 1L; a += 0; if (a != 1L) bad |= 1024;
	a = 65535L; b = a + 65535; if (b != 131070L) bad |= 2048;
	a = 3L; b = a - 70000L; if (b != -69997L) bad |= 4096;
	return bad == 0;
}
