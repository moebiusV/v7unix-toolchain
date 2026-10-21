/* expect: 1 -- calls of inline functions that return nothing, or nothing useful: empty, with an early return, in a branch, as a statement and in a comma list */
static int g, h;
static inline void nothing() { }
static inline void bump() { g++; }
static inline void early(x) int x; { if (x) return; h++; }
static inline int valued() { h += 10; return 5; }

int f(c) int c;
{
	if (c & 1) { nothing(); g += 100; }
	if (c & 2) { bump(); nothing(); }
	early(c);
	nothing(), bump();
	(void)valued();
	return c;
}

int test()
{
	f(0);
	if (g != 1 || h != 11) return 2;
	f(3);
	if (g != 1 + 100 + 1 + 1 || h != 11 + 10) return 3;
	return 1;
}
