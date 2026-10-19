/* expect: 1 -- automatic arrays and structs initialised with zeros: every byte is zero, the neighbours are untouched */
int dirty(n) int n;
{
	char junk[400];
	int i;

	for (i = 0; i < 400; i++)
		junk[i] = 0x55;
	return junk[n];
}

struct g { char pre[4]; char a[17]; char post[4]; };
struct h { char pre[4]; char a[100]; char mid[3]; char b[33]; char post[4]; };

int nz(p, n) char *p; int n;
{
	int i, c = 0;

	for (i = 0; i < n; i++)
		if (p[i])
			c++;
	return c;
}

int t1() { char a[16] = { 0 }; return nz(a, 16); }
int t2() { char a[17] = { 0 }; return nz(a, 17); }
int t3() { char a[31] = { 0 }; return nz(a, 31); }
int t4() { char a[64] = { 0 }; return nz(a, 64); }
int t5() { char a[100] = { 1, 2, 3 }; return nz(a, 100) - 3; }
int t6() { short a[40] = { 0 }; return nz((char *)a, 80); }
int t7() { int a[30] = { 0 }; return nz((char *)a, 30 * sizeof(int)); }
int t8() { long a[10] = { 0 }; return nz((char *)a, 10 * sizeof(long)); }
int t9() { char a[50] = "ab"; return nz(a, 50) - 2; }
int t11()
{
	struct g x = { { 1, 1, 1, 1 }, { 0 }, { 2, 2, 2, 2 } };

	if (x.pre[0] != 1 || x.pre[3] != 1 || x.post[0] != 2 || x.post[3] != 2)
		return 1;
	return nz(x.a, 17);
}
int t12()
{
	struct h x = { { 1, 1, 1, 1 }, { 5 }, { 3, 3, 3 }, { 0 }, { 2, 2, 2, 2 } };

	if (x.pre[3] != 1 || x.mid[0] != 3 || x.mid[2] != 3 || x.post[0] != 2)
		return 1;
	return nz(x.a, 100) - 1 + nz(x.b, 33);
}
int t13() { struct h x = { { 0 } }; return nz((char *)&x, sizeof x); }

int test()
{
	int r;

	dirty(1); if ((r = t1()) != 0) return 10 + r;
	dirty(2); if ((r = t2()) != 0) return 30 + r;
	dirty(3); if ((r = t3()) != 0) return 50 + r;
	dirty(4); if ((r = t4()) != 0) return 70 + r;
	dirty(5); if ((r = t5()) != 0) return 90 + r;
	dirty(6); if ((r = t6()) != 0) return 110 + r;
	dirty(7); if ((r = t7()) != 0) return 130 + r;
	dirty(8); if ((r = t8()) != 0) return 150 + r;
	dirty(9); if ((r = t9()) != 0) return 170 + r;
	dirty(10); if ((r = t11()) != 0) return 190 + r;
	dirty(11); if ((r = t12()) != 0) return 210 + r;
	dirty(12); if ((r = t13()) != 0) return 230 + r;
	return 1;
}
