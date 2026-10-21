/* expect: 1 -- memcpy of a constant size between fields of a structure reached by a pointer, and the value it returns */
extern void *memcpy(void *, const void *, unsigned long);
struct ctx { char a[8]; char h[8]; char c[8]; char t[4]; int n; };

void copyfld(struct ctx *p) { memcpy(p->h, p->c, 8); }
void *copyret(struct ctx *p, struct ctx *q) { return memcpy(p->t, q->t + 1, 2); }
void shift(struct ctx *p, int k) { memcpy(p->a + 2, p->c + k, 4); }

int test()
{
	struct ctx x, y;
	int i;
	void *r;

	for (i = 0; i < 8; i++) {
		x.a[i] = i; x.h[i] = 10 + i; x.c[i] = 20 + i;
	}
	for (i = 0; i < 4; i++) { x.t[i] = 40 + i; y.t[i] = 50 + i; }
	copyfld(&x);
	for (i = 0; i < 8; i++)
		if (x.h[i] != 20 + i || x.c[i] != 20 + i) return 2;
	r = copyret(&x, &y);
	if (r != (void *)x.t || x.t[0] != 51 || x.t[1] != 52 || x.t[2] != 42) return 3;
	shift(&x, 1);
	if (x.a[0] != 0 || x.a[1] != 1 || x.a[2] != 21 || x.a[5] != 24 || x.a[6] != 6) return 4;
	return 1;
}
