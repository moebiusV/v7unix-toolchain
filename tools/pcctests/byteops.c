/* expect: 1 -- read-modify-write and stores on bytes */
struct b { char c; unsigned char u; char d; int i; };
struct b g;
char gc;
unsigned char gu;

int run(register struct b *p, int n)
{
	int bad, v;

	bad = 0;
	p->c = 0; p->u = 0; p->d = 0x55;
	p->c |= 0200; p->u |= 0200;			/* high bit */
	if (p->c != -128) bad |= 1;
	if (p->u != 128) bad |= 2;
	p->c |= 1; p->u |= 0x7f;
	if (p->c != -127) bad |= 4;
	if (p->u != 255) bad |= 8;
	p->u &= ~0200;					/* bic, high bit */
	if (p->u != 0x7f) bad |= 16;
	p->c &= 0x0f;					/* clears the sign bit too */
	if (p->c != 1) bad |= 32;
	p->d &= ~0x11;
	if (p->d != 0x44) bad |= 64;
	if (p->i != 0) bad |= 128;			/* neighbors untouched */

	p->c = 3;
	p->c += 5; p->c -= 2; p->c ^= 1;
	if (p->c != 7) bad |= 256;
	p->u = 250; p->u += 10;				/* wraps */
	if (p->u != 4) bad |= 512;

	p->c = n;					/* int to byte */
	if (p->c != 44) bad |= 1024;			/* n == 300 */
	p->u = n + 1;
	if (p->u != 45) bad |= 2048;

	v = (p->d |= 0x80);				/* the value is used */
	if (v != -60) bad |= 4096;
	v = (p->u &= 0xf);
	if (v != 13) bad |= 8192;
	if ((p->c |= 0100) & 0100) ; else bad |= 16384;

	gc = 1; gc |= 4; gc &= ~1;			/* named bytes */
	gu = 0xff; gu &= 0xf0; gu |= 0x1;
	if (gc != 4) bad |= 1;
	if (gu != 0xf1) bad |= 2;
	return bad;
}

int test(void)
{
	return run(&g, 300) == 0;
}
