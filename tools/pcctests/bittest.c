/* expect: 1 -- (x & mask) tested against zero, every operand kind */
struct f { int w; char c; unsigned char u; };
struct f g;
int mask;

/* returns a bit per test that behaved wrongly */
int run(register struct f *p, int m)
{
	int bad;

	bad = 0;
	p->w = 05; p->c = 012; p->u = 0200;
	if (p->w & 1)	; else bad |= 1;	/* int, bit set */
	if (p->w & 2)	bad |= 2;		/* int, bit clear */
	if (!(p->w & 4)) bad |= 4;
	if (p->c & 2)	; else bad |= 8;	/* char, bit set */
	if (p->c & 1)	bad |= 16;		/* char, bit clear */
	if (p->u & 0200) ; else bad |= 32;	/* uchar, high bit set */
	if (p->u & 0100) bad |= 64;		/* uchar, bit clear */
	if (p->w & m)	; else bad |= 128;	/* variable mask */
	if (p->c & (m << 2))	bad |= 256;	/* variable, clear */
	if ((p->w & 5) != 5) bad |= 512;	/* compared to nonzero */
	if (!(g.w & 0) == 0) bad |= 1024;	/* x & 0 is zero */
	return bad;
}

int test(void)
{
	mask = 4;
	g.w = 3;
	return run(&g, mask) == 0;
}
