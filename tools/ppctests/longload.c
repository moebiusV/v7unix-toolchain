/* expect: 1 -- a long loaded through a pointer into a register pair that holds the pointer */
struct two { unsigned long hi, lo; };
struct two g;
long lo(a) struct two *a; { return a->lo; }
long hi(a) struct two *a; { return a->hi; }
long deref(p) long *p; { return *p; }
unsigned long uderef(p) unsigned long *p; { return p[1]; }
long idx(a, i) long *a; int i; { return a[i]; }

int test()
{
	int bad;
	long v[3];

	bad = 0;
	g.hi = 0x11223344L; g.lo = 0x55667788L;
	if (lo(&g) != 0x55667788L) bad |= 2;
	if (hi(&g) != 0x11223344L) bad |= 4;
	v[0] = 100000L; v[1] = -7L; v[2] = 0x7ffffff0L;
	if (deref(v) != 100000L || deref(v + 1) != -7L) bad |= 8;
	if (uderef((unsigned long *)v) != (unsigned long)-7L) bad |= 16;
	if (idx(v, 2) != 0x7ffffff0L || idx(v, 0) != 100000L) bad |= 32;
	return bad ? 2 * bad + 1000 : 1;
}
