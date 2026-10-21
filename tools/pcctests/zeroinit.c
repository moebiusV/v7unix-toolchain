/* expect: 1 -- an automatic struct or array initialized with zeros only (and padding) is cleared whole; inline functions that do it; a function used only in dead code */
struct inner { char c; long l; char buf[40]; };
struct outer { int id; union { struct inner a; char raw[100]; } u; long tail[10]; };
static inline struct outer mk(void) { const struct outer v = { 0, { { 0 } } }; return v; }
static inline long dead(long x) { return x * 3 + (x >> 4); }
static const unsigned short det = { 0x100 };

long probe(void)
{
	struct outer o = { 0, { { 0 } } };
	long s = 0;
	int i;
	unsigned char *p = (unsigned char *)&o;

	for (i = 0; i < (int)sizeof o; i++) s += p[i];
	return s;
}

int test()
{
	struct outer a;
	long z[20] = { 0 };
	unsigned char *p;
	int i;

	a = mk();
	p = (unsigned char *)&a.u;
	for (i = 0; i < 100; i++) if (p[i]) return 2;
	if (a.id != 0 || a.tail[9] != 0) return 3;
	for (i = 0; i < 20; i++) if (z[i]) return 4;
	if (probe() != 0) return 5;
	if (*(unsigned char *)&det == 0x01) return dead(5);
	return 1;
}
