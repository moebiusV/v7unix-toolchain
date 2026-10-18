/* expect: 1 -- a double parameter updated in place (negate, scale, subtract) and a char/short parameter read narrow from its slot */
double neg(a, p) double a; double *p;
{
	if (a < 0) a = -a;
	*p = a * 2;
	a = a - 0.5;
	a = a * 4;
	return a;
}
double sum(a, b) double a, b;
{
	double t;
	int i;

	t = 0;
	for (i = 0; i < 3; i++) {
		a = a + b;
		t = t + a;
	}
	return t - b;
}
int narrow(c, s) char c; short s;
{
	c = c + 1;
	s = s + c;
	return s + c;
}
int uns(c) unsigned char c; { return c + 1; }

int test()
{
	int bad;
	double d, r;

	bad = 0;
	r = neg(-3.0, &d);
	if (d != 6.0 || r != 10.0) bad |= 2;
	r = neg(2.0, &d);
	if (d != 4.0 || r != 6.0) bad |= 4;
	if (sum(1.0, 2.0) != 3.0 + 5.0 + 7.0 - 2.0) bad |= 8;
	if (narrow(10, 100) != 100 + 11 + 11) bad |= 16;
	if (narrow(127, 0) != 0 + (-128) + (-128)) bad |= 32;
	if (uns(255) != 256) bad |= 64;
	return bad ? 2 * bad + 1000 : 1;
}
