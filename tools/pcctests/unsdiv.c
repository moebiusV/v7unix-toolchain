/* expect: 1 -- unsigned divide/remainder survives a cast to int in return */
int f(n) unsigned n; { return n / 4; }
int g(n) unsigned n; { return n % 8; }
int h(n) unsigned n; { return (int)(n / 2) + 1; }

int test(void)
{
	int bad;

	bad = 0;
	if (f(40000u) != 10000) bad |= 1;
	if (g(40001u) != 1) bad |= 2;
	if (f(65535u) != 16383) bad |= 4;
	if (h(65535u) != 32768) bad |= 8;
	return bad == 0;
}
