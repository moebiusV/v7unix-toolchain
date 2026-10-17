/* expect: 1 -- ((long)n << 15) / o in a function with every callee-saved register busy (libc's monitor) */
int pa, pb, pd, pcnt;
char *pc;
int sbufv[8];

profil(a, b, c, d) int *a; int b; char *c; int d;
{
	pa = (int)a - (int)sbufv; pb = b; pc = c; pd = d; pcnt++;
	return 0;
}

monitor(lowpc, highpc, buf, bufsiz, cntsiz)
char *lowpc, *highpc;
int *buf, bufsiz;
{
	register o;
	static *sbuf, ssiz;

	ssiz = bufsiz;
	buf[0] = (int)lowpc;
	buf[1] = (int)highpc;
	buf[2] = cntsiz;
	sbuf = buf;
	buf += 3*(cntsiz+1);
	bufsiz -= 3*(cntsiz+1);
	if (bufsiz<=0)
		return;
	o = ((highpc - lowpc)>>1) & 077777;
	if(bufsiz < o)
		o = ((long)bufsiz<<15) / o;
	else
		o = 077777;
	profil(buf, bufsiz<<1, lowpc, o<<1);
}

int test()
{
	int bad, i;

	bad = 0;
	/* range 2000 bytes -> o = 1000; bufsiz 8 - 3 = 5 < 1000: scale = (5 << 15) / 1000 = 163 */
	monitor((char *)100, (char *)2100, sbufv, 8, 0);
	if (pcnt != 1 || pb != 10 || pd != 326) bad |= 1;
	/* a buffer bigger than the range: the scale is 077777 */
	monitor((char *)100, (char *)104, sbufv, 8, 0);
	if (pcnt != 2 || pb != 10 || pd != 2 * 077777) bad |= 2;
	/* several ranges and buffers against the same arithmetic done in a long */
	for (i = 0; i < 6; i++) {
		int bs, rng, o2, want;

		bs = 8 + i * 37;
		rng = 400 + i * 1500;
		o2 = ((rng) >> 1) & 077777;
		if (bs - 3 < o2)
			want = (int)(((long)(bs - 3) << 15) / o2);
		else
			want = 077777;
		monitor((char *)100, (char *)(100 + rng), sbufv, bs, 0);
		if (pd != want * 2) bad |= 8;
	}
	/* bufsiz too small after the counters: nothing */
	monitor((char *)100, (char *)2100, sbufv, 3, 0);
	if (pcnt != 8) bad |= 4;
	return bad ? 2 * bad + 1000 : 1;
}
