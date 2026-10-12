/* expect: 21 -- a loop-carried swap must survive SSA phi removal */
int test()
{
	int a, b, t, n;
	a = 1; b = 2; n = 1;
	while (n-- > 0) { t = a; a = b; b = t; }
	return a * 10 + b;
}
