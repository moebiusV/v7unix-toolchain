/* expect: 1 -- K&R-isms in V7 libc: char->pointer, '=' -less initializers, bare return, float constants */
static	char	tab[] {
	3, 5, 7, 11,
};
static	int	itab[] { 100, 200 };

char *args[3];
char gc;
unsigned char gu;

int setargs(a, b)
char a, b;
{
	args[0] = a;			/* char widened to a pointer */
	args[1] = b;
	return 0;
}

void fillp(void)
{
	gc = -2;
	gu = 200;
	args[0] = gc;			/* sign-extends */
	args[1] = gu;			/* zero-extends */
}

static bare(n)
int n;
{
	if (n > 0)
		return;			/* no value: only a warning */
	n = 5;
}

double scale(double x)
{
	return (x + .03) * 10;
}

int test(void)
{
	int bad;
	double d;

	bad = 0;
	if (tab[0] + tab[1] + tab[2] + tab[3] != 26) bad |= 1;
	if (itab[0] + itab[1] != 300) bad |= 2;
	fillp();
	if ((int)args[0] != -2) bad |= 4;
	if ((int)args[1] != 200) bad |= 8;
	bare(1); bare(-1);
	d = scale(1.5);				/* 15.3 */
	if ((int)d != 15) bad |= 16;
	if ((int)(scale(0.2) + .5) != 2) bad |= 32;
	return bad == 0;
}
