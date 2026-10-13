/* expect: 1 -- p++ and --p as call arguments, in registers and not */
int log[8];
int n;

sink(a, b, c)
{
	log[n++] = a * 100 + b * 10 + c;
}

char buf[8];

int test(void)
{
	register char *p;
	char *q;
	register int *ip;
	int k = 5, bad = 0;

	n = 0;
	p = buf;
	q = buf;
	sink(1, p++ - buf, 3);
	sink(1, p++ - buf, 3);
	sink(2, q++ - buf, 4);
	sink(2, --q - buf, 4);
	ip = log;
	sink(3, ip++ - log, k++);
	sink(3, ++ip - log, k--);
	if (log[0] != 103 || log[1] != 113) bad |= 1;
	if (log[2] != 204 || log[3] != 204) bad |= 2;
	if (log[4] != 305 || log[5] != 326) bad |= 4;
	if (p != buf + 2 || q != buf) bad |= 8;
	return bad == 0;
}
