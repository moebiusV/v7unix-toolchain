/* expect: 1 -- incb/decb: wrap-around, neighbours, indexed and pointer targets, value uses */
/* skip: -xssa (ccom crashes in zzzcode G on ++*q++ under SSA; also on the committed compiler) */
struct s { char a; unsigned char b; char c; int n; };
struct s g;
char gc;
unsigned char gu;
char tab[6];
unsigned char utab[6];

int run(register struct s *p, register char *q, int i)
{
	int bad, v;

	bad = 0;
	g.a = 127; g.b = 255; g.c = 5; g.n = 1000;
	p->a++;				/* 127 -> -128 */
	p->b++;				/* 255 -> 0 */
	if (p->a != -128 || p->b != 0 || p->c != 5 || p->n != 1000) bad |= 1;
	p->a--; p->b--;			/* back: 127, 255 */
	if (p->a != 127 || p->b != 255 || p->c != 5 || p->n != 1000) bad |= 2;
	--p->c; ++p->c; ++p->c;
	if (p->c != 6 || p->a != 127) bad |= 4;

	gc = -1; gu = 0;
	gc++; gu--;
	if (gc != 0 || gu != 255) bad |= 8;
	++gc; --gu; --gc; ++gc;
	if (gc != 1 || gu != 254) bad |= 16;

	for (v = 0; v < 6; v++) tab[v] = v, utab[v] = 250 + v;
	tab[i]++; tab[i + 1]--; utab[i]++; utab[i + 1]--;
	if (tab[2] != 3 || tab[3] != 2 || utab[2] != 253 || utab[3] != 252) bad |= 32;
	if (tab[1] != 1 || tab[4] != 4 || utab[1] != 251 || utab[4] != 254) bad |= 64;

	q = tab + 4;
	(*q)--; ++*q; ++*q;
	if (tab[4] != 5) bad |= 128;
	++*--q;				/* tab[3] */
	if (q != tab + 3 || tab[3] != 3) bad |= 256;
	++*q++;				/* tab[3], then step */
	if (q != tab + 4 || tab[3] != 4) bad |= 512;

	/* the value is used: not a statement of its own */
	gc = 10;
	v = gc++;
	if (v != 10 || gc != 11) bad |= 1024;
	v = --gc;
	if (v != 10 || gc != 10) bad |= 2048;
	if (gc-- != 10 || gc != 9) bad |= 4096;
	return bad;
}

int test(void)
{
	return run(&g, tab, 2) == 0;
}
