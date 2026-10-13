/* expect: 1 -- leaf functions without a frame pointer: arguments off %esp, early returns, address of an argument */
int add3(int a, int b, int c) { return a + b + c; }
int pick(int a, int b, int c) { if (a) return b; return c; }
int addr(int a, int b) { int *p; p = &b; return *p + a; }
int cnt(char *s) { int n; n = 0; while (*s++) n++; return n; }
struct st { int a, b; };
int fld(struct st *p, int k) { return p->a * k + p->b; }
char chr(int i) { return (char)(i + 1); }
long lng(long a, long b) { return a + b; }

int test(void)
{
	int bad;
	struct st x;

	bad = 0;
	if (add3(1, 2, 3) != 6) bad |= 1;
	if (pick(1, 10, 20) != 10 || pick(0, 10, 20) != 20) bad |= 2;
	if (addr(5, 7) != 12) bad |= 4;
	if (cnt("hello") != 5) bad |= 8;
	x.a = 3; x.b = 4;
	if (fld(&x, 5) != 19) bad |= 16;
	if (chr(64) != 65) bad |= 32;
	if (lng(3000000000u, 1) != 3000000001u) bad |= 64;
	return bad == 0;
}
