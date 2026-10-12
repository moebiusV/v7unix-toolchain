/* expect: 1 -- counters tested through their own condition codes: --x >= 0, x++ == 0, byte stores through pointers */
struct f { char *p; int cnt; char buf[8]; };
struct f io;
int gcnt;
char *gp;
char store[8];

int putc1(register struct f *fp, register int c)
{
	return --fp->cnt >= 0 ? (*fp->p++ = c) : -1;
}

int countdown(void)
{
	int n;

	n = 0;
	gcnt = 3;
	while (--gcnt >= 0)
		n++;
	return n * 10 + gcnt;		/* 3 passes, gcnt ends at -1 */
}

int test(void)
{
	int bad, i, r;

	bad = 0;
	io.p = io.buf; io.cnt = 3;
	r = putc1(&io, 'a') + putc1(&io, 'b') + putc1(&io, 'c');
	if (r != 'a' + 'b' + 'c') bad |= 1;
	if (io.cnt != 0 || io.p != io.buf + 3) bad |= 2;
	if (putc1(&io, 'd') != -1) bad |= 4;			/* full */
	if (io.cnt != -1 || io.p != io.buf + 3) bad |= 8;
	if (io.buf[0] != 'a' || io.buf[1] != 'b' || io.buf[2] != 'c') bad |= 16;
	if (countdown() != 29) bad |= 32;
	gcnt = 0;
	if (++gcnt > 0) ; else bad |= 64;
	if (gcnt-- != 1) bad |= 128;
	if (--gcnt != -1) bad |= 256;
	gp = store;
	for (i = 0; i < 5; i++) *gp++ = 'x' + i;
	if (store[4] != '|' || gp != store + 5) bad |= 512;
	return bad == 0;
}
