/* expect: 1 -- reads of bit fields (a field extract on the VAX), shifts of an unsigned word that clear bits, fields in memory and in registers */
struct S { unsigned int a : 5; unsigned int b : 3; unsigned int c : 4; unsigned int d : 4; };
struct S s;
unsigned long w = 0x89abcdefUL;

int pf(p) struct S *p; { return p->a * 1 + p->b * 32 + p->c * 7 + p->d * 3; }
unsigned long g1(x) unsigned long x; { return (x << 8) >> 12; }
unsigned long g2(x) unsigned long x; { return (x << 8) >> 8; }
unsigned long g3(x) unsigned long x; { return (x << 12) >> 4; }
unsigned long g4() { return (w << 8) >> 20; }

int test()
{
	struct S t;
	unsigned long x = 0x89abcdefUL;
	unsigned int *u = (unsigned int *)&t;

	*u = 0;
	t.a = 21; t.b = 5; t.c = 9; t.d = 12;
	if (t.a != 21 || t.b != 5 || t.c != 9 || t.d != 12) return 2;
	if (pf(&t) != 21 + 5 * 32 + 9 * 7 + 12 * 3) return 3;
	s.d = 15; s.c = 1; s.b = 7; s.a = 31;
	if (s.d != 15 || s.c != 1 || s.b != 7 || s.a != 31) return 4;
	if (s.d == 3 || s.c == 3) return 5;
	if (g1(x) != ((x << 8) >> 12)) return 6;
	if (g1(x) != 0x000bcdefUL >> 0 && g1(x) != (x & 0x00ffffffUL) >> 4) return 7;
	if (g2(x) != (x & 0x00ffffffUL)) return 8;
	if (g3(x) != ((x << 8) & 0x0ffffffeUL << 0 | 0) && g3(x) != ((x & 0x000fffffUL) << 8)) return 9;
	if (g4() != ((w & 0x00ffffffUL) >> 12)) return 10;
	return 1;
}
