/* expect: 1 -- conversions among int, unsigned and pointer keep the outer type's meaning */
int *buf;
int gi;
unsigned gu;
char *gp;

int rsh(i) int i; { return (int)((unsigned)i >> 1); }
int sdiv(u) unsigned u; { return (int)u / 2; }
int ucmp(i, j) int i, j; { return (unsigned)i < (unsigned)j; }
int scmp(u, v) unsigned u, v; { return (int)u < (int)v; }
long wide(i) int i; { return (long)(unsigned)i; }
long swide(u) unsigned u; { return (long)(int)u; }
int stores(p, q) char *p; unsigned q;
{
	buf[0] = (int)p;
	buf[1] = (int)q;
	buf[2] = (unsigned)p;
	return (int)(char *)q;
}
int mix(p) char *p; { return (int)p + (int)gp; }

int test()
{
	int bad, w[3];

	bad = 0;
	if (rsh(-2) != (int)(((unsigned)-2) >> 1)) bad |= 1;
	if (rsh(4) != 2) bad |= 2;
	if (sdiv((unsigned)-2) != -1) bad |= 4;
	if (sdiv((unsigned)10) != 5) bad |= 8;
	if (ucmp(-1, 1) != 0 || ucmp(1, -1) != 1) bad |= 16;
	if (scmp((unsigned)-1, (unsigned)1) != 1 || scmp((unsigned)1, (unsigned)-1) != 0) bad |= 32;
	if (wide(-1) != (long)(unsigned)-1) bad |= 64;
	if (swide((unsigned)-1) != (long)(int)(unsigned)-1) bad |= 128;
	buf = w;
	w[0] = w[1] = w[2] = 0;
	if (stores((char *)7, (unsigned)9) != 9 || w[0] != 7 || w[1] != 9 || w[2] != 7) bad |= 256;
	gp = (char *)100;
	if (mix((char *)20) != 120) bad |= 512;
	return bad ? 2 * bad + 1000 : 1;
}
