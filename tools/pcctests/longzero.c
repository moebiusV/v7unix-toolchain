/* expect: 1 -- long compared with 0, all relations, in both polarities */
long z, p, n, big, small;

int cmpz(void)
{
	int r;

	r = 0;
	if (z < 0) r |= 1;   if (z <= 0) r |= 2;   if (z > 0) r |= 4;
	if (z >= 0) r |= 8;  if (z == 0) r |= 16;  if (z != 0) r |= 32;
	return r;
}

int test(void)
{
	int bad;

	bad = 0;
	z = 0L;        if (cmpz() != (2|8|16)) bad |= 1;
	z = 1L;        if (cmpz() != (4|8|32)) bad |= 2;
	z = -1L;       if (cmpz() != (1|2|32)) bad |= 4;
	z = 65536L;    if (cmpz() != (4|8|32)) bad |= 8;		/* low word 0 */
	z = -65536L;   if (cmpz() != (1|2|32)) bad |= 16;
	z = 32768L;    if (cmpz() != (4|8|32)) bad |= 32;		/* low word negative */
	z = 2147483647L; if (cmpz() != (4|8|32)) bad |= 64;
	z = -2147483647L - 1; if (cmpz() != (1|2|32)) bad |= 128;
	return bad == 0;
}
