/* expect: 1 -- &a[i] as a value, for several element sizes */
int ia[10];
short sa[10];
long la[10];
double da[10];
char ca[10];
struct s { int a, b, c; } sv[5];

int *pi(int i) { return &ia[i]; }
short *ps(int i) { return &sa[i]; }
long *pl(int i) { return &la[i]; }
double *pd(int i) { return &da[i]; }
char *pc(int i) { return &ca[i]; }
struct s *pst(int i) { return &sv[i]; }
int *pi2(int i) { return &ia[i + 2]; }
unsigned *pu(unsigned i) { return (unsigned *)&ia[i]; }

int test(void)
{
	int i, bad = 0;

	for (i = 0; i < 5; i++) {
		if (pi(i) != ia + i) bad |= 1;
		if (ps(i) != sa + i) bad |= 2;
		if (pl(i) != la + i) bad |= 4;
		if (pd(i) != da + i) bad |= 8;
		if (pc(i) != ca + i) bad |= 16;
		if (pst(i) != sv + i) bad |= 32;
		if (pi2(i) != ia + i + 2) bad |= 64;
		if (pu(i) != (unsigned *)ia + i) bad |= 128;
	}
	*pi(3) = 42;
	if (ia[3] != 42) bad |= 256;
	return bad == 0;
}
