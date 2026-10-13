/* many small leaf calls */
int add3(int a, int b, int c) { return a + b + c; }
int pick(int a, int b) { if (a & 8) return b; return a; }
int lo(char *s) { return s[0] + s[1]; }
int test(void)
{
	int i, s;
	char t[2];

	t[0] = 1; t[1] = 2;
	s = 0;
	for (i = 0; i < 300; i++) {
		s += add3(i, 1, 2);
		s += pick(i, 3);
		s += lo(t);
	}
	return s;
}
