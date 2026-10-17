/* expect: 1 -- struct assignment from a named object, a local, a parameter and a pointer */
struct s1 { char a; };
struct s2 { char a, b; };
struct s4 { char a, b, c, d; };
struct s8 { char a[8]; };
struct s12 { char a[12]; };
struct big { int v[30]; };
struct s1 g1, h1; struct s2 g2, h2; struct s4 g4, h4; struct s8 g8, h8; struct s12 g12, h12;
struct big gb, hb;

int byval(p) struct big p; { struct big l; l = p; return l.v[0] + l.v[29]; }
int viaptr(p, q) struct big *p, *q; { *p = *q; return p->v[3]; }
struct big ret(p) struct big *p; { return *p; }

int test()
{
	int bad, i;
	struct big lb, lc;
	struct s12 l12;

	bad = 0;
	g1.a = 1; h1 = g1; if (h1.a != 1) bad |= 1;
	g2.a = 2; g2.b = 3; h2 = g2; if (h2.a != 2 || h2.b != 3) bad |= 2;
	g4.d = 4; h4 = g4; if (h4.d != 4) bad |= 4;
	g8.a[7] = 8; h8 = g8; if (h8.a[7] != 8) bad |= 8;
	g12.a[11] = 12; h12 = g12; l12 = h12; if (l12.a[11] != 12) bad |= 16;
	for (i = 0; i < 30; i++) gb.v[i] = i + 1;
	lb = gb; hb = lb; if (hb.v[29] != 30 || lb.v[0] != 1) bad |= 32;
	if (byval(gb) != 31) bad |= 64;
	if (viaptr(&lc, &gb) != 4) bad |= 128;
	lc = ret(&hb); if (lc.v[10] != 11) bad |= 256;
	lb = lc; if (lb.v[29] != 30) bad |= 512;
	return bad ? bad : 1;
}
