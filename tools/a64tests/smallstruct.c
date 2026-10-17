/* expect: 1 -- structures of one and two words assigned: through pointers, between variables, with ++ */
union store { union store *ptr; int dummy; };
struct two { int a, b; };
struct two gl, gm;
union store us, ut;

void cp1(s, t) union store *s, *t; { *t = *s; }
void cp2(s, t) struct two *s, *t; { *t = *s; }

int test(void)
{
	int bad = 0;
	struct two a, b, arr[3], *ap, *bp;
	union store u1, u2, ua[3], *up, *vp;
	int i;

	gm.a = 11; gm.b = -22;
	gl = gm;
	if (gl.a != 11 || gl.b != -22) bad |= 1;
	a.a = 70; a.b = -80;
	cp2(&a, &b);
	if (b.a != 70 || b.b != -80) bad |= 2;
	cp2(&a, &gl);
	if (gl.a != 70 || gl.b != -80 || gm.a != 11) bad |= 4;
	u1.dummy = 1234;
	cp1(&u1, &u2);
	if (u2.dummy != 1234) bad |= 8;
	for (i = 0; i < 3; i++) { arr[i].a = i; arr[i].b = i * 10; ua[i].dummy = i + 5; }
	ap = arr;
	bp = &b;
	*bp = *ap++;
	if (b.a != 0 || b.b != 0 || ap != arr + 1) bad |= 16;
	*bp = *ap++;
	if (b.a != 1 || b.b != 10 || ap != arr + 2) bad |= 32;
	up = ua; vp = &u2;
	*vp = *up++;
	if (u2.dummy != 5 || up != ua + 1) bad |= 64;
	*vp = *up++;
	if (u2.dummy != 6 || up != ua + 2) bad |= 128;
	us.dummy = 99;
	ut = us;
	if (ut.dummy != 99) bad |= 256;
	return bad == 0;
}
