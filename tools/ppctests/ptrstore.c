/* expect: 1 -- stores of constants through off(reg) with the pointer in a register: the value must not take the pointer's register */
struct b { int cnt; char *ptr; int n; long l; };
struct b g;
char text[6];
int f(p, t) struct b *p; char *t;
{
	p->cnt = 5; p->n = 1; p->l = 100000L; p->ptr = t;
	t[0] = 3; t[1] = 4; t[2] = 5;
	return p->n;
}
int test()
{
	int bad = 0;

	if (f(&g, text) != 1) bad |= 1;
	if (g.cnt != 5 || g.n != 1 || g.l != 100000L || g.ptr != text) bad |= 2;
	if (text[0] != 3 || text[1] != 4 || text[2] != 5) bad |= 4;
	return bad ? 1000 + bad : 1;
}
