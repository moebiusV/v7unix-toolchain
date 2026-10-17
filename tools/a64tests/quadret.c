/* expect: 1 -- r1 is live at ret only in a file with a double or long long return */
double half(x) double x; { return x / 2; }
long long ll(a) long long a; { return a + 1; }
char *ms;
int adv() { if (*ms++ == 0x2c) return 1; return 0; }
int cnt;
int bump(i) int i; { cnt++; if (i) cnt++; return cnt; }
double dsum(a, b) double a, b; { double r; r = a + b; return r; }

int test()
{
	int bad;
	char buf[4];

	bad = 0;
	if (half(5.0) != 2.5) bad |= 1;
	if (ll(4294967295LL) != 4294967296LL) bad |= 2;
	buf[0] = 0x2c; buf[1] = 0x2c; buf[2] = 0; ms = buf;
	if (adv() != 1 || adv() != 1 || adv() != 0 || ms != buf + 3) bad |= 4;
	cnt = 0;
	if (bump(1) != 2 || bump(0) != 3) bad |= 8;
	if (dsum(1.5, 2.25) != 3.75) bad |= 16;
	return bad ? bad : 1;
}
