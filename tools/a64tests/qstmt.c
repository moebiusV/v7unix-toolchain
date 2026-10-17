/* expect: 1 -- ?: used as a statement: the arms run for effect, only one of them */
int a, b, calls;
int f(v) int v; { calls++; return v; }

int test(void)
{
	int bad = 0, i;
	char buf[4], *p;

	a = b = 0;
	i = 1;
	i ? (a = 5) : (b = 6);
	if (a != 5 || b != 0) bad |= 1;
	i = 0;
	i ? (a = 7) : (b = 8);
	if (a != 5 || b != 8) bad |= 2;
	calls = 0;
	i ? f(1) : f(2);
	if (calls != 1) bad |= 4;
	p = buf;
	i = 1;
	i ? (*p++ = 'x') : f(3);
	i = 0;
	i ? (*p++ = 'y') : (*p++ = 'z');
	if (p != buf + 2 || buf[0] != 'x' || buf[1] != 'z') bad |= 8;
	i = 2;
	(i > 1 ? (i > 5 ? (a = 1) : (a = 2)) : (a = 3));
	if (a != 2) bad |= 16;
	return bad == 0;
}
