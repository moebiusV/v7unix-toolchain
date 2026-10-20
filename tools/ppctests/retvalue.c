/* expect: 1 -- values returned in r0 / r0:r1 survive the optimizer's dead-register logic; a function that returns nothing may use r0 freely */
double dret(a) double a; { if (a < 0) return -a; return a * 2; }
int iret(a, b) int a, b; { if (a) return b; return a; }
long lret(a) long a; { return a + 1; }
struct two { int x, y; };
struct two sret(a) int a; { struct two t; t.x = a; t.y = a + 1; return t; }
int g;
nothing(p, q) int *p, *q; { g = *p; *q = g; if (g) g = 3; }
int usesnothing(p, q) int *p, *q; { nothing(p, q); return g + 1; }

int test()
{
	int bad, a, b;
	struct two t;

	bad = 0;
	if (dret(-2.5) != 2.5) bad |= 2;
	if (dret(1.5) != 3.0) bad |= 4;
	if (iret(0, 7) != 0 || iret(2, 7) != 7) bad |= 8;
	if (lret(100000L) != 100001L) bad |= 16;
	t = sret(4);
	if (t.x != 4 || t.y != 5) bad |= 32;
	a = 5; b = 0;
	if (usesnothing(&a, &b) != 4 || b != 5) bad |= 64;
	return bad ? 2 * bad + 1000 : 1;
}
