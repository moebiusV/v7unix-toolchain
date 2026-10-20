/* expect: 1 -- long x *= e, x /= e, x %= e in memory (global, pointer, parameter) */
long g, h;

long param(t, d) long t; long d;
{
	t += 30;
	t /= 60;
	t *= d;
	t %= 1000L;
	return t;
}

long viaptr(p, d) long *p; long d;
{
	*p *= d;
	*p /= 3L;
	*p %= 100000L;
	return *p;
}

int test()
{
	int bad;
	long l;

	bad = 0;
	if (param(3600L, 7L) != 420L) bad |= 1;	/* (3630/60=60)*7=420; 420 % 1000 */
	g = 1000L; g *= 123L; g /= 7L; g %= 1000L;
	if (g != 571L) bad |= 2;		/* 123000/7=17571; 17571%1000=571 */
	h = -100000L; h /= 7L;
	if (h != -14285L) bad |= 4;
	h = -100000L; h %= 7L;
	if (h != -5L) bad |= 8;
	l = 70000L;
	if (viaptr(&l, 5L) != 16666L) bad |= 16;	/* 350000/3=116666; %100000 */
	g = 123456L; g *= g;
	if (g != 123456L * 123456L) bad |= 32;
	return bad ? 2 * bad + 1000 : 1;
}
