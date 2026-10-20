/* expect: 1 -- (rN)+ and -(rN) for pointers that are not register variables (made by c2) */
int wcopy(s, t, n) int *s, *t; int n; { while (n--) *t++ = *s++; return 0; }
int bcopy(s, t) char *s, *t; { while (*t++ = *s++) ; return 0; }
int wsum(p, n) int *p; int n; { int s; s = 0; while (n--) s += *p++; return s; }
int rsum(p, n) int *p; int n; { int s; s = 0; p += n; while (n--) s += *--p; return s; }
int rcopy(s, t, n) char *s, *t; int n; { s += n; t += n; while (n--) *--t = *--s; return 0; }
int last(p) int *p; { int a; a = *p++; return a + *p; }
int twice(p, q) int *p, *q; { *p++ = *q++; *p++ = *q++; return (int)(p - q); }
int deref(p) int **p; { int *q; q = *p++; return *q + (int)(p != 0); }

int test()
{
	int bad, i;
	int w1[6], w2[6], *ip;
	char b1[8], b2[8];

	bad = 0;
	for (i = 0; i < 6; i++) { w1[i] = i * 3 + 1; w2[i] = 0; }
	wcopy(w1, w2, 6);
	for (i = 0; i < 6; i++) if (w2[i] != w1[i]) bad |= 1;
	b1[0] = 'a'; b1[1] = 'b'; b1[2] = 'c'; b1[3] = 0;
	bcopy(b1, b2);
	if (b2[0] != 'a' || b2[1] != 'b' || b2[2] != 'c' || b2[3] != 0) bad |= 2;
	if (wsum(w1, 6) != 1 + 4 + 7 + 10 + 13 + 16) bad |= 4;
	if (rsum(w1, 6) != 51) bad |= 8;
	for (i = 0; i < 8; i++) b2[i] = 0;
	rcopy(b1, b2, 3);
	if (b2[0] != 'a' || b2[2] != 'c') bad |= 16;
	if (last(w1) != 1 + 4) bad |= 32;
	w2[0] = w2[1] = 0;
	if (twice(w2, w1) != w2 - w1) bad |= 64;
	if (w2[0] != 1 || w2[1] != 4) bad |= 128;
	ip = &w1[2];
	if (deref(&ip) != 7 + 1) bad |= 256;
	return bad ? 2 * bad + 1000 : 1;
}
