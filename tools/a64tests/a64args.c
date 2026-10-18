/* expect: 1 -- arguments in registers and on the stack, mixed integer and floating, recursion, calls through pointers */
int sum12(a, b, c, d, e, f, g, h, i, j, k, l)
int a, b, c, d, e, f, g, h, i, j, k, l;
{ return a + 2*b + 3*c + 4*d + 5*e + 6*f + 7*g + 8*h + 9*i + 10*j + 11*k + 12*l; }

long lsum10(a, b, c, d, e, f, g, h, i, j)
long a, b, c, d, e, f, g, h, i, j;
{ return a - b + c - d + e - f + g - h + i - j; }

double dsum10(a, b, c, d, e, f, g, h, i, j)
double a, b, c, d, e, f, g, h, i, j;
{ return a + 2*b + 3*c + 4*d + 5*e + 6*f + 7*g + 8*h + 9*i + 10*j; }

double mix(int a, double x, long b, float y, char c, double z, short *p)
{ return a + x + b + y + c + z + *p; }

int manyd(a, b, c, d, e, f, g, h, i, n, x, m)
double a, b, c, d, e, f, g, h, i; int n; double x; long m;
{ return (int)(a + b + c + d + e + f + g + h + i + x) + n + (int)m; }

int fib(n) int n; { return n < 2 ? n : fib(n - 1) + fib(n - 2); }

int apply(f, x) int (*f)(); int x; { return f(x); }
int dbl(x) int x; { return 2 * x; }
int inc(x) int x; { return x + 1; }
char chr(c) char c; { return c + 1; }
short sh(s) short s; { return s * 2; }
unsigned char uc(c) unsigned char c; { return c + 200; }

int deep(n, a, b, c, d, e, f, g, h, i, j) int n, a, b, c, d, e, f, g, h, i, j;
{
	if (n == 0)
		return a + b + c + d + e + f + g + h + i + j;
	return deep(n - 1, b, c, d, e, f, g, h, i, j, a);
}

int test()
{
	int bad = 0;
	short sv = 7;
	int (*tab[2])();

	if (sum12(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12) != 650) bad |= 1;
	if (lsum10(10L, 1L, 20L, 2L, 30L, 3L, 40L, 4L, 50L, 5L) != 135) bad |= 2;
	if (dsum10(1., 2., 3., 4., 5., 6., 7., 8., 9., 10.) != 385.) bad |= 4;
	if (mix(1, 2.5, 3L, 4.5f, 'a', 0.5, &sv) != 1 + 2.5 + 3 + 4.5 + 97 + 0.5 + 7) bad |= 8;
	if (manyd(1., 2., 3., 4., 5., 6., 7., 8., 9., 10, 0.5, 100L) != 45 + 10 + 100) bad |= 16;
	if (fib(15) != 610) bad |= 32;
	tab[0] = dbl; tab[1] = inc;
	if (apply(dbl, 21) != 42 || apply(inc, 9) != 10 || tab[1](tab[0](5)) != 11) bad |= 64;
	if (chr(127) != -128 || sh(20000) != -25536 || uc(100) != 44) bad |= 128;
	if (deep(10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10) != 55) bad |= 256;
	return bad ? 2 * bad + 1000 : 1;
}
