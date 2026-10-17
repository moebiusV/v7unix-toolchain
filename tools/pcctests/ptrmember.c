/* expect: 1 -- p->m++, p->m op= v and *p->q++ through a parameter and a global pointer: the pointer is read once */
struct b { int cnt; char *ptr; int n; long l; };
struct b g, *gb;
char text[6];
int loads;

struct b *pick() { loads++; return &g; }

int viaparam(iop) struct b *iop;
{
	int r;

	iop->n++;
	iop->n += 3;
	iop->n |= 8;
	iop->l += 2L;
	--iop->cnt;
	r = *iop->ptr++;
	r += *iop->ptr++;
	return r;
}

int viaglobal()
{
	gb->n++;
	gb->n -= 2;
	gb->cnt <<= 1;
	return gb->n;
}

int test()
{
	int bad;

	bad = 0;
	g.cnt = 5; g.n = 1; g.l = 100000L; g.ptr = text;
	text[0] = 3; text[1] = 4;
	if (viaparam(&g) != 7) bad |= 1;
	if (g.n != 13 || g.cnt != 4 || g.l != 100002L || g.ptr != text + 2) bad |= 2;
	gb = &g;
	if (viaglobal() != 12 || g.cnt != 8) bad |= 4;
	pick()->n++;
	pick()->n += 5;
	if (loads != 2 || g.n != 18) bad |= 8;
	return bad ? 2 * bad + 1000 : 1;
}
