/* expect: 1 -- comparing bytes in memory with constants: signedness matters */
struct b { char c; unsigned char u; int i; };
struct b g;

int run(register struct b *p)
{
	int bad;

	bad = 0;
	p->c = -1; p->u = 200; p->i = -1;
	if (!(p->c < 0))	bad |= 1;	/* signed char is negative */
	if (!(p->c == -1))	bad |= 2;
	if (p->c > 0)		bad |= 4;
	if (!(p->u > 100))	bad |= 8;	/* unsigned char is not */
	if (!(p->u == 200))	bad |= 16;
	if (p->u < 0)		bad |= 32;
	if (p->c == 255)	bad |= 64;	/* -1 != 255 as ints */
	if (!(p->u == 0310))	bad |= 128;
	if (!(p->c != 5))	bad |= 256;
	if (!(p->c >= -1))	bad |= 512;
	if (!(p->u <= 200))	bad |= 1024;
	if (p->u != (unsigned char)200) bad |= 2048;
	p->c = 127; p->u = 0;
	if (!(p->c > 126))	bad |= 4096;
	if (p->u != 0)		bad |= 8192;
	return bad;
}

int test(void)
{
	return run(&g) == 0;
}
