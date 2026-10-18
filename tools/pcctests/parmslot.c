/* expect: 1 -- parameters used 0, 1, 2 and many times, modified, and passed on: read from their slots or from registers, never stale */
int sink;

unused(a, b, c) int a, b, c; { return 7; }
once(a, b) int a, b; { return b; }
twice(a, b) int a, b; { return a + b + a; }
modified(n) int n; { n = n + 1; n = n * 2; return n; }
decr(n, s) int n; char *s; { while (--n > 0) s++; return *s; }
many(a, b) int a, b;
{
	int i, t;

	t = 0;
	for (i = 0; i < a; i++)
		t += a + b + i;
	return t + b;
}
passed(a, b) int a, b; { return twice(b, a) + once(a, b); }
dbl(x, y) double x, y; { x = x + y; return x * x > y; }
ptr(p, q) char *p, *q; { *p++ = *q++; *p = *q; return p - q; }
byte(c, d) char c, d; { c = c + d; return c; }

int test()
{
	int bad;
	char buf[8], out[8];

	bad = 0;
	if (unused(1, 2, 3) != 7) bad |= 2;
	if (once(1, 9) != 9) bad |= 4;
	if (twice(2, 3) != 7) bad |= 8;
	if (modified(4) != 10) bad |= 16;
	buf[0] = 'a'; buf[1] = 'b'; buf[2] = 'c'; buf[3] = 0;
	if (decr(3, buf) != 'c') bad |= 32;
	if (many(3, 2) != 2 + 3 * 5 + 3) bad |= 64;
	if (passed(2, 3) != twice(3, 2) + 3) bad |= 128;
	if (!dbl(2.0, 3.0) || dbl(-1.0, 2.0)) bad |= 256;
	out[0] = 'x'; out[1] = 'y';
	buf[0] = 'p'; buf[1] = 'q';
	ptr(out, buf);
	if (out[0] != 'p' || out[1] != 'q') bad |= 512;
	if (byte(100, 27) != 127) bad |= 1024;
	return bad ? 2 * bad + 1000 : 1;
}
