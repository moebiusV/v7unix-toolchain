/* expect: 1 -- leaf functions without csv/cret: arguments off sp, rts, mixed with framed callers */
int tab[4];
long lg;

int sq(int x) { return x * x + 1; }
int sel(int a, int b, int c) { return a ? b : c; }
char *idx(char *s, int i) { return s + i; }
int glob(void) { return tab[2]; }
int diff(int a, int b) { return a - b; }
int pick(char *p, int i) { return p[i]; }
long wide(long a, int b) { return a + b; }
double half(double d) { return d * 0.5; }
int cnt(char *s) { int n; n = 0; while (*s++) n++; return n; }		/* needs a register */
int frame(int x) { int a[2]; a[0] = x; a[1] = x + 1; return a[0] + a[1]; }
int chain(int x) { return sq(x) + sel(x, 1, 2); }			/* calls */
int addr(int n) { int *p; p = &n; return *p + 1; }			/* takes an address */

int test(void)
{
	int bad;
	char buf[6];
	double h;

	bad = 0;
	tab[2] = 77;
	buf[0] = 'a'; buf[1] = 'b'; buf[2] = 'c'; buf[3] = 0;
	if (sq(5) != 26) bad |= 1;
	if (sel(1, 10, 20) != 10 || sel(0, 10, 20) != 20) bad |= 2;
	if (*idx(buf, 2) != 'c') bad |= 4;
	if (glob() != 77) bad |= 8;
	if (diff(9, 4) != 5 || diff(4, 9) != -5) bad |= 16;
	if (pick(buf, 1) != 'b') bad |= 32;
	if (wide(70000L, 5) != 70005L) bad |= 64;
	h = half(5.0);
	if ((int)(h * 2) != 5) bad |= 128;
	if (cnt(buf) != 3) bad |= 256;
	if (frame(10) != 21) bad |= 512;
	if (chain(3) != 10 + 1) bad |= 1024;
	if (addr(4) != 5) bad |= 2048;
	return bad == 0;
}
