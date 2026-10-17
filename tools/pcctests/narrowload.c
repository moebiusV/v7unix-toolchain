/* expect: 1 -- a char made from an int in memory is its low byte */
int gi;
unsigned gu;

int lowc(v) int v; { char c; c = v; return c; }
int lowg() { char c; c = gi; return c; }
int lowu() { char c; c = gu; return c; }
int lowb(p) int *p; { char c; c = *p; return c; }
int ulow(v) int v; { return (char)v & 0377; }

int test()
{
	int bad;
	int a[2];

	bad = 0;
	if (lowc(0x1ff) != -1 || lowc(0x17f) != 127 || lowc(-129) != 127) bad |= 1;
	gi = 0x280; if (lowg() != -128) bad |= 2;
	gu = 0xfff1; if (lowu() != -15) bad |= 4;
	a[0] = 0x1234; a[1] = -2;
	if (lowb(a) != 0x34 || lowb(a + 1) != -2) bad |= 8;
	if (ulow(0x1ff) != 255 || ulow(-1) != 255) bad |= 16;
	return bad ? bad : 1;
}
